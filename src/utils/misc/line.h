#ifndef LINE_H
#define LINE_H

#include <QTableView>
#include <QList>
#include <QOpenGLFunctions>

#include "../math/qxmathlib.h"

class Line : protected QOpenGLFunctions
{
        // The length of the line is given by: vecLen(position_a - position_b) + offset_a + offset_b

    public:
        Line();
        Line(const Line &other);
        Line(Matrix<double> pos_a, Matrix<double> pos_b);
        Line(double x0, double y0, double z0, double x1, double y1, double z1);
        ~Line();

        GLuint * vbo();

        void setPositionA(Matrix<double> pos);
        void setPositionB(Matrix<double> pos);
        void setTagged(bool value);
        void setComment(QString str);
        void setOffsetA(double value);
        void setOffsetB(double value);

        void setPrismSideA(double value);
        void setPrismSideB(double value);

        double prismSideA() const;
        double prismSideB() const;

        Matrix<double> basePos();
        const Matrix<double> aVec() const;
        const Matrix<double> bVec() const;
        const Matrix<double> cVec() const;

        Matrix<double> effectivePosA();
        Matrix<double> effectivePosB();
        double length() const;
        double distancePointToLine(Matrix<double> pos);
        const Matrix<double> &positionA() const;
        const Matrix<double> &positionB() const;
        double offsetA() const;
        double offsetB() const;
        const Matrix<float> &vertices() const;
        const QString comment() const;
        bool tagged() const;

    private:
        Matrix<double> p_a;
        Matrix<double> p_b;
        Matrix<double> p_c;


        GLuint p_vbo;

        bool p_is_tagged;
        bool p_verts_computed;

        double p_offset_a;
        double p_offset_b;
        QString p_comment;

        Matrix<double> p_position_a;
        Matrix<double> p_position_b;
        Matrix<float> p_vertices;

        void computeVertices();
        void computePrism();

        double p_prism_side_a;
        double p_prism_side_b;
};

Q_DECLARE_METATYPE(Line);

QDebug operator<<(QDebug dbg, const Line &line);

QDataStream &operator<<(QDataStream &out, const Line &line);
QDataStream &operator>>(QDataStream &in, Line &line);

#endif // LINE_H
