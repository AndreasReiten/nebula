#ifndef TRANSFERFUNCTION_H
#define TRANSFERFUNCTION_H

#include "matrix.h"

class TransferFunction
{
public:
    TransferFunction();

    Matrix<double> getSplined();
    Matrix<double> getPreIntegrated();

    void setColorScheme(int color_style, int alpha_style);
    void setSpline(size_t resolution);
    void setPreIntegrated();

private:
    Matrix<double> x_position;
    Matrix<double> tsf_base;
    Matrix<double> tsf_splined;
    Matrix<double> tsf_preintegrated;
};

#endif // TRANSFERFUNCTION_H

