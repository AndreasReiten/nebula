#ifndef TRANSFERFUNCTION_H
#define TRANSFERFUNCTION_H

#include "../../math/qxmathlib.h"

class TransferFunction
{
    public:
        TransferFunction();

        Matrix<double> * getSplined();
        Matrix<double> * getPreIntegrated();
        Matrix<double> * getThumb();

        void setColorScheme(int color_style, int alpha_style);
        void setSpline(size_t resolution);
        void setPreIntegrated();

    private:
        Matrix<double> x_position;
        Matrix<double> tsf_base;
        Matrix<double> tsf_splined;
        Matrix<double> tsf_preintegrated;
        Matrix<double> tsf_thumb;
};

#endif // TRANSFERFUNCTION_H

