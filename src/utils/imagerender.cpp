#include "imagerender.h"

ImageRenderWindow::ImageRenderWindow()
    : isInitialized(false),
    isAlphaImgInitialized(false),
    isBetaImgInitialized(false),
    isGammaImgInitialized(false),
    isTsfImgInitialized(false),
    isTsfTexInitialized(false)
{
    this->image_w = 640;
    this->image_h = 1024;

    // Transfer texture
    tsf_color_scheme = 0;
    tsf_alpha_scheme = 0;

    // Color
    GLfloat white_buf[] = {1,1,1,0.4};
    GLfloat black_buf[] = {0,0,0,0.4};
    white.setDeep(1,4,white_buf);
    black.setDeep(1,4,black_buf);
    clear_color = white;
    clear_color_inverse = black;
}

void ImageRenderWindow::setImageWidth(int value)
{
    if (value != image_w)
    {
        this->image_w = value;
        this->setTarget();
    }
}

void ImageRenderWindow::setImageHeight(int value)
{
    if (value != image_h)
    {
        this->image_h = value;
        this->setTarget();
    }
}

void ImageRenderWindow::setImageSize(int w, int h)
{
    if ((w != image_w) || (h != image_h))
    {
        this->image_w = w;
        this->image_h = h;
        this->setTarget();
    }
}

ImageRenderWindow::~ImageRenderWindow()
{
    if (isAlphaImgInitialized) clReleaseMemObject(cl_img_alpha);
    if (isBetaImgInitialized) clReleaseMemObject(cl_img_beta);
    if (isGammaImgInitialized) clReleaseMemObject(cl_img_gamma);
}



void ImageRenderWindow::initialize()
{
    initializePaintTools();
    setTsfTexture();

    paint_device_gl->setSize(size());
    isInitialized = true;
}

void ImageRenderWindow::beginRawGLCalls(QPainter * painter)
{
    painter->beginNativePainting();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
}

void ImageRenderWindow::endRawGLCalls(QPainter * painter)
{
    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    painter->endNativePainting();
}

void ImageRenderWindow::render(QPainter *painter)
{
    glClearColor(clear_color[0], clear_color[1], clear_color[2], 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    beginRawGLCalls(painter);
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    // Draw raytracing texture
    drawImages();
    endRawGLCalls(painter);

    // Draw overlay
    drawOverlay(painter);
}

void ImageRenderWindow::drawImages()
{
    alpha_rect.setRect(0, 0, height()*image_w/image_h, height());
    beta_rect.setRect(alpha_rect.x() + alpha_rect.width(), 0, height()*image_w/image_h, height());
    gamma_rect.setRect(beta_rect.x() + beta_rect.width(), 0, height()*image_w/image_h, height());

    alpha_rect -= QMargins(5,5,5,31);
    beta_rect -= QMargins(5,5,5,31);
    gamma_rect -= QMargins(5,5,5,31);

    if (isAlphaImgInitialized && isBetaImgInitialized && isGammaImgInitialized)
    {
        shared_window->std_2d_tex_program->bind();

        GLuint indices[] = {0,1,3,1,2,3};

        GLfloat texpos[] = {
            0.0, 0.0,
            1.0, 0.0,
            1.0, 1.0,
            0.0, 1.0
        };
        glVertexAttribPointer(shared_window->std_2d_texpos, 2, GL_FLOAT, GL_FALSE, 0, texpos);

        glEnableVertexAttribArray(shared_window->std_2d_fragpos);
        glEnableVertexAttribArray(shared_window->std_2d_texpos);

        // Alpha
        Matrix<GLfloat> gl_alpha_rect(4,2);
        glRect(&gl_alpha_rect, &alpha_rect);
        glVertexAttribPointer(shared_window->std_2d_fragpos, 2, GL_FLOAT, GL_FALSE, 0, gl_alpha_rect.data());

        glBindTexture(GL_TEXTURE_2D, image_tex[0]);
        shared_window->std_2d_tex_program->setUniformValue(shared_window->std_2d_texture, 0);
        glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);

        // Beta
        Matrix<GLfloat> gl_beta_rect(4,2);
        glRect(&gl_beta_rect, &beta_rect);
        glVertexAttribPointer(shared_window->std_2d_fragpos, 2, GL_FLOAT, GL_FALSE, 0, gl_beta_rect.data());

        glBindTexture(GL_TEXTURE_2D, image_tex[1]);
        shared_window->std_2d_tex_program->setUniformValue(shared_window->std_2d_texture, 0);
        glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);

        // Gamma
        Matrix<GLfloat> gl_gamma_rect(4,2);
        glRect(&gl_gamma_rect, &gamma_rect);
        glVertexAttribPointer(shared_window->std_2d_fragpos, 2, GL_FLOAT, GL_FALSE, 0, gl_gamma_rect.data());

        glBindTexture(GL_TEXTURE_2D, image_tex[2]);
        shared_window->std_2d_tex_program->setUniformValue(shared_window->std_2d_texture, 0);
        glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);


        glDisableVertexAttribArray(shared_window->std_2d_texpos);
        glDisableVertexAttribArray(shared_window->std_2d_fragpos);
        glBindTexture(GL_TEXTURE_2D, 0);

        shared_window->std_2d_tex_program->release();
    }
}

void ImageRenderWindow::drawOverlay(QPainter * painter)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(*fill_brush);
    painter->setPen(*normal_pen);


    // Alpha text
    QString alpha_string("Raw data");
    QRect alpha_string_rect(alpha_rect.x(),
                                 height() - 31,
                                 alpha_rect.width(),
                                 31);
    alpha_string_rect -= QMargins(0,2,0,2);

    painter->drawRect(alpha_string_rect);
    painter->drawText(alpha_string_rect, Qt::AlignCenter, alpha_string);

    // beta text
    QString beta_string("Background subtracted");
    QRect beta_string_rect(beta_rect.x(),
                                 height() - 31,
                                 beta_rect.width(),
                                 31);
    beta_string_rect -= QMargins(0,2,0,2);

    painter->drawRect(beta_string_rect);
    painter->drawText(beta_string_rect, Qt::AlignCenter, beta_string);

    // gamma text
    QString gamma_string("Lorentz-Polarization corrected");
    QRect gamma_string_rect(gamma_rect.x(),
                                 height() - 31,
                                 gamma_rect.width(),
                                 31);
    gamma_string_rect -= QMargins(0,2,0,2);

    painter->drawRect(gamma_string_rect);
    painter->drawText(gamma_string_rect, Qt::AlignCenter, gamma_string);

    // Alpha frame
    painter->setBrush(*normal_brush);
    painter->drawRect(alpha_rect);

    // Beta frame
    painter->drawRect(beta_rect);

    // Gamma frame
    painter->drawRect(gamma_rect);
}

void ImageRenderWindow::mouseMoveEvent(QMouseEvent* ev)
{
    Q_UNUSED(ev);
}

void ImageRenderWindow::wheelEvent(QWheelEvent* ev)
{
    Q_UNUSED(ev);
}

void ImageRenderWindow::resizeEvent(QResizeEvent * ev)
{
    Q_UNUSED(ev);

    if (paint_device_gl) paint_device_gl->setSize(size());
}


void ImageRenderWindow::releaseSharedBuffers()
{
     // Release shared CL/GL objects
    err = clFinish(*context_cl->getCommandQueue());
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    err = clEnqueueReleaseGLObjects(*context_cl->getCommandQueue(), 1, &cl_img_alpha, 0, 0, 0);
    err |= clEnqueueReleaseGLObjects(*context_cl->getCommandQueue(), 1, &cl_img_beta, 0, 0, 0);
    err |= clEnqueueReleaseGLObjects(*context_cl->getCommandQueue(), 1, &cl_img_gamma, 0, 0, 0);
    err |= clEnqueueReleaseGLObjects(*context_cl->getCommandQueue(), 1, &cl_tsf_tex, 0, 0, 0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void ImageRenderWindow::initResourcesGL()
{
}

void ImageRenderWindow::initializePaintTools()
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

void ImageRenderWindow::setTsfTexture()
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
}

cl_mem * ImageRenderWindow::getTsfImgCLGL()
{
    return &cl_tsf_tex;
}

cl_mem * ImageRenderWindow::getAlphaImgCLGL()
{
    return &cl_img_alpha;
}

cl_mem * ImageRenderWindow::getGammaImgCLGL()
{
    return &cl_img_gamma;
}

cl_mem * ImageRenderWindow::getBetaImgCLGL()
{
    return &cl_img_beta;
}


void ImageRenderWindow::aquireSharedBuffers()
{
    glFinish();
    err = clEnqueueAcquireGLObjects(*context_cl->getCommandQueue(), 1, &cl_img_alpha, 0, 0, 0);
    err |= clEnqueueAcquireGLObjects(*context_cl->getCommandQueue(), 1, &cl_img_beta, 0, 0, 0);
    err |= clEnqueueAcquireGLObjects(*context_cl->getCommandQueue(), 1, &cl_img_gamma, 0, 0, 0);
    err |= clEnqueueAcquireGLObjects(*context_cl->getCommandQueue(), 1, &cl_tsf_tex, 0, 0, 0);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
}

void ImageRenderWindow::setSharedWindow(SharedContextWindow * window)
{
    this->shared_window = window;
    shared_context = window->getGLContext();
}

void ImageRenderWindow::setTarget()
{
    if (isAlphaImgInitialized) clReleaseMemObject(cl_img_alpha);
    if (isBetaImgInitialized) clReleaseMemObject(cl_img_beta);
    if (isGammaImgInitialized) clReleaseMemObject(cl_img_gamma);

    // Set GL texture
    glDeleteTextures(1, &image_tex[0]);
    glGenTextures(1, &image_tex[0]);

    glBindTexture(GL_TEXTURE_2D, image_tex[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        image_w,
        image_h,
        0,
        GL_RGBA,
        GL_FLOAT,
        NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Convert to CL texture
    cl_img_alpha = clCreateFromGLTexture2D(*context_cl->getContext(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, image_tex[0], &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    isAlphaImgInitialized = true;

    // Set GL texture
    glDeleteTextures(1, &image_tex[1]);
    glGenTextures(1, &image_tex[1]);

    glBindTexture(GL_TEXTURE_2D, image_tex[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        image_w,
        image_h,
        0,
        GL_RGBA,
        GL_FLOAT,
        NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Convert to CL texture
    cl_img_beta = clCreateFromGLTexture2D(*context_cl->getContext(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, image_tex[1], &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));
    isBetaImgInitialized = true;

    // Set GL texture
    glDeleteTextures(1, &image_tex[2]);
    glGenTextures(1, &image_tex[2]);

    glBindTexture(GL_TEXTURE_2D, image_tex[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        image_w,
        image_h,
        0,
        GL_RGBA,
        GL_FLOAT,
        NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Convert to CL texture
    cl_img_gamma = clCreateFromGLTexture2D(*context_cl->getContext(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, image_tex[2], &err);
    if ( err != CL_SUCCESS) qFatal(cl_error_cstring(err));

    isGammaImgInitialized = true;
}

