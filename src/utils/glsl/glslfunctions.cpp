#include "glslfunctions.h"

GLSLFunctions::GLSLFunctions()
{
    QOpenGLFunctions::initializeOpenGLFunctions();
    
    // Shader for drawing textures in 2D
    std_2d_tex_program = new QOpenGLShaderProgram();
    std_2d_tex_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/std_2d_tex.v.glsl");
    std_2d_tex_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/std_2d_tex.f.glsl");
    if (!std_2d_tex_program->link()) qFatal(std_2d_tex_program->log().toStdString().c_str());

    if ((std_2d_tex_fragpos = std_2d_tex_program->attributeLocation("fragpos")) == -1) qFatal("Invalid attribute");
    if ((std_2d_tex_pos = std_2d_tex_program->attributeLocation("texpos")) == -1) qFatal("Invalid attribute");
    if ((std_2d_tex_texture = std_2d_tex_program->uniformLocation("texture")) == -1) qFatal("Invalid uniform");
    if ((std_2d_tex_transform = std_2d_tex_program->uniformLocation("transform")) == -1) qFatal("Invalid uniform");

    // Shader for drawing textures in 2D with a highlighted rectangle
    rect_hl_2d_tex_program = new QOpenGLShaderProgram();
    rect_hl_2d_tex_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/rect_hl_2d_tex.v.glsl");
    rect_hl_2d_tex_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/rect_hl_2d_tex.f.glsl");
    if (!rect_hl_2d_tex_program->link()) qFatal(rect_hl_2d_tex_program->log().toStdString().c_str());

    if ((rect_hl_2d_tex_fragpos = rect_hl_2d_tex_program->attributeLocation("fragpos")) == -1) qFatal("Invalid attribute");
    if ((rect_hl_2d_tex_pos = rect_hl_2d_tex_program->attributeLocation("texpos")) == -1) qFatal("Invalid attribute");
    if ((rect_hl_2d_tex_texture = rect_hl_2d_tex_program->uniformLocation("texture")) == -1) qFatal("Invalid uniform");
    if ((rect_hl_2d_tex_transform = rect_hl_2d_tex_program->uniformLocation("transform")) == -1) qFatal("Invalid uniform");

    // Shader for drawing lines and similar in 3D
    std_3d_col_program = new QOpenGLShaderProgram();
    std_3d_col_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/std_3d_col.v.glsl");
    std_3d_col_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/std_3d_col.f.glsl");
    if (!std_3d_col_program->link()) qFatal(std_3d_col_program->log().toStdString().c_str());

    if ((std_3d_col_fragpos = std_3d_col_program->attributeLocation("fragpos")) == -1) qFatal("Invalid attribute");
    if ((std_3d_col_color = std_3d_col_program->uniformLocation("color")) == -1) qFatal("Invalid uniform");
    if ((std_3d_col_transform = std_3d_col_program->uniformLocation("transform")) == -1) qFatal("Invalid uniform");

    // Shader for drawing lines and similar in 2D
    std_2d_col_program = new QOpenGLShaderProgram();
    std_2d_col_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/std_2d_col.v.glsl");
    std_2d_col_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/std_2d_col.f.glsl");
    if (!std_2d_col_program->link()) qFatal(std_2d_col_program->log().toStdString().c_str());

    if ((std_2d_col_fragpos = std_2d_col_program->attributeLocation("fragpos")) == -1) qFatal("Invalid attribute");
    if ((std_2d_col_color = std_2d_col_program->uniformLocation("color")) == -1) qFatal("Invalid uniform");
    if ((std_2d_col_transform = std_2d_col_program->uniformLocation("transform")) == -1) qFatal("Invalid uniform");

    // Shader for blending two textures
    std_blend_program = new QOpenGLShaderProgram();
    std_blend_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/std_blend.v.glsl");
    std_blend_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/std_blend.f.glsl");
    if (!std_blend_program->link()) qFatal(std_blend_program->log().toStdString().c_str());

    if ((std_blend_fragpos = std_blend_program->attributeLocation("fragpos")) == -1) qFatal("Invalid attribute");
    if ((std_blend_texpos = std_blend_program->attributeLocation("texpos")) == -1) qFatal("Invalid attribute");
    if ((std_blend_tex_a = std_blend_program->uniformLocation("texture_alpha")) == -1) qFatal("Invalid uniform");
    if ((std_blend_tex_b = std_blend_program->uniformLocation("texture_beta")) == -1) qFatal("Invalid uniform");
    if ((std_blend_method = std_blend_program->uniformLocation("method")) == -1) qFatal("Invalid uniform");

    // Shader for drawing a (reciprocal) unitcell grid
    unitcell_program = new QOpenGLShaderProgram();
    unitcell_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/unitcell.v.glsl");
    unitcell_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/unitcell.f.glsl");
    if (!unitcell_program->link()) qFatal(unitcell_program->log().toStdString().c_str());

    if ((unitcell_fragpos = unitcell_program->attributeLocation("fragpos")) == -1) qFatal("Invalid attribute");
    if ((unitcell_color = unitcell_program->uniformLocation("color")) == -1) qFatal("Invalid uniform");
    if ((unitcell_transform = unitcell_program->uniformLocation("transform")) == -1) qFatal("Invalid uniform");
    if ((unitcell_u = unitcell_program->uniformLocation("u")) == -1) qFatal("Invalid uniform");
    if ((unitcell_lim_low = unitcell_program->uniformLocation("lim_low")) == -1) qFatal("Invalid uniform");
    if ((unitcell_lim_high = unitcell_program->uniformLocation("lim_high")) == -1) qFatal("Invalid uniform");
}

GLSLFunctions::~GLSLFunctions()
{
    
}

void GLSLFunctions::setVbo(GLuint vbo, float * buf, size_t length, GLenum usage)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT)*length, buf, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

QPointF GLSLFunctions::posGLtoQt(QPointF coord, size_t w, size_t h)
{
    QPointF QtPoint;

    QtPoint.setX(0.5 * (float)w * (coord.x()+1.0) - 1.0);
    QtPoint.setY(0.5 * (float)h * (1.0-coord.y()) - 1.0);

    return QtPoint;
}

QPointF GLSLFunctions::posQttoGL(QPointF coord, size_t w, size_t h)
{
    QPointF GLPoint;
    GLPoint.setX((coord.x()+1.0)/(float) (w)*2.0-1.0);
    GLPoint.setY((1.0 - (coord.y()+1.0)/(float) h)*2.0-1.0);
    return GLPoint;
}

void GLSLFunctions::getPosition2D(float * pos_2d, float * pos_3d, Matrix<double> * transform)
{
    Matrix<float> pos_3d_matrix;
    Matrix<float> pos_2d_matrix(4, 1);

    pos_3d_matrix.setDeep(4, 1, pos_3d);
    pos_3d_matrix[3] = 1.0;

    pos_2d_matrix = transform->toFloat() * pos_3d_matrix;

    pos_2d[0] = pos_2d_matrix[0]/pos_2d_matrix[3];
    pos_2d[1] = pos_2d_matrix[1]/pos_2d_matrix[3];
}

void GLSLFunctions::getPosition2D(double * pos_2d, double * pos_3d, Matrix<double> * transform)
{
    Matrix<double> pos_3d_matrix;
    Matrix<double> pos_2d_matrix(4, 1);

    pos_3d_matrix.setDeep(4, 1, pos_3d);
    pos_3d_matrix[3] = 1.0;

    pos_2d_matrix = *transform * pos_3d_matrix;

    pos_2d[0] = pos_2d_matrix[0]/pos_2d_matrix[3];
    pos_2d[1] = pos_2d_matrix[1]/pos_2d_matrix[3];
}

Matrix<GLfloat> GLSLFunctions::glRect(QRectF & qt_rect, size_t width, size_t height)
{
    Matrix<GLfloat> gl_rect(1,8);

    qreal x,y,w,h;
    qreal xf,yf,wf,hf;
    qt_rect = qt_rect.normalized();
    qt_rect.getRect(&x, &y, &w, &h);

    xf = (x / (qreal) width) * 2.0 - 1.0;
    yf = 1.0 - (y + h)/ (qreal) height * 2.0;
    wf = (w / (qreal) width) * 2.0;
    hf = (h / (qreal) height) * 2.0;

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
