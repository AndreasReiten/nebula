#include "contextgl.h"

ContextGLWidget::ContextGLWidget(const QGLFormat & format, QWidget * parent) :
    QGLWidget(format, parent)
{
    //~std::cout << "this->format().hasOverlay() " <<  this->format().hasOverlay() << std::endl;
    //~std::cout << "this->format().alpha() " <<  this->format().alpha() << std::endl;
    verbosity = 1;
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+" called");
    isGLIntitialized = false;
    glInit();
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+" done");
}

ContextGLWidget::~ContextGLWidget()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    if (isGLIntitialized)
    {
        if (*queue) clReleaseCommandQueue(*queue);
        if (*context) clReleaseContext(*context);
    }
}

cl_command_queue * ContextGLWidget::getCLCommandQueue()
{
    if (isGLIntitialized) return queue;
    else return NULL;
}
cl_device * ContextGLWidget::getCLDevice()
{
    if (isGLIntitialized) return device;
    else return NULL;
}
cl_context * ContextGLWidget::getCLContext()
{
    if (isGLIntitialized) return context;
    else return NULL;
}

void ContextGLWidget::initializeGL()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    if (!this->initResourcesGL()) std::cout << "Error initializing OpenGL" << std::endl;
    if (!this->initDeviceCL(1)) std::cout << "Error initializing OpenCL Device" << std::endl;
    if (!this->initResourcesCL()) std::cout << "Error initializing OpenCL" << std::endl;

    isGLIntitialized = true;
}



void ContextGLWidget::paintGL()
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}


void ContextGLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}


//~void ContextGLWidget::paintOverlayGL()
//~{
    //~;
//~}
//~void ContextGLWidget::resizeOverlayGL(int w, int h)
//~{
    //~;
//~}
//~void ContextGLWidget::initializeOverlayGL()
//~{
    //~;
//~}


void ContextGLWidget::setMessageString(QString str)
{
    emit changedMessageString(str);
}


int ContextGLWidget::initResourcesGL()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    /* Initialize OpenGL */
    GLenum glew_status = glewInit();
    if (GLEW_OK != glew_status) {
        std::cout << "Error: " << glewGetErrorString(glew_status) << std::endl;
        return 0;
    }

    if (!GLEW_VERSION_4_0) {
        std::cout << "No support for OpenGL 4.0 found" << std::endl;
        return 0;
    }

    // BLEND
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

    return 1;
}

int ContextGLWidget::initDeviceCL(int verbosity)
{
    // DEVICE & INFO
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    device = new cl_device;

    err = clGetPlatformIDs(1, &device->platform_id, &num_platforms);
    if ( err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
        return 0;
    }
    err = clGetDeviceIDs( device->platform_id, CL_DEVICE_TYPE_GPU, 1, &device->device_id, &num_devices);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
        return 0;
    }
    clGetDeviceInfo(device->device_id, CL_DEVICE_NAME, sizeof(char)*64, &device->cl_device_name, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_VERSION, sizeof(char)*64, &device->cl_device_version, NULL);
    clGetDeviceInfo(device->device_id, CL_DRIVER_VERSION, sizeof(char)*64, &device->cl_driver_version, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(device->gpu_max_mem_alloc_size), &device->gpu_max_mem_alloc_size, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &device->gpu_clock_max, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &device->gpu_global_mem, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(cl_uint), &device->gpu_global_mem_cache_line, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &device->gpu_local_mem, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &device->gpu_compute_units, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(cl_ulong), &device->gpu_work_group_size, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &device->gpu_work_item_dim, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &device->gpu_image_support, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t)*3, device->gpu_work_item_sizes, NULL);

    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_READ_IMAGE_ARGS, sizeof(cl_uint), &device->max_read_image_args, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, sizeof(cl_uint), &device->max_write_image_args, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_SAMPLERS, sizeof(cl_uint), &device->max_samplers, NULL);

    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(cl_uint), &device->max_constant_buffer_size, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_VENDOR, sizeof(char)*64, &device->vendor, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_TYPE, sizeof(cl_device_type), &device->type, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_VENDOR_ID, sizeof(cl_uint), &device->vendor_id, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_EXTENSIONS, sizeof(char)*2048, &device->extensions, NULL);

    if (verbosity == 1)
    {
        std::stringstream ss;
        ss << "CL_DEVICE_NAME:                       " << device->cl_device_name << std::endl;
        ss << "CL_DEVICE_VERSION:                    " << device->cl_device_version << std::endl;
        ss << "CL_DEVICE_TYPE:                       " << device->type << std::endl;
        ss << "CL_DRIVER_VERSION:                    " << device->cl_driver_version << std::endl;
        ss << "CL_DEVICE_VENDOR:                     " << device->vendor << std::endl;
        ss << "CL_DEVICE_VENDOR_ID:                  " << device->vendor_id << std::endl;
        ss << "CL_DEVICE_MAX_MEM_ALLOC_SIZE:         " << device->gpu_max_mem_alloc_size << std::endl;
        ss << "CL_DEVICE_MAX_CLOCK_FREQUENCY:        " << device->gpu_clock_max << std::endl;
        ss << "CL_DEVICE_GLOBAL_MEM_SIZE:            " << device->gpu_global_mem << std::endl;
        ss << "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:  " << device->gpu_global_mem_cache_line << std::endl;
        ss << "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:   " << device->max_constant_buffer_size << std::endl;
        ss << "CL_DEVICE_LOCAL_MEM_SIZE:             " << device->gpu_local_mem << std::endl;
        ss << "CL_DEVICE_MAX_COMPUTE_UNITS:          " << device->gpu_compute_units << std::endl;
        ss << "CL_DEVICE_MAX_WORK_GROUP_SIZE:        " << device->gpu_work_group_size << std::endl;
        ss << "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:   " << device->gpu_work_item_dim << std::endl;
        ss << "CL_DEVICE_IMAGE_SUPPORT:              " << device->gpu_image_support << std::endl;
        ss << "CL_DEVICE_MAX_READ_IMAGE_ARGS:        " << device->max_read_image_args << std::endl;
        ss << "CL_DEVICE_MAX_WRITE_IMAGE_ARGS:       " << device->max_write_image_args << std::endl;
        ss << "CL_DEVICE_MAX_SAMPLERS:               " << device->max_samplers << std::endl;
        ss << "CL_DEVICE_EXTENSIONS:               " << device->extensions << std::endl;
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"]\n"+ss.str().c_str());
    }
    return 1;
}

void ContextGLWidget::writeLog(QString str)
{
    writeToLogAndPrint(str.toStdString().c_str(), "riv.log", 1);
}

int ContextGLWidget::initResourcesCL()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    context = new cl_context;
    queue = new cl_command_queue;

    if (device->gpu_image_support != CL_TRUE)
    {
        std::cout << "Device lacks CL image support!"  << std::endl;
        return 0;
    }

    // Context with GL interopability
    #ifdef __linux__
	cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(),
        CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(),
        CL_CONTEXT_PLATFORM, (cl_context_properties) device->platform_id,
        0};
	#endif
	#ifdef _WIN32
	cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
        CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
        CL_CONTEXT_PLATFORM, (cl_context_properties) device->platform_id,
        0};
	#endif


    *context = clCreateContext(properties, 1, &device->device_id, NULL, NULL, &err);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
        return 0;
    }

    // Command queue
    *queue = clCreateCommandQueue(*context, device->device_id, 0, &err);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
        return 0;
    }
    return 1;
}
