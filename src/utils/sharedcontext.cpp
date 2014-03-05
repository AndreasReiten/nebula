#include "sharedcontext.h"

SharedContextWindow::SharedContextWindow()
    : std_2d_tex_program(0),
    std_2d_col_program(0),
    std_3d_col_program(0),
    std_blend_program(0),
    gl_worker(0)
{

}

SharedContextWindow::~SharedContextWindow()
{
}

void SharedContextWindow::initializeWorker()
{
    initializeGLContext();
}

GLuint SharedContextWindow::loadShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);
    return shader;
}

void SharedContextWindow::initialize()
{
    // Shader for drawing textures in 2D
    std_2d_tex_program = new QOpenGLShaderProgram(this);
    std_2d_tex_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "://src/shaders/std_2d_tex.v.glsl");
    std_2d_tex_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "://src/shaders/std_2d_tex.f.glsl");
    if (!std_2d_tex_program->link()) qFatal(std_2d_tex_program->log().toStdString().c_str());

    if ((std_2d_tex_fragpos = std_2d_tex_program->attributeLocation("fragpos")) == -1) qCritical("Invalid attribute");
    if ((std_2d_tex_pos = std_2d_tex_program->attributeLocation("texpos")) == -1) qCritical("Invalid attribute");
    if ((std_2d_tex_texture = std_2d_tex_program->uniformLocation("texture")) == -1) qCritical("Invalid uniform");

    // Shader for drawing lines and similar in 3D
    std_3d_col_program = new QOpenGLShaderProgram(this);
    std_3d_col_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "://src/shaders/std_3d_col.v.glsl");
    std_3d_col_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "://src/shaders/std_3d_col.f.glsl");
    if (!std_3d_col_program->link()) qFatal(std_3d_col_program->log().toStdString().c_str());

    if ((std_3d_col_fragpos = std_3d_col_program->attributeLocation("fragpos")) == -1) qCritical("Invalid attribute");
    if ((std_3d_col_color = std_3d_col_program->uniformLocation("color")) == -1) qCritical("Invalid uniform");
    if ((std_3d_col_transform = std_3d_col_program->uniformLocation("transform")) == -1) qCritical("Invalid uniform");
    
    // Shader for drawing lines and similar in 2D
    std_2d_col_program = new QOpenGLShaderProgram(this);
    std_2d_col_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "://src/shaders/std_2d_col.v.glsl");
    std_2d_col_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "://src/shaders/std_2d_col.f.glsl");
    if (!std_2d_col_program->link()) qFatal(std_2d_col_program->log().toStdString().c_str());

    if ((std_2d_col_fragpos = std_2d_col_program->attributeLocation("fragpos")) == -1) qCritical("Invalid attribute");
    if ((std_2d_col_color = std_2d_col_program->uniformLocation("color")) == -1) qCritical("Invalid uniform");
    if ((std_2d_col_transform = std_2d_col_program->uniformLocation("transform")) == -1) qCritical("Invalid uniform");

    // Shader for blending two textures
    std_blend_program = new QOpenGLShaderProgram(this);
    std_blend_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "://src/shaders/std_blend.v.glsl");
    std_blend_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "://src/shaders/std_blend.f.glsl");
    if (!std_blend_program->link()) qFatal(std_blend_program->log().toStdString().c_str());

    if ((std_blend_fragpos = std_blend_program->attributeLocation("fragpos")) == -1) qCritical("Invalid attribute");
    if ((std_blend_texpos = std_blend_program->attributeLocation("texpos")) == -1) qCritical("Invalid attribute");
    if ((std_blend_tex_a = std_blend_program->uniformLocation("texture_alpha")) == -1) qCritical("Invalid uniform");
    if ((std_blend_tex_b = std_blend_program->uniformLocation("texture_beta")) == -1) qCritical("Invalid uniform");
    if ((std_blend_method = std_blend_program->uniformLocation("method")) == -1) qCritical("Invalid uniform");
    
    // Shader for drawing a (reciprocal) unitcell grid
    unitcell_program = new QOpenGLShaderProgram(this);
    unitcell_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "://src/shaders/unitcell.v.glsl");
    unitcell_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "://src/shaders/unitcell.f.glsl");
    if (!unitcell_program->link()) qFatal(unitcell_program->log().toStdString().c_str());

    if ((unitcell_fragpos = unitcell_program->attributeLocation("fragpos")) == -1) qCritical("Invalid attribute");
    if ((unitcell_color = unitcell_program->uniformLocation("color")) == -1) qCritical("Invalid uniform");
    if ((unitcell_transform = unitcell_program->uniformLocation("transform")) == -1) qCritical("Invalid uniform");
    if ((unitcell_lim_low = unitcell_program->uniformLocation("lim_low")) == -1) qCritical("Invalid uniform");
    if ((unitcell_lim_high = unitcell_program->uniformLocation("lim_high")) == -1) qCritical("Invalid uniform");
    if ((unitcell_diagonal = unitcell_program->uniformLocation("unitcell_diagonal")) == -1) qCritical("Invalid uniform");
    
    // Initialization of OpenCL/OpenGL interop context
    this->getCLContext()->initDevices();
    this->getCLContext()->initSharedContext();
    this->getCLContext()->initCommandQueue();
}


void SharedContextWindow::setMessageString(QString str)
{
    emit changedMessageString(str);
}


