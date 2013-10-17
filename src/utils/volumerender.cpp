#include "volumerender.h"

VolumeRenderWindow::VolumeRenderWindow()
    : isInitialized(false),
      isRayTexInitialized(false),
      isTsfTexInitialized(false),
      ray_tex_resolution(100)
{
    // Matrices
    double extent[8] = {
        -1.0,1.0,
        -1.0,1.0,
        -1.0,1.0,
         1.0,1.0};
    data_extent.setDeep(4, 2, extent);
    data_view_extent.setDeep(4, 2, extent);
    tsf_parameters.reserve(1,6);
    misc_ints.reserve(1,16);
    svo_misc_floats.reserve(1,16);
    model_misc_floats.reserve(1,16);

    // View matrices
    view_matrix.setIdentity(4);
    ctc_matrix.setIdentity(4);
    rotation.setIdentity(4);
    data_translation.setIdentity(4);
    data_scaling.setIdentity(4);
    bbox_scaling.setIdentity(4);
    bbox_translation.setIdentity(4);
    normalization_scaling.setIdentity(4);
    scalebar_view_matrix.setIdentity(4);
    scalebar_rotation.setIdentity(4);
    projection_scaling.setIdentity(4);

    double N = 0.1;
    double F = 10.0;
    double fov = 10.0;

    bbox_translation[11] = -N -(F - N)*0.5;
    ctc_matrix.setN(N);
    ctc_matrix.setF(F);
    ctc_matrix.setFov(fov);
    ctc_matrix.setProjection(0);

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

    // Color
    clear_color = Qt::white;
    clear_color_inverse = Qt::black;

    // Scalebar
    scalebar_coords.reserve(20000,3);
}

VolumeRenderWindow::~VolumeRenderWindow()
{
}

void VolumeRenderWindow::mouseMoveEvent(QMouseEvent* ev)
{
    float move_scaling = 1.0;
    if(ev->modifiers() & Qt::ControlModifier) move_scaling = 0.2;

    if ((ev->buttons() & Qt::LeftButton) && !(ev->buttons() & Qt::RightButton))
    {
        /* Rotation happens multiplicatively around a rolling axis given
         * by the mouse move direction and magnitude.
         * Moving the mouse alters rotation.
         * */

        double eta = std::atan2(ev->x() - last_mouse_pos_x, ev->y() - last_mouse_pos_y) - pi*1.0;
        double roll = move_scaling * pi/((float) size().height()) * std::sqrt((ev->x() - last_mouse_pos_x)*(ev->x() - last_mouse_pos_x) + (ev->y() - last_mouse_pos_y)*(ev->y() - last_mouse_pos_y));

        RotationMatrix<double> roll_rotation;
        roll_rotation.setArbRotation(-0.5*pi, eta, roll);

        if(ev->modifiers() & Qt::ShiftModifier) scalebar_rotation = roll_rotation * scalebar_rotation;
        else
        {
            rotation = roll_rotation * rotation;
            scalebar_rotation = roll_rotation * scalebar_rotation;
        }

        this->timerLastAction->start();
        this->isRefreshRequired = true;
    }
    if ((ev->buttons() & Qt::LeftButton) && (ev->buttons() & Qt::RightButton))
    {
        /* Rotation happens multiplicatively around a rolling axis given
         * by the mouse move direction and magnitude.
         * Moving the mouse alters rotation.
         * */

        RotationMatrix<double> roll_rotation;
        double roll = move_scaling * pi/((float) size().height()) * (ev->y() - last_mouse_pos_y);

        roll_rotation.setArbRotation(0, 0, roll);

        if(ev->modifiers() & Qt::ShiftModifier) scalebar_rotation = roll_rotation * scalebar_rotation;
        else
        {
            rotation = roll_rotation * rotation;
            scalebar_rotation = roll_rotation * scalebar_rotation;
        }

        this->timerLastAction->start();
        this->isRefreshRequired = true;
    }
    else if (ev->buttons() & Qt::MiddleButton)
    {
        /* X/Y translation happens multiplicatively. Here it is
         * important to retain the bounding box accordingly  */
        float dx = move_scaling * 2.0*(data_view_extent[1]-data_view_extent[0])/((float) size().height()) * (ev->x() - last_mouse_pos_x);
        float dy = move_scaling * -2.0*(data_view_extent[3]-data_view_extent[2])/((float) size().height()) * (ev->y() - last_mouse_pos_y);

        Matrix<double> data_translation_prev;
        data_translation_prev.setIdentity(4);
        data_translation_prev = data_translation;

        data_translation.setIdentity(4);
        data_translation[3] = dx;
        data_translation[7] = dy;

        data_translation = ( rotation.getInverse() * data_translation * rotation) * data_translation_prev;

        this->data_view_extent =  (data_scaling * data_translation).getInverse() * data_extent;
        this->timerLastAction->start();
        this->isRefreshRequired = true;
    }
    else if (!(ev->buttons() & Qt::LeftButton) && (ev->buttons() & Qt::RightButton))
    {
        /* Z translation happens multiplicatively */
        float dz = move_scaling * 2.0*(data_view_extent[5]-data_view_extent[4])/((float) size().height()) * (ev->y() - last_mouse_pos_y);

        Matrix<double> data_translation_prev;
        data_translation_prev.setIdentity(4);
        data_translation_prev = data_translation;

        data_translation.setIdentity(4);
        data_translation[11] = dz;

        data_translation = ( rotation.getInverse() * data_translation * rotation) * data_translation_prev;

        this->data_view_extent =  (data_scaling * data_translation).getInverse() * data_extent;

        this->timerLastAction->start();
        this->isRefreshRequired = true;
    }
    last_mouse_pos_x = ev->x();
    last_mouse_pos_y = ev->y();
}

void VolumeRenderWindow::wheelEvent(QWheelEvent* ev)
{
    float move_scaling = 1.0;
    if(ev->modifiers() & Qt::ShiftModifier) move_scaling = 5.0;
    else if(ev->modifiers() & Qt::ControlModifier) move_scaling = 0.2;


    double delta = move_scaling*((double)ev->delta())*0.0008;

    if (!(ev->buttons() & Qt::LeftButton) && !(ev->buttons() & Qt::RightButton))
    {
        if ((data_scaling[0] > 0.0001) || (delta > 0))
        {
            data_scaling[0] += data_scaling[0]*delta;
            data_scaling[5] += data_scaling[5]*delta;
            data_scaling[10] += data_scaling[10]*delta;

            data_view_extent =  (data_scaling * data_translation).getInverse() * data_extent;
            this->timerLastAction->start();
            this->isRefreshRequired = true;
        }
    }
    else
    {
        if ((bbox_scaling[0] > 0.0001) || (delta > 0))
        {
            bbox_scaling[0] += bbox_scaling[0]*delta;
            bbox_scaling[5] += bbox_scaling[5]*delta;
            bbox_scaling[10] += bbox_scaling[10]*delta;

            this->timerLastAction->start();
            this->isRefreshRequired = true;
        }
    }
}



void VolumeRenderWindow::initialize()
{
    initResourcesCL();

    // Textures
    setRayTexture();
    setTsfTexture();

    // Core set functions
    setDataExtent();
    setViewMatrix();
    setTsfParameters();
    setMiscArrays();

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

void VolumeRenderWindow::setViewMatrix()
{

    normalization_scaling[0] = bbox_scaling[0] * projection_scaling[0] * 2.0 / (data_extent[1] - data_extent[0]);
    normalization_scaling[5] = bbox_scaling[5] * projection_scaling[5] * 2.0 / (data_extent[3] - data_extent[2]);
    normalization_scaling[10] = bbox_scaling[10] * projection_scaling[10] * 2.0 / (data_extent[5] - data_extent[4]);

    view_matrix = ctc_matrix * bbox_translation * normalization_scaling * data_scaling * rotation * data_translation;
    scalebar_view_matrix = ctc_matrix * bbox_translation * normalization_scaling * data_scaling * scalebar_rotation * data_translation;

//    view_matrix.print(2, "view_matrix");
    scalebar_view_matrix.print(2, "scalebar_view_matrix");
    ctc_matrix.print(2, "ctc_matrix");
    bbox_translation.print(2, "bbox_tranlation");
    normalization_scaling.print(2, "normalization_scaling");
    data_scaling.print(2, "data_scaling");
    scalebar_rotation.print(2, "scalebar_rotation");
//    rotation.print(2, "rotation");
    data_translation.print(2, "data_translation");

//    view_matrix.getInverse().print(2, "view_matrix_inverse");

    err = clEnqueueWriteBuffer (*context_cl->getCommanQueue(),
        cl_view_matrix_inverse,
        CL_TRUE,
        0,
        view_matrix.bytes()/2,
        view_matrix.getInverse().toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clSetKernelArg(cl_model_raytrace, 3, sizeof(cl_mem), (void *) &cl_view_matrix_inverse);
    err |= clSetKernelArg(cl_svo_raytrace, 7, sizeof(cl_mem), (void *) &cl_view_matrix_inverse);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void VolumeRenderWindow::setDataExtent()
{
    err = clEnqueueWriteBuffer (*context_cl->getCommanQueue(),
        cl_data_extent,
        CL_TRUE,
        0,
        data_extent.bytes()/2,
        data_extent.toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clEnqueueWriteBuffer (*context_cl->getCommanQueue(),
        cl_data_view_extent,
        CL_TRUE,
        0,
        data_view_extent.bytes()/2,
        data_view_extent.toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clSetKernelArg(cl_model_raytrace, 4, sizeof(cl_mem),  &cl_data_extent);
    err |= clSetKernelArg(cl_model_raytrace, 5, sizeof(cl_mem), &cl_data_view_extent);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    err = clSetKernelArg(cl_svo_raytrace, 8, sizeof(cl_mem),  &cl_data_extent);
    err |= clSetKernelArg(cl_svo_raytrace, 9, sizeof(cl_mem), &cl_data_view_extent);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void VolumeRenderWindow::setTsfParameters()
{
    err = clEnqueueWriteBuffer (*context_cl->getCommanQueue(),
        cl_tsf_parameters,
        CL_TRUE,
        0,
        tsf_parameters.bytes()/2,
        tsf_parameters.toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clSetKernelArg(cl_model_raytrace, 6, sizeof(cl_mem), &cl_tsf_parameters);
    err |= clSetKernelArg(cl_svo_raytrace, 10, sizeof(cl_mem), &cl_tsf_parameters);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void VolumeRenderWindow::setMiscArrays()
{
    err = clEnqueueWriteBuffer (*context_cl->getCommanQueue(),
        cl_misc_ints,
        CL_TRUE,
        0,
        misc_ints.bytes(),
        misc_ints.data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clEnqueueWriteBuffer (*context_cl->getCommanQueue(),
        cl_model_misc_floats,
        CL_TRUE,
        0,
        model_misc_floats.bytes()/2,
        model_misc_floats.toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clEnqueueWriteBuffer (*context_cl->getCommanQueue(),
        cl_svo_misc_floats,
        CL_TRUE,
        0,
        svo_misc_floats.bytes()/2,
        svo_misc_floats.toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clSetKernelArg(cl_model_raytrace, 7, sizeof(cl_mem), &cl_model_misc_floats);
    err |= clSetKernelArg(cl_model_raytrace, 8, sizeof(cl_mem), &cl_misc_ints);
    err |= clSetKernelArg(cl_svo_raytrace, 11, sizeof(cl_mem), &cl_svo_misc_floats);
    err |= clSetKernelArg(cl_svo_raytrace, 12, sizeof(cl_mem), &cl_misc_ints);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void VolumeRenderWindow::resizeEvent(QResizeEvent * ev)
{
    ctc_matrix.setWindow(size().width(), size().height());
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
    setDataExtent();
    setViewMatrix();

    glClearColor(clear_color.redF(), clear_color.greenF(), clear_color.blueF(), 0.0f);

    painter->setPen(clear_color_inverse);
    painter->setFont(QFont("Courier"));

    painter->beginNativePainting();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    // Draw relative scalebar
    if (1)
    {
        scalebar_coord_count = setScaleBars();
//        std::cout << scalebar_coord_count << std::endl;
        shared_window->std_3d_color_program->bind();

        shared_window->std_3d_color_program->setUniformValue(shared_window->std_3d_color, clear_color.redF(), clear_color.greenF(), clear_color.blueF(), clear_color.alphaF());
        shared_window->std_3d_color_program->setUniformValueArray(shared_window->std_3d_transform, scalebar_view_matrix.getColMajor().toFloat().data(), scalebar_view_matrix.size(), 1);

        glVertexAttribPointer(shared_window->std_3d_fragpos, 3, GL_FLOAT, GL_FALSE, 0, scalebar_coords.data()); // This one maps to vec4 but is 3 long

        glEnableVertexAttribArray(shared_window->std_3d_fragpos);

        glDrawArrays(GL_LINES,  0, scalebar_coord_count);

        glDisableVertexAttribArray(shared_window->std_3d_fragpos);

        shared_window->std_3d_color_program->release();
    }

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

size_t VolumeRenderWindow::setScaleBars()
{
    // Draw the scalebars. The coordinates of the ticks are independent of the position in the volume, so it is a relative scalebar.
    double length = data_view_extent[1] - data_view_extent[0];

    double tick_interdistance_min = 0.005*length; // % of length

    int tick_levels = 0;
    int tick_levels_max = 2;

    size_t coord_counter = 0;

    // Draw ticks
    for (int i = 5; i >= -5; i--)
    {
        double tick_interdistance = std::pow((double) 10.0, (double) -i);

        if (( tick_interdistance >= tick_interdistance_min) && (tick_levels < tick_levels_max))
        {
            int tick_number = ((length*0.5)/ tick_interdistance);

            double x_start = data_view_extent[0] + length * 0.5;
            double y_start = data_view_extent[2] + length * 0.5;
            double z_start = data_view_extent[4] + length * 0.5;
            double tick_width = tick_interdistance*0.2;

            // Each tick consists of 4 points to form a cross
            for (int j = -tick_number; j <= tick_number; j++)
            {
                if (j != 0)
                {
                    // X-tick
                    scalebar_coords[(coord_counter+0)*3+0] = x_start + j * tick_interdistance;
                    scalebar_coords[(coord_counter+0)*3+1] = data_view_extent[2] + length * 0.5 + tick_width * 0.5;
                    scalebar_coords[(coord_counter+0)*3+2] = data_view_extent[4] + length * 0.5;

                    scalebar_coords[(coord_counter+1)*3+0] = x_start + j * tick_interdistance;
                    scalebar_coords[(coord_counter+1)*3+1] = data_view_extent[2] + length * 0.5 - tick_width * 0.5;
                    scalebar_coords[(coord_counter+1)*3+2] = data_view_extent[4] + length * 0.5;

                    scalebar_coords[(coord_counter+2)*3+0] = x_start + j * tick_interdistance;
                    scalebar_coords[(coord_counter+2)*3+1] = data_view_extent[2] + length * 0.5;
                    scalebar_coords[(coord_counter+2)*3+2] = data_view_extent[4] + length * 0.5 + tick_width * 0.5;

                    scalebar_coords[(coord_counter+3)*3+0] = x_start + j * tick_interdistance;
                    scalebar_coords[(coord_counter+3)*3+1] = data_view_extent[2] + length * 0.5;
                    scalebar_coords[(coord_counter+3)*3+2] = data_view_extent[4] + length * 0.5 - tick_width * 0.5;

                    // Y-tick
                    scalebar_coords[(coord_counter+4)*3+0] = data_view_extent[0] + length * 0.5 + tick_width * 0.5;
                    scalebar_coords[(coord_counter+4)*3+1] = y_start + j * tick_interdistance;
                    scalebar_coords[(coord_counter+4)*3+2] = data_view_extent[4] + length * 0.5;

                    scalebar_coords[(coord_counter+5)*3+0] = data_view_extent[0] + length * 0.5 - tick_width * 0.5;
                    scalebar_coords[(coord_counter+5)*3+1] = y_start + j * tick_interdistance;
                    scalebar_coords[(coord_counter+5)*3+2] = data_view_extent[4] + length * 0.5;

                    scalebar_coords[(coord_counter+6)*3+0] = data_view_extent[0] + length * 0.5;
                    scalebar_coords[(coord_counter+6)*3+1] = y_start + j * tick_interdistance;
                    scalebar_coords[(coord_counter+6)*3+2] = data_view_extent[4] + length * 0.5 + tick_width * 0.5;

                    scalebar_coords[(coord_counter+7)*3+0] = data_view_extent[0] + length * 0.5;
                    scalebar_coords[(coord_counter+7)*3+1] = y_start + j * tick_interdistance;
                    scalebar_coords[(coord_counter+7)*3+2] = data_view_extent[4] + length * 0.5 - tick_width * 0.5;

                    // Z-tick
                    scalebar_coords[(coord_counter+8)*3+0] = data_view_extent[0] + length * 0.5 + tick_width * 0.5;
                    scalebar_coords[(coord_counter+8)*3+1] = data_view_extent[2] + length * 0.5;
                    scalebar_coords[(coord_counter+8)*3+2] = z_start + j * tick_interdistance;

                    scalebar_coords[(coord_counter+9)*3+0] = data_view_extent[0] + length * 0.5 - tick_width * 0.5;
                    scalebar_coords[(coord_counter+9)*3+1] = data_view_extent[2] + length * 0.5;
                    scalebar_coords[(coord_counter+9)*3+2] = z_start + j * tick_interdistance;

                    scalebar_coords[(coord_counter+10)*3+0] = data_view_extent[0] + length * 0.5;
                    scalebar_coords[(coord_counter+10)*3+1] = data_view_extent[2] + length * 0.5 + tick_width * 0.5;
                    scalebar_coords[(coord_counter+10)*3+2] = z_start + j * tick_interdistance;

                    scalebar_coords[(coord_counter+11)*3+0] = data_view_extent[0] + length * 0.5;
                    scalebar_coords[(coord_counter+11)*3+1] = data_view_extent[2] + length * 0.5 - tick_width * 0.5;
                    scalebar_coords[(coord_counter+11)*3+2] = z_start + j * tick_interdistance;


                    // Text
//                    if(tick_levels == tick_levels_max - 1)
//                    {
//                        Matrix<float> xy(2,1);
//                        getScreenPosition(xy.data(), scalebar_coords.data() + (coord_counter+0)*3, SCALEBAR_MATRIX.data());
//                        std_text_draw(QString::number(j * 0.1).toStdString().c_str(), fontSmall, clearInv.data(), xy.data(), 1.0, this->WIDTH, this->HEIGHT);

//                        getScreenPosition(xy.data(), scalebar_coords.data() + (coord_counter+4)*3, SCALEBAR_MATRIX.data());
//                        std_text_draw(QString::number(j * 0.1).toStdString().c_str(), fontSmall, clearInv.data(), xy.data(), 1.0, this->WIDTH, this->HEIGHT);

//                        getScreenPosition(xy.data(), scalebar_coords.data() + (coord_counter+8)*3, SCALEBAR_MATRIX.data());
//                        std_text_draw(QString::number(j * 0.1).toStdString().c_str(), fontSmall, clearInv.data(), xy.data(), 1.0, this->WIDTH, this->HEIGHT);

//                        if(!isMultiplierDrawn)
//                        {
//                            Matrix<float> xy(2,1);
//                            xy[0] = 0.5;
//                            xy[1] = -0.5;
//                            std_text_draw(QString("x "+QString::number(tick_interdistance * 10.0, 'e', 0)).toStdString().c_str(), fontMedium, clearInv.data(), xy.data(), 1.0, this->WIDTH, this->HEIGHT);
//                            isMultiplierDrawn = true;
//                        }
//                    }
                    coord_counter += 12;
                }
            }
            tick_levels++;
        }
    }

    // Cross
    scalebar_coords[(coord_counter+0)*3+0] = data_view_extent[0];
    scalebar_coords[(coord_counter+0)*3+1] = data_view_extent[2] + length * 0.5;
    scalebar_coords[(coord_counter+0)*3+2] = data_view_extent[4] + length * 0.5;

    scalebar_coords[(coord_counter+1)*3+0] = data_view_extent[1];
    scalebar_coords[(coord_counter+1)*3+1] = data_view_extent[2] + length * 0.5;
    scalebar_coords[(coord_counter+1)*3+2] = data_view_extent[4] + length * 0.5;

    scalebar_coords[(coord_counter+2)*3+0] = data_view_extent[0] + length * 0.5;
    scalebar_coords[(coord_counter+2)*3+1] = data_view_extent[2];
    scalebar_coords[(coord_counter+2)*3+2] = data_view_extent[4] + length * 0.5;

    scalebar_coords[(coord_counter+3)*3+0] = data_view_extent[0] + length * 0.5;
    scalebar_coords[(coord_counter+3)*3+1] = data_view_extent[3];
    scalebar_coords[(coord_counter+3)*3+2] = data_view_extent[4] + length * 0.5;

    scalebar_coords[(coord_counter+4)*3+0] = data_view_extent[0] + length * 0.5;
    scalebar_coords[(coord_counter+4)*3+1] = data_view_extent[2] + length * 0.5;
    scalebar_coords[(coord_counter+4)*3+2] = data_view_extent[4];

    scalebar_coords[(coord_counter+5)*3+0] = data_view_extent[0] + length * 0.5;
    scalebar_coords[(coord_counter+5)*3+1] = data_view_extent[2] + length * 0.5;
    scalebar_coords[(coord_counter+5)*3+2] = data_view_extent[5];

    coord_counter += 6;
    return coord_counter;
}
