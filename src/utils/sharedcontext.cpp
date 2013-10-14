#include "sharedcontext.h"

SharedContextWindow::SharedContextWindow()
    : m_program(0),
    std_2d_tex_program(0),
    std_3d_color_program(0),
    std_blend_program(0)
{

}

SharedContextWindow::~SharedContextWindow()
{
}

static const char *vertexShaderSource =
    "attribute highp vec4 posAttr;\n"
    "attribute lowp vec4 colAttr;\n"
    "varying lowp vec4 col;\n"
    "uniform highp mat4 matrix;\n"
    "void main() {\n"
    "   col = colAttr;\n"
    "   gl_Position = matrix * posAttr;\n"
    "}\n";

static const char *fragmentShaderSource =
    "varying lowp vec4 col;\n"
    "void main() {\n"
    "   gl_FragColor = col;\n"
    "}\n";

GLuint SharedContextWindow::loadShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);
    return shader;
}

void SharedContextWindow::initialize()
{
    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();
    m_posAttr = m_program->attributeLocation("posAttr");
    m_colAttr = m_program->attributeLocation("colAttr");
    m_matrixUniform = m_program->uniformLocation("matrix");

    // Shader for drawing textures in 2D
    std_2d_tex_program = new QOpenGLShaderProgram(this);
    std_2d_tex_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "://src/shaders/std_2d_tex.v.glsl");
    std_2d_tex_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "://src/shaders/std_2d_tex.f.glsl");
    if (!std_2d_tex_program->link()) qFatal(std_2d_tex_program->log().toStdString().c_str());

    if ((std_2d_fragpos = std_2d_tex_program->attributeLocation("fragpos")) == -1) qCritical("Invalid attribute");
    if ((std_2d_texpos = std_2d_tex_program->attributeLocation("texpos")) == -1) qCritical("Invalid attribute");
    if ((std_2d_texture = std_2d_tex_program->uniformLocation("texture")) == -1) qCritical("Invalid uniform");

    // Shader for drawing lines and similar in 3D
    std_3d_color_program = new QOpenGLShaderProgram(this);
    std_3d_color_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "://src/shaders/std_3d_color.v.glsl");
    std_3d_color_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "://src/shaders/std_3d_color.f.glsl");
    if (!std_3d_color_program->link()) qFatal(std_2d_tex_program->log().toStdString().c_str());

    if ((std_3d_fragpos = std_3d_color_program->attributeLocation("fragpos")) == -1) qCritical("Invalid attribute");
    if ((std_3d_texpos = std_3d_color_program->attributeLocation("texpos")) == -1) qCritical("Invalid attribute");
    if ((std_3d_color = std_3d_color_program->uniformLocation("color")) == -1) qCritical("Invalid uniform");
    if ((std_3d_transform = std_3d_color_program->uniformLocation("transform")) == -1) qCritical("Invalid uniform");

    // Shader for blending two textures
    std_blend_program = new QOpenGLShaderProgram(this);
    std_blend_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "://src/shaders/std_blend.v.glsl");
    std_blend_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "://src/shaders/std_blend.f.glsl");
    if (!std_blend_program->link()) qFatal(std_2d_tex_program->log().toStdString().c_str());

    if ((std_blend_fragpos = std_blend_program->attributeLocation("fragpos")) == -1) qCritical("Invalid attribute");
    if ((std_blend_texpos = std_blend_program->attributeLocation("texpos")) == -1) qCritical("Invalid attribute");
    if ((std_blend_tex_a = std_blend_program->uniformLocation("texture_alpha")) == -1) qCritical("Invalid uniform");
    if ((std_blend_tex_b = std_blend_program->uniformLocation("texture_beta")) == -1) qCritical("Invalid uniform");
    if ((std_blend_method = std_blend_program->uniformLocation("method")) == -1) qCritical("Invalid uniform");

    // Initialization of OpenCL/OpenGL interop context
    context_cl->initDevices();
    context_cl->initSharedContext();
    context_cl->initCommandQueue();
}


void SharedContextWindow::setMessageString(QString str)
{
    emit changedMessageString(str);
}


