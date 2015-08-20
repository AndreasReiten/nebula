#ifndef COLORMATRIX_H
#define COLORMATRIX_H

#include "matrix.h"

template <class T>
class ColorMatrix : public Matrix<T>
{
    public:
        ColorMatrix();
        ColorMatrix(T R, T G, T B, T A);
        ~ColorMatrix();

        ColorMatrix<T> &operator = (Matrix<T> other);
        ColorMatrix<T> &operator = (ColorMatrix<T> other);

        QColor toQColor();

        void set(T R, T G, T B, T A);
};

template <class T>
ColorMatrix<T>::ColorMatrix()
{
    this->reserve(1, 4);
}

template <class T>
ColorMatrix<T>::ColorMatrix(T R, T G, T B, T A)
{
    this->reserve(1, 4);
    this->data()[0] = R;
    this->data()[1] = G;
    this->data()[2] = B;
    this->data()[3] = A;
}

template <class T>
ColorMatrix<T>::~ColorMatrix()
{
    ;
}

template <class T>
ColorMatrix<T> &ColorMatrix<T>::operator = (Matrix<T> other)
{
    if (other.size() != this->size())
    {
        qWarning() << "Sizes do not match: " << other.size() << "!=" << this->size();
    }

    this->swap(*this, other);
    return * this;
}

template <class T>
ColorMatrix<T> &ColorMatrix<T>::operator = (ColorMatrix<T> other)
{
    this->swap(*this, other);
    return * this;
}

template <class T>
QColor ColorMatrix<T>::toQColor()
{
    QColor color;
    color.setRedF(this->at(0));
    color.setGreenF(this->at(1));
    color.setBlueF(this->at(2));
    color.setAlphaF(this->at(3));

    return color;
}

template <class T>
void ColorMatrix<T>::set(T R, T G, T B, T A)
{
    this->reserve(1, 4);
    this->data()[0] = R;
    this->data()[1] = G;
    this->data()[2] = B;
    this->data()[3] = A;
}

#endif // COLORMATRIX_H
