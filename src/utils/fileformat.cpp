#include "fileformat.h"

PilatusFile::~PilatusFile()
{

}
PilatusFile::PilatusFile() :
    active_angle(2)
{
    srchrad_sugg_low = std::numeric_limits<float>::max();
    srchrad_sugg_high = std::numeric_limits<float>::min();
    max_counts = 0;
    STATUS_OK = 0;
}
PilatusFile::PilatusFile(QString path, OpenCLContext *context):
    active_angle(2)
{
    context_cl = context;
    srchrad_sugg_low = std::numeric_limits<float>::max();
    srchrad_sugg_high = std::numeric_limits<float>::min();
    max_counts = 0;
    STATUS_OK = this->set(path, context_cl);
}

void PilatusFile::setActiveAngle(int value)
{
    active_angle = value;
}

QString PilatusFile::getHeaderText()
{
    std::stringstream ss;
    ss << "__Detector___________" << std::endl;
    ss << "Detector:    " << detector.toStdString().c_str() << std::endl;
    ss << "Pixel size:  " << pixel_size_x << " x " << pixel_size_y << " m"<<  std::endl;
    ss << "Exposure time:   "<< exposure_time << " s" << std::endl;
    ss << "Count cutoff:    "<< count_cutoff << std::endl << std::endl;
    
    
    ss << "__Beam_______________" << std::endl;
    ss << "Wavelength:  " << wavelength << " Å"<< std::endl;
    ss << "Flux:        "<< flux << std::endl << std::endl;

    
    ss << "__Geometry___________" << std::endl;
    ss << "Detector distance:   " << detector_distance << " m" << std::endl;
    ss << "Beam x y:        "<< beam_x << ", "<< beam_y << " pixels" << std::endl;
    ss << "Start angle:     "<< start_angle*180.0/pi << " deg" << std::endl;
    ss << "Angle increment: "<< angle_increment*180.0/pi << " deg"<< std::endl;
    ss << "Omega:           "<< omega*180.0/pi   << " deg" << std::endl;
    ss << "Kappa:           "<< kappa*180.0/pi   << " deg" << std::endl;
    ss << "Phi:             "<< phi*180.0/pi << " deg" << std::endl << std::endl;
    
    QString text(ss.str().c_str());

    return text;
}

int PilatusFile::set(QString path, OpenCLContext *context)
{
    this->context_cl = context;

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
    glb_ws[0] = fast_dimension + loc_ws[0] - (fast_dimension%loc_ws[0]);
    glb_ws[1] = slow_dimension + loc_ws[1] - (slow_dimension%loc_ws[1]);

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

int PilatusFile::getWidth() const
{
    return fast_dimension;
}
int PilatusFile::getHeight() const
{
    return slow_dimension;
}

size_t PilatusFile::getBytes() const
{
    return data_buf.bytes();
}

Matrix<float> PilatusFile::getTest()
{
    return data_buf;
}

float PilatusFile::getMaxCount()
{
    return max_counts;
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
    QString reqPixSizex("(?:Pixel_size\\s+)(\\d+(?:\\.\\d+)?e[+-]\\d+)");
    QString reqPixSizey("(?:Pixel_size\\s+)(?:\\d+(?:\\.\\d+)?e[+-]\\d+)\\sm\\sx\\s(\\d+(?:\\.\\d+)?e[+-]\\d+)");

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
        qDebug() << "File does not exist: " << path.toStdString().c_str();
        return 0;
    }
    else if (file_info.size() <= 0)
    {
        qDebug() << "File does not exist: " << path.toStdString().c_str();
        return 0;
    }
    // Read file
    QFile file(path.toStdString().c_str());
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Error reading file: " << path.toStdString().c_str();
        return 0;
    }
    QString header = file.read(4096);
    file.close();


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
//    ~ alpha = regExp(&, &header, 0, 1).toFloat()*pi/180.0;
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

QString PilatusFile::getPath() const
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
        return value;
    }
    else
    {
        return QString("");
    }
}

void PilatusFile::clearData()
{
    data_buf.clear();
}


void PilatusFile::setOpenCLBuffers(cl_mem * cl_img_alpha, cl_mem * cl_img_beta, cl_mem * cl_img_gamma, cl_mem * cl_tsf_tex)
{
    this->cl_img_alpha = cl_img_alpha;
    this->cl_img_beta = cl_img_beta;
    this->cl_img_gamma = cl_img_gamma;
    this->cl_tsf_tex = cl_tsf_tex;
}

void PilatusFile::print()
{
    std::stringstream ss;
    ss << "__________ PILATUS FILE __________" << std::endl;
    ss << "Path: " << path.toStdString().c_str() << std::endl;
    ss << "CL context: " << context_cl << std::endl;
    ss << "CL image alpha: " << cl_img_alpha << std::endl;
    ss << "CL image beta: " << cl_img_beta << std::endl;
    ss << "CL image gamma: " << cl_img_gamma << std::endl;
    ss << "CL kernel: " << project_kernel << std::endl;
    ss << "CL local work dim: " << loc_ws[0] << " " << loc_ws[1] << std::endl;
    ss << "CL global work dim: " << glb_ws[0] << " " << glb_ws[1] << std::endl;
    ss << "Data elements: " << data_buf.size() << std::endl;
    ss << "Dimensions: " << fast_dimension << " x " << slow_dimension << std::endl;
    ss << "Max counts: " << max_counts << std::endl;
    ss << "Reduction threshold: " << threshold_reduce_low << " " << threshold_reduce_high << std::endl;
    ss << "Projection threshold: " << threshold_project_low << " "<< threshold_project_high << std::endl;
    ss << "Search radius: " << srchrad_sugg_low << " " << srchrad_sugg_high << std::endl; 
    ss << "..." << std::endl;
    ss << "Detector: " << detector.toStdString().c_str() << std::endl;
    ss << "Pixel size: " << pixel_size_x << " x " << pixel_size_y << std::endl;
    ss << "Exposure time: " << exposure_time << std::endl;
    ss << "Exposure period: " << exposure_period << std::endl;
    ss << "Count cutoff: " << count_cutoff << std::endl;    
    ss << "Wavelength: " << wavelength << std::endl;
    ss << "Detector distance: " << detector_distance << std::endl;
    ss << "Beam position: " << beam_x << " x " << beam_y << std::endl;
    ss << "Flux: " << flux << std::endl;
    ss << "Start angle: " << start_angle << std::endl;
    ss << "Angle increment: " << angle_increment << std::endl;
    ss << "Alpha: " << alpha << std::endl;
    ss << "Beta: " << beta << std::endl;
    ss << "Kappa: " << kappa << std::endl;
    ss << "Phi: " << phi << std::endl;
    ss << "Omega: " << omega << std::endl;
    
    qDebug() << ss.str().c_str();
}

int PilatusFile::filterData(size_t * n, float * outBuf, float threshold_reduce_low, float threshold_reduce_high, float threshold_project_low, float threshold_project_high, bool isProjectionActive)
{

    this->threshold_reduce_low = threshold_reduce_low;
    this->threshold_reduce_high = threshold_reduce_high;
    this->threshold_project_low = threshold_project_low;
    this->threshold_project_high = threshold_project_high;
    
//    qDebug() << threshold_reduce_low;
//    qDebug() << threshold_reduce_high;
//    qDebug() << threshold_project_low;
//    qDebug() << threshold_project_high;

    cl_image_format target_format;
    target_format.image_channel_order = CL_RGBA;
    target_format.image_channel_data_type = CL_FLOAT;

    // Prepare the target for storage of projected and corrected pixels (intensity but also xyz position)
    cl_mem xyzi_target_cl = clCreateImage2D ( *context_cl->getContext(),
        CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
        &target_format,
        fast_dimension,
        slow_dimension,
        0,
        NULL,
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Load data into a CL texture
    cl_image_format source_format;
    source_format.image_channel_order = CL_INTENSITY;
    source_format.image_channel_data_type = CL_FLOAT;

    cl_mem source_cl = clCreateImage2D ( *context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        &source_format,
        fast_dimension,
        slow_dimension,
        fast_dimension*sizeof(cl_float),
        data_buf.data(),
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

//    cl_mem background_cl = clCreateImage2D ( *context_cl->getContext(),
//        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
//        &source_format,
//        fast_dimension,
//        slow_dimension,
//        fast_dimension*sizeof(cl_float),
//        background->data(),
//        &err);
//    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // A sampler. The filtering should be CL_FILTER_NEAREST unless a linear interpolation of the data is actually what you want
    cl_sampler intensity_sampler = clCreateSampler(*context_cl->getContext(), false, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Sample rotation matrix to be applied to each projected pixel to account for rotations. First set the active angle. Ideally this would be given by the header file, but for some reason it is not stated in there. Maybe it is just so normal to rotate around the omega angle to keep the resolution function consistent

    qDebug() << "The active angle is " << active_angle;
    if(active_angle == 0) phi = start_angle + 0.5*angle_increment;
    else if(active_angle == 1) kappa = start_angle + 0.5*angle_increment;
    else if(active_angle == 2) omega = start_angle + 0.5*angle_increment;

    RotationMatrix<float> PHI;
    RotationMatrix<float> KAPPA;
    RotationMatrix<float> OMEGA;
    RotationMatrix<float> sampleRotMat;

    alpha =  0.8735582;
    beta =  0.000891863;
    
//    qDebug() << "PHI";
    PHI.setArbRotation(beta, 0, -phi); 
//    qDebug() << "KAPPA";
    KAPPA.setArbRotation(alpha, 0, -kappa);
//    qDebug() << "OMEGA";
    OMEGA.setZRotation(-omega);

//    qDebug() << phi;
//    PHI.print(5);
    
    // The sample rotation matrix. Some rotations perturb the other rotation axes, and in the above calculations for phi, kappa, and omega we use fixed axes. It is therefore neccessary to put a rotation axis back into its basic position before the matrix is applied. In our case omega perturbs kappa and phi, and kappa perturbs phi. Thus we must first rotate omega back into the base position to recover the base rotation axis of kappa. Then we recover the base rotation axis for phi in the same manner. The order of matrix operations thus becomes:
    
    sampleRotMat = PHI*KAPPA*OMEGA;

    cl_mem sample_rotation_matrix_cl = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
        sampleRotMat.bytes(),
        sampleRotMat.data(),
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // The sampler for cl_tsf_tex
    cl_sampler tsf_sampler = clCreateSampler(*context_cl->getContext(), true, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Set kernel arguments
    err = clSetKernelArg(*project_kernel, 0, sizeof(cl_mem), (void *) &xyzi_target_cl);
    err |= clSetKernelArg(*project_kernel, 1, sizeof(cl_mem), (void *) cl_img_alpha);
    err |= clSetKernelArg(*project_kernel, 2, sizeof(cl_mem), (void *) cl_img_beta);
    err |= clSetKernelArg(*project_kernel, 3, sizeof(cl_mem), (void *) cl_img_gamma);
    err |= clSetKernelArg(*project_kernel, 4, sizeof(cl_mem), (void *) cl_tsf_tex);
//    err |= clSetKernelArg(*project_kernel, 5, sizeof(cl_mem), (void *) &background_cl);
    err |= clSetKernelArg(*project_kernel, 5, sizeof(cl_mem), (void *) &source_cl);
    err |= clSetKernelArg(*project_kernel, 6, sizeof(cl_sampler), &tsf_sampler);
    err |= clSetKernelArg(*project_kernel, 7, sizeof(cl_sampler), &intensity_sampler);
    err |= clSetKernelArg(*project_kernel, 8, sizeof(cl_mem), (void *) &sample_rotation_matrix_cl);
    float threshold_one[2] = {this->threshold_reduce_low, this->threshold_reduce_high};
    float threshold_two[2] = {this->threshold_project_low, this->threshold_project_high};
    err |= clSetKernelArg(*project_kernel, 9, 2*sizeof(cl_float), threshold_one);
    err |= clSetKernelArg(*project_kernel, 10, 2*sizeof(cl_float), threshold_two);
    err |= clSetKernelArg(*project_kernel, 11, sizeof(cl_float), &background_flux);
    err |= clSetKernelArg(*project_kernel, 12, sizeof(cl_float), &backgroundExpTime);
    err |= clSetKernelArg(*project_kernel, 13, sizeof(cl_float), &pixel_size_x);
    err |= clSetKernelArg(*project_kernel, 14, sizeof(cl_float), &pixel_size_y);
    err |= clSetKernelArg(*project_kernel, 15, sizeof(cl_float), &exposure_time);
    err |= clSetKernelArg(*project_kernel, 16, sizeof(cl_float), &wavelength);
    err |= clSetKernelArg(*project_kernel, 17, sizeof(cl_float), &detector_distance);
    err |= clSetKernelArg(*project_kernel, 18, sizeof(cl_float), &beam_x);
    err |= clSetKernelArg(*project_kernel, 19, sizeof(cl_float), &beam_y);
    err |= clSetKernelArg(*project_kernel, 20, sizeof(cl_float), &flux);
    err |= clSetKernelArg(*project_kernel, 21, sizeof(cl_float), &start_angle);
    err |= clSetKernelArg(*project_kernel, 22, sizeof(cl_float), &angle_increment);
    err |= clSetKernelArg(*project_kernel, 23, sizeof(cl_float), &kappa);
    err |= clSetKernelArg(*project_kernel, 24, sizeof(cl_float), &phi);
    err |= clSetKernelArg(*project_kernel, 25, sizeof(cl_float), &omega);
    err |= clSetKernelArg(*project_kernel, 26, sizeof(cl_float), &max_counts);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    /* Launch rendering kernel */
    size_t area_per_call[2] = {128, 128};
    size_t call_offset[2] = {0,0};
    for (size_t glb_x = 0; glb_x < glb_ws[0]; glb_x += area_per_call[0])
    {
        for (size_t glb_y = 0; glb_y < glb_ws[1]; glb_y += area_per_call[1])
        {
            call_offset[0] = glb_x;
            call_offset[1] = glb_y;

            err = clEnqueueNDRangeKernel(*context_cl->getCommandQueue(), *project_kernel, 2, call_offset, area_per_call, loc_ws, 0, NULL, NULL);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        }
    }
    clFinish(*context_cl->getCommandQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Read the data
    size_t origin[3];
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;

    size_t region[3];
    region[0] = fast_dimension;
    region[1] = slow_dimension;
    region[2] = 1;

    Matrix<float> projected_data_buf(1,fast_dimension*slow_dimension*4);
    err = clEnqueueReadImage ( *context_cl->getCommandQueue(), xyzi_target_cl, true, origin, region, 0, 0, projected_data_buf.data(), 0, NULL, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    if (xyzi_target_cl){
        err = clReleaseMemObject(xyzi_target_cl);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
    if (source_cl){
        err = clReleaseMemObject(source_cl);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
//    if (background_cl){
//        err = clReleaseMemObject(background_cl);
//        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
//    }
    if (sample_rotation_matrix_cl){
        err = clReleaseMemObject(sample_rotation_matrix_cl);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
    if (intensity_sampler){
        err = clReleaseSampler(intensity_sampler);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
    if (tsf_sampler){
        err = clReleaseSampler(tsf_sampler);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }

    if (isProjectionActive)
    {
        for (size_t i = 0; i < fast_dimension*slow_dimension; i++)
        {
            if (projected_data_buf[i*4+3] != 0.0)
            {
                outBuf[(*n)+0] = projected_data_buf[i*4+0];
                outBuf[(*n)+1] = projected_data_buf[i*4+1];
                outBuf[(*n)+2] = projected_data_buf[i*4+2];
                outBuf[(*n)+3] = projected_data_buf[i*4+3];
                (*n)+=4;
            }
        }
    }

    return 1;
}

void PilatusFile::setBackground(float flux, float exposure_time)
{
//    this->background = buffer;
    this->background_flux = flux;
    this->backgroundExpTime = exposure_time;
}

void PilatusFile::setProjectionKernel(cl_kernel * kernel)
{
    this->project_kernel = kernel;
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

    // Read file
    char * buf = new char[length];
    in.read(buf, length);
    in.close();

    this->data_buf.reserve(1, fast_dimension*slow_dimension);

    // Find beginning of binary section
    for (int i = 0; i < header_length_max; i++)
    {
        if ((int) buf[i] == -43)
        {
            offset = i + 1;
            break;
        }
    }
    
    // Decompress data and neglect data outside given thresholds
    int prev = 0;
    size_t id = offset;
    int counts;
    int int16;
    int int32;

    int i, j;
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

//            if ((i < 10) && (j < 10)) qDebug() << "i,j" << i << j << "data" << data_buf[i*fast_dimension+j] << "prev" << prev;
            
            if (max_counts < counts) max_counts = counts;
        }
    }
    delete[] buf;
    
//    data_buf.print(0);
    qDebug() << "offset" << offset;
    
    
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
