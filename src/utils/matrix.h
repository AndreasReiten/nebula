#ifndef MATRIX_H
#define MATRIX_H

/*
 * A class supplying matrix functionality and with various subclasses
 * */

#include <cassert>
#include <sstream>
#include <iostream>
#include <cmath>
#include <iomanip>
#include <limits>
#include <cstring>
#include <QDebug>
#include <QColor>
#include <vector>

const double pi = 4.0*atan(1.0);

template <class T>
class Matrix {
    public:
        Matrix();
        Matrix(size_t m, size_t n, T value);
        Matrix(size_t m, size_t n);
        Matrix(const Matrix & other);
        Matrix(Matrix && other);
        ~Matrix();

        // Operators
        const Matrix operator * (const Matrix&) const;
        const Matrix operator * (const T&) const;
        const Matrix operator - (const Matrix&) const;
        const Matrix operator - (const T&) const;
        const Matrix operator + (const Matrix&) const;
        const Matrix operator + (const T&) const;
        
        T& operator[] (const size_t index);
        const T& operator[] (const size_t index) const;
        Matrix<T> &operator =(Matrix<T> other);
        

        // Utility
        Matrix<T> getInverse(int verbose = false) const;
        Matrix<T> getInverse4x4(int verbose = false) const;
        Matrix<T> getColMajor() const;
        Matrix<float> toFloat() const;

        T sum();
        QVector<T> toQVector() const;
        void setIdentity(size_t n);
        void set(size_t m, size_t n, T value);
        void setDeep(size_t m, size_t n, T * buffer);
        void reserve(size_t m, size_t n);
        void resize(size_t m, size_t n);
        void clear();

        const T *data() const;
        T * data();
        T at(size_t index);

        size_t getM() const;
        size_t getN() const;
        size_t size() const;
        size_t bytes() const;
        
        void print(int precision = 0, const char * id = "") const;
        
        // Vector math friends
        template <class F>
        friend F vecLength(const Matrix<F> A);
        
        template <class F>
        friend Matrix<F> vecNormalize(const Matrix<F> A);
        
        template <class F>
        friend Matrix<F> vecCross(const Matrix<F> A, const Matrix<F> B);
        
        template <class F>
        friend F vecDot(const Matrix<F> A, const Matrix<F> B);
        
        template <class F>
        friend F zeta(const Matrix<F> A);
        
        template <class F>
        friend F eta(const Matrix<F> A);
        
        // Other friends
        template <class F>
        friend Matrix<F> operator*(F factor, const Matrix<F> B);
        
        template <class F>
        friend std::ostream & operator << (std::ostream & stream, const Matrix<F> M);

    protected:
        size_t m;
        size_t n;
        std::vector<T> buffer;

        /* Swap function as per C++11 idiom */
        void swap(Matrix &first, Matrix &second);
};

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
}



template <class F>
Matrix<F> vecCross(const Matrix<F> A, const Matrix<F> B)
{
    Matrix<F> C(1,3,0);

    if ((A.getM()*A.getN() != 3) || (B.getM()*B.getN() != 3))
    {
        qWarning() << "Attempt to take cross product of vectors with dimensions" << A.getM() << "x" << A.getN() << "and" << B.getM() << "x" << B.getN();
    }
    else
    {
        C[0] = A[1]*B[2] - A[2]*B[1];
        C[1] = A[2]*B[0] - A[0]*B[2];
        C[2] = A[0]*B[1] - A[1]*B[0];
    }
    return C;
}

template <class F>
F vecDot(const Matrix<F> A, const Matrix<F> B)
{
    F value = 0;

    for (size_t i = 0; i < A.size(); i++)
    {
        value += A[i]*B[i];
    }

    return value;
}

template <class F>
Matrix<F> operator*(F factor, const Matrix<F> A)
{
    return A*factor;
}

template <class F>
F vecLength(const Matrix<F> A)
{
    F sum = 0;

    for (size_t i = 0; i < A.getM()*A.getN(); i++)
    {
        sum += A[i]*A[i];
    }

    return sqrt(sum);
}

template <class F>
F zeta(const Matrix<F> A)
{
    F x = A[0], z = A[2];
    return atan2(-x,z);
}

template <class F>
F eta(const Matrix<F> A)
{
    F x = A[0], y = A[1], z = A[2];
    return asin(y/sqrt(x*x + y*y + z*z));
}

template <class F>
Matrix<F> vecNormalize(const Matrix<F> A)
{
    return A*(1.0/vecLength(A));
}

template <class F>
std::ostream & operator << (std::ostream & stream, const Matrix<F> M)
{
    std::stringstream ss;
    ss << std::endl;

    ss << "("<< M.getM() << ", " << M.getN() << "):"<<std::endl;
    for (size_t i = 0; i < M.getM(); i++)
    {
        if (i == 0) ss << "{{ ";
        else ss << " { ";

        for (size_t j = 0; j < M.getN(); j++)
        {
            ss << std::setprecision(2) << std::fixed << M.data()[i*M.getN()+j];
            if (j != M.getN()-1) ss << ", ";
        }

        if (i == M.getM()-1) ss << " }}" << std::endl;
        else ss << " }," << std::endl;
    }
    stream << ss.str();

    return stream;
}

template <class T>
QVector<T> Matrix<T>::toQVector() const
{
    QVector<T> buf;
    buf.resize(this->size());

    for (size_t i = 0; i < this->size(); i++)
    {
        buf[i] = this->buffer[i];
    }

    return buf;
}

template <class T>
Matrix<T>::Matrix(const Matrix & other)
{
    this->m = other.getM();
    this->n = other.getN();
    this->buffer.resize(m*n);
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
        m = 0;
        n = 0;
    }
}

template <class T>
void Matrix<T>::resize(size_t m, size_t n)
{
    // This resize function retains any old values and fills voids with zeros
    Matrix<T> temp(m,n);
    
    for (size_t i = 0; i < m; i++)
    {
        for (size_t j = 0; j < n; j++)
        {
            if ((i < this->m) && (j < this->n))
            {
                temp[n*i+j] = buffer[n*i+j];    
            }
            else 
            {
                temp[n*i+j] = 0;
            }
        }    
    }
    
    *this = temp;
}


template <class T>
T Matrix<T>::sum()
{
    T sum = 0;
    for (size_t i = 0; i < n; i++)
    {
        for (size_t j = 0; j < m; j++)
        {
            sum += buffer[i*m + j];
        }
    }

    return sum;
}


template <class T>
void Matrix<T>::swap(Matrix &first, Matrix &second)
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
Matrix<T> Matrix<T>::getColMajor()  const
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
Matrix<T> Matrix<T>::getInverse4x4(int verbose)  const
{
    Q_UNUSED(verbose);

    if((m != 4) || (n != 4)) qWarning() << "Matrix is can not be inverted: m (= " << m  << ") != n (=" << n << ")";

    Matrix<T> INV(4,4);
    Matrix<T> M = this->getColMajor();

    T det;
    INV[0] = M[5]  * M[10] * M[15] -
             M[5]  * M[11] * M[14] -
             M[9]  * M[6]  * M[15] +
             M[9]  * M[7]  * M[14] +
             M[13] * M[6]  * M[11] -
             M[13] * M[7]  * M[10];

    INV[4] = -M[4]  * M[10] * M[15] +
              M[4]  * M[11] * M[14] +
              M[8]  * M[6]  * M[15] -
              M[8]  * M[7]  * M[14] -
              M[12] * M[6]  * M[11] +
              M[12] * M[7]  * M[10];

    INV[8] = M[4]  * M[9] * M[15] -
             M[4]  * M[11] * M[13] -
             M[8]  * M[5] * M[15] +
             M[8]  * M[7] * M[13] +
             M[12] * M[5] * M[11] -
             M[12] * M[7] * M[9];

    INV[12] = -M[4]  * M[9] * M[14] +
               M[4]  * M[10] * M[13] +
               M[8]  * M[5] * M[14] -
               M[8]  * M[6] * M[13] -
               M[12] * M[5] * M[10] +
               M[12] * M[6] * M[9];

    INV[1] = -M[1]  * M[10] * M[15] +
              M[1]  * M[11] * M[14] +
              M[9]  * M[2] * M[15] -
              M[9]  * M[3] * M[14] -
              M[13] * M[2] * M[11] +
              M[13] * M[3] * M[10];

    INV[5] = M[0]  * M[10] * M[15] -
             M[0]  * M[11] * M[14] -
             M[8]  * M[2] * M[15] +
             M[8]  * M[3] * M[14] +
             M[12] * M[2] * M[11] -
             M[12] * M[3] * M[10];

    INV[9] = -M[0]  * M[9] * M[15] +
              M[0]  * M[11] * M[13] +
              M[8]  * M[1] * M[15] -
              M[8]  * M[3] * M[13] -
              M[12] * M[1] * M[11] +
              M[12] * M[3] * M[9];

    INV[13] = M[0]  * M[9] * M[14] -
              M[0]  * M[10] * M[13] -
              M[8]  * M[1] * M[14] +
              M[8]  * M[2] * M[13] +
              M[12] * M[1] * M[10] -
              M[12] * M[2] * M[9];

    INV[2] = M[1]  * M[6] * M[15] -
             M[1]  * M[7] * M[14] -
             M[5]  * M[2] * M[15] +
             M[5]  * M[3] * M[14] +
             M[13] * M[2] * M[7] -
             M[13] * M[3] * M[6];

    INV[6] = -M[0]  * M[6] * M[15] +
              M[0]  * M[7] * M[14] +
              M[4]  * M[2] * M[15] -
              M[4]  * M[3] * M[14] -
              M[12] * M[2] * M[7] +
              M[12] * M[3] * M[6];

    INV[10] = M[0]  * M[5] * M[15] -
              M[0]  * M[7] * M[13] -
              M[4]  * M[1] * M[15] +
              M[4]  * M[3] * M[13] +
              M[12] * M[1] * M[7] -
              M[12] * M[3] * M[5];

    INV[14] = -M[0]  * M[5] * M[14] +
               M[0]  * M[6] * M[13] +
               M[4]  * M[1] * M[14] -
               M[4]  * M[2] * M[13] -
               M[12] * M[1] * M[6] +
               M[12] * M[2] * M[5];

    INV[3] = -M[1] * M[6] * M[11] +
              M[1] * M[7] * M[10] +
              M[5] * M[2] * M[11] -
              M[5] * M[3] * M[10] -
              M[9] * M[2] * M[7] +
              M[9] * M[3] * M[6];

    INV[7] = M[0] * M[6] * M[11] -
             M[0] * M[7] * M[10] -
             M[4] * M[2] * M[11] +
             M[4] * M[3] * M[10] +
             M[8] * M[2] * M[7] -
             M[8] * M[3] * M[6];

    INV[11] = -M[0] * M[5] * M[11] +
               M[0] * M[7] * M[9] +
               M[4] * M[1] * M[11] -
               M[4] * M[3] * M[9] -
               M[8] * M[1] * M[7] +
               M[8] * M[3] * M[5];

    INV[15] = M[0] * M[5] * M[10] -
              M[0] * M[6] * M[9] -
              M[4] * M[1] * M[10] +
              M[4] * M[2] * M[9] +
              M[8] * M[1] * M[6] -
              M[8] * M[2] * M[5];

    det = M[0] * INV[0] + M[1] * INV[4] + M[2] * INV[8] + M[3] * INV[12];

    if (det == 0) qWarning("Determinant is zero");

    det = 1.0 / det;

    INV = INV * det;

    return INV.getColMajor();
//        for (i = 0; i < 16; i++)
//            INVOut[i] = INV[i] * det;
}


template <class T>
Matrix<T> Matrix<T>::getInverse(int verbose)  const
{
//    qDebug() << "LU decomp" << m << n;

    if(m != n) qDebug() << "Matrix is can not be inverted: m (= " << m  << ") != n (=" << n << ")";
    Matrix<T> L, y, I, U, x;
    L.set(n, n, 0);
    y.set(n, n, 0);
    I.setIdentity(n);
    
    U.set(n, n, 0);
    x.set(n, n, 0);

    /* Ax = LUx = I method */
    
    /* LU Decomposition */
    int i, j, k;
    T sum;

    if (verbose)
    {
        this->print(3,"M");
    }

    for (i = 0; i < n; i++) {
        U[i*n+i] = 1;
    }

    for (j = 0; j < n; j++) {
        for(i = j; i < n; i++) {
            sum = 0;
            for(k = 0; k < j; k++) {
                sum += L[i*n+k] * U[k*n+j];
            }
            L[i*n+j] = buffer[i*n+j] - sum;
        }

        for(i = j; i < n; i++){
            sum = 0;
            for(k = 0; k < j; k++){
                sum +=  L[j*n+k]*U[k*n+i];
            }
            if(L[j*n+j] == 0) {
                qFatal("det(L) close to 0!\n Can't divide by 0...");
            }
            U[j*n+i] = (buffer[j*n+i]-sum)/L[j*n+j];
        }
    }

    if (verbose)
    {
        L.print(3,"L");
        U.print(3,"U");

        (L*U).print(3,"L*U");
    }

    /* Solve LY = I for Y (= UX) */
    for(i = 0; i < n; i++)
    {
        for(j = 0; j < n; j++)
        {
            T sum = 0;
            
            for (k = 0; k < n; k++)
            {
                if (k != i) sum += y[k*n+j] * L[i*n+k];
            }
            
            y[i*n+j] = (I[i*n+j] - sum)/L[i*n+i];
        }
    }
    
    if (verbose)
    {
        y.print(3,"Y");

        (L*y).print(3,"LY = I");
    }

    /* Solve UX = Y for X */
    for(i = n-1; i >= 0; --i)
    {
        for(j = n-1; j >= 0; --j)
        {
            T sum = 0;
            
            for (k = 0; k < n; k++)
            {
                if (k != i) sum += x[k*n+j] * U[i*n+k];
            }
            
            x[i*n+j] = (y[i*n+j] - sum)/U[i*n+i];
        }
    }

    if (verbose)
    {
        x.print(3,"X");

        (U*x).print(3,"UX = Y");

        (*this*x).print(3,"I");
    }
    return x;

}

template <class T>
const Matrix<T> Matrix<T>::operator + (const Matrix& M) const
{
    Matrix<T> c(*this);

    if ((this->n != M.getN()) || (this->m != M.getM()))
    {
        qWarning("Matrix dimesions do not agree!");
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

    if ((this->n != M.getN()) || (this->m != M.getM()))
    {
        qWarning("Matrix dimesions do not agree!");
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
        qWarning("Matrix dimesions do not agree!");
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
    std::stringstream ss;
    ss << std::endl;

    if (strlen(id) > 0) ss << id << "("<< this->m << ", " << this->n << "):"<<std::endl;
    for (size_t i = 0; i < m; i++)
    {
        if (i == 0) ss << " [ ";
        else ss << " [ ";

        for (size_t j = 0; j < n; j++)
        {
            ss << std::setprecision(precision) << std::fixed << this->buffer[i*n+j];
            if (j != n-1) ss << ", ";
        }

        if (i == m-1) ss << " ]" << std::endl;
        else ss << " ]" << std::endl;
    }

    qDebug() << ss.str().c_str();
}

template <class T>
Matrix<T>& Matrix<T>::operator = (Matrix<T> other)
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
    this->buffer.resize(m*n);
    for (size_t i = 0; i < m*n; i++)
    {
        this->buffer[i] = value;
    }
}

template <class T>
void Matrix<T>::setDeep(size_t m, size_t n, T * buffer)
{
    this->clear();
    this->m = m;
    this->n = n;
    this->buffer.resize(m*n);
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
    this->buffer.resize(m*n);
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
        // Since C++ vectors are optimized for speed, calling clear will not always free up the associated memory. It will be freed if the destructor is called, though. Here we swap the vector data over to a decoy which is destroyed when the function returns.
        std::vector<T> decoy;
        std::vector<T> (decoy).swap(buffer);
        this->buffer.clear();
        this->m = 0;
        this->n = 0;
    }
}

template <class T>
T * Matrix<T>::data()
{
    return this->buffer.data();
}

template <class T>
const T * Matrix<T>::data() const
{
    return this->buffer.data();
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











#endif
