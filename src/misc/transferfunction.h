#ifndef TRANSFERFUNCTION_H
#define TRANSFERFUNCTION_H

#include "../math/matrix.h"

#include <QString>

class TransferFunction
{
    public:
        TransferFunction();

        Matrix<double> * getSplined();
        Matrix<double> * getPreIntegrated();
        Matrix<double> * getThumb();

        void setColorScheme(QString rgb, QString alpha);
        void setRgb(QString rgb);
        void setAlpha(QString alpha);
        void setSpline(size_t resolution);
        void setPreIntegrated();

    private:
        Matrix<double> p_rgb;
        Matrix<double> p_alpha;
        Matrix<double> p_x;
//        Matrix<double> rgba;
        Matrix<double> tsf_splined;
        Matrix<double> tsf_preintegrated;

        QString p_alpha_str;
        QString p_rgb_str;

        bool p_has_rgb;
        bool p_has_alpha;
//        QString uuuh;

};

#endif // TRANSFERFUNCTION_H

