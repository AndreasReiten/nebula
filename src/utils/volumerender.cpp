#include "volumerender.h"

VolumeRenderWindow::VolumeRenderWindow()
    : gl_worker(0),
      isInitialized(false)
{

}

VolumeRenderWindow::~VolumeRenderWindow()
{

}

VolumeRenderWorker *  VolumeRenderWindow::getWorker()
{
    return gl_worker;
}

void VolumeRenderWindow::renderNow()
{
    if (!isExposed())
    {
        emit stopRendering();
        return;
    }
    if (isWorkerBusy)
    {
        if (isAnimating) renderLater();
        return;
    }
    else
    {
        if (!isInitialized) initializeWorker();

        if (gl_worker)
        {
            if (isMultiThreaded)
            {
                isWorkerBusy = true;
                worker_thread->start();
                emit render();
            }
            else
            {
                context_gl->makeCurrent(this);
                gl_worker->process();
                emit render();
            }

        }
    }
    if (isAnimating) renderLater();
}

void VolumeRenderWindow::initializeWorker()
{
    initializeGLContext();

    gl_worker = new VolumeRenderWorker;
    gl_worker->setRenderSurface(this);
    gl_worker->setGLContext(context_gl);
    gl_worker->setOpenCLContext(context_cl);
    gl_worker->setSharedWindow(shared_window);
    gl_worker->setMultiThreading(isMultiThreaded);

    if (isMultiThreaded)
    {
        // Set up worker thread
        gl_worker->moveToThread(worker_thread);
        connect(this, SIGNAL(render()), gl_worker, SLOT(process()));
        connect(this, SIGNAL(stopRendering()), worker_thread, SLOT(quit()));
        connect(gl_worker, SIGNAL(finished()), this, SLOT(setSwapState()));

        // Transfering mouse events
//        connect(this, SIGNAL(metaMouseMoveEventCaughtCompact(QMouseEvent)), gl_worker, SLOT(metaMouseMoveEventCompact(QMouseEvent)));
        connect(this, SIGNAL(metaMouseMoveEventCaught(int, int, int, int, int, int, int)), gl_worker, SLOT(metaMouseMoveEvent(int, int, int, int, int, int, int)));
        connect(this, SIGNAL(metaMousePressEventCaught(int, int, int, int, int, int, int)), gl_worker, SLOT(metaMousePressEvent(int, int, int, int, int, int, int)));
        connect(this, SIGNAL(metaMouseReleaseEventCaught(int, int, int, int, int, int, int)), gl_worker, SLOT(metaMouseReleaseEvent(int, int, int, int, int, int, int)));
//        connect(this, SIGNAL(mouseMoveEventCaught(QMouseEvent*)), gl_worker, SLOT(mouseMoveEvent(QMouseEvent*)));//,, Qt::DirectConnection);
        connect(this, SIGNAL(resizeEventCaught(QResizeEvent*)), gl_worker, SLOT(resizeEvent(QResizeEvent*)));//, Qt::DirectConnection);
        connect(this, SIGNAL(wheelEventCaught(QWheelEvent*)), gl_worker, SLOT(wheelEvent(QWheelEvent*)), Qt::DirectConnection);
    }
    else
    {
        connect(this, SIGNAL(mouseMoveEventCaught(QMouseEvent*)), gl_worker, SLOT(mouseMoveEvent(QMouseEvent*)), Qt::DirectConnection);
        connect(this, SIGNAL(resizeEventCaught(QResizeEvent*)), gl_worker, SLOT(resizeEvent(QResizeEvent*)));//, Qt::DirectConnection);
        connect(this, SIGNAL(wheelEventCaught(QWheelEvent*)), gl_worker, SLOT(wheelEvent(QWheelEvent*)), Qt::DirectConnection);

        connect(this, SIGNAL(render()), gl_worker, SLOT(process()));
    }

    isInitialized = true;
}

void VolumeRenderWindow::setSharedWindow(SharedContextWindow * window)
{
    this->shared_window = window;
    shared_context = window->getGLContext();
}

VolumeRenderWorker::VolumeRenderWorker(QObject *parent)
    : OpenGLWorker(parent),
      isInitialized(false),
      isRayTexInitialized(false),
      isTsfTexInitialized(false),
      isIntegrationTexInitialized(false),
      isDSActive(false),
      isOrthonormal(true),
      isLogarithmic(true),
      isModelActive(true),
      isUnitcellActive(false),
      isSvoInitialized(false),
      isScalebarActive(true),
      isSlicingActive(false),
      isIntegration2DActive(false),
      isIntegration3DActive(false),
      isRendering(true),
      isShadowActive(false),
      isLogarithmic2D(false),
      isOrthoGridActive(false),
      isBackgroundBlack(false),
      isDataExtentReadOnly(true),
      isCenterlineActive(true),
      isRulerActive(false),
      isLMBDown(false),
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
    double fov = 20.0;

    bbox_translation[11] = -N -(F - N)*0.5;
    ctc_matrix.setN(N);
    ctc_matrix.setF(F);
    ctc_matrix.setFov(fov);
    ctc_matrix.setProjection(isOrthonormal);
    
    // Timer
    session_age.start();
    
    // Ray tex
    ray_tex_dim.reserve(1, 2);
    ray_glb_ws.reserve(1,2);
    ray_loc_ws.reserve(1,2);
    ray_loc_ws[0] = 16;
    ray_loc_ws[1] = 16;
    pixel_size.reserve(2,1);

    // Center line
    centerline_coords.set(2,3, 0.0);

    // Transfer texture
    tsf_color_scheme = 0;
    tsf_alpha_scheme = 0;

    tsf_parameters_model[0] = 0.0; // texture min
    tsf_parameters_model[1] = 1.0; // texture max

    tsf_parameters_svo[0] = 0.0; // texture min
    tsf_parameters_svo[1] = 1.0; // texture max
    tsf_parameters_svo[4] = 0.5; // alpha
    tsf_parameters_svo[5] = 2.0; // brightness

    // Ray texture timing
    fps_requested = 60;
    work = 1.0;
    work_time = 0.0;
    quality_factor = 0.5;

    // Scalebars
    position_scalebar_ticks.reserve(100,3);
    count_scalebar_ticks.reserve(100,3);
    count_minor_scalebar_ticks.reserve(100,3);
    n_count_scalebar_ticks = 0;
    n_count_minor_scalebar_ticks = 0;
    n_position_scalebar_ticks = 0;
            

    // Color
    GLfloat white_buf[] = {1,1,1,0.4};
    GLfloat black_buf[] = {0,0,0,0.4};
    GLfloat yellow_buf[] = {1,0.2,0,0.8};
    white.setDeep(1,4,white_buf);
    black.setDeep(1,4,black_buf);
    yellow.setDeep(1,4,yellow_buf);
    clear_color = white;
    clear_color_inverse = black;
    centerline_color = yellow;

    // Fps
    fps_string_width_prev = 0;

    // Shadow
    shadow_vector.reserve(4,1);
    shadow_vector[0] = -1.0;
    shadow_vector[1] = +1.0;
    shadow_vector[2] = -1.0;
    shadow_vector[3] = 0.0;
    
    // Ruler
    ruler.reserve(1,4);
}

VolumeRenderWorker::~VolumeRenderWorker()
{
    if (isInitialized) glDeleteBuffers(1, &scalebar_vbo);
    if (isInitialized) glDeleteBuffers(1, &count_scalebar_vbo);
    if (isInitialized) glDeleteBuffers(1, &centerline_vbo);
    if (isInitialized) glDeleteBuffers(1, &point_vbo);
}


void VolumeRenderWorker::metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
//    std::stringstream ss;
//    ss << "___EVENT CAUGHT___\n x:" << x << "\n y:" << y << "\n L:" << left_button << "\n M:" << mid_button << "\n R:" << right_button;
//    std::cout << ss.str().c_str() << std::endl;
    if (isRulerActive && left_button)
    {
        ruler[0] = x;
        ruler[1] = y;
        ruler[2] = x;
        ruler[3] = y;
    }
    
}

void VolumeRenderWorker::metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
//    std::stringstream ss;
//    ss << "___EVENT CAUGHT___\n x:" << x << "\n y:" << y << "\n L:" << left_button << "\n M:" << mid_button << "\n R:" << right_button;
//    std::cout << ss.str().c_str() << std::endl;
}

void VolumeRenderWorker::metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
//        std::stringstream ss;
//        ss << "___EVENT CAUGHT___\n x:" << x << "\n y:" << y << "\n L:" << left_button << "\n M:" << mid_button << "\n R:" << right_button;
//        std::cout << ss.str().c_str() << std::endl;
        
        if (left_button) isLMBDown = true;
        else isLMBDown = false;
    
        if (isLMBDown && isRulerActive)
        {
            ruler[2] = x;
            ruler[3] = y;
//            ruler.print();
        }
        
        if (!isRendering && (std::abs(last_mouse_pos_x - x) < 50) && (std::abs(last_mouse_pos_y - y) < 50));
        {
            float move_scaling = 1.0;
            if(ctrl_button) move_scaling = 0.2;

            if (left_button && !right_button && !isRulerActive)
            {
                /* Rotation happens multiplicatively around a rolling axis given
                 * by the mouse move direction and magnitude.
                 * Moving the mouse alters rotation.
                 * */

                double eta = std::atan2((double)x - last_mouse_pos_x, (double)y - last_mouse_pos_y) - pi*1.0;
                double roll = move_scaling * pi/((float) render_surface->height()) * std::sqrt((double)(x - last_mouse_pos_x)*(x - last_mouse_pos_x) + (y - last_mouse_pos_y)*(y - last_mouse_pos_y));

                RotationMatrix<double> roll_rotation;
                roll_rotation.setArbRotation(-0.5*pi, eta, roll);

                if(shift_button) scalebar_rotation = roll_rotation * scalebar_rotation;
                else
                {
                    rotation = roll_rotation * rotation;
                    scalebar_rotation = roll_rotation * scalebar_rotation;
                }

            }
            else if (left_button && right_button && !mid_button && !isRulerActive)// && (ev->buttons() & Qt::RightButton))
            {
                /* Rotation happens multiplicatively around a rolling axis given
                 * by the mouse move direction and magnitude.
                 * Moving the mouse alters rotation.
                 * */

                RotationMatrix<double> roll_rotation;
                double roll = move_scaling * pi/((float) render_surface->height()) * (y - last_mouse_pos_y);

                roll_rotation.setArbRotation(0, 0, roll);

                if(shift_button) scalebar_rotation = roll_rotation * scalebar_rotation;
                else
                {
                    rotation = roll_rotation * rotation;
                    scalebar_rotation = roll_rotation * scalebar_rotation;
                }

            }
            else if (mid_button && !left_button && !right_button)
            {
                /* X/Y translation happens multiplicatively. Here it is
                 * important to retain the bounding box accordingly  */
                float dx = move_scaling * 2.0*(data_view_extent[1]-data_view_extent[0])/((float) render_surface->height()) * (x - last_mouse_pos_x);
                float dy = move_scaling * -2.0*(data_view_extent[3]-data_view_extent[2])/((float) render_surface->height()) * (y - last_mouse_pos_y);

                Matrix<double> data_translation_prev;
                data_translation_prev.setIdentity(4);
                data_translation_prev = data_translation;

                data_translation.setIdentity(4);
                data_translation[3] = dx;
                data_translation[7] = dy;

                data_translation = ( rotation.getInverse() * data_translation * rotation) * data_translation_prev;

                this->data_view_extent =  (data_scaling * data_translation).getInverse() * data_extent;
            }
            else if (!left_button && right_button && !mid_button)
            {
                /* Z translation happens multiplicatively */
                float dz = move_scaling * 2.0*(data_view_extent[5]-data_view_extent[4])/((float) render_surface->height()) * (y - last_mouse_pos_y);

                Matrix<double> data_translation_prev;
                data_translation_prev.setIdentity(4);
                data_translation_prev = data_translation;

                data_translation.setIdentity(4);
                data_translation[11] = dz;

                data_translation = ( rotation.getInverse() * data_translation * rotation) * data_translation_prev;

                this->data_view_extent =  (data_scaling * data_translation).getInverse() * data_extent;
            }

        }

        last_mouse_pos_x = x;
        last_mouse_pos_y = y;
}


void VolumeRenderWorker::setCenterLine()
{
    centerline_coords[3] = data_view_extent[0] + (data_view_extent[1] - data_view_extent[0])*0.5;
    centerline_coords[4] = data_view_extent[2] + (data_view_extent[3] - data_view_extent[2])*0.5;
    centerline_coords[5] = data_view_extent[4] + (data_view_extent[5] - data_view_extent[4])*0.5;

    // Center line
    setVbo(centerline_vbo, centerline_coords.data(), 6, GL_STATIC_DRAW);
}


void VolumeRenderWorker::drawCenterLine()
{
    setCenterLine();

    shared_window->std_3d_color_program->bind();
    glEnableVertexAttribArray(shared_window->std_3d_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, centerline_vbo);
    glVertexAttribPointer(shared_window->std_3d_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUniformMatrix4fv(shared_window->std_3d_transform, 1, GL_FALSE, view_matrix.getColMajor().toFloat().data());


    glUniform4fv(shared_window->std_3d_color, 1, clear_color_inverse.data());

    glDrawArrays(GL_LINES,  0, 2);

    glDisableVertexAttribArray(shared_window->std_3d_fragpos);

    shared_window->std_3d_color_program->release();
}

void VolumeRenderWorker::drawSenseOfRotation(double zeta, double eta, double rpm)
{
    Matrix<float> point_coords(1,3*100, 0.0);
    point_coords[0*3+2] = -5;
    point_coords[1*3+2] = 5;
   
    for (int i = 2; i < 100; i++)
    {
        point_coords[i*3+2] = -5.0 + (10.0/97.0) * (i - 2);
        point_coords[i*3+0] = 0.5*sin(2.0*point_coords[i*3+2]);
        point_coords[i*3+1] = 0.5*cos(2.0*point_coords[i*3+2]);
    }
    
    setVbo(point_vbo, point_coords.data(), 3*100, GL_DYNAMIC_DRAW);
    
    
    shared_window->std_3d_color_program->bind();
    glEnableVertexAttribArray(shared_window->std_3d_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, point_vbo);
    glVertexAttribPointer(shared_window->std_3d_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0); 
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    
    double gamma = fmod(session_age.elapsed()*(rpm/60000)*2*pi, 2*pi);

    // Vertices on the axis
    RotationMatrix<double> point_on_axis, RyPlus, RxPlus;
    RyPlus.setYRotation(zeta);
    RxPlus.setXRotation(eta);
    point_on_axis = ctc_matrix * bbox_translation * normalization_scaling * rotation * RyPlus * RxPlus;
    
    // Vertices rotating around the axis    
    RotationMatrix<double> point_around_axis, axis_rotation;
    axis_rotation.setArbRotation(zeta, eta, gamma);
    point_around_axis = ctc_matrix * bbox_translation * normalization_scaling * rotation * axis_rotation * RyPlus * RxPlus;
    
    
    glUniform4fv(shared_window->std_3d_color, 1, clear_color_inverse.data());
    
    glPointSize(5);
    
    
    glUniformMatrix4fv(shared_window->std_3d_transform, 1, GL_FALSE, point_around_axis.getColMajor().toFloat().data());
    glDrawArrays(GL_POINTS,  2, 98);
    
    
    glUniformMatrix4fv(shared_window->std_3d_transform, 1, GL_FALSE, point_on_axis.getColMajor().toFloat().data());
    glDrawArrays(GL_LINE_STRIP,  0, 2);
    
    glDisableVertexAttribArray(shared_window->std_3d_fragpos);

    shared_window->std_3d_color_program->release();
}

void VolumeRenderWorker::wheelEvent(QWheelEvent* ev)
{
    if (!isDataExtentReadOnly)
    {
        float move_scaling = 1.0;
        if(ev->modifiers() & Qt::ShiftModifier) move_scaling = 5.0;
        else if(ev->modifiers() & Qt::ControlModifier) move_scaling = 0.2;

        double delta = 0;
        if ((ev->delta() > -1000) && (ev->delta() < 1000))
        {
            delta = move_scaling*((double)ev->delta())*0.0008;
        }
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
}



void VolumeRenderWorker::initialize()
{
    initResourcesCL();
    initResourcesGL();

    isInitialized = true;

    // Textures
    setRayTexture();
    setTsfTexture();

    // Core set functions
    setDataExtent();
    setViewMatrices();
    setTsfParameters();
    setMiscArrays();

    // Pens
    initializePaintTools();
}

void VolumeRenderWorker::initializePaintTools()
{
    normal_pen = new QPen;
    normal_pen->setWidthF(1.0);
    border_pen = new QPen;
    border_pen->setWidth(1);

    whatever_pen = new QPen;

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

    histogram_brush = new QBrush;
    histogram_brush->setStyle(Qt::SolidPattern);
    histogram_brush->setColor(QColor(40,225,40,225));
}

void VolumeRenderWorker::initResourcesGL()
{
    glGenBuffers(1, &scalebar_vbo);
    glGenBuffers(1, &count_scalebar_vbo);
    glGenBuffers(1, &centerline_vbo);
    glGenBuffers(1, &point_vbo);
}

void VolumeRenderWorker::initResourcesCL()
{
    // Build program from OpenCL kernel source
    Matrix<const char *> paths(1,4);
    paths[0] = "kernels/render_shared.cl";
    paths[1] = "kernels/render_svo.cl";
    paths[2] = "kernels/render_model.cl";
    paths[3] = "kernels/integrate.cl";

    program = context_cl->createProgram(&paths, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    context_cl->buildProgram(&program, "-Werror");


    // Kernel handles
    cl_svo_raytrace = clCreateKernel(program, "svoRayTrace", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_model_raytrace = clCreateKernel(program, "modelRayTrace", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_integrate_image = clCreateKernel(program, "integrateImage", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    // Buffers
    cl_view_matrix_inverse = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        view_matrix.toFloat().bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_scalebar_rotation = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        scalebar_rotation.toFloat().bytes(),
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

void VolumeRenderWorker::setViewMatrices()
{
//    qDebug() << "Setting view matrix";

    normalization_scaling[0] = bbox_scaling[0] * projection_scaling[0] * 2.0 / (data_extent[1] - data_extent[0]);
    normalization_scaling[5] = bbox_scaling[5] * projection_scaling[5] * 2.0 / (data_extent[3] - data_extent[2]);
    normalization_scaling[10] = bbox_scaling[10] * projection_scaling[10] * 2.0 / (data_extent[5] - data_extent[4]);

    view_matrix = ctc_matrix * bbox_translation * normalization_scaling * data_scaling * rotation * data_translation;
    scalebar_view_matrix = ctc_matrix * bbox_translation * normalization_scaling * data_scaling * scalebar_rotation * data_translation;

    err = clEnqueueWriteBuffer (*context_cl->getCommandQueue(),
        cl_view_matrix_inverse,
        CL_TRUE,
        0,
        view_matrix.bytes()/2,
        view_matrix.getInverse().toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clEnqueueWriteBuffer (*context_cl->getCommandQueue(),
        cl_scalebar_rotation,
        CL_TRUE,
        0,
        scalebar_rotation.bytes()/2,
        (rotation.getInverse() * scalebar_rotation).toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clSetKernelArg(cl_model_raytrace, 3, sizeof(cl_mem), (void *) &cl_view_matrix_inverse);
    err |= clSetKernelArg(cl_model_raytrace, 9, sizeof(cl_mem), (void *) &cl_scalebar_rotation);

    err |= clSetKernelArg(cl_svo_raytrace, 7, sizeof(cl_mem), (void *) &cl_view_matrix_inverse);
    err |= clSetKernelArg(cl_svo_raytrace, 12, sizeof(cl_mem), (void *) &cl_scalebar_rotation);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
//    qDebug() << "Done setting view matrix";
}

void VolumeRenderWorker::setDataExtent()
{
    err = clEnqueueWriteBuffer (*context_cl->getCommandQueue(),
        cl_data_extent,
        CL_TRUE,
        0,
        data_extent.bytes()/2,
        data_extent.toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clEnqueueWriteBuffer (*context_cl->getCommandQueue(),
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

void VolumeRenderWorker::setTsfParameters()
{
    err = clEnqueueWriteBuffer (*context_cl->getCommandQueue(),
        cl_tsf_parameters_model,
        CL_TRUE,
        0,
        tsf_parameters_model.bytes()/2,
        tsf_parameters_model.toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clEnqueueWriteBuffer (*context_cl->getCommandQueue(),
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

void VolumeRenderWorker::setMiscArrays()
{
    misc_ints[2] = isLogarithmic;
    misc_ints[3] = isDSActive;
    misc_ints[4] = isSlicingActive;
    misc_ints[5] = isIntegration2DActive;
    misc_ints[6] = isShadowActive;
    misc_ints[7] = isIntegration3DActive;

    err = clEnqueueWriteBuffer (*context_cl->getCommandQueue(),
        cl_misc_ints,
        CL_TRUE,
        0,
        misc_ints.bytes(),
        misc_ints.data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clEnqueueWriteBuffer (*context_cl->getCommandQueue(),
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
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void VolumeRenderWorker::resizeEvent(QResizeEvent * ev)
{
    Q_UNUSED(ev);

    if (paint_device_gl) paint_device_gl->setSize(render_surface->size());
    ctc_matrix.setWindow(render_surface->width(), render_surface->height());
}

void VolumeRenderWorker::setRayTexture()
{
    // Only resize the texture if the change is somewhat significant (in area cahnged)
    if (isInitialized && ((!isRayTexInitialized) || (std::abs(1.0 - quality_factor) > 0.15)))
    {
        // Scale down the change in quality factor a bit so that a stable resolution will be easier to reach. If not we risk having the resolution jump between two states that both fullfill the if-criterion enclosing this code
        quality_factor = 1.0 + (quality_factor - 1.0)*0.25;

        // Set a texture for the volume rendering kernel
        Matrix<int> ray_tex_new(1, 2);
        ray_tex_new[0] = (int)((float)render_surface->width()*(ray_tex_resolution*0.01)*std::sqrt(quality_factor));
        ray_tex_new[1] = (int)((float)render_surface->height()*(ray_tex_resolution*0.01)*std::sqrt(quality_factor));

        // Clamp
        if (ray_tex_new[0] < 32) ray_tex_new[0] = 32;
        if (ray_tex_new[1] < 32) ray_tex_new[1] = 32;

        if (ray_tex_new[0] > render_surface->width()) ray_tex_new[0] = render_surface->width();
        if (ray_tex_new[1] > render_surface->height()) ray_tex_new[1] = render_surface->height();

        // Calculate the actual quality factor multiplier
        quality_factor = std::pow((double) ray_tex_new[0] / ((double)render_surface->width()*(ray_tex_resolution*0.01)), 2.0);

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

        // Integration texture
        if (isIntegrationTexInitialized){
            err = clReleaseMemObject(integration_tex_alpha_cl);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
            err = clReleaseMemObject(integration_tex_beta_cl);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        }

        cl_image_format integration_format;
        integration_format.image_channel_order = CL_INTENSITY;
        integration_format.image_channel_data_type = CL_FLOAT;

        integration_tex_alpha_cl = clCreateImage2D ( *context_cl->getContext(),
            CL_MEM_READ_WRITE  | CL_MEM_ALLOC_HOST_PTR,
            &integration_format,
            ray_tex_dim[0],
            ray_tex_dim[1],
            0,
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        integration_tex_beta_cl = clCreateImage2D ( *context_cl->getContext(),
            CL_MEM_READ_WRITE  | CL_MEM_ALLOC_HOST_PTR,
            &integration_format,
            ray_tex_dim[0],
            ray_tex_dim[1],
            0,
            NULL,
            &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        // Sampler for integration texture
        integration_sampler_cl = clCreateSampler(*context_cl->getContext(), false, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        isIntegrationTexInitialized = true;

        // Pass textures to CL kernels
        if (isInitialized) err = clSetKernelArg(cl_integrate_image, 0, sizeof(cl_mem), (void *) &integration_tex_alpha_cl);
        if (isInitialized) err |= clSetKernelArg(cl_integrate_image, 1, sizeof(cl_mem), (void *) &integration_tex_beta_cl);
        if (isInitialized) err |= clSetKernelArg(cl_integrate_image, 3, sizeof(cl_sampler), &integration_sampler_cl);

        if (isInitialized) err |= clSetKernelArg(cl_svo_raytrace, 0, sizeof(cl_mem), (void *) &ray_tex_cl);
        if (isInitialized) err |= clSetKernelArg(cl_svo_raytrace, 13, sizeof(cl_mem), (void *) &integration_tex_alpha_cl);

        if (isInitialized) err |= clSetKernelArg(cl_model_raytrace, 0, sizeof(cl_mem), (void *) &ray_tex_cl);
        if (isInitialized) err |= clSetKernelArg(cl_model_raytrace, 10, sizeof(cl_mem), (void *) &integration_tex_alpha_cl);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }
}

void VolumeRenderWorker::setTsfTexture()
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


void VolumeRenderWorker::setSharedWindow(SharedContextWindow * window)
{
    this->shared_window = window;
}

void VolumeRenderWorker::render(QPainter *painter)
{
//    qDebug() << "Render";
//    QCoreApplication::processEvents();
    isRendering = true;
//    emit renderState(1);
//    this->blockSignals(true);
    isDataExtentReadOnly = true;
    setDataExtent();
    setViewMatrices();

    glClearColor(clear_color[0], clear_color[1], clear_color[2], 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    beginRawGLCalls(painter);
    glLineWidth(1.5);
    const qreal retinaScale = render_surface->devicePixelRatio();
    glViewport(0, 0, render_surface->width() * retinaScale, render_surface->height() * retinaScale);

    // Draw relative scalebar
    if (isScalebarActive) drawPositionScalebars();
    if (isCenterlineActive) drawCenterLine();

    isDataExtentReadOnly = false;

    // Draw raytracing texture
    drawRayTex();
    
    // Test (zeta, eta)
    drawSenseOfRotation(-0.5*pi, 0, 30.0);
    
    // PHI
//    drawSenseOfRotation(0.000891863, 0, 30.0);
    // KAPPA
//    drawSenseOfRotation(0.8735582, 0, 30.0);
    
    endRawGLCalls(painter);

    // Compute the projected pixel size in orthonormal configuration
    computePixelSize();

//    qDebug("Do not change");
    // Visualize 2D to 1D integration
    if (isIntegration2DActive) drawIntegral(painter);
//    qDebug("Ok, change");
    // Draw overlay
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(*normal_brush);

    if (isOrthoGridActive) drawGrid(painter);
    if (isRulerActive) drawRuler(painter);
    drawCountScalebar(painter);
    
    painter->setPen(*normal_pen);
    
    drawOverlay(painter);
//    this->blockSignals(false);
//    emit renderState(0);

    isRendering = false;
}


void VolumeRenderWorker::takeScreenShot(QString path, float quality)
{
    QOpenGLFramebufferObjectFormat format;

    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setMipmap(true);
    format.setSamples(64);
    format.setTextureTarget(GL_TEXTURE_2D);
    format.setInternalTextureFormat(GL_RGBA32F);

//    qDebug() << render_surface->size() << render_surface->format().samples();

    QOpenGLFramebufferObject buffy(render_surface->width(), render_surface->height(), format);

    buffy.bind();

    // Render into buffer using max quality
    QPainter painter(paint_device_gl);
    quality_factor = 1.0e9;

    render(&painter);

//    quality_factor = 1.0e-9;

    buffy.release();

    buffy.toImage().save(path);

    // Save buffer as image

//    qDebug() << render_surface->size() << render_surface->format().samples();

}


void VolumeRenderWorker::drawIntegral(QPainter *painter)
{
    // Sum the rows and columns of the integrated texture (which resides as a pure OpenCL image buffer)

    // __ROWS__

    // sum along w (rows, x): direction = 0
    // sum along h (columns, y): direction = 1
    int direction = 0;
    int block_size = 128;

    // Set kernel arguments
    err = clSetKernelArg(cl_integrate_image, 2, block_size* sizeof(cl_float), NULL);
    err |= clSetKernelArg(cl_integrate_image, 4, sizeof(cl_int), &direction);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Launch kernel
    Matrix<size_t> local_size(1,2);
    local_size[0] = block_size;
    local_size[1] = 1;

    Matrix<size_t> global_size(1,2);
    global_size[0] = (block_size - (ray_tex_dim[0] % block_size)) + ray_tex_dim[0];
    global_size[1] = ray_tex_dim[1];

    Matrix<size_t> work_size(1,2);
    work_size[0] = global_size[0];
    work_size[1] = 1;

    Matrix<size_t> global_offset(1,2);
    global_offset[0] = 0;
    global_offset[1] = 0;

    for (size_t row = 0; row < global_size[1]; row += local_size[1])
    {
        global_offset[1] = row;

        err = clEnqueueNDRangeKernel(*context_cl->getCommandQueue(), cl_integrate_image, 2, global_offset.data(), work_size.data(), local_size.data(), 0, NULL, NULL);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }

    err = clFinish(*context_cl->getCommandQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    // Read the output data
    Matrix<float> output(ray_tex_dim[1], global_size[0]/block_size);

    Matrix<size_t> origin(1,3);
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;

    Matrix<size_t> region(1,3);
    region[0] = global_size[0]/block_size;
    region[1] = ray_tex_dim[1];
    region[2] = 1;


    err = clEnqueueReadImage ( 	*context_cl->getCommandQueue(),
        integration_tex_beta_cl,
        CL_TRUE,
        origin.data(),
        region.data(),
        0,
        0,
        output.data(),
        0, NULL, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Sum up the partially reduced array
    Matrix<float> row_sum(ray_tex_dim[1], 1, 0.0f);
    float max = 0;
    float min = 1e9;
    double sum = 0;
    for (int i = 0; i < output.getM(); i++)
    {
        for(int j = 0; j < output.getN(); j++)
        {
            row_sum[i] += output[i*output.getN() + j];
        }

        if (row_sum[i] > max) max = row_sum[i];
        if ((row_sum[i] < min) && (row_sum[i] > 0)) min = row_sum[i];

        sum += row_sum[i];
    }

    if (sum > 0)
    {
        for (int i = 0; i < row_sum.getM(); i++)
        {
            row_sum[i] *= 100000.0/sum;
        }
        max *=  100000.0/sum;
        min *=  100000.0/sum;

        if (isLogarithmic2D)
        {
            for (int i = 0; i < row_sum.getM(); i++)
            {
                if (row_sum[i] <= 0) row_sum[i] = min;
                row_sum[i] = log10(row_sum[i]);
            }
            max =  log10(max);
            min =  log10(min);
        }

        QPolygonF row_polygon;
        row_polygon << QPointF(0,0);
        for (int i = 0; i < row_sum.getM(); i++)
        {
            QPointF point_top, point_bottom;

            float value = ((row_sum[row_sum.getM() - i - 1] - min) / (max - min))*render_surface->width()/10.0;

            point_top.setX(value);
            point_top.setY(((float) i / (float) row_sum.getM())*render_surface->height());

            point_bottom.setX(value);
            point_bottom.setY((((float) i + 1) / (float) row_sum.getM())*render_surface->height());
            row_polygon << point_top << point_bottom;
        }
        row_polygon << QPointF(0,render_surface->height());



        painter->setRenderHint(QPainter::Antialiasing);

        QLinearGradient lgrad(QPointF(0,0), QPointF(render_surface->width()/10.0,0));
                    lgrad.setColorAt(0.0, Qt::transparent);
                    lgrad.setColorAt(1.0, Qt::blue);

        QBrush histogram_brush_lg(lgrad);

        painter->setBrush(histogram_brush_lg);
        painter->setPen(*normal_pen);

        painter->drawPolygon(row_polygon);
    }


    // __COLUMNS__
    direction = 1;
    block_size = 128;

    // Set kernel arguments
    err = clSetKernelArg(cl_integrate_image, 2, block_size* sizeof(cl_float), NULL);
    err |= clSetKernelArg(cl_integrate_image, 4, sizeof(cl_int), &direction);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Launch kernel
    local_size[0] = 1;
    local_size[1] = block_size;

    global_size[0] = ray_tex_dim[0];
    global_size[1] = (block_size - (ray_tex_dim[1] % block_size)) + ray_tex_dim[1];

    work_size[0] = 1;
    work_size[1] = global_size[1];

    global_offset[0] = 0;
    global_offset[1] = 0;

    for (size_t column = 0; column < global_size[0]; column += local_size[0])
    {
        global_offset[0] = column;

        err = clEnqueueNDRangeKernel(*context_cl->getCommandQueue(), cl_integrate_image, 2, global_offset.data(), work_size.data(), local_size.data(), 0, NULL, NULL);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    }

    err = clFinish(*context_cl->getCommandQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    // Read the output data
    Matrix<float> output2(global_size[1]/block_size, ray_tex_dim[0]);

    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;

    region[0] = ray_tex_dim[0];
    region[1] = global_size[1]/block_size;
    region[2] = 1;

    err = clEnqueueReadImage ( 	*context_cl->getCommandQueue(),
        integration_tex_beta_cl,
        CL_TRUE,
        origin.data(),
        region.data(),
        0,
        0,
        output2.data(),
        0, NULL, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    // Sum up the partially reduced array
//    qDebug() << "_____________________Control:" << ray_tex_dim[0];
    Matrix<float> column_sum(1, ray_tex_dim[0], 0.0f);
    max = 0;
    min = 1e9;
    sum = 0;

    for (int i = 0; i < output2.getN(); i++)
    {
        for(int j = 0; j < output2.getM(); j++)
        {
//            qDebug() << "Loop:" << ray_tex_dim[0];
            column_sum[i] += output2[j*output2.getN() + i];
        }

        if (column_sum[i] > max) max = column_sum[i];
        if ((column_sum[i] < min) && (column_sum[i] > 0)) min = column_sum[i];

        sum += column_sum[i];
    }

    if (sum > 0)
    {
        for (int i = 0; i < column_sum.getN(); i++)
        {
            column_sum[i] *= 100000.0/sum;
        }
        max *=  100000.0/sum;
        min *=  100000.0/sum;

        if (isLogarithmic2D)
        {
            for (int i = 0; i < column_sum.getN(); i++)
            {
                if (column_sum[i] <= 0) column_sum[i] = min;
                column_sum[i] = log10(column_sum[i]);
            }
            max =  log10(max);
            min =  log10(min);
        }

        QPolygonF column_polygon;
        column_polygon << QPointF(0,render_surface->height());
        for (int i = 0; i < column_sum.getN(); i++)
        {
            QPointF point_top, point_bottom;

            float value = render_surface->height() - (column_sum[i] - min) / (max - min)*render_surface->height()*0.1;

            point_top.setX(((float) i / (float) column_sum.getN())*render_surface->width());
            point_top.setY(value);

            point_bottom.setX((((float) i + 1) / (float) column_sum.getN())*render_surface->width());
            point_bottom.setY(value);
            column_polygon << point_top << point_bottom;
        }
        column_polygon << QPointF(render_surface->width(),render_surface->height());



        painter->setRenderHint(QPainter::Antialiasing);

        QLinearGradient lgrad(QPointF(0,render_surface->height()*0.9), QPointF(0,render_surface->height()));
                    lgrad.setColorAt(0.0, Qt::blue);
                    lgrad.setColorAt(1.0, Qt::transparent);

        QBrush histogram_brush_lg(lgrad);

        painter->setBrush(histogram_brush_lg);
        painter->setPen(*normal_pen);

        painter->drawPolygon(column_polygon);
    }

}


void VolumeRenderWorker::beginRawGLCalls(QPainter * painter)
{
    painter->beginNativePainting();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
}

void VolumeRenderWorker::endRawGLCalls(QPainter * painter)
{
    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    painter->endNativePainting();
}

void VolumeRenderWorker::setOrthoGrid()
{
    isOrthoGridActive = !isOrthoGridActive;
}

void VolumeRenderWorker::drawRuler(QPainter * painter)
{
    double screen_width = pixel_size[0]*render_surface->width();
    double screen_height = pixel_size[1]*render_surface->height();
    
    // Draw ruler and alignment crosses 
    QVector<QLine> lines;
    
    lines << QLine(ruler[0],ruler[1],ruler[2],ruler[3]);
    lines << QLine(ruler[0],ruler[1]+8000,ruler[0],ruler[1]-8000);
    lines << QLine(ruler[0]+8000,ruler[1],ruler[0]-8000,ruler[1]);
    lines << QLine(ruler[2],ruler[3]+8000,ruler[2],ruler[3]-8000);
    lines << QLine(ruler[2]+8000,ruler[3],ruler[2]-8000,ruler[3]);
    
    QVector<qreal> dashes;
    dashes << 2 << 2;
    
    whatever_pen->setWidthF(1.0);
    whatever_pen->setStyle(Qt::CustomDashLine);
    whatever_pen->setDashPattern(dashes);
    whatever_pen->setColor(QColor(
                                255.0*clear_color_inverse[0],
                                255.0*clear_color_inverse[1],
                                255.0*clear_color_inverse[2],
                                255));
    painter->setPen(*whatever_pen);
    
    painter->drawLines(lines.data(), 5);
    
    // Draw text with info 
    double length = sqrt((ruler[2]-ruler[0])*(ruler[2]-ruler[0])*pixel_size[0]*pixel_size[0] + (ruler[3]-ruler[1])*(ruler[3]-ruler[1])*pixel_size[1]*pixel_size[1]);
    
    QString centerline_string(QString::number(length, 'g', 5)+" 1/Å");
    QRect centerline_string_rect = emph_fontmetric->boundingRect(centerline_string);
    centerline_string_rect += QMargins(5,5,5,5);
    centerline_string_rect.moveBottomLeft(QPoint(ruler[0]+5, ruler[1]-5));
    
    painter->setPen(*normal_pen);
    painter->setBrush(*fill_brush);
    painter->drawRoundedRect(centerline_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(centerline_string_rect, Qt::AlignCenter, centerline_string);
}

void VolumeRenderWorker::drawGrid(QPainter * painter)
{
    // Draw grid lines, the center of the screen is (0,0)
    double screen_width = pixel_size[0]*render_surface->width();
    double screen_height = pixel_size[1]*render_surface->height();

    // Find appropriate tick intervals
    int levels = 2;
    int qualified_levels = 0;
    double level_min_ticks = 2.0;
    double min_pix_interdist = 20.0;
    double exponent = 5.0;

    Matrix<int> exponent_by_level(1,levels);
    Matrix<int> ticks_by_level(1,levels);

    while (qualified_levels < levels)
    {

        if (((screen_width / pow(10.0, (double) exponent)) > level_min_ticks))
        {
            if ((render_surface->width()/(screen_width / pow(10.0, (double) exponent)) < min_pix_interdist)) break;
            exponent_by_level[qualified_levels] = exponent;
            ticks_by_level[qualified_levels] = (screen_width / pow(10.0, (double) exponent)) + 1;
            qualified_levels++;
        }
        exponent--;
    }

    if (qualified_levels > 0)
    {
        // Draw lines according to tick intervals
        Matrix<QPointF> vertical_lines(1, (int)((screen_width / pow(10.0, (double) exponent_by_level[qualified_levels-1])) + 5)*2);
        Matrix<QPointF> horizontal_lines(1, (int)((screen_width / pow(10.0, (double) exponent_by_level[qualified_levels-1])) + 5)*2);
        for (int i = qualified_levels-1; i >= 0; i--)
        {
            vertical_lines[2] = QPointF(render_surface->width()*0.5, render_surface->height());
            vertical_lines[3] = QPointF(render_surface->width()*0.5, 0);

            for (int j = 1; j < ticks_by_level[i]/2+1; j++)
            {
                vertical_lines[j*4] = QPointF((j * pow(10.0, exponent_by_level[i]))/(screen_width*0.5)*render_surface->width()*0.5 + render_surface->width()*0.5, render_surface->height());
                vertical_lines[j*4+1] = QPointF((j * pow(10.0, exponent_by_level[i]))/(screen_width*0.5)*render_surface->width()*0.5 + render_surface->width()*0.5, 0);

                vertical_lines[j*4+2] = QPointF((-j * pow(10.0, exponent_by_level[i]))/(screen_width*0.5)*render_surface->width()*0.5 + render_surface->width()*0.5, render_surface->height());
                vertical_lines[j*4+3] = QPointF((-j * pow(10.0, exponent_by_level[i]))/(screen_width*0.5)*render_surface->width()*0.5 + render_surface->width()*0.5, 0);
            }

            horizontal_lines[2] = QPointF(render_surface->width(), render_surface->height()*0.5);
            horizontal_lines[3] = QPointF(0,render_surface->height()*0.5);

            for (int j = 1; j < ticks_by_level[i]/2+1; j++)
            {
                horizontal_lines[j*4] = QPointF(render_surface->width(), (j * pow(10.0, exponent_by_level[i]))/(screen_height*0.5)*render_surface->height()*0.5 + render_surface->height()*0.5);
                horizontal_lines[j*4+1] = QPointF(0,(j * pow(10.0, exponent_by_level[i]))/(screen_height*0.5)*render_surface->height()*0.5 + render_surface->height()*0.5);

                horizontal_lines[j*4+2] = QPointF(render_surface->width(), (-j * pow(10.0, exponent_by_level[i]))/(screen_height*0.5)*render_surface->height()*0.5 + render_surface->height()*0.5);
                horizontal_lines[j*4+3] = QPointF(0,(-j * pow(10.0, exponent_by_level[i]))/(screen_height*0.5)*render_surface->height()*0.5 + render_surface->height()*0.5);
            }

            if (i == 0)
            {
//                QVector<qreal> dashes;
//                dashes << 6 << 6;
                
                whatever_pen->setWidthF(1.0);
                whatever_pen->setStyle(Qt::SolidLine);
//                whatever_pen->setDashPattern(dashes);
//                whatever_pen->setColor(QColor(40,255,40,255));
                whatever_pen->setColor(QColor(50,50,255,255));
                painter->setPen(*whatever_pen);
            }
            else
            {
                QVector<qreal> dashes;
                dashes << 3 << 3;
                
                whatever_pen->setWidthF(0.3);
                whatever_pen->setStyle(Qt::CustomDashLine);
                whatever_pen->setDashPattern(dashes);
//                whatever_pen->setColor(QColor(50,50,255,255));
                whatever_pen->setColor(QColor(255.0*clear_color_inverse[0],
                                        255.0*clear_color_inverse[1],
                                        255.0*clear_color_inverse[2],
                                        255));
                painter->setPen(*whatever_pen);
            }

            painter->drawLines(vertical_lines.data()+2, (ticks_by_level[i]/2)*2+1);
            painter->drawLines(horizontal_lines.data()+2, (ticks_by_level[i]/2)*2+1);
        }

        normal_pen->setWidthF(1.0);
    }
}

void VolumeRenderWorker::alignX()
{
    RotationMatrix<double> x_aligned;
    x_aligned.setYRotation(pi*0.5);
    
//    x_aligned.print(2);
    
    RotationMatrix<double> delta_rotation;
    
    delta_rotation = x_aligned * scalebar_rotation.getInverse();
    
//    delta_rotation.print(2);
    
    scalebar_rotation = x_aligned;
    
    rotation = delta_rotation * rotation;
    
//    scalebar_rotation.print(2);
    
//    rotation.print(2);
}

void VolumeRenderWorker::alignY()
{
    RotationMatrix<double> y_aligned;
    y_aligned.setXRotation(-pi*0.5);
    
//    y_aligned.print(2);
    
    RotationMatrix<double> delta_rotation;
    
    delta_rotation = y_aligned * scalebar_rotation.getInverse();
    
//    delta_rotation.print(2);
    
    scalebar_rotation = y_aligned;
    
    rotation = delta_rotation * rotation;
    
//    scalebar_rotation.print(2);
    
//    rotation.print(2);
}
void VolumeRenderWorker::alignZ()
{
    RotationMatrix<double> z_aligned;
    z_aligned.setYRotation(0.0);
    
//    z_aligned.print(2);
    
    RotationMatrix<double> delta_rotation;
    
    delta_rotation = z_aligned * scalebar_rotation.getInverse();
    
//    delta_rotation.print(2);
    
    scalebar_rotation = z_aligned;
    
    rotation = delta_rotation * rotation;
    
//    scalebar_rotation.print(2);
    
//    rotation.print(2);
    
}
void VolumeRenderWorker::rotateLeft()
{
    RotationMatrix<double> rot;
    rot.setYRotation(pi*0.25);
    
    scalebar_rotation = rot * scalebar_rotation;
    
    rotation = rot * rotation;
}
void VolumeRenderWorker::rotateRight()
{
    RotationMatrix<double> rot;
    rot.setYRotation(-pi*0.25);
    
    scalebar_rotation = rot * scalebar_rotation;
    
    rotation = rot * rotation;
}
void VolumeRenderWorker::rotateUp()
{
    RotationMatrix<double> rot;
    rot.setXRotation(pi*0.25);
    
    scalebar_rotation = rot * scalebar_rotation;
    
    rotation = rot * rotation;
}
void VolumeRenderWorker::rotateDown()
{
    RotationMatrix<double> rot;
    rot.setXRotation(-pi*0.25);
    
    scalebar_rotation = rot * scalebar_rotation;
    
    rotation = rot * rotation;
}


void VolumeRenderWorker::computePixelSize()
{
    setViewMatrices();

    // Calculate the current pixel size()
    Matrix<double> ndc00(4,1);
    ndc00[0] = 0.0;
    ndc00[1] = 0.0;
    ndc00[2] = -1.0;
    ndc00[3] = 1.0;

    Matrix<double> ndc01(4,1);
    ndc01[0] = 0.0;
    ndc01[1] = 2.0/(double)render_surface->height();
    ndc01[2] = -1.0;
    ndc01[3] = 1.0;

    Matrix<double> ndc10(4,1);
    ndc10[0] = 2.0/(double)render_surface->width();
    ndc10[1] = 0.0;
    ndc10[2] = -1.0;
    ndc10[3] = 1.0;

    Matrix<double> xyz_00(4,1);
    Matrix<double> xyz_01(4,1);
    Matrix<double> xyz_10(4,1);

    xyz_00 = view_matrix.getInverse()*ndc00;
    xyz_01 = view_matrix.getInverse()*ndc01;
    xyz_10 = view_matrix.getInverse()*ndc10;

    Matrix<double> w_vec = xyz_00 - xyz_10;
    Matrix<double> h_vec = xyz_00 - xyz_01;


    pixel_size[0] = std::sqrt(w_vec[0]*w_vec[0] + w_vec[1]*w_vec[1] + w_vec[2]*w_vec[2]);
    pixel_size[1] = std::sqrt(h_vec[0]*h_vec[0] + h_vec[1]*h_vec[1] + h_vec[2]*h_vec[2]);


//    ray_tex_dim.print(0,"ray_tex_dim");
//    pixel_size.print(4,"Pixel Size");
}

void VolumeRenderWorker::drawOverlay(QPainter * painter)
{
    // Draw text to indicate lab reference frame directions
    Matrix<float> x_high(1,3,0), y_high(1,3,0), z_high(1,3,0);
    float length = data_view_extent[1] - data_view_extent[0];
    x_high[0] = data_view_extent[1] + length * 0.05;
    x_high[1] = data_view_extent[2] + length * 0.5;
    x_high[2] = data_view_extent[4] + length * 0.5;
    
    y_high[0] = data_view_extent[0] + length * 0.5;
    y_high[1] = data_view_extent[3] + length * 0.05;
    y_high[2] = data_view_extent[4] + length * 0.5;

    z_high[0] = data_view_extent[0] + length * 0.5;
    z_high[1] = data_view_extent[2] + length * 0.5;
    z_high[2] = data_view_extent[5] + length * 0.05;
    
    Matrix<float> x_2d(1,2,0), y_2d(1,2,0), z_2d(1,2,0);
    getPosition2D(x_2d.data(), x_high.data(), &view_matrix);
    getPosition2D(y_2d.data(), y_high.data(), &view_matrix);
    getPosition2D(z_2d.data(), z_high.data(), &view_matrix);
    
    painter->drawText(QPointF((x_2d[0]+ 1.0) * 0.5 *render_surface->width(), (1.0 - ( x_2d[1]+ 1.0) * 0.5) *render_surface->height()), QString("X (towards source)"));
    painter->drawText(QPointF((y_2d[0]+ 1.0) * 0.5 *render_surface->width(), (1.0 - ( y_2d[1]+ 1.0) * 0.5) *render_surface->height()), QString("Y (up)"));
    painter->drawText(QPointF((z_2d[0]+ 1.0) * 0.5 *render_surface->width(), (1.0 - ( z_2d[1]+ 1.0) * 0.5) *render_surface->height()), QString("Z"));

    // Position scalebar tick labels
    if (isScalebarActive)
    {
        for (int i = 0; i < n_position_scalebar_ticks; i++)
        {
            painter->drawText(QPointF(position_scalebar_ticks[i*3+0], position_scalebar_ticks[i*3+1]), QString::number(position_scalebar_ticks[i*3+2]));
        }
    }

    // Count scalebar tick labels
    if (n_count_scalebar_ticks >= 2)
    {
        for (int i = 0; i < n_count_scalebar_ticks; i++)
        {
            painter->drawText(QPointF(count_scalebar_ticks[i*3+0], count_scalebar_ticks[i*3+1]), QString::number(count_scalebar_ticks[i*3+2], 'g', 4));
        }
    }
    else
    {
        for (int i = 0; i < n_count_minor_scalebar_ticks; i++)
        {
            painter->drawText(QPointF(count_minor_scalebar_ticks[i*3+0], count_minor_scalebar_ticks[i*3+1]), QString::number(count_minor_scalebar_ticks[i*3+2], 'g', 4));
        }
    }
    
    // Distance from (000), length of center line
    double distance = std::sqrt(centerline_coords[3]*centerline_coords[3] + centerline_coords[4]*centerline_coords[4] + centerline_coords[5]*centerline_coords[5]);

    QString centerline_string("Distance from (000): "+QString::number(distance, 'g', 5)+" 1/Å");
    QRect centerline_string_rect = emph_fontmetric->boundingRect(centerline_string);
    centerline_string_rect += QMargins(5,5,5,5);
    centerline_string_rect.moveBottomRight(QPoint(render_surface->width()-5,render_surface->height()-5));

    painter->setBrush(*fill_brush);
    painter->drawRoundedRect(centerline_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(centerline_string_rect, Qt::AlignCenter, centerline_string);

    // Draw some text at the (000) position
    Matrix<float> zero_position(1,4);
    getPosition2D(zero_position.data(), centerline_coords.data(), &view_matrix);
    painter->drawText(QPointF(zero_position[0]*render_surface->width()*0.5 + render_surface->width()*0.5, render_surface->height() - (zero_position[1]*render_surface->height()*0.5 + render_surface->height()*0.5)), "(000)");

    // Fps
    QString fps_string("Fps: "+QString::number(getFps(), 'f', 0));
    QRect fps_string_rect = emph_fontmetric->boundingRect(fps_string);
    fps_string_rect.setWidth(std::max(fps_string_width_prev, fps_string_rect.width()));
    fps_string_width_prev = fps_string_rect.width();
    fps_string_rect += QMargins(5,5,5,5);
    fps_string_rect.moveTopRight(QPoint(render_surface->width()-5,5));

    painter->setBrush(*fill_brush);
    painter->drawRoundedRect(fps_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(fps_string_rect, Qt::AlignCenter, fps_string);

    // Texture resolution
    QString resolution_string("Texture resolution: "+QString::number(ray_tex_resolution, 'f', 1)+"%");//+"%, Volume Rendering Fps: "+QString::number(fps_requested));
    QRect resolution_string_rect = emph_fontmetric->boundingRect(resolution_string);
    resolution_string_rect += QMargins(5,5,5,5);
    resolution_string_rect.moveBottomLeft(QPoint(5, render_surface->height() - 5));

    painter->drawRoundedRect(resolution_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(resolution_string_rect, Qt::AlignCenter, resolution_string);
    
    // Scalebar multiplier
    QString multiplier_string("x"+QString::number(scalebar_multiplier)+" 1/Å");
    multiplier_string_rect = emph_fontmetric->boundingRect(multiplier_string);
    multiplier_string_rect += QMargins(5,5,5,5);
    multiplier_string_rect.moveTopRight(QPoint(render_surface->width()-5, fps_string_rect.bottom() + 5));

    painter->drawRoundedRect(multiplier_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(multiplier_string_rect, Qt::AlignCenter, multiplier_string);
}

void VolumeRenderWorker::drawPositionScalebars()
{
    scalebar_coord_count = setScaleBars();

    shared_window->std_3d_color_program->bind();
    glEnableVertexAttribArray(shared_window->std_3d_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, scalebar_vbo);
    glVertexAttribPointer(shared_window->std_3d_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glUniform4fv(shared_window->std_3d_color, 1, clear_color_inverse.data());
    
    glUniformMatrix4fv(shared_window->std_3d_transform, 1, GL_FALSE, scalebar_view_matrix.getColMajor().toFloat().data());
    
    glDrawArrays(GL_LINES,  0, scalebar_coord_count);
    
    // Draw in addition the lab reference frame (directions)
    glUniformMatrix4fv(shared_window->std_3d_transform, 1, GL_FALSE, view_matrix.getColMajor().toFloat().data());
    
    glDrawArrays(GL_LINES,  scalebar_coord_count - 6, 6);
    
    glDisableVertexAttribArray(shared_window->std_3d_fragpos);

    shared_window->std_3d_color_program->release();
}

void VolumeRenderWorker::drawCountScalebar(QPainter *painter)
{
    /*
     * Based on the current display values (min and max), draw ticks on the counts scalebar. There are major and minor ticks.
     * */
    
    double data_min, data_max;//, data_delta;
    double tick_interdist_min = 10; // pixels
    double exponent;
    
    // Draw transfer function bounding box
    QRect tsf_rect(0, 0, 20, render_surface->height() - (multiplier_string_rect.bottom() + 5) - 50);
    tsf_rect += QMargins(30,5,5,5);
    tsf_rect.moveTopRight(QPoint(render_surface->width()-5, multiplier_string_rect.bottom() + 5));

    painter->setBrush(*fill_brush);
//    painter->drawRoundedRect(tsf_rect, 5, 5, Qt::AbsoluteSize);

    tsf_rect -= QMargins(30,5,5,5);
    Matrix<GLfloat> gl_tsf_rect(4,2);
    glRect(&gl_tsf_rect, &tsf_rect);
    
    
    // Find appropriate tick positions
    if (isModelActive)
    {
        data_min = tsf_parameters_model[2];
        data_max = tsf_parameters_model[3];
    }
    else
    {
        data_min = tsf_parameters_svo[2];
        data_max = tsf_parameters_svo[3];      
    }
    
    if(isLogarithmic)
    {
        if (data_min <= 0) data_min = 1.0e-9;
        if (data_max <= 0) data_min = 1.0e-9;
        
        data_min = log10(data_min);
        data_max = log10(data_max);
    }
    
    if (data_min < data_max)
    {
        double start, current;
        int iter = 0, num_ticks = 0;
        tickzerize(data_min, data_max, (double) tsf_rect.height(), tick_interdist_min, &exponent, &start, &num_ticks);
        current = start;
        n_count_scalebar_ticks = 0, n_count_minor_scalebar_ticks = 0;;
                
        Matrix<double> ticks(num_ticks+1,4);
        
        while ((current < data_max) && (iter < ticks.size()/4))
        {
            ticks[iter*4+0] = -1.0 + ((tsf_rect.left()-10)/ (double) render_surface->width())*2.0;
            ticks[iter*4+1] = -1.0 + ((render_surface->height() - tsf_rect.bottom() + (current - data_min)/(data_max-data_min)*tsf_rect.height())/ (double) render_surface->height())*2.0;
            ticks[iter*4+2] = -1.0 + (tsf_rect.right()/ (double) render_surface->width())*2.0;
            ticks[iter*4+3] = -1.0 + ((render_surface->height() - tsf_rect.bottom() + (current - data_min)/(data_max-data_min)*tsf_rect.height())/ (double) render_surface->height())*2.0;
            
            if (((int)round(current*pow(10.0, -exponent)) % 10) == 0)
            {
                ticks[iter*4+0] = -1.0 + ((tsf_rect.left()-25)/ (double) render_surface->width())*2.0;
                
                if(n_count_scalebar_ticks < count_scalebar_ticks.size())
                {
                    count_scalebar_ticks[n_count_scalebar_ticks*3+0] = tsf_rect.left()-35;
                    count_scalebar_ticks[n_count_scalebar_ticks*3+1] = tsf_rect.bottom() - (current - data_min)/(data_max-data_min)*tsf_rect.height();
                    if (isLogarithmic) count_scalebar_ticks[n_count_scalebar_ticks*3+2] = pow(10,current);
                    else count_scalebar_ticks[n_count_scalebar_ticks*3+2] = current;
                    
                    n_count_scalebar_ticks++;
                }
            }
            
            if(n_count_minor_scalebar_ticks < count_minor_scalebar_ticks.size())
            {
                count_minor_scalebar_ticks[n_count_minor_scalebar_ticks*3+0] = tsf_rect.left()-35;
                count_minor_scalebar_ticks[n_count_minor_scalebar_ticks*3+1] = tsf_rect.bottom() - (current - data_min)/(data_max-data_min)*tsf_rect.height();
                if (isLogarithmic) count_minor_scalebar_ticks[n_count_minor_scalebar_ticks*3+2] = pow(10,current);
                else count_minor_scalebar_ticks[n_count_minor_scalebar_ticks*3+2] = current;
                
                n_count_minor_scalebar_ticks++;
            }
            
            current += pow(10.0, exponent);
            iter++;
        }
        
        beginRawGLCalls(painter);
        setVbo(count_scalebar_vbo, ticks.toFloat().data(), iter*4, GL_STATIC_DRAW);
        
        // Draw transfer function texture
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
        
        
        // Draw the ticks
        RotationMatrix<double> identity;
        identity.setIdentity(2);
        
        shared_window->std_2d_color_program->bind();
        glEnableVertexAttribArray(shared_window->std_2d_color_fragpos);
    
        glBindBuffer(GL_ARRAY_BUFFER, count_scalebar_vbo);
        glVertexAttribPointer(shared_window->std_2d_color_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    
        glUniformMatrix2fv(shared_window->std_2d_color_transform, 1, GL_FALSE, identity.getColMajor().toFloat().data());
    
        glUniform4fv(shared_window->std_2d_color_color, 1, clear_color_inverse.data());
    
        glDrawArrays(GL_LINES,  0, iter*2);
    
        glDisableVertexAttribArray(shared_window->std_2d_color_fragpos);
    
        shared_window->std_2d_color_program->release();
        
        endRawGLCalls(painter);
    
        painter->setBrush(*normal_brush);
        painter->drawRect(tsf_rect);
    }
    
}


void VolumeRenderWorker::tickzerize(double min, double max, double size, double min_interdist, double * qualified_exponent, double * start, int * num_ticks)
{
    double delta = max - min;
    double exponent = -10;
    
    while (exponent < 10)
    {
        if ((size/(delta / pow(10.0, exponent))) >= min_interdist) 
        {
            *qualified_exponent = exponent;
            *num_ticks = delta / pow(10.0, exponent);
            break;
        }
        exponent++;
    }
    *start = ((int) ceil(min * pow(10.0, -*qualified_exponent))) * pow(10.0, *qualified_exponent);; 
}

void VolumeRenderWorker::drawRayTex()
{
    // Volume rendering
    if (isModelActive) raytrace(cl_model_raytrace);
    else if(isSvoInitialized) raytrace(cl_svo_raytrace);

    // Draw texture given one of the above is true
    if (isModelActive || isSvoInitialized)
    {
//        glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);

        shared_window->std_2d_tex_program->bind();

        glActiveTexture(GL_TEXTURE0);
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

//        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
}

void VolumeRenderWorker::raytrace(cl_kernel kernel)
{
    setRayTexture();

//    if (isShadowActive) setShadowVector();
    setShadowVector();

    // Aquire shared CL/GL objects
    glFinish();
    err = clEnqueueAcquireGLObjects(*context_cl->getCommandQueue(), 1, &ray_tex_cl, 0, 0, 0);
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

            err = clEnqueueNDRangeKernel(*context_cl->getCommandQueue(), kernel, 2, call_offset.data(), area_per_call.data(), ray_loc_ws.data(), 0, NULL, NULL);
            if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
        }
    }

    err = clFinish(*context_cl->getCommandQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    work_time = (double) ray_kernel_timer.nsecsElapsed();

    // Calculate how much the quality must be reduced (if any) in order to achieve the requested fps. This fps does not take into account the time spent on other rendering details.
    double actual_time = work_time * 1.0e-9;
    double requested_time = 1.0 / fps_requested;
    quality_factor = requested_time / actual_time;

    // Release shared CL/GL objects
    err = clEnqueueReleaseGLObjects(*context_cl->getCommandQueue(), 1, &ray_tex_cl, 0, 0, 0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clFinish(*context_cl->getCommandQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void VolumeRenderWorker::setSvo(SparseVoxelOcttree * svo)
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

    Matrix<size_t> pool_dim(1,3);
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
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    isSvoInitialized = true;
}

void VolumeRenderWorker::resetViewMatrix()
{
    data_scaling.setIdentity(4);
    rotation.setIdentity(4);
    scalebar_rotation.setIdentity (4);
    data_translation.setIdentity(4);
}

size_t VolumeRenderWorker::setScaleBars()
{
    // Draw the scalebars. The coordinates of the ticks are independent of the position in the volume, so it is a relative scalebar.
    double length = data_view_extent[1] - data_view_extent[0];

    double tick_interdistance_min = 0.005*length; // % of length

    int tick_levels = 0;
    int tick_levels_max = 2;

    size_t coord_counter = 0;

    Matrix<GLfloat> scalebar_coords(20000,3);

    n_position_scalebar_ticks = 0;

    // Calculate positions of ticks
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
                    // X-tick cross
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

                    // Y-tick cross
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

                    // Z-tick cross
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


                    // Get positions for tick text
                    if (tick_levels == tick_levels_max - 1)
                    {
                        if ((size_t) n_position_scalebar_ticks+3 < position_scalebar_ticks.getM())
                        {
                            getPosition2D(position_scalebar_ticks.data() + 3 * n_position_scalebar_ticks, scalebar_coords.data() + (coord_counter+0)*3, &scalebar_view_matrix);
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] = (position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] + 1.0) * 0.5 *render_surface->width();
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] = (1.0 - (position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] + 1.0) * 0.5) *render_surface->height();
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 2] = j * 0.1;
                            n_position_scalebar_ticks++;

                            getPosition2D(position_scalebar_ticks.data() + 3 * n_position_scalebar_ticks, scalebar_coords.data() + (coord_counter+4)*3, &scalebar_view_matrix);
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] = (position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] + 1.0) * 0.5 *render_surface->width();
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] = (1.0 - (position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] + 1.0) * 0.5) *render_surface->height();
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 2] = j * 0.1;
                            n_position_scalebar_ticks++;

                            getPosition2D(position_scalebar_ticks.data() + 3 * n_position_scalebar_ticks, scalebar_coords.data() + (coord_counter+8)*3, &scalebar_view_matrix);
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] = (position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] + 1.0) * 0.5 *render_surface->width();
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] = (1.0 - (position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] + 1.0) * 0.5) *render_surface->height();
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 2] = j * 0.1;
                            n_position_scalebar_ticks++;
                        }
                        scalebar_multiplier = tick_interdistance * 10.0;
                    }
                    coord_counter += 12;
                }
            }
            tick_levels++;
        }
    }

    // Base cross 
    // X
    scalebar_coords[(coord_counter+0)*3+0] = data_view_extent[0];
    scalebar_coords[(coord_counter+0)*3+1] = data_view_extent[2] + length * 0.5;
    scalebar_coords[(coord_counter+0)*3+2] = data_view_extent[4] + length * 0.5;

    scalebar_coords[(coord_counter+1)*3+0] = data_view_extent[1];
    scalebar_coords[(coord_counter+1)*3+1] = data_view_extent[2] + length * 0.5;
    scalebar_coords[(coord_counter+1)*3+2] = data_view_extent[4] + length * 0.5;

    // Y
    scalebar_coords[(coord_counter+2)*3+0] = data_view_extent[0] + length * 0.5;
    scalebar_coords[(coord_counter+2)*3+1] = data_view_extent[2];
    scalebar_coords[(coord_counter+2)*3+2] = data_view_extent[4] + length * 0.5;

    scalebar_coords[(coord_counter+3)*3+0] = data_view_extent[0] + length * 0.5;
    scalebar_coords[(coord_counter+3)*3+1] = data_view_extent[3];
    scalebar_coords[(coord_counter+3)*3+2] = data_view_extent[4] + length * 0.5;

    // Z
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

void VolumeRenderWorker::setQuality(int value)
{
   fps_requested = (float) value;
}

void VolumeRenderWorker::setProjection()
{
    isOrthonormal = !isOrthonormal;
    ctc_matrix.setProjection(isOrthonormal);

    float f;
    if (isOrthonormal) f = 1;
    else f = 1.0/1.1;

    projection_scaling[0] = f;
    projection_scaling[5] = f;
    projection_scaling[10] = f;
}

void VolumeRenderWorker::setBackground()
{
    isBackgroundBlack = !isBackgroundBlack;

    Matrix<GLfloat> tmp;
    tmp = clear_color;

    // Swap color
    clear_color = clear_color_inverse;
    clear_color_inverse = tmp;

    normal_pen->setColor(QColor(255.0*clear_color_inverse[0],
                        255.0*clear_color_inverse[1],
                        255.0*clear_color_inverse[2],
                        255));
    whatever_pen->setColor(QColor(255.0*clear_color_inverse[0],
                        255.0*clear_color_inverse[1],
                        255.0*clear_color_inverse[2],
                        255));
    fill_brush->setColor(QColor(255.0*clear_color[0],
                        255.0*clear_color[1],
                        255.0*clear_color[2],
                        255.0*0.7));
//    normal_pen->setWidth(1);
}
void VolumeRenderWorker::setLogarithmic()
{
    isLogarithmic = !isLogarithmic;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWorker::setLogarithmic2D()
{
    isLogarithmic2D = !isLogarithmic2D;
}
void VolumeRenderWorker::setDataStructure()
{
    isDSActive = !isDSActive;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWorker::setSlicing()
{
    isSlicingActive = !isSlicingActive;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWorker::setShadow()
{
    isShadowActive = !isShadowActive;

    if (isInitialized) setMiscArrays();
}

void VolumeRenderWorker::setShadowVector()
{
    Matrix<float> shadow_kernel_arg;

    shadow_kernel_arg = shadow_vector;

    shadow_kernel_arg = rotation.getInverse().toFloat()*shadow_kernel_arg;

    clSetKernelArg(cl_model_raytrace, 11, sizeof(cl_float4),  shadow_kernel_arg.data());
}

void VolumeRenderWorker::setIntegration2D()
{
    isIntegration2DActive = !isIntegration2DActive;

    if (!isOrthonormal) emit changedMessageString("\nWarning: Perspective projection is currently active.");

    if (isInitialized) setMiscArrays();
}
void VolumeRenderWorker::setIntegration3D()
{
//    qDebug();

    isIntegration3DActive = !isIntegration3DActive;

    if (!isOrthonormal) emit changedMessageString("\nWarning: Perspective projection is currently active.");

    if (isInitialized) setMiscArrays();
}
void VolumeRenderWorker::setTsfColor(int value)
{
    tsf_color_scheme = value;
    if (isInitialized) setTsfTexture();
}
void VolumeRenderWorker::setTsfAlpha(int value)
{
    tsf_alpha_scheme = value;
    if (isInitialized) setTsfTexture();
}
void VolumeRenderWorker::setDataMin(double value)
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
void VolumeRenderWorker::setDataMax(double value)
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
void VolumeRenderWorker::setAlpha(double value)
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
void VolumeRenderWorker::setBrightness(double value)
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
void VolumeRenderWorker::setUnitcell()
{
    isUnitcellActive = !isUnitcellActive;
}
void VolumeRenderWorker::setModel()
{
    isModelActive = !isModelActive;
}
void VolumeRenderWorker::setModelParam0(double value)
{
    model_misc_floats[0] = value;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWorker::setModelParam1(double value)
{
    model_misc_floats[1] = value;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWorker::setModelParam2(double value)
{
    model_misc_floats[2] = value;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWorker::setModelParam3(double value)
{
    model_misc_floats[3] = value;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWorker::setModelParam4(double value)
{
    model_misc_floats[4] = value;
    if (isInitialized) setMiscArrays();
}
void VolumeRenderWorker::setModelParam5(double value)
{
    model_misc_floats[5] = value;
    if (isInitialized) setMiscArrays();
}

void VolumeRenderWorker::setScalebar()
{
    isScalebarActive = !isScalebarActive;
}
void VolumeRenderWorker::toggleRuler()
{
    isRulerActive = !isRulerActive;
    qDebug() << isRulerActive;
}

