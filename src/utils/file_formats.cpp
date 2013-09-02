#include "file_formats.h"

PilatusFile::~PilatusFile()
{
    
}
PilatusFile::PilatusFile()
{
    srchrad_sugg_low = std::numeric_limits<float>::max();
    srchrad_sugg_high = std::numeric_limits<float>::min();
    max_counts = 0;
    STATUS_OK = 0;
}
PilatusFile::PilatusFile(QString path, cl_context * context, cl_command_queue * queue, cl_kernel * kernel)
{
    srchrad_sugg_low = std::numeric_limits<float>::max();
    srchrad_sugg_high = std::numeric_limits<float>::min();
    max_counts = 0;
    STATUS_OK = this->set(path, context, queue, kernel);
}

int PilatusFile::set(QString path, cl_context * context, cl_command_queue * queue, cl_kernel * kernel)
{
    this->context = context;
    this->queue = queue;
    this->filterKernel = kernel;
    
    this->path = path;
    if (!this->readHeader()) return 0;
    if (detector == "PILATUS 1M")
    {
        fast_dimension = 981;
        slow_dimension = 1043;
    }
    else if (detector == "PILATUS 2M")
    {
        fast_dimension = 1475;
        slow_dimension = 1679;
    }
    else if (detector == "PILATUS 6M")
    {
        fast_dimension = 2463;
        slow_dimension = 2527;
    }
    else 
    {
        std::cout << "Unknown detector: " << detector.toStdString().c_str() << std::endl;
        return 0;
    }
    this->suggestSearchRadius();

    loc_ws[0] = 16;
    loc_ws[1] = 16;
    glb_ws[0] = fast_dimension + loc_ws[0]%fast_dimension;
    glb_ws[1] = slow_dimension + loc_ws[1]%slow_dimension;
    
    return 1;
}



float PilatusFile::getSearchRadiusLowSuggestion()
{
    return srchrad_sugg_low;
}
float PilatusFile::getSearchRadiusHighSuggestion()
{
    return srchrad_sugg_high;
}
float PilatusFile::getQSuggestion()
{
    return 1.0/wavelength;
}

int PilatusFile::getWidth()
{
    return fast_dimension;
}
int PilatusFile::getHeight()
{
    return slow_dimension;
}

size_t PilatusFile::getBytes()
{
    return data_buf.size()*sizeof(float) + intensity.size()*sizeof(float) + index.size()*sizeof(int);
}

float * PilatusFile::getImage()
{
    return data_buf.data();
}

MiniArray<float> PilatusFile::getTest()
{
    return data_buf;
}

float PilatusFile::getMaxCount()
{
    return max_counts;
}

int PilatusFile::project(size_t * n, float * outBuf, int treshold_project_low, int treshold_project_high)
{
    this->treshold_project_low = treshold_project_low;
    this->treshold_project_high = treshold_project_high;
    
    // The frame has its axes like this, looking from the source to 
    // the detector in the zero rotation position. We use the
    // cartiesian coordinate system described in 
    // doi:10.1107/S0021889899007347
    //         y
    //         ^
    //         |
    //         |
    // z <-----x------ (fast)
    //         |
    //         |
    //       (slow)
    
    // Set the active angle
    int active_angle = 2;
    
    if(active_angle == 0) phi = start_angle + 0.5*angle_increment;
    else if(active_angle == 1) kappa = start_angle + 0.5*angle_increment;
    else if(active_angle == 2) omega = start_angle + 0.5*angle_increment;


    //~ std::cout << omega << std::endl;
    // Process the data points
    Matrix<float> XYZ(4,1,1);
    
        
    float k = 1.0f/wavelength; // Multiply with 2pi if desired
    
    for(size_t i = 0; i < intensity.size(); i++)
    {
        size_t fast_index = fast_dimension - (index.at(i) % fast_dimension) - 1;
        size_t slow_index = slow_dimension - (index.at(i) / fast_dimension) - 1;
        
        alpha =  0.8735582;
        beta =  0.000891863;
        
        XYZ[0] = 0;
        XYZ[1] = pixel_size_y * ((float) slow_index);
        XYZ[2] = pixel_size_x * ((float) fast_index);
        outBuf[(*n)+3] = intensity.at(i);
        
        // Center the detector
        XYZ[1] -= beam_x * pixel_size_y; // Not sure if one should offset by half a pixel more/less
        XYZ[2] -= beam_y * pixel_size_x;
        
        // Titlt the detector around origo assuming it correctly 
        // coincides with the actual center of rotation ( not yet implemented)
        
        
        // Project onto Ewald's sphere, moving to k-space
        XYZ[0] = -detector_distance;
        float len = std::sqrt(XYZ[0]*XYZ[0] + XYZ[1]*XYZ[1] + XYZ[2]*XYZ[2]);
        XYZ[0] = XYZ[0]*k/len;
        XYZ[1] = XYZ[1]*k/len;
        XYZ[2] = XYZ[2]*k/len; 
        {
            // XYZ now has the direction of the scattered ray with respect to the incident one. This can be used to calculate the scattering angle for correction purposes. 
            float x = XYZ[0];
            float y = XYZ[1];
            float z = XYZ[2];
            
            // lab_theta and lab_phi are not to be confused with the detector/sample angles. These are simply the circular coordinate representation of the pixel position
            float lab_theta = std::asin(y/k);
            float lab_phi = std::atan2(z,-x);
            
            /* Lorentz Polarization correction - The Lorentz part will depend on the scanning axis, and has to be applied if the frames are integrated over some time */
            
            // Assuming rotation around the z-axis of the lab frame:
            float L = std::sin(lab_theta);
            
            // The polarization correction also needs a bit more work
            //float P = 
            
            outBuf[(*n)+3] *= L;
        }
        
        XYZ[0] += k; // This step must be applied _after_ any detector rotation if such is used. This translation by k gives us the scattering vector Q, which is the reciprocal coordinate of the intensity.
        
        // Sample rotation
        RotationMatrix<float> PHI;
        RotationMatrix<float> KAPPA;
        RotationMatrix<float> OMEGA;

        PHI.setArbRotation(-phi, beta, 0);
        KAPPA.setArbRotation(-kappa, alpha, 0);
        OMEGA.setZRotation(-omega);
        
        XYZ = PHI*KAPPA*OMEGA*XYZ;
        
        
        // Store result
        outBuf[(*n)+0] = XYZ[0];
        outBuf[(*n)+1] = XYZ[1];
        outBuf[(*n)+2] = XYZ[2];
        (*n)+=4;
    }

    return 1;
}

float PilatusFile::getFlux()
{
    return flux;
}
float PilatusFile::getExpTime()
{
    return exposure_time;
}

int PilatusFile::readHeader()
{
    const float pi = 4.0*atan(1.0);
    
    // Based on the PILATUS 1.2 header convention. Regular expressions are used to fetch header values
    QString reqExpDetector("(?:Detector:\\s+)(\\S+\\s+\\S+),");
    QString reqExpTime("(?:Exposure_time\\s+)(\\d+(?:\\.\\d+)?)");
    QString reqPixSizex("(?:Pixel_size\\s+)(\\d+e[+-]\\d+)");
    QString reqPixSizey("(?:Pixel_size\\s+)(?:\\d+e[+-]\\d+)\\sm\\sx\\s(\\d+e[+-]\\d+)");
        
    QString optExpWl( "Wavelength\\s+(\\d+(?:\\.\\d+)?)" );
    QString optExpStAng( "Start_angle\\s+(\\d+(?:\\.\\d+)?)" );
    QString optExpAngInc( "Angle_increment\\s+(\\d+(?:\\.\\d+)?)" );
    QString optExpFlux( "Flux\\s+(\\d+(?:\\.\\d+)?)" );
    QString optExpDd( "Detector_distance\\s+(-?\\d+(?:\\.\\d+)?)" );
    QString optExpBeamx( "Beam_xy\\s+\\((-?\\d+(?:\\.\\d+)?)" );
    QString optExpBeamy( "Beam_xy\\s+\\((?:-?\\d+(?:\\.\\d+)?)(?:\\s*\\,\\s*)(-?\\d+(?:\\.\\d+)?)" );
    QString optExpPhi( "Phi\\s+(-?\\d+(?:\\.\\d+)?)" );
    QString optExpKappa( "Kappa\\s+(-?\\d+(?:\\.\\d+)?)" );
    QString optExpOmega( "Omega\\s+(-?\\d+(?:\\.\\d+)?)" );
    
    
    // Open file
    QFileInfo file_info(path);
    if (!file_info.exists())
    {
        std::cout << "File does not exist: " << path.toStdString().c_str() << std::endl;
        return 0;
    }
    else if (file_info.size() <= 0)
    {
        std::cout << "File is zero bytes: " << path.toStdString().c_str() << std::endl;
        return 0;
    }
    // Read file
    std::ifstream in(path.toStdString().c_str(), std::ios::in | std::ios::binary);
    if (!in)
    {
        std::cout << "Error reading file: " << path.toStdString().c_str() << std::endl;
        return 0;
    }

    std::string contents;
    contents.reserve(4096);
    in.read(&contents[0], 4096);
    in.close();
    QString header(contents.c_str());
    
    // Fetch keywords!
    /* Non-optional keywords */
    detector = regExp(&reqExpDetector, &header, 0, 1);
    pixel_size_x = regExp(&reqPixSizex, &header, 0, 1).toFloat();
    pixel_size_y = regExp(&reqPixSizey, &header, 0, 1).toFloat();
    //~ silicon_sensor_thickness = regExp(&, &header, 0, 1).toFloat();
    exposure_time = regExp(&reqExpTime, &header, 0, 1).toFloat();
    //~ exposure_period = regExp(&, &header, 0, 1).toFloat();
    //~ tau = regExp(&, &header, 0, 1).toFloat();
    //~ count_cutoff = regExp(&, &header, 0, 1).toInt();
    //~ threshold_setting = regExp(&, &header, 0, 1).toInt();
    //~ gain_setting = regExp(&, &header, 0, 1);
    //~ n_excluded_pixels = regExp(&, &header, 0, 1).toInt();
    //~ excluded_pixels = regExp(&, &header, 0, 1);
    //~ flat_field = regExp(&, &header, 0, 1);
    //~ time_file = regExp(&, &header, 0, 1);
    //~ image_path = regExp(&, &header, 0, 1);
    
    /* Optional keywords */
    wavelength = regExp(&optExpWl, &header, 0, 1).toFloat();
    //~ energy_range_low = regExp(&, &header, 0, 1).toInt();
    //~ energy_range_high = regExp(&, &header, 0, 1).toInt();
    detector_distance = regExp(&optExpDd, &header, 0, 1).toFloat();
    //~ detector_voffset = regExp(&, &header, 0, 1).toFloat();
    beam_x = regExp(&optExpBeamx, &header, 0, 1).toFloat();
    beam_y = regExp(&optExpBeamy, &header, 0, 1).toFloat();
    flux = regExp(&optExpFlux, &header, 0, 1).toFloat();
    //~ filter_transmission = regExp(&, &header, 0, 1).toFloat();
    start_angle = regExp(&optExpStAng, &header, 0, 1).toFloat()*pi/180.0;
    angle_increment = regExp(&optExpAngInc, &header, 0, 1).toFloat()*pi/180.0;
    //~ detector_2theta = regExp(&, &header, 0, 1).toFloat()*pi/180.0;
    //~ polarization = regExp(&, &header, 0, 1).toFloat();
    //~ alpha = regExp(&, &header, 0, 1).toFloat()*pi/180.0;
    kappa = regExp(&optExpKappa, &header, 0, 1).toFloat()*pi/180.0;
    phi = regExp(&optExpPhi, &header, 0, 1).toFloat()*pi/180.0;
    //~ phi_increment = regExp(&, &header, 0, 1).toFloat()*pi/180.0;
    //~ chi = regExp(&, &header, 0, 1).toFloat()*pi/180.0;
    //~ chi_increment = regExp(&, &header, 0, 1).toFloat()*pi/180.0;
    omega = regExp(&optExpOmega, &header, 0, 1).toFloat()*pi/180.0;
    //~ omega_increment = regExp(&, &header, 0, 1).toFloat()*pi/180.0;
    //~ oscillation_axis = regExp(&, &header, 0, 1);
    //~ n_oscillations = regExp(&, &header, 0, 1).toInt();
    //~ start_position = regExp(&, &header, 0, 1).toFloat();
    //~ position_increment = regExp(&, &header, 0, 1).toFloat();
    //~ shutter_time = regExp(&, &header, 0, 1).toFloat();

    return 1;
}

QString PilatusFile::getPath()
{
    return path;
}

QString PilatusFile::regExp(QString * regular_expression, QString * source, size_t offset, size_t i)
{
    QRegExp tmp(*regular_expression);
    int pos = tmp.indexIn(*source, offset);
    if (pos > -1) 
    {
        QString value = tmp.cap(i); 
        //~ qDebug() << *regular_expression << " gave: " << value;
        return value;
    }
    else
    {
        //~ qDebug() << "Warning! Regexp '" << *regular_expression << "' gave no matches!" ;
        return QString("");
    }
}

void PilatusFile::clearData()
{
    data_buf.clear();
}

int PilatusFile::filterData(int treshold_reduce_low, int treshold_reduce_high)
{
    this->treshold_reduce_low = treshold_reduce_low;
    this->treshold_reduce_high = treshold_reduce_high;

    // Targets
    cl_image_format target_format;
    target_format.image_channel_order = CL_INTENSITY;
    target_format.image_channel_data_type = CL_FLOAT;
    
    // Prepare the target for the corrected pixels (only intensity)
    cl_mem i_target_cl = clCreateImage2D ( (*context),
        CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
        &target_format,
        fast_dimension,
        slow_dimension,
        0,
        NULL,
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }

    target_format.image_channel_order = CL_RGBA;
    target_format.image_channel_data_type = CL_FLOAT;
    
    // Prepare the target for storage of projected and corrected pixels (intensity but also xyz position)
    cl_mem xyzi_target_cl = clCreateImage2D ( (*context),
        CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
        &target_format,
        fast_dimension,
        slow_dimension,
        0,
        NULL,
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }

    
    // Load data into a CL texture 
    cl_image_format source_format;
    source_format.image_channel_order = CL_INTENSITY;
    source_format.image_channel_data_type = CL_FLOAT;
    
    cl_mem source_cl = clCreateImage2D ( (*context),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        &source_format,
        fast_dimension,
        slow_dimension,
        fast_dimension*sizeof(cl_float),
        data_buf.data(),
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }
    cl_mem background_cl = clCreateImage2D ( (*context),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        &source_format,
        fast_dimension,
        slow_dimension,
        fast_dimension*sizeof(cl_float),
        background->data(),
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }
    
    // The sampler 
    cl_sampler intensity_sampler = clCreateSampler((*context), false, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Could not create sampler: " << cl_error_cstring(err) << std::endl;
    }

    std::cout << "Bg  " << background_flux  << " " << backgroundExpTime << std::endl;
    
    // SET KERNEL ARGS
    err = clSetKernelArg(*filterKernel, 0, sizeof(cl_mem), (void *) &i_target_cl);
    err |= clSetKernelArg(*filterKernel, 1, sizeof(cl_mem), (void *) &xyzi_target_cl);
    err |= clSetKernelArg(*filterKernel, 2, sizeof(cl_mem), (void *) &background_cl);
    err |= clSetKernelArg(*filterKernel, 3, sizeof(cl_mem), (void *) &source_cl);
    err |= clSetKernelArg(*filterKernel, 4, sizeof(cl_sampler), &intensity_sampler);
    float threshold_reduce[2] = {treshold_reduce_low, treshold_reduce_high};
    err |= clSetKernelArg(*filterKernel, 5, 2*sizeof(cl_float), threshold_reduce);
    err |= clSetKernelArg(*filterKernel, 6, sizeof(cl_float), &background_flux);
    err |= clSetKernelArg(*filterKernel, 7, sizeof(cl_float), &backgroundExpTime);
    err |= clSetKernelArg(*filterKernel, 8, sizeof(cl_float), &pixel_size_x);
    err |= clSetKernelArg(*filterKernel, 9, sizeof(cl_float), &pixel_size_y);
    err |= clSetKernelArg(*filterKernel, 10, sizeof(cl_float), &exposure_time);
    err |= clSetKernelArg(*filterKernel, 11, sizeof(cl_float), &wavelength);
    err |= clSetKernelArg(*filterKernel, 12, sizeof(cl_float), &detector_distance);
    err |= clSetKernelArg(*filterKernel, 13, sizeof(cl_float), &beam_x);
    err |= clSetKernelArg(*filterKernel, 14, sizeof(cl_float), &beam_y);
    err |= clSetKernelArg(*filterKernel, 15, sizeof(cl_float), &flux);
    err |= clSetKernelArg(*filterKernel, 16, sizeof(cl_float), &start_angle);
    err |= clSetKernelArg(*filterKernel, 17, sizeof(cl_float), &angle_increment);
    err |= clSetKernelArg(*filterKernel, 18, sizeof(cl_float), &kappa);
    err |= clSetKernelArg(*filterKernel, 19, sizeof(cl_float), &phi);
    err |= clSetKernelArg(*filterKernel, 20, sizeof(cl_float), &omega);
    
    if (err != CL_SUCCESS)
    {
        std::cout << "Error setting kernel argument: " << cl_error_cstring(err) << std::endl;
    }

    /* Launch rendering kernel */
    size_t area_per_call[2] = {128, 128};
    size_t call_offset[2] = {0,0};

    for (size_t glb_x = 0; glb_x < glb_ws[0]; glb_x += area_per_call[0])
    {
        for (size_t glb_y = 0; glb_y < glb_ws[1]; glb_y += area_per_call[1])
        {
            call_offset[0] = glb_x;
            call_offset[1] = glb_y;
            
            err = clEnqueueNDRangeKernel((*queue), *filterKernel, 2, call_offset, area_per_call, loc_ws, 0, NULL, NULL);
            if (err != CL_SUCCESS)
            {
                std::cout << "Error launching kernel: " << cl_error_cstring(err) << std::endl;
            }
        }
    }
    clFinish(*queue);
    
    // Read the data
    size_t origin[3];
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;

    size_t region[3];
    region[0] = fast_dimension;
    region[1] = slow_dimension;
    region[2] = 1;
    
    clEnqueueReadImage ( 	*queue,
        i_target_cl,
        true,
        origin,
        region,
        fast_dimension*sizeof(cl_float),
        0,
        this->data_buf.data(),
        0,
        NULL,
        NULL);
    
    if (i_target_cl) clReleaseMemObject(i_target_cl);
    if (ixyz_target_cl) clReleaseMemObject(ixyz_target_cl);
    if (source_cl) clReleaseMemObject(source_cl);
    if (background_cl) clReleaseMemObject(background_cl);
    if (intensity_sampler) clReleaseSampler(intensity_sampler);

    // Populate the intenity and index arrays by extracting nonzero values
    this->intensity.reserve(fast_dimension*slow_dimension);
    this->index.reserve(fast_dimension*slow_dimension);
    size_t iter = 0;
    
    for (int i = 0; i < (int) slow_dimension*fast_dimension; i++)
    {
        if (data_buf[i] > 0.0f)
        {
            intensity[iter] = data_buf[i];
            index[iter] = i;
            iter++;
        }  
    }
    
    this->intensity.resize(iter);
    this->index.resize(iter);

    return 1;
}

void PilatusFile::setBackground(Matrix<float>  * buffer, float flux, float exposure_time)
{
    this->background = buffer;
    this->background_flux = flux;
    this->backgroundExpTime = exposure_time;
}

int PilatusFile::readData()
{
    // Open file
    std::ifstream in(path.toStdString().c_str(), std::ios::in | std::ios::binary);
    if (!in)
    {
        std::cout << "Error reading file: " << path.toStdString().c_str() << std::endl;
        return 0;
    }
    
    in.seekg (0, in.end);
    int length = in.tellg();
    in.seekg (0, in.beg);
    int offset = 0;
    int header_length_max = 2000;
    //~ std::cout << "Mark r" << std::endl;
    // Read file
    char * buf = new char[length];
    in.read(buf, length);
    in.close();

    this->data_buf.reserve(fast_dimension*slow_dimension);
    //~ std::cout << "Mark r22" << std::endl;
    // Find beginning of binary section
    for (int i = 0; i < header_length_max; i++)
    {
        if ((int) buf[i] == -43) 
        {
            offset = i + 1;
            break;
        }
    }
    //~ std::cout << "Mark r1" << std::endl;
    // Decompress data and neglect data outside given tresholds
    int prev = 0;
    size_t id = offset;
    int counts;
    int int16;
    int int32;
   
    int i, j;
    //~ std::cout << "Mark r2" << std::endl;
    for (i = 0; i < (int) slow_dimension; i++)
    {
        for (j = 0; j < (int) fast_dimension; j++)
        {
            // Get value
            if (buf[id] == -128)
            {
                id++;
                int16 =  ((buf[id+1] << 8 ) | (buf[id] & 0xff));
                if (int16 == -32768)
                {
                    id+=2;
                    int32 = ((buf[id+3] << 24) | ((buf[id+2]& 0xff) << 16) | ((buf[id+1]& 0xff) << 8) | (buf[id]& 0xff));
                    counts = prev + int32;
                    id+=4;
                }
                else
                {
                    counts = prev + int16;
                    id+=2;
                }
            }
            else
            {
                counts = prev + (int) (buf[id]);
                id++;
            }
            prev = counts;

            data_buf[i*fast_dimension+j] = (float) counts;
            
            if (max_counts < counts) max_counts = counts;
        }
    }
    //~ std::cout << "Mark r3" << std::endl;
    delete[] buf;

    
//~ std::cout << "Mark r4" << std::endl;
    return 1;
}


void PilatusFile::suggestSearchRadius()
{
    /* A search radius can be found based on the projected size of a pixel in reciprocal space. The following calculations assume a detector that can be translated but not rotated. Could give a fair estimate even for a rotating detector*/

    // For several pixel position extrema:
    float y_config[] = {(float) 0, (float) slow_dimension/2, (float) slow_dimension-1, (float) slow_dimension-1, (float) 0};
    float z_config[] = {(float) 0, (float) fast_dimension/2, (float) fast_dimension-1, (float) 0, (float) fast_dimension-1};
    
    for (int i = 0; i < 5; i++)
    {
        // Calculate the projected size of the pixel onto the Ewald sphere. In effect, we find the width along the diagonal of a rectangular pixel 
        float xyz_a[3] = {
            (float)(-detector_distance),
            (float)(((y_config[i]-0.5) - beam_x) * pixel_size_x),
            (float)(((z_config[i]-0.5) - beam_y) * pixel_size_y)};
        
        float len_xyz_a = sqrt(xyz_a[0]*xyz_a[0] + xyz_a[1]*xyz_a[1] + xyz_a[2]*xyz_a[2]);
        
        xyz_a[0] /= len_xyz_a;
        xyz_a[1] /= len_xyz_a;
        xyz_a[2] /= len_xyz_a;
        
        float xyz_b[3] = {
            (float)(-detector_distance),
            (float)(((y_config[i]+0.5) - beam_x) * pixel_size_y),
            (float)(((z_config[i]+0.5) - beam_y) * pixel_size_x)};
        
        float len_xyz_b = sqrt(xyz_b[0]*xyz_b[0] + xyz_b[1]*xyz_b[1] + xyz_b[2]*xyz_b[2]);
        
        xyz_b[0] /= len_xyz_b;
        xyz_b[1] /= len_xyz_b;
        xyz_b[2] /= len_xyz_b;
        
        float k = 1/wavelength;
        
        float k_a[3] = {k*xyz_a[0], k*xyz_a[1], k*xyz_a[2]};
        float k_b[3] = {k*xyz_b[0], k*xyz_b[1], k*xyz_b[2]};
        
        float k_delta[3] = {k_b[0]-k_a[0], k_b[1]-k_a[1], k_b[2]-k_a[2]};
        float temp_search_radius = sqrt(k_delta[0]*k_delta[0] + k_delta[1]*k_delta[1] + k_delta[2]*k_delta[2]);
        
        if (temp_search_radius < srchrad_sugg_low) srchrad_sugg_low = temp_search_radius;
        if (temp_search_radius > srchrad_sugg_high) srchrad_sugg_high = temp_search_radius;
    }
}
