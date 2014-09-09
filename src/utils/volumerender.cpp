#include "volumerender.h"

/* Useful C++ libs */
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
//#include <cstdio>

#include <QDebug>

/* GL and CL*/
#include <CL/opencl.h>

/* QT */
#include <QTimer>
#include <QElapsedTimer>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QResizeEvent>
#include <QPolygonF>
#include <QScreen>
#include <QPainter>
#include <QOpenGLFramebufferObject>

VolumeRenderWindow::VolumeRenderWindow()
    : isInitialized(false)
    , gl_worker(0)
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
//    if (isWorkerBusy)
//    {
//        if (isAnimating) renderLater();
//        return;
//    }
    else if (!isWorkerBusy)
    {
        if (!isInitialized) initializeWorker();

        if (gl_worker)
        {
            if (isThreaded)
            {
//                qDebug() << "render";
                isWorkerBusy = true;
                worker_thread->start();
                emit render();
            }
//            else
//            {
//                context_gl->makeCurrent(this);
//                gl_worker->process();
//                emit render();
//            }

        }
    }
//    if (isAnimating) renderLater();
    renderLater();
}

void VolumeRenderWindow::initializeWorker()
{
    initializeGLContext();

    gl_worker = new VolumeRenderWorker;
    gl_worker->setRenderSurface(this);
    gl_worker->setOpenGLContext(context_gl);
    gl_worker->setOpenCLContext(context_cl);
    gl_worker->setSharedWindow(shared_window);
    gl_worker->setMultiThreading(isThreaded);

    if (isThreaded)
    {
        // Set up worker thread
        gl_worker->moveToThread(worker_thread);
        connect(this, SIGNAL(render()), gl_worker, SLOT(process()));
        connect(this, SIGNAL(stopRendering()), worker_thread, SLOT(quit()));
        connect(gl_worker, SIGNAL(finished()), this, SLOT(setSwapState()));

        // Transfering mouse events
        connect(this, SIGNAL(metaMouseMoveEventCaught(int, int, int, int, int, int, int)), gl_worker, SLOT(metaMouseMoveEvent(int, int, int, int, int, int, int)));
//        connect(this, SIGNAL(mouseMoveEventCaught(QMouseEvent)), gl_worker, SLOT(mouseMoveEvent(QMouseEvent)));
        connect(this, SIGNAL(metaMousePressEventCaught(int, int, int, int, int, int, int)), gl_worker, SLOT(metaMousePressEvent(int, int, int, int, int, int, int)));
        connect(this, SIGNAL(metaMouseReleaseEventCaught(int, int, int, int, int, int, int)), gl_worker, SLOT(metaMouseReleaseEvent(int, int, int, int, int, int, int)));
        connect(this, SIGNAL(resizeEventCaught(QResizeEvent*)), gl_worker, SLOT(resizeEvent(QResizeEvent*)));//, Qt::DirectConnection);
        connect(this, SIGNAL(wheelEventCaught(QWheelEvent*)), gl_worker, SLOT(wheelEvent(QWheelEvent*)), Qt::DirectConnection);
        
        emit render();
    }
//    else
//    {
//        connect(this, SIGNAL(mouseMoveEventCaught(QMouseEvent*)), gl_worker, SLOT(mouseMoveEvent(QMouseEvent*)), Qt::DirectConnection);
//        connect(this, SIGNAL(resizeEventCaught(QResizeEvent*)), gl_worker, SLOT(resizeEvent(QResizeEvent*)));//, Qt::DirectConnection);
//        connect(this, SIGNAL(wheelEventCaught(QWheelEvent*)), gl_worker, SLOT(wheelEvent(QWheelEvent*)), Qt::DirectConnection);

//        connect(this, SIGNAL(render()), gl_worker, SLOT(process()));
//    }
    
    
    
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
      isOrthonormal(false),
      isLogarithmic(true),
      isModelActive(true),
      isUnitcellActive(true),
      isSvoInitialized(false),
      isScalebarActive(true),
      isSlicingActive(false),
      isIntegration2DActive(false),
      isIntegration3DActive(true),
//      isRendering(true),
      isShadowActive(false),
      isLogarithmic2D(false),
      isOrthoGridActive(false),
      isBackgroundBlack(false),
      isDataExtentReadOnly(true),
      isCenterlineActive(true),
      isRulerActive(false),
      isLMBDown(false),
      isURotationActive(false),
      isLabFrameActive(true),
      isMiniCellActive(true),
      isCountIntegrationActive(false),
      n_marker_indices(0),
      quality_percentage(20)
{
    // Marker
    markers_selected_indices.set(100,1,0);
            
    // Matrices
    double extent[8] = {
        -pi,pi,
        -pi,pi,
        -pi,pi,
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
    minicell_scaling.setIdentity(4);
    bbox_translation.setIdentity(4);
    normalization_scaling.setIdentity(4);
    scalebar_view_matrix.setIdentity(4);
    scalebar_rotation.setIdentity(4);
    projection_scaling.setIdentity(4);
    projection_scaling[0] = 0.7;
    projection_scaling[5] = 0.7;
    projection_scaling[10] = 0.7;
    unitcell_view_matrix.setIdentity(4);

    double N = 0.1;
    double F = 10.0;
    double fov = 20.0;

    bbox_translation[11] = -N -(F - N)*0.5;
    ctc_matrix.setN(N);
    ctc_matrix.setF(F);
    ctc_matrix.setFov(fov);
    ctc_matrix.setProjection(isOrthonormal);
    
    // hkl selection
    hklCurrent.set(1,3,0);
    
    // hkl text
    hkl_text.set(4*4*4,6,0);
    
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
//    fps_requested = 60;
//    work = 1.0;
//    work_time = 0.0;
//    quality_factor = 0.5;

    // Scalebars
    position_scalebar_ticks.reserve(100,3);
    count_scalebar_ticks.reserve(100,3);
    count_minor_scalebar_ticks.reserve(100,3);
    n_count_scalebar_ticks = 0;
    n_count_minor_scalebar_ticks = 0;
    n_position_scalebar_ticks = 0;
            

    // Color    
    white.set(1,1,1,0.4);
    black.set(0,0,0,0.4);
    yellow.set(1,0.2,0,0.8);
    red.set(1,0,0,1);
    green.set(0,1,0,1);
    green_light.set(0.3,1,0.3,0.9);
    blue.set(0,0,1,1);
    blue_light.set(0.1,0.1,1.0,0.9);
    magenta.set(1.0,0.0,0.7,0.9);
    magenta_light.set(1.0,0.1,0.7,0.9);
    clear_color = white;
    clear_color_inverse = black;
    centerline_color = yellow;
    marker_line_color = blue; 

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
    
    // Roll
    accumulated_roll = 0;
    
    identity.setIdentity(4);
}

VolumeRenderWorker::~VolumeRenderWorker()
{
    if (isInitialized)
    {
        glDeleteBuffers(1, &lab_frame_vbo);
        glDeleteBuffers(1, &scalebar_vbo);
        glDeleteBuffers(1, &count_scalebar_vbo);
        glDeleteBuffers(1, &centerline_vbo);
        glDeleteBuffers(1, &point_vbo);
        glDeleteBuffers(1, &point_vbo);
        glDeleteBuffers(1, &unitcell_vbo);
        glDeleteBuffers(1, &marker_centers_vbo);
        glDeleteBuffers(1, &minicell_vbo);
    }
}

float VolumeRenderWorker::sumGpuArray(cl_mem cl_data, unsigned int read_size, size_t work_group_size)
{
    /* Set initial kernel parameters (they will change for each iteration)*/
    Matrix<size_t> local_size(1,1,work_group_size);
    Matrix<size_t> global_size(1,1);
    unsigned int read_offset = 0;
    unsigned int write_offset;

    global_size[0] = read_size + (read_size % local_size[0] ? local_size[0] - (read_size % local_size[0]) : 0);
    write_offset = global_size[0];

    bool forth = true;
    float sum;

    /* Pass arguments to kernel */
    err = clSetKernelArg(cl_parallel_reduce, 0, sizeof(cl_mem), (void *) &cl_data);
    err |= clSetKernelArg(cl_parallel_reduce, 1, local_size[0]*sizeof(cl_float), NULL);
    err |= clSetKernelArg(cl_parallel_reduce, 2, sizeof(cl_uint), &read_size);
    err |= clSetKernelArg(cl_parallel_reduce, 3, sizeof(cl_uint), &read_offset);
    err |= clSetKernelArg(cl_parallel_reduce, 4, sizeof(cl_uint), &write_offset);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    /* Launch kernel repeatedly until the summing is done */
    while (read_size > 1)
    {
        err = clEnqueueNDRangeKernel(*context_cl->getCommandQueue(), cl_parallel_reduce, 1, 0, global_size.data(), local_size.data(), 0, NULL, NULL);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        err = clFinish(*context_cl->getCommandQueue());
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        /* Extract the sum */
        err = clEnqueueReadBuffer ( *context_cl->getCommandQueue(),
            cl_data,
            CL_TRUE,
            forth ? global_size[0]*sizeof(cl_float) : 0,
            sizeof(cl_float),
            &sum,
            0, NULL, NULL);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

        /* Prepare the kernel parameters for the next iteration */
        forth = !forth;

        // Prepare to read memory in front of the separator and write to the memory behind it
        if (forth)
        {
            read_size = (global_size[0])/local_size[0];
            if (read_size % local_size[0]) global_size[0] = read_size + local_size[0] - (read_size % local_size[0]);
            else global_size[0] = read_size;

            read_offset = 0;
            write_offset = global_size[0];
        }
        // Prepare to read memory behind the separator and write to the memory in front of it
        else
        {
            read_offset = global_size[0];
            write_offset = 0;

            read_size = global_size[0]/local_size[0];
            if (read_size % local_size[0]) global_size[0] = read_size + local_size[0] - (read_size % local_size[0]);
            else global_size[0] = read_size;
        }

        err = clSetKernelArg(cl_parallel_reduce, 2, sizeof(cl_uint), &read_size);
        err |= clSetKernelArg(cl_parallel_reduce, 3, sizeof(cl_uint), &read_offset);
        err |= clSetKernelArg(cl_parallel_reduce, 4, sizeof(cl_uint), &write_offset);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    }

    return sum;
}

void VolumeRenderWorker::setHCurrent(int value)
{
    hklCurrent[0] = value;
    setHkl(hklCurrent);
}

void VolumeRenderWorker::setKCurrent(int value)
{
    hklCurrent[1] = value;
    setHkl(hklCurrent);
}

void VolumeRenderWorker::setLCurrent(int value)
{
    hklCurrent[2] = value;
    setHkl(hklCurrent);
}

void VolumeRenderWorker::setHkl(Matrix<int> & hkl)
{
    Matrix<double> hkl_focus(1,3,0);
    
    hkl_focus[0] = hkl[0]*UB[0] + hkl[1]*UB[1] + hkl[2]*UB[2];
    hkl_focus[1] = hkl[0]*UB[3] + hkl[1]*UB[4] + hkl[2]*UB[5];
    hkl_focus[2] = hkl[0]*UB[6] + hkl[1]*UB[7] + hkl[2]*UB[8];
    
    data_translation[3] = -hkl_focus[0];
    data_translation[7] = -hkl_focus[1];
    data_translation[11] = -hkl_focus[2];
    
    data_view_extent =  (data_scaling * data_translation).inverse4x4() * data_extent;
    
    updateUnitCellText();
}

void VolumeRenderWorker::metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(left_button);
    Q_UNUSED(mid_button);
    Q_UNUSED(right_button);
    Q_UNUSED(ctrl_button);
    Q_UNUSED(shift_button);
    
    if (isRulerActive && left_button)
    {
        ruler[0] = x;
        ruler[1] = y;
        ruler[2] = x;
        ruler[3] = y;
    }
    
    accumulated_roll = 0;
    
    GLfloat depth;
    glReadPixels(x, render_surface->height() - y - 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    
    if (( depth < 1.0) && (markers.size() > 0) && left_button && !right_button && !mid_button)
    {
        Matrix<double> xyz_clip(4,1,1.0);
        xyz_clip[0] = 2.0 * (double) x / (double) render_surface->width() - 1.0;
        xyz_clip[1] = 2.0 * (double) (render_surface->height() - y)/ (double) render_surface->height() - 1.0;
        xyz_clip[2] = 2.0 * depth - 1.0;
            
        Matrix<double> xyz = view_matrix.inverse4x4() * xyz_clip;
        
        xyz[0] /= xyz[3];
        xyz[1] /= xyz[3];
        xyz[2] /= xyz[3];
        
        int closest = 0;  
        double min_distance = 1e9;
            
        for (int i = 0; i < markers.size(); i++)
        {
            // Find the closest marker
            if (min_distance > markers[i].getDistance(xyz[0], xyz[1], xyz[2])) 
            {
                min_distance = markers[i].getDistance(xyz[0], xyz[1], xyz[2]); 
                closest = i;
            }
        }
        
        if (ctrl_button)
        {
            markers.remove(closest);
            glDeleteBuffers(1, &marker_vbo[closest]);
            marker_vbo.remove(closest);
        }
        else
        {
            markers[closest].setTagged(!markers[closest].getTagged());
        }  
        
        Matrix<float> marker_selected(10,3); 
        int n_markers_selected = 0;
        int iter = 0;
        n_marker_indices = 0;
        
        for (int i = 0; i < markers.size(); i++)
        {
            // Generate a vbo to draw lines between selected markers
            if (markers[i].getTagged())
            {
                marker_selected[n_markers_selected*3+0] = markers[i].getCenter()[0];
                marker_selected[n_markers_selected*3+1] = markers[i].getCenter()[1];
                marker_selected[n_markers_selected*3+2] = markers[i].getCenter()[2];
                
                for (int i = 0; i < n_markers_selected; i++)
                {
                    markers_selected_indices[iter*2+0] = n_markers_selected;
                    markers_selected_indices[iter*2+1] = i;
                    iter++;
                    n_marker_indices += 2;
                }
                
                n_markers_selected++;
            }
            
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, marker_centers_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT)*marker_selected.size(), marker_selected.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
    }
    
    
}

void VolumeRenderWorker::metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(left_button);
    Q_UNUSED(mid_button);
    Q_UNUSED(right_button);
    Q_UNUSED(ctrl_button);
    Q_UNUSED(shift_button);
}


//void VolumeRenderWorker::mouseMoveEvent(QMouseEvent ev)
//{
//    qDebug() << ev.x() << ev.y();
//}

void VolumeRenderWorker::metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button)
{
    if (left_button) isLMBDown = true;
    else isLMBDown = false;

    if (isLMBDown && isRulerActive)
    {
        ruler[2] = x;
        ruler[3] = y;
    }
    
    if ((std::abs(last_mouse_pos_x - x) < 50) && (std::abs(last_mouse_pos_y - y) < 50))
    {
        float move_scaling = 0.6;
        if(ctrl_button) move_scaling = 0.1;

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
            
            if (shift_button && isURotationActive && isUnitcellActive)
            {
                U = rotation.inverse4x4() * roll_rotation * rotation * U;
                UB.setUMatrix(U.to3x3());
                updateUnitCellText();
            }
            else if(shift_button) 
            {
                scalebar_rotation = rotation.inverse4x4() * roll_rotation * rotation * scalebar_rotation;
            }
            else
            {
                rotation = roll_rotation * rotation;
            }
        }
        else if (left_button && right_button && !mid_button && !isRulerActive)
        {
            /* Rotation happens multiplicatively around a rolling axis given
             * by the mouse move direction and magnitude.
             * Moving the mouse alters rotation.
             * */

            RotationMatrix<double> roll_rotation;
            double roll = move_scaling * pi/((float) render_surface->height()) * (y - last_mouse_pos_y);
            
            accumulated_roll += roll;
            
            roll_rotation.setArbRotation(0, 0, roll);

            if (shift_button && isURotationActive && isUnitcellActive)
            {
                U = rotation.inverse4x4() * roll_rotation *  rotation * U;
                UB.setUMatrix(U.to3x3());
                updateUnitCellText();
            }
            else if(shift_button) 
            {
                scalebar_rotation = rotation.inverse4x4() * roll_rotation *  rotation * scalebar_rotation;
            }
            else
            {
                rotation = roll_rotation * rotation;
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

            data_translation = ( rotation.inverse4x4() * data_translation * rotation) * data_translation_prev;

            this->data_view_extent =  (data_scaling * data_translation).inverse4x4() * data_extent;
            
            updateUnitCellText();
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

            data_translation = ( rotation.inverse4x4() * data_translation * rotation) * data_translation_prev;

            this->data_view_extent =  (data_scaling * data_translation).inverse4x4() * data_extent;
            
            updateUnitCellText();
        }

    }
    
    last_mouse_pos_x = x;
    last_mouse_pos_y = y;
}

void VolumeRenderWorker::setUB_a(double value)
{
    UB.setA(value);
    if (isInitialized)
    {
        updateUnitCellVertices();
        updateUnitCellText();
    }
}

void VolumeRenderWorker::setUB_b(double value)
{
    UB.setB(value);
    if (isInitialized)
    {
        updateUnitCellVertices();
        updateUnitCellText();
    }
}
void VolumeRenderWorker::setUB_c(double value)
{
    UB.setC(value);
    if (isInitialized)
    {
        updateUnitCellVertices();
        updateUnitCellText();
    }
}

void VolumeRenderWorker::setUB_alpha(double value)
{
    UB.setAlpha(value*pi/180.0);
    if (isInitialized)
    {
        updateUnitCellVertices();
        updateUnitCellText();
    }
}
void VolumeRenderWorker::setUB_beta(double value)
{
    UB.setBeta(value*pi/180.0);
    if (isInitialized)
    {
        updateUnitCellVertices();
        updateUnitCellText();
    }
}
void VolumeRenderWorker::setUB_gamma(double value)
{
    UB.setGamma(value*pi/180.0);
    if (isInitialized)
    {
        updateUnitCellVertices();
        updateUnitCellText();
    }
}


void VolumeRenderWorker::setUBMatrix(UBMatrix<double> & mat)
{
    this->UB = mat;
    this->U.setIdentity(4);
    U.setFrom3x3(UB.getUMatrix());
}

UBMatrix<double> & VolumeRenderWorker::getUBMatrix()
{
    return UB;
}

void VolumeRenderWorker::updateUnitCellText()
{
    /* Generates a unit cell grid based on an UB matrix. The grid consists of one set of reciprocal basis vectors per hkl */
    
    // Start by setting limits of the unitcell that we will visualize
    Matrix<int> hkl_limits(1,6);
    hkl_limits[0] = -10;
    hkl_limits[1] = 10;
    hkl_limits[2] = -10;
    hkl_limits[3] = 10;
    hkl_limits[4] = -10;
    hkl_limits[5] = 10;
    
    
    size_t i = 0;
    hkl_text_counter = 0;
    
    Matrix<double> B = UB; // Why B and not UB?
    
    for(int h = hkl_limits[0]; h < hkl_limits[1]; h++)
    {
        for(int k = hkl_limits[2]; k < hkl_limits[3]; k++)
        {
            for(int l = hkl_limits[4]; l < hkl_limits[5]; l++)
            {
                // Assign a coordinate to use for font rendering since we want to see the indices of the peaks. Only render those that lie within the view extent and only when this list is less than some number of items to avoid clutter.
                double x = h*B[0] + k*B[1] + l*B[2];
                double y = h*B[3] + k*B[4] + l*B[5];
                double z = h*B[6] + k*B[7] + l*B[8];
                
                if (i+5 < hkl_text.size())
                {
                    if (((x > data_view_extent[0]) && (x < data_view_extent[1])) && 
                        ((y > data_view_extent[2]) && (y < data_view_extent[3])) && 
                        ((z > data_view_extent[4]) && (z < data_view_extent[5])))
                    {
                        hkl_text[i+0] = h;
                        hkl_text[i+1] = k;
                        hkl_text[i+2] = l;
                        hkl_text[i+3] = x;
                        hkl_text[i+4] = y;
                        hkl_text[i+5] = z;
                        
                        i+=6;
                        
                        hkl_text_counter++;
                    }
                }
                else hkl_text_counter = 0;
            }    
        }
    }
}

void VolumeRenderWorker::updateUnitCellVertices()
{
    /* Generates a unit cell grid based on an UB matrix. The grid consists of one set of reciprocal basis vectors per hkl */
    
    // Start by setting limits of the unitcell that we will visualize
    Matrix<int> hkl_limits(1,6);
    hkl_limits[0] = -10;
    hkl_limits[1] = 10;
    hkl_limits[2] = -10;
    hkl_limits[3] = 10;
    hkl_limits[4] = -10;
    hkl_limits[5] = 10;
    
    int n_basis = ((hkl_limits[1] - hkl_limits[0] + 1)*(hkl_limits[3] - hkl_limits[2] + 1)*(hkl_limits[5] - hkl_limits[4] + 1));
    
    // Generate the positions ([number of bases] X [number of vertices per basis] X [number of float values per vertice]);
    Matrix<float> vertices(n_basis*6,3);
    
    size_t m = 0;
    hkl_text_counter = 0;

    Matrix<double> B = UB.getBMatrix(); // Why B and not UB?
    
    for(int h = hkl_limits[0]; h < hkl_limits[1]; h++)
    {
        for(int k = hkl_limits[2]; k < hkl_limits[3]; k++)
        {
            for(int l = hkl_limits[4]; l < hkl_limits[5]; l++)
            {
                // Assign 6 vertices, each with 4 float values;
                double x = h*B[0] + k*B[1] + l*B[2];
                double y = h*B[3] + k*B[4] + l*B[5];
                double z = h*B[6] + k*B[7] + l*B[8];
                
                vertices[m+0] = x;
                vertices[m+1] = y;
                vertices[m+2] = z;
                
                vertices[m+3] = (1+h)*B[0] + k*B[1] + l*B[2];
                vertices[m+4] = (1+h)*B[3] + k*B[4] + l*B[5];
                vertices[m+5] = (1+h)*B[6] + k*B[7] + l*B[8];
                
                vertices[m+6] = x;
                vertices[m+7] = y;
                vertices[m+8] = z;
                
                vertices[m+9] = h*B[0] + (1+k)*B[1] + l*B[2];
                vertices[m+10] = h*B[3] + (1+k)*B[4] + l*B[5];
                vertices[m+11] = h*B[6] + (1+k)*B[7] + l*B[8];
                
                vertices[m+12] = x;
                vertices[m+13] = y;
                vertices[m+14] = z;
                
                vertices[m+15] = h*B[0] + k*B[1] + (1+l)*B[2];
                vertices[m+16] = h*B[3] + k*B[4] + (1+l)*B[5];
                vertices[m+17] = h*B[6] + k*B[7] + (1+l)*B[8];
                
                m += 6*3;
            }    
        }
    }
    
    setVbo(unitcell_vbo, vertices.data(), vertices.size(), GL_STATIC_DRAW);
    unitcell_nodes = n_basis*6;
}

void VolumeRenderWorker::drawUnitCell(QPainter * painter)
{
    beginRawGLCalls(painter);
    
    glLineWidth(0.7);
    shared_window->unitcell_program->bind();
    glEnableVertexAttribArray(shared_window->unitcell_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, unitcell_vbo);
    glVertexAttribPointer(shared_window->unitcell_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUniformMatrix4fv(shared_window->unitcell_transform, 1, GL_FALSE, unitcell_view_matrix.colmajor().toFloat().data());
    
    glUniformMatrix4fv(shared_window->unitcell_u, 1, GL_FALSE, U.colmajor().toFloat().data());
    
    
    float alpha = pow((std::max(std::max(UB.cStar(), UB.bStar()), UB.cStar()) / (data_view_extent[1]-data_view_extent[0])) * 5.0, 2);
    
    if (alpha > 0.3) alpha = 0.3;
    
    Matrix<float> color = clear_color_inverse;
    
    color[3] = alpha;
    
    glUniform4fv(shared_window->unitcell_color, 1, color.data());
    
    Matrix<float> lim_low(1,3);
    lim_low[0] = data_view_extent[0];
    lim_low[1] = data_view_extent[2];
    lim_low[2] = data_view_extent[4];
    
    Matrix<float> lim_high(1,3);
    lim_high[0] = data_view_extent[1];
    lim_high[1] = data_view_extent[3];
    lim_high[2] = data_view_extent[5];
    
    glUniform3fv(shared_window->unitcell_lim_low, 1, lim_low.data());
    
    glUniform3fv(shared_window->unitcell_lim_high, 1, lim_high.data());
    
    glDrawArrays(GL_LINES,  0, unitcell_nodes);
    
    glDisableVertexAttribArray(shared_window->unitcell_fragpos);

    shared_window->unitcell_program->release();
    
    
    glLineWidth(1.0);
    
    endRawGLCalls(painter);
}

void VolumeRenderWorker::drawHelpCell(QPainter * painter)
{
    // Generate the vertices for the minicell
    Matrix<double> B = UB.getBMatrix();
    
    // Offset by half a diagonal to center the cell
    Matrix<float> hd(3,1); 
    hd[0] = (B[0] + B[1] + B[2])*0.5;
    hd[1] = (B[3] + B[4] + B[5])*0.5;       
    hd[2] = (B[6] + B[7] + B[8])*0.5;
    
    Matrix<float> vetices(8,3,0);
    
    vetices[3*0+0] = 0 - hd[0];
    vetices[3*0+1] = 0 - hd[1];
    vetices[3*0+2] = 0 - hd[2];
    
    vetices[3*1+0] = B[0] - hd[0];
    vetices[3*1+1] = B[3] - hd[1];
    vetices[3*1+2] = B[6] - hd[2];
    
    vetices[3*2+0] = B[1] - hd[0];
    vetices[3*2+1] = B[4] - hd[1];
    vetices[3*2+2] = B[7] - hd[2];
    
    vetices[3*3+0] = B[2] - hd[0];
    vetices[3*3+1] = B[5] - hd[1];
    vetices[3*3+2] = B[8] - hd[2];
    
    vetices[3*4+0] = B[0] + B[1] - hd[0];
    vetices[3*4+1] = B[3] + B[4] - hd[1];
    vetices[3*4+2] = B[6] + B[7] - hd[2];
    
    vetices[3*5+0] = B[1] + B[2] - hd[0];
    vetices[3*5+1] = B[4] + B[5] - hd[1];
    vetices[3*5+2] = B[7] + B[8] - hd[2];
    
    vetices[3*6+0] = B[0] + B[2] - hd[0];
    vetices[3*6+1] = B[3] + B[5] - hd[1];
    vetices[3*6+2] = B[6] + B[8] - hd[2];
    
    vetices[3*7+0] = B[0] + B[1] + B[2] - hd[0];
    vetices[3*7+1] = B[3] + B[4] + B[5] - hd[1];
    vetices[3*7+2] = B[6] + B[7] + B[8] - hd[2];
    
    
    // Scaling. The cell has four diagonals. We find the longest one  and scale the cell to it
    Matrix<float> diagonal_one(1,3);
    diagonal_one[0] = vetices[3*0+0] - vetices[3*7+0];
    diagonal_one[1] = vetices[3*0+1] - vetices[3*7+1];
    diagonal_one[2] = vetices[3*0+1] - vetices[3*7+2];
    
    Matrix<float> diagonal_two(1,3);
    diagonal_two[0] = vetices[3*1+0] - vetices[3*5+0];
    diagonal_two[1] = vetices[3*1+1] - vetices[3*5+1];
    diagonal_two[2] = vetices[3*1+2] - vetices[3*5+2];

    Matrix<float> diagonal_three(1,3);
    diagonal_three[0] = vetices[3*2+0] - vetices[3*6+0];
    diagonal_three[1] = vetices[3*2+1] - vetices[3*6+1];
    diagonal_three[2] = vetices[3*2+2] - vetices[3*6+2];
    
    Matrix<float> diagonal_four(1,3);
    diagonal_four[0] = vetices[3*3+0] - vetices[3*4+0];
    diagonal_four[1] = vetices[3*3+1] - vetices[3*4+1];
    diagonal_four[2] = vetices[3*3+2] - vetices[3*4+2];
    
    double scale_factor = 1.5/std::max(vecLength(diagonal_one),std::max(vecLength(diagonal_two),std::max(vecLength(diagonal_three),vecLength(diagonal_four))));
    
    minicell_scaling[0] = scale_factor;
    minicell_scaling[5] = scale_factor;
    minicell_scaling[10] = scale_factor;
    
    // Minicell backdrop
    QRect minicell_rect(0,0,200,200);

    painter->setPen(*normal_pen);
    painter->setBrush(*fill_brush);
    painter->drawRoundedRect(minicell_rect, 5, 5, Qt::AbsoluteSize);
    
    beginRawGLCalls(painter);
    
    setVbo(minicell_vbo, vetices.data(), vetices.size(), GL_STATIC_DRAW);
    
    
    // Generate indices for glDrawElements
    Matrix<GLuint> a_indices(1,2,0);
    a_indices[1] = 1;
    Matrix<GLuint> b_indices(1,2,0);
    b_indices[1] = 2;
    Matrix<GLuint> c_indices(1,2,0);
    c_indices[1] = 3;
    Matrix<GLuint> wire;
    unsigned int wire_buf[] = {1,4, 2,4, 1,6, 4,7, 2,5, 3,6, 3,5, 5,7, 6,7};
    wire.setDeep(1,18,wire_buf);
    
    // Draw it
    glViewport(0,render_surface->height()-200,200,200);
    glLineWidth(3.0);
    
    shared_window->std_3d_col_program->bind();
    
    glEnableVertexAttribArray(shared_window->std_3d_col_fragpos);

    glUniformMatrix4fv(shared_window->std_3d_col_transform, 1, GL_FALSE, minicell_view_matrix.colmajor().toFloat().data());
    
    glBindBuffer(GL_ARRAY_BUFFER, minicell_vbo);
    glVertexAttribPointer(shared_window->std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glUniform4fv(shared_window->std_3d_col_color, 1, red.data());
    glDrawElements(GL_LINES,  2, GL_UNSIGNED_INT, a_indices.data());
    glUniform4fv(shared_window->std_3d_col_color, 1, green.data());
    glDrawElements(GL_LINES,  2, GL_UNSIGNED_INT, b_indices.data());
    glUniform4fv(shared_window->std_3d_col_color, 1, blue.data());
    glDrawElements(GL_LINES,  2, GL_UNSIGNED_INT, c_indices.data());
    glUniform4fv(shared_window->std_3d_col_color, 1, clear_color_inverse.data());
    glLineWidth(1.5);
    glDrawElements(GL_LINES,  18, GL_UNSIGNED_INT, wire.data());
    
    glDisableVertexAttribArray(shared_window->std_3d_col_fragpos);
    
    glLineWidth(1.0);
    
    shared_window->std_3d_col_program->release();
    
    glViewport(0,0,render_surface->width(),render_surface->height());
    
    
    endRawGLCalls(painter);
    
    // Minicell text
    Matrix<float> x_2d(1,2,0), y_2d(1,2,0), z_2d(1,2,0);
    getPosition2D(x_2d.data(), vetices.data()+3, &minicell_view_matrix);
    getPosition2D(y_2d.data(), vetices.data()+6, &minicell_view_matrix);
    getPosition2D(z_2d.data(), vetices.data()+9, &minicell_view_matrix);
    
    painter->setFont(*minicell_font);
    
    painter->drawText(QPointF((x_2d[0]+ 1.0) * 0.5 *200, (1.0 - ( x_2d[1]+ 1.0) * 0.5) *200), QString("a*"));
    painter->drawText(QPointF((y_2d[0]+ 1.0) * 0.5 *200, (1.0 - ( y_2d[1]+ 1.0) * 0.5) *200), QString("b*"));
    painter->drawText(QPointF((z_2d[0]+ 1.0) * 0.5 *200, (1.0 - ( z_2d[1]+ 1.0) * 0.5) *200), QString("c*"));
    
    painter->setFont(*normal_font);
}


void VolumeRenderWorker::drawMarkers(QPainter * painter)
{
    beginRawGLCalls(painter);
    
    glLineWidth(3.0);
    
    shared_window->std_3d_col_program->bind();
    
    glEnableVertexAttribArray(shared_window->std_3d_col_fragpos);

    glUniformMatrix4fv(shared_window->std_3d_col_transform, 1, GL_FALSE, view_matrix.colmajor().toFloat().data());
    
    for (int i = 0; i < markers.size(); i++)
    {
        glUniform4fv(shared_window->std_3d_col_color, 1, markers[i].getColor());
        
        glBindBuffer(GL_ARRAY_BUFFER, marker_vbo[i]);
        glVertexAttribPointer(shared_window->std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        glEnable(GL_DEPTH_TEST);
        glDrawArrays(GL_LINES,  0, 6);
        glDisable(GL_DEPTH_TEST);
    }
    glLineWidth(1.5);
    
    glUniform4fv(shared_window->std_3d_col_color, 1, marker_line_color.data());
    
    glBindBuffer(GL_ARRAY_BUFFER, marker_centers_vbo);
    glVertexAttribPointer(shared_window->std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glDrawElements(GL_LINES,  n_marker_indices, GL_UNSIGNED_INT, markers_selected_indices.data());
    
    glDisableVertexAttribArray(shared_window->std_3d_col_fragpos);

    shared_window->std_3d_col_program->release();
    
    glLineWidth(1.0);
    
    endRawGLCalls(painter);

    qDebug() << n_marker_indices;
}


void VolumeRenderWorker::drawCountIntegral(QPainter * painter)
{
    float sum = sumViewBox();

    QString sum_string("Integrated intensity: "+QString::number(sum, 'e',3)+" 1/Å^3");
    QRect sum_string_rect = emph_fontmetric->boundingRect(sum_string);
    sum_string_rect += QMargins(5,5,5,5);
    sum_string_rect.moveTopLeft(QPoint(5,205));

    painter->setBrush(*fill_brush);
    painter->drawRoundedRect(sum_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(sum_string_rect, Qt::AlignCenter, sum_string);
}

void VolumeRenderWorker::drawHklText(QPainter * painter)
{
    painter->setFont(*tiny_font);
    
    for (size_t i = 0; i < hkl_text_counter; i++)
    {
        Matrix<double> pos2d(1,2);
        
        getPosition2D(pos2d.data(), hkl_text.data()+i*6+3, &view_matrix);
        
        QString text = "("+QString::number((int) hkl_text[i*6+0])+","+QString::number((int) hkl_text[i*6+1])+","+QString::number((int) hkl_text[i*6+2])+")";
        
        painter->drawText(QPointF((pos2d[0]+ 1.0) * 0.5 *render_surface->width(), (1.0 - ( pos2d[1]+ 1.0) * 0.5) *render_surface->height()), text);
    }
    
    painter->setPen(*normal_pen);
    painter->setFont(*normal_font);    
}

void VolumeRenderWorker::setCenterLine()
{
    centerline_coords[3] = data_view_extent[0] + (data_view_extent[1] - data_view_extent[0])*0.5;
    centerline_coords[4] = data_view_extent[2] + (data_view_extent[3] - data_view_extent[2])*0.5;
    centerline_coords[5] = data_view_extent[4] + (data_view_extent[5] - data_view_extent[4])*0.5;

    // Center line
    setVbo(centerline_vbo, centerline_coords.data(), 6, GL_STATIC_DRAW);
}


void VolumeRenderWorker::drawCenterLine(QPainter * painter)
{
    beginRawGLCalls(painter);
    
    setCenterLine();

    shared_window->std_3d_col_program->bind();
    glEnableVertexAttribArray(shared_window->std_3d_col_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, centerline_vbo);
    glVertexAttribPointer(shared_window->std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUniformMatrix4fv(shared_window->std_3d_col_transform, 1, GL_FALSE, view_matrix.colmajor().toFloat().data());


    glUniform4fv(shared_window->std_3d_col_color, 1, clear_color_inverse.data());

    glDrawArrays(GL_LINES,  0, 2);

    glDisableVertexAttribArray(shared_window->std_3d_col_fragpos);

    shared_window->std_3d_col_program->release();
    
    endRawGLCalls(painter);
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
    
    
    shared_window->std_3d_col_program->bind();
    glEnableVertexAttribArray(shared_window->std_3d_col_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, point_vbo);
    glVertexAttribPointer(shared_window->std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0); 
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
    
    
    glUniform4fv(shared_window->std_3d_col_color, 1, clear_color_inverse.data());
    
    glPointSize(5);
    
    
    glUniformMatrix4fv(shared_window->std_3d_col_transform, 1, GL_FALSE, point_around_axis.colmajor().toFloat().data());
    glDrawArrays(GL_POINTS,  2, 98);
    
    
    glUniformMatrix4fv(shared_window->std_3d_col_transform, 1, GL_FALSE, point_on_axis.colmajor().toFloat().data());
    glDrawArrays(GL_LINE_STRIP,  0, 2);
    
    glDisableVertexAttribArray(shared_window->std_3d_col_fragpos);

    shared_window->std_3d_col_program->release();
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

                data_view_extent =  (data_scaling * data_translation).inverse4x4() * data_extent;
                
                updateUnitCellText();
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
    setRayTexture(20);
    setTsfTexture();

    // Core set functions
    setDataExtent();
    setViewMatrices();
    setTsfParameters();
    setMiscArrays();

    // Pens
    initializePaintTools();
    
    // Initial drawings
    updateUnitCellVertices();
    updateUnitCellText();
}

void VolumeRenderWorker::initializePaintTools()
{
    normal_pen = new QPen;
    normal_pen->setWidthF(1.0);
    border_pen = new QPen;
    border_pen->setWidth(1);
    
    minicell_font = new QFont;
    minicell_font->setBold(true);
    minicell_font->setItalic(true);
    
    
    
    whatever_pen = new QPen;

    normal_font = new QFont();
    tiny_font = new QFont;
    tiny_font->setPixelSize(12);
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
    glGenBuffers(1, &lab_frame_vbo);
    glGenBuffers(1, &scalebar_vbo);
    glGenBuffers(1, &count_scalebar_vbo);
    glGenBuffers(1, &centerline_vbo);
    glGenBuffers(1, &point_vbo);
    glGenBuffers(1, &unitcell_vbo);
    glGenBuffers(1, &marker_centers_vbo);
    glGenBuffers(1, &minicell_vbo);
}

void VolumeRenderWorker::initResourcesCL()
{
    // Build program from OpenCL kernel source
    QStringList paths;
    paths << "kernels/models.cl";
    paths << "kernels/render_shared.cl";
    paths << "kernels/render_svo.cl";
    paths << "kernels/render_model.cl";
    paths << "kernels/integrate.cl";
    paths << "kernels/box_sampler.cl";
    paths << "kernels/parallel_reduction.cl";

    program = context_cl->createProgram(paths, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    context_cl->buildProgram(&program, "-Werror");


    // Kernel handles
    cl_svo_raytrace = clCreateKernel(program, "svoRayTrace", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_model_raytrace = clCreateKernel(program, "modelRayTrace", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_integrate_image = clCreateKernel(program, "integrateImage", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_box_sampler = clCreateKernel(program, "boxSample", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_parallel_reduce = clCreateKernel(program, "psum", &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));


    // Buffers
    cl_view_matrix_inverse = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        view_matrix.toFloat().bytes(),
        NULL, &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    cl_scalebar_rotation = clCreateBuffer(*context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        (rotation*scalebar_rotation).toFloat().bytes(),
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
    normalization_scaling[0] = bbox_scaling[0] * projection_scaling[0] *2.0 / (data_extent[1] - data_extent[0]);
    normalization_scaling[5] = bbox_scaling[5] * projection_scaling[5] * 2.0 / (data_extent[3] - data_extent[2]);
    normalization_scaling[10] = bbox_scaling[10] * projection_scaling[10] * 2.0 / (data_extent[5] - data_extent[4]);

    view_matrix = ctc_matrix * bbox_translation * normalization_scaling * data_scaling * rotation * data_translation;
    scalebar_view_matrix = ctc_matrix * bbox_translation * normalization_scaling * data_scaling * rotation * scalebar_rotation * data_translation;
    unitcell_view_matrix = ctc_matrix * bbox_translation * normalization_scaling * data_scaling * rotation * data_translation * U;
    ctc_matrix.setWindow(200, 200);
    minicell_view_matrix = ctc_matrix * bbox_translation * minicell_scaling * rotation * U;
    ctc_matrix.setWindow(render_surface->width(),render_surface->height());
    
    err = clEnqueueWriteBuffer (*context_cl->getCommandQueue(),
        cl_view_matrix_inverse,
        CL_TRUE,
        0,
        view_matrix.bytes()/2,
        view_matrix.inverse4x4().toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clEnqueueWriteBuffer (*context_cl->getCommandQueue(),
        cl_scalebar_rotation,
        CL_TRUE,
        0,
        scalebar_rotation.bytes()/2,
        scalebar_rotation.toFloat().data(),
        0,0,0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clSetKernelArg(cl_model_raytrace, 3, sizeof(cl_mem), (void *) &cl_view_matrix_inverse);
    err |= clSetKernelArg(cl_model_raytrace, 9, sizeof(cl_mem), (void *) &cl_scalebar_rotation);

    err |= clSetKernelArg(cl_svo_raytrace, 7, sizeof(cl_mem), (void *) &cl_view_matrix_inverse);
    err |= clSetKernelArg(cl_svo_raytrace, 12, sizeof(cl_mem), (void *) &cl_scalebar_rotation);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
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
//    model_misc_floats.print(2,"misc floats");
    
    
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

    setRayTexture(quality_percentage);
}

void VolumeRenderWorker::setRayTexture(int percentage)
{
    if (isInitialized)
    {
        // Set a texture for the volume rendering kernel
        Matrix<int> ray_tex_new(1, 2);

        ray_tex_new[0] = (int)(sqrt((double) percentage * 0.01) * (float)render_surface->width());
        ray_tex_new[1] = (int)(sqrt((double) percentage * 0.01) * (float)render_surface->height());

        // Clamp
        if (ray_tex_new[0] < 16) ray_tex_new[0] = 16;
        if (ray_tex_new[1] < 16) ray_tex_new[1] = 16;

        if (ray_tex_new[0] > render_surface->width()) ray_tex_new[0] = render_surface->width();
        if (ray_tex_new[1] > render_surface->height()) ray_tex_new[1] = render_surface->height();

        // Calculate the actual quality factor multiplier
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
//    if (1) qDebug() << "setting tsf" << tsf_color_scheme << tsf_alpha_scheme;
    
    if (isTsfTexInitialized){
        err = clReleaseSampler(tsf_tex_sampler);
        if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
//    }
//    if (isTsfTexInitialized){
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
        tsf.getSplined()->n(),
        1,
        0,
        GL_RGBA,
        GL_FLOAT,
        tsf.getSplined()->colmajor().toFloat().data());
    glBindTexture(GL_TEXTURE_2D, 0);

    // Buffer for tsf_tex_gl_thumb
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
        tsf.getThumb()->n(),
        1,
        0,
        GL_RGB,
        GL_FLOAT,
        tsf.getThumb()->colmajor().toFloat().data());
    glBindTexture(GL_TEXTURE_2D, 0);
    
//    tsf.getThumb()->colmajor().print(2);
    
    // Buffer for tsf_tex_cl
    cl_image_format tsf_format;
    tsf_format.image_channel_order = CL_RGBA;
    tsf_format.image_channel_data_type = CL_FLOAT;

    tsf_tex_cl = clCreateImage2D ( *context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        &tsf_format,
        tsf.getSplined()->n(),
        1,
        0,
        tsf.getSplined()->colmajor().toFloat().data(),
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

float VolumeRenderWorker::sumViewBox()
{
    /* Sample the viewing box and put the values in an array. pr = parallel reduction */
    int box_samples_per_side = 256; // Power of two. This dictates the integration resolution
    int pr_read_size = box_samples_per_side*box_samples_per_side*box_samples_per_side;
    int pr_local_size = 64; // Power of two. Has constraints depending on GPU
    int pr_global_size = pr_read_size + (pr_read_size % pr_local_size ? pr_local_size - (pr_read_size % pr_local_size) : 0);
    int pr_padded_size = pr_global_size + pr_global_size/pr_local_size;

    /* Prepare array */
    cl_mem  cl_data_array = clCreateBuffer(*context_cl->getContext(),
                                 CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                                 pr_padded_size*sizeof(cl_float),
                                 NULL,
                                 &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    /* Prepare and launch the kernel */
    Matrix<size_t> box_global_size(1,3,box_samples_per_side);
    Matrix<size_t> box_local_size(1,3,4);

    unsigned int n_tree_levels = misc_ints[0];
    unsigned int brick_dim = misc_ints[1];

    err = clSetKernelArg(cl_box_sampler, 0, sizeof(cl_mem), (void *) &cl_svo_pool);
    err |= clSetKernelArg(cl_box_sampler, 1, sizeof(cl_mem), (void *) &cl_svo_index);
    err |= clSetKernelArg(cl_box_sampler, 2, sizeof(cl_mem), (void *) &cl_svo_brick);
    err |= clSetKernelArg(cl_box_sampler, 3, sizeof(cl_mem), (void *) &cl_data_extent);
    err |= clSetKernelArg(cl_box_sampler, 4, sizeof(cl_mem), (void *) &cl_data_view_extent);
    err |= clSetKernelArg(cl_box_sampler, 5, sizeof(cl_sampler), &cl_svo_pool_sampler);
    err |= clSetKernelArg(cl_box_sampler, 6, sizeof(cl_uint), &n_tree_levels);
    err |= clSetKernelArg(cl_box_sampler, 7, sizeof(cl_uint), &brick_dim);
    err |= clSetKernelArg(cl_box_sampler, 8, sizeof(cl_mem), (void *) &cl_data_array);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clEnqueueNDRangeKernel(*context_cl->getCommandQueue(), cl_box_sampler, 3, 0, box_global_size.data(), box_local_size.data(), 0, NULL, NULL);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clFinish(*context_cl->getCommandQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    float sum = sumGpuArray(cl_data_array, pr_read_size, pr_local_size);

    /* Clean up */
    err = clReleaseMemObject(cl_data_array);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    return sum;
}

void VolumeRenderWorker::setSharedWindow(SharedContextWindow * window)
{
    this->shared_window = window;
}

void VolumeRenderWorker::render(QPainter *painter)
{
//    isRendering = true;
    isDataExtentReadOnly = true;
    setDataExtent();
    setViewMatrices();
    
    beginRawGLCalls(painter);
    glClearColor(clear_color[0], clear_color[1], clear_color[2], 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glLineWidth(1.0);
    const qreal retinaScale = render_surface->devicePixelRatio();
    glViewport(0, 0, render_surface->width() * retinaScale, render_surface->height() * retinaScale);
    endRawGLCalls(painter);
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    if (isScalebarActive) drawPositionScalebars(painter);
    if (isLabFrameActive) drawCenterLine(painter);
    if (isUnitcellActive) drawUnitCell(painter);
    if (isLabFrameActive) drawLabFrame(painter);
    
    isDataExtentReadOnly = false;

    // Draw raytracing texture
    drawRayTex(painter);
    
    // Compute the projected pixel size in orthonormal configuration
    computePixelSize();

    if (isIntegration2DActive) drawIntegral(painter);
    drawCountScalebar(painter);
    
    drawOverlay(painter);
    if (isUnitcellActive) drawHklText(painter);
    if (n_marker_indices > 0) drawMarkers(painter);
    if (isMiniCellActive) drawHelpCell(painter);
    if (isSvoInitialized && isCountIntegrationActive) drawCountIntegral(painter);
    
//    isRendering = false;
}


void VolumeRenderWorker::takeScreenShot(QString path)
{
    // Set resolution back to former value
    setRayTexture(100);
    
    QOpenGLFramebufferObjectFormat format;

    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setMipmap(true);
    format.setSamples(64);
    format.setTextureTarget(GL_TEXTURE_2D);
    format.setInternalTextureFormat(GL_RGBA32F);

    QOpenGLFramebufferObject buffy(render_surface->width(), render_surface->height(), format);

    buffy.bind();
    
    // Render into buffer using max quality
    QPainter painter(paint_device_gl);

    render(&painter);

    buffy.release();
    
    // Save buffer as image
    buffy.toImage().save(path);
    
    // Set resolution back to former value
    setRayTexture(quality_percentage);
}


void VolumeRenderWorker::drawIntegral(QPainter *painter)
{
    // Sum the rows and columns of the integrated texture (which resides as a pure OpenCL image buffer)

    // __ROWS__

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
    for (size_t i = 0; i < output.m(); i++)
    {
        for(size_t j = 0; j < output.n(); j++)
        {
            row_sum[i] += output[i*output.n() + j];
        }

        if (row_sum[i] > max) max = row_sum[i];
        if ((row_sum[i] < min) && (row_sum[i] > 0)) min = row_sum[i];

        sum += row_sum[i];
    }

    if (sum > 0)
    {
        for (size_t i = 0; i < row_sum.m(); i++)
        {
            row_sum[i] *= 100000.0/sum;
        }
        max *=  100000.0/sum;
        min *=  100000.0/sum;

        if (isLogarithmic2D)
        {
            for (size_t i = 0; i < row_sum.m(); i++)
            {
                if (row_sum[i] <= 0) row_sum[i] = min;
                row_sum[i] = log10(row_sum[i]);
            }
            max =  log10(max);
            min =  log10(min);
        }

        QPolygonF row_polygon;
        row_polygon << QPointF(0,0);
        for (size_t i = 0; i < row_sum.m(); i++)
        {
            QPointF point_top, point_bottom;

            float value = ((row_sum[row_sum.m() - i - 1] - min) / (max - min))*render_surface->width()/10.0;

            point_top.setX(value);
            point_top.setY(((float) i / (float) row_sum.m())*render_surface->height());

            point_bottom.setX(value);
            point_bottom.setY((((float) i + 1) / (float) row_sum.m())*render_surface->height());
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
    Matrix<float> column_sum(1, ray_tex_dim[0], 0.0f);
    max = 0;
    min = 1e9;
    sum = 0;

    for (size_t i = 0; i < output2.n(); i++)
    {
        for(size_t j = 0; j < output2.m(); j++)
        {
            column_sum[i] += output2[j*output2.n() + i];
        }

        if (column_sum[i] > max) max = column_sum[i];
        if ((column_sum[i] < min) && (column_sum[i] > 0)) min = column_sum[i];

        sum += column_sum[i];
    }

    if (sum > 0)
    {
        for (size_t i = 0; i < column_sum.n(); i++)
        {
            column_sum[i] *= 100000.0/sum;
        }
        max *=  100000.0/sum;
        min *=  100000.0/sum;

        if (isLogarithmic2D)
        {
            for (size_t i = 0; i < column_sum.n(); i++)
            {
                if (column_sum[i] <= 0) column_sum[i] = min;
                column_sum[i] = log10(column_sum[i]);
            }
            max =  log10(max);
            min =  log10(min);
        }

        QPolygonF column_polygon;
        column_polygon << QPointF(0,render_surface->height());
        for (size_t i = 0; i < column_sum.n(); i++)
        {
            QPointF point_top, point_bottom;

            float value = render_surface->height() - (column_sum[i] - min) / (max - min)*render_surface->height()*0.1;

            point_top.setX(((float) i / (float) column_sum.n())*render_surface->width());
            point_top.setY(value);

            point_bottom.setX((((float) i + 1) / (float) column_sum.n())*render_surface->width());
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

void VolumeRenderWorker::setMiniCell()
{
    isMiniCellActive = !isMiniCellActive;
}

void VolumeRenderWorker::setOrthoGrid()
{
    isOrthoGridActive = !isOrthoGridActive;
}

void VolumeRenderWorker::drawRuler(QPainter * painter)
{
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
    ruler.print(2,"Ruler");
    double length = sqrt((ruler[2]-ruler[0])*(ruler[2]-ruler[0])*pixel_size[0]*pixel_size[0] + (ruler[3]-ruler[1])*(ruler[3]-ruler[1])*pixel_size[1]*pixel_size[1]);
    
    QString centerline_string(QString::number(length, 'g', 5)+" 1/Å ("+QString::number(1.0/length, 'g', 5)+"Å)");
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
                whatever_pen->setWidthF(1.0);
                whatever_pen->setStyle(Qt::SolidLine);
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

void VolumeRenderWorker::alignAStartoZ()
{
    Matrix<double> vec(4,1,1);
    vec[0] = UB[0];
    vec[1] = UB[3];
    vec[2] = UB[6];
    
    RotationMatrix<double> zeta_rotation;
    zeta_rotation.setYRotation(-zeta(vec));
    
    RotationMatrix<double> eta_rotation;
    eta_rotation.setXRotation(-eta(vec));
    
    rotation = eta_rotation * zeta_rotation;
    
    setViewMatrices();
    
    view_matrix.inverse4x4(1);
}
void VolumeRenderWorker::alignBStartoZ()
{
    Matrix<double> vec(4,1,1);
    vec[0] = UB[1];
    vec[1] = UB[4];
    vec[2] = UB[7];
    
    RotationMatrix<double> zeta_rotation;
    zeta_rotation.setYRotation(-zeta(vec));
    
    RotationMatrix<double> eta_rotation;
    eta_rotation.setXRotation(-eta(vec));
    
    rotation = eta_rotation * zeta_rotation;
    
    setViewMatrices();
}
void VolumeRenderWorker::alignCStartoZ()
{
    Matrix<double> vec(4,1,1);
    vec[0] = UB[2];
    vec[1] = UB[5];
    vec[2] = UB[8];
    
    RotationMatrix<double> zeta_rotation;
    zeta_rotation.setYRotation(-zeta(vec));
    
    RotationMatrix<double> eta_rotation;
    eta_rotation.setXRotation(-eta(vec));
    
    rotation = eta_rotation * zeta_rotation;

    setViewMatrices();
}


void VolumeRenderWorker::alignLabXtoSliceX()
{
    RotationMatrix<double> x_aligned;
    x_aligned.setYRotation(pi*0.5);
    
    rotation =  x_aligned * scalebar_rotation.inverse4x4();
}

void VolumeRenderWorker::alignLabYtoSliceY()
{
    RotationMatrix<double> y_aligned;
    y_aligned.setXRotation(-pi*0.5);
    
    rotation =  y_aligned * scalebar_rotation.inverse4x4();
}
void VolumeRenderWorker::alignLabZtoSliceZ()
{
    RotationMatrix<double> z_aligned;
    z_aligned.setYRotation(0.0);
    
    rotation =  z_aligned * scalebar_rotation.inverse4x4();
}

void VolumeRenderWorker::alignSliceToLab()
{
    scalebar_rotation.setIdentity(4);
}

void VolumeRenderWorker::rotateLeft()
{
    RotationMatrix<double> rot;
    rot.setYRotation(pi*0.25);
    
    rotation = rot * rotation;
}
void VolumeRenderWorker::rotateRight()
{
    RotationMatrix<double> rot;
    rot.setYRotation(-pi*0.25);
    
    rotation = rot * rotation;
}
void VolumeRenderWorker::rotateUp()
{
    RotationMatrix<double> rot;
    rot.setXRotation(pi*0.25);
    
    rotation = rot * rotation;
}
void VolumeRenderWorker::rotateDown()
{
    RotationMatrix<double> rot;
    rot.setXRotation(-pi*0.25);
    
    rotation = rot * rotation;
}

void VolumeRenderWorker::rollCW()
{
    RotationMatrix<double> rot;
    rot.setZRotation(pi*0.25);
    
    rotation = rot * rotation;
}
void VolumeRenderWorker::rollCCW()
{
    RotationMatrix<double> rot;
    rot.setZRotation(-pi*0.25);
    
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
    
    // The height of a single pixel
    Matrix<double> ndc01(4,1);
    ndc01[0] = 0.0;
    ndc01[1] = 2.0/(double)render_surface->height();
    ndc01[2] = -1.0;
    ndc01[3] = 1.0;
    
    // The width of a single pixel
    Matrix<double> ndc10(4,1);
    ndc10[0] = 2.0/(double)render_surface->width();
    ndc10[1] = 0.0;
    ndc10[2] = -1.0;
    ndc10[3] = 1.0;

    Matrix<double> xyz_00(4,1);
    Matrix<double> xyz_01(4,1);
    Matrix<double> xyz_10(4,1);

    xyz_00 = view_matrix.inverse4x4()*ndc00;
    xyz_01 = view_matrix.inverse4x4()*ndc01;
    xyz_10 = view_matrix.inverse4x4()*ndc10;

    Matrix<double> w_vec = xyz_00 - xyz_10;
    Matrix<double> h_vec = xyz_00 - xyz_01;

    pixel_size[0] = std::sqrt(w_vec[0]*w_vec[0] + w_vec[1]*w_vec[1] + w_vec[2]*w_vec[2]);
    pixel_size[1] = std::sqrt(h_vec[0]*h_vec[0] + h_vec[1]*h_vec[1] + h_vec[2]*h_vec[2]);
}

void VolumeRenderWorker::drawOverlay(QPainter * painter)
{
    painter->setPen(*normal_pen);
    
    // Position scalebar tick labels
    if (isScalebarActive)
    {
        for (size_t i = 0; i < n_position_scalebar_ticks; i++)
        {
            painter->drawText(QPointF(position_scalebar_ticks[i*3+0], position_scalebar_ticks[i*3+1]), QString::number(position_scalebar_ticks[i*3+2]));
        }
    }
    
    // Count scalebar tick labels
    if (n_count_scalebar_ticks >= 2)
    {
        for (size_t i = 0; i < n_count_scalebar_ticks; i++)
        {
            painter->drawText(QPointF(count_scalebar_ticks[i*3+0], count_scalebar_ticks[i*3+1]), QString::number(count_scalebar_ticks[i*3+2], 'g', 4));
        }
    }
    else
    {
        for (size_t i = 0; i < n_count_minor_scalebar_ticks; i++)
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

    // Fps
    QString fps_string("Fps: "+QString::number(getFps(), 'f', 0));
    fps_string_rect = emph_fontmetric->boundingRect(fps_string);
    fps_string_rect.setWidth(std::max(fps_string_width_prev, fps_string_rect.width()));
    fps_string_width_prev = fps_string_rect.width();
    fps_string_rect += QMargins(5,5,5,5);
    fps_string_rect.moveTopRight(QPoint(render_surface->width()-5,5));

    painter->setBrush(*fill_brush);
    painter->drawRoundedRect(fps_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(fps_string_rect, Qt::AlignCenter, fps_string);

    // Texture resolution
    QString resolution_string("Texture resolution: "+QString::number(100.0*(ray_tex_dim[0]*ray_tex_dim[1])/(render_surface->width()*render_surface->height()), 'f', 1)+"%");
    QRect resolution_string_rect = emph_fontmetric->boundingRect(resolution_string);
    resolution_string_rect += QMargins(5,5,5,5);
    resolution_string_rect.moveBottomLeft(QPoint(5, render_surface->height() - 5));

    painter->drawRoundedRect(resolution_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(resolution_string_rect, Qt::AlignCenter, resolution_string);
    
    // Draw accumulated roll for a mouse move event
    /*QString roll_string("Roll: "+QString::number(accumulated_roll*180/pi, 'g', 5)+" deg");
    QRect roll_string_rect = emph_fontmetric->boundingRect(roll_string);
    roll_string_rect += QMargins(5,5,5,5);
    roll_string_rect.moveBottomLeft(QPoint(5,render_surface->height() -10 -resolution_string_rect.height()));

    painter->setBrush(*fill_brush);
    painter->drawRoundedRect(roll_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(roll_string_rect, Qt::AlignCenter, roll_string);*/
    
    // Scalebar multiplier
    /*QString multiplier_string("x"+QString::number(scalebar_multiplier)+" 1/Å");
    multiplier_string_rect = emph_fontmetric->boundingRect(multiplier_string);
    multiplier_string_rect += QMargins(5,5,5,5);
    multiplier_string_rect.moveTopRight(QPoint(render_surface->width()-5, fps_string_rect.bottom() + 5));

    painter->drawRoundedRect(multiplier_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(multiplier_string_rect, Qt::AlignCenter, multiplier_string);*/
}

void VolumeRenderWorker::drawPositionScalebars(QPainter * painter)
{
    beginRawGLCalls(painter);
    
    scalebar_coord_count = setScaleBars();

    shared_window->std_3d_col_program->bind();
    glEnableVertexAttribArray(shared_window->std_3d_col_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, scalebar_vbo);
    glVertexAttribPointer(shared_window->std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    if (isBackgroundBlack) glUniform4fv(shared_window->std_3d_col_color, 1, magenta_light.data());
    else glUniform4fv(shared_window->std_3d_col_color, 1, blue_light.data());
    
    glUniformMatrix4fv(shared_window->std_3d_col_transform, 1, GL_FALSE, scalebar_view_matrix.colmajor().toFloat().data());
    
    glDrawArrays(GL_LINES,  0, scalebar_coord_count);
    
    glDisableVertexAttribArray(shared_window->std_3d_col_fragpos);

    shared_window->std_3d_col_program->release();
    
    endRawGLCalls(painter);
}


void VolumeRenderWorker::drawLabFrame(QPainter *painter)
{
    beginRawGLCalls(painter);
    
    // Generate the vertices
    Matrix<GLfloat> vertices(4,3,0);
    
    vertices[1*3+0] = (data_view_extent[1] - data_view_extent[0])*0.5;
    vertices[2*3+1] = (data_view_extent[1] - data_view_extent[0])*0.5;
    vertices[3*3+2] = (data_view_extent[1] - data_view_extent[0])*0.5;
    
    setVbo(lab_frame_vbo, vertices.data(), vertices.size(), GL_STATIC_DRAW);
    
    // Draw the lab reference frame 
    shared_window->std_3d_col_program->bind();
    glEnableVertexAttribArray(shared_window->std_3d_col_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, lab_frame_vbo);
    glVertexAttribPointer(shared_window->std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    if (isBackgroundBlack) glUniform4fv(shared_window->std_3d_col_color, 1, magenta_light.data());
    else glUniform4fv(shared_window->std_3d_col_color, 1, blue_light.data());
    
    glUniformMatrix4fv(shared_window->std_3d_col_transform, 1, GL_FALSE, view_matrix.colmajor().toFloat().data());
    
    GLuint indices[] = {0,1,0,2,0,3};
    glDrawElements(GL_LINES,  6, GL_UNSIGNED_INT, indices);
    
    glDisableVertexAttribArray(shared_window->std_3d_col_fragpos);

    shared_window->std_3d_col_program->release();
    
    endRawGLCalls(painter);
    
    // Draw text to indicate lab reference frame directions
    painter->setPen(*normal_pen);
    painter->setBrush(*fill_brush);
    
    Matrix<float> x_2d(1,2,0), y_2d(1,2,0), z_2d(1,2,0);
    getPosition2D(x_2d.data(), vertices.data()+3, &view_matrix);
    getPosition2D(y_2d.data(), vertices.data()+6, &view_matrix);
    getPosition2D(z_2d.data(), vertices.data()+9, &view_matrix);
    
    painter->drawText(QPointF((x_2d[0]+ 1.0) * 0.5 *render_surface->width(), (1.0 - ( x_2d[1]+ 1.0) * 0.5) *render_surface->height()), QString("X (towards source)"));
    painter->drawText(QPointF((y_2d[0]+ 1.0) * 0.5 *render_surface->width(), (1.0 - ( y_2d[1]+ 1.0) * 0.5) *render_surface->height()), QString("Y (up)"));
    painter->drawText(QPointF((z_2d[0]+ 1.0) * 0.5 *render_surface->width(), (1.0 - ( z_2d[1]+ 1.0) * 0.5) *render_surface->height()), QString("Z"));
}

void VolumeRenderWorker::drawCountScalebar(QPainter *painter)
{
    /*
     * Based on the current display values (min and max), draw ticks on the counts scalebar. There are major and minor ticks.
     * */
    
    double data_min, data_max;
    double tick_interdist_min = 10; // pixels
    double exponent;
    
    // Draw transfer function bounding box
    QRectF tsf_rect(0, 0, 20, render_surface->height() - (fps_string_rect.bottom() + 5) - 50);
    tsf_rect += QMargins(30,5,5,5);
    tsf_rect.moveTopRight(QPoint(render_surface->width()-5, fps_string_rect.bottom() + 5));

    tsf_rect -= QMargins(30,5,5,5);
    Matrix<GLfloat> gl_tsf_rect;
    gl_tsf_rect = glRect(tsf_rect);
    
    
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
        size_t iter = 0, num_ticks = 0;
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
        shared_window->std_2d_tex_program->setUniformValue(shared_window->std_2d_tex_texture, 0);
    
        GLfloat texpos[] = {
            0.0, 1.0,
            0.0, 0.0,
            1.0, 0.0,
            1.0, 1.0
        };
    
        GLuint indices[] = {0,1,3,1,2,3};
        
//        gl_tsf_rect.print(2,"gl_tsf_rect");
        glUniformMatrix4fv(shared_window->std_2d_tex_transform, 1, GL_FALSE, identity.data());
        
        glVertexAttribPointer(shared_window->std_2d_tex_fragpos, 2, GL_FLOAT, GL_FALSE, 0, gl_tsf_rect.data());
        glVertexAttribPointer(shared_window->std_2d_tex_pos, 2, GL_FLOAT, GL_FALSE, 0, texpos);
    
        glEnableVertexAttribArray(shared_window->std_2d_tex_fragpos);
        glEnableVertexAttribArray(shared_window->std_2d_tex_pos);
    
        glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);
    
        glDisableVertexAttribArray(shared_window->std_2d_tex_pos);
        glDisableVertexAttribArray(shared_window->std_2d_tex_fragpos);
        glBindTexture(GL_TEXTURE_2D, 0);
    
        shared_window->std_2d_tex_program->release();
        
        
        // Draw the ticks
//        RotationMatrix<double> identity;
//        identity.setIdentity(4);
        
        shared_window->std_2d_col_program->bind();
        glEnableVertexAttribArray(shared_window->std_2d_col_fragpos);
    
        glBindBuffer(GL_ARRAY_BUFFER, count_scalebar_vbo);
        glVertexAttribPointer(shared_window->std_2d_col_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    
        glUniformMatrix4fv(shared_window->std_2d_col_transform, 1, GL_FALSE, identity.data());
    
        glUniform4fv(shared_window->std_2d_col_color, 1, clear_color_inverse.data());
    
        glDrawArrays(GL_LINES,  0, iter*2);
    
        glDisableVertexAttribArray(shared_window->std_2d_col_fragpos);
    
        shared_window->std_2d_col_program->release();
        
        endRawGLCalls(painter);
    
        painter->setBrush(*normal_brush);
        painter->drawRect(tsf_rect);
    }
    
}

void VolumeRenderWorker::addMarker()
{
    if (markers.size() < 10)
    {
        Matrix<double> xyz(1,3);
        xyz[0] = data_view_extent[0] + (data_view_extent[1] - data_view_extent[0])*0.5;
        xyz[1] = data_view_extent[2] + (data_view_extent[3] - data_view_extent[2])*0.5;
        xyz[2] = data_view_extent[4] + (data_view_extent[5] - data_view_extent[4])*0.5;
        
        markers.append(Marker(xyz[0],xyz[1],xyz[2]));
        
        Matrix<float> vertices = markers.last().getVerts();
        
        
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT)*vertices.size(), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        marker_vbo.append(vbo);
    }
}

void VolumeRenderWorker::tickzerize(double min, double max, double size, double min_interdist, double * qualified_exponent, double * start, size_t * num_ticks)
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

void VolumeRenderWorker::drawRayTex(QPainter *painter)
{
    beginRawGLCalls(painter);
    
    // Volume rendering
    if (isModelActive) raytrace(cl_model_raytrace);
    else if(isSvoInitialized) raytrace(cl_svo_raytrace);

    // Draw texture given one of the above is true
    if (isModelActive || isSvoInitialized)
    {
        shared_window->std_2d_tex_program->bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ray_tex_gl);
        shared_window->std_2d_tex_program->setUniformValue(shared_window->std_2d_tex_texture, 0);

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
        
        glUniformMatrix4fv(shared_window->std_2d_col_transform, 1, GL_FALSE, identity.data());

        glVertexAttribPointer(shared_window->std_2d_tex_fragpos, 2, GL_FLOAT, GL_FALSE, 0, fragpos);
        glVertexAttribPointer(shared_window->std_2d_tex_pos, 2, GL_FLOAT, GL_FALSE, 0, texpos);

        glEnableVertexAttribArray(shared_window->std_2d_tex_fragpos);
        glEnableVertexAttribArray(shared_window->std_2d_tex_pos);

        glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);

        glDisableVertexAttribArray(shared_window->std_2d_tex_pos);
        glDisableVertexAttribArray(shared_window->std_2d_tex_fragpos);
        glBindTexture(GL_TEXTURE_2D, 0);

        shared_window->std_2d_tex_program->release();
    }
    
    endRawGLCalls(painter);
}

void VolumeRenderWorker::raytrace(cl_kernel kernel)
{
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
    pool_dim[2] = ((n_bricks) / ((1 << svo->getBrickPoolPower())*(1 << svo->getBrickPoolPower())) + 1)*svo->getBrickOuterDimension();
    
//    qDebug() << n_bricks << pool_dim[0] << pool_dim[1] << pool_dim[2] << pool_dim[0]*pool_dim[1]*pool_dim[2];
    
//    unsigned int non_empty_node_counter_rounded_up = non_empty_node_counter + ((pool_dimension[0] * pool_dimension[1] / (svo->getBrickOuterDimension()*svo->getBrickOuterDimension())) - (non_empty_node_counter % (pool_dimension[0] * pool_dimension[1] / (svo->getBrickOuterDimension()*svo->getBrickOuterDimension()))));
    
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

    Matrix<float> tmp(1, pool_dim[0]*pool_dim[1]*pool_dim[2], 0);
    
//    qDebug() << tmp.size() << svo->pool.size();
    
    // This will be obsolete when the voxels are stored in a better way
    for (size_t i = 0; i < svo->pool.size(); i++)
    {
        tmp[i] = svo->pool[i];    
    }
    
    cl_svo_pool = clCreateImage3D ( *context_cl->getContext(),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        &cl_pool_format,
        pool_dim[0],
        pool_dim[1],
        pool_dim[2],
        0,
        0,
        tmp.data(),
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
                        scalebar_multiplier = tick_interdistance * 10.0;
                        if ((size_t) n_position_scalebar_ticks+3 < position_scalebar_ticks.m())
                        {
                            getPosition2D(position_scalebar_ticks.data() + 3 * n_position_scalebar_ticks, scalebar_coords.data() + (coord_counter+0)*3, &scalebar_view_matrix);
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] = (position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] + 1.0) * 0.5 *render_surface->width();
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] = (1.0 - (position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] + 1.0) * 0.5) *render_surface->height();
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 2] = j * 0.1 * scalebar_multiplier;
                            n_position_scalebar_ticks++;

                            getPosition2D(position_scalebar_ticks.data() + 3 * n_position_scalebar_ticks, scalebar_coords.data() + (coord_counter+4)*3, &scalebar_view_matrix);
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] = (position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] + 1.0) * 0.5 *render_surface->width();
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] = (1.0 - (position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] + 1.0) * 0.5) *render_surface->height();
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 2] = j * 0.1 * scalebar_multiplier;
                            n_position_scalebar_ticks++;

                            getPosition2D(position_scalebar_ticks.data() + 3 * n_position_scalebar_ticks, scalebar_coords.data() + (coord_counter+8)*3, &scalebar_view_matrix);
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] = (position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] + 1.0) * 0.5 *render_surface->width();
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] = (1.0 - (position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] + 1.0) * 0.5) *render_surface->height();
                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 2] = j * 0.1 * scalebar_multiplier;
                            n_position_scalebar_ticks++;
                        }
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
   quality_percentage = value;
}

void VolumeRenderWorker::refreshTexture()
{
   setRayTexture(quality_percentage);
}

void VolumeRenderWorker::setCountIntegration()
{
    isCountIntegrationActive = !isCountIntegrationActive;
}

void VolumeRenderWorker::setProjection()
{
    isOrthonormal = !isOrthonormal;
    ctc_matrix.setProjection(isOrthonormal);

    float f;
    if (isOrthonormal) f = 0.9;
    else f = 0.7;

    projection_scaling[0] = f;
    projection_scaling[5] = f;
    projection_scaling[10] = f;
}

void VolumeRenderWorker::setLabFrame()
{
    isLabFrameActive = !isLabFrameActive;
}

void VolumeRenderWorker::setBackground()
{
    isBackgroundBlack = !isBackgroundBlack;

    Matrix<GLfloat> tmp;
    tmp = clear_color;

    // Swap color
    clear_color = clear_color_inverse;
    clear_color_inverse = tmp;
    
    QColor normal_color = clear_color_inverse.toQColor();
    normal_color.setAlphaF(1.0);
    QColor fill_color = clear_color.toQColor();
    fill_color.setAlphaF(0.7);
    
    normal_pen->setColor(normal_color);
    whatever_pen->setColor(normal_color);
    fill_brush->setColor(fill_color);
}

void VolumeRenderWorker::setURotation()
{
    isURotationActive = !isURotationActive;
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

    shadow_kernel_arg = rotation.inverse4x4().toFloat()*shadow_kernel_arg;

    clSetKernelArg(cl_model_raytrace, 11, sizeof(cl_float4),  shadow_kernel_arg.data());
}

void VolumeRenderWorker::setIntegration2D()
{
    isIntegration2DActive = !isIntegration2DActive;

//    if (!isOrthonormal) emit changedMessageString("\nWarning: Perspective projection is currently active.");

    if (isInitialized) setMiscArrays();
}
void VolumeRenderWorker::setIntegration3D()
{
    isIntegration3DActive = !isIntegration3DActive;

//    if (!isOrthonormal) emit changedMessageString("\nWarning: Perspective projection is currently active.");

    if (isInitialized) setMiscArrays();
}
void VolumeRenderWorker::setViewMode(int value)
{
    if (value == 0)
    {
        isIntegration3DActive = true;
    }
    else if (value == 1)
    {
        isIntegration3DActive = false;
        isSlicingActive = false;
    }
    else if (value == 2)
    {
        isSlicingActive = true;
    }
    
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
}

