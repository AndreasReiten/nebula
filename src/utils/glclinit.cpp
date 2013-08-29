#include "glclinit.h"

InitGLCLWidget::InitGLCLWidget(const QGLFormat & format, QWidget * parent) :
    QGLWidget(format, parent)
{
    std::cout << "Constructing InitGLCLWidget" << std::endl;
    isGLIntitialized = false;
    std::cout << "Done Constructing InitGLCLWidget" << std::endl;
}

InitGLCLWidget::~InitGLCLWidget()
{
    if (isGLIntitialized)
    {
        std::cout << "CL/GL: DESTRUCTION!" << std::endl;
        if (*queue) clReleaseCommandQueue(*queue);
        if (*context2) clReleaseContext(*context2);
        std::cout << "CL/GL: DESTROYED!" << std::endl;
    }
}

cl_command_queue * InitGLCLWidget::getCLCommandQueue()
{
    if (isGLIntitialized) return queue;
    else return NULL;
}
cl_device * InitGLCLWidget::getCLDevice()
{
    if (isGLIntitialized) return device;
    else return NULL;
}
cl_context * InitGLCLWidget::getCLContext()
{
    if (isGLIntitialized) return context2;
    else return NULL;
}

void InitGLCLWidget::initializeGL()
{
    std::cout << "Initializing OpenGL and CL" << std::endl;

    if (!this->init_gl()) std::cout << "Error initializing OpenGL" << std::endl;
    if (!this->init_cl_device()) std::cout << "Error initializing OpenCL Device" << std::endl;
    if (!this->init_cl()) std::cout << "Error initializing OpenCL" << std::endl;
    //~ std::cout << "INITGLCL Render: Alpha Channel = " << this->context()->format().alpha() << std::endl;
    std::cout << "Done Initializing OpenGL and CL " << std::endl;
}



void InitGLCLWidget::paintGL()
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    //~ std::cout << "INITGLCL Render: Alpha Channel = " << this->context()->format().alpha() << std::endl;
}




void InitGLCLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}


void InitGLCLWidget::setMessageString(QString str)
{
        emit changedMessageString(str);
}


int InitGLCLWidget::init_gl()
{
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

    isGLIntitialized = true;
    
    // BLEND
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
    
    return 1;
}

int InitGLCLWidget::init_cl_device()
{
    // DEVICE & INFO
    device = new cl_device;
    
    err = clGetPlatformIDs(1, &device->platform_id, &num_platforms);
    if ( err != CL_SUCCESS)
    {
        std::cout << "Could not establish a CL platform: " << cl_error_cstring(err) << std::endl;
        return 0;
    }
    err = clGetDeviceIDs( device->platform_id, CL_DEVICE_TYPE_GPU, 1, &device->device_id, &num_devices);
    if (err != CL_SUCCESS)
    {
        std::cout << "Could not get device IDs: " << cl_error_cstring(err) << std::endl;
        return 0;
    }
    clGetDeviceInfo(device->device_id, CL_DEVICE_NAME, sizeof(device->cl_device_name)*8, &device->cl_device_name, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_VERSION, sizeof(device->cl_device_version)*8, &device->cl_device_version, NULL);
    clGetDeviceInfo(device->device_id, CL_DRIVER_VERSION, sizeof(device->cl_driver_version)*8, &device->cl_driver_version, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(device->gpu_max_mem_alloc_size)*8, &device->gpu_max_mem_alloc_size, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(device->gpu_clock_max)*8, &device->gpu_clock_max, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(device->gpu_global_mem)*8, &device->gpu_global_mem, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(device->gpu_global_mem_cache_line)*8, &device->gpu_global_mem_cache_line, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(device->gpu_local_mem)*8, &device->gpu_local_mem, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(device->gpu_compute_units)*8, &device->gpu_compute_units, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(device->gpu_work_group_size)*8, &device->gpu_work_group_size, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(device->gpu_work_item_dim)*8, &device->gpu_work_item_dim, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_IMAGE_SUPPORT, sizeof(device->gpu_image_support)*8, &device->gpu_image_support, NULL);
    clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(device->gpu_work_item_sizes)*8, device->gpu_work_item_sizes, NULL);
    
    return 1;
}

int InitGLCLWidget::init_cl()
{
    context2 = new cl_context;
    queue = new cl_command_queue;
     
    if (device->gpu_image_support != CL_TRUE)
    {
        std::cout << "Device lacks CL image support!"  << std::endl;
        return 0;
    }
     
    // Context with GL interopability
    #ifdef __linux__
	cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(), //AAAAW...MAYBE THIS IS BAD?
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
      
     
    *context2 = clCreateContext(properties, 1, &device->device_id, NULL, NULL, &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "MainWindow: Could not establish CL context: " << cl_error_cstring(err) << std::endl;
        return 0;
    }
      
    // Command queue
    *queue = clCreateCommandQueue(*context2, device->device_id, 0, &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "MainWindow: Could not create command queue: " << cl_error_cstring(err) << std::endl;
        return 0;
    }
    return 1;
}
