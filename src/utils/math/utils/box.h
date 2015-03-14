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

        Matrix<T> planeXY(Matrix<T> transform);
        Matrix<T> planeXZ(Matrix<T> transform);
        Matrix<T> planeYZ(Matrix<T> transform);

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
Matrix<T> BoxMatrix<T>::planeXY(Matrix<T> transform)
{

}

template <class T>
Matrix<T> BoxMatrix<T>::planeXZ(Matrix<T> transform)
{

}

template <class T>
Matrix<T> BoxMatrix<T>::planeYZ(Matrix<T> transform)
{

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

