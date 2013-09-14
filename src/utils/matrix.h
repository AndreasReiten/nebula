#ifndef MATRIX_H
#define MATRIX_H

#include <cassert>
#include <iostream>
#include <cmath>
#include <iomanip>
#include <limits>
#include <cstring>
const double pi = 4.0*std::atan(1.0);

template <class T>
class Matrix {
    public:
        Matrix();
        Matrix(size_t m, size_t n, T value);
        Matrix(size_t m, size_t n);
        Matrix(const Matrix & other);
        Matrix(Matrix && other);
        ~Matrix();

        const Matrix<T> getInverse() const;
        const Matrix<T> getColMajor() const;
        Matrix<float> toFloat() const;

        const Matrix operator * (const Matrix&) const;
        const Matrix operator * (const T&) const;
        const Matrix operator - (const Matrix&) const;
        const Matrix operator - (const T&) const;
        const Matrix operator + (const Matrix&) const;
        const Matrix operator + (const T&) const;

        T& operator[] (const size_t index);
        const T& operator[] (const size_t index) const;
        Matrix& operator = (Matrix other);

        void setIdentity(size_t n);
        void set(size_t m, size_t n, T value);
        void setShallow(size_t m, size_t n, T * buffer);
        void setDeep(size_t m, size_t n, T * buffer);
        void reserve(size_t m, size_t n);
        void clear();

        T * data() const;
        T * data();
        T at(size_t index);

        size_t getM() const;
        size_t getN() const;
        size_t size() const;
        size_t bytes() const;

        void print(int precision = 0, const char * id = "") const;


    protected:
        size_t m;
        size_t n;
        T * buffer;

        /* Swap function as per C++11 idiom */
        void swap(Matrix& first, Matrix& second);
};

template<class T>
Matrix<T> operator* (T factor, const Matrix<T> &M)
{
    return M * factor;
}


template <class T>
Matrix<T>::Matrix(size_t m, size_t n, T value)
{
    this->m = 0;
    this->n = 0;
    this->set(m, n, value);
}
template <class T>

Matrix<T>::Matrix(size_t m, size_t n)
{
    this->m = 0;
    this->n = 0;
    this->reserve(m, n);
}

template <class T>
Matrix<T>::Matrix()
{
    this->m = 0;
    this->n = 0;
    this->buffer = NULL;
}

template <class T>
Matrix<T>::Matrix(const Matrix & other)
{
    this->m = other.getM();
    this->n = other.getN();
    this->buffer = new T[m*n];
    for (size_t i = 0; i < m*n; i++)
    {
        this->buffer[i] = other[i];
    }
}

template <class T>
Matrix<T>::Matrix(Matrix && other)
{
    swap(*this, other);
}


template <class T>
Matrix<T>::~Matrix()
{
    if (m*n > 0)
    {
        delete[] this->buffer;
    }
}

template <class T>
void Matrix<T>::swap(Matrix& first, Matrix& second)
{
    std::swap(first.m, second.m);
    std::swap(first.n, second.n);
    std::swap(first.buffer, second.buffer);
}

template <class T>
Matrix<float> Matrix<T>::toFloat() const
{
    Matrix<float> buf(this->m, this->n);

    for (size_t i = 0; i < this->m*this->n; i++)
    {
        buf[i] = (float) this->buffer[i];
    }

    return buf;
}

template <class T>
void Matrix<T>::setIdentity(size_t n)
{
    this->set(n, n, 0);
    for (size_t i = 0, j = 0; i < n; i++, j++)
    {
        buffer[i*n+j] = 1;
    }
}

template <class T>
size_t Matrix<T>::bytes() const
{
    return m*n*sizeof(T);
}

template <class T>
const Matrix<T> Matrix<T>::getColMajor()  const
{
    Matrix<T> ColMajor;
    ColMajor.reserve(this->getN(), this->getM());
    size_t count = 0;

    for (size_t i = 0; i < this->getM(); i++)
    {
        for (size_t j = 0; j < this->getN(); j++)
        {
            ColMajor[i*this->getN()+j] = this->data()[(count%this->getM())*this->getN() + (count/this->getM())%this->getN()];
            count++;
        }
    }

    return ColMajor;
}

template <class T>
const Matrix<T> Matrix<T>::getInverse()  const
{
    if(m != n) std::cout << "Matrix is can not be inverted: m (= " << m  << ") != n (=" << n << ")" << std::endl;
    Matrix<double> L;
    L.reserve(m, n);

    Matrix<double> U;
    U.reserve(m, n);

    /* LU Decomposition */
    {
        size_t i, j, k;
        double sum = 0;

        for (i = 0; i < n; i++)
        {
                U[i*n+i] = 1;
        }

        for (j = 0; j < n; j++)
        {
            for(i = j; i < n; i++)
            {
                sum = 0;
                for(k = 0; k < j; k++)
                {
                        sum += L[i*n+k] * U[k*n+j];
                }
                L[i*n+j] = (double) buffer[i*n+j] - sum;
            }

            for(i = j; i < n; i++){
                sum = 0;
                for(k = 0; k < j; k++)
                {
                    sum +=  L[j*n+k]*U[k*n+i];
                }
                if(L[j*n+j] == 0)
                {
                    std::cout << "det(L) close to 0!\n Can't divide by 0...\n" << std::endl;
                }
                U[j*n+i]=(double)(buffer[j*n+i]-sum)/(double)L[j*n+j];
            }
        }
    }


    /* Forward-backward substitution */
    Matrix<double> I;
    I.setIdentity(n);

    /* Compute Y = UX in LY = I */
    Matrix<double> Y;
    Y.reserve(L.getN(), I.getN());

    // For each column
    for (size_t j = 0; j < I.getN(); j++)
    {
        // Do forward substitution
        Y[j] = I[j] / L[0];

        for (size_t i = 1; i < L.getN(); i++)
        {
            double sum = 0;

            for (size_t k = 0; k <= i - 1; k++)
            {
                sum += L[i*L.getN()+k] * Y[k*Y.getN()+j];
            }
            Y[i*Y.getN()+j] = (I[i*I.getN()+j] - sum) / L[i*L.getN()+i];
        }
    }

    /* Compute X in UX = Y */
    Matrix<T> X;
    X.reserve(U.getN(), Y.getN());

    // For each column
    for (size_t j = 0; j < Y.getN(); j++)
    {
        // Do backward substitution
        X[(X.getN()-1)*X.getN()+j] = Y[(Y.getN()-1)*Y.getN()+j] / U[(X.getN()-1)*X.getN()+(X.getN()-1)];

        for (size_t i = U.getN() - 1; i-- != 0;)
        {
            double sum = 0;

            for (size_t k = i+1; k < U.getN(); k++)
            {
                sum += U[i*U.getN()+k] * X[k*X.getN()+j];
            }
            X[i*X.getN()+j] = (Y[i*Y.getN()+j] - sum) / U[i*U.getN()+i];
        }
    }

    return X;
}

template <class T>
const Matrix<T> Matrix<T>::operator + (const Matrix& M) const
{
    Matrix<T> c(*this);
    //~ c.copy(this->m, this->n, this->data());

    if ((this->n != M.getN()) || (this->m != M.getM()))
    {
        std::cout << "Warning: Matrix dimesions do not agree!" << std::endl;
        return c;
    }

    for (size_t i = 0; i < c.getM(); i++)
    {
        for (size_t j = 0; j < c.getN(); j++)
        {
            c[i*c.getN() + j] += M[i*c.getN() + j];
        }
    }

    return c;
}


template <class T>
const Matrix<T> Matrix<T>::operator + (const T& value) const
{
    Matrix<T> c(*this);
    //~ c.copy(this->m, this->n, this->data());

    for (size_t i = 0; i < c.getM(); i++)
    {
        for (size_t j = 0; j < c.getN(); j++)
        {
            c[i*c.getN() + j] += value;
        }
    }

    return c;
}


template <class T>
const Matrix<T> Matrix<T>::operator - (const Matrix& M) const
{
    Matrix<T> c(*this);
    //~ c.copy(this->m, this->n, this->data());

    if ((this->n != M.getN()) || (this->m != M.getM()))
    {
        std::cout << "Warning: Matrix dimesions do not agree!" << std::endl;
        return c;
    }

    for (size_t i = 0; i < c.getM(); i++)
    {
        for (size_t j = 0; j < c.getN(); j++)
        {
            c[i*c.getN() + j] -= M[i*c.getN() + j];
        }
    }

    return c;
}


template <class T>
const Matrix<T> Matrix<T>::operator - (const T& value) const
{
    Matrix<T> c(*this);
    //~ c.copy(this->m, this->n, this->data());

    for (size_t i = 0; i < c.getM(); i++)
    {
        for (size_t j = 0; j < c.getN(); j++)
        {
            c[i*c.getN() + j] -= value;
        }
    }

    return c;
}

template <class T>
const Matrix<T> Matrix<T>::operator * (const T& factor) const
{
    Matrix<T> c(*this);
    for (size_t i = 0; i < c.getM(); i++)
    {
        for (size_t j = 0; j < c.getN(); j++)
        {
            c[i*c.getN() + j] = c[i*c.getN() + j] * factor;
        }
    }
    return c;
}


template <class T>
const Matrix<T> Matrix<T>::operator * (const Matrix& M) const
{
    Matrix<T> c;

    if (this->n != M.getM())
    {
        std::cout << "Warning: Matrix dimesions do not agree!" << std::endl;
        return c;
    }

    c.set(this->m, M.getN(), 0.0f);

    for (size_t i = 0; i < c.getM(); i++)
    {
        for (size_t j = 0; j < c.getN(); j++)
        {
            for (size_t k = 0; k < this->n; k++)
            {
                c[i*c.getN()+j] += this->buffer[i*this->n+k] * M[k*c.getN()+j];
            }
        }
    }

    return c;
}

template <class T>
void Matrix<T>::print(int precision, const char * id) const
{
    if (strlen(id) > 0) std::cout << id << "("<< this->m << ", " << this->n << "):"<<std::endl;
    for (int i = 0; i < m; i++)
    {
        if (i == 0) std::cout << "{{ ";
        else std::cout << " { ";

        for (int j = 0; j < n; j++)
        {
            if (this->buffer[i*n+j] >= 0) std::cout << " "<< std::setprecision(precision) << std::fixed << (double) this->buffer[i*n+j];
            else std::cout << std::setprecision(precision) << std::fixed << (double) this->buffer[i*n+j];
            if (j != n-1) std::cout << ", ";
        }

        if (i == m-1) std::cout << " }}" << std::endl;
        else std::cout << " }," << std::endl;
    }
}

//~ template <class T>
//~ void Matrix<T>::copy(size_t m, size_t n, T * buffer)
//~ {
    //~ this->clear();
    //~ this->m = m;
    //~ this->n = n;
    //~ this->buffer = new T[m*n];
    //~ for (size_t i = 0; i < m*n; i++)
    //~ {
        //~ this->buffer[i] = buffer[i];
    //~ }
//~ }


template <class T>
Matrix<T>& Matrix<T>::operator = (Matrix other)
{
    swap(*this, other);

    return * this;
}


template <class T>
void Matrix<T>::set(size_t m, size_t n, T value)
{
    this->clear();
    this->m = m;
    this->n = n;
    this->buffer = new T[m*n];
    for (size_t i = 0; i < m*n; i++)
    {
        this->buffer[i] = value;
    }
}

template <class T>
void Matrix<T>::setShallow(size_t m, size_t n, T * buffer)
{
    this->clear();
    this->m = m;
    this->n = n;
    this->buffer = buffer;
}

template <class T>
void Matrix<T>::setDeep(size_t m, size_t n, T * buffer)
{
    this->clear();
    this->m = m;
    this->n = n;
    this->buffer = new T[m*n];
    for (size_t i = 0; i < m*n; i++)
    {
        this->buffer[i] = buffer[i];
    }
}

template <class T>
void Matrix<T>::reserve(size_t m, size_t n)
{
    this->clear();
    this->m = m;
    this->n = n;
    this->buffer = new T[m*n];
}

template <class T>
T& Matrix<T>::operator[] (const size_t index)
{
    assert(index < m*n);

    return buffer[index];
}

template <class T>
const T& Matrix<T>::operator[] (const size_t index) const
{
    assert(index < m*n);

    return buffer[index];
}

template <class T>
void Matrix<T>::clear()
{
    if (m*n > 0)
    {
        delete[] this->buffer;
        this->m = 0;
        this->n = 0;
    }
}

template <class T>
T * Matrix<T>::data()
{
    return buffer;
}

template <class T>
T * Matrix<T>::data() const
{
    return buffer;
}

template <class T>
T Matrix<T>::at(size_t index)
{
    assert(index < m*n);

    return buffer[index];
}

template <class T>
size_t Matrix<T>::getM() const
{
    return this->m;
}

template <class T>
size_t Matrix<T>::getN() const
{
    return this->n;
}

template <class T>
size_t Matrix<T>::size() const
{
    return m*n;
}

/* CameraToClipMatrix */

template <class T>
class CameraToClipMatrix : public Matrix<T>{
    public:
        using Matrix<T>::Matrix;
        CameraToClipMatrix();
        ~CameraToClipMatrix();

        CameraToClipMatrix& operator = (const CameraToClipMatrix &);
        CameraToClipMatrix& operator = (const Matrix<T> &);

        void setN(double N);
        void setF(double F);
        void setFov(double fov);
        void setWindow(size_t w, size_t h);
        void setProjection(bool value);

        private:
            double N, F, fov;
            size_t w, h;
            bool projection;
};

template <class T>
CameraToClipMatrix<T>::CameraToClipMatrix()
{
    this->set(4,4,0);
    this->w = 1.0;
    this->projection = true;
    this->N = 1.0;
    this->F = 7.0;
    this->fov = 10.0;
}

template <class T>
CameraToClipMatrix<T>::~CameraToClipMatrix()
{
    this->clear();
}

template <class T>
CameraToClipMatrix<T>& CameraToClipMatrix<T>::operator = (const CameraToClipMatrix & other)
{
    this->m = other.getM();
    this->n = other.getN();
    T * local_buffer = new T[this->m*this->n];
    for (size_t i = 0; i < this->m*this->n; i++)
    {
        local_buffer[i] = other[i];
    }
    delete[] this->buffer;
    this->buffer = local_buffer;
    return * this;
}

template <class T>
CameraToClipMatrix<T>& CameraToClipMatrix<T>::operator = (const Matrix<T> & other)
{
    this->m = other.getM();
    this->n = other.getN();
    T * local_buffer = new T[this->m*this->n];
    for (size_t i = 0; i < this->m*this->n; i++)
    {
        local_buffer[i] = other[i];
    }
    delete[] this->buffer;
    this->buffer = local_buffer;
    return * this;
}

template <class T>
void CameraToClipMatrix<T>::setN(double N)
{
    this->N = N;
    this->setProjection(this->projection);
}

template <class T>
void CameraToClipMatrix<T>::setF(double F)
{
    this->F = F;
    this->setProjection(this->projection);
}

template <class T>
void CameraToClipMatrix<T>::setFov(double fov)
{
    this->fov = fov;
    this->setProjection(this->projection);
}

template <class T>
void CameraToClipMatrix<T>::setWindow(size_t w, size_t h)
{
    this->w = w;
    this->h = h;
    this->setProjection(this->projection);
}

template <class T>
void CameraToClipMatrix<T>::setProjection(bool value)
{
    this->projection = value;

    if (this->projection == true)
    {
        this->data()[0] = 1.0 / std::tan(0.5*fov * pi / 180.0) * (double) h / (double) w;
        this->data()[5] = 1.0 / std::tan(0.5*fov * pi / 180.0);
        this->data()[10] = (F + N)/(N - F);
        this->data()[11] = 2.0*F*N/(N - F);
        this->data()[14] = -1.0;
        this->data()[15] = 0.0;
    }
    else
    {
        this->data()[0] = (double) h / (double) w;
        this->data()[5] = 1.0;
        this->data()[10] = 2.0/(N - F);
        this->data()[11] = (F + N)/(N - F);
        this->data()[14] = 0.0;
        this->data()[15] = 1.0;
    }
}


/* TsfMatrix */
template <class T>
class TsfMatrix : public Matrix<T>{
    public:
        using Matrix<T>::Matrix;
        TsfMatrix();
        ~TsfMatrix();

        TsfMatrix<T>& operator = (TsfMatrix other);

        Matrix<T> getSpline();
        Matrix<T> getPreIntegrated();
        void setSpline(size_t resolution);
        void setPreIntegrated();

    private:
        Matrix<T> splinedTsf;
        Matrix<T> preIntegratedTsf;
};

template <class T>
TsfMatrix<T>::TsfMatrix()
{
    this->setIdentity(4);
}

template <class T>
TsfMatrix<T>::~TsfMatrix()
{
    this->clear();
}

template <class T>
TsfMatrix<T>& TsfMatrix<T>::operator = (TsfMatrix other)
{
    swap(*this, other);

    return * this;
}


template <class T>
Matrix<T> TsfMatrix<T>::getSpline()
{
    return splinedTsf;
}
template <class T>
Matrix<T> TsfMatrix<T>::getPreIntegrated()
{
    return preIntegratedTsf;
}
template <class T>
void TsfMatrix<T>::setSpline(size_t resolution)
{
    // Spline interpolation for each row oversampled at resolution > n''

    // Calculate the second derivative for the function in all points
    Matrix<double> secondDerivatives(this->m, this->n);
    double stepLength = 1.0/((float) (this->n - 1));

    for (size_t i = 0; i < this->m; i++)
    {
        Matrix<double> A(this->n, this->n, 0.0);
        Matrix<double> X(this->n, 1, 0.0);
        Matrix<double> B(this->n, 1, 0.0);

        // Set the boundary conditions
        A[0] = 1.0;
        A[(this->n)*(this->n)-1] = 1.0;
        B[0] = 0.0;
        B[this->n-1] = 0.0;
        for (size_t j = 1; j < this->n - 1; j++)
        {
            double x_prev = (j - 1) * stepLength;
            double x = j * stepLength;
            double x_next = (j + 1) * stepLength;

            double f_prev = this->buffer[i*this->n+j-1];
            double f = this->buffer[i*this->n+j];
            double f_next = this->buffer[i*this->n+j+1];

            B[j] = ((f_next - f)/(x_next - x) - (f - f_prev)/(x - x_prev));

            A[j*this->n+j-1] = (x - x_prev) / 6.0;
            A[j*this->n+j] = (x_next - x_prev) / 3.0;
            A[j*this->n+j+1] = (x_next - x) / 6.0;
        }

        X = A.getInverse()*B;

        for (size_t j = 0; j < this->n; j++)
        {
            secondDerivatives[i*this->n+j] = X[j];
        }

    }
    //~ secondDerivatives.print(2,"Second derivatives");

    this->splinedTsf.reserve(this->m, resolution);
    double interpolationStepLength = 1.0/((float) (resolution - 1));

    for (size_t i = 0; i < this->m; i++)
    {
        for (size_t j = 0; j < resolution; j++)
        {

            double x = j * interpolationStepLength;
            size_t k = ((float)(this->n - 1) * (float)j / ((float)(resolution-1)));
            if ( k >= this->n - 1) k = this->n - 2;

            double x_k = (k) * stepLength;
            double x_k_next = (k + 1) * stepLength;

            double f_k = this->buffer[i*this->n+k];
            double f_k_next = this->buffer[i*this->n+k+1];

            double f_dd_k = secondDerivatives[i*this->n+k];
            double f_dd_k_next = secondDerivatives[i*this->n+k+1];

            double a = (x_k_next - x)/(x_k_next - x_k);
            double b = 1.0 - a;
            double c = (a*a*a - a)*(x_k_next - x)*(x_k_next - x)/6.0;
            double d = (b*b*b - b)*(x_k_next - x)*(x_k_next - x)/6.0;
            splinedTsf[i*resolution+j] = a*f_k + b*f_k_next + c*f_dd_k + d*f_dd_k_next;

            if (splinedTsf[i*resolution+j] < 0.0) splinedTsf[i*resolution+j] = 0.0;
            if (splinedTsf[i*resolution+j] > 1.0) splinedTsf[i*resolution+j] = 1.0;
        }
    }
}
template <class T>
void TsfMatrix<T>::setPreIntegrated()
{
    size_t resolution = splinedTsf.getN();

    preIntegratedTsf.set(splinedTsf.getM(), resolution, 0.0);

    double stepLength = 1.0/((float) (resolution - 1));

    preIntegratedTsf[0*resolution] = 0;
    preIntegratedTsf[1*resolution] = 0;
    preIntegratedTsf[2*resolution] = 0;
    preIntegratedTsf[3*resolution] = 0;

    for (size_t j = 1; j < resolution; j++)
    {
        double R = splinedTsf[0*resolution+j];
        double G = splinedTsf[1*resolution+j];
        double B = splinedTsf[2*resolution+j];
        double A = splinedTsf[3*resolution+j];

        double R_prev = splinedTsf[0*resolution+j-1];
        double G_prev = splinedTsf[1*resolution+j-1];
        double B_prev = splinedTsf[2*resolution+j-1];
        double A_prev = splinedTsf[3*resolution+j-1];

        preIntegratedTsf[0*resolution+j] = preIntegratedTsf[0*resolution+j-1] + stepLength*0.5*(R*A + R_prev*A_prev);
        preIntegratedTsf[1*resolution+j] = preIntegratedTsf[1*resolution+j-1] + stepLength*0.5*(G*A + G_prev*A_prev);
        preIntegratedTsf[2*resolution+j] = preIntegratedTsf[2*resolution+j-1] + stepLength*0.5*(B*A + B_prev*A_prev);
        preIntegratedTsf[3*resolution+j] = preIntegratedTsf[3*resolution+j-1] + stepLength*0.5*(A + A_prev);
    }
}


/* RotationMatrix */
template <class T>
class RotationMatrix : public Matrix<T>{
    public:
        using Matrix<T>::Matrix;
        RotationMatrix();
        ~RotationMatrix();


        RotationMatrix<T>& operator = (RotationMatrix other);
        //~ RotationMatrix& operator = (const RotationMatrix &);
        RotationMatrix& operator = (const Matrix<T> &);

        void setXRotation(double value);
        void setYRotation(double value);
        void setZRotation(double value);
        void setArbRotation(double zeta, double eta, double gamma);

        const RotationMatrix<T> getXRotation(double value);
        const RotationMatrix<T> getYRotation(double value);
        const RotationMatrix<T> getZRotation(double value);
        const RotationMatrix<T> getArbRotation(double zeta, double eta, double gamma);
};

template <class T>
RotationMatrix<T>::RotationMatrix()
{
    this->setIdentity(4);
}

template <class T>
RotationMatrix<T>::~RotationMatrix()
{
    this->clear();
}

template <class T>
RotationMatrix<T>& RotationMatrix<T>::operator = (RotationMatrix other)
{
    swap(*this, other);

    return * this;
}

//~ template <class T>
//~ RotationMatrix<T>& RotationMatrix<T>::operator = (const RotationMatrix & other)
//~ {
    //~ this->m = other.getM();
    //~ this->n = other.getN();
    //~ T * local_buffer = new T[this->m*this->n];
    //~ for (size_t i = 0; i < this->m*this->n; i++)
    //~ {
        //~ local_buffer[i] = other[i];
    //~ }
    //~ delete[] this->buffer;
    //~ this->buffer = local_buffer;
    //~ return * this;
//~ }

template <class T>
RotationMatrix<T>& RotationMatrix<T>::operator = (const Matrix<T> & other)
{
    this->m = other.getM();
    this->n = other.getN();
    T * local_buffer = new T[this->m*this->n];
    for (size_t i = 0; i < this->m*this->n; i++)
    {
        local_buffer[i] = other[i];
    }
    delete[] this->buffer;
    this->buffer = local_buffer;
    return * this;
}

template <class T>
void RotationMatrix<T>::setXRotation(double value)
{
    this->setIdentity(4);

    double s = std::sin(value);
    double c = std::cos(value);

    this->data()[5] = c;
    this->data()[6] = s;
    this->data()[9] = -s;
    this->data()[10] = c;
}

template <class T>
void RotationMatrix<T>::setYRotation(double value)
{
    this->setIdentity(4);

    double s = std::sin(value);
    double c = std::cos(value);

    this->data()[0] = c;
    this->data()[2] = -s;
    this->data()[8] = s;
    this->data()[10] = c;
}

template <class T>
void RotationMatrix<T>::setZRotation(double value)
{
    this->setIdentity(4);

    double s = std::sin(value);
    double c = std::cos(value);

    this->data()[0] = c;
    this->data()[1] = s;
    this->data()[4] = -s;
    this->data()[5] = c;
}

template <class T>
void RotationMatrix<T>::setArbRotation(double zeta, double eta, double gamma)
{
    /* Rotation around a axis whose tilts are given by zeta and eta */
    //~ this->setIdentity(4);

    RotationMatrix<T> RyPlus, RxPlus, RzGamma, RxMinus, RyMinus;

    RyPlus.setYRotation(zeta);
    RxPlus.setXRotation(eta);
    RzGamma.setZRotation(gamma);
    RxMinus.setXRotation(-eta);
    RyMinus.setYRotation(-zeta);

    (*this) = RyPlus * RxPlus * RzGamma * RxMinus * RyMinus;
}


template <class T>
const RotationMatrix<T> RotationMatrix<T>::getXRotation(double value)
{
    RotationMatrix m;

    double s = std::sin(value);
    double c = std::cos(value);

    m[5] = c;
    m[6] = s;
    m[9] = -s;
    m[10] = c;

    return m;
}

template <class T>
const RotationMatrix<T> RotationMatrix<T>::getYRotation(double value)
{
    RotationMatrix m;

    double s = std::sin(value);
    double c = std::cos(value);

    m[0] = c;
    m[2] = -s;
    m[8] = s;
    m[10] = c;

    return m;
}

template <class T>
const RotationMatrix<T> RotationMatrix<T>::getZRotation(double value)
{
    RotationMatrix m;

    double s = std::sin(value);
    double c = std::cos(value);

    m[0] = c;
    m[1] = s;
    m[4] = -s;
    m[5] = c;

    return m;
}

template <class T>
const RotationMatrix<T> RotationMatrix<T>::getArbRotation(double zeta, double eta, double gamma)
{
    /* Rotation around a axis whose tilts are given by zeta and eta */
    //~ this->setIdentity(4);

    RotationMatrix<T> RyPlus, RxPlus, RzGamma, RxMinus, RyMinus;

    RyPlus.setYRotation(zeta);
    RxPlus.setXRotation(eta);
    RzGamma.setZRotation(gamma);
    RxMinus.setXRotation(-eta);
    RyMinus.setYRotation(-zeta);

    return RyPlus * RxPlus * RzGamma * RxMinus * RyMinus;
}
#endif
