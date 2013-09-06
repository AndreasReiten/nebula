#include "volumerender.h"

VolumeRenderGLWidget::VolumeRenderGLWidget(cl_device * device, cl_context * context2, cl_command_queue * queue, const QGLFormat & format, QWidget *parent, const QGLWidget * shareWidget) :
    QGLWidget(format, parent, shareWidget)
{
    //~std::cout << "Constructing VolumeRenderGLWidget" << std::endl;
    //~ std::cout << "Volume Render: Alpha Channel = " << this->context()->format().alpha() << std::endl;
    isGLIntitialized = false;
    verbose = 1;

    ray_res = 20;

    LINEWIDTH = 0.5;
    pp_samples = 5;
    pp_deviation = 2;
    pp_scale = 1;

    /* colors */
    float c_white[4] = {1,1,1,1};
    float c_transparent[4] = {0,0,0,0};
    float c_black[4] = {0,0,0,1};
    float c_blue[4] = {0,0,1,1};
    float c_red[4] = {1,0,0,1};
    float c_green[4] = {0,1,0,1};

    this->isPerspectiveRequired = true;
    this->isLog = true;

    clear.setDeep(4, c_white);
    clearInv.setDeep(4, c_black);
    white.setDeep(4, c_white);
    transparent.setDeep(4, c_transparent);
    black.setDeep(4, c_black);
    blue.setDeep(4, c_blue);
    red.setDeep(4, c_red);
    green.setDeep(4, c_green);
    colorBackDropA.setDeep(4, c_black);
    colorBackDropB.setDeep(4, c_white);
    colorBackDropC.setDeep(4, c_transparent);

    HIST_MINMAX.reserve(2);

    TSF_PARAMETERS.reserve(1,6);
    TSF_PARAMETERS[0] = 0.0;
    TSF_PARAMETERS[1] = 1.0;

    MISC_FLOAT_K_FUNCTION.reserve(1,4);

    MISC_FLOAT_K_RAYTRACE.reserve(1,8);
    MISC_FLOAT_K_RAYTRACE[0] = 3.5;
    MISC_FLOAT_K_RAYTRACE[1] = 0.0;
    MISC_FLOAT_K_RAYTRACE[2] = 0.0;
    MISC_FLOAT_K_RAYTRACE[3] = 0.0;
    MISC_FLOAT_K_RAYTRACE[4] = 1.0;
    MISC_FLOAT_K_RAYTRACE[5] = 1.0;

    MISC_INT.reserve(1,8);
    MISC_INT[2] = this->isLog;
    MISC_INT[3] = 0;
    MISC_INT[5] = this->isPerspectiveRequired;
    MISC_INT[6] = 32;
    MISC_INT[7] = 32;

    this->MSAA_SAMPLES = 8;
    this->MSAA_METHOD = 0;
    this->MSAA_EXPOSURE = 1.0;

    this->auto_rotation_delay = 1000*30;

    isUnitcellValid = false;
    isFunctionActive = true;
    this->lastPos_x = 0.0;
    this->lastPos_y = 0.0;
    this->zeta = 0.0;
    this->eta = 0.0;
    this->X_rotation = 0.0;
    this->Y_rotation = 0.0;
    this->WIDTH = 32;
    this->HEIGHT = 32;
    this->SMALL_WIDTH = 32;
    this->SMALL_HEIGHT = 32;
    this->ray_loc_ws[0] = 16;
    this->ray_loc_ws[1] = 16;
    this->device = device;
    this->queue = queue;
    this->context2 = context2;
    this->N = 0.1;
    this->F = 10.0;
    this->fov = 10.0;
    this->pixel_size.set(1,2,0.0);

    {
        float tmp = 1.0;
        float extent[8] = {
            -tmp,tmp,
            -tmp,tmp,
            -tmp,tmp,
             1.0f,1.0f};

        this->BBOX_EXTENT.setDeep(4, 2, extent);
    }
    {
        float tmp = 2*pi;
        float extent[8] = {
            -tmp,tmp,
            -tmp,tmp,
            -tmp,tmp,
             1.0f,1.0f};
        this->DATA_EXTENT.setDeep(4, 2, extent);
        this->DATA_VIEW_EXTENT.setDeep(4, 2, extent);
    }
    this->U.setIdentity(4);
    this->U3x3.setIdentity(3);
    this->DATA_SCALING.setIdentity(4);
    this->BBOX_SCALING.setIdentity(4);
    this->NORM_SCALING.setIdentity(4);

    BBOX_SCALING[0] = 0.3;
    BBOX_SCALING[5] = 0.3;
    BBOX_SCALING[10] = 0.3;

    this->PROJECTION_SCALING.setIdentity(4);
    this->DATA_TRANSLATION.setIdentity(4);
    this->BBOX_TRANSLATION.setIdentity(4);
    this->BBOX_TRANSLATION[11] = -N -(F - N)*0.5;

    this->DATA_VIEW_MATRIX.setIdentity(4);
    this->BBOX_VIEW_MATRIX.setIdentity(4);
    this->CELL_VIEW_MATRIX.setIdentity(4);
    this->I.setIdentity(4);

    this->CTC_MATRIX.setN(N);
    this->CTC_MATRIX.setF(F);
    this->CTC_MATRIX.setFov(fov);
    this->CTC_MATRIX.setProjection(isPerspectiveRequired);

    this->setFocusPolicy(Qt::StrongFocus);

    /* This timer keeps track of the time since this constructor was
     * called */
    time = new QElapsedTimer();
    time->start();

    /* This timer keeps track of the time since the last user input */
    rotationTimer = new QElapsedTimer();
    rotationTimer->start();

    /* This timer keeps track of the time since the last kernel output changing action */
    timerLastAction = new QElapsedTimer();
    timerLastAction->start();

    /* This timer keeps track of the time spent by a kernel call */
    callTimer = new QElapsedTimer();
    callTimer->start();

    /* This timer emits a signal every 1000.0/FPS_MAX milli seconds.
     * The slot is the QGL repaint function */
    timer = new QTimer(this);

    int FPS_MAX = 60;
    timer->start(1000.0/FPS_MAX);
    connect(timer,SIGNAL(timeout()),this,SLOT(repaint()));



    //~std::cout << "Done Constructing VolumeRenderGLWidget" << std::endl;
}

VolumeRenderGLWidget::~VolumeRenderGLWidget()
{
    if (isGLIntitialized)
    {
        if (verbose) emit appendLog( "VolumeRenderGLWidget-> Destroying..." );

        if (ray_tex_cl) clReleaseMemObject(ray_tex_cl);
        if (misc_int_cl) clReleaseMemObject(misc_int_cl);
        if (misc_float_cl) clReleaseMemObject(misc_float_cl);
        if (misc_float_k_raytrace_cl) clReleaseMemObject(misc_float_k_raytrace_cl);
        if (tsf_parameters_cl) clReleaseMemObject(tsf_parameters_cl);
        if (data_view_extent_cl) clReleaseMemObject(data_view_extent_cl);
        if (data_extent_cl) clReleaseMemObject(data_extent_cl);
        if (bbox_extent_cl) clReleaseMemObject(bbox_extent_cl);
        if (view_matrix_inv_cl) clReleaseMemObject(view_matrix_inv_cl);
        if (function_view_matrix_inv_cl) clReleaseMemObject(function_view_matrix_inv_cl);

        if (oct_index_cl) clReleaseMemObject(oct_index_cl);
        if (oct_brick_cl) clReleaseMemObject(oct_brick_cl);

        if (bricks_sampler) clReleaseSampler(bricks_sampler);
        if (bricks_cl) clReleaseMemObject(bricks_cl);

        if (tsf_tex_sampler) clReleaseSampler(tsf_tex_sampler);
        if (tsf_tex_cl) clReleaseMemObject(tsf_tex_cl);

        if (K_FUNCTION_RAYTRACE) clReleaseKernel(K_FUNCTION_RAYTRACE);
        if (K_SVO_RAYTRACE) clReleaseKernel(K_SVO_RAYTRACE);

        //~ if (queue) clReleaseCommandQueue(queue);
        //~ if (context) clReleaseContext(context);
        if (program) clReleaseProgram(program);


        glDeleteFramebuffers(1, &STD_FBO);
        glDeleteFramebuffers(1, &MSAA_FBO);
        glDeleteFramebuffers(1, &SMALL_FBO);

        glDeleteTextures(1, &msaa_intermediate_storage_tex);
        glDeleteTextures(1, &small_storage_tex);
        glDeleteTextures(1, &storage_tex);
        glDeleteTextures(1, &glow_tex);
        glDeleteTextures(1, &mini_uc_tex);
        glDeleteTextures(1, &blend_tex);
        glDeleteTextures(1, &msaa_tex);
        glDeleteTextures(1, &msaa_depth_tex);
        glDeleteTextures(1, &std_2d_tex);
        glDeleteTextures(1, &std_3d_tex);
        glDeleteTextures(1, &ray_tex);
        glDeleteTextures(1, &tsf_tex);
        glDeleteTextures(1, &hist_tex);
        glDeleteTextures(1, &hist_tex_norm);
        glDeleteTextures(1, &hist_tex_log);

        glDeleteBuffers(10, tex_coord_vbo);
        glDeleteBuffers(20, position_vbo);
        glDeleteBuffers(5, lab_reference_vbo);
        glDeleteBuffers(5, data_extent_vbo);
        glDeleteBuffers(5, data_view_extent_vbo);
        glDeleteBuffers(5, unitcell_vbo);
        glDeleteBuffers(5, screen_vbo);
        glDeleteBuffers(1, &sampleWeightBuf);
        glDeleteBuffers(1, &text_position_vbo);
        glDeleteBuffers(1, &text_texpos_vbo);


        glDeleteProgram(std_2d_tex_program);
        glDeleteProgram(std_2d_color_program);
        glDeleteProgram(std_3d_program);
        glDeleteProgram(pp_glow_program);
        glDeleteProgram(blend_program);
        glDeleteProgram(msaa_program);
        glDeleteProgram(msaa_hdr_program);
        glDeleteProgram(std_text_program);

        if (verbose) emit appendLog( "VolumeRenderGLWidget-> Destruction done" );
    }

}

void VolumeRenderGLWidget::takeScreenshot()
{
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = QString("screens/Screenshot_"+dateTime.toString("yyyy_MM_dd_hh_mm_ss")+".bmp");
    screenshot(WIDTH, HEIGHT, dateTimeString.toStdString().c_str());
    setMessageString(QString("\n Saved: "+dateTimeString));
}
void VolumeRenderGLWidget::setResolutionf(double value)
{
    // Clamp
    if (value < 20) value = 20;
    if (value > 100) value = 100;
    if ((ray_res != value))
    {
        // Set resolution
        //~ std::cout << "Setting resolution to " << value << " %" << std::endl;
        ray_res = (float) value;

        // Limit the deepest SVO descent level
        //~ int level = LEVELS;
        //~ if (value <= 60) level = LEVELS - 1;
        //~ if (value <= 40) level = LEVELS - 2;
        if (value <= 20) this->setMinLevel(256.0);
        else if (value <= 30) this->setMinLevel(128.0);
        else if (value <= 40) this->setMinLevel(64.0);
        else if (value <= 50) this->setMinLevel(32.0);
        else if (value <= 60) this->setMinLevel(16.0);
        else if (value <= 70) this->setMinLevel(8.0);
        else if (value <= 80) this->setMinLevel(4.0);
        else if (value <= 90) this->setMinLevel(2.0);
        else this->setMinLevel(1.0);
        //~ if (level > LEVELS) level = LEVELS;
        //~ if (level < 2) level = 2;

        //~ std::cout << ray_res << ": "<< level << " / " << LEVELS << std::endl;


        this->gen_ray_tex();
        this->screen_buffer_refresh();
        //~ emit changedResolutioni( (int) value);
    }
}

void VolumeRenderGLWidget::init_freetype()
{
    /* Initialize the FreeType2 library */
    FT_Library ft;
    FT_Face face;
    FT_Error error;
    const char * fontfilename = "../../src/fonts/FreeMonoOblique.ttf";

    error = FT_Init_FreeType(&ft);
    if(error)
    {
        std::cout << "Could not init freetype library: " << std::endl;
    }
    /* Load a font */
    if(FT_New_Face(ft, fontfilename, 0, &face)) {
        std::cout << "Could not open font " << fontfilename << std::endl;
    }

    fontSmall = new Atlas(face, 12);
    fontMedium = new Atlas(face, 24);
    fontLarge = new Atlas(face, 48);
}

void VolumeRenderGLWidget::setResolutioni(int value)
{
    if (value > 0)
    {
        setResolutionf((double)value);
        //~ emit changedResolutionf( (double) value);
    }
}
//~ void VolumeRenderGLWidget::toggleFastMove(bool value)
//~ {
    //~ fastMove = value;
//~ }
void VolumeRenderGLWidget::togglePerspective()
{
    isPerspectiveRequired = !isPerspectiveRequired;
    CTC_MATRIX.setProjection(isPerspectiveRequired);
    //~ CTC_MATRIX.print(2);

    // Scale up/down a bit
    float f;
    if (isPerspectiveRequired) f = 1;
    else f = 2.3;
    PROJECTION_SCALING[0] = f;
    PROJECTION_SCALING[5] = f;
    PROJECTION_SCALING[10] = f;

    MISC_INT[5] = isPerspectiveRequired;
    this->setMiscArrays();

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}
void VolumeRenderGLWidget::toggleBackground()
{
    //~ std::cout << "Tiem for new BG!" << std::endl;

    MiniArray<float> tmp(4);
    tmp.setDeep(4,clear.data());


    clear.setDeep(4, clearInv.data());
    clearInv.setDeep(4, tmp.data());

    //~ if  (bricks_cl && oct_index_cl && oct_brick_cl && !isFunctionActive)
    //~ {
        //~ hist_tex_norm = getHistogramTexture(HIST_NORM, 100);
        //~ hist_tex_log = getHistogramTexture(HIST_LOG, 100);
    //~ }

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}

void VolumeRenderGLWidget::toggleFunctionView()
{
    isFunctionActive = !isFunctionActive;

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}
void VolumeRenderGLWidget::setFuncParamA(double value)
{
    MISC_FLOAT_K_FUNCTION[0] = value;
    this->setMiscArrays();

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}
void VolumeRenderGLWidget::setFuncParamB(double value)
{
    MISC_FLOAT_K_FUNCTION[1] = value;
    this->setMiscArrays();

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}
void VolumeRenderGLWidget::setFuncParamC(double value)
{
    MISC_FLOAT_K_FUNCTION[2] = value;
    this->setMiscArrays();

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}
void VolumeRenderGLWidget::setFuncParamD(double value)
{
    MISC_FLOAT_K_FUNCTION[3] = value;
    this->setMiscArrays();

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}

void VolumeRenderGLWidget::auto_rotate(int time, int threshold)
{
    if (time > threshold)
    {
        double roll = std::fmod(pi * (time - threshold) * 0.00001, pi*2.0);

        AUTO_ROTATION.setArbRotation(-0.5*pi, 0.5*pi, roll);

        this->timerLastAction->start();
        this->isRefreshRequired = true;
    }
    else AUTO_ROTATION.setIdentity(4);


}
void VolumeRenderGLWidget::view_matrix_reset()
{
    DATA_SCALING.setIdentity(4);
    ROTATION.setIdentity(4);
    DATA_TRANSLATION.setIdentity(4);
}

void VolumeRenderGLWidget::setOCT_INDEX(MiniArray<unsigned int> * OCT_INDEX, size_t n_levels, float * extent)
{
    //~ std::cout << "(3) setOCT_INDEX" << std::endl;
    //~
    this->OCT_INDEX = OCT_INDEX;
    this->LEVELS = n_levels;
    this->setMinLevel(1.0);
    this->setMiscArrays();
    //~
    this->DATA_EXTENT.setDeep(4, 2, extent);
    this->DATA_VIEW_EXTENT.setDeep(4, 2, extent);
    this->data_extent_refresh();
    this->vbo_buffers_refresh();
    this->view_matrix_reset();

    MISC_INT[0] = (int) LEVELS;
    //~
    //~
    //~ std::cout << "this->OCT_INDEX.size() " << this->OCT_INDEX->size() << std::endl;
    //~ std::cout << "this->LEVELS " << this->LEVELS << std::endl;
    //~ std::cout << "this->DATA_EXTENT: " << std::endl;
    //~ DATA_EXTENT.print(3);


    /* Load the contents into a CL texture */
    oct_index_cl = clCreateBuffer((*context2),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        this->OCT_INDEX->size()*sizeof(cl_uint),
        this->OCT_INDEX->data(),
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }

    /* Send to the kernel */
    err = clSetKernelArg(K_SVO_RAYTRACE, 3, sizeof(cl_mem), &oct_index_cl);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error setting kernel argument: " << cl_error_cstring(err) << std::endl;
    }
}

void VolumeRenderGLWidget::setMeta(MiniArray<double> * HIST_NORM, MiniArray<double> * HIST_LOG, MiniArray<double> * HIST_MINMAX, MiniArray<char> * SVO_COMMENT)
{
    this->HIST_NORM = HIST_NORM;
    this->HIST_LOG = HIST_LOG;
    getHistogramTexture(&hist_tex_norm, HIST_NORM, 100, white.data());
    getHistogramTexture(&hist_tex_log, HIST_LOG, 100, white.data());
    this->HIST_MINMAX[0] = HIST_MINMAX->at(0);
    this->HIST_MINMAX[1] = HIST_MINMAX->at(1);

    TSF_PARAMETERS[2] = 1.0;
    TSF_PARAMETERS[3] = this->HIST_MINMAX[1];
    // If the texture exists, it should be drawn!
}

void VolumeRenderGLWidget::getHistogramTexture(GLuint * tex, MiniArray<double> * buf, size_t height, float * color)
{
    //~ buf->print(2);

    double max = buf->max();

    Matrix<float> texture(height, buf->size()*4, 0.0);
    for (size_t i = 0; i < buf->size(); i++)
    {
        size_t span = (buf->at(i)*(double)height/max);
        for (size_t j = 0; j < span; j++)
        {
            texture[(i + j*buf->size())*4+0] = color[0];
            texture[(i + j*buf->size())*4+1] = color[1];
            texture[(i + j*buf->size())*4+2] = color[2];
            texture[(i + j*buf->size())*4+3] = (float)(j*0.5+span*0.5)/(float)span;
        }
    }


    glDeleteBuffers(1, tex);
    glGenBuffers(1, tex);

    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        texture.getN()/4,
        texture.getM(),
        0,
        GL_RGBA,
        GL_FLOAT,
        texture.data());
    glBindTexture(GL_TEXTURE_2D, 0);

}

void VolumeRenderGLWidget::setOCT_BRICK(MiniArray<unsigned int> * OCT_BRICK, size_t pool_power)
{
    //~ std::cout << "(4) setOCT_BRICK" << std::endl;

    this->OCT_BRICK = OCT_BRICK;
    this->BPP = pool_power;
    this->isFunctionActive = false;

    std::cout << "this->OCT_BRICK.size() " << this->OCT_BRICK->size() << std::endl;
    std::cout << "this->BPP " << this->BPP << std::endl;

    /* Load the contents into a CL texture */
    oct_brick_cl = clCreateBuffer((*context2),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        this->OCT_BRICK->size()*sizeof(cl_uint),
        this->OCT_BRICK->data(),
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }

    /* Send to the kernel */
    err = clSetKernelArg(K_SVO_RAYTRACE, 4, sizeof(cl_mem), &oct_brick_cl);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error setting kernel argument: " << cl_error_cstring(err) << std::endl;
    }

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}


void VolumeRenderGLWidget::brick_to_tex(float * buf_in, float * buf_out, size_t id, size_t brick_dim, size_t pool_power)
{
    // Bitwise operations used here:
    // X / 2^n = X >> n
    // X % 2^n = X & (2^n - 1)
    // 2^n = 1 << n

    size_t n_points_brick = brick_dim*brick_dim*brick_dim;
    size_t n_brick_pool = 1 << pool_power;
    size_t n_points_pool = brick_dim*n_brick_pool;
    size_t n_points_offset = id*n_points_brick;
    size_t brick_xyz[3];
    size_t remainder;
    size_t id_x, id_y, id_z, i = 0;

    remainder = id % (n_brick_pool*n_brick_pool);
    brick_xyz[2] = id / (n_brick_pool*n_brick_pool);
    brick_xyz[1] = remainder / n_brick_pool;
    brick_xyz[0] = remainder % n_brick_pool;

    for (id_z = brick_xyz[2]*brick_dim; id_z < (brick_xyz[2]+1)*brick_dim; id_z++)
    {
        for (id_y = brick_xyz[1]*brick_dim; id_y < (brick_xyz[1]+1)*brick_dim; id_y++)
        {
            for (id_x = brick_xyz[0]*brick_dim; id_x < (brick_xyz[0]+1)*brick_dim; id_x++)
            {
                buf_out[id_x + id_y*n_points_pool + id_z*n_points_pool*n_points_pool] = buf_in[i + n_points_offset];
                i++;
            }
        }
    }

    //~ std::cout << id << " brick_xyz = [" << brick_xyz[0] << " " << brick_xyz[1] << " " << brick_xyz[2] << "]" << std::endl;
}

void VolumeRenderGLWidget::setBRICKS(MiniArray<float> * BRICKS, size_t n_bricks, size_t dim_bricks)
{
    //~ std::cout << "(2, 5) setBRICKS" << std::endl;
    this->BRICKS = BRICKS;
    this->N_BRICKS = n_bricks;
    this->DIM_BRICKS = dim_bricks;

    MISC_INT[1] = (int) DIM_BRICKS;

    std::cout << "this->BRICKS.size() " << this->BRICKS->size() << std::endl;
    std::cout << "this->N_BRICKS " << this->N_BRICKS << std::endl;
    std::cout << "this->DIM_BRICKS " << this->DIM_BRICKS << std::endl;

    MiniArray<float> tex_buf;

    size_t tex_buf_dim[3];
    tex_buf_dim[0] = (1 << BPP)*DIM_BRICKS;
    tex_buf_dim[1] = (1 << BPP)*DIM_BRICKS;
    tex_buf_dim[2] = (N_BRICKS) / ((1 << BPP)*(1 << BPP));

    if ((N_BRICKS) % ((1 << BPP)*(1 << BPP))) tex_buf_dim[2]++;
    if (tex_buf_dim[2] < 2) tex_buf_dim[2] = 2;
    tex_buf_dim[2] *= (DIM_BRICKS);

    tex_buf.reserve(tex_buf_dim[0]*tex_buf_dim[1]*tex_buf_dim[2]);

    /* Rearrange bricks for a 3D texture*/
    for (size_t i = 0; i < N_BRICKS; i++)
    {
        brick_to_tex(this->BRICKS->data(), tex_buf.data(), i, DIM_BRICKS, BPP);
    }

    /* Load the contents into a CL texture */
    bricks_format.image_channel_order = CL_INTENSITY;
    bricks_format.image_channel_data_type = CL_FLOAT;

    std::cout << tex_buf_dim[0] << " " << tex_buf_dim[2] << " " << std::endl;

    bricks_cl = clCreateImage3D ( (*context2),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        &bricks_format,
        tex_buf_dim[0],
        tex_buf_dim[1],
        tex_buf_dim[2], // This is padded according to spec
        tex_buf_dim[0]*sizeof(cl_float),
        tex_buf_dim[0]*tex_buf_dim[1]*sizeof(cl_float),
        tex_buf.data(),
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }

    bricks_sampler = clCreateSampler((*context2), CL_TRUE, CL_ADDRESS_CLAMP, CL_FILTER_LINEAR, &err);

    /* Send to the kernel */
    err = clSetKernelArg(K_SVO_RAYTRACE, 2, sizeof(cl_mem), &bricks_cl);
    err |= clSetKernelArg(K_SVO_RAYTRACE, 5, sizeof(cl_sampler), &bricks_sampler);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error setting kernel argument: " << cl_error_cstring(err) << std::endl;
    }

    this->setMiscArrays();
}

QSize VolumeRenderGLWidget::minimumSizeHint() const
{
    return QSize(100, 100);
}

QSize VolumeRenderGLWidget::sizeHint() const
{
    return QSize(2000, 2000);
}

void VolumeRenderGLWidget::initializeGL()
{
    //~std::cout << " Initializing OpenGL and CL" << std::endl;
    setMouseTracking( true );
    if (!this->init_gl()) std::cout << "Error initializing OpenGL" << std::endl;
    if (!this->init_cl()) std::cout << "VolumeRenderGLWidget: OpenCL context could not be initialized!" << std::endl;

    /* Initialize and set the other stuff */
    this->init_freetype();
    init_tsf(0, 0, &transferFunction);
    this->gen_tsf_tex(&transferFunction);
    this->data_extent_refresh();
    this->setTsfParameters();
    this->setMiscArrays();
    this->setEmit();

    isGLIntitialized = true;
    //~std::cout << "Done Initializing OpenGL and CL " << std::endl;
}
void VolumeRenderGLWidget::setEmit()
{
    emit changedDataMinValue(10.0);
    emit changedDataMaxValue(1000.0);
    emit changedAlphaValue(0.5);
    emit changedBrightnessValue(1.5);
    emit changedFuncParamA(13.5);
    emit changedFuncParamB(10.5);
    emit changedFuncParamC(10.0);
    emit changedFuncParamD(0.001);
}


void VolumeRenderGLWidget::setProjection(double F, double N, double fov, bool isPerspectiveRequired)
{
    CTC_MATRIX.setN(N);
    CTC_MATRIX.setF(F);
    CTC_MATRIX.setFov(fov);
    CTC_MATRIX.setProjection(isPerspectiveRequired);
}

void VolumeRenderGLWidget::setMinLevel(float level)
{
    //~ std::cout << "Setting value " << level << std::endl;
    MISC_FLOAT_K_RAYTRACE[4] = level;
    this->setMiscArrays();
}

void VolumeRenderGLWidget::toggleLog()
{
    isLog = !isLog;
    MISC_INT[2] = isLog;
    this->setMiscArrays();

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}
void VolumeRenderGLWidget::toggleDataStructure()
{
    MISC_INT[3] = !MISC_INT[3];
    this->setMiscArrays();

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}

void VolumeRenderGLWidget::setTsfParameters()
{
    if ((*queue))
    {
        //~ MiniArray<float> BUF(TSF_PARAMETERS.size());
        //~ BUF.setDeep(BUF.size(), TSF_PARAMETERS.data());

        err = clEnqueueWriteBuffer ( (*queue),
            tsf_parameters_cl,
            CL_TRUE,
            0,
            TSF_PARAMETERS.size()*sizeof(cl_float),
            TSF_PARAMETERS.data(),
            0,0,0);
        if (err != CL_SUCCESS)
        {
            std::cout << "Error writing to CL buffer 'tsf_parameters_cl': " << cl_error_cstring(err) << std::endl;
        }

        err = clSetKernelArg(K_FUNCTION_RAYTRACE, 6, sizeof(cl_mem), &tsf_parameters_cl);
        err |= clSetKernelArg(K_SVO_RAYTRACE, 10, sizeof(cl_mem), &tsf_parameters_cl);
        if (err != CL_SUCCESS)
        {
            std::cout << "Error setting kernel argument 'tsf_parameters_cl': " << cl_error_cstring(err) << std::endl;
        }
        //~ {
            //~ Matrix<float> tsf_texcoords;
            //~ float buf2[] = {
                //~ TSF_PARAMETERS[0],TSF_PARAMETERS[1],TSF_PARAMETERS[1],TSF_PARAMETERS[0],
                //~ 0.0,0.0,1.0,1.0};
            //~ tsf_texcoords.setDeep(2, 4, buf2);
            //~ setVbo(&tex_coord_vbo[2], tsf_texcoords.getColMajor().data(), tsf_texcoords.size());
        //~ }

        //~ this->gen_tsf_tex(&transferFunction);
    }
}

void VolumeRenderGLWidget::setMiscArrays()
{
    if ((*queue))
    {
        err = clEnqueueWriteBuffer ( (*queue),
            misc_int_cl,
            CL_TRUE,
            0,
            MISC_INT.size()*sizeof(cl_float),
            MISC_INT.data(),
            0,0,0);
        if (err != CL_SUCCESS)
        {
            std::cout << "Error writing to CL buffer 'misc_int_cl': " << cl_error_cstring(err) << std::endl;
        }

        err = clEnqueueWriteBuffer ( (*queue),
            misc_float_cl,
            CL_TRUE,
            0,
            MISC_FLOAT_K_FUNCTION.size()*sizeof(cl_float),
            MISC_FLOAT_K_FUNCTION.data(),
            0,0,0);
        if (err != CL_SUCCESS)
        {
            std::cout << "Error writing to CL buffer 'misc_float_cl': " << cl_error_cstring(err) << std::endl;
        }

        err = clEnqueueWriteBuffer ( (*queue),
            misc_float_k_raytrace_cl,
            CL_TRUE,
            0,
            MISC_FLOAT_K_RAYTRACE.size()*sizeof(cl_float),
            MISC_FLOAT_K_RAYTRACE.data(),
            0,0,0);
        if (err != CL_SUCCESS)
        {
            std::cout << "Error writing to CL buffer 'misc_float_cl': " << cl_error_cstring(err) << std::endl;
        }

        err = clSetKernelArg(K_FUNCTION_RAYTRACE, 7, sizeof(cl_mem), &misc_float_cl);
        err |= clSetKernelArg(K_FUNCTION_RAYTRACE, 8, sizeof(cl_mem), &misc_int_cl);
        err |= clSetKernelArg(K_SVO_RAYTRACE, 11, sizeof(cl_mem), &misc_float_k_raytrace_cl);
        err |= clSetKernelArg(K_SVO_RAYTRACE, 12, sizeof(cl_mem), &misc_int_cl);
        if (err != CL_SUCCESS)
        {
            std::cout << "Error setting kernel argument 'misc_float/int_cl': " << cl_error_cstring(err) << std::endl;
        }
    }
}

void VolumeRenderGLWidget::histTexPos(int log, float hist_min, float hist_max, float data_min, float data_max)
{
    Matrix<float> position;
    {
        float x_low;
        float x_high;

        if (log)
        {
            if (hist_min < 1) hist_min = 1;
            hist_min = std::log10(hist_min);

            if (hist_max < 1) hist_max = 1;
            hist_max = std::log10(hist_max);


            if (data_min <= 0.0) data_min = 0.0001;
            if (data_max <= 0.0) data_max = 0.0001;

            x_low = (std::log10(data_min) - hist_min)/(hist_max - hist_min);
            x_high = (std::log10(data_max) - hist_min)/(hist_max - hist_min);

            //~ std::cout << "data: "<< std::log10(data_min) << ", " << std::log10(data_max) << " hist: " << hist_min << ", " << hist_max << std::endl;
        }
        else
        {
            x_low = (data_min - hist_min)/(hist_max - hist_min);
            x_high = (data_max - hist_min)/(hist_max - hist_min);
        }

        // Clamp
        //~ if (x_low < 0.0) x_low = 0.0;
        //~ if (x_low > 1.0) x_low;
        //~ if () ;
        //~ if () ;

        float buf[] = {
            x_low,x_high,x_high,x_low,
            0,0,1.0,1.0};
        position.setDeep(2, 4, buf);

        //~ position.print(2);
    }
    setVbo(&tex_coord_vbo[3], position.getColMajor().data(), position.size());
}

void VolumeRenderGLWidget::backDropTexPos(GLuint * vbo, int border_pixel_offset)
{
    Matrix<float> tex_position;

    // screen coordinate pixel size, scps
    float scps[2];
    scps[0] = 2.0/WIDTH;
    scps[1] = 2.0/HEIGHT;

    float buf[] = {
        0.9, 0.9, 0.95, 0.95,   0.9, 0.9, 0.8, 0.8,     1.0, 1.0, 0.83, 0.83,         0.83, 0.83,
        -0.9, 0.9, 0.9, -0.9,   -0.9, 0.9, 0.9, -0.9,   -0.90, 0.90, 0.90, -0.90,   0.90, -0.90};
    buf[12] -= scps[0]*border_pixel_offset;
    buf[13] -= scps[0]*border_pixel_offset;
    buf[22] -= scps[1]*border_pixel_offset;
    buf[23] += scps[1]*border_pixel_offset;
    buf[24] += scps[1]*border_pixel_offset;
    buf[25] -= scps[1]*border_pixel_offset;

    tex_position.setDeep(2, 14, buf);
    setVbo(vbo, tex_position.getColMajor().data(), tex_position.size());
}

void VolumeRenderGLWidget::pixPos(GLuint * vbo, float x, float y, float w, float h)
{
    Matrix<float> tex_position;

    w *= 2.0/WIDTH;
    h *= 2.0/HEIGHT;

    float buf[] = {
        x, x+w, x+w, x,
        y, y, y+h, y+h};

    tex_position.setDeep(2, 4, buf);
    setVbo(vbo, tex_position.getColMajor().data(), tex_position.size());
}

void VolumeRenderGLWidget::setMatrixU(float * buf)
{
    this->U[0] = buf[0];
    this->U[1] = buf[1];
    this->U[2] = buf[2];
    this->U[4] = buf[3];
    this->U[5] = buf[4];
    this->U[6] = buf[5];
    this->U[8] = buf[6];
    this->U[9] = buf[7];
    this->U[10] = buf[8];

    U3x3.setDeep(3,3,buf);
}

void VolumeRenderGLWidget::setMatrixB(float * buf)
{
    Matrix<float> B(3,3);
    B.setDeep(3,3,buf);

    //~ B.print(2);

    Matrix<float> a(3,1,0);
    a[0] = 1;
    Matrix<float> b(3,1,0);
    b[1] = 1;
    Matrix<float> c(3,1,0);
    c[2] = 1;

    a = B*a;
    b = B*b;
    c = B*c;

    this->a.setDeep(3,1,a.data());
    this->b.setDeep(3,1,b.data());
    this->c.setDeep(3,1,c.data());

    isUnitcellValid = true;
    isUnitcellActive = true;
    //~ a.print(5);
    //~ b.print(5);
    //~ c.print(5);

    int hkl_low[3] = {-20,-20,-20};
    int hkl_high[3] = {20,20,20};
    hkl_text_pos.set(((hkl_high[0]-hkl_low[0] + 1) * (hkl_high[1]-hkl_low[1] + 1) * (hkl_high[2]-hkl_low[2] + 1)),4,1);
    hkl_text_index.set(((hkl_high[0]-hkl_low[0] + 1) * (hkl_high[1]-hkl_low[1] + 1) * (hkl_high[2]-hkl_low[2] + 1)),4,1);
    int n = getUnitcellVBO(&unitcell_vbo[0], hkl_low, hkl_high, this->a.data(), this->b.data(), this->c.data());


    hkl_indices.reserve(6*n);
    for (int i = 0; i < n; i++)
    {
        hkl_indices[i*6] = i*4;
        hkl_indices[i*6+1] = i*4+1;
        hkl_indices[i*6+2] = i*4;
        hkl_indices[i*6+3] = i*4+2;
        hkl_indices[i*6+4] = i*4;
        hkl_indices[i*6+5] = i*4+3;
    }
}

void VolumeRenderGLWidget::toggleUnitcellView()
{
    isUnitcellActive = !isUnitcellActive;
}

int VolumeRenderGLWidget::getUnitcellVBO(GLuint * xyz_coords, int * hkl_low, int * hkl_high, float * a, float * b, float * c)
{
    int n = (hkl_high[0]-hkl_low[0] + 1) * (hkl_high[1]-hkl_low[1] + 1) * (hkl_high[2]-hkl_low[2] + 1);

    float * buf = new float[n*4*3];

    int hkl_offset[3];
    int offset = 0;
    int iter = 0;

    for (int h = hkl_low[0]; h <= hkl_high[0]; h++)
    {
        for (int k = hkl_low[1]; k <= hkl_high[1]; k++)
        {
            for (int l = hkl_low[2]; l <= hkl_high[2]; l++)
            {
                //~ qDebug() << "hkl = " << h << " " << k << " " << l;

                hkl_offset[0] = h;
                hkl_offset[1] = k;
                hkl_offset[2] = l;

                getUnitcellBasis(buf+offset, hkl_offset, a, b, c);

                hkl_text_pos[iter*3+0] = buf[offset+0];
                hkl_text_pos[iter*3+1] = buf[offset+1];
                hkl_text_pos[iter*3+2] = buf[offset+2];

                hkl_text_index[iter*3+0] = h;
                hkl_text_index[iter*3+1] = k;
                hkl_text_index[iter*3+2] = l;
                iter++;
                offset += 4*3;
            }
        }
    }

    setVbo(xyz_coords, buf, n*4*3);
    //~ delete[] buf;
    return n;
}

void VolumeRenderGLWidget::getUnitcellBasis(float * buf, int * hkl_offset, float * a, float * b, float * c)
{
    Matrix<float> origo(3,1);
    Matrix<float> h(3,1);
    Matrix<float> k(3,1);
    Matrix<float> l(3,1);

    origo[0] = (a[0]*hkl_offset[0] + b[0]*hkl_offset[1] + c[0]*hkl_offset[2]);
    origo[1] = (a[1]*hkl_offset[0] + b[1]*hkl_offset[1] + c[1]*hkl_offset[2]);
    origo[2] = (a[2]*hkl_offset[0] + b[2]*hkl_offset[1] + c[2]*hkl_offset[2]);

    h[0] = origo[0] + a[0];
    h[1] = origo[1] + a[1];
    h[2] = origo[2] + a[2];

    k[0] = origo[0] + b[0];
    k[1] = origo[1] + b[1];
    k[2] = origo[2] + b[2];

    l[0] = origo[0] + c[0];
    l[1] = origo[1] + c[1];
    l[2] = origo[2] + c[2];

    origo = U3x3*origo;
    h = U3x3*h;
    k = U3x3*k;
    l = U3x3*l;

    buf[0] = origo[0];
    buf[1] = origo[1];
    buf[2] = origo[2];

    buf[3] = h[0];
    buf[4] = h[1];
    buf[5] = h[2];

    buf[6] = k[0];
    buf[7] = k[1];
    buf[8] = k[2];

    buf[9] = l[0];
    buf[10] = l[1];
    buf[11] = l[2];

}
//~
//~ void VolumeRenderGLWidget::getColorLine(float * buf, float * a, float * b, float w_scale)
//~ {
    //~ Matrix<float> A(3);
    //~ A.setDeep(3,a);
    //~
    //~ Matrix<float> B(3);
    //~ A.setDeep(3,b);
//~
    //~ Matrix<float> delta(3);
    //~
    //~ delta = B - A;
    //~
//~
    //~ Matrix<float> VERTS(3,4*2);
//~ }

void VolumeRenderGLWidget::setTsf(int value)
{
    tsf_style = value;
    init_tsf(value, tsf_alpha_style, &transferFunction);
    this->gen_tsf_tex(&transferFunction);

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}


void VolumeRenderGLWidget::setTsfAlphaStyle(int value)
{
    tsf_alpha_style = value;
    init_tsf(tsf_style, value, &transferFunction);
    this->gen_tsf_tex(&transferFunction);

    this->timerLastAction->start();
    this->isRefreshRequired = true;
};

void VolumeRenderGLWidget::paintGL()
{
    glEnable(GL_MULTISAMPLE);
    QElapsedTimer paintTimer;
    paintTimer.start();

    this->auto_rotate(rotationTimer->elapsed(), auto_rotation_delay);
    this->view_matrix_refresh();
    this->vbo_buffers_refresh();

    GLuint indices[6] = {0,1,3,1,2,3};

    /*
     *  Draw GL_LINES to std_3d_tex
     * */
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, STD_FBO);
        GLenum buffers[] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, buffers);

        glClearColor(transparent[0], transparent[1], transparent[2], transparent[3]);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glLineWidth(LINEWIDTH);

        MiniArray<float> bbox_min(3), bbox_max(3);
        bbox_min[0] = DATA_VIEW_EXTENT[0];
        bbox_min[1] = DATA_VIEW_EXTENT[2];
        bbox_min[2] = DATA_VIEW_EXTENT[4];
        bbox_max[0] = DATA_VIEW_EXTENT[1];
        bbox_max[1] = DATA_VIEW_EXTENT[3];
        bbox_max[2] = DATA_VIEW_EXTENT[5];

        // Draw the unitcell
        if (isUnitcellValid && isUnitcellActive)
        {
            {
                color.setDeep(4, clearInv.data());

                float len_a = std::sqrt(this->a[0]*this->a[0]+ this->a[1]*this->a[1]+ this->a[2]*this->a[2]);
                float len_b = std::sqrt(this->b[0]*this->b[0]+ this->b[1]*this->b[1]+ this->b[2]*this->b[2]);
                float len_c = std::sqrt(this->c[0]*this->c[0]+ this->c[1]*this->c[1]+ this->c[2]*this->c[2]);

                float len_max = std::max(len_a,(std::max(len_b, len_c)));
                color[3] = (15 - ((DATA_VIEW_EXTENT[1] - DATA_VIEW_EXTENT[0]) / len_max))/15*0.7  ;
                if (color[3] > 0.7) color[3] = 0.7;
                if (color[3] < 0.0) color[3] = 0.0;
            }

            std_3d_color_draw(hkl_indices.data(), hkl_indices.size(), color.data(), &unitcell_vbo[0] , CELL_VIEW_MATRIX.getColMajor().data(), I.getColMajor().data(), bbox_min.data(), bbox_max.data() );
        }

        // Draw bounding boxes
        GLuint elements[24] = {0,1, 1,2, 2,3, 3,0, 1,5, 2,6, 3,7, 0,4, 5,6, 6,7, 7,4, 4,5};
        color.setDeep(4, clearInv.data());
        color[3] = 0.4;//*(DATA_VIEW_EXTENT[1]-DATA_VIEW_EXTENT[0])/(DATA_EXTENT[1]-DATA_EXTENT[0]);
        //~ if (color[3] > 0.4) color[3] = 0.4;
        //~ if (color[3] < 0.0) color[3] = 0.0;

        if (!isFunctionActive)
        {
            std_3d_color_draw(elements, 24, color.data(), &data_extent_vbo[0] , DATA_VIEW_MATRIX.getColMajor().data(), I.getColMajor().data(), bbox_min.data(), bbox_max.data() );
            std_3d_color_draw(elements, 24, color.data(), &data_view_extent_vbo[0] , DATA_VIEW_MATRIX.getColMajor().data(), I.getColMajor().data(), bbox_min.data(), bbox_max.data() );
        }
        else
        {
            std_3d_color_draw(elements, 24, color.data(), &data_view_extent_vbo[0] , (DATA_VIEW_MATRIX).getColMajor().data(), I.getColMajor().data(), bbox_min.data(), bbox_max.data() );
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }



    /*
     *  Draw GL_LINES to mini_uc_tex
     * */
    {
        glLineWidth(LINEWIDTH*16);
        glViewport(0, 0, SMALL_WIDTH, SMALL_HEIGHT);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, SMALL_FBO);
        GLenum buffers[] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, buffers);

        //~ glClearColor(clearInv[0], clearInv[1], clearInv[2], clearInv[3]);
        glClearColor(transparent[0], transparent[1], transparent[2], transparent[3]);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        if (isUnitcellValid)
        {
            MiniArray<float> bbox_min(3), bbox_max(3);
            bbox_min[0] = -10000;
            bbox_min[1] = -10000;
            bbox_min[2] = -10000;
            bbox_max[0] = 10000;
            bbox_max[1] = 10000;
            bbox_max[2] = 10000;

            int hkl_low[3] = {0,0,0};
            int hkl_high[3] = {0,0,0};

            float len_a = std::sqrt(this->a[0]*this->a[0]+ this->a[1]*this->a[1]+ this->a[2]*this->a[2]);
            float len_b = std::sqrt(this->b[0]*this->b[0]+ this->b[1]*this->b[1]+ this->b[2]*this->b[2]);
            float len_c = std::sqrt(this->c[0]*this->c[0]+ this->c[1]*this->c[1]+ this->c[2]*this->c[2]);

            float len_max = std::max(len_a,(std::max(len_b, len_c)));

            getUnitcellVBO(&unitcell_vbo[1], hkl_low, hkl_high, (this->a*(0.4/len_max)).data(), (this->b*(0.4/len_max)).data(), (this->c*(0.4/len_max)).data());

            /* Reciprocal unitcell vectors*/
            std_3d_color_draw(hkl_indices.data(), 2, red.data(), &unitcell_vbo[1]  , MINI_CELL_VIEW_MATRIX.getColMajor().data(), I.getColMajor().data(), bbox_min.data(), bbox_max.data() );

            std_3d_color_draw(hkl_indices.data()+2, 2, green.data(), &unitcell_vbo[1], MINI_CELL_VIEW_MATRIX.getColMajor().data(), I.getColMajor().data(), bbox_min.data(), bbox_max.data() );

            std_3d_color_draw(hkl_indices.data()+4, 2, blue.data(), &unitcell_vbo[1], MINI_CELL_VIEW_MATRIX.getColMajor().data(), I.getColMajor().data(), bbox_min.data(), bbox_max.data() );

            /* text for h k l*/
            Matrix<float> xy(2,1);
            Matrix<float> MINI_CELL_VIEW_MATRIX_U(4,4);
            MINI_CELL_VIEW_MATRIX_U = MINI_CELL_VIEW_MATRIX*U;

            getScreenPosition(xy.data(), (this->a*(0.4/len_max)).data(), MINI_CELL_VIEW_MATRIX_U.data());
            std_text_draw("h", fontLarge, clearInv.data(), xy.data(), 1.0, this->SMALL_WIDTH, this->SMALL_HEIGHT);
            getScreenPosition(xy.data(), (this->b*(0.4/len_max)).data(), MINI_CELL_VIEW_MATRIX_U.data());
            std_text_draw("k", fontLarge, clearInv.data(), xy.data(), 1.0, this->SMALL_WIDTH, this->SMALL_HEIGHT);
            getScreenPosition(xy.data(), (this->c*(0.4/len_max)).data(), MINI_CELL_VIEW_MATRIX_U.data());
            std_text_draw("l", fontLarge, clearInv.data(), xy.data(), 1.0, this->SMALL_WIDTH, this->SMALL_HEIGHT);
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, WIDTH, HEIGHT);
    }

    /*
     *  Draw Textures and HUD to std_2d_tex
     * */
    {
        glLineWidth(LINEWIDTH);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, STD_FBO);
        GLenum buffers[] = {GL_COLOR_ATTACHMENT1};
        glDrawBuffers(1, buffers);

        glClearColor(transparent[0], transparent[1], transparent[2], transparent[3]);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        GLuint indices[6] = {0,1,3,1,2,3};
        GLuint indices_side_backdrop[24] = { 8,9,11,9,10,11, 10,11,12, 11,13,12};


        backDropTexPos(&position_vbo[7], 6);
        backDropTexPos(&position_vbo[8], 4);
        backDropTexPos(&position_vbo[9], 2);


        {
            if (bricks_cl && oct_index_cl && oct_brick_cl && !isFunctionActive) this->ray_tex_refresh(K_SVO_RAYTRACE);
            else this->ray_tex_refresh(K_FUNCTION_RAYTRACE);

            std_2d_tex_draw(indices, 6, 0, ray_tex, &position_vbo[1], &tex_coord_vbo[1]);

            glBlendFunc(GL_ONE, GL_ZERO);
            std_2d_color_draw(indices_side_backdrop, 12, black.data(),  &position_vbo[7]);
            std_2d_color_draw(indices_side_backdrop, 12, white.data(),  &position_vbo[8]);
            std_2d_color_draw(indices_side_backdrop, 12, black.data(),  &position_vbo[9]);
            std_2d_color_draw(indices, 6, white.data(),  &position_vbo[10]);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            std_2d_tex_draw(indices, 6, 0, tsf_tex, &position_vbo[2], &tex_coord_vbo[2]);

            histTexPos(isLog, (float) HIST_MINMAX[0], (float) HIST_MINMAX[1], TSF_PARAMETERS[2], TSF_PARAMETERS[3]);
            //~ this->setTsfParameters();

            if (isLog) std_2d_tex_draw(indices, 6, 0, hist_tex_log, &position_vbo[3], &tex_coord_vbo[3]);
            else std_2d_tex_draw(indices, 6, 0, hist_tex_norm, &position_vbo[3], &tex_coord_vbo[3]);

            if (isUnitcellValid && isUnitcellActive)
            {
                // Draw hkl text
                Matrix<float> xy(2,1);
                MiniArray<int> hkl(4*4*4);
                size_t iter = 0;
                bool isIterFat = false;

                for (size_t i = 0; i < hkl_indices.size()/6; i++)
                {
                    if (
                    ((hkl_text_pos[i*3+0] > DATA_VIEW_EXTENT[0]) && (hkl_text_pos[i*3+0] < DATA_VIEW_EXTENT[1])) &&
                    ((hkl_text_pos[i*3+1] > DATA_VIEW_EXTENT[2]) && (hkl_text_pos[i*3+1] < DATA_VIEW_EXTENT[3])) &&
                    ((hkl_text_pos[i*3+2] > DATA_VIEW_EXTENT[4]) && (hkl_text_pos[i*3+2] < DATA_VIEW_EXTENT[5])))
                    {
                        if (iter >= 4*4*4)
                        {
                            isIterFat = true;
                            break;
                        }
                        hkl[iter] = i;
                        iter++;
                    }
                }
                if (!isIterFat)
                {
                    for (size_t i = 0; i < iter; i++)
                    {

                        std::stringstream ss;
                        ss << hkl_text_index[hkl[i]*3+0] << hkl_text_index[hkl[i]*3+1] << hkl_text_index[hkl[i]*3+2];

                        getScreenPosition(xy.data(), hkl_text_pos.data() + hkl[i]*3 , CELL_VIEW_MATRIX.data());
                        std_text_draw(ss.str().c_str(), fontMedium, clearInv.data(), xy.data(), 1.0, this->WIDTH, this->HEIGHT);
                    }
                }
            }
        }
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    // Set resolution seamlessly in accordance with requirements
    if (ray_res != 100) this->isRefreshRequired = true;
    if (timerLastAction->elapsed() >= timeLastActionMin)
    {
        this->setResolutionf(ray_res + 10);
    }
    else if (isBadCall)
    {
        this->setResolutionf(ray_res - 10);
    }
    if (!isBadCall)
    {
        glClearColor(clear[0], clear[0], clear[0], clear[0]);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        std_2d_tex_draw(indices, 6, 0, std_3d_tex, &screen_vbo[4], &tex_coord_vbo[1]);
        std_2d_tex_draw(indices, 6, 0, std_2d_tex, &screen_vbo[4], &tex_coord_vbo[1]);
        std_2d_tex_draw(indices, 6, 0, mini_uc_tex, &screen_vbo[0], &tex_coord_vbo[1]);
    }
}

void VolumeRenderGLWidget::setHklFocus(const QString str)
{
    QRegExp regExp("(?:\\D+)?([-+]?\\d+)(?:\\D+)?([-+]?\\d+)(?:\\D+)?([-+]?\\d+)");

    int pos = 0;
    pos = regExp.indexIn(str, pos);
    if ((pos > -1) && isUnitcellValid)
    {

        QString value = regExp.cap(1);
        float h = value.toFloat();
        value = regExp.cap(2);
        float k = value.toFloat();
        value = regExp.cap(3);
        float l = value.toFloat();
        value = regExp.cap(0);

        Matrix<float> xyzw(4,1);

        xyzw[0] = h * (this->a[0] + this->b[0] + this->c[0]);
        xyzw[1] = k * (this->a[1] + this->b[1] + this->c[1]);
        xyzw[2] = l * (this->a[2] + this->b[2] + this->c[2]);
        xyzw[3] = 1.0;
        //~ qDebug() << pos << ": "<< value << " <->" << h << " " << k << " " << l << " ";


        xyzw = U*xyzw;

        DATA_TRANSLATION.setIdentity(4);
        DATA_TRANSLATION[3] = -xyzw[0];
        DATA_TRANSLATION[7] = -xyzw[1];
        DATA_TRANSLATION[11] = -xyzw[2];

        DATA_VIEW_EXTENT =  (DATA_SCALING * DATA_TRANSLATION).getInverse() * DATA_EXTENT;
        //~ DATA_TRANSLATION = ( ROTATION.getInverse() * DATA_TRANSLATION * ROTATION) * DATA_TRANSLATION_PREV;
        this->timerLastAction->start();
        this->isRefreshRequired = true;
    }

}

void VolumeRenderGLWidget::std_3d_color_draw(GLuint * elements, int num_elements, GLfloat * color, GLuint * xyz_vbo, float * M1, float * M2, float * bbox_min, float * bbox_max )
{
        glUseProgram(std_3d_program);
        glEnableVertexAttribArray(std_3d_attribute_position);

        // Set std_3d_attribute_position
        glBindBuffer(GL_ARRAY_BUFFER, xyz_vbo[0]);
        glVertexAttribPointer(std_3d_attribute_position, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Set std_3d_uniform_transform
        glUniformMatrix4fv(std_3d_uniform_transform, 1, GL_FALSE, M1);

        // Set std_3d_uniform_u
        glUniformMatrix4fv(std_3d_uniform_u, 1, GL_FALSE, M2);

        // Set color
        glUniform4fv(std_3d_uniform_color, 1, color);

        // Set bbox
        glUniform3fv(std_3d_uniform_bbox_min, 1, bbox_min);
        glUniform3fv(std_3d_uniform_bbox_max, 1, bbox_max);

        // Draw verices
        glDrawElements(GL_LINES,  num_elements,  GL_UNSIGNED_INT,  elements);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDisableVertexAttribArray(std_3d_attribute_position);
        glUseProgram(0);
}

void VolumeRenderGLWidget::std_2d_tex_draw(GLuint * elements, int num_elements, int active_tex, GLuint texture, GLuint * xy_coords, GLuint * tex_coords)
{
    glUseProgram(std_2d_tex_program);

    // Set std_2d_tex_uniform_texture
    glActiveTexture(GL_TEXTURE0 + active_tex);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(std_2d_tex_uniform_texture, active_tex);

    // Set std_2d_tex_attribute_position
    glEnableVertexAttribArray(std_2d_tex_attribute_position);
    glBindBuffer(GL_ARRAY_BUFFER, *xy_coords);
    glVertexAttribPointer(std_2d_tex_attribute_position, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Set std_2d_tex_attribute_texpos
    glEnableVertexAttribArray(std_2d_tex_attribute_texpos);
    glBindBuffer(GL_ARRAY_BUFFER, *tex_coords);
    glVertexAttribPointer(std_2d_tex_attribute_texpos, 2, GL_FLOAT,  GL_FALSE,   0,  0 );
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Draw verices
    glDrawElements(GL_TRIANGLES,  num_elements,  GL_UNSIGNED_INT,  elements);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDisableVertexAttribArray(std_2d_tex_attribute_position);
    glDisableVertexAttribArray(std_2d_tex_attribute_texpos);
    glUseProgram(0);
}


void VolumeRenderGLWidget::std_2d_color_draw(GLuint * elements, int num_elements, GLfloat * color, GLuint * xy_coords)
{
    glUseProgram(std_2d_color_program);


    // Set std_2d_color_attribute_position
    glEnableVertexAttribArray(std_2d_color_attribute_position);
    glBindBuffer(GL_ARRAY_BUFFER, *xy_coords);
    glVertexAttribPointer(std_2d_color_attribute_position, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Set std_2d_color_uniform_color
    glUniform4fv(std_2d_color_uniform_color, 1, color);

    // Draw verices
    glDrawElements(GL_TRIANGLES,  num_elements,  GL_UNSIGNED_INT,  elements);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDisableVertexAttribArray(std_2d_color_attribute_position);
    glUseProgram(0);
}



void VolumeRenderGLWidget::resizeGL(int w, int h)
{
    //~ std::cout << "resizeGL" << std::endl;
    this->WIDTH = w;
    this->HEIGHT = h;
    this->SMALL_WIDTH = w/2;
    this->SMALL_HEIGHT = h/2;
    this->gen_ray_tex();
    this->screen_buffer_refresh();

    CTC_MATRIX.setWindow(WIDTH, HEIGHT);

    glViewport(0, 0, w, h);
    this->timerLastAction->start();
    this->isRefreshRequired = true;
}

void VolumeRenderGLWidget::data_extent_refresh()
{
    err = clEnqueueWriteBuffer ( (*queue),
        data_extent_cl,
        CL_TRUE,
        0,
        DATA_EXTENT.size()*sizeof(cl_float),
        DATA_EXTENT.data(),
        0,0,0);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error writing to CL buffer 'data_extent_cl': " << cl_error_cstring(err) << std::endl;
    }

    err = clEnqueueWriteBuffer ( (*queue),
        data_view_extent_cl,
        CL_TRUE,
        0,
        DATA_VIEW_EXTENT.size()*sizeof(cl_float),
        DATA_VIEW_EXTENT.data(),
        0,0,0);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error writing to CL buffer 'data_view_extent_cl': " << cl_error_cstring(err) << std::endl;
    }

    err = clSetKernelArg(K_FUNCTION_RAYTRACE, 4, sizeof(cl_mem),  &data_extent_cl);
    err |= clSetKernelArg(K_FUNCTION_RAYTRACE, 5, sizeof(cl_mem), &data_view_extent_cl);

    err |= clSetKernelArg(K_SVO_RAYTRACE, 8, sizeof(cl_mem),  &data_extent_cl);
    err |= clSetKernelArg(K_SVO_RAYTRACE, 9, sizeof(cl_mem), &data_view_extent_cl);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error setting kernel argument: " << cl_error_cstring(err) << std::endl;
    }
}


void VolumeRenderGLWidget::view_matrix_refresh()
{

    NORM_SCALING[0] = BBOX_SCALING[0] * PROJECTION_SCALING[0] * (BBOX_EXTENT[1] - BBOX_EXTENT[0]) / (DATA_EXTENT[1] - DATA_EXTENT[0]);
    NORM_SCALING[5] = BBOX_SCALING[5] * PROJECTION_SCALING[5] * (BBOX_EXTENT[3] - BBOX_EXTENT[2]) / (DATA_EXTENT[3] - DATA_EXTENT[2]);
    NORM_SCALING[10] = BBOX_SCALING[10] * PROJECTION_SCALING[10] * (BBOX_EXTENT[5] - BBOX_EXTENT[4]) / (DATA_EXTENT[5] - DATA_EXTENT[4]);

    DATA_VIEW_MATRIX = CTC_MATRIX * BBOX_TRANSLATION * NORM_SCALING * DATA_SCALING * AUTO_ROTATION * ROTATION * DATA_TRANSLATION;

    CELL_VIEW_MATRIX = DATA_VIEW_MATRIX*I;


    CameraToClipMatrix<float> SMALL_CTC_MATRIX;
    SMALL_CTC_MATRIX.setDeep(4,4,CTC_MATRIX.data());
    SMALL_CTC_MATRIX.setWindow(SMALL_WIDTH, SMALL_HEIGHT);
    MINI_CELL_VIEW_MATRIX = SMALL_CTC_MATRIX * BBOX_TRANSLATION * AUTO_ROTATION * ROTATION * I;

    err = clEnqueueWriteBuffer ( (*queue),
        view_matrix_inv_cl,
        CL_TRUE,
        0,
        DATA_VIEW_MATRIX.size()*sizeof(cl_float),
        DATA_VIEW_MATRIX.getInverse().data(),
        0,0,0);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error writing to CL buffer 'view_matrix_inv_cl': " << cl_error_cstring(err) << std::endl;
    }

    err = clEnqueueWriteBuffer ( (*queue),
        function_view_matrix_inv_cl,
        CL_TRUE,
        0,
        DATA_VIEW_MATRIX.size()*sizeof(cl_float),
        (DATA_VIEW_MATRIX).getInverse().data(),
        0,0,0);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error writing to CL buffer 'view_matrix_inv_cl': " << cl_error_cstring(err) << std::endl;
    }

    err = clSetKernelArg(K_FUNCTION_RAYTRACE, 3, sizeof(cl_mem), (void *) &function_view_matrix_inv_cl);
    err |= clSetKernelArg(K_SVO_RAYTRACE, 7, sizeof(cl_mem), (void *) &view_matrix_inv_cl);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error setting kernel argument: " << cl_error_cstring(err) << std::endl;
    }

    this->data_extent_refresh();
}


void VolumeRenderGLWidget::setMessageString(QString str)
{
        emit changedMessageString(str);
}

void VolumeRenderGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    rotationTimer->restart();
    float move_scaling = 1.0;
    if(event->modifiers() & Qt::ShiftModifier) move_scaling = 5.0;
    else if(event->modifiers() & Qt::ControlModifier) move_scaling = 0.2;


    if ((event->buttons() & Qt::LeftButton) && !(event->buttons() & Qt::RightButton))
    {
        /* Rotation happens multiplicatively around a rolling axis given
         * by the mouse move direction and magnitude.
         * Moving the mouse alters rotation.
         * */

        double eta = std::atan2(event->x() - lastPos_x, event->y() - lastPos_y) - pi*1.0;
        double roll = move_scaling * pi/((float) HEIGHT) * std::sqrt((event->x() - lastPos_x)*(event->x() - lastPos_x) + (event->y() - lastPos_y)*(event->y() - lastPos_y));

        ROLL_ROTATION.setArbRotation(-0.5*pi, eta, roll);

        ROTATION = ROLL_ROTATION * ROTATION;
        this->timerLastAction->start();
        this->isRefreshRequired = true;
    }
    if ((event->buttons() & Qt::LeftButton) && (event->buttons() & Qt::RightButton))
    {
        /* Rotation happens multiplicatively around a rolling axis given
         * by the mouse move direction and magnitude.
         * Moving the mouse alters rotation.
         * */

        //~ double eta = std::atan2(event->x() - lastPos_x, event->y() - lastPos_y) - pi*1.0;
        double roll = move_scaling * pi/((float) HEIGHT) * (event->y() - lastPos_y);

        ROLL_ROTATION.setArbRotation(0, 0, roll);

        ROTATION = ROLL_ROTATION * ROTATION;
        this->timerLastAction->start();
        this->isRefreshRequired = true;
    }
    else if (event->buttons() & Qt::MiddleButton)
    {
        /* X/Y translation happens multiplicatively. Here it is
         * important to retain the bounding box accordingly  */
        float dx = move_scaling * 2.0*(DATA_VIEW_EXTENT[1]-DATA_VIEW_EXTENT[0])/((float) HEIGHT) * (event->x() - lastPos_x);
        float dy = move_scaling * -2.0*(DATA_VIEW_EXTENT[3]-DATA_VIEW_EXTENT[2])/((float) HEIGHT) * (event->y() - lastPos_y);

        Matrix<float> DATA_TRANSLATION_PREV;
        DATA_TRANSLATION_PREV.setIdentity(4);
        DATA_TRANSLATION_PREV = DATA_TRANSLATION;

        DATA_TRANSLATION.setIdentity(4);
        DATA_TRANSLATION[3] = dx;
        DATA_TRANSLATION[7] = dy;

        DATA_TRANSLATION = ( ROTATION.getInverse() * DATA_TRANSLATION * ROTATION) * DATA_TRANSLATION_PREV;
        this->timerLastAction->start();
        this->isRefreshRequired = true;
    }
    else if (!(event->buttons() & Qt::LeftButton) && (event->buttons() & Qt::RightButton))
    {
        /* Z translation happens multiplicatively */
        float dz = move_scaling * 2.0*(DATA_VIEW_EXTENT[5]-DATA_VIEW_EXTENT[4])/((float) HEIGHT) * (event->y() - lastPos_y);

        Matrix<float> DATA_TRANSLATION_PREV;
        DATA_TRANSLATION_PREV.setIdentity(4);
        DATA_TRANSLATION_PREV = DATA_TRANSLATION;

        DATA_TRANSLATION.setIdentity(4);
        DATA_TRANSLATION[11] = dz;

        DATA_TRANSLATION = ( ROTATION.getInverse() * DATA_TRANSLATION * ROTATION) * DATA_TRANSLATION_PREV;
        this->timerLastAction->start();
        this->isRefreshRequired = true;
    }



    DATA_VIEW_EXTENT =  (DATA_SCALING * DATA_TRANSLATION).getInverse() * DATA_EXTENT;
    lastPos_x = event->x();
    lastPos_y = event->y();
}


void VolumeRenderGLWidget::keyPressEvent(QKeyEvent *event)
{
    //~ std::cout << "Key event!" << std::endl;
    float sign_modifier = 1.0;
    this->timerLastAction->start();
    this->isRefreshRequired = true;

    if (event->modifiers() & Qt::ShiftModifier) sign_modifier = -1.0;


    switch(event->key())
    {
        case (Qt::Key_F1):
            MSAA_METHOD = !MSAA_METHOD;
            std::cout << "MSAA_METHOD " << MSAA_METHOD << std::endl;
            break;
        case (Qt::Key_F2):
            if (sign_modifier+1) MSAA_SAMPLES = MSAA_SAMPLES/2;
            else MSAA_SAMPLES = MSAA_SAMPLES*2;
            if (MSAA_SAMPLES < 1) MSAA_SAMPLES = 1;
            if (MSAA_SAMPLES > 32) MSAA_SAMPLES = 32;
            std::cout << "MSAA_SAMPLES " << MSAA_SAMPLES << std::endl;
            screen_buffer_refresh();
            break;
        case (Qt::Key_F3):
            MSAA_EXPOSURE += sign_modifier*0.1;
            std::cout << "MSAA_EXPOSURE " << MSAA_EXPOSURE << std::endl;
            break;
        case (Qt::Key_F4):
            if (isLog)
            {
                if (TSF_PARAMETERS[2] <= 0.0) TSF_PARAMETERS[2] = 0.001;
                if (TSF_PARAMETERS[3] <= 0.0) TSF_PARAMETERS[3] = 0.001;

                float cur_exp_low = std::log10(TSF_PARAMETERS[2]);
                float cur_exp_high = std::log10(TSF_PARAMETERS[3]);

                cur_exp_high += (cur_exp_high - cur_exp_low) * 0.01 * sign_modifier;

                TSF_PARAMETERS[3] = std::pow(10.0, cur_exp_high);
            }
            else TSF_PARAMETERS[3] += (TSF_PARAMETERS[3] - TSF_PARAMETERS[2]) * 0.01 * sign_modifier;
            break;
        case (Qt::Key_F5):
            TSF_PARAMETERS[4] += 0.01 * sign_modifier;
            break;
        case (Qt::Key_F6):
            TSF_PARAMETERS[5] += 0.01 * sign_modifier;
            break;
        case (Qt::Key_F7):
            MISC_FLOAT_K_FUNCTION[0] += 0.01 * std::abs(MISC_FLOAT_K_FUNCTION[0]) * sign_modifier;
            break;
        case (Qt::Key_F8):
            MISC_FLOAT_K_FUNCTION[1] += 0.01 * std::abs(MISC_FLOAT_K_FUNCTION[0]) * sign_modifier;
            break;
        case (Qt::Key_F9):
            MISC_FLOAT_K_FUNCTION[2] += 0.01 * std::abs(MISC_FLOAT_K_FUNCTION[0]) * sign_modifier;
            break;
        case (Qt::Key_F10):
            MISC_FLOAT_K_FUNCTION[3] += 0.001 * std::abs(MISC_FLOAT_K_FUNCTION[0]) * sign_modifier;
            break;
        case (Qt::Key_F11):
            LINEWIDTH += sign_modifier*0.1;
            std::cout << "LINEWIDTH " << LINEWIDTH << std::endl;
            break;
        case (Qt::Key_F12):
            MISC_FLOAT_K_RAYTRACE[5] += sign_modifier*0.1;
            this->setMinLevel(MISC_FLOAT_K_RAYTRACE[5]);
            std::cout << "MISC_FLOAT_K_RAYTRACE[5] " << MISC_FLOAT_K_RAYTRACE[5] << std::endl;
            break;


        case (Qt::Key_Escape):
            break;
    }

    //~ std::cout << std::setprecision(3) << std::fixed << "Tsf = " << TSF_PARAMETERS[0] << ", " << TSF_PARAMETERS[1] << "] Data = [" << TSF_PARAMETERS[2]<< ", " << TSF_PARAMETERS[3] << "] Alpha factor = " << TSF_PARAMETERS[4] << " Intensity = " << TSF_PARAMETERS[5] <<std::endl;
    //~ std::cout << std::setprecision(3) << std::fixed << "Misc float = [" << MISC_FLOAT_K_FUNCTION[0] << ", " << MISC_FLOAT_K_FUNCTION[1] << ", " << MISC_FLOAT_K_FUNCTION[2]<< ", " << MISC_FLOAT_K_FUNCTION[3] << "]" << std::endl;
    //~ std::cout << std::setprecision(3) << std::fixed << "Misc float k_raytrace = [" << MISC_FLOAT_K_RAYTRACE[0] << ", " << MISC_FLOAT_K_RAYTRACE[1] << ", " << MISC_FLOAT_K_RAYTRACE[2]<< ", " << MISC_FLOAT_K_RAYTRACE[3] << ", " << MISC_FLOAT_K_RAYTRACE[4] << "]" << std::endl;
    this->setTsfParameters();
    this->setMiscArrays();
    this->gen_ray_tex();
    this->screen_buffer_refresh();
}

void VolumeRenderGLWidget::wheelEvent(QWheelEvent *event)
{
    float move_scaling = 1.0;
    if(event->modifiers() & Qt::ShiftModifier) move_scaling = 5.0;
    else if(event->modifiers() & Qt::ControlModifier) move_scaling = 0.2;


    double delta = move_scaling*((double)event->delta())*0.0008;

    if (!(event->buttons() & Qt::LeftButton) && !(event->buttons() & Qt::RightButton))
    {
        if ((DATA_SCALING[0] > 0.0001) || (delta > 0))
        {
            DATA_SCALING[0] += DATA_SCALING[0]*delta;
            DATA_SCALING[5] += DATA_SCALING[5]*delta;
            DATA_SCALING[10] += DATA_SCALING[10]*delta;

            DATA_VIEW_EXTENT =  (DATA_SCALING * DATA_TRANSLATION).getInverse() * DATA_EXTENT;
            this->timerLastAction->start();
            this->isRefreshRequired = true;
        }
    }
    else
    {
        if ((BBOX_SCALING[0] > 0.0001) || (delta > 0))
        {
            BBOX_SCALING[0] += BBOX_SCALING[0]*delta;
            BBOX_SCALING[5] += BBOX_SCALING[5]*delta;
            BBOX_SCALING[10] += BBOX_SCALING[10]*delta;
            this->timerLastAction->start();
            this->isRefreshRequired = true;
        }
    }
    rotationTimer->restart();
}


int VolumeRenderGLWidget::init_gl()
{

    // BLEND
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    // AA
    //~ glEnable(GL_LINE_SMOOTH);
    //~ glEnable(GL_POINT_SMOOTH);
    //~ glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    //~ glDisable(GL_DEPTH_TEST);
    //~ glDisable(GL_MULTISAMPLE);

    // Textures
    glGenTextures(1, &ray_tex);
    glGenTextures(1, &tsf_tex);
    glGenTextures(1, &hist_tex);
    glGenTextures(1, &std_3d_tex);
    glGenTextures(1, &std_2d_tex);
    glGenTextures(1, &glow_tex);
    glGenTextures(1, &blend_tex);
    glGenTextures(1, &mini_uc_tex);
    glGenTextures(1, &small_storage_tex);
    glGenTextures(1, &storage_tex);
    glGenTextures(1, &msaa_tex);
    glGenTextures(1, &msaa_depth_tex);
    glGenTextures(1, &msaa_intermediate_storage_tex);
    this->screen_buffer_refresh();

    // Shader programs
    init_gl_programs();

    // Vertice buffer objects
    glGenBuffers(1, &text_texpos_vbo);
    glGenBuffers(1, &text_position_vbo);
    glGenBuffers(10, tex_coord_vbo);
    glGenBuffers(20, position_vbo);
    glGenBuffers(5, lab_reference_vbo);
    glGenBuffers(5, data_extent_vbo);
    glGenBuffers(5, data_view_extent_vbo);
    glGenBuffers(5, unitcell_vbo);
    glGenBuffers(5, screen_vbo);
    glGenBuffers(1, &sampleWeightBuf);

    this->vbo_buffers_refresh();

    {
        Matrix<float> tex_position;
        float buf[] = {
            -1.0, 0.0, 0.0,-1.0,
            -1.0,-1.0, 0.0, 0.0};

        tex_position.setDeep(2, 4, buf);
        setVbo(&screen_vbo[0], tex_position.getColMajor().data(), tex_position.size());
    }
    {
        Matrix<float> tex_position;
        float buf[] = {
            0.0, 1.0, 1.0, 0.0,
            -1.0,-1.0, 0.0, 0.0};

        tex_position.setDeep(2, 4, buf);
        setVbo(&screen_vbo[1], tex_position.getColMajor().data(), tex_position.size());
    }
    {
        Matrix<float> tex_position;
        float buf[] = {
            0.0, 1.0, 1.0, 0.0,
            0.0, 0.0, 1.0, 1.0};

        tex_position.setDeep(2, 4, buf);
        setVbo(&screen_vbo[2], tex_position.getColMajor().data(), tex_position.size());
    }
    {
        Matrix<float> tex_position;
        float buf[] = {
            -1.0, -1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 1.0};

        tex_position.setDeep(2, 4, buf);
        setVbo(&screen_vbo[3], tex_position.getColMajor().data(), tex_position.size());
    }
    {
        Matrix<float> tex_position;
        float buf[] = {
            -1.0, 1.0, 1.0,-1.0,
            -1.0,-1.0, 1.0, 1.0};

        tex_position.setDeep(2, 4, buf);
        setVbo(&screen_vbo[4], tex_position.getColMajor().data(), tex_position.size());
    }
    {
        Matrix<float> tex_position;
        float buf[] = {
            -1.0, -0.5, -0.5,-1.0,
            -1.0,-1.0, -0.5, -0.5};

        tex_position.setDeep(2, 4, buf);
        setVbo(&screen_vbo[0], tex_position.getColMajor().data(), tex_position.size());
    }


    {
        Matrix<float> tex_position;
        float buf[] = {
            -1.0, 1.0, 1.0,-1.0,
            1.0,1.0, 0.7, 0.7};

        tex_position.setDeep(2, 4, buf);
        setVbo(&position_vbo[0], tex_position.getColMajor().data(), tex_position.size());
    }
    {
        Matrix<float> tex_position;
        float buf[] = {
            -1.0, 1.0, 1.0,-1.0,
            -1.0,-1.0, 1.0, 1.0};

        tex_position.setDeep(2, 4, buf);
        setVbo(&position_vbo[1], tex_position.getColMajor().data(), tex_position.size());
    }
    {
        Matrix<float> tex_position;
        float buf[] = {
            0.94, 0.94, 0.99, 0.99,
            -0.9, 0.9, 0.9, -0.9};
        tex_position.setDeep(2, 4, buf);
        setVbo(&position_vbo[2], tex_position.getColMajor().data(), tex_position.size());
    }
    {
        Matrix<float> tex_position;
        float buf[] = {
            0.935, 0.935, 0.84, 0.84,
            -0.9, 0.9, 0.9, -0.9};
        tex_position.setDeep(2, 4, buf);
        setVbo(&position_vbo[3], tex_position.getColMajor().data(), tex_position.size());
    }
    {
        Matrix<float> tex_position;
        float buf[] = {
            0.9, 0.9, 0.8, 0.8,
            -0.9, 0.9, 0.9, -0.9};
        tex_position.setDeep(2, 4, buf);
        setVbo(&position_vbo[4], tex_position.getColMajor().data(), tex_position.size());
    }
    {
        Matrix<float> tex_position;
        float buf[] = {
            -1.0, 0.0, 0.0, -1.0,
            -1.0, -1.0, 0.0, 0.0};

        tex_position.setDeep(2, 4, buf);
        setVbo(&position_vbo[5], tex_position.getColMajor().data(), tex_position.size());
    }
    {
        Matrix<float> tex_position;
        float buf[] = {
            0.0, 1.0, 1.0, 0.0,
            -1.0, -1.0, 0.0, 0.0};

        tex_position.setDeep(2, 4, buf);
        setVbo(&position_vbo[6], tex_position.getColMajor().data(), tex_position.size());
    }

    {
        Matrix<float> tex_position;
        float buf[] = {
            0.935, 0.935, 0.995, 0.995,
            -0.9, 0.9, 0.9, -0.9};
        tex_position.setDeep(2, 4, buf);
        setVbo(&position_vbo[10], tex_position.getColMajor().data(), tex_position.size());
    }

    {
        Matrix<float> mat;
        float buf[8] = {
            0.0,1.0,1.0,0.0,
            0.0,0.0,1.0,1.0};

        mat.setDeep(2, 4, buf);
        setVbo(&tex_coord_vbo[0], mat.getColMajor().data(), mat.size());
        setVbo(&tex_coord_vbo[1], mat.getColMajor().data(), mat.size());
        setVbo(&tex_coord_vbo[2], mat.getColMajor().data(), mat.size());
        setVbo(&tex_coord_vbo[3], mat.getColMajor().data(), mat.size());
    }

    /* Framebuffer objects */
    GLenum status;
    glGenFramebuffers(1, &STD_FBO);
    glGenFramebuffers(1, &MSAA_FBO);
    glGenFramebuffers(1, &SMALL_FBO);

    glBindFramebuffer(GL_FRAMEBUFFER, STD_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, std_3d_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, std_2d_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, blend_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, storage_tex, 0);
    //~ glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT0, GL_TEXTURE_2D, small_depth_tex, 0);
    if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "STD_FBO: Error in framebuffer: " << gl_framebuffer_error_cstring(status) << std::endl;
        return 0;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, SMALL_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mini_uc_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, glow_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, small_storage_tex, 0);
    if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "SMALL_FBO: Error in framebuffer: " << gl_framebuffer_error_cstring(status) << std::endl;
        return 0;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, MSAA_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msaa_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, msaa_depth_tex, 0);
    if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "MSAA_FBO: Error in framebuffer: " << gl_framebuffer_error_cstring(status) << std::endl;
        return 0;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return 1;
}

void VolumeRenderGLWidget::vbo_buffers_refresh()
{
    {
        Matrix<float> data_extent_position;
        float buf[] = {
            DATA_EXTENT[0], DATA_EXTENT[1], DATA_EXTENT[1], DATA_EXTENT[0], DATA_EXTENT[0], DATA_EXTENT[1], DATA_EXTENT[1], DATA_EXTENT[0],
            DATA_EXTENT[2], DATA_EXTENT[2], DATA_EXTENT[3], DATA_EXTENT[3], DATA_EXTENT[2], DATA_EXTENT[2], DATA_EXTENT[3], DATA_EXTENT[3],
            DATA_EXTENT[5], DATA_EXTENT[5], DATA_EXTENT[5], DATA_EXTENT[5], DATA_EXTENT[4], DATA_EXTENT[4], DATA_EXTENT[4], DATA_EXTENT[4]};

        data_extent_position.setDeep(3, 8, buf);
        glBindBuffer(GL_ARRAY_BUFFER, data_extent_vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT)*data_extent_position.size(), data_extent_position.getColMajor().data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    {
        Matrix<float> data_view_extent_position;
        float buf[] = {
            DATA_VIEW_EXTENT[0], DATA_VIEW_EXTENT[1], DATA_VIEW_EXTENT[1], DATA_VIEW_EXTENT[0], DATA_VIEW_EXTENT[0], DATA_VIEW_EXTENT[1], DATA_VIEW_EXTENT[1], DATA_VIEW_EXTENT[0],
            DATA_VIEW_EXTENT[2], DATA_VIEW_EXTENT[2], DATA_VIEW_EXTENT[3], DATA_VIEW_EXTENT[3], DATA_VIEW_EXTENT[2], DATA_VIEW_EXTENT[2], DATA_VIEW_EXTENT[3], DATA_VIEW_EXTENT[3],
            DATA_VIEW_EXTENT[5], DATA_VIEW_EXTENT[5], DATA_VIEW_EXTENT[5], DATA_VIEW_EXTENT[5], DATA_VIEW_EXTENT[4], DATA_VIEW_EXTENT[4], DATA_VIEW_EXTENT[4], DATA_VIEW_EXTENT[4]};

        data_view_extent_position.setDeep(3, 8, buf);
        glBindBuffer(GL_ARRAY_BUFFER, data_view_extent_vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT)*data_view_extent_position.size(), data_view_extent_position.getColMajor().data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void VolumeRenderGLWidget::screen_buffer_refresh()
{
    /* std_3d_tex */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, std_3d_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (this->WIDTH), (this->HEIGHT), 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* std_2d_tex */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, std_2d_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->WIDTH, this->HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* blend_tex */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blend_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->WIDTH, this->HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* small_storage_tex */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, small_storage_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->SMALL_WIDTH, this->SMALL_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* storage_tex */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, storage_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->WIDTH, this->HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* mini_uc_tex */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mini_uc_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->SMALL_WIDTH, this->SMALL_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* glow_tex */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glow_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->SMALL_WIDTH, this->SMALL_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* msaa_tex */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaa_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, MSAA_SAMPLES, GL_RGBA32F, this->WIDTH, this->HEIGHT, false );
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

    /* msaa_depth_tex */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaa_depth_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, MSAA_SAMPLES, GL_DEPTH_COMPONENT32, this->WIDTH, this->HEIGHT, 0);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);


    //~ /* msaa_intermediate_storage_tex */
    //~ glGenTextures(1, &msaa_intermediate_storage_tex);
    //~ glActiveTexture(GL_TEXTURE0);
    //~ glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaa_intermediate_storage_tex);
    //~ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //~ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //~ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //~ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //~ glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, (GLsizei) MSAA_SAMPLES, GL_RGBA, this->WIDTH, this->HEIGHT, false );
    //~ glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

    //~ std::cout << "Screen buffers set to " << this->WIDTH << " x "<< this->HEIGHT << std::endl;
}


void VolumeRenderGLWidget::setTsfMin(double value)
{
    TSF_PARAMETERS[2] = value;
    this->setTsfParameters();

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}
void VolumeRenderGLWidget::setTsfMax(double value)
{
    TSF_PARAMETERS[3] = value;
    this->setTsfParameters();

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}
void VolumeRenderGLWidget::setTsfAlpha(double value)
{
    TSF_PARAMETERS[4] = value;
    this->setTsfParameters();

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}
void VolumeRenderGLWidget::setTsfBrightness(double value)
{
    TSF_PARAMETERS[5] = value;
    this->setTsfParameters();

    this->timerLastAction->start();
    this->isRefreshRequired = true;
}

void VolumeRenderGLWidget::ray_tex_refresh(cl_kernel kernel)
{
    if (ray_tex_cl && tsf_tex_cl && isRefreshRequired)
    {
        /* Aquire shared CL/GL objects */
        callTimer->start();
        glFinish();
        this->isRefreshRequired = false;

        err = clEnqueueAcquireGLObjects((*queue), 1, &ray_tex_cl, 0, 0, 0);
        if (err != CL_SUCCESS)
        {
            std::cout << "Error aquiring shared CL/GL objects: " << cl_error_cstring(err) << std::endl;
        }

        /* Launch rendering kernel */
        size_t area_per_call[2] = {64, 64};
        size_t call_offset[2] = {0,0};
        callTimeMax = 1000/30;
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

                err = clEnqueueNDRangeKernel((*queue), kernel, 2, call_offset, area_per_call, ray_loc_ws, 0, NULL, NULL);
                if (err != CL_SUCCESS)
                {
                    std::cout << "Error launching kernel: " << cl_error_cstring(err) << std::endl;
                }
                clFinish((*queue));
            }
        }

        /* Release shared CL/GL objects */
        err = clEnqueueReleaseGLObjects((*queue), 1, &ray_tex_cl, 0, 0, 0);
        clFinish((*queue));
        if (err != CL_SUCCESS)
        {
            std::cout << "Error releasing shared CL/GL objects: " << cl_error_cstring(err) << std::endl;
        }


        //~ std::cout << "Call: " << callTimer->elapsed() << std::endl;
    }
}

void VolumeRenderGLWidget::getScreenPosition(float * screen_pos, float * space_pos, float * transform)
{
    Matrix<float> TRANSFORM;
    TRANSFORM.setDeep(4, 4, transform);
    Matrix<float> SPACE;
    SPACE.setDeep(4, 1, space_pos);
    SPACE[3] = 1.0;
    Matrix<float> SCREEN(4, 1);

    SCREEN = TRANSFORM * SPACE;

    screen_pos[0] = SCREEN[0]/SCREEN[3];
    screen_pos[1] = SCREEN[1]/SCREEN[3];
}

void VolumeRenderGLWidget::std_text_draw(const char *text, Atlas *a, float * color, float * xy, float scale, int w, int h)
{
	const uint8_t *p;

	MiniArray<float> position(2 * 4 * strlen(text)); // 2 triangles and 4 verts per character
    MiniArray<float> texpos(2 * 4 * strlen(text)); // 2 triangles and 4 verts per character
    MiniArray<unsigned int> indices(6 * strlen(text)); // 6 indices per character

	int c = 0;
    float sx = scale*2.0/w;
    float sy = scale*2.0/h;
    float x = xy[0];
    float y = xy[1];

    x -= std::fmod(x,sx);
    y -= std::fmod(y,sy);

	/* Loop through all characters */
	for(p = (const uint8_t *)text; *p; p++)
    {
		/* Calculate the vertex and texture coordinates */
		float x2 = x + a->c[*p].bl * sx;
		float y2 = y + a->c[*p].bt * sy;
		float foo_w = a->c[*p].bw * sx;
		float foo_h = a->c[*p].bh * sy;

		/* Advance the cursor to the start of the next character */
		x += a->c[*p].ax * sx;
		y += a->c[*p].ay * sy;

		/* Skip glyphs that have no pixels */
		if(!foo_w || !foo_h)
			continue;

        position[c*8+0] = x2;
        position[c*8+1] = y2;
        position[c*8+2] = x2 + foo_w;
        position[c*8+3] = y2;
        position[c*8+4] = x2;
        position[c*8+5] = y2 - foo_h;
        position[c*8+6] = x2 + foo_w;
        position[c*8+7] = y2 - foo_h;

        texpos[c*8+0] = a->c[*p].tx;
        texpos[c*8+1] = a->c[*p].ty;
        texpos[c*8+2] = a->c[*p].tx + a->c[*p].bw / a->tex_w;
        texpos[c*8+3] = a->c[*p].ty;
        texpos[c*8+4] = a->c[*p].tx;
        texpos[c*8+5] = a->c[*p].ty + a->c[*p].bh / a->tex_h;
        texpos[c*8+6] = a->c[*p].tx + a->c[*p].bw / a->tex_w;
        texpos[c*8+7] = a->c[*p].ty + a->c[*p].bh / a->tex_h;

        indices[c*6+0] = c*4 + 0;
        indices[c*6+1] = c*4 + 2;
        indices[c*6+2] = c*4 + 3;
        indices[c*6+3] = c*4 + 0;
        indices[c*6+4] = c*4 + 1;
        indices[c*6+5] = c*4 + 3;

        c++;
    }

    glUseProgram(std_text_program);

	// Set std_text_uniform_texture
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, a->tex);
	glUniform1i(std_text_uniform_tex, 0);

    // Set std_text_uniform_color
	 glUniform4fv(std_text_uniform_color, 1, color);

    // Set std_2d_tex_attribute_position
    glEnableVertexAttribArray(std_text_attribute_position);
    glBindBuffer(GL_ARRAY_BUFFER, text_position_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * position.size(), position.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(std_text_attribute_position, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Set std_2d_tex_attribute_texpos
    glEnableVertexAttribArray(std_text_attribute_texpos);
    glBindBuffer(GL_ARRAY_BUFFER, text_texpos_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * texpos.size(), texpos.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(std_text_attribute_texpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

	/* Draw all the character on the screen in one go */
	glDrawElements(GL_TRIANGLES,  c*6,  GL_UNSIGNED_INT,  indices.data());

    glDisableVertexAttribArray(std_text_attribute_position);
	glDisableVertexAttribArray(std_text_attribute_texpos);
    glUseProgram(0);
}

void VolumeRenderGLWidget::init_gl_programs()
{

    GLint link_ok = GL_FALSE;
    GLuint vertice_shader, fragment_shader; // Delete these after use?

    // Standard 3D Program
    {
        vertice_shader = create_shader(":/src/shaders/std_3d_tex.v.glsl", GL_VERTEX_SHADER);
        fragment_shader = create_shader(":/src/shaders/std_3d_tex.f.glsl", GL_FRAGMENT_SHADER);

        std_3d_program = glCreateProgram();
        glAttachShader(std_3d_program, vertice_shader);
        glAttachShader(std_3d_program, fragment_shader);
        glLinkProgram(std_3d_program);

        glGetProgramiv(std_3d_program, GL_LINK_STATUS, &link_ok);
        if (!link_ok) {
            std::cout << "glLinkProgram: ";
            GLint log_length = 0;

            if (glIsProgram(std_3d_program))
            {
                glGetProgramiv(std_3d_program, GL_INFO_LOG_LENGTH, &log_length);
                char* log = new char[log_length];

                glGetProgramInfoLog(std_3d_program, log_length, NULL, log);
                std::cout << log << std::endl;

                delete[] log;
                glDeleteProgram(std_3d_program);
            }
            else
            {
                std::cout << "std_3d: Could not create GL program: Supplied argument is not a program!" << std::endl;
            }
        }

        std_3d_attribute_position = glGetAttribLocation(std_3d_program, "position");
        if (std_3d_attribute_position == -1) {
            std::cout << "std_3d: Could not bind attribute: position" << std::endl;
        }

        std_3d_attribute_texpos = glGetAttribLocation(std_3d_program, "texpos");
        if (std_3d_attribute_texpos == -1) {
            std::cout << "std_3d: Could not bind attribute: texpos" << std::endl;
        }

        std_3d_uniform_transform = glGetUniformLocation(std_3d_program, "transform");
        if (std_3d_uniform_transform == -1) {
            std::cout << "std_3d: Could not bind uniform: transform" << std::endl;
        }

        std_3d_uniform_u = glGetUniformLocation(std_3d_program, "U");
        if (std_3d_uniform_u == -1) {
            std::cout << "std_3d: Could not bind uniform: U" << std::endl;
        }

        std_3d_uniform_color = glGetUniformLocation(std_3d_program, "color");
        if (std_3d_uniform_color == -1) {
            std::cout << "std_3d: Could not bind uniform: color" << std::endl;
        }

        std_3d_uniform_bbox_max = glGetUniformLocation(std_3d_program, "bbox_max");
        if (std_3d_uniform_bbox_max == -1) {
            std::cout << "std_3d: Could not bind uniform: bbox_max" << std::endl;
        }

        std_3d_uniform_bbox_min = glGetUniformLocation(std_3d_program, "bbox_min");
        if (std_3d_uniform_bbox_min == -1) {
            std::cout << "std_3d: Could not bind uniform: bbox_min" << std::endl;
        }

        std_3d_uniform_texture = glGetUniformLocation(std_3d_program, "texy");
        if (std_3d_uniform_texture == -1) {
            std::cout << "std_3d: Could not bind uniform: texy" << std::endl;
        }

        std_3d_uniform_time = glGetUniformLocation(std_3d_program, "time");
        if (std_3d_uniform_time == -1) {
            std::cout << "std_3d: Could not bind uniform: time" << std::endl;
        }
    }

    // Program for standard font texture rendering
    {
        vertice_shader = create_shader(":/src/shaders/text.v.glsl", GL_VERTEX_SHADER);
        fragment_shader = create_shader(":/src/shaders/text.f.glsl", GL_FRAGMENT_SHADER);

        std_text_program = glCreateProgram();
        glAttachShader(std_text_program, vertice_shader);
        glAttachShader(std_text_program, fragment_shader);
        glLinkProgram(std_text_program);

        glGetProgramiv(std_text_program, GL_LINK_STATUS, &link_ok);
        if (!link_ok) {
            std::cout << "glLinkProgram: ";
            GLint log_length = 0;

            if (glIsProgram(std_text_program))
            {
                glGetProgramiv(std_text_program, GL_INFO_LOG_LENGTH, &log_length);
                char* log = new char[log_length];

                glGetProgramInfoLog(std_text_program, log_length, NULL, log);
                std::cout << log << std::endl;

                delete[] log;
                glDeleteProgram(std_text_program);
            }
            else
            {
                std::cout << "Could not create GL program: Supplied argument is not a program!" << std::endl;
            }
        }

        std_text_attribute_position = glGetAttribLocation(std_text_program, "position");
        if (std_text_attribute_position == -1) {
            std::cout << "std_text: Could not bind attribute: position" << std::endl;
        }

        std_text_attribute_texpos = glGetAttribLocation(std_text_program, "texpos");
        if (std_text_attribute_texpos == -1) {
            std::cout << "std_text: Could not bind attribute: texpos" << std::endl;
        }

        std_text_uniform_tex = glGetUniformLocation(std_text_program, "tex");
        if (std_text_uniform_tex == -1) {
            std::cout << "std_text: Could not bind attribute: tex" << std::endl;
        }

        std_text_uniform_color = glGetUniformLocation(std_text_program, "color");
        if (std_text_uniform_color == -1) {
            std::cout << "std_text: Could not bind uniform: color" << std::endl;
        }
    }


    // Program for standard 2D texture rendering
    {
        vertice_shader = create_shader(":/src/shaders/std_2d_tex.v.glsl", GL_VERTEX_SHADER);
        fragment_shader = create_shader(":/src/shaders/std_2d_tex.f.glsl", GL_FRAGMENT_SHADER);

        std_2d_tex_program = glCreateProgram();
        glAttachShader(std_2d_tex_program, vertice_shader);
        glAttachShader(std_2d_tex_program, fragment_shader);
        glLinkProgram(std_2d_tex_program);

        glGetProgramiv(std_2d_tex_program, GL_LINK_STATUS, &link_ok);
        if (!link_ok) {
            std::cout << "glLinkProgram: ";
            GLint log_length = 0;

            if (glIsProgram(std_2d_tex_program))
            {
                glGetProgramiv(std_2d_tex_program, GL_INFO_LOG_LENGTH, &log_length);
                char* log = new char[log_length];

                glGetProgramInfoLog(std_2d_tex_program, log_length, NULL, log);
                std::cout << log << std::endl;

                delete[] log;
                glDeleteProgram(std_2d_tex_program);
            }
            else
            {
                std::cout << "Could not create GL program: Supplied argument is not a program!" << std::endl;
            }
        }

        std_2d_tex_attribute_position = glGetAttribLocation(std_2d_tex_program, "position");
        if (std_2d_tex_attribute_position == -1) {
            std::cout << "std_2d_tex: Could not bind attribute: position" << std::endl;
        }

        std_2d_tex_attribute_texpos = glGetAttribLocation(std_2d_tex_program, "texpos");
        if (std_2d_tex_attribute_texpos == -1) {
            std::cout << "std_2d_tex: Could not bind attribute: texpos" << std::endl;
        }

        std_2d_tex_uniform_texture = glGetUniformLocation(std_2d_tex_program, "texy");
        if (std_2d_tex_uniform_texture == -1) {
            std::cout << "std_2d_tex: Could not bind uniform: texy" << std::endl;
        }
    }

    // Program for standard 2D colored vertice rendering
    {
        vertice_shader = create_shader(":/src/shaders/std_2d_color.v.glsl", GL_VERTEX_SHADER);
        fragment_shader = create_shader(":/src/shaders/std_2d_color.f.glsl", GL_FRAGMENT_SHADER);

        std_2d_color_program = glCreateProgram();
        glAttachShader(std_2d_color_program, vertice_shader);
        glAttachShader(std_2d_color_program, fragment_shader);
        glLinkProgram(std_2d_color_program);

        glGetProgramiv(std_2d_color_program, GL_LINK_STATUS, &link_ok);
        if (!link_ok) {
            std::cout << "glLinkProgram: ";
            GLint log_length = 0;

            if (glIsProgram(std_2d_color_program))
            {
                glGetProgramiv(std_2d_color_program, GL_INFO_LOG_LENGTH, &log_length);
                char* log = new char[log_length];

                glGetProgramInfoLog(std_2d_color_program, log_length, NULL, log);
                std::cout << log << std::endl;

                delete[] log;
                glDeleteProgram(std_2d_color_program);
            }
            else
            {
                std::cout << "Could not create GL program: Supplied argument is not a program!" << std::endl;
            }
        }

        std_2d_color_attribute_position = glGetAttribLocation(std_2d_color_program, "position");
        if (std_2d_color_attribute_position == -1) {
            std::cout << "std_2d_color: Could not bind attribute: position" << std::endl;
        }

        std_2d_color_uniform_color = glGetUniformLocation(std_2d_color_program, "color");
        if (std_2d_color_uniform_color == -1) {
            std::cout << "std_2d_color: Could not bind uniform: color" << std::endl;
        }
    }


    // Post processing (pp) program to apply glow
    {
        vertice_shader = create_shader(":/src/shaders/pp_glow.v.glsl", GL_VERTEX_SHADER);
        fragment_shader = create_shader(":/src/shaders/pp_glow.f.glsl", GL_FRAGMENT_SHADER);

        pp_glow_program = glCreateProgram();
        glAttachShader(pp_glow_program, vertice_shader);
        glAttachShader(pp_glow_program, fragment_shader);
        glLinkProgram(pp_glow_program);

        glGetProgramiv(pp_glow_program, GL_LINK_STATUS, &link_ok);
        if (!link_ok) {
            std::cout << "glLinkProgram: ";
            GLint log_length = 0;

            if (glIsProgram(pp_glow_program))
            {
                glGetProgramiv(pp_glow_program, GL_INFO_LOG_LENGTH, &log_length);
                char* log = new char[log_length];

                glGetProgramInfoLog(pp_glow_program, log_length, NULL, log);
                std::cout << log << std::endl;

                delete[] log;
                glDeleteProgram(pp_glow_program);
            }
            else
            {
                std::cout << "Could not create GL program: Supplied argument is not a program!" << std::endl;
            }
        }

        pp_glow_attribute_position = glGetAttribLocation(pp_glow_program, "position");
        if (pp_glow_attribute_position == -1) {
            std::cout << "pp_glow: Could not bind attribute: position" << std::endl;
        }

        pp_glow_attribute_texpos = glGetAttribLocation(pp_glow_program, "texpos");
        if (pp_glow_attribute_texpos == -1) {
            std::cout << "pp_glow: Could not bind attribute: texpos" << std::endl;
        }

        pp_glow_uniform_scale = glGetUniformLocation(pp_glow_program, "scale");
        if (pp_glow_uniform_scale == -1) {
            std::cout << "pp_glow: Could not bind uniform: scale" << std::endl;
        }

        pp_glow_uniform_deviation = glGetUniformLocation(pp_glow_program, "deviation");
        if (pp_glow_uniform_deviation == -1) {
            std::cout << "pp_glow: Could not bind uniform: deviation" << std::endl;
        }

        pp_glow_uniform_samples = glGetUniformLocation(pp_glow_program, "samples");
        if (pp_glow_uniform_samples == -1) {
            std::cout << "pp_glow: Could not bind uniform: samples" << std::endl;
        }

        pp_glow_uniform_color = glGetUniformLocation(pp_glow_program, "color");
        if (pp_glow_uniform_color == -1) {
            std::cout << "pp_glow: Could not bind uniform: color" << std::endl;
        }

        pp_glow_uniform_pixel_size = glGetUniformLocation(pp_glow_program, "pixel_size");
        if (pp_glow_uniform_pixel_size == -1) {
            std::cout << "pp_glow: Could not bind uniform: pixel_size" << std::endl;
        }

        pp_glow_uniform_texture = glGetUniformLocation(pp_glow_program, "texy");
        if (pp_glow_uniform_texture == -1) {
            std::cout << "pp_glow: Could not bind uniform: texy" << std::endl;
        }

        pp_glow_uniform_time = glGetUniformLocation(pp_glow_program, "time");
        if (pp_glow_uniform_texture == -1) {
            std::cout << "pp_glow: Could not bind uniform: time" << std::endl;
        }

        pp_glow_uniform_orientation = glGetUniformLocation(pp_glow_program, "orientation");
        if (pp_glow_uniform_orientation == -1) {
            std::cout << "pp_glow: Could not bind uniform: orientation" << std::endl;
        }
    }
    // Blend program to combine textures
    {
        vertice_shader = create_shader(":/src/shaders/blend.v.glsl", GL_VERTEX_SHADER);
        fragment_shader = create_shader(":/src/shaders/blend.f.glsl", GL_FRAGMENT_SHADER);

        blend_program = glCreateProgram();
        glAttachShader(blend_program, vertice_shader);
        glAttachShader(blend_program, fragment_shader);
        glLinkProgram(blend_program);

        glGetProgramiv(blend_program, GL_LINK_STATUS, &link_ok);
        if (!link_ok) {
            std::cout << "glLinkProgram: ";
            GLint log_length = 0;

            if (glIsProgram(blend_program))
            {
                glGetProgramiv(blend_program, GL_INFO_LOG_LENGTH, &log_length);
                char* log = new char[log_length];

                glGetProgramInfoLog(blend_program, log_length, NULL, log);
                std::cout << log << std::endl;

                delete[] log;
                glDeleteProgram(blend_program);
            }
            else
            {
                std::cout << "Could not create GL program: Supplied argument is not a program!" << std::endl;
            }
        }

        blend_attribute_position = glGetAttribLocation(blend_program, "position");
        if (blend_attribute_position == -1) {
            std::cout << "blend: Could not bind attribute: position" << std::endl;
        }

        blend_attribute_texpos = glGetAttribLocation(blend_program, "texpos");
        if (blend_attribute_texpos == -1) {
            std::cout << "blend: Could not bind attribute: texpos" << std::endl;
        }

        blend_uniform_top_tex = glGetUniformLocation(blend_program, "top_texture");
        if (blend_uniform_top_tex == -1) {
            std::cout << "blend: Could not bind uniform: top_texture" << std::endl;
        }

        blend_uniform_bot_tex = glGetUniformLocation(blend_program, "bot_texture");
        if (blend_uniform_bot_tex == -1) {
            std::cout << "blend: Could not bind uniform: bot_texture" << std::endl;
        }
    }
    // MSAA_SAMPLES for multisampling
    {
        vertice_shader = create_shader(":/src/shaders/msaa.v.glsl", GL_VERTEX_SHADER);
        fragment_shader = create_shader(":/src/shaders/msaa.f.glsl", GL_FRAGMENT_SHADER);

        msaa_program = glCreateProgram();
        glAttachShader(msaa_program, vertice_shader);
        glAttachShader(msaa_program, fragment_shader);
        glLinkProgram(msaa_program);

        glGetProgramiv(msaa_program, GL_LINK_STATUS, &link_ok);
        if (!link_ok) {
            std::cout << "glLinkProgram: ";
            GLint log_length = 0;

            if (glIsProgram(msaa_program))
            {
                glGetProgramiv(msaa_program, GL_INFO_LOG_LENGTH, &log_length);
                char* log = new char[log_length];

                glGetProgramInfoLog(msaa_program, log_length, NULL, log);
                std::cout << log << std::endl;

                delete[] log;
                glDeleteProgram(msaa_program);
            }
            else
            {
                std::cout << "Could not create GL program: Supplied argument is not a program!" << std::endl;
            }
        }

        msaa_attribute_position = glGetAttribLocation(msaa_program, "position");
        if (msaa_attribute_position == -1) {
            std::cout << "msaa: Could not bind attribute: position" << std::endl;
        }

        msaa_attribute_texpos = glGetAttribLocation(msaa_program, "texpos");
        if (msaa_attribute_texpos == -1) {
            std::cout << "msaa: Could not bind attribute: texpos" << std::endl;
        }

        msaa_uniform_samples = glGetUniformLocation(msaa_program, "samples");
        if (msaa_uniform_samples == -1) {
            std::cout << "msaa: Could not bind uniform: samples" << std::endl;
        }

        msaa_uniform_texture = glGetUniformLocation(msaa_program, "texy");
        if (msaa_uniform_texture == -1) {
            std::cout << "msaa: Could not bind uniform: texy" << std::endl;
        }

        msaa_uniform_weight = glGetUniformLocation(msaa_program, "sampleWeightSampler");
        if (msaa_uniform_weight == -1) {
            std::cout << "msaa: Could not bind uniform: sampleWeightSampler" << std::endl;
        }
    }

    // MSAA_SAMPLES for HDR multisampling
    {
        vertice_shader = create_shader(":/src/shaders/msaa_hdr.v.glsl", GL_VERTEX_SHADER);
        fragment_shader = create_shader(":/src/shaders/msaa_hdr.f.glsl", GL_FRAGMENT_SHADER);

        msaa_hdr_program = glCreateProgram();
        glAttachShader(msaa_hdr_program, vertice_shader);
        glAttachShader(msaa_hdr_program, fragment_shader);
        glLinkProgram(msaa_hdr_program);

        glGetProgramiv(msaa_hdr_program, GL_LINK_STATUS, &link_ok);
        if (!link_ok) {
            std::cout << "glLinkProgram: ";
            GLint log_length = 0;

            if (glIsProgram(msaa_hdr_program))
            {
                glGetProgramiv(msaa_hdr_program, GL_INFO_LOG_LENGTH, &log_length);
                char* log = new char[log_length];

                glGetProgramInfoLog(msaa_hdr_program, log_length, NULL, log);
                std::cout << log << std::endl;

                delete[] log;
                glDeleteProgram(msaa_hdr_program);
            }
            else
            {
                std::cout << "Could not create GL program: Supplied argument is not a program!" << std::endl;
            }
        }

        msaa_hdr_attribute_position = glGetAttribLocation(msaa_hdr_program, "position");
        if (msaa_hdr_attribute_position == -1) {
            std::cout << "msaa_hdr: Could not bind attribute: position" << std::endl;
        }

        msaa_hdr_attribute_texpos = glGetAttribLocation(msaa_hdr_program, "texpos");
        if (msaa_hdr_attribute_texpos == -1) {
            std::cout << "msaa_hdr: Could not bind attribute: texpos" << std::endl;
        }

        msaa_hdr_uniform_samples = glGetUniformLocation(msaa_hdr_program, "sampleCount");
        if (msaa_hdr_uniform_samples == -1) {
            std::cout << "msaa_hdr: Could not bind uniform: sampleCount" << std::endl;
        }

        msaa_hdr_uniform_method = glGetUniformLocation(msaa_hdr_program, "useWeightedResolve");
        if (msaa_hdr_uniform_method == -1) {
            std::cout << "msaa_hdr: Could not bind uniform: useWeightedResolve" << std::endl;
        }

        msaa_hdr_uniform_exposure = glGetUniformLocation(msaa_hdr_program, "exposure");
        if (msaa_hdr_uniform_exposure == -1) {
            std::cout << "msaa_hdr: Could not bind uniform: exposure" << std::endl;
        }

        msaa_hdr_uniform_texture = glGetUniformLocation(msaa_hdr_program, "origImage");
        if (msaa_hdr_uniform_texture == -1) {
            std::cout << "msaa_hdr: Could not bind uniform: origImage" << std::endl;
        }

        msaa_hdr_uniform_weight = glGetUniformLocation(msaa_hdr_program, "sampleWeightSampler");
        if (msaa_hdr_uniform_weight == -1) {
            std::cout << "msaa_hdr: Could not bind uniform: sampleWeightSampler" << std::endl;
        }
    }
}

void VolumeRenderGLWidget::gen_tsf_tex(TsfMatrix<double> * tsf)
{
    /* Generate a transfer function CL texture */
    if (tsf_tex_sampler) clReleaseSampler(tsf_tex_sampler);
    //~ if (tsf_tex_cl) clReleaseMemObject(tsf_tex_cl);

    // Buffer for tsf_tex
    glActiveTexture(GL_TEXTURE0);
    glDeleteTextures(1, &tsf_tex);
    glGenTextures(1, &tsf_tex);

    glBindTexture(GL_TEXTURE_2D, tsf_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        tsf->getSpline().getN(),
        1,
        0,
        GL_RGBA,
        GL_FLOAT,
        tsf->getSpline().getColMajor().toFloat().data());
    glBindTexture(GL_TEXTURE_2D, 0);

    // Buffer for tsf_tex_cl
    //~ tsf->getPreIntegrated().getColMajor().toFloat().print(2, "preIntegrated");

    cl_image_format tsf_format;
    tsf_format.image_channel_order = CL_RGBA;
    tsf_format.image_channel_data_type = CL_FLOAT;

    tsf_tex_cl = clCreateImage2D ( (*context2),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        &tsf_format,
        tsf->getSpline().getN(),
        1,
        0,
        tsf->getSpline().getColMajor().toFloat().data(),
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }

    // The sampler for tsf_tex_cl
    tsf_tex_sampler = clCreateSampler((*context2), true, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Could not create sampler: " << cl_error_cstring(err) << std::endl;
    }
    // SET KERNEL ARGS
    err = clSetKernelArg(K_SVO_RAYTRACE, 1, sizeof(cl_mem), (void *) &tsf_tex_cl);
    err |= clSetKernelArg(K_SVO_RAYTRACE, 6, sizeof(cl_sampler), &tsf_tex_sampler);
    err |= clSetKernelArg(K_FUNCTION_RAYTRACE, 1, sizeof(cl_mem), (void *) &tsf_tex_cl);
    err |= clSetKernelArg(K_FUNCTION_RAYTRACE, 2, sizeof(cl_sampler), &tsf_tex_sampler);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error setting kernel argument: " << cl_error_cstring(err) << std::endl;
    }
}

int VolumeRenderGLWidget::gen_ray_tex()
{
    /* Set a usable texture for thevolume rendering kernel */
    // Set dimensions
    ray_tex_dim[0] = (int)((float)WIDTH*ray_res*0.01f);
    ray_tex_dim[1] = (int)((float)HEIGHT*ray_res*0.01f);
    // Clamping
    if (ray_tex_dim[0] < 32) ray_tex_dim[0] = 32;
    if (ray_tex_dim[1] < 32) ray_tex_dim[1] = 32;
    // Global work size
    if (ray_tex_dim[0] % ray_loc_ws[0]) ray_glb_ws[0] = ray_loc_ws[0]*(1 + (ray_tex_dim[0] / ray_loc_ws[0]));
    else ray_glb_ws[0] = ray_tex_dim[0];
    if (ray_tex_dim[1] % ray_loc_ws[1]) ray_glb_ws[1] = ray_loc_ws[1]*(1 + (ray_tex_dim[1] / ray_loc_ws[1]));
    else ray_glb_ws[1] = ray_tex_dim[1];


    MISC_INT[6] = ray_tex_dim[0];
    MISC_INT[7] = ray_tex_dim[1];
    setMiscArrays();

    // Update GL texture
    glActiveTexture(GL_TEXTURE0);
    glDeleteTextures(1, &ray_tex);
    glGenTextures(1, &ray_tex);
    glBindTexture(GL_TEXTURE_2D, ray_tex);
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
    //~ if (ray_tex_cl) clReleaseMemObject(ray_tex_cl);

    ray_tex_cl = clCreateFromGLTexture2D((*context2), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, ray_tex, &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL object from GL texture: " << cl_error_cstring(err) << std::endl;
        return 0;
    }

    // Pass texture to CL kernel
    err = clSetKernelArg(K_SVO_RAYTRACE, 0, sizeof(cl_mem), (void *) &ray_tex_cl);
    err |= clSetKernelArg(K_FUNCTION_RAYTRACE, 0, sizeof(cl_mem), (void *) &ray_tex_cl);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error setting kernel argument: " << cl_error_cstring(err) << std::endl;
        return 0;
    }

    return 1;
}


int VolumeRenderGLWidget::init_cl()
{
    // Program
    QByteArray qsrc = open_resource(":/src/kernels/render.cl");
    const char * src = qsrc.data();
    size_t src_length = strlen(src);

    program = clCreateProgramWithSource((*context2), 1, (const char **)&src, &src_length, &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "VolumeRenderGLWidget: Could not create program from source: " << cl_error_cstring(err) << std::endl;
        return 0;
    }

    // Compile kernel
    const char * options = "-cl-single-precision-constant -cl-mad-enable -cl-fast-relaxed-math";
    err = clBuildProgram(program, 1, &device->device_id, options, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        // Compile log
        std::cout << "Could not compile/link program: " << cl_error_cstring(err) << std::endl;
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
    K_SVO_RAYTRACE = clCreateKernel(program, "svoRayTrace", &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Could not create kernel object: " << cl_error_cstring(err) << std::endl;
        return 0;
    }

    K_FUNCTION_RAYTRACE = clCreateKernel(program, "modelRayTrace", &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Could not create kernel object: " << cl_error_cstring(err) << std::endl;
        return 0;
    }

    // Buffers
    view_matrix_inv_cl = clCreateBuffer((*context2),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        16*sizeof(cl_float),
        NULL,
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }

    function_view_matrix_inv_cl = clCreateBuffer((*context2),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        16*sizeof(cl_float),
        NULL,
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }

    data_extent_cl = clCreateBuffer((*context2),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        8*sizeof(cl_float),
        NULL,
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }

    data_view_extent_cl = clCreateBuffer((*context2),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        8*sizeof(cl_float),
        NULL,
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }

    tsf_parameters_cl = clCreateBuffer((*context2),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        TSF_PARAMETERS.size()*sizeof(cl_float),
        NULL,
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }

    misc_float_cl = clCreateBuffer((*context2),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        MISC_FLOAT_K_FUNCTION.size()*sizeof(cl_float),
        NULL,
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }

    misc_int_cl = clCreateBuffer((*context2),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        MISC_INT.size()*sizeof(cl_float),
        NULL,
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }

    misc_float_k_raytrace_cl = clCreateBuffer((*context2),
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        MISC_FLOAT_K_RAYTRACE.size()*sizeof(cl_float),
        NULL,
        &err);
    if (err != CL_SUCCESS)
    {
        std::cout << "Error creating CL buffer: " << cl_error_cstring(err) << std::endl;
    }

    return 1;
}

