#ifndef UBMATRIX_H
#define UBMATRIX_H

#include "matrix.h"
#include "rotationmatrix.h"

/* UB matrix */
template <class T>
class UBMatrix : public Matrix<T>
{
    public:
        UBMatrix();
        ~UBMatrix();

        UBMatrix<T> &operator = (Matrix<T> other);
        UBMatrix<T> &operator = (UBMatrix<T> other);

        void setBMatrix(Matrix<T> mat);
        void setUMatrix(RotationMatrix<T> mat);

        Matrix<T> getUMatrix();
        Matrix<T> getBMatrix();

        void setA(T value);
        void setB(T value);
        void setC(T value);
        void setAlpha(T value);
        void setBeta(T value);
        void setGamma(T value);

        void setAStar(T value);
        void setBStar(T value);
        void setCStar(T value);
        void setAlphaStar(T value);
        void setBetaStar(T value);
        void setGammaStar(T value);

        T V();

        T a();
        T b();
        T c();
        T alpha();
        T beta();
        T gamma();
        Matrix<T> aVec();
        Matrix<T> bVec();
        Matrix<T> cVec();

        T aStar();
        T bStar();
        T cStar();
        T alphaStar();
        T betaStar();
        T gammaStar();
        Matrix<T> aStarVec();
        Matrix<T> bStarVec();
        Matrix<T> cStarVec();

    private:
        RotationMatrix<T> U;
        Matrix<T> B;
};

template <class T>
UBMatrix<T>::UBMatrix()
{
    U.setIdentity(3);
    B.setIdentity(3);
    *this = U * B;

}

template <class T>
UBMatrix<T>::~UBMatrix()
{
    ;
}

template <class T>
UBMatrix<T> &UBMatrix<T>::operator = (Matrix<T> other)
{
    this->swap(other);
    return * this;
}

template <class T>
UBMatrix<T> &UBMatrix<T>::operator = (UBMatrix<T> other)
{
    this->swap(other);
    return * this;
}

template <class T>
void UBMatrix<T>::setBMatrix(Matrix<T> mat)
{
    B = mat;

    *this = getUMatrix() * B;
}
template <class T>
void UBMatrix<T>::setUMatrix(RotationMatrix<T> mat)
{
    U = mat;

    //    U.print(2,"U set");
    *this = U * getBMatrix();
}

template <class T>
Matrix<T> UBMatrix<T>::getUMatrix()
{
    U = (*this) * getBMatrix().inverse();

    return U;
}
template <class T>
Matrix<T> UBMatrix<T>::getBMatrix()
{
    T sa = sin(alpha());
    T ca = cos(alpha());
    T cb = cos(beta());
    T cg = cos(gamma());

    B.set(3, 3, 0);
    B[0] = b() * c() * sa / V();

    B[1] = a() * c() * (ca * cb - cg) / (V() * sa);
    B[4] = 1.0 / (b() * sa);

    B[2] = a() * b() * (ca * cg - cb) / (V() * sa);
    B[5] = -ca / (c() * sa);
    B[8] = 1.0 / c();

    return B;
}

template <class T>
void UBMatrix<T>::setA(T value)
{
    if (value > 0)
    {
        Matrix<T> a_star_tmp = vecNormalize(aStarVec()) * (1.0 / value);

        this->data()[0] = a_star_tmp[0];
        this->data()[3] = a_star_tmp[1];
        this->data()[6] = a_star_tmp[2];
    }
}
template <class T>
void UBMatrix<T>::setB(T value)
{
    if (value > 0)
    {
        Matrix<T> b_star_tmp = vecNormalize(bStarVec()) * (1.0 / value);

        this->data()[1] = b_star_tmp[0];
        this->data()[4] = b_star_tmp[1];
        this->data()[7] = b_star_tmp[2];
    }
}
template <class T>
void UBMatrix<T>::setC(T value)
{
    if (value > 0)
    {
        Matrix<T> c_star_tmp = vecNormalize(cStarVec()) * (1.0 / value);

        this->data()[2] = c_star_tmp[0];
        this->data()[5] = c_star_tmp[1];
        this->data()[8] = c_star_tmp[2];
    }
}

template <class T>
void UBMatrix<T>::setAlpha(T value)
{
    if ((value > 0) && (value > fabs(beta() - gamma())) && (value + beta() + gamma() < 2 * pi))
    {
        // Get all the current values including U
        // Change value in question to the given one
        // Calculate and set B
        Matrix<T> Utmp = getUMatrix();

        T sa = sin(value);
        T ca = cos(value);
        T cb = cos(beta());
        T cg = cos(gamma());

        double gammaStar_tmp = acos((cos(value) * cos(beta()) - cos(gamma())) / (sin(value) * sin(beta())));

        double V_tmp = a() * b() * c() * sin(value) * sin(beta()) * sin(gammaStar_tmp);

        B.set(3, 3, 0);
        B[0] = b() * c() * sa / V_tmp;

        B[1] = a() * c() * (ca * cb - cg) / (V_tmp * sa);
        B[4] = 1.0 / (b() * sa);

        B[2] = a() * b() * (ca * cg - cb) / (V_tmp * sa);
        B[5] = -ca / (c() * sa);
        B[8] = 1.0 / c();

        // Multiply it with U and set UB
        *this = Utmp * B;

    }
}
template <class T>
void UBMatrix<T>::setBeta(T value)
{
    if ((value > 0) && (value > fabs(alpha() - gamma())) && (value + alpha() + gamma() < 2 * pi))
    {
        Matrix<T> Utmp = getUMatrix();

        T sa = sin(alpha());
        T ca = cos(alpha());
        T cb = cos(value);
        T cg = cos(gamma());

        double gammaStar_tmp = acos((cos(alpha()) * cos(value) - cos(gamma())) / (sin(alpha()) * sin(value)));

        double V_tmp = a() * b() * c() * sin(alpha()) * sin(value) * sin(gammaStar_tmp);

        B.set(3, 3, 0);
        B[0] = b() * c() * sa / V_tmp;

        B[1] = a() * c() * (ca * cb - cg) / (V_tmp * sa);
        B[4] = 1.0 / (b() * sa);

        B[2] = a() * b() * (ca * cg - cb) / (V_tmp * sa);
        B[5] = -ca / (c() * sa);
        B[8] = 1.0 / c();

        *this = Utmp * B;
    }
}
template <class T>
void UBMatrix<T>::setGamma(T value)
{
    if ((value > 0) && (value > fabs(beta() - alpha())) && (value + beta() + alpha() < 2 * pi))
    {
        Matrix<T> Utmp = getUMatrix();

        T sa = sin(alpha());
        T ca = cos(alpha());
        T cb = cos(beta());
        T cg = cos(value);

        double betaStar_tmp = acos((cos(alpha()) * cos(value) - cos(beta())) / (sin(alpha()) * sin(value)));

        double V_tmp = a() * b() * c() * sin(alpha()) * sin(betaStar_tmp) * sin(value);

        B.set(3, 3, 0);
        B[0] = b() * c() * sa / V_tmp;

        B[1] = a() * c() * (ca * cb - cg) / (V_tmp * sa);
        B[4] = 1.0 / (b() * sa);

        B[2] = a() * b() * (ca * cg - cb) / (V_tmp * sa);
        B[5] = -ca / (c() * sa);
        B[8] = 1.0 / c();

        *this = Utmp * B;
    }
}

template <class T>
void UBMatrix<T>::setAStar(T value)
{
    if (value > 0)
    {
        Matrix<T> foo = vecNormalize(aStarVec()) * value;

        this->data()[0] = foo[0];
        this->data()[3] = foo[1];
        this->data()[6] = foo[2];
    }
}
template <class T>
void UBMatrix<T>::setBStar(T value)
{
    if (value > 0)
    {
        Matrix<T> foo = vecNormalize(bStarVec()) * value;

        this->data()[1] = foo[0];
        this->data()[4] = foo[1];
        this->data()[7] = foo[2];
    }
}
template <class T>
void UBMatrix<T>::setCStar(T value)
{
    if (value > 0)
    {
        Matrix<T> foo = vecNormalize(bStarVec()) * value;

        this->data()[2] = foo[0];
        this->data()[5] = foo[1];
        this->data()[8] = foo[2];
    }
}
template <class T>
void UBMatrix<T>::setAlphaStar(T value)
{

}
template <class T>
void UBMatrix<T>::setBetaStar(T value)
{

}
template <class T>
void UBMatrix<T>::setGammaStar(T value)
{

}

template <class T>
T UBMatrix<T>::a()
{
    return vecLength(aVec());
}

template <class T>
T UBMatrix<T>::b()
{
    return vecLength(bVec());
}
template <class T>
T UBMatrix<T>::c()
{
    return vecLength(cVec());
}

template <class T>
T UBMatrix<T>::V()
{
    return 1 / (vecDot(aStarVec(), vecCross(bStarVec(), cStarVec())));
}

template <class T>
T UBMatrix<T>::alpha()
{
    return acos(vecDot(bVec(), cVec()) / (vecLength(bVec()) * vecLength(cVec())));
}

template <class T>
T UBMatrix<T>::beta()
{
    return acos(vecDot(aVec(), cVec()) / (vecLength(aVec()) * vecLength(cVec())));
}
template <class T>
T UBMatrix<T>::gamma()
{
    return acos(vecDot(aVec(), bVec()) / (vecLength(aVec()) * vecLength(bVec())));
}

template <class T>
T UBMatrix<T>::aStar()
{
    return vecLength(aStarVec());
}
template <class T>
T UBMatrix<T>::bStar()
{
    return vecLength(bStarVec());
}
template <class T>
T UBMatrix<T>::cStar()
{
    return vecLength(cStarVec());
}

template <class T>
Matrix<T> UBMatrix<T>::aVec()
{
    return vecCross(bStarVec(), cStarVec()) * V();
}
template <class T>
Matrix<T> UBMatrix<T>::bVec()
{
    return vecCross(cStarVec(), aStarVec()) * V();
}
template <class T>
Matrix<T> UBMatrix<T>::cVec()
{
    return vecCross(aStarVec(), bStarVec()) * V();
}

template <class T>
Matrix<T> UBMatrix<T>::aStarVec()
{
    Matrix<T> vec(3, 1);

    vec[0] = this->at(0);
    vec[1] = this->at(3);
    vec[2] = this->at(6);

    return vec;
}
template <class T>
Matrix<T> UBMatrix<T>::bStarVec()
{
    Matrix<T> vec(3, 1);

    vec[0] = this->at(1);
    vec[1] = this->at(4);
    vec[2] = this->at(7);

    return vec;
}
template <class T>
Matrix<T> UBMatrix<T>::cStarVec()
{
    Matrix<T> vec(3, 1);

    vec[0] = this->at(2);
    vec[1] = this->at(5);
    vec[2] = this->at(8);

    return vec;
}

template <class T>
T UBMatrix<T>::alphaStar()
{
    return acos(vecDot(bStarVec(), cStarVec()) / (vecLength(bStarVec()) * vecLength(cStarVec())));
}
template <class T>
T UBMatrix<T>::betaStar()
{
    return acos(vecDot(aStarVec(), cStarVec()) / (vecLength(aStarVec()) * vecLength(cStarVec())));
}
template <class T>
T UBMatrix<T>::gammaStar()
{
    return acos(vecDot(aStarVec(), bStarVec()) / (vecLength(aStarVec()) * vecLength(bStarVec())));
}

#endif // UBMATRIX_H
