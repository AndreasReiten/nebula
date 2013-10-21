#include "volumerender.h"

VolumeRenderWindow::VolumeRenderWindow()
    : isInitialized(false),
      isRayTexInitialized(false),
      isTsfTexInitialized(false),
      isDSActive(false),
      isOrthonormal(true),
      isLogarithmic(true),
      isModelActive(true),
      isUnitcellActive(false),
      isSvoInitialized(false),
      isScalebarActive(true),
      ray_tex_resolution(20)
{
    // Matrices
    double extent[8] = {
        -2.0*pi,2.0*pi,
        -2.0*pi,2.0*pi,
        -2.0*pi,2.0*pi,
         1.0,1.0};
    data_extent.setDeep(4, 2, extent);
    data_view_extent.setDeep(4, 2, extent);
    tsf_parameters_svo.set(1,6, 0.0);
    tsf_parameters_model.set(1,6, 0.0);
    misc_ints.set(1,16, 0.0);
    model_misc_floats.set(1,16, 0.0);

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
    ctc_matrix.setProjection(isOrthonormal);

    // Ray tex
    ray_tex_dim.reserve(1, 2);
    ray_glb_ws.reserve(1,2);
    ray_loc_ws.reserve(1,2);
    ray_loc_ws[0] = 16;
    ray_loc_ws[1] = 16;

    // Transfer texture
    tsf_color_scheme = 0;
    tsf_alpha_scheme = 0;

    tsf_parameters_model[0] = 0.0; // texture min
    tsf_parameters_model[1] = 1.0; // texture max
//    tsf_parameters_model[4] = 0.5; // alpha
//    tsf_parameters_model[5] = 2.0; // brightness

    tsf_parameters_svo[0] = 0.0; // texture min
    tsf_parameters_svo[1] = 1.0; // texture max
    tsf_parameters_svo[4] = 0.5; // alpha
    tsf_parameters_svo[5] = 2.0; // brightness

    // Ray texture timing
    fps_required = 60;
    work = 1.0;
    work_time = 0.0;
    quality_factor = 1.0;

    // Scalebar
    scalebar_ticks.reserve(100,3);

    // Color
    GLfloat white_buf[] = {1,1,1,0.4};
    GLfloat black_buf[] = {0,0,0,0.4};
    white.setDeep(1,4,white_buf);
    black.setDeep(1,4,black_buf);
    clear_color = white;
    clear_color_inverse = black;

    // Fps
    fps_string_width_prev = 0;
}

VolumeRenderWindow::~VolumeRenderWindow()
{
    if (isInitialized) glDeleteBuffers(1, &scalebar_vbo);
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

        double eta = std::atan2((double)ev->x() - last_mouse_pos_x, (double)ev->y() - last_mouse_pos_y) - pi*1.0;
        double roll = move_scaling * pi/((float) height()) * std::sqrt((double)(ev->x() - last_mouse_pos_x)*(ev->x() - last_mouse_pos_x) + (ev->y() - last_mouse_pos_y)*(ev->y() - last_mouse_pos_y));

        RotationMatrix<double> roll_rotation;
        roll_rotation.setArbRotation(-0.5*pi, eta, roll);

        if(ev->modifiers() & Qt::ShiftModifier) scalebar_rotation = roll_rotation * scalebar_rotation;
        else
        {
            rotation = roll_rotation * rotation;
            scalebar_rotation = roll_rotation * scalebar_rotation;
        }
    }
    if ((ev->buttons() & Qt::LeftButton) && (ev->buttons() & Qt::RightButton))
    {
        /* Rotation happens multiplicatively around a rolling axis given
         * by the mouse move direction and magnitude.
         * Moving the mouse alters rotation.
         * */

        RotationMatrix<double> roll_rotation;
        double roll = move_scaling * pi/((float) height()) * (ev->y() - last_mouse_pos_y);

        roll_rotation.setArbRotation(0, 0, roll);

        if(ev->modifiers() & Qt::ShiftModifier) scalebar_rotation = roll_rotation * scalebar_rotation;
        else
        {
            rotation = roll_rotation * rotation;
            scalebar_rotation = roll_rotation * scalebar_rotation;
        }
    }
    else if (ev->buttons() & Qt::MiddleButton)
    {
        /* X/Y translation happens multiplicatively. Here it is
         * important to retain the bounding box accordingly  */
        float dx = move_scaling * 2.0*(data_view_extent[1]-data_view_extent[0])/((float) height()) * (ev->x() - last_mouse_pos_x);
        float dy = move_scaling * -2.0*(data_view_extent[3]-data_view_extent[2])/((float) height()) * (ev->y() - last_mouse_pos_y);

        Matrix<double> data_translation_prev;
        data_translation_prev.setIdentity(4);
        data_translation_prev = data_translation;

        data_translation.setIdentity(4);
        data_translation[3] = dx;
        data_translation[7] = dy;

        data_translation = ( rotation.getInverse() * data_translation * rotation) * data_translation_prev;

        this->data_view_extent =  (data_scaling * data_translation).getInverse() * data_extent;
    }
    else if (!(ev->buttons() & Qt::LeftButton) && (ev->buttons() & Qt::RightButton))
    {
        /* Z translation happens multiplicatively */
        float dz = move_scaling * 2.0*(data_view_extent[5]-data_view_extent[4])/((float) height()) * (ev->y() - last_mouse_pos_y);

        Matrix<double> data_translation_prev;
        data_translation_prev.setIdentity(4);
        data_translation_prev = data_translation;

        data_translation.setIdentity(4);
        data_translation[11] = dz;

        data_translation = ( rotation.getInverse() * data_translation * rotation) * data_translation_prev;

        this->data_view_extent =  (data_scaling * data_translation).getInverse() * data_extent;
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
        }
    }
    else
    {
        if ((bbox_scaling[0] > 0.0001) || (delta > 0))
        {
            bbox_scaling[0] += bbox_scaling[0]*delta;
            bbox_scaling[5] += bbox_scaling[5]*delta;
            bbox_scaling[10] += bbox_scaling[10]*delta;
        }
    }
}



void VolumeRenderWindow::initialize()
{
    initResourcesCL();
    initResourcesGL();

    isInitialized = true;

    // Textures
    setRayTexture();
    setTsfTexture();

    // Core set functions
    setDataExtent();
    setViewMatrix();
    setTsfParameters();
    setMiscArrays();

    // Pens
    initializePaintTools();
}

void VolumeRenderWindow::initializePaintTools()
{
    normal_pen = new QPen;
    normal_pen->setWidth(1);
    border_pen = new QPen;
    border_pen->setWidth(1);

    normal_font = new QFont();
//    normal_font->setStyleHint(QFont::Monospace);
    emph_font = new QFont;
    emph_font->setBold(true);

    normal_fontmetric = new QFontMetrics(*normal_font, paint_device_gl);
    emph_fontmetric = new QFontMetrics(*emph_font, paint_device_gl);

    fill_brush = new QBrush;
    fill_brush->setStyle(Qt::SolidPattern);
    fill_brush->setColor(QColor(255,255,255,155));

    normal_brush = new QBrush;
    normal_brush->setStyle(Qt::NoBrush);

    dark_fill_brush = new QBrush;
    dark_fill_brush->setStyle(Qt::SolidPattern);
    dark_fill_brush->setColor(QColor(0,0,0,255));
}

void VolumeRenderWindow::initResourcesGL()
{
    glGenBuffers(1, &scalebar_vbo);
}

void VolumeRenderWindow::initResourcesCL()
{
    // Build program from OpenCL kernel source
    Matrix<const char *> paths(1,3);
    paths[0] = "cl_kernels/render_shared.cl";
    paths[1] = "cl_kernels/render_svo.cl";
    paths[2] = "cl_kernels/render_model.cl";

    program = context_cl->createProgram(&paths, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    context_cl->buildProgram(&program, "-Werror");


    // Kernel handles
    cl_svo_raytrace = clCreateKernel(program, "svoRayTrace", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_svo_workload = clCreateKernel(program, "svoWorkload", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    cl_model_raytrace = clCreateKernel(program, "modelRayTrace", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_model_workload = clCreateKernel(program, "modelWorkload", &err);
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


    cl_tsf_parameters_svo = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        tsf_parameters_svo.toFloat().bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_tsf_parameters_model = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        tsf_parameters_model.toFloat().bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_misc_ints = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        misc_ints.toFloat().bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_model_misc_floats = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        model_misc_floats.toFloat().bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Enough for a 6000 x 6000 pixel texture
    cl_glb_work = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
        (6000*6000)/(ray_loc_ws[0]*ray_loc_ws[1])*sizeof(cl_float),
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

    err = clEnqueueWriteBuffer (*context_cl->getCommanQueue(),
        cl_view_matrix_inverse,
        CL_TRUE,
        0,
        view_matrix.bytes()/2,
        view_matrix.getInverse().toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clSetKernelArg(cl_model_raytrace, 3, sizeof(cl_mem), (void *) &cl_view_matrix_inverse);
    err = clSetKernelArg(cl_model_workload, 3, sizeof(cl_mem), (void *) &cl_view_matrix_inverse);

    err |= clSetKernelArg(cl_svo_raytrace, 7, sizeof(cl_mem), (void *) &cl_view_matrix_inverse);
    err |= clSetKernelArg(cl_svo_workload, 5, sizeof(cl_mem), (void *) &cl_view_matrix_inverse);
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
    err |= clSetKernelArg(cl_model_workload, 4, sizeof(cl_mem), &cl_data_view_extent);
    err |= clSetKernelArg(cl_model_workload, 5, sizeof(cl_mem), &cl_data_view_extent);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    err = clSetKernelArg(cl_svo_raytrace, 8, sizeof(cl_mem),  &cl_data_extent);
    err |= clSetKernelArg(cl_svo_raytrace, 9, sizeof(cl_mem), &cl_data_view_extent);
    err = clSetKernelArg(cl_svo_workload, 6, sizeof(cl_mem),  &cl_data_extent);
    err |= clSetKernelArg(cl_svo_workload, 7, sizeof(cl_mem), &cl_data_view_extent);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void VolumeRenderWindow::setTsfParameters()
{
    err = clEnqueueWriteBuffer (*context_cl->getCommanQueue(),
        cl_tsf_parameters_model,
        CL_TRUE,
        0,
        tsf_parameters_model.bytes()/2,
        tsf_parameters_model.toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clEnqueueWriteBuffer (*context_cl->getCommanQueue(),
        cl_tsf_parameters_svo,
        CL_TRUE,
        0,
        tsf_parameters_svo.bytes()/2,
        tsf_parameters_svo.toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clSetKernelArg(cl_model_raytrace, 6, sizeof(cl_mem), &cl_tsf_parameters_model);
    err |= clSetKernelArg(cl_svo_raytrace, 10, sizeof(cl_mem), &cl_tsf_parameters_svo);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void VolumeRenderWindow::setMiscArrays()
{
    misc_ints[2] = isLogarithmic;
    misc_ints[3] = isDSActive;
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

    err = clSetKernelArg(cl_model_raytrace, 7, sizeof(cl_mem), &cl_model_misc_floats);
    err |= clSetKernelArg(cl_model_raytrace, 8, sizeof(cl_mem), &cl_misc_ints);
    err |= clSetKernelArg(cl_svo_raytrace, 11, sizeof(cl_mem), &cl_misc_ints);
    err |= clSetKernelArg(cl_svo_workload, 8, sizeof(cl_mem), &cl_misc_ints);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void VolumeRenderWindow::resizeEvent(QResizeEvent * ev)
{
    Q_UNUSED(ev);

    if (paint_device_gl) paint_device_gl->setSize(size());
    ctc_matrix.setWindow(width(), height());
    setRayTexture();
}

void VolumeRenderWindow::setRayTexture()
{
    // Set a texture for the volume rendering kernel
    Matrix<int> ray_tex_new(1, 2);
    ray_tex_new[0] = (int)((float)this->width()*(ray_tex_resolution*0.01)*std::sqrt(quality_factor));
    ray_tex_new[1] = (int)((float)this->height()*(ray_tex_resolution*0.01)*std::sqrt(quality_factor));

    // Clamp
    if (ray_tex_new[0] < 32) ray_tex_new[0] = 32;
    if (ray_tex_new[1] < 32) ray_tex_new[1] = 32;

    if (ray_tex_new[0] > width()) ray_tex_new[0] = width();
    if (ray_tex_new[1] > height()) ray_tex_new[1] = height();

    // Only resize the texture if the change is somewhat significant (in area cahnged)
    double new_area = ray_tex_new[0]*ray_tex_new[1];
    double area = ray_tex_dim[0]*ray_tex_dim[1];

    if (isInitialized && ((!isRayTexInitialized) || ((std::abs(new_area - area) / area) > 0.15)))
    {
        // Calculate the actula quality factor multiplier
        quality_factor = std::pow((double) ray_tex_new[0] / ((double)this->width()*(ray_tex_resolution*0.01)), 2.0);

        ray_tex_resolution *= std::sqrt(quality_factor);
        ray_tex_dim = ray_tex_new;

        // Global work size
        if (ray_tex_dim[0] % ray_loc_ws[0]) ray_glb_ws[0] = ray_loc_ws[0]*(1 + (ray_tex_dim[0] / ray_loc_ws[0]));
        else ray_glb_ws[0] = ray_tex_dim[0];
        if (ray_tex_dim[1] % ray_loc_ws[1]) ray_glb_ws[1] = ray_loc_ws[1]*(1 + (ray_tex_dim[1] / ray_loc_ws[1]));
        else ray_glb_ws[1] = ray_tex_dim[1];

        if (isRayTexInitialized){
            err = clReleaseMemObject(ray_tex_cl);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        }

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
}

void VolumeRenderWindow::setTsfTexture()
{
    if (isTsfTexInitialized){
        err = clReleaseSampler(tsf_tex_sampler);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
    if (isTsfTexInitialized){
        err = clReleaseMemObject(tsf_tex_cl);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }

    tsf.setColorScheme(tsf_color_scheme, tsf_alpha_scheme);
    tsf.setSpline(256);

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
        tsf.getSplined()->getN(),
        1,
        0,
        GL_RGBA,
        GL_FLOAT,
        tsf.getSplined()->getColMajor().toFloat().data());
    glBindTexture(GL_TEXTURE_2D, 0);

    // Buffer for tsf_tex_gl
    glDeleteTextures(1, &tsf_tex_gl_thumb);
    glGenTextures(1, &tsf_tex_gl_thumb);
    glBindTexture(GL_TEXTURE_2D, tsf_tex_gl_thumb);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        tsf.getThumb()->getN(),
        1,
        0,
        GL_RGB,
        GL_FLOAT,
        tsf.getThumb()->getColMajor().toFloat().data());
    glBindTexture(GL_TEXTURE_2D, 0);

    // Buffer for tsf_tex_cl
    cl_image_format tsf_format;
    tsf_format.image_channel_order = CL_RGBA;
    tsf_format.image_channel_data_type = CL_FLOAT;

    tsf_tex_cl = clCreateImage2D ( *context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        &tsf_format,
        tsf.getSplined()->getN(),
        1,
        0,
        tsf.getSplined()->getColMajor().toFloat().data(),
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

    glClearColor(clear_color[0], clear_color[1], clear_color[2], 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


    beginRawGLCalls(painter);
    glLineWidth(1.5);
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    // Draw relative scalebar
    if (isScalebarActive) drawScalebars();

    // Draw raytracing texture
    drawRayTex();
    endRawGLCalls(painter);

    // Draw overlay
    drawOverlay(painter);

}


void VolumeRenderWindow::beginRawGLCalls(QPainter * painter)
{
    painter->beginNativePainting();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
}

void VolumeRenderWindow::endRawGLCalls(QPainter * painter)
{
    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    painter->endNativePainting();
}

void VolumeRenderWindow::drawOverlay(QPainter * painter)
{
    painter->setRenderHint(QPainter::Antialiasing);

    // Tick labels
    if (isScalebarActive)
    {
        painter->setBrush(*normal_brush);
        painter->setPen(*normal_pen);

        for (int i = 0; i < n_scalebar_ticks; i++)
        {
            painter->drawText(QPointF(scalebar_ticks[i*3+0], scalebar_ticks[i*3+1]), QString::number(scalebar_ticks[i*3+2]));
        }
    }
    // Fps
    QString fps_string("Fps: "+QString::number(getFps(), 'f', 0));
    QRect fps_string_rect = emph_fontmetric->boundingRect(fps_string);
    fps_string_rect.setWidth(std::max(fps_string_width_prev, fps_string_rect.width()));
    fps_string_width_prev = fps_string_rect.width();
    fps_string_rect += QMargins(5,5,5,5);
    fps_string_rect.moveTopRight(QPoint(width()-5,5));

    painter->setBrush(*fill_brush);
    painter->drawRoundedRect(fps_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(fps_string_rect, Qt::AlignCenter, fps_string);

    // Texture resolution
    QString resolution_string("Resolution: "+QString::number(ray_tex_resolution, 'f', 1)+"%, Volume Rendering Fps: "+QString::number(fps_required));
    QRect resolution_string_rect = emph_fontmetric->boundingRect(resolution_string);
    resolution_string_rect += QMargins(5,5,5,5);
    resolution_string_rect.moveBottomLeft(QPoint(5, height() - 5));

    painter->drawRoundedRect(resolution_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(resolution_string_rect, Qt::AlignCenter, resolution_string);

    // Scalebar multiplier
    QString multiplier_string("x"+QString::number(scalebar_multiplier)+" Å");
    QRect multiplier_string_rect = emph_fontmetric->boundingRect(multiplier_string);
    multiplier_string_rect += QMargins(5,5,5,5);
    multiplier_string_rect.moveTopRight(QPoint(width()-5, fps_string_rect.bottom() + 5));

    painter->drawRoundedRect(multiplier_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(multiplier_string_rect, Qt::AlignCenter, multiplier_string);

    // Transfer function
    QRect tsf_rect(0, 0, 20, height() - (multiplier_string_rect.bottom() + 5) - 50);
    tsf_rect += QMargins(5,5,5,5);
    tsf_rect.moveTopRight(QPoint(width()-5, multiplier_string_rect.bottom() + 5));

    painter->setBrush(*fill_brush);
    painter->drawRoundedRect(tsf_rect, 5, 5, Qt::AbsoluteSize);

    tsf_rect -= QMargins(5,5,5,5);
    Matrix<GLfloat> gl_tsf_rect(4,2);
    glRect(&gl_tsf_rect, &tsf_rect);

    beginRawGLCalls(painter);

    shared_window->std_2d_tex_program->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tsf_tex_gl_thumb);
    shared_window->std_2d_tex_program->setUniformValue(shared_window->std_2d_texture, 0);

    GLfloat texpos[] = {
        0.0, 1.0,
        0.0, 0.0,
        1.0, 0.0,
        1.0, 1.0
    };

    GLuint indices[] = {0,1,3,1,2,3};

    glVertexAttribPointer(shared_window->std_2d_fragpos, 2, GL_FLOAT, GL_FALSE, 0, gl_tsf_rect.data());
    glVertexAttribPointer(shared_window->std_2d_texpos, 2, GL_FLOAT, GL_FALSE, 0, texpos);

    glEnableVertexAttribArray(shared_window->std_2d_fragpos);
    glEnableVertexAttribArray(shared_window->std_2d_texpos);

    glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);

    glDisableVertexAttribArray(shared_window->std_2d_texpos);
    glDisableVertexAttribArray(shared_window->std_2d_fragpos);
    glBindTexture(GL_TEXTURE_2D, 0);

    shared_window->std_2d_tex_program->release();

    endRawGLCalls(painter);

    painter->setBrush(*normal_brush);
    painter->drawRect(tsf_rect);
}

void VolumeRenderWindow::drawScalebars()
{
    scalebar_coord_count = setScaleBars();

    shared_window->std_3d_color_program->bind();
    glEnableVertexAttribArray(shared_window->std_3d_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, scalebar_vbo);
    glVertexAttribPointer(shared_window->std_3d_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUniformMatrix4fv(shared_window->std_3d_transform, 1, GL_FALSE, scalebar_view_matrix.getColMajor().toFloat().data());

    glUniform4fv(shared_window->std_3d_color, 1, clear_color_inverse.data());

    glDrawArrays(GL_LINES,  0, scalebar_coord_count);

    glDisableVertexAttribArray(shared_window->std_3d_fragpos);

    shared_window->std_3d_color_program->release();
}

void VolumeRenderWindow::drawRayTex()
{
    // Volume rendering
    if (isModelActive) raytrace(cl_model_raytrace, cl_model_workload);
    else if(isSvoInitialized) raytrace(cl_svo_raytrace, cl_svo_workload);

    // Draw texture given one of the above is true
    if (isModelActive || isSvoInitialized)
    {
        shared_window->std_2d_tex_program->bind();

        glBindTexture(GL_TEXTURE_2D, ray_tex_gl);
        shared_window->std_2d_tex_program->setUniformValue(shared_window->std_2d_texture, 0);

        GLfloat fragpos[] = {
            -1.0, -1.0,
            1.0, -1.0,
            1.0, 1.0,
            -1.0, 1.0
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
    }
}

void VolumeRenderWindow::raytrace(cl_kernel kernel, cl_kernel workload)
{
    // Estimate workload and and adjust rendering quality accordingly
    setRayTexture();

    err = clSetKernelArg(cl_model_workload, 0, sizeof(cl_int2), ray_tex_dim.data());
    err |= clSetKernelArg(cl_model_workload, 1, sizeof(cl_mem), &cl_glb_work);
    err |= clSetKernelArg(cl_model_workload, 2, ray_loc_ws[0]*ray_loc_ws[1]*sizeof(cl_int), NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clSetKernelArg(cl_svo_workload, 0, sizeof(cl_int2), ray_tex_dim.data());
    err |= clSetKernelArg(cl_svo_workload, 1, sizeof(cl_mem), &cl_glb_work);
    err |= clSetKernelArg(cl_svo_workload, 2, ray_loc_ws[0]*ray_loc_ws[1]*sizeof(cl_int), NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clEnqueueNDRangeKernel(*context_cl->getCommanQueue(), workload, 2, NULL, ray_glb_ws.data(), ray_loc_ws.data(), 0, NULL, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clFinish(*context_cl->getCommanQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    Matrix<float> glb_work((ray_glb_ws[1]/ray_loc_ws[1]), (ray_glb_ws[0]/ray_loc_ws[0]));

    err = clEnqueueReadBuffer ( *context_cl->getCommanQueue(),
        cl_glb_work,
        CL_TRUE, 0,
        glb_work.bytes(),
        glb_work.data(),
        0, NULL, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    work = (double) glb_work.sum();

    // Aquire shared CL/GL objects
    glFinish();
    err = clEnqueueAcquireGLObjects(*context_cl->getCommanQueue(), 1, &ray_tex_cl, 0, 0, 0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Launch rendering kernel
    Matrix<size_t> area_per_call(1,2);
    area_per_call[0] = 128;
    area_per_call[1] = 128;
    Matrix<size_t> call_offset(1,2);
    call_offset[0] = 0;
    call_offset[1] = 0;

    // Launch the kernel and take the time
    ray_kernel_timer.start();

    for (size_t glb_x = 0; glb_x < ray_glb_ws[0]; glb_x += area_per_call[0])
    {
        for (size_t glb_y = 0; glb_y < ray_glb_ws[1]; glb_y += area_per_call[1])
        {
            call_offset[0] = glb_x;
            call_offset[1] = glb_y;

            err = clEnqueueNDRangeKernel(*context_cl->getCommanQueue(), kernel, 2, call_offset.data(), area_per_call.data(), ray_loc_ws.data(), 0, NULL, NULL);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        }
    }

    err = clFinish(*context_cl->getCommanQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    work_time = (double) ray_kernel_timer.nsecsElapsed();

    // Calculate how much the quality must be reduced (if any) in order to achieve fps_requested
    double time_per_work = (work_time / work) * 1.0e-9;
    double max_work = (1.0 / fps_required) / time_per_work;
    quality_factor = max_work / work;
//    std::cout << quality_factor * 100.0 << std::endl;

    // Release shared CL/GL objects
    err = clEnqueueReleaseGLObjects(*context_cl->getCommanQueue(), 1, &ray_tex_cl, 0, 0, 0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clFinish(*context_cl->getCommanQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void VolumeRenderWindow::setSvo(SparseVoxelOcttree * svo)
{
    data_extent.setDeep(4, 2, svo->getExtent()->data());
    data_view_extent.setDeep(4, 2, svo->getExtent()->data());

    misc_ints[0] = (int) svo->getLevels();;
    misc_ints[1] = (int) svo->getBrickOuterDimension();
    tsf_parameters_svo[2] = svo->getMinMax()->at(0);
    tsf_parameters_svo[3] = svo->getMinMax()->at(1);

    setDataExtent();
    resetViewMatrix();
    setMiscArrays();
    setTsfParameters();
    isModelActive = false;

    size_t n_bricks = svo->pool.size()/(svo->getBrickOuterDimension()*svo->getBrickOuterDimension()*svo->getBrickOuterDimension());

    MiniArray<size_t> pool_dim(3);
    pool_dim[0] = (1 << svo->getBrickPoolPower())*svo->getBrickOuterDimension();
    pool_dim[1] = (1 << svo->getBrickPoolPower())*svo->getBrickOuterDimension();
    pool_dim[2] = ((n_bricks) / ((1 << svo->getBrickPoolPower())*(1 << svo->getBrickPoolPower())))*svo->getBrickOuterDimension();

    // Load the contents into a CL texture
    if (isSvoInitialized){
        err = clReleaseMemObject(cl_svo_brick);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
    if (isSvoInitialized){
        err = clReleaseMemObject(cl_svo_index);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
    if (isSvoInitialized){
        err = clReleaseMemObject(cl_svo_pool);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }

    cl_svo_index = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        svo->index.size()*sizeof(cl_uint),
        svo->index.data(),
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_svo_brick = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        svo->brick.size()*sizeof(cl_uint),
        svo->brick.data(),
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_image_format cl_pool_format;
    cl_pool_format.image_channel_order = CL_INTENSITY;
    cl_pool_format.image_channel_data_type = CL_FLOAT;

    cl_svo_pool = clCreateImage3D ( *context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        &cl_pool_format,
        pool_dim[0],
        pool_dim[1],
        pool_dim[2],
        0,
        0,
        svo->pool.data(),
        &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_svo_pool_sampler = clCreateSampler(*context_cl->getContext(), CL_TRUE, CL_ADDRESS_CLAMP, CL_FILTER_LINEAR, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Send stuff to the kernel
    err = clSetKernelArg(cl_svo_raytrace, 2, sizeof(cl_mem), &cl_svo_pool);
    err |= clSetKernelArg(cl_svo_raytrace, 3, sizeof(cl_mem), &cl_svo_index);
    err |= clSetKernelArg(cl_svo_raytrace, 4, sizeof(cl_mem), &cl_svo_brick);
    err |= clSetKernelArg(cl_svo_raytrace, 5, sizeof(cl_sampler), &cl_svo_pool_sampler);
    err |= clSetKernelArg(cl_svo_workload, 3, sizeof(cl_mem), &cl_svo_index);
    err |= clSetKernelArg(cl_svo_workload, 4, sizeof(cl_mem), &cl_svo_brick);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    isSvoInitialized = true;
}

void VolumeRenderWindow::resetViewMatrix()
{
    data_scaling.setIdentity(4);
    rotation.setIdentity(4);
    scalebar_rotation.setIdentity (4);
    data_translation.setIdentity(4);
}

size_t VolumeRenderWindow::setScaleBars()
{
    // Draw the scalebars. The coordinates of the ticks are independent of the position in the volume, so it is a relative scalebar.
    double length = data_view_extent[1] - data_view_extent[0];

    double tick_interdistance_min = 0.005*length; // % of length

    int tick_levels = 0;
    int tick_levels_max = 2;

    size_t coord_counter = 0;

    Matrix<GLfloat> scalebar_coords(20000,3);

    n_scalebar_ticks = 0;

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
                    if (tick_levels == tick_levels_max - 1)
                    {
                        if (n_scalebar_ticks+3 < scalebar_ticks.getM())
                        {
                            getPosition2D(scalebar_ticks.data() + 3 * n_scalebar_ticks, scalebar_coords.data() + (coord_counter+0)*3, &scalebar_view_matrix);
                            scalebar_ticks[3 * n_scalebar_ticks + 0] = (scalebar_ticks[3 * n_scalebar_ticks + 0] + 1.0) * 0.5 *width();
                            scalebar_ticks[3 * n_scalebar_ticks + 1] = (1.0 - (scalebar_ticks[3 * n_scalebar_ticks + 1] + 1.0) * 0.5) *height();
                            scalebar_ticks[3 * n_scalebar_ticks + 2] = j * 0.1;
                            n_scalebar_ticks++;

                            getPosition2D(scalebar_ticks.data() + 3 * n_scalebar_ticks, scalebar_coords.data() + (coord_counter+4)*3, &scalebar_view_matrix);
                            scalebar_ticks[3 * n_scalebar_ticks + 0] = (scalebar_ticks[3 * n_scalebar_ticks + 0] + 1.0) * 0.5 *width();
                            scalebar_ticks[3 * n_scalebar_ticks + 1] = (1.0 - (scalebar_ticks[3 * n_scalebar_ticks + 1] + 1.0) * 0.5) *height();
                            scalebar_ticks[3 * n_scalebar_ticks + 2] = j * 0.1;
                            n_scalebar_ticks++;

                            getPosition2D(scalebar_ticks.data() + 3 * n_scalebar_ticks, scalebar_coords.data() + (coord_counter+8)*3, &scalebar_view_matrix);
                            scalebar_ticks[3 * n_scalebar_ticks + 0] = (scalebar_ticks[3 * n_scalebar_ticks + 0] + 1.0) * 0.5 *width();
                            scalebar_ticks[3 * n_scalebar_ticks + 1] = (1.0 - (scalebar_ticks[3 * n_scalebar_ticks + 1] + 1.0) * 0.5) *height();
                            scalebar_ticks[3 * n_scalebar_ticks + 2] = j * 0.1;
                            n_scalebar_ticks++;
                        }
                        scalebar_multiplier = tick_interdistance * 10.0;
                    }
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

    setVbo(scalebar_vbo, scalebar_coords.data(), coord_counter*3, GL_STATIC_DRAW);

    return coord_counter;
}

void VolumeRenderWindow::setQuality(int value)
{
   fps_required = (float) value;
}

void VolumeRenderWindow::setProjection()
{
    isOrthonormal = !isOrthonormal;
    ctc_matrix.setProjection(isOrthonormal);

    float f;
    if (isOrthonormal) f = 1;
    else f = 1.0/2.3;

    projection_scaling[0] = f;
    projection_scaling[5] = f;
    projection_scaling[10] = f;
}

void VolumeRenderWindow::setBackground()
{
    Matrix<GLfloat> tmp;
    tmp = clear_color;

    // Swap color
    clear_color = clear_color_inverse;
    clear_color_inverse = tmp;

    normal_pen->setColor(QColor(255.0*clear_color_inverse[0],
                         255.0*clear_color_inverse[1],
                         255.0*clear_color_inverse[2],
                         255));
    fill_brush->setColor(QColor(255.0*clear_color[0],
                         255.0*clear_color[1],
                         255.0*clear_color[2],
                         255.0*0.7));
    normal_pen->setWidth(1);
}
void VolumeRenderWindow::setLogarithmic()
{
    isLogarithmic = !isLogarithmic;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWindow::setDataStructure()
{
    isDSActive = !isDSActive;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWindow::setTsfColor(int value)
{
    tsf_color_scheme = value;
    if (isInitialized) setTsfTexture();
}
void VolumeRenderWindow::setTsfAlpha(int value)
{
    tsf_alpha_scheme = value;
    if (isInitialized) setTsfTexture();
}
void VolumeRenderWindow::setDataMin(double value)
{
    if (isModelActive)
    {
        tsf_parameters_model[2] = value;
    }
    else
    {
        tsf_parameters_svo[2] = value;
    }
    if (isInitialized) setTsfParameters();
}
void VolumeRenderWindow::setDataMax(double value)
{
    if (isModelActive)
    {
        tsf_parameters_model[3] = value;
    }
    else
    {
        tsf_parameters_svo[3] = value;
    }
    if (isInitialized) setTsfParameters();
}
void VolumeRenderWindow::setAlpha(double value)
{
    if (isModelActive)
    {
        tsf_parameters_model[4] = value;
    }
    else
    {
        tsf_parameters_svo[4] = value;
    }
    if (isInitialized) setTsfParameters();
}
void VolumeRenderWindow::setBrightness(double value)
{
    if (isModelActive)
    {
        tsf_parameters_model[5] = value;
    }
    else
    {
        tsf_parameters_svo[5] = value;
    }
    if (isInitialized) setTsfParameters();
}
void VolumeRenderWindow::setUnitcell()
{
    isUnitcellActive = !isUnitcellActive;
}
void VolumeRenderWindow::setModel()
{
    isModelActive = !isModelActive;
}
void VolumeRenderWindow::setModelParam0(double value)
{
    model_misc_floats[0] = value;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWindow::setModelParam1(double value)
{
    model_misc_floats[1] = value;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWindow::setModelParam2(double value)
{
    model_misc_floats[2] = value;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWindow::setModelParam3(double value)
{
    model_misc_floats[3] = value;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWindow::setModelParam4(double value)
{
    model_misc_floats[4] = value;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWindow::setModelParam5(double value)
{
    model_misc_floats[5] = value;
    if (isInitialized) setMiscArrays();
}

void VolumeRenderWindow::setScalebar()
{
    isScalebarActive = !isScalebarActive;
}
