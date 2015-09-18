#include "imagepreview.h"
#include <QBrush>
#include <QRect>
#include <QColor>
#include <QDateTime>
#include <QCoreApplication>
#include <QFontMetrics>
#include <QOpenGLFramebufferObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QFileInfo>
#include <QThreadPool>
#include <QFuture>

#include "sql/sqlqol.h"


static const size_t REDUCED_PIXELS_MAX_BYTES = 1000e6;

ImageWorker::ImageWorker()
{
    initializeOpenCLFunctions();

    context_cl.initDevices();
    context_cl.initNormalContext();
    context_cl.initCommandQueue();
    initializeOpenCLKernels();
}

ImageWorker::~ImageWorker()
{

}

void ImageWorker::setTraceContainer(QList<Matrix<float>> * list)
{
    traces = list;
}

void ImageWorker::initializeOpenCLKernels()
{
    // Build programs from OpenCL kernel source
    QStringList paths;
    paths << "kernels/scattering_data_operations.cl";

    context_cl.createProgram(paths, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    context_cl.buildProgram("-Werror -cl-std=CL1.2");

    cl_buffer_max =  QOpenCLCreateKernel(context_cl.program(), "bufferMax", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }
}

void ImageWorker::reconstructSet(SeriesSet set)
{

}

void ImageWorker::traceSeries(SeriesSet set)
{
    DetectorFile frame;

    frame.setPath(set.current()->begin()->filePath());

    Matrix<float> zeros_like_frame(frame.height(), frame.width(), 0.0f);

    cl_mem trace_image_gpu = QOpenCLCreateBuffer( context_cl.context(),
                             CL_MEM_COPY_HOST_PTR,
                             zeros_like_frame.bytes(),
                             zeros_like_frame.data(),
                             &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // For each image in the series
    for (int j = 0; j < set.current()->size(); j++)
    {
        frame.readHeader();
        frame.readBody();

        // Read data and send to a VRAM buffer.
        cl_mem image_gpu = QOpenCLCreateBuffer( context_cl.context(),
                                                CL_MEM_COPY_HOST_PTR,
                                                frame.bytes(),
                                                (void *) frame.data().data(),
                                                &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        // Use OpenCL to check data against a second VRAM buffer, keeping the max value
        Matrix<size_t> global_ws(1, 2);
        Matrix<size_t> local_ws(1, 2);

        local_ws[0] = 16;
        local_ws[1] = 16;

        global_ws[0] = frame.width() + local_ws[0] - frame.width() % local_ws[0];
        global_ws[1] = frame.height() + local_ws[1] - frame.height() % local_ws[1];

        Matrix<int> image_size(1, 2);
        image_size[0] = frame.width();
        image_size[1] = frame.height();

        err =   QOpenCLSetKernelArg(cl_buffer_max,  0, sizeof(cl_mem), (void *) &image_gpu);
        err |=   QOpenCLSetKernelArg(cl_buffer_max, 1, sizeof(cl_mem), (void *) &trace_image_gpu);
        err |=   QOpenCLSetKernelArg(cl_buffer_max, 2, sizeof(cl_int2), image_size.data());

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        err = QOpenCLEnqueueNDRangeKernel(context_cl.queue(), cl_buffer_max, 2, NULL, global_ws.data(), local_ws.data(), 0, NULL, NULL);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        err =  QOpenCLReleaseMemObject(image_gpu);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        frame.setPath(set.current()->next()->filePath());

//        emit pathChanged(set.current()->current()->path());
        emit progressRangeChanged(0, set.current()->size() - 1);
        emit progressChanged(set.current()->i());
    }

    // Read back the second VRAM buffer and store in system RAM for later usage
    Matrix<float> trace_image(frame.height(), frame.width());

    err =   QOpenCLEnqueueReadBuffer ( context_cl.queue(),
                                       trace_image_gpu,
                                       CL_TRUE,
                                       0,
                                       trace_image.bytes(),
                                       trace_image.data(),
                                       0, NULL, NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    (*traces)[set.i()] = trace_image;

    err =  QOpenCLReleaseMemObject(trace_image_gpu);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    emit traceFinished();
}


ImageOpenGLWidget::ImageOpenGLWidget(QObject * parent) :
    p_application_mode("All with this path"),
    isBeamOverrideActive(false),
    isImageTexInitialized(false),
    isTsfTexInitialized(false),
    isCLInitialized(false),
    isGLInitialized(false),
    isWeightCenterActive(false),
    isSetTraced(false),
    texture_number(0),
    mode(0),
    isLog(1),
    isCorrectionLorentzActive(0),
    isCorrectionNoiseActive(0),
    isCorrectionPlaneActive(0),
    isCorrectionPolarizationActive(0),
    isCorrectionFluxActive(0),
    isCorrectionExposureActive(0),
    isCorrectionPixelProjectionActive(0),
    isEwaldCircleActive(false),
    isImageTooltipActive(true),
    is_populateInterpolationTree_canceled(false)
{
    progressPollTimer = new QTimer;
    progressPollTimer->setInterval(100);
    connect(progressPollTimer, SIGNAL(timeout()), this, SLOT(pollProgress()));

    // Worker
    workerThread = new QThread;
    imageWorker = new ImageWorker;
    imageWorker->setTraceContainer(&set_trace);
    imageWorker->moveToThread(workerThread);
    connect(workerThread, SIGNAL(finished()), imageWorker, SLOT(deleteLater()));
    connect(this, SIGNAL(runTraceWorker(SeriesSet)), imageWorker, SLOT(traceSeries(SeriesSet)));
    connect(imageWorker, SIGNAL(traceFinished()), this, SLOT(setFrame()));
    connect(imageWorker, SIGNAL(traceFinished()), this, SLOT(setSeriesTrace()));
    workerThread->start();


    Q_UNUSED(parent);

    parameter.reserve(1, 16);
    parameter[0] = 0;
    parameter[1] = 1e9;
    parameter[2] = 0;
    parameter[3] = 1e9;
    parameter[12] = 0;
    parameter[13] = 1000;

    texture_view_matrix.setIdentity(4);
    translation_matrix.setIdentity(4);
    zoom_matrix.setIdentity(4);

    texel_view_matrix.setIdentity(4);
    texel_offset_matrix.setIdentity(4);

    image_tex_size.set(1, 2, 0);
    image_buffer_size.set(1, 2, 2);

    prev_pixel.set(1, 2, 0);

    p_future_watcher = new QFutureWatcher<void>(this);

    connect(p_future_watcher, SIGNAL(finished()), this, SLOT(on_populateInterpolationTree_finished()));
    connect(p_future_watcher, SIGNAL(canceled()), this, SLOT(on_populateInterpolationTree_canceled()));
}

ImageWorker * ImageOpenGLWidget::worker()
{
    return imageWorker;
}


QFutureWatcher<void> *ImageOpenGLWidget::watcher()
{
    return p_future_watcher;
}

void ImageOpenGLWidget::paintGL()
{
    QOpenGLPaintDevice paint_device_gl(this->size());

    QPainter painter(&paint_device_gl);

    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    beginRawGLCalls(&painter);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    const qreal retinaScale = this->devicePixelRatio();

    glViewport(0, 0, this->width()*retinaScale, this->height()*retinaScale);

    endRawGLCalls(&painter);

    if (image.isDataRead())
    {
        QRectF image_rect(QPoint(0, 0), QSizeF(image.width(), image.height()));
        image_rect.moveTopLeft(QPointF((qreal) this->width() * 0.5, (qreal) this->height() * 0.5));

        beginRawGLCalls(&painter);
        drawImage(image_rect, image_tex_gl, &painter);
        endRawGLCalls(&painter);

        ColorMatrix<float> analysis_area_color(0.0, 0, 0, 0.3);
        drawSelection(p_working_data[p_current_filepath].selection(), &painter, analysis_area_color);

        if (isCorrectionPlaneActive) drawPlaneMarkerToolTip(&painter);

        if (isWeightCenterActive) drawWeightpoint(p_working_data[p_current_filepath].selection(), &painter);

        if (isEwaldCircleActive) drawConeEwaldIntersect(&painter);

        if (isImageTooltipActive) drawPixelToolTip(&painter);
    }
    else
    {
        QRectF image_rect(QPoint(0, 0), QSizeF(texture_noimage->width(), texture_noimage->height()));
        image_rect.moveTopLeft(QPointF((qreal) this->width() * 0.5, (qreal) this->height() * 0.5));

        beginRawGLCalls(&painter);
        drawImage(image_rect, texture_noimage->textureId(), &painter);
        endRawGLCalls(&painter);
    }
}

void ImageOpenGLWidget::resizeGL(int w, int h)
{
}

void ImageOpenGLWidget::initializeGL()
{
    // Initialize OpenGL
    QOpenGLFunctions::initializeOpenGLFunctions();

    glGenBuffers(5, selections_vbo);
    glGenBuffers(5, weightpoints_vbo);
    glGenTextures(1, &image_tex_gl);
    glGenTextures(1, &tsf_tex_gl);

    // Shader for drawing textures in 2D
    std_2d_tex_program = new QOpenGLShaderProgram(this);
    std_2d_tex_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/std_2d_tex.v.glsl");
    std_2d_tex_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/std_2d_tex.f.glsl");

    if (!std_2d_tex_program->link())
    {
        qFatal(std_2d_tex_program->log().toStdString().c_str());
    }

    if ((std_2d_tex_fragpos = std_2d_tex_program->attributeLocation("fragpos")) == -1)
    {
        qFatal("Invalid attribute");
    }

    if ((std_2d_tex_pos = std_2d_tex_program->attributeLocation("texpos")) == -1)
    {
        qFatal("Invalid attribute");
    }

    if ((std_2d_tex_texture = std_2d_tex_program->uniformLocation("texture")) == -1)
    {
        qFatal("Invalid uniform");
    }

    if ((std_2d_tex_transform = std_2d_tex_program->uniformLocation("transform")) == -1)
    {
        qFatal("Invalid uniform");
    }

    // Shader for drawing sprites in 2D
    std_2d_sprite_program = new QOpenGLShaderProgram(this);
    std_2d_sprite_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/std_2d_sprite.v.glsl");
    std_2d_sprite_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/std_2d_sprite.f.glsl");

    if (!std_2d_sprite_program->link())
    {
        qFatal(std_2d_sprite_program->log().toStdString().c_str());
    }

    if ((std_2d_sprite_fragpos = std_2d_sprite_program->attributeLocation("fragpos")) == -1)
    {
        qFatal("Invalid attribute");
    }

    if ((std_2d_sprite_pos = std_2d_sprite_program->attributeLocation("texpos")) == -1)
    {
        qFatal("Invalid attribute");
    }

    if ((std_2d_sprite_texture = std_2d_sprite_program->uniformLocation("texture")) == -1)
    {
        qFatal("Invalid uniform");
    }

    if ((std_2d_sprite_transform = std_2d_sprite_program->uniformLocation("transform")) == -1)
    {
        qFatal("Invalid uniform");
    }

    // Shader for drawing lines and similar in 2D
    std_2d_col_program = new QOpenGLShaderProgram(this);
    std_2d_col_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/std_2d_col.v.glsl");
    std_2d_col_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/std_2d_col.f.glsl");

    if (!std_2d_col_program->link())
    {
        qFatal(std_2d_col_program->log().toStdString().c_str());
    }

    if ((std_2d_col_fragpos = std_2d_col_program->attributeLocation("fragpos")) == -1)
    {
        qFatal("Invalid attribute");
    }

    if ((std_2d_col_color = std_2d_col_program->uniformLocation("color")) == -1)
    {
        qFatal("Invalid uniform");
    }

    if ((std_2d_col_transform = std_2d_col_program->uniformLocation("transform")) == -1)
    {
        qFatal("Invalid uniform");
    }



    // Load stored textures
    texture_noimage = new QOpenGLTexture(QImage(":/art/noimage.png"));
    texture_noimage->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture_noimage->setMagnificationFilter(QOpenGLTexture::Linear);

    centerCurrentImage();

    texture_image_marker = new QOpenGLTexture(QImage(":/art/crosshair_one.png"));
    texture_image_marker->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture_image_marker->setMagnificationFilter(QOpenGLTexture::Linear);

    isGLInitialized = true;

    // Initialize OpenCL
    initializeCL();

    isCLInitialized = true;

    setRgb("Hot");
    setAlpha("Opaque");
}

ImageOpenGLWidget::~ImageOpenGLWidget()
{

    if (!(isCLInitialized && isGLInitialized))
    {
        return;
    }

    glDeleteBuffers(5, selections_vbo);
    glDeleteBuffers(5, weightpoints_vbo);

    workerThread->quit();
    workerThread->wait();
}

void ImageOpenGLWidget::traceSeriesSlot()
{
//    emit runTraceWorker(p_current_filepath);
}

void ImageOpenGLWidget::setBeamOverrideActive(bool value)
{
    isBeamOverrideActive = value;
}

void ImageOpenGLWidget::setBeamXOverride(double value)
{
    beam_x_override = value;
}

void ImageOpenGLWidget::setBeamYOverride(double value)
{
    beam_y_override = value;
}


void ImageOpenGLWidget::setOffsetOmega(double value)
{
    offset_omega = value * pi / 180.0;
}
void ImageOpenGLWidget::setOffsetKappa(double value)
{
    offset_kappa = value * pi / 180.0;
}
void ImageOpenGLWidget::setOffsetPhi(double value)
{
    offset_phi = value * pi / 180.0;
}

void ImageOpenGLWidget::killProcess()
{
    kill_flag = true;
}

void ImageOpenGLWidget::setReducedPixels(Matrix<float> * reduced_pixels)
{
    this->reduced_pixels = reduced_pixels;
}

int ImageOpenGLWidget::projectFile(DetectorFile * file, Selection selection, Matrix<float> * samples, size_t * n_samples)
{
    // Project and correct the data
    // Prepare the target for storage of projected and corrected pixels (intensity but also xyz position)
    cl_mem buffer_cl = QOpenCLCreateBuffer( context_cl.context(),
                                                 CL_MEM_ALLOC_HOST_PTR,
                                                 file->width() * file->height() * sizeof(cl_float),
                                                 NULL,
                                                 &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Load data into a CL texture
    cl_image_format source_format;
    source_format.image_channel_order = CL_INTENSITY;
    source_format.image_channel_data_type = CL_FLOAT;

    // Sample rotation matrix to be applied to each projected pixel to account for rotations. First set the active angle. Ideally this would be given by the header file, but for some reason it is not stated in there. Maybe it is just so normal to rotate around the omega angle to keep the resolution function consistent
    float phi = file->phi();
    float kappa = file->kappa();
    float omega = file->omega();

    RotationMatrix<double> PHI;
    RotationMatrix<double> KAPPA;
    RotationMatrix<double> OMEGA;

    file->setAlpha(0.8735582);
    file->setBeta(0.000891863);

    PHI.setArbRotation(file->beta(), 0, -(phi + offset_phi));
    KAPPA.setArbRotation(file->alpha(), 0, -(kappa + offset_kappa));
    OMEGA.setZRotation(-(omega + offset_omega));

    // The sample rotation matrix. Some rotations perturb the other rotation axes, and in the above calculations for phi, kappa, and omega we use fixed axes. It is therefore neccessary to put a rotation axis back into its basic position before the matrix is applied. In our case omega perturbs kappa and phi, and kappa perturbs phi. Thus we must first rotate omega back into the base position to recover the base rotation axis of kappa. Then we recover the base rotation axis for phi in the same manner. The order of matrix operations thus becomes:
    RotationMatrix<double> sampleRotMat;
    sampleRotMat = PHI * KAPPA * OMEGA;

    cl_mem sample_rotation_matrix_cl = QOpenCLCreateBuffer(context_cl.context(),
                                       CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                       sampleRotMat.toFloat().bytes(),
                                       sampleRotMat.toFloat().data(),
                                       &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // The sampler for cl_tsf_tex
    cl_sampler tsf_sampler = QOpenCLCreateSampler(context_cl.context(), true, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Set kernel arguments
    float pixel_size_x = file->pixSizeX();
    float pixel_size_y = file->pixSizeY();
    float wavelength = file->wavelength();
    float detector_distance = file->detectorDist();
    float beam_center_x = (isBeamOverrideActive ? file->beamX() + beam_x_override : file->beamX());
    float beam_center_y = (isBeamOverrideActive ? file->beamY() + beam_y_override : file->beamY());
    float start_angle = file->startAngle();
    float angle_increment = file->angleIncrement();

    Matrix<int> image_size(1, 2);
    image_size[0] = file->width();
    image_size[1] = file->height();

    err = QOpenCLSetKernelArg(cl_project_data, 0, sizeof(cl_mem), (void *) &buffer_cl);
    err |= QOpenCLSetKernelArg(cl_project_data, 1, sizeof(cl_mem), (void *) &image_data_corrected_cl);
    err |= QOpenCLSetKernelArg(cl_project_data, 2, sizeof(cl_sampler), &tsf_sampler);
    err |= QOpenCLSetKernelArg(cl_project_data, 3, sizeof(cl_mem), (void *) &sample_rotation_matrix_cl);
    err |= QOpenCLSetKernelArg(cl_project_data, 4, sizeof(cl_float), &pixel_size_x);
    err |= QOpenCLSetKernelArg(cl_project_data, 5, sizeof(cl_float), &pixel_size_y);
    err |= QOpenCLSetKernelArg(cl_project_data, 6, sizeof(cl_float), &wavelength);
    err |= QOpenCLSetKernelArg(cl_project_data, 7, sizeof(cl_float), &detector_distance);
    err |= QOpenCLSetKernelArg(cl_project_data, 8, sizeof(cl_float), &beam_center_x);
    err |= QOpenCLSetKernelArg(cl_project_data, 9, sizeof(cl_float), &beam_center_y);
    err |= QOpenCLSetKernelArg(cl_project_data, 10, sizeof(cl_float), &start_angle);
    err |= QOpenCLSetKernelArg(cl_project_data, 11, sizeof(cl_float), &angle_increment);
    err |= QOpenCLSetKernelArg(cl_project_data, 12, sizeof(cl_float), &kappa);
    err |= QOpenCLSetKernelArg(cl_project_data, 13, sizeof(cl_float), &phi);
    err |= QOpenCLSetKernelArg(cl_project_data, 14, sizeof(cl_float), &omega);
    err |= QOpenCLSetKernelArg(cl_project_data, 15, sizeof(cl_int4), selection.lrtb().data());
    err |= QOpenCLSetKernelArg(cl_project_data, 16, sizeof(cl_int2), image_size.data());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    /* Launch rendering kernel */
    size_t area_per_call[2] = {128, 128};
    size_t call_offset[2] = {0, 0};
    size_t loc_ws[2];
    size_t glb_ws[2];

    loc_ws[0] = 16;
    loc_ws[1] = 16;
    glb_ws[0] = file->width() + loc_ws[0] - (file->width() % loc_ws[0]);
    glb_ws[1] = file->height() + loc_ws[1] - (file->height() % loc_ws[1]);

    for (size_t glb_x = 0; glb_x < glb_ws[0]; glb_x += area_per_call[0])
    {
        for (size_t glb_y = 0; glb_y < glb_ws[1]; glb_y += area_per_call[1])
        {
            call_offset[0] = glb_x;
            call_offset[1] = glb_y;

            err = QOpenCLEnqueueNDRangeKernel(context_cl.queue(), cl_project_data, 2, call_offset, area_per_call, loc_ws, 0, NULL, NULL);

            if ( err != CL_SUCCESS)
            {
                qFatal(cl_error_cstring(err));
            }
        }
    }

    QOpenCLFinish(context_cl.queue());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Read the data
    Matrix<size_t> host_origin(1, 3, 0);

    Matrix<size_t> buffer_origin(1,3);
    buffer_origin[0] = selection.left();
    buffer_origin[1] = selection.top();
    buffer_origin[2] = 0;

    Matrix<size_t> region(1,3);
    region[0] = selection.width();
    region[1] = selection.height();
    region[2] = 1;

    Matrix<float> projected_data_buf(selection.height(), selection.width() * 4);

    err =   QOpenCLEnqueueReadBufferRect ( context_cl.queue(),
                                           buffer_cl,
                                           CL_TRUE,
                                           buffer_origin.data(),
                                           host_origin.data(),
                                           region.data(),
                                           file->width() * sizeof(cl_float4),
                                           file->width() * file->height() * sizeof(cl_float4),
                                           selection.width() * sizeof(cl_float4),
                                           selection.width() * selection.height() * sizeof(cl_float4),
                                           projected_data_buf.data(),
                                           0, NULL, NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    if (buffer_cl)
    {
        err = QOpenCLReleaseMemObject(buffer_cl);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }

    if (sample_rotation_matrix_cl)
    {
        err = QOpenCLReleaseMemObject(sample_rotation_matrix_cl);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }

    if (tsf_sampler)
    {
        err = QOpenCLReleaseSampler(tsf_sampler);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }

    emit changedFormatMemoryUsage(QString("Mem usage: %p% (%v of %m MB)"));


    for (int i = 0; i < selection.width()*selection.height(); i++)
    {
        if (projected_data_buf[i * 4 + 3] > 0.0) // Above 0 check
        {
            if ((*n_samples) + 3 < samples->size())
            {
                (*samples)[*n_samples + 0] = projected_data_buf[i * 4 + 0];
                (*samples)[*n_samples + 1] = projected_data_buf[i * 4 + 1];
                (*samples)[*n_samples + 2] = projected_data_buf[i * 4 + 2];
                (*samples)[*n_samples + 3] = projected_data_buf[i * 4 + 3];
                *n_samples += 4;
            }
            else
            {
                emit changedRangeMemoryUsage(0, REDUCED_PIXELS_MAX_BYTES / 1e6);
                emit changedMemoryUsage(*n_samples * 4 / 1e6);
                return 0;
            }
        }
    }

    emit changedRangeMemoryUsage(0, REDUCED_PIXELS_MAX_BYTES / 1e6);
    emit changedMemoryUsage(*n_samples * 4 / 1e6);


    return 1;
}


void ImageOpenGLWidget::processScatteringData(cl_mem data_buf_cl, cl_mem out_buf_cl, Matrix<float> &param, Matrix<size_t> &image_size, Matrix<size_t> &local_ws, float mean, float deviation, int task)
{
    // Prepare kernel parameters
    Matrix<size_t> global_ws(1, 2);

    global_ws[0] = image_size[0] + (local_ws[0] - ((size_t) image_size[0]) % local_ws[0]);
    global_ws[1] = image_size[1] + (local_ws[1] - ((size_t) image_size[1]) % local_ws[1]);

    // Set kernel parameters
    err =   QOpenCLSetKernelArg(cl_process_data,  0, sizeof(cl_mem), (void *) &data_buf_cl);
    err |=   QOpenCLSetKernelArg(cl_process_data, 1, sizeof(cl_mem), (void *) &out_buf_cl);
    err |=   QOpenCLSetKernelArg(cl_process_data, 2, sizeof(cl_mem), &parameter_cl);
    err |=   QOpenCLSetKernelArg(cl_process_data, 3, sizeof(cl_int2), image_size.toInt().data());
    err |=   QOpenCLSetKernelArg(cl_process_data, 4, sizeof(cl_int), &isCorrectionLorentzActive);
    err |=   QOpenCLSetKernelArg(cl_process_data, 5, sizeof(cl_int), &task);
    err |=   QOpenCLSetKernelArg(cl_process_data, 6, sizeof(cl_float), &mean);
    err |=   QOpenCLSetKernelArg(cl_process_data, 7, sizeof(cl_float), &deviation);
    err |=   QOpenCLSetKernelArg(cl_process_data, 8, sizeof(cl_int), &isCorrectionNoiseActive);
    err |=   QOpenCLSetKernelArg(cl_process_data, 9, sizeof(cl_int), &isCorrectionPlaneActive);
    err |=   QOpenCLSetKernelArg(cl_process_data, 10, sizeof(cl_int), &isCorrectionPolarizationActive);
    err |=   QOpenCLSetKernelArg(cl_process_data, 11, sizeof(cl_int), &isCorrectionFluxActive);
    err |=   QOpenCLSetKernelArg(cl_process_data, 12, sizeof(cl_int), &isCorrectionExposureActive);
    err |=   QOpenCLSetKernelArg(cl_process_data, 13, sizeof(cl_float4), getPlane().toFloat().data());
    err |=   QOpenCLSetKernelArg(cl_process_data, 14, sizeof(cl_int), &isCorrectionPixelProjectionActive);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }


    // Launch the kernel
    err =   QOpenCLEnqueueNDRangeKernel(context_cl.queue(), cl_process_data, 2, NULL, global_ws.data(), local_ws.data(), 0, NULL, NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err =   QOpenCLFinish(context_cl.queue());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }
}

void ImageOpenGLWidget::scatteringDataToImage(cl_mem data_buf_cl, cl_mem frame_image_cl, cl_mem tsf_image_cl, Matrix<float> &data_limit, Matrix<size_t> &image_size, Matrix<size_t> &local_ws, cl_sampler tsf_sampler, int log)
{
    /*
     * Display an image buffer object, matching intensity to color
     * */

    if (!image.isDataRead())
    {
        return;
    }

    // Aquire shared CL/GL objects
    makeCurrent();

    glFinish();

    err =  QOpenCLEnqueueAcquireGLObjects(context_cl.queue(), 1, &frame_image_cl, 0, 0, 0);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err =  QOpenCLEnqueueAcquireGLObjects(context_cl.queue(), 1, &tsf_image_cl, 0, 0, 0);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Prepare kernel parameters
    Matrix<size_t> global_ws(1, 2);
    global_ws[0] = image_size[0] + (local_ws[0] - ((size_t) image_size[0]) % local_ws[0]);
    global_ws[1] = image_size[1] + (local_ws[1] - ((size_t) image_size[1]) % local_ws[1]);

    // Set kernel parameters
    err =   QOpenCLSetKernelArg(cl_data_to_image,  0, sizeof(cl_mem), (void *) &data_buf_cl);
    err |=   QOpenCLSetKernelArg(cl_data_to_image, 1, sizeof(cl_mem), (void *) &frame_image_cl);
    err |=   QOpenCLSetKernelArg(cl_data_to_image, 2, sizeof(cl_mem), (void *) &tsf_image_cl);
    err |=   QOpenCLSetKernelArg(cl_data_to_image, 3, sizeof(cl_sampler), &tsf_sampler);
    err |=   QOpenCLSetKernelArg(cl_data_to_image, 4, sizeof(cl_float2), data_limit.data());
    err |=   QOpenCLSetKernelArg(cl_data_to_image, 5, sizeof(cl_int), &log);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }


    // Launch the kernel
    err =   QOpenCLEnqueueNDRangeKernel(context_cl.queue(), cl_data_to_image, 2, NULL, global_ws.data(), local_ws.data(), 0, NULL, NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err =   QOpenCLFinish(context_cl.queue());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Release shared CL/GL objects
    err =  QOpenCLEnqueueReleaseGLObjects(context_cl.queue(), 1, &frame_image_cl, 0, 0, 0);
    err |=  QOpenCLEnqueueReleaseGLObjects(context_cl.queue(), 1, &tsf_image_cl, 0, 0, 0);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }
}

float ImageOpenGLWidget::sumGpuArray(cl_mem cl_data, unsigned int read_size, Matrix<size_t> &local_ws)
{
    /*
     * This function for parallel reduction does its work over multiple passes. First pass
     * sums the data in blocks corresponding to the local size (work group size). The
     * results from each block is written to an extension of the array, or padding. The
     * next pass sums the data (again in blocks) in the padded section, and writes it to
     * the beginning of the buffer. This back and forth cycle is repeated until the buffer
     * is reduced to a single value, the sum.
     * */

    /* Set initial kernel parameters (they will change for each iteration)*/
    Matrix<size_t> global_ws(1, 1);
    unsigned int read_offset = 0;
    unsigned int write_offset;

    global_ws[0] = read_size + (read_size % local_ws[0] ? local_ws[0] - (read_size % local_ws[0]) : 0);
    write_offset = global_ws[0];

    bool forth = true;
    float sum;

    /* Pass arguments to kernel */
    err =   QOpenCLSetKernelArg(cl_parallel_reduction, 0, sizeof(cl_mem), (void *) &cl_data);
    err |=   QOpenCLSetKernelArg(cl_parallel_reduction, 1, local_ws[0] * sizeof(cl_float), NULL);
    err |=   QOpenCLSetKernelArg(cl_parallel_reduction, 2, sizeof(cl_uint), &read_size);
    err |=   QOpenCLSetKernelArg(cl_parallel_reduction, 3, sizeof(cl_uint), &read_offset);
    err |=   QOpenCLSetKernelArg(cl_parallel_reduction, 4, sizeof(cl_uint), &write_offset);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    /* Launch kernel repeatedly until the summing is done */
    while (read_size > 1)
    {
        err =   QOpenCLEnqueueNDRangeKernel(context_cl.queue(), cl_parallel_reduction, 1, 0, global_ws.data(), local_ws.data(), 0, NULL, NULL);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        err =   QOpenCLFinish(context_cl.queue());

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        /* Extract the sum */
        err =   QOpenCLEnqueueReadBuffer ( context_cl.queue(),
                                           cl_data,
                                           CL_TRUE,
                                           forth ? global_ws[0] * sizeof(cl_float) : 0,
                                           sizeof(cl_float),
                                           &sum,
                                           0, NULL, NULL);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        /* Prepare the kernel parameters for the next iteration */
        forth = !forth;

        // Prepare to read memory in front of the separator and write to the memory behind it
        if (forth)
        {
            read_size = (global_ws[0]) / local_ws[0];

            if (read_size % local_ws[0])
            {
                global_ws[0] = read_size + local_ws[0] - (read_size % local_ws[0]);
            }
            else
            {
                global_ws[0] = read_size;
            }

            read_offset = 0;
            write_offset = global_ws[0];
        }
        // Prepare to read memory behind the separator and write to the memory in front of it
        else
        {
            read_offset = global_ws[0];
            write_offset = 0;

            read_size = global_ws[0] / local_ws[0];

            if (read_size % local_ws[0])
            {
                global_ws[0] = read_size + local_ws[0] - (read_size % local_ws[0]);
            }
            else
            {
                global_ws[0] = read_size;
            }
        }

        err =   QOpenCLSetKernelArg(cl_parallel_reduction, 2, sizeof(cl_uint), &read_size);
        err |=   QOpenCLSetKernelArg(cl_parallel_reduction, 3, sizeof(cl_uint), &read_offset);
        err |=   QOpenCLSetKernelArg(cl_parallel_reduction, 4, sizeof(cl_uint), &write_offset);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

    }

    return sum;
}

void ImageOpenGLWidget::processScatteringDataProxy()
{
    /*
     * Carry out calculations on an image buffer, such as corrections and calculation of variance and skewness
     * */
    if (!image.isDataRead())
    {
        return;
    }

    Matrix<size_t> origin(3, 1, 0);

    Matrix<size_t> local_ws(1, 2);
    local_ws[0] = 64;
    local_ws[1] = 1;

    Matrix<size_t> image_size(1, 3);
    image_size[0] = image.width();
    image_size[1] = image.height();
    image_size[2] = 0;

    Matrix<size_t> image_region(1, 3);
    image_region[0] = image.width() * sizeof(cl_float);
    image_region[1] = image.height();
    image_region[2] = 1;

    if (mode == 0)
    {
        // Normal intensity
        {
            switch (texture_number)
            {
                case 0:
                    processScatteringData(image_data_raw_cl, image_data_corrected_cl, parameter, image_size, local_ws, 0, 0, 0);
                    break;

                case 1:
                    processScatteringData(image_data_trace_cl, image_data_corrected_cl, parameter, image_size, local_ws, 0, 0, 0);
                    break;
            }

            // Calculate the weighted intensity position
            processScatteringData(image_data_corrected_cl, image_data_weight_x_cl, parameter, image_size, local_ws, 0, 0, 3);
            processScatteringData(image_data_corrected_cl, image_data_weight_y_cl, parameter, image_size, local_ws, 0, 0, 4);
        }

    }

    if (mode == 1)
    {
        // Variance
        {
            switch (texture_number)
            {
                case 0:
                    processScatteringData(image_data_raw_cl, image_data_corrected_cl, parameter, image_size, local_ws, 0, 0, 0);
                    break;

                case 1:
                    processScatteringData(image_data_trace_cl, image_data_corrected_cl, parameter, image_size, local_ws, 0, 0, 0);
                    break;
            }

            // Calculate the variance
            err = QOpenCLEnqueueCopyBufferRect ( 	context_cl.queue(),
                image_data_corrected_cl,
                image_data_generic_cl,
                origin.data(),
                origin.data(),
                image_region.data(),
                image.width() * sizeof(cl_float),
                0,
                image.width() * sizeof(cl_float),
                0,
                0,
                NULL,
                NULL);

            if ( err != CL_SUCCESS)
            {
                qFatal(cl_error_cstring(err));
            }

            err =   QOpenCLFinish(context_cl.queue());

            if ( err != CL_SUCCESS)
            {
                qFatal(cl_error_cstring(err));
            }

            float mean = sumGpuArray(image_data_generic_cl, image_size[0] * image_size[1], local_ws) / (image_size[0] * image_size[1]);

            processScatteringData(image_data_corrected_cl, image_data_variance_cl, parameter, image_size, local_ws, mean, 0, 1);

            // Calculate the weighted intensity position
            processScatteringData(image_data_variance_cl, image_data_weight_x_cl, parameter, image_size, local_ws, 0, 0, 3);
            processScatteringData(image_data_variance_cl, image_data_weight_y_cl, parameter, image_size, local_ws, 0, 0, 4);
        }
    }
    else if (mode == 2)
    {
        // Skewness
        {
            switch (texture_number)
            {
                case 0:
                    processScatteringData(image_data_raw_cl, image_data_corrected_cl, parameter, image_size, local_ws, 0, 0, 0);
                    break;

                case 1:
                    processScatteringData(image_data_trace_cl, image_data_corrected_cl, parameter, image_size, local_ws, 0, 0, 0);
                    break;
            }

            // Calculate the variance
            err = QOpenCLEnqueueCopyBufferRect ( 	context_cl.queue(),
                image_data_corrected_cl,
                image_data_generic_cl,
                origin.data(),
                origin.data(),
                image_region.data(),
                image.width() * sizeof(cl_float),
                0,
                image.width() * sizeof(cl_float),
                0,
                0,
                NULL,
                NULL);

            if ( err != CL_SUCCESS)
            {
                qFatal(cl_error_cstring(err));
            }

            err =   QOpenCLFinish(context_cl.queue());

            if ( err != CL_SUCCESS)
            {
                qFatal(cl_error_cstring(err));
            }

            float mean = sumGpuArray(image_data_generic_cl, image_size[0] * image_size[1], local_ws) / (image_size[0] * image_size[1]);

            processScatteringData(image_data_corrected_cl, image_data_variance_cl, parameter, image_size, local_ws, mean, 0, 1);

            // Calculate the skewness
            err = QOpenCLEnqueueCopyBufferRect ( 	context_cl.queue(),
                image_data_variance_cl,
                image_data_skewness_cl,
                origin.data(),
                origin.data(),
                image_region.data(),
                image.width() * sizeof(cl_float),
                0,
                image.width() * sizeof(cl_float),
                0,
                0,
                NULL,
                NULL);

            if ( err != CL_SUCCESS)
            {
                qFatal(cl_error_cstring(err));
            }

            err =   QOpenCLFinish(context_cl.queue());

            if ( err != CL_SUCCESS)
            {
                qFatal(cl_error_cstring(err));
            }
//            copyBufferRect(image_data_variance_cl, image_data_generic_cl, image_size, origin, image_size, origin, local_ws);

            float variance = sumGpuArray(image_data_generic_cl, image_size[0] * image_size[1], local_ws) / (image_size[0] * image_size[1]);

            processScatteringData(image_data_variance_cl, image_data_skewness_cl, parameter, image_size, local_ws, mean, sqrt(variance), 2);

            // Calculate the weighted intensity position
            processScatteringData(image_data_skewness_cl, image_data_weight_x_cl, parameter, image_size, local_ws, 0, 0, 3);
            processScatteringData(image_data_skewness_cl, image_data_weight_y_cl, parameter, image_size, local_ws, 0, 0, 4);
        }
    }
    else
    {
        // Should not happen
    }
}



void ImageOpenGLWidget::populateInterpolationTreeMap()
{
    // Note: If the spawned threads consume more than the available memory, either on the host or the gpu, the behaviour is undefined
    QSqlQuery query(QSqlDatabase::database());
    query.prepare("SELECT FilePath FROM cbf_table WHERE Active = :Active ORDER BY FilePath ASC");
    query.bindValue(":Active", 1);
    if (!query.exec()) qDebug() << sqlQueryError(query);

    p_future_list.clear();
    p_interpolation_octree.clear();
    p_interpolation_octree.setParent(NULL);

    double Q = 1.0; // This value should be more related to the actual wavelength
    Matrix<double> extent(1,6);
    extent[0] = -Q;
    extent[1] = Q;
    extent[2] = -Q;
    extent[3] = Q;
    extent[4] = -Q;
    extent[5] = Q;
    p_interpolation_octree.setExtent(extent);

    while (query.next())
    {
        // Prepare detector file.
        QString file_path = query.value(0).toString();
        DetectorFile file(file_path);

        // Set the appropriate selection.
        if (!p_working_data.contains(file_path)) p_working_data[file_path] = ImageInfo(file_path);
        file.setSubImage(p_working_data[file_path].selection());

        //  Pass an object containing variable parameters (typically given by the UI).
        DataCorrectionArgs args;
        args.lorentz_correction = isCorrectionLorentzActive;
        args.flat_background_correction = isCorrectionNoiseActive;
        args.planar_background_correction = isCorrectionPlaneActive;
        args.polarization_correction = isCorrectionPolarizationActive;
        args.flux_correction = isCorrectionFluxActive;
        args.exposure_time_correction = isCorrectionExposureActive;
        args.pixel_projection_correction = isCorrectionPixelProjectionActive;
        args.noise_low = parameter[0];
        file.setCorrectionArgs(args);

        // Give it a CL context, queue, BUT NOT kernel handles. clSetKernelArg is NOT thread safe when used across multiple threads on the same kernel object.
        file.setCLContext(&context_cl);

        // Pass a pointer to an interpolation octree in which to put treated data points.
        file.setInterpolationTree(&p_interpolation_octree);

        // Set mutex for reading and writing to shared data
        file.setMutex(&p_mutex);

        p_future_list << file;
    }

    // Perform the operation
    p_future_watcher->setFuture(QtConcurrent::map(p_future_list, &DetectorFile::populateInterpolationTree));
}

void ImageOpenGLWidget::on_populateInterpolationTree_finished()
{
    if (is_populateInterpolationTree_canceled) p_interpolation_octree.clear();
    p_future_list.clear();
    is_populateInterpolationTree_canceled = false;
}

void ImageOpenGLWidget::on_populateInterpolationTree_canceled()
{
    is_populateInterpolationTree_canceled = true;
    emit message("Canceled operation");
}


void ImageOpenGLWidget::pollProgress()
{
    emit progressChanged(p_n_returned_tasks);
}

void ImageOpenGLWidget::setFrame()
{
    // Set the frame
    image.setPath(p_working_data[p_current_filepath].filePath());

    if (!isCLInitialized || !isGLInitialized)
    {
        return;
    }

    if (!image.readHeader())
    {
        return;
    }

    if (!image.readBody())
    {
        return;
    }

    Selection analysis_area = p_working_data[p_current_filepath].selection();

    // Restrict selection, this could be moved elsewhere and it would look better
    if (analysis_area.left() < 0)
    {
        analysis_area.setLeft(0);
    }

    if (analysis_area.right() >= image.width())
    {
        analysis_area.setRight(image.width() - 1);
    }

    if (analysis_area.top() < 0)
    {
        analysis_area.setTop(0);
    }

    if (analysis_area.bottom() >= image.height())
    {
        analysis_area.setBottom(image.height() - 1);
    }

    p_working_data[p_current_filepath].setSelection(analysis_area);
//    qDebug() << "...3";
//    qDebug() << "Hi liksom" << p_working_data[p_current_filepath].selection();

    Matrix<size_t> image_size(1, 2);
    image_size[0] = image.width();
    image_size[1] = image.height();

    clMaintainImageBuffers(image_size);

    // Write the frame data to the GPU
    err =  QOpenCLEnqueueWriteBuffer(
               context_cl.queue(),
               image_data_raw_cl,
               CL_TRUE,
               0,
               image.bytes(),
               image.data().data(),
               0, 0, 0);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Parameters essential to the frame
    parameter[4] = image.flux();
    parameter[5] = image.expTime();
    parameter[6] = image.wavelength();
    parameter[7] = image.detectorDist();
    parameter[8] = (isBeamOverrideActive ? image.beamX() + beam_x_override : image.beamX());
    parameter[9] = (isBeamOverrideActive ? image.beamY() + beam_y_override : image.beamY());
    parameter[10] = image.pixSizeX();
    parameter[11] = image.pixSizeY();

    setParameter(parameter);

    // Do relevant calculations and render
    processScatteringDataProxy();
    updateImageTexture();
    processSelectionDataProxy(&analysis_area);

    p_working_data[p_current_filepath].setSelection(analysis_area);

    // Emit the image instead of components
//    emit pathChanged(p_working_data[p_current_filepath].path());
}



void ImageOpenGLWidget::clMaintainImageBuffers(Matrix<size_t> &image_size)
{
    if ((image_size[0] != image_buffer_size[0]) || (image_size[1] != image_buffer_size[1]))
    {
        err =  QOpenCLReleaseMemObject(image_data_raw_cl);
        err |=  QOpenCLReleaseMemObject(image_data_trace_cl);
        err |=  QOpenCLReleaseMemObject(image_data_corrected_cl);
        err |=  QOpenCLReleaseMemObject(image_data_weight_x_cl);
        err |=  QOpenCLReleaseMemObject(image_data_weight_y_cl);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        image_data_raw_cl =  QOpenCLCreateBuffer( context_cl.context(),
                             CL_MEM_ALLOC_HOST_PTR,
                             image_size[0] * image_size[1] * sizeof(cl_float),
                             NULL,
                             &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        image_data_trace_cl =  QOpenCLCreateBuffer( context_cl.context(),
                               CL_MEM_ALLOC_HOST_PTR,
                               image_size[0] * image_size[1] * sizeof(cl_float),
                               NULL,
                               &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        image_data_corrected_cl =  QOpenCLCreateBuffer( context_cl.context(),
                                   CL_MEM_ALLOC_HOST_PTR,
                                   image_size[0] * image_size[1] * sizeof(cl_float),
                                   NULL,
                                   &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        image_data_variance_cl =  QOpenCLCreateBuffer( context_cl.context(),
                                  CL_MEM_ALLOC_HOST_PTR,
                                  image_size[0] * image_size[1] * sizeof(cl_float),
                                  NULL,
                                  &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        image_data_skewness_cl =  QOpenCLCreateBuffer( context_cl.context(),
                                  CL_MEM_ALLOC_HOST_PTR,
                                  image_size[0] * image_size[1] * sizeof(cl_float),
                                  NULL,
                                  &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        image_data_weight_x_cl =  QOpenCLCreateBuffer( context_cl.context(),
                                  CL_MEM_ALLOC_HOST_PTR,
                                  image_size[0] * image_size[1] * sizeof(cl_float),
                                  NULL,
                                  &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        image_data_weight_y_cl =  QOpenCLCreateBuffer( context_cl.context(),
                                  CL_MEM_ALLOC_HOST_PTR,
                                  image_size[0] * image_size[1] * sizeof(cl_float),
                                  NULL,
                                  &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        image_data_generic_cl =  QOpenCLCreateBuffer( context_cl.context(),
                                 CL_MEM_ALLOC_HOST_PTR,
                                 image_size[0] * image_size[1] * sizeof(cl_float) * 2, // *2 so it can be used for parallel reduction
                                 NULL,
                                 &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        image_buffer_size[0] = image_size[0];
        image_buffer_size[1] = image_size[1];
    }
}

void ImageOpenGLWidget::processSelectionDataProxy(Selection * area)
{
    Matrix<size_t> local_ws(1, 2);
    local_ws[0] = 64;
    local_ws[1] = 1;

    Matrix<size_t> image_size(1, 2);
    image_size[0] = image.width();
    image_size[1] = image.height();

    if (mode == 0)
    {
        // Normal intensity
        processSelectionData(area, image_data_corrected_cl, image_data_weight_x_cl, image_data_weight_y_cl, image_size, local_ws);
    }

    if (mode == 1)
    {
        // Variance
        processSelectionData(area, image_data_variance_cl, image_data_weight_x_cl, image_data_weight_y_cl, image_size, local_ws);
    }
    else if (mode == 2)
    {
        // Skewness
        processSelectionData(area, image_data_skewness_cl, image_data_weight_x_cl, image_data_weight_y_cl, image_size, local_ws);
    }
    else
    {
        // Should not happen
    }


}

void ImageOpenGLWidget::updateImageTexture()
{
    /*
     * Refresh the image buffer
     * */

    if (!image.isDataRead())
    {
        return;
    }

    Matrix<size_t> local_ws(1, 2);
    local_ws[0] = 8;
    local_ws[1] = 8;

    Matrix<size_t> image_size(1, 2);
    image_size[0] = image.width();
    image_size[1] = image.height();

    Matrix<float> data_limit(1, 2);
    data_limit[0] = parameter[12];
    data_limit[1] = parameter[13];

    maintainImageTexture(image_size);

    if (mode == 0)
    {
        // Normal intensity
        scatteringDataToImage(image_data_corrected_cl, image_tex_cl, tsf_tex_cl, data_limit, image_size, local_ws, tsf_sampler, isLog);
    }

    if (mode == 1)
    {
        // Variance
        scatteringDataToImage(image_data_variance_cl, image_tex_cl, tsf_tex_cl, data_limit, image_size, local_ws, tsf_sampler, isLog);
    }
    else if (mode == 2)
    {
        // Skewness
        scatteringDataToImage(image_data_skewness_cl, image_tex_cl, tsf_tex_cl, data_limit, image_size, local_ws, tsf_sampler, isLog);
    }
    else
    {
        // Should not happen
    }
}

void ImageOpenGLWidget::maintainImageTexture(Matrix<size_t> &image_size)
{
    if ((image_size[0] != image_tex_size[0]) || (image_size[1] != image_tex_size[1]) || !isImageTexInitialized)
    {
        if (isImageTexInitialized)
        {
            err =  QOpenCLReleaseMemObject(image_tex_cl);

            if ( err != CL_SUCCESS)
            {
                qFatal(cl_error_cstring(err));
            }
        }

        glBindTexture(GL_TEXTURE_2D, image_tex_gl);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA32F,
            image_size[0],
            image_size[1],
            0,
            GL_RGBA,
            GL_FLOAT,
            NULL);
        glBindTexture(GL_TEXTURE_2D, 0);

        image_tex_size[0] = image_size[0];
        image_tex_size[1] = image_size[1];

        // Share the texture with the OpenCL runtime so that OpenCL can modify the texture
        image_tex_cl =  QOpenCLCreateFromGLTexture2D(context_cl.context(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, image_tex_gl, &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        isImageTexInitialized = true;
    }
}

QString ImageOpenGLWidget::integrationFrameString(DetectorFile &f, ImageInfo &image)
{
    Matrix<double> Q = getScatteringVector(f, image.selection().weighted_x(), image.selection().weighted_y());
    double value = 180 * getScatteringAngle(f, image.selection().weighted_x(), image.selection().weighted_y()) / pi;

    QString str;
    str += QString::number(image.selection().integral(), 'E') + " "
           + QString::number(image.selection().left()) + " "
           + QString::number(image.selection().top()) + " "
           + QString::number(image.selection().width()) + " "
           + QString::number(image.selection().height()) + " "
           + QString::number(image.selection().weighted_x(), 'E') + " "
           + QString::number(image.selection().weighted_y(), 'E') + " "
           + QString::number(Q[0], 'E') + " "
           + QString::number(Q[1], 'E') + " "
           + QString::number(Q[2], 'E') + " "
           + QString::number(vecLength(Q), 'E') + " "
           + QString::number(value, 'E') + " "
           + image.filePath() + "\n";
    return str;
}

void ImageOpenGLWidget::showImageTooltip(bool value)
{
    isImageTooltipActive = value;

    processScatteringDataProxy();
    updateImageTexture();

    Selection analysis_area = p_working_data[p_current_filepath].selection();
    processSelectionDataProxy(&analysis_area);
    p_working_data[p_current_filepath].setSelection(analysis_area);

    update();
}

void ImageOpenGLWidget::showEwaldCircle(bool value)
{
    isEwaldCircleActive = value;

    processScatteringDataProxy();
    updateImageTexture();

    Selection analysis_area = p_working_data[p_current_filepath].selection();
    processSelectionDataProxy(&analysis_area);
    p_working_data[p_current_filepath].setSelection(analysis_area);

    update();
}

void ImageOpenGLWidget::setCorrectionNoise(bool value)
{
    isCorrectionNoiseActive = (int) value;

    processScatteringDataProxy();
    updateImageTexture();

//    if (!p_set.isEmpty())
//    {
        Selection analysis_area = p_working_data[p_current_filepath].selection();
        processSelectionDataProxy(&analysis_area);
        p_working_data[p_current_filepath].setSelection(analysis_area);
//    }

    update();
}
void ImageOpenGLWidget::setCorrectionPlane(bool value)
{
    isCorrectionPlaneActive = (int) value;

    processScatteringDataProxy();
    updateImageTexture();

//    if (!p_set.isEmpty())
//    {
        Selection analysis_area = p_working_data[p_current_filepath].selection();
        processSelectionDataProxy(&analysis_area);
        p_working_data[p_current_filepath].setSelection(analysis_area);
//    }

    update();
}
void ImageOpenGLWidget::setCorrectionClutter(bool value)
{
    isCorrectionClutterActive = (int) value;

    processScatteringDataProxy();
    updateImageTexture();

//    if (!p_set.isEmpty())
//    {
        Selection analysis_area = p_working_data[p_current_filepath].selection();
        processSelectionDataProxy(&analysis_area);
        p_working_data[p_current_filepath].setSelection(analysis_area);
//    }

    update();
}
void ImageOpenGLWidget::setCorrectionMedian(bool value)
{
    isCorrectionMedianActive = (int) value;

    processScatteringDataProxy();
    updateImageTexture();

//    if (!p_set.isEmpty())
//    {
        Selection analysis_area = p_working_data[p_current_filepath].selection();
        processSelectionDataProxy(&analysis_area);
        p_working_data[p_current_filepath].setSelection(analysis_area);
//    }

    update();
}
void ImageOpenGLWidget::setCorrectionPolarization(bool value)
{
    isCorrectionPolarizationActive = (int) value;

    processScatteringDataProxy();
    updateImageTexture();

//    if (!p_set.isEmpty())
//    {
        Selection analysis_area = p_working_data[p_current_filepath].selection();
        processSelectionDataProxy(&analysis_area);
        p_working_data[p_current_filepath].setSelection(analysis_area);
//    }

    update();
}
void ImageOpenGLWidget::setCorrectionFlux(bool value)
{
    isCorrectionFluxActive = (int) value;

    processScatteringDataProxy();
    updateImageTexture();

//    if (!p_set.isEmpty())
//    {
        Selection analysis_area = p_working_data[p_current_filepath].selection();
        processSelectionDataProxy(&analysis_area);
        p_working_data[p_current_filepath].setSelection(analysis_area);
//    }

    update();
}
void ImageOpenGLWidget::setCorrectionExposure(bool value)
{
    isCorrectionExposureActive = (int) value;

    processScatteringDataProxy();
    updateImageTexture();

//    if (!p_set.isEmpty())
//    {
        Selection analysis_area = p_working_data[p_current_filepath].selection();
        processSelectionDataProxy(&analysis_area);
        p_working_data[p_current_filepath].setSelection(analysis_area);
//    }

    update();
}
void ImageOpenGLWidget::setCorrectionPixelProjection(bool value)
{
    isCorrectionPixelProjectionActive = (int) value;

    processScatteringDataProxy();
    updateImageTexture();

//    if (!p_set.isEmpty())
//    {
        Selection analysis_area = p_working_data[p_current_filepath].selection();
        processSelectionDataProxy(&analysis_area);
        p_working_data[p_current_filepath].setSelection(analysis_area);
//    }

    update();
}
void ImageOpenGLWidget::showTraceTexture(bool value)
{
    texture_number = (int) value;

    processScatteringDataProxy();
    updateImageTexture();

//    if (!p_set.isEmpty())
//    {
        Selection analysis_area = p_working_data[p_current_filepath].selection();
        processSelectionDataProxy(&analysis_area);
        p_working_data[p_current_filepath].setSelection(analysis_area);

        setSeriesTrace();
        setFrame();
//    }

    update();
}


void ImageOpenGLWidget::setLsqSamples(int value)
{
    n_lsq_samples = value;

    processScatteringDataProxy();
    updateImageTexture();

//    if (!p_set.isEmpty())
//    {
        Selection analysis_area = p_working_data[p_current_filepath].selection();
        processSelectionDataProxy(&analysis_area);
        p_working_data[p_current_filepath].setSelection(analysis_area);
//    }

    update();
}

void ImageOpenGLWidget::analyze(QString str)
{
    /* emit progressTaskActive(false);

    showTraceTexture(false);

    if (str == "undef")
    {
        setFrame();
        update();

        QString result;
        result += "# Analysis of single frame\n";
        result += "# " + QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm:ss t") + "\n";
        result += "#\n";
        result += "# (integral, origin x, origin y, width, height, weight x, weight y, Qx, Qy, Qz, |Q|, 2theta, background, origin x, origin y, width, height, path)\n";

        result += integrationFrameString(image, *p_set.current()->current());
        emit resultFinished(result);
    }
    else if (str == "Series")
    {

        Matrix<double> q_weightpoint(3, 1, 0);
        double x_weightpoint = 0;
        double y_weightpoint = 0;
        double angle_weightpoint = 0;
        double integral = 0;

        QString frames;
        p_set.current()->saveCurrentIndex();
        p_set.current()->begin();

        emit progressRangeChanged(0, p_set.current()->size() - 1);

        for (int i = 0; i < p_set.current()->size(); i++)
        {
            // Draw the frame and update the intensity OpenCL buffer prior to further operations
            setFrame();
            update();

            // Math
            double wt_x = p_working_data[p_current_filepath].selection().weighted_x();
            double wt_y = p_working_data[p_current_filepath].selection().weighted_y();
            double sum = p_working_data[p_current_filepath].selection().integral();

            x_weightpoint += sum * wt_x;
            y_weightpoint += sum * wt_y;
            angle_weightpoint += sum * (image.startAngle() + image.angleIncrement() * 0.5);

            Matrix<double> Q = getScatteringVector(image, wt_x, wt_y);

            q_weightpoint += sum * Q;

            integral += sum;

            frames += integrationFrameString(image, *p_set.current()->current());

            p_set.current()->next();

            emit progressChanged(i);
        }

        p_set.current()->loadSavedIndex();

        if (integral > 0)
        {
            q_weightpoint = q_weightpoint / integral;
            x_weightpoint = x_weightpoint / integral;
            y_weightpoint = y_weightpoint / integral;
            angle_weightpoint = angle_weightpoint / integral;
        }
        else
        {
            q_weightpoint.set(1, 3, 0);
            x_weightpoint = 0;
            y_weightpoint = 0;
            angle_weightpoint = 0;
        }

        QString result;
        result += "# Analysis of frames in series " + p_set.current()->path() + "\n";
        result += "# " + QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm:ss t") + "\n";
        result += "#\n";
        result += "# Sum of total integrated area in series " + QString::number(integral, 'E') + "\n";
        result += "# Weightpoint X:" + QString::number(x_weightpoint) + " Y:" + QString::number(y_weightpoint) + " Angle:" + QString::number(angle_weightpoint * 180.0 / pi) + "\n";
        result += "# Q weightpoint xyz " + QString::number(q_weightpoint[0], 'E') + " " + QString::number(q_weightpoint[1], 'E') + " " + QString::number(q_weightpoint[2], 'E') + " " + QString::number(vecLength(q_weightpoint), 'E') + "\n";
        result += "# Analysis of the individual frames (integral, origin x, origin y, width, height, weight x, weight y, Qx, Qy, Qz, |Q|, 2theta, background, origin x, origin y, width, height, path)\n";
        result += frames;

        emit resultFinished(result);
    }
    else if (str == "Set")
    {
        QStringList series_integral;
        QStringList series_xyangle_weightpoint;
        QStringList series_q_weightpoint;
        QStringList series_frames;
        QString str;

        double x_weightpoint = 0;
        double y_weightpoint = 0;
        double angle_weightpoint = 0;
        double integral = 0;

        p_set.saveCurrentIndex();
        p_set.begin();

        for (int i = 0; i < p_set.size(); i++)
        {
            p_set.current()->saveCurrentIndex();
            p_set.current()->begin();

            emit progressRangeChanged(0, p_set.current()->size() - 1);

            Matrix<double> q_weightpoint(3, 1, 0);

            for (int j = 0; j < p_set.current()->size(); j++)
            {
                // Draw the frame and update the intensity OpenCL buffer prior to further operations
                setFrame();
                update();

                // Math
                double wt_x = p_working_data[p_current_filepath].selection().weighted_x();
                double wt_y = p_working_data[p_current_filepath].selection().weighted_y();
                double sum = p_working_data[p_current_filepath].selection().integral();

                x_weightpoint += sum * wt_x;
                y_weightpoint += sum * wt_y;
                angle_weightpoint += sum * (image.startAngle() + image.angleIncrement() * 0.5);

                Matrix<double> Q = getScatteringVector(image, wt_x, wt_y);

                q_weightpoint += sum * Q;

                integral += sum;

                str += integrationFrameString(image, *p_set.current()->current());

                p_set.current()->next();

                emit progressChanged(j);
            }

            if (integral > 0)
            {
                q_weightpoint = q_weightpoint / integral;
                x_weightpoint = x_weightpoint / integral;
                y_weightpoint = y_weightpoint / integral;
                angle_weightpoint = angle_weightpoint / integral;
            }
            else
            {
                q_weightpoint.set(1, 3, 0);
                x_weightpoint = 0;
                y_weightpoint = 0;
                angle_weightpoint = 0;
            }

            series_q_weightpoint << QString::number(q_weightpoint[0], 'E') + " " + QString::number(q_weightpoint[1], 'E') + " " + QString::number(q_weightpoint[2], 'E') + " " + QString::number(vecLength(q_weightpoint), 'E') + "\n";

            series_xyangle_weightpoint << QString::number(x_weightpoint) + " " + QString::number(y_weightpoint) + " " + QString::number(angle_weightpoint * 180.0 / pi) + "\n";

            series_integral << QString(QString::number(integral, 'E') + "\n");
            integral = 0;

            series_frames << str;
            str.clear();

            p_set.current()->loadSavedIndex();
            p_set.next();
        }

        p_set.loadSavedIndex();
        p_set.current()->loadSavedIndex();

        QString result;

        result += "# Analysis of frames in several series\n";
        result += "# " + QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm:ss t") + "\n";
        result += "#\n";
        result += "# Sum of total integrated area in series\n";

        foreach (const QString &str, series_integral)
        {
            result += str;
        }

        result += "# Weightpoints in series (x, y, angle)\n";

        foreach (const QString &str, series_xyangle_weightpoint)
        {
            result += str;
        }

        result += "# Weightpoints in series (Qx, Qy, Qz, |Q|)\n";

        foreach (const QString &str, series_q_weightpoint)
        {
            result += str;
        }

        result += "# Analysis of the individual frames for each series (integral, origin x, origin y, width, height, weight x, weight y, Qx, Qy, Qz, |Q|, 2theta, background, origin x, origin y, width, height, path)\n";

        for (int i = 0; i < series_integral.size(); i++)
        {
            result += "# Folder integral " + series_integral.at(i);
            result += series_frames.at(i);
        }

        emit resultFinished(result);
    }

    emit progressTaskActive(true);*/
}


void ImageOpenGLWidget::applyPlaneMarker(QString str)
{
//    if (!p_set.isEmpty())
    {
        if (str == "All with this path")
        {
//            p_set.current()->setPlaneMarker(p_working_data[p_current_filepath].planeMarker());
        }
        else if (str == "All")
        {
//            p_set.setPlaneMarker(p_working_data[p_current_filepath].planeMarker());
        }
    }
}

Matrix<double> ImageOpenGLWidget::getPlane()
{
    if (!isCorrectionPlaneActive)
    {
        return Matrix<double>(4,1,0);
    }

    QList<Selection> marker = p_working_data[p_current_filepath].planeMarker();

    // Compute sample values
    for (int i = 0; i < n_lsq_samples; i++)
    {
        // Intensity average under the marker
        Matrix<size_t> buffer_origin(1, 3, 0);
        buffer_origin[0] = marker.at(i).x() * sizeof(float); // In bytes (see comment below)
        buffer_origin[1] = marker.at(i).y(); // In units
        Matrix<size_t> host_origin(1, 3, 0);
        Matrix<size_t> region(1, 3, 1);
        region[0] = marker.at(i).width() * sizeof(float);
        region[1] = marker.at(i).height(); // The 1.1 OpenCL doc is unclear on this, but based on how slice pitches are calculated region[1] should not be in bytes, but elements

        Matrix<float> marker_buf(marker.at(i).height(), marker.at(i).width()); // Too small in comparison to region

        err =   QOpenCLEnqueueReadBufferRect ( context_cl.queue(),
                                               image_data_raw_cl,
                                               CL_TRUE,
                                               buffer_origin.data(),
                                               host_origin.data(),
                                               region.data(),
                                               image_buffer_size[0] * sizeof(float),
                                               image_buffer_size[0] * image_buffer_size[1] * sizeof(float),
                                               marker_buf.n() * sizeof(float),
                                               marker_buf.m() * marker_buf.n() * sizeof(float),
                                               marker_buf.data(),
                                               0, NULL, NULL);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        marker[i].setSum(marker_buf.sum());
    }

    p_working_data[p_current_filepath].setPlaneMarker(marker);


    // Create LSQ matrix
    double xx = 0, yy = 0, xy = 0, zx = 0, zy = 0, x = 0, y = 0, z = 0;

    for (int i = 0; i < n_lsq_samples; i++)
    {
        double x_val = marker[i].center().x();
        double y_val = marker[i].center().y();
        double z_val = marker[i].integral() / (marker[i].width() * marker[i].height());

        xx += x_val * x_val;
        xy += x_val * y_val;
        yy += y_val * y_val;
        zx += z_val * x_val;
        zy += z_val * y_val;
        x += x_val;
        y += y_val;
        z += z_val;
    }

    Matrix<double> A(3, 3);
    Matrix<double> B(3, 1);
    Matrix<double> C(3, 1);

    A[0] = xx;
    A[1] = xy;
    A[2] = x;

    A[3] = xy;
    A[4] = yy;
    A[5] = y;

    A[6] = x;
    A[7] = y;
    A[8] = (float) n_lsq_samples;

    C[0] = -zx;
    C[1] = -zy;
    C[2] = -z;

    B = A.inverse() * C;

    // The error
    double err = 0;

    for (int i = 0; i < n_lsq_samples; i++)
    {
        double x_val = marker[i].center().x();
        double y_val = marker[i].center().y();
        double z_val = marker[i].integral() / (marker[i].width() * marker[i].height());

        err += pow(z_val + B[0] * x_val + B[1] * y_val + B[2], 2);
    }

    err /= (float) n_lsq_samples;

    B.resize(4, 1);
    B[3] = 1;
    B[2] = B[3] / B[2];
    B[1] = B[1] * B[2];
    B[0] = B[0] * B[2];

    return B;
}

void ImageOpenGLWidget::applySelection()
{
    QFileInfo info(p_current_filepath);
    QSqlQuery query(QSqlDatabase::database());

    // Prepare query
    if (p_application_mode == "All with this path")
    {
        query.prepare("SELECT * FROM cbf_table WHERE Path = :Path ORDER BY FilePath ASC");
        query.bindValue(":Path", info.path());
    }
    else if (p_application_mode == "All")
    {
        query.prepare("SELECT * FROM cbf_table");
    }
    else
    {
        qDebug() << "Unrecognized application mode:" << p_application_mode;
    }

    if (!query.exec()) qDebug() << sqlQueryError(query);

    // Apply selection
    while (query.next())
    {
        QString path = query.value(0).toString();
        if (!p_working_data.contains(path)) p_working_data[path] = ImageInfo(path);
        p_working_data[path].setSelection(p_working_data[p_current_filepath].selection());
    }
}

void ImageOpenGLWidget::setApplicationMode(QString str)
{
    p_application_mode = str;
}

void ImageOpenGLWidget::setFilePath(QString str)
{
//    qDebug() << str;

    p_current_filepath = str;

    if (!p_working_data.contains(str)) p_working_data[str] = ImageInfo(str);

    setFrame();

    update();
}

void ImageOpenGLWidget::setSeriesTrace()
{
    /*if (!isCLInitialized || !isGLInitialized)
    {
        return;
    }

    err =  QOpenCLReleaseMemObject(image_data_trace_cl);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    image_data_trace_cl =  QOpenCLCreateBuffer( context_cl.context(),
                           CL_MEM_COPY_HOST_PTR,
                           set_trace[p_set.i()].bytes(),
                           set_trace[p_set.i()].data(),
                           &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    update();*/
}

void ImageOpenGLWidget::showWeightCenter(bool value)
{
    isWeightCenterActive = value;
}

void ImageOpenGLWidget::processSelectionData(Selection * area, cl_mem image_data_cl, cl_mem image_pos_weight_x_cl_new, cl_mem image_pos_weight_y_cl_new, Matrix<size_t> &image_size, Matrix<size_t> &local_ws)
{
    /*
     * When an image is processed by the imagepreview kernel, it saves data into GPU buffers that can be used
     * for further calculations. This functions copies data from these buffers into smaller buffers depending
     * on the selected area. The buffers are then summed, effectively doing operations such as integration
     * */

    if (!image.isDataRead())
    {
        return;
    }

    // Set the size of the cl buffer that will be used to store the data in the marked selection. The padded size is neccessary for the subsequent parallel reduction
    int selection_read_size = area->width() * area->height();
    int selection_local_size = local_ws[0] * local_ws[1];
    int selection_global_size = selection_read_size + (selection_read_size % selection_local_size ? selection_local_size - (selection_read_size % selection_local_size) : 0);
    int selection_padded_size = selection_global_size + selection_global_size / selection_local_size;

    if (selection_read_size <= 0)
    {
        return;
    }

    if (0)
    {
        qDebug() << *area;
        qDebug() << selection_read_size;
        qDebug() << selection_local_size;
        qDebug() << selection_global_size;
        qDebug() << selection_padded_size;
    }

    // Copy a chunk of GPU memory for further calculations
    local_ws[0] = 64;
    local_ws[1] = 1;

    Matrix<size_t> buffer_origin(1, 3);
    buffer_origin[0] = area->left() * sizeof(cl_float);
    buffer_origin[1] = area->top();
    buffer_origin[2] = 0;

    Matrix<size_t> copy_origin(1, 3);
    copy_origin[0] = 0;
    copy_origin[1] = 0;
    copy_origin[2] = 0;

    Matrix<size_t> region(1, 3);
    region[0] = area->width() * sizeof(cl_float);
    region[1] = area->height();
    region[2] = 1;


    // Prepare buffers to put data into that coincides with the selected area
    cl_mem selection_intensity_cl =  QOpenCLCreateBuffer( context_cl.context(),
                                     CL_MEM_ALLOC_HOST_PTR,
                                     selection_padded_size * sizeof(cl_float),
                                     NULL,
                                     &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_mem selection_pos_weight_x_cl =  QOpenCLCreateBuffer( context_cl.context(),
                                        CL_MEM_ALLOC_HOST_PTR,
                                        selection_padded_size * sizeof(cl_float),
                                        NULL,
                                        &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_mem selection_pos_weight_y_cl =  QOpenCLCreateBuffer( context_cl.context(),
                                        CL_MEM_ALLOC_HOST_PTR,
                                        selection_padded_size * sizeof(cl_float),
                                        NULL,
                                        &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Transfer data to above buffers
    err = QOpenCLEnqueueCopyBufferRect ( 	context_cl.queue(),
        image_data_cl,
        selection_intensity_cl,
        buffer_origin.data(),
        copy_origin.data(),
        region.data(),
        image_size[0] * sizeof(cl_float),
        0,
        area->width() * sizeof(cl_float),
        0,
        0,
        NULL,
        NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }


    err = QOpenCLEnqueueCopyBufferRect ( 	context_cl.queue(),
        image_pos_weight_x_cl_new,
        selection_pos_weight_x_cl,
        buffer_origin.data(),
        copy_origin.data(),
        region.data(),
        image_size[0] * sizeof(cl_float),
        0,
        area->width() * sizeof(cl_float),
        0,
        0,
        NULL,
        NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLEnqueueCopyBufferRect ( 	context_cl.queue(),
        image_pos_weight_y_cl_new,
        selection_pos_weight_y_cl,
        buffer_origin.data(),
        copy_origin.data(),
        region.data(),
        image_size[0] * sizeof(cl_float),
        0,
        area->width() * sizeof(cl_float),
        0,
        0,
        NULL,
        NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }


    err =   QOpenCLFinish(context_cl.queue());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }


    local_ws[0] = local_ws[0] * local_ws[1];
    local_ws[1] = 1;

    // Do parallel reduction of the chunks and save the results
    area->setSum(sumGpuArray(selection_intensity_cl, selection_read_size, local_ws));

    if (area->integral() > 0)
    {
        area->setWeightedX(sumGpuArray(selection_pos_weight_x_cl, selection_read_size, local_ws) / area->integral());
        area->setWeightedY(sumGpuArray(selection_pos_weight_y_cl, selection_read_size, local_ws) / area->integral());
    }
    else
    {
        area->setWeightedX(0);
        area->setWeightedY(0);
    }

    err =  QOpenCLReleaseMemObject(selection_intensity_cl);
    err |=  QOpenCLReleaseMemObject(selection_pos_weight_x_cl);
    err |=  QOpenCLReleaseMemObject(selection_pos_weight_y_cl);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }
}


void ImageOpenGLWidget::initializeCL()
{
    initializeOpenCLFunctions();

    // Set the OpenCL context
    context_cl.initDevices();
    context_cl.initSharedContext();
    context_cl.initCommandQueue();

//    imageWorker->setOpenCLContext(&context_cl);

    // Build programs from OpenCL kernel source
    QStringList paths;
    paths << "kernels/scattering_data_operations.cl";
    paths << "kernels/parallel_reduction.cl";

    context_cl.createProgram(paths, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    context_cl.buildProgram("-Werror -cl-std=CL1.2");

    // Kernel handles
    cl_data_to_image =  QOpenCLCreateKernel(context_cl.program(), "scatteringDataToImage", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_process_data =  QOpenCLCreateKernel(context_cl.program(), "processScatteringData", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_project_data = QOpenCLCreateKernel(context_cl.program(), "projectScatteringData", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

//    cl_rect_copy_float = QOpenCLCreateKernel(context_cl.program(), "rectCopyFloat", &err);

//    if ( err != CL_SUCCESS)
//    {
//        qFatal(cl_error_cstring(err));
//    }

    cl_parallel_reduction = QOpenCLCreateKernel(context_cl.program(), "parallelReduction", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Image sampler
    image_sampler =  QOpenCLCreateSampler(context_cl.context(), false, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Tsf sampler
    tsf_sampler =  QOpenCLCreateSampler(context_cl.context(), true, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Parameters
    parameter_cl =  QOpenCLCreateBuffer(context_cl.context(),
                                        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                        parameter.bytes(),
                                        NULL, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Image buffers
    image_data_raw_cl =  QOpenCLCreateBuffer( context_cl.context(),
                         CL_MEM_ALLOC_HOST_PTR,
                         image_buffer_size[0] * image_buffer_size[1] * sizeof(cl_float),
                         NULL,
                         &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    image_data_trace_cl =  QOpenCLCreateBuffer( context_cl.context(),
                           CL_MEM_ALLOC_HOST_PTR,
                           image_buffer_size[0] * image_buffer_size[1] * sizeof(cl_float),
                           NULL,
                           &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    image_data_corrected_cl =  QOpenCLCreateBuffer( context_cl.context(),
                               CL_MEM_ALLOC_HOST_PTR,
                               image_buffer_size[0] * image_buffer_size[1] * sizeof(cl_float),
                               NULL,
                               &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    image_data_variance_cl =  QOpenCLCreateBuffer( context_cl.context(),
                              CL_MEM_ALLOC_HOST_PTR,
                              image_buffer_size[0] * image_buffer_size[1] * sizeof(cl_float),
                              NULL,
                              &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    image_data_skewness_cl =  QOpenCLCreateBuffer( context_cl.context(),
                              CL_MEM_ALLOC_HOST_PTR,
                              image_buffer_size[0] * image_buffer_size[1] * sizeof(cl_float),
                              NULL,
                              &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    image_data_weight_x_cl =  QOpenCLCreateBuffer( context_cl.context(),
                              CL_MEM_ALLOC_HOST_PTR,
                              image_buffer_size[0] * image_buffer_size[1] * sizeof(cl_float),
                              NULL,
                              &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    image_data_weight_y_cl =  QOpenCLCreateBuffer( context_cl.context(),
                              CL_MEM_ALLOC_HOST_PTR,
                              image_buffer_size[0] * image_buffer_size[1] * sizeof(cl_float),
                              NULL,
                              &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    image_data_generic_cl =  QOpenCLCreateBuffer( context_cl.context(),
                             CL_MEM_ALLOC_HOST_PTR,
                             image_buffer_size[0] * image_buffer_size[1] * sizeof(cl_float) * 2, // *2 so it can be used for parallel reduction
                             NULL,
                             &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // BG buffer
    Matrix<size_t> region(1, 3);
    region[0] = 16;
    region[1] = 16;
    region[2] = 16;

    cl_image_format format_3Dimg;
    format_3Dimg.image_channel_order = CL_INTENSITY;
    format_3Dimg.image_channel_data_type = CL_FLOAT;

    series_interpol_gpu_3Dimg = QOpenCLCreateImage3D ( context_cl.context(),
                                CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                &format_3Dimg,
                                region[0],
                                region[1],
                                region[2],
                                0,
                                0,
                                NULL,
                                &err);
    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    isCLInitialized = true;

    setParameter(parameter);
}

void ImageOpenGLWidget::setTsf(TransferFunction &tsf)
{
    if (!isCLInitialized || !isGLInitialized)
    {
        return;
    }

    if (isTsfTexInitialized)
    {
        err =  QOpenCLReleaseMemObject(tsf_tex_cl);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }

    // Buffer for tsf_tex_gl
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

    isTsfTexInitialized = true;

    tsf_tex_cl =  QOpenCLCreateFromGLTexture2D(context_cl.context(), CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, tsf_tex_gl, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }
}

void ImageOpenGLWidget::setRgb(QString style)
{
//    rgb_style = str;

    tsf.setRgb(style);
    tsf.setSpline(256);

    setTsf(tsf);

    updateImageTexture();

    update();
}
void ImageOpenGLWidget::setAlpha(QString style)
{
    tsf.setAlpha(style);
    tsf.setSpline(256);

    setTsf(tsf);

    updateImageTexture();

    update();
}
void ImageOpenGLWidget::setLog(bool value)
{
    isLog = (int) value;

    updateImageTexture();

    update();
}

void ImageOpenGLWidget::setDataMin(double value)
{
    parameter[12] = value;
    setParameter(parameter);

    updateImageTexture();

    update();
}
void ImageOpenGLWidget::setDataMax(double value)
{
    parameter[13] = value;
    setParameter(parameter);

    updateImageTexture();

    update();
}

void ImageOpenGLWidget::setNoise(double value)
{
    parameter[0] = value;
    setParameter(parameter);

    processScatteringDataProxy();
    updateImageTexture();

//    if (!p_set.isEmpty())
    {
        Selection analysis_area = p_working_data[p_current_filepath].selection();
        processSelectionDataProxy(&analysis_area);
        p_working_data[p_current_filepath].setSelection(analysis_area);
    }

    update();
}

void ImageOpenGLWidget::beginRawGLCalls(QPainter * painter)
{
    painter->beginNativePainting();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
}

void ImageOpenGLWidget::endRawGLCalls(QPainter * painter)
{
    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    painter->endNativePainting();
}

void ImageOpenGLWidget::drawImage(QRectF rect, GLuint texture, QPainter * painter)
{
    std_2d_sprite_program->bind();

    std_2d_sprite_program->setUniformValue(std_2d_sprite_texture, 0);

    GLfloat texpos[] =
    {
        0.0, 0.0,
        1.0, 0.0,
        1.0, 1.0,
        0.0, 1.0
    };

    GLuint indices[] = {0, 1, 3, 1, 2, 3};

    texture_view_matrix = zoom_matrix * translation_matrix;

    glUniformMatrix4fv(std_2d_sprite_transform, 1, GL_FALSE, texture_view_matrix.colmajor().toFloat().data());

    glEnableVertexAttribArray(std_2d_sprite_fragpos);
    glEnableVertexAttribArray(std_2d_sprite_pos);

    Matrix<GLfloat> fragpos;

    // Draw image
    glBindTexture(GL_TEXTURE_2D, texture);
    fragpos = glRect(rect);
    glVertexAttribPointer(std_2d_sprite_fragpos, 2, GL_FLOAT, GL_FALSE, 0, fragpos.data());
    glVertexAttribPointer(std_2d_sprite_pos, 2, GL_FLOAT, GL_FALSE, 0, texpos);

    glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);

    glDisableVertexAttribArray(std_2d_sprite_pos);
    glDisableVertexAttribArray(std_2d_sprite_fragpos);

    std_2d_sprite_program->release();
}

Matrix<GLfloat> ImageOpenGLWidget::glRect(QRectF &qt_rect)
{
    Matrix<GLfloat> gl_rect(1, 8);

    qreal x, y, w, h;
    qreal xf, yf, wf, hf;
    qt_rect = qt_rect.normalized();
    qt_rect.getRect(&x, &y, &w, &h);

    xf = (x / (qreal) this->width()) * 2.0 - 1.0;
    yf = 1.0 - (y + h) / (qreal) this->height() * 2.0;
    wf = (w / (qreal) this->width()) * 2.0;
    hf = (h / (qreal) this->height()) * 2.0;

    gl_rect[0] = (GLfloat) xf;
    gl_rect[1] = (GLfloat) yf;
    gl_rect[2] = (GLfloat) xf + wf;
    gl_rect[3] = (GLfloat) yf;
    gl_rect[4] = (GLfloat) xf + wf;
    gl_rect[5] = (GLfloat) yf + hf;
    gl_rect[6] = (GLfloat) xf;
    gl_rect[7] = (GLfloat) yf + hf;

    return gl_rect;
}

void ImageOpenGLWidget::centerImage(QSizeF size)
{
//    qDebug() << size << this->size();

    // Center an image using matrices that operate on the GL space
    translation_matrix[3] =  - 2.0 * (0.5 * size.width() / ( (qreal) this->width()));
    translation_matrix[7] =  2.0 * (0.5 * size.height() / ( (qreal) this->height()));

    zoom_matrix[0] = std::min((qreal) this->width() / size.width(), (qreal) this->height() / size.height());
    zoom_matrix[5] = zoom_matrix[0];
    zoom_matrix[10] = zoom_matrix[0];

    update();
}

void ImageOpenGLWidget::centerCurrentImage()
{
    if (image.isDataRead()) centerImage(image.size());
    else centerImage(QSizeF(texture_noimage->width(),texture_noimage->height()));
}

void ImageOpenGLWidget::drawSelection(Selection area, QPainter * painter, Matrix<float> &color, QPointF offset)
{
    //    glLineWidth(2.0);

    float selection_left = (((qreal) area.left() + 0.5 * this->width() + offset.x()) / (qreal) this->width()) * 2.0 - 1.0; // Left
    float selection_right = (((qreal) area.x() + area.width()  + 0.5 * this->width() + offset.x()) / (qreal) this->width()) * 2.0 - 1.0; // Right

    float selection_top = (1.0 - (qreal) (area.top() + 0.5 * this->height() + offset.y()) / (qreal) this->height()) * 2.0 - 1.0; // Top
    float selection_bot = (1.0 - (qreal) (area.y() + area.height() + 0.5 * this->height() + offset.y()) / (qreal) this->height()) * 2.0 - 1.0; // Bottom

    float frame_left = (((qreal) - 4 + 0.5 * this->width() + offset.x()) / (qreal) this->width()) * 2.0 - 1.0; // Left
    float frame_right = (((qreal) image.width() + 4  + 0.5 * this->width() + offset.x()) / (qreal) this->width()) * 2.0 - 1.0; // Right

    float frame_top = (1.0 - (qreal) (-4 + 0.5 * this->height() + offset.y()) / (qreal) this->height()) * 2.0 - 1.0; // Top
    float frame_bot = (1.0 - (qreal) (image.height() + 4 + 0.5 * this->height() + offset.y()) / (qreal) this->height()) * 2.0 - 1.0; // Bottom

    // Points
    Matrix<GLfloat> point(8, 2);
    point[0] = frame_left;
    point[1] = frame_top;
    point[2] = frame_left;
    point[3] = frame_bot;

    point[4] = frame_right;
    point[5] = frame_top;
    point[6] = frame_right;
    point[7] = frame_bot;

    point[8] = selection_left;
    point[9] = selection_top;
    point[10] = selection_left;
    point[11] = selection_bot;

    point[12] = selection_right;
    point[13] = selection_top;
    point[14] = selection_right;
    point[15] = selection_bot;

    setVbo(selections_vbo[0], point.data(), point.size(), GL_DYNAMIC_DRAW);

    beginRawGLCalls(painter);

    std_2d_col_program->bind();
    glEnableVertexAttribArray(std_2d_col_fragpos);

    glUniform4fv(std_2d_col_color, 1, color.data());

    glBindBuffer(GL_ARRAY_BUFFER, selections_vbo[0]);
    glVertexAttribPointer(std_2d_col_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    texture_view_matrix = zoom_matrix * translation_matrix;

    glUniformMatrix4fv(std_2d_col_transform, 1, GL_FALSE, texture_view_matrix.colmajor().toFloat().data());


    //    GLuint indices[] = {0,1,3,2,0, 4,6,7,5,4,0};
    GLuint indices[] = {0, 1, 4, 1, 4, 5, 1, 3, 5, 3, 5, 7, 2, 3, 7, 2, 6, 7, 0, 2, 6, 0, 4, 6};
    glDrawElements(GL_TRIANGLES,  3 * 8, GL_UNSIGNED_INT, indices);

    glDisableVertexAttribArray(std_2d_col_fragpos);

    std_2d_col_program->release();

    endRawGLCalls(painter);
}

void ImageOpenGLWidget::setVbo(GLuint vbo, float * buf, size_t length, GLenum usage)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT)*length, buf, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


//void ImagePreviewWorker::drawPlaneMarker(QList<Selection> marker, QPainter *painter, QPoint offset)
//{
//    beginRawGLCalls(painter);

//    std_2d_col_program->bind();

//    glEnableVertexAttribArray(std_2d_col_fragpos);

//    for (int i = 0; i < n_lsq_samples; i++)
//    {
//        float selection_left = (((qreal) marker[i].left() + offset.x() + 0.5*this->width()) / (qreal) this->width()) * 2.0 - 1.0; // Left
//        float selection_right = (((qreal) marker[i].x() + offset.x() + marker[i].width()  + 0.5*this->width())/ (qreal) this->width()) * 2.0 - 1.0; // Right

//        float selection_top = (1.0 - (qreal) (marker[i].top() + offset.y() + 0.5*this->height())/ (qreal) this->height()) * 2.0 - 1.0; // Top
//        float selection_bot = (1.0 - (qreal) (marker[i].y() + offset.y() + marker[i].height() + 0.5*this->height())/ (qreal) this->height()) * 2.0 - 1.0; // Bottom

//        // Points
//        Matrix<GLfloat> point(4,2);
//        point[0] = selection_left;
//        point[1] = selection_top;
//        point[2] = selection_left;
//        point[3] = selection_bot;

//        point[4] = selection_right;
//        point[5] = selection_top;
//        point[6] = selection_right;
//        point[7] = selection_bot;

//        setVbo(selections_vbo[0], point.data(), point.size(), GL_DYNAMIC_DRAW);

//        ColorMatrix<float> color(0,0,0,0.9);

//        glUniform4fv(std_2d_col_color, 1, color.data());

//        glBindBuffer(GL_ARRAY_BUFFER, selections_vbo[0]);
//        glVertexAttribPointer(std_2d_col_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
//        glBindBuffer(GL_ARRAY_BUFFER, 0);

//        texture_view_matrix = zoom_matrix*translation_matrix;

//        glUniformMatrix4fv(std_2d_col_transform, 1, GL_FALSE, texture_view_matrix.colmajor().toFloat().data());

//        GLuint indices[] = {0,1,2, 1,2,3};
//        glDrawElements(GL_TRIANGLES,  3*2, GL_UNSIGNED_INT, indices);
//    }

//    glDisableVertexAttribArray(std_2d_col_fragpos);

//    std_2d_col_program->release();

//    endRawGLCalls(painter);
//}

void ImageOpenGLWidget::drawWeightpoint(Selection area, QPainter * painter)
{
    ColorMatrix<float> color(1.0, 1.0, 0.0, 0.8);

    // Change to draw a faded polygon
    glLineWidth(1.5);

    float x0 = (((qreal) area.left() + 0.5 * this->width()) / (qreal) this->width()) * 2.0 - 1.0; // Left
    float x2 = (((qreal) area.x() + area.width()  + 0.5 * this->width()) / (qreal) this->width()) * 2.0 - 1.0; // Right
    float x1 = (((qreal) area.weighted_x()  + 0.5 * this->width()) / (qreal) this->width()) * 2.0 - 1.0; // Center

    float y0 = (1.0 - (qreal) (area.top() + 0.5 * this->height()) / (qreal) this->height()) * 2.0 - 1.0; // Top
    float y2 = (1.0 - (qreal) (area.y() + area.height() + 0.5 * this->height()) / (qreal) this->height()) * 2.0 - 1.0; // Bottom
    float y1 = (1.0 - (qreal) (area.weighted_y() + 0.5 * this->height()) / (qreal) this->height()) * 2.0 - 1.0; // Center

    float x_offset = (x2 - x0) * 0.02;
    float y_offset = (y2 - y0) * 0.02;


    Matrix<GLfloat> selection_lines(8, 2);
    selection_lines[0] = x0;
    selection_lines[1] = y1;
    selection_lines[2] = x1 - x_offset;
    selection_lines[3] = y1;

    selection_lines[4] = x1 + x_offset;
    selection_lines[5] = y1;
    selection_lines[6] = x2;
    selection_lines[7] = y1;

    selection_lines[8] = x1;
    selection_lines[9] = y1 - y_offset;
    selection_lines[10] = x1;
    selection_lines[11] = y0;

    selection_lines[12] = x1;
    selection_lines[13] = y1 + y_offset;
    selection_lines[14] = x1;
    selection_lines[15] = y2 ;



    setVbo(weightpoints_vbo[0], selection_lines.data(), selection_lines.size(), GL_DYNAMIC_DRAW);

    beginRawGLCalls(painter);

    std_2d_col_program->bind();
    glEnableVertexAttribArray(std_2d_col_fragpos);

    glUniform4fv(std_2d_col_color, 1, color.data());

    glBindBuffer(GL_ARRAY_BUFFER, weightpoints_vbo[0]);
    glVertexAttribPointer(std_2d_col_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    texture_view_matrix = zoom_matrix * translation_matrix;

    glUniformMatrix4fv(std_2d_col_transform, 1, GL_FALSE, texture_view_matrix.colmajor().toFloat().data());

    glDrawArrays(GL_LINES,  0, 8);

    glDisableVertexAttribArray(std_2d_col_fragpos);

    std_2d_col_program->release();

    endRawGLCalls(painter);
}

Matrix<double> ImageOpenGLWidget::getScatteringVector(DetectorFile &f, double x, double y)
{
    // Assumes that the incoming ray is parallel to the z axis.

    double k = 1.0f / f.wavelength(); // Multiply with 2pi if desired

    Matrix<double> k_i(3, 1, 0);
    k_i[0] = -k;

    Matrix<double> k_f(3, 1, 0);
    k_f[0] =    -f.detectorDist();
    k_f[1] =    f.pixSizeX() * ((double) (f.height() - y - 0.5) - (isBeamOverrideActive ? f.beamX() + beam_x_override : f.beamX())); /* DANGER */
//    k_f[2] =    f.pixSizeY() * ((double) (f.width() - x - 0.5) - (isBeamOverrideActive ? beam_y_override : f.beamY())); /* DANGER */
    k_f[2] =    f.pixSizeY() * -((double) (x + 0.5) - (isBeamOverrideActive ? f.beamY() + beam_y_override : f.beamY())); /* DANGER */

    k_f = k * vecNormalize(k_f);

    Matrix<double> Q = k_f - k_i;

    return Q;
}

double ImageOpenGLWidget::getScatteringAngle(DetectorFile &f, double x, double y)
{
    // Assumes that the incoming ray is parallel to the z axis.

    double k = 1.0f / f.wavelength(); // Multiply with 2pi if desired

    Matrix<double> k_i(1, 3, 0);
    k_i[0] = -k;

    Matrix<double> k_f(1, 3, 0);
    k_f[0] =    -f.detectorDist();
    k_f[1] =    f.pixSizeX() * ((double) (f.height() - y - 0.5) - (isBeamOverrideActive ? f.beamX() + beam_x_override : f.beamX())); /* DANGER */
//    k_f[2] =    f.pixSizeY() * ((double) (f.width() - x - 0.5) - (isBeamOverrideActive ? beam_y_override : f.beamY())); /* DANGER */
    k_f[2] =    f.pixSizeY() * -((double) (x + 0.5) - (isBeamOverrideActive ? f.beamY() + beam_y_override : f.beamY())); /* DANGER */

    k_f = k * vecNormalize(k_f);

    return acos(vecDot(k_f, k_i) / (k * k));
}

void ImageOpenGLWidget::drawPixelToolTip(QPainter * painter)
{
    //Position
    Matrix<double> screen_pixel_pos(4, 1, 0); // Uses GL coordinates
    screen_pixel_pos[0] = 2.0 * (double) pos.x() / (double) this->width() - 1.0;
    screen_pixel_pos[1] = 2.0 * (1.0 - (double) pos.y() / (double) this->height()) - 1.0;
    screen_pixel_pos[2] = 0;
    screen_pixel_pos[3] = 1.0;

    Matrix<double> image_pixel_pos(4, 1); // Uses GL coordinates

    image_pixel_pos = texture_view_matrix.inverse4x4() * screen_pixel_pos;

    double pixel_x = image_pixel_pos[0] * this->width() * 0.5;
    double pixel_y = - image_pixel_pos[1] * this->height() * 0.5;

    double pixel_x_bounded = pixel_x;
    double pixel_y_bounded = pixel_y;

    if (pixel_x < 0)
    {
        pixel_x_bounded = 0;
    }

    if (pixel_y < 0)
    {
        pixel_y_bounded = 0;
    }

    if (pixel_x >= image.width())
    {
        pixel_x_bounded = image.width() - 1;
    }

    if (pixel_y >= image.height())
    {
        pixel_y_bounded = image.height() - 1;
    }

    // Intensity
    float value = 0;

    if (mode == 0)
    {
        err =   QOpenCLEnqueueReadBuffer ( context_cl.queue(),
                                           image_data_corrected_cl,
                                           CL_TRUE,
                                           ((int) pixel_y_bounded * image.width() + (int) pixel_x_bounded) * sizeof(cl_float),
                                           sizeof(cl_float),
                                           &value,
                                           0, NULL, NULL);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }
    else if (mode == 1)
    {
        err =   QOpenCLEnqueueReadBuffer ( context_cl.queue(),
                                           image_data_variance_cl,
                                           CL_TRUE,
                                           ((int) pixel_y_bounded * image.width() + (int) pixel_x_bounded) * sizeof(cl_float),
                                           sizeof(cl_float),
                                           &value,
                                           0, NULL, NULL);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }
    else if (mode == 2)
    {
        err =   QOpenCLEnqueueReadBuffer ( context_cl.queue(),
                                           image_data_skewness_cl,
                                           CL_TRUE,
                                           ((int) pixel_y_bounded * image.width() + (int) pixel_x_bounded) * sizeof(cl_float),
                                           sizeof(cl_float),
                                           &value,
                                           0, NULL, NULL);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }

    // Q vector
    Matrix<double> Q = getScatteringVector(image, pixel_x, pixel_y);

    // G vector, de-rotated
    image.setAlpha(0.8735582);
    image.setBeta(0.000891863);

    RotationMatrix<double> PHI;
    RotationMatrix<double> KAPPA;
    RotationMatrix<double> OMEGA;

    PHI.setArbRotation(image.beta(), 0, -(image.phi() + offset_phi));
    KAPPA.setArbRotation(image.alpha(), 0, -(image.kappa() + offset_kappa));
    OMEGA.setZRotation(-(image.omega() + offset_omega));

    RotationMatrix<double> sampleRotMat;
    sampleRotMat = PHI * KAPPA * OMEGA;

    Matrix<double> G = sampleRotMat.to3x3() * Q;

//    qDebug() << "Phi" << 90 - atan2(G[0],G[2])*180/pi;

    QString tip;
    tip += "<p style=\"font-family: monospace, times, serif; font-size:10pt; font-weight:normal; color:white\">";
    tip += "<font color=\"white\">Value [" + QString::number((int) pixel_x_bounded) + ", " + QString::number((int) pixel_y_bounded) + "</font>";
    tip += "] <font color=\"#85DAFF\">" + QString::number(value) + "</font><br/>";
    tip += "Q[<font color=\"#85DAFF\">" + QString::number(Q[0], 'f', 2) + "</font>, <font color=\"#85DAFF\">" + QString::number(Q[1], 'f', 2) + "</font>, <font color=\"#85DAFF\">" + QString::number(Q[2], 'f', 2) + "</font>]";
    tip += ", G<sub>rot</sub>[<font color=\"#85DAFF\">" + QString::number(G[0], 'f', 2) + "</font>, <font color=\"#85DAFF\">" + QString::number(G[1], 'f', 2) + "</font>, <font color=\"#85DAFF\">" + QString::number(G[2], 'f', 2) + "</font>]";
    tip += " (<font color=\"#85DAFF\">" + QString::number(vecLength(Q), 'f', 2) + "</font> Å<sup>-1</sup>) (<font color=\"#85DAFF\">" + QString::number(1.0 / vecLength(Q), 'f', 2) + "</font> Å)<br/>";
    tip += "2&theta;: <font color=\"#85DAFF\">" + QString::number(180 * getScatteringAngle(image, pixel_x, pixel_y) / pi, 'f', 2) + "</font>&deg;";
    tip += ", Sum: <font color=\"#85DAFF\">" + QString::number(p_working_data[p_current_filepath].selection().integral(), 'f', 2) + "</font>";
    tip += ", Center of mass: <font color=\"#85DAFF\">" + QString::number(p_working_data[p_current_filepath].selection().weighted_x(), 'f', 2) + "</font>, <font color=\"#85DAFF\">" + QString::number(p_working_data[p_current_filepath].selection().weighted_y(), 'f', 2) + "</font>";
    tip += "</p>";

    // Prepare painter
    QFont font("Monospace", 10);
    QFontMetrics fm(font);

    QBrush brush(Qt::SolidPattern);
    brush.setColor(QColor(0, 0, 0, 155));

    QPen pen(Qt::white);
    //    painter->setFont(font);
    pen.setWidthF(0.5);
    painter->setPen(pen);
    painter->setBrush(brush);


    // Define the area assigned to displaying the tooltip
    //    QRect area = fm.boundingRect (this->geometry(), Qt::AlignLeft, tip);

    //    area.moveBottomLeft(QPoint(5,this->height()-5));

    QRect area(QPoint(0, this->height() - 3*fm.height()), QPoint(this->width(), this->height()));

    area += QMargins(2, 2, 2, 2);
    painter->drawRect(area);
    area -= QMargins(2, 2, 2, 2);

    m_staticText.setTextWidth(area.width());

    m_staticText.setText(tip);

    // Draw tooltip
    //    painter->drawText(area, Qt::AlignLeft, tip);
    painter->drawStaticText(area.topLeft()+QPoint(5,0), m_staticText);
}

void ImageOpenGLWidget::drawImageMarkers(QPainter * painter)
{
    // Next shit: Make sidebar tree view for le image_markers

//    qsrand(QTime::currentTime().msec());

//    QList<ImageMarker> list;
//    for (int i = 0; i < 4; i++)
//    {
//        int x = qrand()%1475;
//        int y = qrand()%1675;
//        int w = 20 + qrand()%280;
//        list << ImageMarker(x,y,w,w);
//    }
//    image_markers << list;

    QVector<GLfloat> fragpos;
    QVector<GLfloat> texpos;

    for (int i = 0; i < image_markers.size(); i++)
    {
        for (int j = 0; j < image_markers[i].size(); j++)
        {
            double x = (image_markers[i][j].x() / (double) this->width()) * 2.0 - 1.0;
            double y = 1.0 - ((image_markers[i][j].y()+image_markers[i][j].h()) / (double) this->height()) * 2.0;
            double w = (image_markers[i][j].w() / (double) this->width()) * 2.0;
            double h = (image_markers[i][j].h() / (double) this->height()) * 2.0;

            fragpos << x << y
                    << x << y + h
                    << x + w << y
                    << x << y + h
                    << x + w << y
                    << x + w << y + h;

            texpos  << 0 << 0
                    << 0 << 1
                    << 1 << 0
                    << 0 << 1
                    << 1 << 0
                    << 1 << 1;
        }
    }

    painter->beginNativePainting();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);



    std_2d_sprite_program->bind();

    texture_image_marker->bind();
    std_2d_sprite_program->setUniformValue(std_2d_sprite_texture, 0);

    texture_view_matrix = zoom_matrix * translation_matrix;

    glUniformMatrix4fv(std_2d_sprite_transform, 1, GL_FALSE, texture_view_matrix.colmajor().toFloat().data());

    glEnableVertexAttribArray(std_2d_sprite_fragpos);
    glEnableVertexAttribArray(std_2d_sprite_pos);

    glVertexAttribPointer(std_2d_sprite_fragpos, 2, GL_FLOAT, GL_FALSE, 0, fragpos.data());
    glVertexAttribPointer(std_2d_sprite_pos, 2, GL_FLOAT, GL_FALSE, 0, texpos.data());

    // Draw image
    glDrawArrays(GL_TRIANGLES,  0, fragpos.size()/2);

    glDisableVertexAttribArray(std_2d_sprite_pos);
    glDisableVertexAttribArray(std_2d_sprite_fragpos);

    texture_image_marker->release();
    std_2d_sprite_program->release();

    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    painter->endNativePainting();
}

void ImageOpenGLWidget::drawConeEwaldIntersect(QPainter * painter)
{
    // A bug makes this function draw badly to the screenshot buffer

    // Draw circle corresponding to cone intersection of the Ewald
//    QPointF beam_xy(image.beamY() + this->width() * 0.5, image.beamX() + this->height() * 0.5);
//    QPointF((qreal) this->width() * 0.5, (qreal) this->height() * 0.5)

    Matrix<double> beam_image_pos(4, 1, 0);
    Matrix<double> beam_screen_pos(4, 1, 0);

    beam_image_pos[0] = 2.0 * (isBeamOverrideActive ? image.beamY() + beam_y_override : image.beamY()) / this->width(); // DANGER
    beam_image_pos[1] = 2.0 * ((isBeamOverrideActive ? image.beamX() + beam_x_override : image.beamX()) - image.height()) / this->height(); // DANGER
    beam_image_pos[2] = 0;
    beam_image_pos[3] = 1.0;

    beam_screen_pos = texture_view_matrix * beam_image_pos;

    beam_screen_pos[0] = (beam_screen_pos[0] + 1.0) * 0.5 * this->width();
    beam_screen_pos[1] = (-(beam_screen_pos[1] + 1.0) * 0.5 + 1.0) * this->height();

    double radius = sqrt(pow(beam_screen_pos[0] - pos.x(), 2.0) + pow(beam_screen_pos[1] - pos.y(), 2.0));

    QPen pen(QColor(255.0, 255.0, 0.0, 150));
    pen.setWidthF(1.5);
    painter->setPen(pen);

    QRectF circle(beam_screen_pos[0] - radius, beam_screen_pos[1] - radius, radius*2, radius*2);

    painter->drawEllipse(circle);

    pen.setStyle(Qt::DashDotLine);
    painter->setPen(pen);

    painter->drawLine(QPoint(beam_screen_pos[0], beam_screen_pos[1]), pos);

}

void ImageOpenGLWidget::drawPlaneMarkerToolTip(QPainter * painter)
{
    QList<Selection> marker = p_working_data[p_current_filepath].planeMarker();

    for (int i = 0; i < n_lsq_samples; i++)
    {
        // Draw box to indicate area
        QFont font("Monospace", 10);
        QFontMetrics fm(font);

        QBrush brush(Qt::SolidPattern);
        brush.setColor(QColor(0, 0, 0, 155));

        QPen pen(Qt::white);
        painter->setFont(font);
        painter->setPen(pen);
        painter->setBrush(brush);

        QPointF text_pos_gl = posQttoGL(QPointF(
                                            (float)marker.at(i).topLeft().x() + (float) this->width() * 0.5,
                                            (float)marker.at(i).topLeft().y() + (float) this->height() * 0.5));
        Matrix<double> pos_gl(4, 1);
        pos_gl[0] = text_pos_gl.x();
        pos_gl[1] = text_pos_gl.y();
        pos_gl[2] = 0;
        pos_gl[3] = 1;

        pos_gl = texture_view_matrix * pos_gl;

        QPointF topleft_text_pos_qt = posGLtoQt(QPointF(pos_gl[0] / pos_gl[3], pos_gl[1] / pos_gl[3]));

        text_pos_gl = posQttoGL(QPointF(
                                    (float)marker.at(i).bottomRight().x() + (float) this->width() * 0.5,
                                    (float)marker.at(i).bottomRight().y() + (float) this->height() * 0.5));
        pos_gl[0] = text_pos_gl.x();
        pos_gl[1] = text_pos_gl.y();
        pos_gl[2] = 0;
        pos_gl[3] = 1;

        pos_gl = texture_view_matrix * pos_gl;

        QPointF botright_text_pos_qt = posGLtoQt(QPointF(pos_gl[0] / pos_gl[3], pos_gl[1] / pos_gl[3]));

        QRectF rect(topleft_text_pos_qt, botright_text_pos_qt);

        painter->drawRect(rect);

        QString tip_medium = QString::number(marker.at(i).average(), 'g', 3);
        QString tip_small = QString::number(marker.at(i).average(), 'g', 2);

        if ((fm.boundingRect(tip_medium).width() + 5 < rect.width()) && (fm.boundingRect(tip_medium).height() < rect.height()))
        {
            painter->drawText(rect, tip_medium);
        }
        else if ((fm.boundingRect(tip_small).width() + 5 < rect.width()) && (fm.boundingRect(tip_small).height() < rect.height()))
        {
            painter->drawText(rect, tip_small);
        }

    }

    p_working_data[p_current_filepath].setPlaneMarker(marker);

}

QPointF ImageOpenGLWidget::posGLtoQt(QPointF coord)
{
    QPointF QtPoint;

    QtPoint.setX(0.5 * (float)this->width() * (coord.x() + 1.0) - 1.0);
    QtPoint.setY(0.5 * (float)this->height() * (1.0 - coord.y()) - 1.0);

    return QtPoint;
}

QPointF ImageOpenGLWidget::posQttoGL(QPointF coord)
{
    QPointF GLPoint;
    GLPoint.setX((coord.x() + 1.0) / (float) (this->width()) * 2.0 - 1.0);
    GLPoint.setY((1.0 - (coord.y() + 1.0) / (float) this->height()) * 2.0 - 1.0);
    return GLPoint;
}

void ImageOpenGLWidget::setMode(int value)
{
    mode = value;
    processScatteringDataProxy();
    updateImageTexture();

//    if (!p_set.isEmpty())
    {
        Selection analysis_area = p_working_data[p_current_filepath].selection();
        processSelectionDataProxy(&analysis_area);
        p_working_data[p_current_filepath].setSelection(analysis_area);
    }

    update();
}

void ImageOpenGLWidget::setCorrectionLorentz(bool value)
{
    isCorrectionLorentzActive = (int) value;

    processScatteringDataProxy();
    updateImageTexture();

//    if (!p_set.isEmpty())
    {
        Selection analysis_area = p_working_data[p_current_filepath].selection();
        processSelectionDataProxy(&analysis_area);
        p_working_data[p_current_filepath].setSelection(analysis_area);
    }

    update();
}

void ImageOpenGLWidget::setCorrectionBackground(bool value)
{
    isBackgroundCorrected = (int) value;

    processScatteringDataProxy();
    updateImageTexture();

//    if (!p_set.isEmpty())
    {
        Selection analysis_area = p_working_data[p_current_filepath].selection();
        processSelectionDataProxy(&analysis_area);
        p_working_data[p_current_filepath].setSelection(analysis_area);
    }

    update();
}

void ImageOpenGLWidget::setParameter(Matrix<float> &data)
{
    if (0)
    {
        qDebug() << "th_a_low" << data[0];
        qDebug() << "th_a_high" << data[1];
        qDebug() << "th_b_low" << data[2];
        qDebug() << "th_b_high" << data[3];
        qDebug() << "flux" << data[4];
        qDebug() << "exp_time" << data[5];
        qDebug() << "wavelength" << data[6];
        qDebug() << "det_dist" << data[7];
        qDebug() << "beam_x" << data[8];
        qDebug() << "beam_y" << data[9];
        qDebug() << "pix_size_x" << data[10];
        qDebug() << "pix_size_y" << data[11];
        qDebug() << "intensity_min" << data[12];
        qDebug() << "intensity_max" << data[13];
    }

    if (isCLInitialized)
    {
        err =  QOpenCLEnqueueWriteBuffer (context_cl.queue(),
                                          parameter_cl,
                                          CL_TRUE,
                                          0,
                                          data.bytes(),
                                          data.data(),
                                          0, 0, 0);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }
}

QPoint ImageOpenGLWidget::getImagePixel(QPoint pos)
{
    // Find the OpenGL coordinate of the cursor
    Matrix<double> screen_pos_gl(4, 1);
    screen_pos_gl[0] = 2.0 * (double) pos.x() / (double) this->width() - 1.0;
    screen_pos_gl[1] = - 2.0 * (double) pos.y() / (double) this->height() + 1.0;
    screen_pos_gl[2] = 0;
    screen_pos_gl[3] = 1.0;

    // Use the inverse transform to find the corresponding image pixel, rounding to nearest
    Matrix<double> image_pos_gl(4, 1);
    image_pos_gl = texture_view_matrix.inverse4x4() * screen_pos_gl;

    QPoint image_pixel;

    image_pixel.setX(0.5 * image_pos_gl[0]*this->width());
    image_pixel.setY(-0.5 * image_pos_gl[1]*this->height());

    if (image_pixel.x() < 0)
    {
        image_pixel.setX(0);
    }

    if (image_pixel.x() >= image.width())
    {
        image_pixel.setX(image.width() - 1);
    }

    if (image_pixel.y() < 0)
    {
        image_pixel.setY(0);
    }

    if (image_pixel.y() >= image.height())
    {
        image_pixel.setY(image.height() - 1);
    }

    return image_pixel;
}

void ImageOpenGLWidget::mouseMoveEvent(QMouseEvent * event)
{
    float move_scaling = 1.0;
    pos = event->pos();

    if ((event->buttons() & Qt::LeftButton))
    {
        if ((event->modifiers() & Qt::ShiftModifier) && image.isDataRead())
        {
            Selection analysis_area = p_working_data[p_current_filepath].selection();

            QPoint pixel = getImagePixel(pos);

            analysis_area.setBottomRight(pixel);

            analysis_area = analysis_area.normalized();

            p_working_data[p_current_filepath].setSelection(analysis_area);
        }
        else if ((event->modifiers() & Qt::ControlModifier) && image.isDataRead())
        {
            Selection analysis_area = p_working_data[p_current_filepath].selection();

            QPoint pixel = getImagePixel(pos);

            analysis_area.moveCenter(pixel);

            analysis_area = analysis_area.normalized();

            p_working_data[p_current_filepath].setSelection(analysis_area);
        }
        else
        {
            // Check for selected objects
            bool isSomethingSelected = false;

            if (image.isDataRead())
            {
                QList<Selection> marker(p_working_data[p_current_filepath].planeMarker());

                for (int i = 0; i < n_lsq_samples; i++)
                {
                    if (marker[i].selected() == true)
                    {
                        isSomethingSelected = true;
                        marker[i].moveTo(marker[i].topLeft() + (getImagePixel(pos) - getImagePixel(prev_pos)));
                        marker[i].restrictToRect(QRect(0, 0, image.width(), image.height()));
                    }
                }

                p_working_data[p_current_filepath].setPlaneMarker(marker);
            }

            if (!isSomethingSelected)
            {
                double dx = (pos.x() - prev_pos.x()) * 2.0 / (this->width() * zoom_matrix[0]);
                double dy = -(pos.y() - prev_pos.y()) * 2.0 / (this->height() * zoom_matrix[0]);

                translation_matrix[3] += dx * move_scaling;
                translation_matrix[7] += dy * move_scaling;
            }
        }
    }
    prev_pos = pos;
    update();
}

void ImageOpenGLWidget::mousePressEvent(QMouseEvent * event)
{
    pos = event->pos();

    if (image.isDataRead())
    {
        if (!(event->modifiers() & Qt::ShiftModifier) && (event->buttons() & Qt::LeftButton))
        {
            //Position
            Matrix<double> screen_pixel_pos(4, 1, 0); // Uses GL coordinates
            screen_pixel_pos[0] = 2.0 * (double) pos.x() / (double) this->width() - 1.0;
            screen_pixel_pos[1] = 2.0 * (1.0 - (double) pos.y() / (double) this->height()) - 1.0;
            screen_pixel_pos[2] = 0;
            screen_pixel_pos[3] = 1.0;

            Matrix<double> image_pixel_pos(4, 1); // Uses GL coordinates

            image_pixel_pos = texture_view_matrix.inverse4x4() * screen_pixel_pos;

            double pixel_x = image_pixel_pos[0] * this->width() * 0.5;
            double pixel_y = - image_pixel_pos[1] * this->height() * 0.5;

            if (pixel_x < 0)
            {
                pixel_x = 0;
            }

            if (pixel_y < 0)
            {
                pixel_y = 0;
            }

            if (pixel_x >= image.width())
            {
                pixel_x = image.width() - 1;
            }

            if (pixel_y >= image.height())
            {
                pixel_y = image.height() - 1;
            }

            QList<Selection> marker(p_working_data[p_current_filepath].planeMarker());

            for (int i = 0; i < n_lsq_samples; i++)
            {
                if (((pixel_x >= marker[i].x()) && (pixel_x <= marker[i].x() + marker[i].width())) && ((pixel_y >= marker[i].y()) && (pixel_y <= marker[i].y() + marker[i].height())))
                {
                    marker[i].setSelected(true);
                    break;
                }
            }

            p_working_data[p_current_filepath].setPlaneMarkerTest(marker);
        }
        else if ((event->modifiers() & Qt::ShiftModifier) && (event->buttons() & Qt::LeftButton))
        {
            Selection analysis_area = p_working_data[p_current_filepath].selection();

            QPoint pixel = getImagePixel(pos);

            analysis_area.setTopLeft(pixel);

            analysis_area = analysis_area.normalized();

            processSelectionDataProxy(&analysis_area);

            p_working_data[p_current_filepath].setSelection(analysis_area);
        }
    }
    update();
}

void ImageOpenGLWidget::mouseReleaseEvent(QMouseEvent * event)
{
    pos =  event->pos();

    if (image.isDataRead())
    {
        // A bit overkill to set [...] on mouse release as well as mouse move and push
        if ((event->modifiers() & Qt::ShiftModifier) && !(event->buttons() & Qt::LeftButton))
        {
            Selection analysis_area = p_working_data[p_current_filepath].selection();

            QPoint pixel = getImagePixel(pos);

            analysis_area.setBottomRight(pixel);

            analysis_area = analysis_area.normalized();

            processSelectionDataProxy(&analysis_area);

            p_working_data[p_current_filepath].setSelection(analysis_area);
        }
        else if ((event->modifiers() & Qt::ControlModifier) && !(event->buttons() & Qt::LeftButton))
        {
            Selection analysis_area = p_working_data[p_current_filepath].selection();

            QPoint pixel = getImagePixel(pos);

            analysis_area.moveCenter(pixel);

            analysis_area = analysis_area.normalized();

            processSelectionDataProxy(&analysis_area);

            p_working_data[p_current_filepath].setSelection(analysis_area);
        }


        // Deselect objects
        QList<Selection> marker(p_working_data[p_current_filepath].planeMarker());

        for (int i = 0; i < marker.size(); i++)
        {
            marker[i].setSelected(false);
        }

//        p_set.current()->setPlaneMarker(marker);

        // Recalculate
        processScatteringDataProxy();
        updateImageTexture();
    }
    update();
}

void ImageOpenGLWidget::wheelEvent(QWheelEvent * event)
{
    float move_scaling = 1.5;

    if (event->modifiers() & Qt::ShiftModifier)
    {
        move_scaling = 3.0;
    }
    else if (event->modifiers() & Qt::ControlModifier)
    {
        move_scaling = 0.2;
    }

    double delta = move_scaling * ((double)event->delta()) * 0.0008;

    if ((zoom_matrix[0] + zoom_matrix[0]*delta < 256))// && (isRendering == false))
    {
        /*
         * Zooming happens around the GL screen coordinate (0,0), i.e. the middle,
         * but by first tranlating the frame to the position of the cursor, we can
         * make zooming happen on the cursor instead. Then the frame should be
         * translated back such that the object under the cursor does not appear
         * to actually have been translated.
         * */

        // Translate cursor position to middle
        double dx = -(event->x() - this->width() * 0.5) * 2.0 / (this->width() * zoom_matrix[0]);
        double dy = -((this->height() - event->y()) - this->height() * 0.5) * 2.0 / (this->height() * zoom_matrix[0]);

        translation_matrix[3] += dx;
        translation_matrix[7] += dy;

        double tmp = zoom_matrix[0];

        // Zoom
        zoom_matrix[0] += zoom_matrix[0] * delta;
        zoom_matrix[5] += zoom_matrix[5] * delta;
        zoom_matrix[10] += zoom_matrix[10] * delta;

        // Translate from middle back to cursor position, taking into account the new zoom
        translation_matrix[3] -= dx * tmp / zoom_matrix[0];
        translation_matrix[7] -= dy * tmp / zoom_matrix[0];

        update();
    }
}

void ImageOpenGLWidget::takeScreenShot(QString path)
{
    bool tmp = isEwaldCircleActive;
    isEwaldCircleActive = false;

    QOpenGLFramebufferObjectFormat format;
    format.setSamples(64);
    format.setInternalTextureFormat(GL_RGBA32F);

    QOpenGLFramebufferObject buffy(this->size(), format);

    buffy.bind();

    // Render into buffer using max quality
    paintGL();
    glFinish();

    buffy.release();

    // Save buffer as image
    buffy.toImage().save(path);

    isEwaldCircleActive = tmp;
}

void ImageOpenGLWidget::saveImage(QString path)
{
    QOpenGLFramebufferObjectFormat format;

    //    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    //    format.setMipmap(true);
    format.setSamples(64);
    //    format.setTextureTarget(GL_TEXTURE_2D);
    format.setInternalTextureFormat(GL_RGBA32F);

    QOpenGLFramebufferObject buffy(image.width(), image.height(), format);

    buffy.bind();

    // Render into buffer
    QOpenGLPaintDevice paint_device_gl(this->size());

    QPainter painter(&paint_device_gl);

    beginRawGLCalls(&painter);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glViewport(0, 0, image.width(), image.height());

//    if (!p_set.isEmpty())
    {
        std_2d_tex_program->bind();

        std_2d_tex_program->setUniformValue(std_2d_tex_texture, 0);

        GLfloat texpos[] =
        {
            0.0, 0.0,
            1.0, 0.0,
            1.0, 1.0,
            0.0, 1.0
        };

        GLuint indices[] = {0, 1, 3, 1, 2, 3};

        texture_view_matrix.setIdentity(4);

        glUniformMatrix4fv(std_2d_tex_transform, 1, GL_FALSE, texture_view_matrix.colmajor().toFloat().data());

        glEnableVertexAttribArray(std_2d_tex_fragpos);
        glEnableVertexAttribArray(std_2d_tex_pos);

        GLfloat fragpos[] =
        {
            -1.0, -1.0,
            1.0, -1.0,
            1.0, 1.0,
            -1.0, 1.0
        };

        // Draw image
        glBindTexture(GL_TEXTURE_2D, image_tex_gl);
        glVertexAttribPointer(std_2d_tex_fragpos, 2, GL_FLOAT, GL_FALSE, 0, fragpos);
        glVertexAttribPointer(std_2d_tex_pos, 2, GL_FLOAT, GL_FALSE, 0, texpos);

        glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);

        glDisableVertexAttribArray(std_2d_tex_pos);
        glDisableVertexAttribArray(std_2d_tex_fragpos);

        std_2d_tex_program->release();

        // Save buffer as image
        buffy.toImage().save(path);
    }

    endRawGLCalls(&painter);
    /////////////////////////////////

    buffy.release();
}
