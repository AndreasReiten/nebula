#include "vol_data_set.h"

VolumeDataSet::VolumeDataSet(cl_device * device, cl_context * context, cl_command_queue * queue)
{
    BRICK_DIM = 7;
    BRICK_BORDER = 1;
    BRICK_DIM_TOT = BRICK_DIM  + BRICK_BORDER;
    N_VOX_BRICK = BRICK_DIM_TOT*BRICK_DIM_TOT*BRICK_DIM_TOT;
    BRICK_POOL_POWER = 7; // <= 10 (limited to 10 bits)
    MAX_BRICKS = (1 << BRICK_POOL_POWER)*(1 << BRICK_POOL_POWER)*(1 << BRICK_POOL_POWER);

    testBackground.set(1679, 1475, 0.0);

    this->device = device;
    this->context = context;
    this->queue = queue;
    this->initCL();
    this->currentDisplayFrame = 0;
    genprog_value = -1;

    suggested_q = std::numeric_limits<float>::min();

    suggested_search_radius_low = std::numeric_limits<float>::max();
    suggested_search_radius_high = std::numeric_limits<float>::min();
}

VolumeDataSet::~VolumeDataSet()
{
}

void VolumeDataSet::setImageRenderWidget(ImageRenderGLWidget * widget)
{
    imageRenderWidget = widget;
}

int VolumeDataSet::initCL()
{
    // Program
    QByteArray qsrc = open_resource(":/src/kernels/frameFilter.cl");
    const char * src = qsrc.data();
    size_t src_length = strlen(src);

    program = clCreateProgramWithSource((*context), 1, (const char **)&src, &src_length, &err);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
        return 0;
    }
    // Compile kernel
    const char * options = "-cl-single-precision-constant -cl-mad-enable -cl-fast-relaxed-math";
    err = clBuildProgram(program, 1, &device->device_id, options, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        // Compile log
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
        std::cout << "--- START KERNEL COMPILE LOG ---" << std::endl;
        char* build_log;
        size_t log_size;
        clGetProgramBuildInfo(program, device->device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        build_log = new char[log_size+1];
        clGetProgramBuildInfo(program, device->device_id, CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
        build_log[log_size] = '\0';
        std::cout << build_log << std::endl;
        std::cout << "---  END KERNEL COMPILE LOG  ---" << std::endl;
        delete[] build_log;
        return 0;
    }

    // Entry points
    K_FRAME_FILTER = clCreateKernel(program, "FRAME_FILTER", &err);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
        return 0;
    }

    return 1;
}

void VolumeDataSet::saveSVO()
{
    if (!(OCT_INDEX.size() > 0) && !(LEVELS > 0) && !(OCT_BRICK.size() > 0))
    {
        setMessageString("\nWarning: No data to save!");
        return;
    }
    QString fileName = QFileDialog::getSaveFileName(0, tr("Save File"), "", tr(".h5 (*.h5);; All Files (*)"));

    if ((fileName != ""))
    {
        /* HDF5 File structure
        * File ->
        *   /bricks -> (Data)
        *       n_bricks (Attribute)
        *       dim_brick (Attribute)
        *
        *   /oct_index -> (Data)
        *       n_nodes (Attribute)
        *       n_levels (Attribute)
        *       extent (Attribute)
        *
        *   /oct_brick -> (Data)
        *       brick_pool_power (Attribute)
        */

        // HDF5
        hid_t file_id;
        hid_t dset_id, dspace_id, atrib_id, plist_id;
        herr_t status;
        hsize_t dims[1];
        size_t size_t_value;

        /* Create file */
        file_id = H5Fcreate(fileName.toStdString().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

        /* Create property list needed for chunking and compression */
        plist_id  = H5Pcreate (H5P_DATASET_CREATE);

        // Dataset must be chunked for compression
        dims[0] = (size_t)N_VOX_BRICK;
        status = H5Pset_chunk (plist_id, 1, dims);

        /* Set ZLIB / DEFLATE Compression using compression level 4.
         * To use SZIP Compression comment out these lines.
        */
        status = H5Pset_deflate (plist_id, 4);

        /* Save traversal index part of octtree data and related metadata */
        dims[0] = OCT_INDEX.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        dset_id = H5Dcreate(file_id, "/oct_index", H5T_STD_U32LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Dwrite (dset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, OCT_INDEX.data());
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = OCT_INDEX.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "n_nodes", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = LEVELS;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "n_levels", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 8;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "extent", H5T_IEEE_F32LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_FLOAT, volume_extent);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        status = H5Dclose(dset_id);

        /* Save the brick index part of the octtree data */
        dims[0] = OCT_BRICK.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        dset_id = H5Dcreate(file_id, "/oct_brick", H5T_STD_U32LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Dwrite (dset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, OCT_BRICK.data());
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = BRICK_POOL_POWER;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "brick_pool_power", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
        status = H5Dclose(dset_id);

        /* Save the actual brick data */
        dims[0] = BRICKS.size();
        dspace_id = H5Screate_simple(1, dims, NULL);
        dset_id = H5Dcreate(file_id, "/bricks", H5T_IEEE_F32LE, dspace_id, H5P_DEFAULT, plist_id, H5P_DEFAULT);
        status = H5Dwrite (dset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, BRICKS.data());
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = BRICKS.size()/N_VOX_BRICK;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "n_bricks", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = BRICK_DIM + BRICK_BORDER;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "dim_brick", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        status = H5Dclose(dset_id);

        /* Histograms */
        size_t bins = 1000;
        double min = 1.0;
        double max = BRICKS.max();

        std::cout << "max: " << max << std::endl;

        double * hist_log = BRICKS.histogram(bins, min, max, 1, 1);
        double * hist_norm = BRICKS.histogram(bins, min, max, 0, 1);

        hist_log[0] = 0.0;
        hist_norm[0] = 0.0;

        MiniArray<double> histogram_normal(bins, hist_norm);
        MiniArray<double> histogram_log(bins, hist_log);

        //~ histogram_normal.print(2);
        //~ histogram_log.print(2);

        //~ histogram_normal.print(2);
        //~ histogram_log.print(2);

        /* Hin/Max */
        MiniArray<double> intensity_minmax(2);
        intensity_minmax[0] = min;
        intensity_minmax[1] = max;

        /* Comment */
        const char comment[] = "Riv format 0.1";

        MiniArray<char> svo_comment(std::strlen(comment));


        /* Other metadata */
        dims[0] = svo_comment.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        dset_id = H5Dcreate(file_id, "/meta", H5T_STD_I8LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Dwrite (dset_id, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, svo_comment.data());
        status = H5Sclose(dspace_id);

        dims[0] = histogram_normal.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "histogram_normal", H5T_IEEE_F64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_DOUBLE, histogram_normal.data());
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = histogram_normal.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "hist_norm_len", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = histogram_log.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "histogram_log10", H5T_IEEE_F64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_DOUBLE, histogram_log.data());
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        dspace_id = H5Screate_simple (1, dims, NULL);
        size_t_value = histogram_log.size();
        atrib_id = H5Acreate(dset_id, "hist_log10_len", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = intensity_minmax.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "value_min_max", H5T_IEEE_F64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_DOUBLE, intensity_minmax.data());
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        status = H5Dclose(dset_id);

        status = H5Pclose (plist_id);

        /* Close the file */
        status = H5Fget_filesize(file_id, &dims[0] );
        status = H5Fclose(file_id);

        setMessageString("\nData saved in: \""+fileName+"\" ("+QString::number(dims[0]/1000000.0, 'g', 3)+" MB)");
    }
}
float * VolumeDataSet::getExtent()
{
    return volume_extent;
}
size_t VolumeDataSet::getLEVELS()
{
    return LEVELS;
}

size_t VolumeDataSet::getBPP()
{
    return BRICK_POOL_POWER;
}
size_t VolumeDataSet::getN_BRICKS()
{
    return N_BRICKS;
}
size_t VolumeDataSet::getBRICK_DIM_TOT()
{
    return BRICK_DIM_TOT;
}

MiniArray<float> * VolumeDataSet::getBRICKS()
{
    return &BRICKS;
}
MiniArray<unsigned int> * VolumeDataSet::getOCT_INDEX()
{
    return &OCT_INDEX;
}
MiniArray<unsigned int> * VolumeDataSet::getOCT_BRICK()
{
    return &OCT_BRICK;
}

void VolumeDataSet::setFormatGenericProgress(QString str)
{
        emit changedFormatGenericProgress(str);
}

void VolumeDataSet::setValueGenericProgress(int value)
{
    if (value != genprog_value)
    {
        genprog_value = value;
        emit changedGenericProgress(value);
    }
}

void VolumeDataSet::setMessageString(QString str)
{
        emit changedMessageString(str);
}


unsigned int getOctIndex(unsigned int msdFlag, unsigned int dataFlag, unsigned int child)
{
    return (msdFlag << 31) | (dataFlag << 30) | child;
}

unsigned int getOctBrick(unsigned int poolX, unsigned int poolY, unsigned int poolZ)
{
    return (poolX << 20) | (poolY << 10) | (poolZ << 0);
}

int VolumeDataSet::GENERATE_SVO_OCTTREE()
{
    setFormatGenericProgress("Generating Sparse Voxel Octtree");
    setMessageString("\n[SVO] Generating Sparse Voxel Octtree "+QString::number(LEVELS)+" levels deep.");
    setMessageString("\n[SVO] The source data is "+QString::number(POINTS.bytes()/1000000.0, 'g', 3)+" MB");


    // The extents of the volume
    volume_extent[0] = -suggested_q;
    volume_extent[1] = +suggested_q;
    volume_extent[2] = -suggested_q;
    volume_extent[3] = +suggested_q;
    volume_extent[4] = -suggested_q;
    volume_extent[5] = +suggested_q;
    volume_extent[6] = 1.0;
    volume_extent[7] = 1.0;

    /* Generate an octtree data structure from which to construct bricks */
    Matrix<double> extent(3,2);
    extent[0] = volume_extent[0];
    extent[1] = volume_extent[1];
    extent[2] = volume_extent[2];
    extent[3] = volume_extent[3];
    extent[4] = volume_extent[4];
    extent[5] = volume_extent[5];

    SearchNode root(NULL, extent.data());

    setValueGenericProgress(0);
    for (size_t i = 0; i < POINTS.size()/4; i++)
    {
        root.insert(POINTS.data()+i*4);
        setValueGenericProgress((i+1)*100/(POINTS.size()/4));
    }

    /* Create an octtree from brick data. The nodes are maintained in a linear array rather than on the heap. This is due to lack of proper support for recursion on GPUs */
    MiniArray<BrickNode> OCTTREE(MAX_BRICKS*2);
    MiniArray<unsigned int> nodes;
    nodes.set(64, (unsigned int) 0);
    nodes[0] = 1;
    nodes[1] = 8;

    unsigned int confirmed_nodes = 1, non_empty_node_counter = 1;


    // Intitialize the first level
    {
        setMessageString("\n[SVO] Constructing Level 0 (dim: "+QString::number(BRICK_DIM * (1 <<  0))+")");

        OCTTREE[0].setMsdFlag(0);
        OCTTREE[0].setDataFlag(1);
        OCTTREE[0].setChild(1);
        OCTTREE[0].setPoolId(0,0,0);
        OCTTREE[0].setBrickId(0,0,0);
        OCTTREE[0].setLevel(0);
        OCTTREE[1].setParent(0);
        OCTTREE[2].setParent(0);
        OCTTREE[3].setParent(0);
        OCTTREE[4].setParent(0);
        OCTTREE[5].setParent(0);
        OCTTREE[6].setParent(0);
        OCTTREE[7].setParent(0);

        float * brick_data = new float[N_VOX_BRICK];
        srchrad = sqrt(3.0f)*0.5f*((volume_extent[1]-volume_extent[0])/ (BRICK_DIM*(1 << 0)));

        if (srchrad < suggested_search_radius_high) srchrad = suggested_search_radius_high;
        root.getBrick(brick_data, extent.data(), 1.0, srchrad, BRICK_DIM_TOT);

        OCTTREE[0].setBrick(brick_data);

        setMessageString(" ...done");
    }

    // Cycle through the remaining levels
    for (size_t lvl = 1; lvl < LEVELS; lvl++)
    {
        {
            setFormatGenericProgress("Constructing Level "+QString::number(lvl)+" (dim: "+QString::number(BRICK_DIM * (1 <<  lvl))+"): %p%");
            setMessageString("\n[SVO] Constructing Level "+QString::number(lvl)+" (dim: "+QString::number(BRICK_DIM * (1 <<  lvl))+")");
            setValueGenericProgress(0);
        }
        this->timer.start();

        // Find the correct range search radius
        srchrad = sqrt(3.0f)*0.5f*((volume_extent[1]-volume_extent[0])/ (BRICK_DIM*(1 << lvl)));
        if (srchrad < suggested_search_radius_high) srchrad = suggested_search_radius_high;

        double tmp = (extent[1]-extent[0])/(1 << lvl);

        // For each node
        size_t iter = 0;
        for (size_t i = 0; i < nodes[lvl]; i++)
        {
            // The id of the octnode in the octnode array
            unsigned int currentId = confirmed_nodes + i;

            // Set the level
            OCTTREE[currentId].setLevel(lvl);

            // Set the brick id
            OCTTREE[currentId].calcBrickId(i%8 ,&OCTTREE[OCTTREE[currentId].getParent()]);

            // Based on brick id calculate the brick extent and then calculate and set the brick data
            unsigned int * brickId = OCTTREE[currentId].getBrickId();

            Matrix<double> brick_extent(3,2);
            brick_extent[0] = extent[0] + tmp*brickId[0];
            brick_extent[1] = extent[0] + tmp*(brickId[0]+1);
            brick_extent[2] = extent[2] + tmp*brickId[1];
            brick_extent[3] = extent[2] + tmp*(brickId[1]+1);
            brick_extent[4] = extent[4] + tmp*brickId[2];
            brick_extent[5] = extent[4] + tmp*(brickId[2]+1);

            float * brick_data = new float[N_VOX_BRICK];

            bool isEmpty = root.getBrick(brick_data, brick_extent.data(), 1.0, srchrad, BRICK_DIM_TOT);

            if (isEmpty)
            {
                OCTTREE[currentId].setDataFlag(0);
                OCTTREE[currentId].setMsdFlag(1);
                OCTTREE[currentId].setChild(0);
                delete[] brick_data;
            }
            else
            {
                OCTTREE[currentId].setDataFlag(1);
                OCTTREE[currentId].setMsdFlag(0);
                if (lvl >= LEVELS - 1) OCTTREE[currentId].setMsdFlag(1);
                OCTTREE[currentId].setBrick(brick_data);
                OCTTREE[currentId].calcPoolId(BRICK_POOL_POWER, non_empty_node_counter);

                if (!OCTTREE[currentId].getMsdFlag())
                {
                    unsigned int childId = confirmed_nodes + nodes[lvl] + iter*8;
                    OCTTREE[currentId].setChild(childId);

                    // For each child
                    for (size_t j = 0; j < 8; j++)
                    {
                        OCTTREE[childId+j].setParent(currentId);
                        nodes[lvl+1]++;
                    }
                }
                non_empty_node_counter++;
                iter++;
            }
            setValueGenericProgress((i+1)*100/nodes[lvl]);
        }
        confirmed_nodes += nodes[lvl];

        size_t t = this->timer.restart();
        setMessageString(" ...done (time: "+QString::number(t)+" ms)");
    }

    // Use the node structure to populate the GPU arrays
    OCT_INDEX.reserve(confirmed_nodes);
    OCT_BRICK.reserve(confirmed_nodes);
    BRICKS.reserve(non_empty_node_counter*N_VOX_BRICK);
    N_BRICKS = non_empty_node_counter;

    setValueGenericProgress(0);
    size_t iter = 0;
    for (size_t i = 0; i < confirmed_nodes; i++)
    {
        OCT_INDEX[i] = getOctIndex(OCTTREE[i].getMsdFlag(), OCTTREE[i].getDataFlag(), OCTTREE[i].getChild());
        OCT_BRICK[i] = getOctBrick(OCTTREE[i].getPoolId()[0], OCTTREE[i].getPoolId()[1], OCTTREE[i].getPoolId()[2]);

        if (OCTTREE[i].getDataFlag())
        {
            for (size_t j = 0; j < N_VOX_BRICK; j++)
            {
                BRICKS[N_VOX_BRICK*iter + j] = OCTTREE[i].getBrick()[j];
            }
            iter++;
        }
        setValueGenericProgress((i+1)*100/confirmed_nodes);
    }

    setMessageString("\n[SVO] Total SVO size (uncompressed): "+QString::number((OCT_INDEX.bytes() + OCT_BRICK.bytes() + BRICKS.bytes())/1e6, 'g', 3)+" MB");
    setMessageString("\n[SVO] Number of bricks: "+QString::number(confirmed_nodes));

    return 1;
}


void VolumeDataSet::setSvoLevels(int value)
{
    LEVELS = value;
}

void VolumeDataSet::setLowThresholdReduce(double value)
{
    threshold_reduce[0] = value;
}

void VolumeDataSet::setLowThresholdProject(double value)
{
    threshold_project[0] = value;
}

void VolumeDataSet::setHighThresholdReduce(double value)
{
    threshold_reduce[1] = value;
}

void VolumeDataSet::setHighThresholdProject(double value)
{
    threshold_project[1] = value;
}

void VolumeDataSet::setFormat(int value)
{

}

int VolumeDataSet::funcAllInOne()
{
    /* Initialize a list of files */
    if (paths.size() == 0)
    {
        setMessageString("\n[AllInOne] Warning: No files specified!");
        return 0;
    }
    this->paths = paths;

    setMessageString("\n[AllInOne] Treating "+QString::number(paths.size())+" files...");
    setFormatGenericProgress(QString("Treating Files %p%"));

    // Clear previous data
    //~ RAWFILE.clear();

    // Prepare container for final projected data
    size_t n = 0;
    size_t limit = 0.25e9;
    POINTS.reserve(limit);
    size_t size_raw = 0;
    size_t n_ok_files = 0;


    timer.start();
    for (size_t i = 0; i < (size_t) paths.size(); i++)
    {
        PilatusFile file;
        // Set file and get status
        int STATUS_OK = file.set(paths[i], context, queue);//, &K_FRAME_FILTER, imageRenderWidget);
        // Set the background that will be subtracted from the data
        file.setBackground(&testBackground, file.getFlux(), file.getExpTime());
        if (STATUS_OK)
        {
            // Get suggestions on the minimum search radius that can safely be applied during interpolation
            if (suggested_search_radius_low > file.getSearchRadiusLowSuggestion()) suggested_search_radius_low = file.getSearchRadiusLowSuggestion();
            if (suggested_search_radius_high < file.getSearchRadiusHighSuggestion()) suggested_search_radius_high = file.getSearchRadiusHighSuggestion();

            // Get suggestions on the size of the largest reciprocal Q-vector in the data set (physics)
            if (suggested_q < file.getQSuggestion()) suggested_q = file.getQSuggestion();
        }
        else
        {
            continue;
            setMessageString("\n[AllInOne] Warning: \""+QString(paths[i])+"\"");
        }
        // Read file and get status
        STATUS_OK = file.readData();
        size_raw += file.getBytes();
        if (STATUS_OK)
        {
            //~ emit changedRawImage(&file);
        }
        else
        {
            setMessageString("\n[AllInOne] Error: could not read \""+file.getPath()+"\"");
            return 0;
        }
        // Project
        if (n > limit)
        {
            // Break if there is too much data.
            setMessageString(QString("\n[AllInOne] Warning: Projecting too much data! Breaking off early!"));
            break;
        }
        else
        {
            imageRenderWidget->setImageSize(file.getWidth(), file.getHeight());

            STATUS_OK = file.filterData( &n, POINTS.data(), threshold_reduce[0], threshold_reduce[1], threshold_project[0], threshold_project[1], 1);
            if (STATUS_OK)
            {
                //~ emit changedCorrectedImage(&RAWFILE[i]);
                emit repaintRequest();
            }
            else
            {
                setMessageString("\n[AllInOne] Error: could not project \""+file.getPath()+"\"");
                return 0;
            }
        }
        n_ok_files++;
        // Update the progress bar
        setValueGenericProgress(100*(i+1)/paths.size());
    }

    POINTS.resize(n);
    size_t t = timer.restart();

    setMessageString("\n[AllInOne] "+QString::number(n_ok_files)+" of "+QString::number(paths.size())+" files were successfully set, read, corrected, projected, and merged ("+QString::number(POINTS.bytes()/1000000.0, 'g', 3)+" MB) (time: " +timeString(t)+ ", "+QString::number(t/n_ok_files, 'g', 3)+" ms/file)");

    // From q and the search radius it is straigthforward to calculate the required resolution and thus octtree level
    float resolution_min = 2*suggested_q/suggested_search_radius_high;
    float resolution_max = 2*suggested_q/suggested_search_radius_low;

    float level_min = std::log(resolution_min/(float)BRICK_DIM)/std::log(2);
    float level_max = std::log(resolution_max/(float)BRICK_DIM)/std::log(2);

    setMessageString("\n[AllInOne] Max scattering vector Q: "+QString::number(suggested_q, 'g', 3)+" inverse "+trUtf8("Å"));
    setMessageString("\n[AllInOne] Search radius: "+QString::number(suggested_search_radius_low, 'g', 2)+" to "+QString::number(suggested_search_radius_high, 'g', 2)+" inverse "+trUtf8("Å"));
    setMessageString("\n[AllInOne] Suggesting minimum resolution: "+QString::number(resolution_min, 'f', 0)+" to "+QString::number(resolution_max, 'f', 0)+" voxels");
    setMessageString("\n[AllInOne] Suggesting minimum octtree level: "+QString::number(level_min, 'f', 2)+" to "+QString::number(level_max, 'f', 2)+"");

    return 1;
}

int VolumeDataSet::funcSetFiles()
{
    /* Initialize a list of files */
    if (paths.size() == 0)
    {
        setMessageString("\n[Set] Warning: No files specified!");
        return 0;
    }
    this->paths = paths;

    setMessageString("\n[Set] Setting "+QString::number(paths.size())+" files (headers etc.)...");
    setFormatGenericProgress(QString("Setting Files %p%"));

    // Clear previous data
    RAWFILE.clear();
    RAWFILE.reserve(paths.size());

    timer.start();
    for (size_t i = 0; i < (size_t) paths.size(); i++)
    {
        // Set file and get status
        RAWFILE.append(PilatusFile());
        int STATUS_OK = RAWFILE.back().set(paths[i], context, queue);//, &K_FRAME_FILTER);
        if (STATUS_OK)
        {
            // Get suggestions on the minimum search radius that can safely be applied during interpolation
            if (suggested_search_radius_low > RAWFILE.back().getSearchRadiusLowSuggestion()) suggested_search_radius_low = RAWFILE.back().getSearchRadiusLowSuggestion();
            if (suggested_search_radius_high < RAWFILE.back().getSearchRadiusHighSuggestion()) suggested_search_radius_high = RAWFILE.back().getSearchRadiusHighSuggestion();

            // Get suggestions on the size of the largest reciprocal Q-vector in the data set (physics)
            if (suggested_q < RAWFILE.back().getQSuggestion()) suggested_q = RAWFILE.back().getQSuggestion();

            // Set the background that will be subtracted from the data
            RAWFILE.back().setBackground(&testBackground, RAWFILE.front().getFlux(), RAWFILE.front().getExpTime());
        }
        else
        {
            RAWFILE.removeLast();
            setMessageString("\n[Set] Warning: \""+QString(paths[i])+"\"");
        }

        // Update the progress bar
        setValueGenericProgress(100*(i+1)/paths.size());
    }
    size_t t = timer.restart();

    setMessageString("\n[Set] "+QString::number(RAWFILE.size())+" of "+QString::number(paths.size())+" files were successfully set (time: " + QString::number(t) + " ms, "+QString::number(t/paths.size(), 'g', 3)+" ms/file)");

    // From q and the search radius it is straigthforward to calculate the required resolution and thus octtree level
    float resolution_min = 2*suggested_q/suggested_search_radius_high;
    float resolution_max = 2*suggested_q/suggested_search_radius_low;

    float level_min = std::log(resolution_min/(float)BRICK_DIM)/std::log(2);
    float level_max = std::log(resolution_max/(float)BRICK_DIM)/std::log(2);

    setMessageString("\n[Set] Max scattering vector Q: "+QString::number(suggested_q, 'g', 3)+" inverse "+trUtf8("Å"));
    setMessageString("\n[Set] Search radius: "+QString::number(suggested_search_radius_low, 'g', 2)+" to "+QString::number(suggested_search_radius_high, 'g', 2)+" inverse "+trUtf8("Å"));
    setMessageString("\n[Set] Suggesting minimum resolution: "+QString::number(resolution_min, 'f', 0)+" to "+QString::number(resolution_max, 'f', 0)+" voxels");
    setMessageString("\n[Set] Suggesting minimum octtree level: "+QString::number(level_min, 'f', 2)+" to "+QString::number(level_max, 'f', 2)+"");

    return 1;
}

void VolumeDataSet::incrementDisplayFrame1()
{
    currentDisplayFrame ++;
    if (currentDisplayFrame < 0) currentDisplayFrame = 0;
    if (currentDisplayFrame >= paths.size() - 1) currentDisplayFrame = paths.size() - 1;
    emit displayFrameChanged(currentDisplayFrame);
}
void VolumeDataSet::decrementDisplayFrame1()
{
    currentDisplayFrame --;
    if (currentDisplayFrame < 0) currentDisplayFrame = 0;
    if (currentDisplayFrame >= paths.size() - 1) currentDisplayFrame = paths.size() - 1;
    emit displayFrameChanged(currentDisplayFrame);
}
void VolumeDataSet::incrementDisplayFrame5()
{
    currentDisplayFrame +=5;
    if (currentDisplayFrame < 0) currentDisplayFrame = 0;
    if (currentDisplayFrame >= paths.size() - 1) currentDisplayFrame = paths.size() - 1;
    emit displayFrameChanged(currentDisplayFrame);
}
void VolumeDataSet::decrementDisplayFrame5()
{
    currentDisplayFrame -=5;
    if (currentDisplayFrame < 0) currentDisplayFrame = 0;
    if (currentDisplayFrame >= paths.size() - 1) currentDisplayFrame = paths.size() - 1;
    emit displayFrameChanged(currentDisplayFrame);
}

void VolumeDataSet::setPaths(QStringList strlist)
{
    this->paths = strlist;
}

void VolumeDataSet::setDisplayFrame(int value)
{
    // clamp
    if (value < 0) value = 0;
    if (value >= paths.size() - 1) value = paths.size() - 1;
    if (paths.size() == 0) return;
    currentDisplayFrame = value;

    //~ setMessageString("\n[NebulaX] Displaying frame "+QString::number(value)+": \""+QString(paths[value])+"\"");
    PilatusFile file;

    // Set file and get status
    int STATUS_OK = file.set(paths[value], context, queue);//, &K_FRAME_FILTER, imageRenderWidget);
    // Set the background that will be subtracted from the data
    file.setBackground(&testBackground, file.getFlux(), file.getExpTime());
    if (!STATUS_OK)
    {
        setMessageString("\n[Set] Warning: \""+QString(paths[value])+"\"");
        return;
    }

    // Read file and get status
    STATUS_OK = file.readData();
    if (STATUS_OK)
    {
        //~ emit changedRawImage(&file);
    }
    else
    {
        setMessageString("\n[Read] Error: could not read \""+file.getPath()+"\"");
        return;
    }
    // Filter file and get status
    imageRenderWidget->setImageSize(file.getWidth(), file.getHeight());

    STATUS_OK = file.filterData( 0, NULL, threshold_reduce[0], threshold_reduce[1], threshold_project[0], threshold_project[1], 0);
    if (STATUS_OK)
    {
        //~ emit changedCorrectedImage(&file);
        emit repaintRequest();
    }
    else
    {
        setMessageString("\nRead] Error: could not filter \""+file.getPath()+"\"");
        return;
    }

}

int VolumeDataSet::funcReadFiles()
{
    /* Read the intensity data in each file. For PILATUS files the data must be decompressed */
    setMessageString("\n[Read] Reading "+QString::number(RAWFILE.size())+" files...");
    setFormatGenericProgress(QString("Reading Files %p%"));
    size_t size_raw = 0;

    //~ QElapsedTimer stopWatch;
    timer.start();
    for (size_t i = 0; i < (size_t) RAWFILE.size(); i++)
    {
        // Read file and get status
        int STATUS_OK = RAWFILE[i].readData();
        size_raw += RAWFILE[i].getBytes();
        if (STATUS_OK)
        {
            emit changedRawImage(&RAWFILE[i]);
            emit repaintRequest();
        }
        else
        {
            setMessageString("\n[Read] Error: could not read \""+RAWFILE[i].getPath()+"\"");
            return 0;
        }
        // Update the progress bar
        setValueGenericProgress(100*(i+1)/RAWFILE.size());
    }
    size_t t = timer.restart();
    setMessageString("\n[Read] "+QString::number(RAWFILE.size())+" files were successfully read ("+QString::number(size_raw/1000000.0, 'g', 3)+" MB) (time: " + QString::number(t) + " ms, "+QString::number(t/RAWFILE.size(), 'g', 3)+" ms/file)");

    return 1;
}

int VolumeDataSet::funcProjectFiles()
{
    /* For each file, project the detector coordinate and corresponding intensity down onto the Ewald sphere. Intensity corrections are also carried out in this step. The header of each file should include all the required information to to the transformations. The result is stored in a seprate container. There are different file formats, and all files coming here should be of the same base type. */

    setMessageString("\n[Cor/Proj] Correcting and Projecting "+QString::number(RAWFILE.size())+" files...");
    setFormatGenericProgress(QString("Correcting and Projecting Files %p%"));

    size_t n = 0;
    size_t limit = 0.25e9;
    POINTS.reserve(limit);

    timer.start();
    for (size_t i = 0; i < (size_t) RAWFILE.size(); i++)
    {
        if (n > limit)
        {
            // Break if there is too much data.
            setMessageString(QString("\n[Cor/Proj] Warning: There was too much data! Breaking off early!"));
            break;
        }
        else
        {
            imageRenderWidget->setImageSize(RAWFILE[i].getWidth(), RAWFILE[i].getHeight());

            int STATUS_OK = RAWFILE[i].filterData( &n, POINTS.data(), threshold_reduce[0], threshold_reduce[1], threshold_project[0], threshold_project[1],1);
            if (STATUS_OK)
            {
                emit changedRawImage(&RAWFILE[i]);
                emit changedCorrectedImage(&RAWFILE[i]);
                emit repaintRequest();
            }
            else
            {
                setMessageString("\n[Cor/Proj] Error: could not process data \""+RAWFILE[i].getPath()+"\"");
                return 0;
            }
        }

        // Update the progress bar
        setValueGenericProgress(100*(i+1)/RAWFILE.size());
    }

    POINTS.resize(n);
    size_t t = timer.restart();

    setMessageString("\n[Cor/Proj] "+QString::number(RAWFILE.size())+" files were successfully projected and merged ("+QString::number(POINTS.bytes()/1000000.0, 'g', 3)+" MB) (time: " + QString::number(t) + " ms, "+QString::number(t/RAWFILE.size(), 'g', 3)+" ms/file)");

    return 1;
}


int VolumeDataSet::funcGenerateSvo()
{
    int STATUS_OK = GENERATE_SVO_OCTTREE();
    return STATUS_OK;
}

