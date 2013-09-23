#include "imagerender.h"
//~ if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": At line "+QString::number(__LINE__));
ImageRenderGLWidget::ImageRenderGLWidget(cl_device * device, cl_context * context, cl_command_queue * queue, const QGLFormat & format, QWidget *parent, const QGLWidget * shareWidget) :
    QGLWidget(format, parent, shareWidget)
{
    verbosity = 1;
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    isGLIntitialized = false;
    isAlphaImgInitialized = false;
    isBetaImgInitialized = false;
    isGammaImgInitialized = false;
    isTsfImgInitialized= false;;

    this->device = device;
    this->queue = queue;
    this->context = context;

    /* colors */
    float c_white[4] = {1,1,1,1};
    float c_transparent[4] = {0,0,0,0};
    float c_black[4] = {0,0,0,1};
    float c_blue[4] = {0,0,1,1};
    float c_red[4] = {1,0,0,1};
    float c_green[4] = {0,1,0,1};


    white.setDeep(4, c_white);
    transparent.setDeep(4, c_transparent);
    black.setDeep(4, c_black);
    blue.setDeep(4, c_blue);
    red.setDeep(4, c_red);
    green.setDeep(4, c_green);

    this->image_w = 32;
    this->image_h = 32;
    this->WIDTH = 32;
    this->HEIGHT = 32;

    this->setFocusPolicy(Qt::StrongFocus);


    /* This timer keeps track of the time since this constructor was
     * called */
    time = new QElapsedTimer();
    time->start();

    /* This timer emits a signal every 1000.0/FPS_MAX milli seconds.
     * The slot is the QGL repaint function */
    timer = new QTimer(this);

    //~int FPS_MAX = 30;
    //~timer->start(1000.0/FPS_MAX);
    //~connect(timer,SIGNAL(timeout()),this,SLOT(repaint()));

    this->glInit();
}

void ImageRenderGLWidget::setImageWidth(int value)
{
    if (value != image_w)
    {
        //~this->releaseSharedBuffers();
        this->image_w = value;
        this->setTarget();
        this->setTexturePositions();
        //~this->aquireSharedBuffers();
    }
}

void ImageRenderGLWidget::setImageHeight(int value)
{
    if (value != image_h)
    {
        //~this->releaseSharedBuffers();
        this->image_h = value;
        this->setTarget();
        this->setTexturePositions();
        //~this->aquireSharedBuffers();
    }
}

void ImageRenderGLWidget::setImageSize(int w, int h)
{
    if ((w != image_w) || (h != image_h))
    {
        this->image_w = w;
        this->image_h = h;
        this->setTarget();
        this->setTexturePositions();
    }
}

ImageRenderGLWidget::~ImageRenderGLWidget()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    if (isGLIntitialized)
    {
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] Line "+QString::number(__LINE__));
        if (isAlphaImgInitialized) clReleaseMemObject(alpha_img_clgl);
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] Line "+QString::number(__LINE__));
        if (isBetaImgInitialized) clReleaseMemObject(beta_img_clgl);
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] Line "+QString::number(__LINE__));
        if (isGammaImgInitialized) clReleaseMemObject(gamma_img_clgl);
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] Line "+QString::number(__LINE__));
        glDeleteTextures(5, image_tex);
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] Line "+QString::number(__LINE__));
        glDeleteBuffers(1, &text_coord_vbo);
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] Line "+QString::number(__LINE__));
        glDeleteBuffers(1, &text_texpos_vbo);
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] Line "+QString::number(__LINE__));
        glDeleteBuffers(5, screen_texpos_vbo);
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] Line "+QString::number(__LINE__));
        glDeleteBuffers(5, screen_coord_vbo);
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] Line "+QString::number(__LINE__));
        glDeleteProgram(std_text_program);
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] Line "+QString::number(__LINE__));
        glDeleteProgram(std_2d_tex_program);
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] Line "+QString::number(__LINE__));
        glDeleteProgram(std_2d_color_program);
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] Line "+QString::number(__LINE__));
    }
}

void ImageRenderGLWidget::initFreetype()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    /* Initialize the FreeType2 library */
    FT_Library ft;
    FT_Face face;
    FT_Error error;

    //~ QByteArray qsrc = open_resource(":/src/fonts/FreeMono.ttf");
    //~ const char * fontfilename = qsrc.data();
    const char * fontfilename = "../../../src/fonts/FreeMono.ttf";

    error = FT_Init_FreeType(&ft);
    if(error)
    {
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__));
        if (verbosity == 1) writeLog("Could not init freetype library");
    }
    /* Load a font */
    if(FT_New_Face(ft, fontfilename, 0, &face))
    {
        if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__));
        if (verbosity == 1) writeLog("Could not open font: "+QString(fontfilename));
    }

    fontSmall = new Atlas(face, 12);
    fontMedium = new Atlas(face, 24);
    fontLarge = new Atlas(face, 48);
}

QSize ImageRenderGLWidget::minimumSizeHint() const
{
    return QSize(100, 100);
}

QSize ImageRenderGLWidget::sizeHint() const
{
    return QSize(2000, 2000);
}

void ImageRenderGLWidget::initializeGL()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    if (!isGLIntitialized){
        if (!this->initResourcesGL()) std::cout << "Error initializing OpenGL" << std::endl;

        /* Initialize and set the other stuff */
        this->initFreetype();
        this->setTarget();
        this->setTsfTexture(&transferFunction);

        isGLIntitialized = true;
    }
}

void ImageRenderGLWidget::writeLog(QString str)
{
    writeToLogAndPrint(str.toStdString().c_str(), "riv.log", 1);
}

void ImageRenderGLWidget::paintGL()
{

    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    GLuint indices[6] = {0,1,3,1,2,3};
    std_2d_color_draw(indices, 6, black.data(),  &screen_coord_vbo[0]);
    std_2d_color_draw(indices, 6, black.data(),  &screen_coord_vbo[1]);
    std_2d_color_draw(indices, 6, black.data(),  &screen_coord_vbo[2]);

    std_2d_tex_draw(indices, 6, 0, image_tex[0], &screen_coord_vbo[0], &screen_texpos_vbo[0]);
    std_2d_tex_draw(indices, 6, 0, image_tex[1], &screen_coord_vbo[1], &screen_texpos_vbo[1]);
    std_2d_tex_draw(indices, 6, 0, image_tex[2], &screen_coord_vbo[2], &screen_texpos_vbo[2]);

    Matrix<float> xy(2,1);
    xy[0] = -1.0;
    xy[1] = -1.0;

    std_text_draw("Raw Data", fontMedium, white.data(), xy.data(), 1.0, this->WIDTH, this->HEIGHT);

    float IMAGE_WIDTH = 2*((float)image_w/(float)image_h)*((float)HEIGHT/(float)WIDTH);
    xy[0] = xy[0] + IMAGE_WIDTH+ 0.02;
    std_text_draw("BG Subtracted, Threshold One", fontMedium, white.data(), xy.data(), 1.0, this->WIDTH, this->HEIGHT);

    xy[0] = xy[0] + IMAGE_WIDTH+ 0.02;
    std_text_draw("LP Correction, Threshold Two", fontMedium, white.data(), xy.data(), 1.0, this->WIDTH, this->HEIGHT);

}


void ImageRenderGLWidget::std_2d_tex_draw(GLuint * elements, int num_elements, int active_tex, GLuint texture, GLuint * xy_coords, GLuint * tex_coords)
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


void ImageRenderGLWidget::std_2d_color_draw(GLuint * elements, int num_elements, GLfloat * color, GLuint * xy_coords)
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



void ImageRenderGLWidget::resizeGL(int w, int h)
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    this->WIDTH = w;
    this->HEIGHT = h;
    this->setTexturePositions();
    glViewport(0, 0, w, h);
}

void ImageRenderGLWidget::releaseSharedBuffers()
{
    //~if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

     // Release shared CL/GL objects
    err = clEnqueueReleaseGLObjects((*queue), 1, &alpha_img_clgl, 0, 0, 0);
    err |= clEnqueueReleaseGLObjects((*queue), 1, &beta_img_clgl, 0, 0, 0);
    err |= clEnqueueReleaseGLObjects((*queue), 1, &gamma_img_clgl, 0, 0, 0);
    err |= clEnqueueReleaseGLObjects((*queue), 1, &tsf_img_clgl, 0, 0, 0);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
    }
}
void ImageRenderGLWidget::setMessageString(QString str)
{
        emit changedMessageString(str);
}



void ImageRenderGLWidget::setTexturePositions()
{
    /* Set the positions of the textures */

    Matrix<float> imgAlphaCoord(2,4);
    Matrix<float> imgBetaCoord(2,4);
    Matrix<float> imgGammaCoord(2,4);

    float IMAGE_WIDTH = 2*((float)image_w/(float)image_h)*((float)HEIGHT/(float)WIDTH);
    float X0 = -1;
    float X1 = X0 + IMAGE_WIDTH;
    float X2 = X1 + 0.02;
    float X3 = X2 + IMAGE_WIDTH;
    float X4 = X3 + 0.02;
    float X5 = X4 + IMAGE_WIDTH;

    float Y0 = -1;
    float Y1 = 1;

    {
        float buf[8] = {
            X0, X1, X1, X0,
            Y0, Y0, Y1, Y1};
        imgAlphaCoord.setDeep(2, 4, buf);
    }
    {
        float buf[8] = {
            X2, X3, X3, X2,
            Y0, Y0, Y1, Y1};
        imgBetaCoord.setDeep(2, 4, buf);
    }
    {
        float buf[8] = {
            X4, X5, X5, X4,
            Y0, Y0, Y1, Y1};
        imgGammaCoord.setDeep(2, 4, buf);
    }
    setVbo(&screen_coord_vbo[0], imgAlphaCoord.getColMajor().data(), imgAlphaCoord.size());
    setVbo(&screen_coord_vbo[1], imgBetaCoord.getColMajor().data(), imgBetaCoord.size());
    setVbo(&screen_coord_vbo[2], imgGammaCoord.getColMajor().data(), imgGammaCoord.size());
}


int ImageRenderGLWidget::initResourcesGL()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    isGLIntitialized = true;

    // BLEND
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Generate Textures
    glGenTextures(5, image_tex);

    // Shader programs
    initializeProgramsGL();

    // Vertice buffer objects
    glGenBuffers(5, screen_texpos_vbo);
    glGenBuffers(5, screen_coord_vbo);
    glGenBuffers(1, &text_coord_vbo);
    glGenBuffers(1, &text_texpos_vbo);

    {
        Matrix<float> mat;
        float buf[8] = {
            1.0,0.0,0.0,1.0,
            0.0,0.0,1.0,1.0};

        mat.setDeep(2, 4, buf);
        setVbo(&screen_texpos_vbo[0], mat.getColMajor().data(), mat.size());
        setVbo(&screen_texpos_vbo[1], mat.getColMajor().data(), mat.size());
        setVbo(&screen_texpos_vbo[2], mat.getColMajor().data(), mat.size());
        setVbo(&screen_texpos_vbo[3], mat.getColMajor().data(), mat.size());
        setVbo(&screen_texpos_vbo[4], mat.getColMajor().data(), mat.size());
    }

    init_tsf(3, 0, &transferFunction);

    return 1;
}


void ImageRenderGLWidget::std_text_draw(const char *text, Atlas *a, float * color, float * xy, float scale, int w, int h)
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
	//~ for(p = (const char *)text; *p; p++)
    for (p = (const uint8_t *)text; *p; p++)
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
    glBindBuffer(GL_ARRAY_BUFFER, text_coord_vbo);
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



void ImageRenderGLWidget::initializeProgramsGL()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    GLint link_ok = GL_FALSE;
    GLuint vertice_shader, fragment_shader; // Delete these after use?

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
                if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"]\n"+QString(log));

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
                if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"]\n"+QString(log));

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
                if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"]\n"+QString(log));

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
}


void ImageRenderGLWidget::setTsfTexture(TsfMatrix<double> * tsf)
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);
    /* Generate a transfer function CL texture */

    if (isTsfImgInitialized) clReleaseMemObject(tsf_img_clgl);

    // Buffer for tsf_tex
    glActiveTexture(GL_TEXTURE0);
    glDeleteTextures(1, &image_tex[3]);
    glGenTextures(1, &image_tex[3]);

    glBindTexture(GL_TEXTURE_2D, image_tex[3]);
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

    // Buffer for tsf_img_clgl
    tsf_img_clgl = clCreateFromGLTexture2D((*context), CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, image_tex[3], &err);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
    }
    isTsfImgInitialized = true;
}

cl_mem * ImageRenderGLWidget::getTsfImgCLGL()
{
    return &tsf_img_clgl;
}

cl_mem * ImageRenderGLWidget::getAlphaImgCLGL()
{
    return &alpha_img_clgl;
}

cl_mem * ImageRenderGLWidget::getGammaImgCLGL()
{
    return &gamma_img_clgl;
}

cl_mem * ImageRenderGLWidget::getBetaImgCLGL()
{
    return &beta_img_clgl;
}

//~void ImageRenderGLWidget::resizeEvent(QResizeEvent * event)
//~{
    //~this->makeCurrent();
    //~QSize size(event->size());
    //~this->resizeGL(size.rwidth(), size.rheight());
//~}
//~
//~void ImageRenderGLWidget::paintEvent(QPaintEvent * event)
//~{
    //~this->makeCurrent();
    //~if (!isGLIntitialized) this->initializeGL();
    //~this->paintGL();
    //~this->updateGL();
//~}


void ImageRenderGLWidget::aquireSharedBuffers()
{
    //~if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    glFinish();
    err = clEnqueueAcquireGLObjects((*queue), 1, &alpha_img_clgl, 0, 0, 0);
    err |= clEnqueueAcquireGLObjects((*queue), 1, &beta_img_clgl, 0, 0, 0);
    err |= clEnqueueAcquireGLObjects((*queue), 1, &gamma_img_clgl, 0, 0, 0);
    err |= clEnqueueAcquireGLObjects((*queue), 1, &tsf_img_clgl, 0, 0, 0);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
    }
}

int ImageRenderGLWidget::setTarget()
{
    if (verbosity == 1) writeLog("["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO);

    if (isAlphaImgInitialized) clReleaseMemObject(alpha_img_clgl);
    if (isBetaImgInitialized) clReleaseMemObject(beta_img_clgl);
    if (isGammaImgInitialized) clReleaseMemObject(gamma_img_clgl);

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
    alpha_img_clgl = clCreateFromGLTexture2D((*context), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, image_tex[0], &err);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
        return 0;
    }
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
    beta_img_clgl = clCreateFromGLTexture2D((*context), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, image_tex[1], &err);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
        return 0;
    }
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
    gamma_img_clgl = clCreateFromGLTexture2D((*context), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, image_tex[2], &err);
    if (err != CL_SUCCESS)
    {
        writeLog("[!]["+QString(this->metaObject()->className())+"] "+Q_FUNC_INFO+": Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));;
        return 0;
    }
    isGammaImgInitialized = true;
    return 1;
}

