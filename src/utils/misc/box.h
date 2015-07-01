#ifndef BOX_H
#define BOX_H

#include <QOpenGLFunctions>

#include "../math/qxmathlib.h"

class Box : protected QOpenGLFunctions
{
    // A box is spanned by three orthogonal vectors. A box has its own translation and rotation matrices. It has a rotation center in the middle.

    public:
        Box();
        /*Box(const Box &other);
        ~Box();

        GLuint * vbo();

        void alignWithVec(Matrix<double> vec);
        void setCenter(Matrix<double> pos);
        void setComment(QString str);

        void setSideA(double value);
        void setSideB(double value);
        void setSideC(double value);

        void rotate(Matrix<double> mat);
        void translate(Matrix<double> mat);

        double sideA() const;
        double sideB() const;
        double sideC() const;

        const Matrix<double> aVec() const;
        const Matrix<double> bVec() const;
        const Matrix<double> cVec() const;

        const Matrix<float> &vertices() const;
        const QString comment() const;

    private:
        double p_side_a;
        double p_side_b;
        double p_side_c;

        Matrix<double> rotation;
        Matrix<double> translation;

        GLuint p_vbo;

        bool p_verts_computed;

        QString p_comment;

        Matrix<float> p_vertices;

        void computeVertices();*/
};

Q_DECLARE_METATYPE(Box);

/*QDebug operator<<(QDebug dbg, const Box &Box);
QDebug operator<<(QDebug dbg, const Box &Box);
QDataStream &operator<<(QDataStream &out, const Box &Box);
QDataStream &operator>>(QDataStream &in, Box &Box);
QTextStream &operator<<(QTextStream &out, const Box &Box);*/

#endif // BOX_H
