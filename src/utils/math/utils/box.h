#ifndef BOX_H
#define BOX_H

#include "matrix.h"

/* UB matrix */
template <class T>
class BoxMatrix : public Matrix<T>
{
    public:
        BoxMatrix();
        BoxMatrix(T x0, T x1, T y0, T y1, T z0, T z1);
        ~BoxMatrix();

        Matrix<T> planeXY0();
        Matrix<T> planeXY1();
        Matrix<T> planeXZ0();
        Matrix<T> planeXZ1();
        Matrix<T> planeYZ0();
        Matrix<T> planeYZ1();

        BoxMatrix<T> &operator = (Matrix<T> other);
        BoxMatrix<T> &operator = (BoxMatrix<T> other);
};


template <class T>
BoxMatrix<T>::BoxMatrix()
{
    this->set(4, 2, 0);
}

template <class T>
BoxMatrix<T>::BoxMatrix(T x0, T x1, T y0, T y1, T z0, T z1)
{
    T buf[8] = {x0, x1, y0, y1, z0, z1, 1, 1};

    this->setDeep(4, 2, buf);
}

template <class T>
BoxMatrix<T>::~BoxMatrix()
{
    ;
}

template <class T>
Matrix<T> BoxMatrix<T>::planeXY0()
{
    Matrix<T> plane(4, 1);

    Matrix<T> norm(3, 1, 0);
    norm[2] = (this->data()[5] - this->data()[4]);

    norm = vecNormalize(norm);

    Matrix<T> origin(3, 1);
    origin[0] = this->data()[0];
    origin[1] = this->data()[2];
    origin[2] = this->data()[4];

    plane[0] = norm[0];
    plane[1] = norm[1];
    plane[2] = norm[2];
    plane[3] = -vecDot(norm, origin);

    return plane;
}

template <class T>
Matrix<T> BoxMatrix<T>::planeXY1()
{
    Matrix<T> plane(4, 1);

    Matrix<T> norm(3, 1, 0);
    norm[2] = -(this->data()[5] - this->data()[4]);

    norm = vecNormalize(norm);

    Matrix<T> origin(3, 1);
    origin[0] = this->data()[0];
    origin[1] = this->data()[2];
    origin[2] = this->data()[5];

    plane[0] = norm[0];
    plane[1] = norm[1];
    plane[2] = norm[2];
    plane[3] = -vecDot(norm, origin);

    return plane;
}

template <class T>
Matrix<T> BoxMatrix<T>::planeXZ0()
{
    Matrix<T> plane(4, 1);

    Matrix<T> norm(3, 1, 0);
    norm[1] = (this->data()[3] - this->data()[2]);

    norm = vecNormalize(norm);

    Matrix<T> origin(3, 1);
    origin[0] = this->data()[0];
    origin[1] = this->data()[2];
    origin[2] = this->data()[4];

    plane[0] = norm[0];
    plane[1] = norm[1];
    plane[2] = norm[2];
    plane[3] = -vecDot(norm, origin);

    return plane;
}

template <class T>
Matrix<T> BoxMatrix<T>::planeXZ1()
{
    Matrix<T> plane(4, 1);

    Matrix<T> norm(3, 1, 0);
    norm[1] = -(this->data()[3] - this->data()[2]);

    norm = vecNormalize(norm);

    Matrix<T> origin(3, 1);
    origin[0] = this->data()[0];
    origin[1] = this->data()[3];
    origin[2] = this->data()[4];

    plane[0] = norm[0];
    plane[1] = norm[1];
    plane[2] = norm[2];
    plane[3] = -vecDot(norm, origin);

    return plane;
}

template <class T>
Matrix<T> BoxMatrix<T>::planeYZ0()
{
    Matrix<T> plane(4, 1);

    Matrix<T> norm(3, 1, 0);
    norm[0] = (this->data()[1] - this->data()[0]);

    norm = vecNormalize(norm);

    Matrix<T> origin(3, 1);
    origin[0] = this->data()[0];
    origin[1] = this->data()[2];
    origin[2] = this->data()[4];

    plane[0] = norm[0];
    plane[1] = norm[1];
    plane[2] = norm[2];
    plane[3] = -vecDot(norm, origin);

    return plane;
}

template <class T>
Matrix<T> BoxMatrix<T>::planeYZ1()
{
    Matrix<T> plane(4, 1);

    Matrix<T> norm(3, 1, 0);
    norm[0] = -(this->data()[1] - this->data()[0]);

    norm = vecNormalize(norm);

    Matrix<T> origin(3, 1);
    origin[0] = this->data()[1];
    origin[1] = this->data()[2];
    origin[2] = this->data()[4];

    plane[0] = norm[0];
    plane[1] = norm[1];
    plane[2] = norm[2];
    plane[3] = -vecDot(norm, origin);

    return plane;
}

template <class T>
BoxMatrix<T> &BoxMatrix<T>::operator = (Matrix<T> other)
{
    this->swap(*this, other);
    return * this;
}

template <class T>
BoxMatrix<T> &BoxMatrix<T>::operator = (BoxMatrix<T> other)
{
    this->swap(*this, other);
    return * this;
}

#endif // BOX_H

