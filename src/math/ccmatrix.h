#ifndef CCMATRIX_H
#define CCMATRIX_H

#include "matrix.h"

/* CCMatrix */
template <class T>
class CCMatrix : public Matrix<T>
{
    public:
        CCMatrix();
        ~CCMatrix();

        CCMatrix<T> &operator = (Matrix<T> other);
        CCMatrix<T> &operator = (CCMatrix<T> other);

        void setN(double N);
        void setF(double F);
        void setFov(double fov);
        void setWindow(size_t w, size_t h);
        void setProjection(bool value);

    private:
        double N, F, fov;
        size_t w, h;
        int projection;
};

template <class T>
CCMatrix<T>::CCMatrix()
{
    this->set(4, 4, 0);
    this->w = 1.0;
    this->projection = true;
    this->N = 1.0;
    this->F = 7.0;
    this->fov = 10.0;
}

template <class T>
CCMatrix<T>::~CCMatrix()
{
    this->clear();
}

template <class T>
CCMatrix<T> &CCMatrix<T>::operator = (Matrix<T> other)
{
    this->swap(*this, other);

    return * this;
}

template <class T>
CCMatrix<T> &CCMatrix<T>::operator = (CCMatrix<T> other)
{
    this->swap(*this, other);

    return * this;
}

template <class T>
void CCMatrix<T>::setN(double N)
{
    this->N = N;
    this->setProjection(this->projection);
}

template <class T>
void CCMatrix<T>::setF(double F)
{
    this->F = F;
    this->setProjection(this->projection);
}

template <class T>
void CCMatrix<T>::setFov(double fov)
{
    this->fov = fov;
    this->setProjection(this->projection);
}

template <class T>
void CCMatrix<T>::setWindow(size_t w, size_t h)
{
    this->w = w;
    this->h = h;
    this->setProjection(this->projection);
}

template <class T>
void CCMatrix<T>::setProjection(bool value)
{
    this->projection = value;

    if (this->projection == 0)
    {
        // Perspective
        this->data()[0] = 1.0 / std::tan(0.5 * fov * pi / 180.0) * (double) h / (double) w;
        this->data()[5] = 1.0 / std::tan(0.5 * fov * pi / 180.0);
        this->data()[10] = (F + N) / (N - F);
        this->data()[11] = 2.0 * F * N / (N - F);
        this->data()[14] = -1.0;
        this->data()[15] = 0.0;
    }
    else
    {
        // Orthonormal
        this->data()[0] = (double) h / (double) w;
        this->data()[5] = 1.0;
        this->data()[10] = 2.0 / (N - F);
        this->data()[11] = (F + N) / (N - F);
        this->data()[14] = 0.0;
        this->data()[15] = 1.0;
    }
}


#endif // CCMATRIX_H
