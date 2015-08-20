#ifndef GLSLFUNCTIONS_H
#define GLSLFUNCTIONS_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

#include "../math/qxmathlib.h"

class GLSLFunctions : private QOpenGLFunctions
{
    public:
        GLSLFunctions();
        ~GLSLFunctions();

        QPointF posGLtoQt(QPointF coord, size_t w, size_t h);
        QPointF posQttoGL(QPointF coord, size_t w, size_t h);

        void getPosition2D(float * pos_2d, float * pos_3d, Matrix<double> * transform);
        void getPosition2D(double * pos_2d, double * pos_3d, Matrix<double> * transform);

        void setVbo(GLuint vbo, float * buf, size_t length, GLenum usage);
        Matrix<GLfloat> glRect(QRectF &qt_rect, size_t width, size_t height);

        // Shaders
        GLint std_2d_tex_fragpos;
        GLint std_2d_tex_pos;
        GLint std_2d_tex_texture;
        GLint std_2d_tex_transform;
        QOpenGLShaderProgram * std_2d_tex_program;

        GLint rect_hl_2d_tex_fragpos;
        GLint rect_hl_2d_tex_pos;
        GLint rect_hl_2d_tex_texture;
        GLint rect_hl_2d_tex_transform;
        GLint rect_hl_2d_tex_bounds;
        GLint rect_hl_2d_tex_pixel_size;
        QOpenGLShaderProgram * rect_hl_2d_tex_program;

        GLint std_2d_col_color;
        GLint std_2d_col_transform;
        GLint std_2d_col_fragpos;
        QOpenGLShaderProgram * std_2d_col_program;

        GLint std_3d_col_color;
        GLint std_3d_col_transform;
        GLint std_3d_col_fragpos;
        QOpenGLShaderProgram * std_3d_col_program;

        GLint unitcell_color;
        GLint unitcell_transform;
        GLint unitcell_fragpos;
        GLint unitcell_lim_low;
        GLint unitcell_lim_high;
        GLint unitcell_u;
        QOpenGLShaderProgram * unitcell_program;

        GLint std_blend_fragpos;
        GLint std_blend_texpos;
        GLint std_blend_tex_a;
        GLint std_blend_tex_b;
        GLint std_blend_method;
        QOpenGLShaderProgram * std_blend_program;
};


#endif
