#include "volumerender.h"

VolumeRenderWindow::VolumeRenderWindow()
    : isInitialized(false),
      isRayTexInitialized(false),
      isTsfTexInitialized(false),
      ray_tex_resolution(100)
{
    // Matrices
    view_matrix.reserve(1,16);
    data_extent.reserve(1, 8);
    data_view_extent.reserve(1,8);
    tsf_parameters.reserve(1,6);
    misc_ints.reserve(1,16);
    svo_misc_floats.reserve(1,16);
    model_misc_floats.reserve(1,16);

    // Ray tex
    ray_tex_dim.reserve(1, 2);
    ray_glb_ws.reserve(1,2);
    ray_loc_ws.reserve(1,2);

    // Ray texture timing
    isBadCall = true;
    isRefreshRequired = true;
    fps_required = 60;
    timerLastAction = new QElapsedTimer;
    callTimer = new QElapsedTimer;
}

VolumeRenderWindow::~VolumeRenderWindow()
{
}

void VolumeRenderWindow::mouseMoveEvent(QMouseEvent* ev)
{
//    std::cout << ev->x() << " " << ev->y() << std::endl;
}

void VolumeRenderWindow::wheelEvent(QWheelEvent* ev)
{
//    std::cout << ev->delta() << std::endl;
}



void VolumeRenderWindow::initialize()
{
    initResourcesCL();
    setRayTexture();
    setTsfTexture();
    isInitialized = true;
}

void VolumeRenderWindow::initResourcesCL()
{
    // Build program from OpenCL kernel source
    program = context_cl->createProgram("cl_kernels/render.cl", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    context_cl->buildProgram(&program, "-Werror");


    // Kernel handles
    cl_svo_raytrace = clCreateKernel(program, "svoRayTrace", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    cl_model_raytrace = clCreateKernel(program, "modelRayTrace", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    // Buffers
    cl_view_matrix_inverse = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        view_matrix.toFloat().bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    cl_data_extent = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        data_extent.toFloat().bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    cl_data_view_extent = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        data_view_extent.toFloat().bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    cl_tsf_parameters = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        tsf_parameters.toFloat().bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    cl_misc_ints = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        misc_ints.toFloat().bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    cl_svo_misc_floats = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        svo_misc_floats.toFloat().bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    cl_model_misc_floats = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        model_misc_floats.toFloat().bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

}

void VolumeRenderWindow::resizeEvent(QResizeEvent * ev)
{
    setRayTexture();
}

void VolumeRenderWindow::setRayTexture()
{
    // Set a texture for the volume rendering kernel
    ray_tex_dim[0] = (int)((float)this->size().width()*ray_tex_resolution*0.01f);
    ray_tex_dim[1] = (int)((float)this->size().height()*ray_tex_resolution*0.01f);

    // Clamp
    if (ray_tex_dim[0] < 32) ray_tex_dim[0] = 32;
    if (ray_tex_dim[1] < 32) ray_tex_dim[1] = 32;

    // Local work size
    ray_loc_ws[0] = 16;
    ray_loc_ws[1] = 16;

    // Global work size
    if (ray_tex_dim[0] % ray_loc_ws[0]) ray_glb_ws[0] = ray_loc_ws[0]*(1 + (ray_tex_dim[0] / ray_loc_ws[0]));
    else ray_glb_ws[0] = ray_tex_dim[0];
    if (ray_tex_dim[1] % ray_loc_ws[1]) ray_glb_ws[1] = ray_loc_ws[1]*(1 + (ray_tex_dim[1] / ray_loc_ws[1]));
    else ray_glb_ws[1] = ray_tex_dim[1];

    if (isRayTexInitialized) clReleaseMemObject(ray_tex_cl);

    // Update GL texture
//    glActiveTexture(GL_TEXTURE0);
    glDeleteTextures(1, &ray_tex_gl);
    glGenTextures(1, &ray_tex_gl);
    glBindTexture(GL_TEXTURE_2D, ray_tex_gl);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        ray_tex_dim[0],
        ray_tex_dim[1],
        0,
        GL_RGBA,
        GL_FLOAT,
        NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Convert to CL texture
    ray_tex_cl = clCreateFromGLTexture2D(*context_cl->getContext(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, ray_tex_gl, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    isRayTexInitialized = true;

    // Pass texture to CL kernel
    if (isInitialized) err = clSetKernelArg(cl_svo_raytrace, 0, sizeof(cl_mem), (void *) &ray_tex_cl);
    if (isInitialized) err |= clSetKernelArg(cl_model_raytrace, 0, sizeof(cl_mem), (void *) &ray_tex_cl);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void VolumeRenderWindow::setTsfTexture()
{
    if (isTsfTexInitialized) clReleaseSampler(tsf_tex_sampler);
    if (isTsfTexInitialized) clReleaseMemObject(tsf_tex_cl);

    tsf.setColorScheme(0,0);
    tsf.setSpline(256);
//    tsf.getSplined().print(2);

    // Buffer for tsf_tex_gl
    glActiveTexture(GL_TEXTURE0);
    glDeleteTextures(1, &tsf_tex_gl);
    glGenTextures(1, &tsf_tex_gl);
    glBindTexture(GL_TEXTURE_2D, tsf_tex_gl);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        tsf.getSplined().getN(),
        1,
        0,
        GL_RGBA,
        GL_FLOAT,
        tsf.getSplined().getColMajor().toFloat().data());
    glBindTexture(GL_TEXTURE_2D, 0);

    // Buffer for tsf_tex_cl
    //~ tsf->getPreIntegrated().getColMajor().toFloat().print(2, "preIntegrated");
    cl_image_format tsf_format;
    tsf_format.image_channel_order = CL_RGBA;
    tsf_format.image_channel_data_type = CL_FLOAT;

    tsf_tex_cl = clCreateImage2D ( *context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        &tsf_format,
        tsf.getSplined().getN(),
        1,
        0,
        tsf.getSplined().getColMajor().toFloat().data(),
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // The sampler for tsf_tex_cl
    tsf_tex_sampler = clCreateSampler(*context_cl->getContext(), true, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    isTsfTexInitialized = true;

    // Set corresponding kernel arguments
    if (isInitialized) err = clSetKernelArg(cl_svo_raytrace, 1, sizeof(cl_mem), (void *) &tsf_tex_cl);
    if (isInitialized) err |= clSetKernelArg(cl_svo_raytrace, 6, sizeof(cl_sampler), &tsf_tex_sampler);
    if (isInitialized) err |= clSetKernelArg(cl_model_raytrace, 1, sizeof(cl_mem), (void *) &tsf_tex_cl);
    if (isInitialized) err |= clSetKernelArg(cl_model_raytrace, 2, sizeof(cl_sampler), &tsf_tex_sampler);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}


void VolumeRenderWindow::setSharedWindow(SharedContextWindow * window)
{
    this->shared_window = window;
    shared_context = window->getGLContext();
}

void VolumeRenderWindow::render(QPainter *painter)
{
    painter->setPen(Qt::yellow);
    painter->setFont(QFont("Monospace"));

    painter->beginNativePainting();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    // Draw raytracing texture
    shared_window->std_2d_tex_program->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tsf_tex_gl);
    shared_window->std_2d_tex_program->setUniformValue(shared_window->std_2d_texture, 0);


    GLfloat fragpos[] = {
        -1.0, -1.0,
        1.0, -1.0,
        1.0, 0.0,
        -1.0, 0.0
    };

    GLfloat texpos[] = {
        0.0, 0.0,
        1.0, 0.0,
        1.0, 1.0,
        0.0, 1.0
    };

    GLuint indices[] = {0,1,3,1,2,3};

    glVertexAttribPointer(shared_window->std_2d_fragpos, 2, GL_FLOAT, GL_FALSE, 0, fragpos);
    glVertexAttribPointer(shared_window->std_2d_texpos, 2, GL_FLOAT, GL_FALSE, 0, texpos);

    glEnableVertexAttribArray(shared_window->std_2d_fragpos);
    glEnableVertexAttribArray(shared_window->std_2d_texpos);

    glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);

    glDisableVertexAttribArray(shared_window->std_2d_texpos);
    glDisableVertexAttribArray(shared_window->std_2d_fragpos);
    glBindTexture(GL_TEXTURE_2D, 0);

    shared_window->std_2d_tex_program->release();

    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);

    painter->endNativePainting();

    painter->drawText(50, 50, QString("Qt is a cross-platform application and UI framework for developers using C++ or QML, a CSS & JavaScript like language. Qt Creator is the supporting Qt IDE."));

    painter->drawText(50, 100, QString::number(getFps()));

}

void VolumeRenderWindow::raytrace(cl_kernel kernel)
{
    //TODO: This should be done in a separate, non-blocking, thread. Could also use a much less costly kernel to measure processing cost (number of texture fetches) and adjust resolution and update time requirement accordingly. Separate threading would make the rendering appear quite fast, but the ray texture might in fact lag behind a bit. Still a vast improvement over UI blocking.

    if (isRefreshRequired)
    {
        callTimer->start();
        glFinish();
        this->isRefreshRequired = false;

        // Aquire shared CL/GL objects
        err = clEnqueueAcquireGLObjects(*context_cl->getCommanQueue(), 1, &ray_tex_cl, 0, 0, 0);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        // Launch rendering kernel
        size_t area_per_call[2] = {128, 128};
        size_t call_offset[2] = {0,0};
        callTimeMax = 1000/fps_required; // Dividend is FPS
        timeLastActionMin = 1000;
        isBadCall = false;

        for (size_t glb_x = 0; glb_x < ray_glb_ws[0]; glb_x += area_per_call[0])
        {
            if (isBadCall) break;

            for (size_t glb_y = 0; glb_y < ray_glb_ws[1]; glb_y += area_per_call[1])
            {
                if ((timerLastAction->elapsed() < timeLastActionMin) && (callTimer->elapsed() > callTimeMax))
                {
                    isBadCall = true;
                    break;
                }
                call_offset[0] = glb_x;
                call_offset[1] = glb_y;

                err = clEnqueueNDRangeKernel(*context_cl->getCommanQueue(), kernel, 2, call_offset, area_per_call, ray_loc_ws.data(), 0, NULL, NULL);
                if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

                err = clFinish(*context_cl->getCommanQueue());
                if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
            }
        }

        // Release shared CL/GL objects
        err = clEnqueueReleaseGLObjects(*context_cl->getCommanQueue(), 1, &ray_tex_cl, 0, 0, 0);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        err = clFinish(*context_cl->getCommanQueue());
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
}
