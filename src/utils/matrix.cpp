#include "matrix.h"

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
