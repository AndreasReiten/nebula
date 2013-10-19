#include "transferfunction.h"

TransferFunction::TransferFunction()
{
}



Matrix<double> *TransferFunction::getSplined()
{
    return &tsf_splined;
}


Matrix<double> *TransferFunction::getPreIntegrated()
{
    return &tsf_preintegrated;
}


void TransferFunction::setColorScheme(int color_style, int alpha_style)
{
    // The format is [x, r, g, b, a]
    // Todo: make each component independent

    double buf_hot[] = {
        0.00,  0.00,  0.00,  0.00,  0.00,
        1.00,  0.00,  0.00,  0.00,  1.00,
        2.00,  0.70,  0.00,  0.00,  1.00,
        3.00,  1.00,  0.00,  0.00,  1.00,
        4.00,  1.00,  0.50,  0.00,  1.00,
        5.00,  1.00,  1.00,  0.00,  1.00,
        6.00,  1.00,  1.00,  0.50,  1.00,
        7.00,  1.00,  1.00,  1.00,  1.00};
    Matrix<double> hot;
    hot.setDeep(8, 5, buf_hot);

    double buf_galaxy[] = {
        0.00,  0.00,  0.00,  1.00,  0.00,
        1.00,  0.00,  0.00,  1.00,  1.00,
        2.00,  1.00,  0.00,  1.00,  1.00,
        3.00,  1.00,  0.00,  0.00,  1.00,
        4.00,  1.00,  0.50,  0.00,  1.00,
        5.00,  1.00,  1.00,  0.00,  1.00,
        6.00,  1.00,  1.00,  0.50,  1.00,
        7.00,  1.00,  1.00,  1.00,  1.00};
    Matrix<double> galaxy;
    galaxy.setDeep(8, 5, buf_galaxy);

    double buf_hsv[] = {
        0.00,  1.00,  0.00,  0.00,  0.00,
        1.00,  1.00,  0.00,  0.00,  1.00,
        2.00,  1.00,  0.00,  1.00,  1.00,
        3.00,  0.00,  0.00,  1.00,  1.00,
        4.00,  0.00,  1.00,  1.00,  1.00,
        5.00,  0.00,  1.00,  0.00,  1.00,
        6.00,  1.00,  1.00,  0.00,  1.00,
        7.00,  1.00,  0.00,  0.00,  1.00};
    Matrix<double> hsv;
    hsv.setDeep(8, 5, buf_hsv);

    double buf_binary[] = {
        0.00,  0.00,  0.00,  0.00,  0.00,
        1.00,  0.14,  0.14,  0.14,  1.00,
        2.00,  0.29,  0.29,  0.29,  1.00,
        3.00,  0.43,  0.43,  0.43,  1.00,
        4.00,  0.57,  0.57,  0.57,  1.00,
        5.00,  0.71,  0.71,  0.71,  1.00,
        6.00,  0.86,  0.86,  0.86,  1.00,
        7.00,  1.00,  1.00,  1.00,  1.00};
    Matrix<double> binary;
    binary.setDeep(8, 5, buf_binary);

    double buf_yranib[] = {
        0.00,  1.00,  1.00,  1.00,  0.00,
        1.00,  0.86,  0.86,  0.86,  1.00,
        2.00,  0.71,  0.71,  0.71,  1.00,
        3.00,  0.57,  0.57,  0.57,  1.00,
        4.00,  0.43,  0.43,  0.43,  1.00,
        5.00,  0.29,  0.29,  0.29,  1.00,
        6.00,  0.14,  0.14,  0.14,  1.00,
        7.00,  0.00,  0.00,  0.00,  1.00};
    Matrix<double> yranib;
    yranib.setDeep(8, 5, buf_yranib);

    double buf_rainbow[] = {
        0.00,  1.00,  0.00,  0.00,  0.00,
        1.00,  1.00,  0.00,  0.00,  1.00,
        2.00,  1.00,  1.00,  0.00,  1.00,
        3.00,  0.00,  1.00,  0.00,  1.00,
        4.00,  0.00,  1.00,  1.00,  1.00,
        5.00,  0.00,  0.00,  1.00,  1.00,
        6.00,  1.00,  0.00,  1.00,  1.00};
    Matrix<double> rainbow;
    rainbow.setDeep(7, 5, buf_rainbow);

    Matrix<double> choice;
    switch (color_style)
    {
        case 0:
            choice = rainbow.getColMajor();
            break;
        case 1:
            choice = hot.getColMajor();
            break;
        case 2:
            choice = hsv.getColMajor();
            break;
        case 3:
            choice = galaxy.getColMajor();
            break;
        case 4:
            choice = binary.getColMajor();
            break;
        case 5:
            choice = yranib.getColMajor();
            break;
        default:
            choice = rainbow.getColMajor();
            break;
    }

    // Compute the alpha
    switch (alpha_style)
    {
        case 0:
            // Linearly increasing alpha
            for (size_t i = 0; i < choice.getN(); i++)
            {
                choice[i+choice.getN()*4] = choice[i] / choice[choice.getN()-1];
            }
            break;
        case 1:
            // Exponentially increasing data
            for (size_t i = 0; i < choice.getN(); i++)
            {
                choice[i+choice.getN()*4] = std::exp(-(1.0 - choice[i] / choice[choice.getN()-1])*3.0);
            }
            choice[choice.getN()*4] = 0;
            break;
        case 2:
            // Uniform alpha except for the first vertex
            for (size_t i = 0; i < choice.getN(); i++)
            {
                choice[i+choice.getN()*4] = 1.0;
            }
            choice[choice.getN()*4] = 0;
            break;
    }

    x_position.setDeep(1, choice.getN(), choice.data());
    tsf_base.setDeep(choice.getM()-1, choice.getN(), choice.data() + choice.getN());
    tsf_thumb.setDeep(choice.getM()-2, choice.getN(), choice.data() + choice.getN());
}

Matrix<double> *TransferFunction::getThumb()
{
    return &tsf_thumb;
}

void TransferFunction::setSpline(size_t resolution)
{
    /* Cubic spline interpolation for each row oversampled at resolution > n.
     * The goal of cubic spline interpolation is to get an interpolation formula
     * that is continuous in both the first and second derivatives, both within
     * the intervals and at the interpolating nodes. */

    // Calculate the second derivative for the function in all points
    Matrix<double> secondDerivatives(tsf_base.getM(), tsf_base.getN());
//    double stepLength = 1.0/((double) (tsf_base.getN() - 1));

    for (size_t i = 0; i < tsf_base.getM(); i++)
    {
        Matrix<double> A(tsf_base.getN(), tsf_base.getN(), 0.0);
        Matrix<double> X(tsf_base.getN(), 1, 0.0);
        Matrix<double> B(tsf_base.getN(), 1, 0.0);

        // Set the boundary conditions
        A[0] = 1.0;
        A[(tsf_base.getN())*(tsf_base.getN())-1] = 1.0;
        B[0] = 0.0;
        B[tsf_base.getN()-1] = 0.0;
        for (size_t j = 1; j < tsf_base.getN() - 1; j++)
        {
            double x_prev = x_position[j-1];//(j - 1) * stepLength;
            double x = x_position[j];//j * stepLength;
            double x_next = x_position[j+1];//(j + 1) * stepLength;

            double f_prev = tsf_base[i*tsf_base.getN()+j-1];
            double f = tsf_base[i*tsf_base.getN()+j];
            double f_next = tsf_base[i*tsf_base.getN()+j+1];

            B[j] = ((f_next - f)/(x_next - x) - (f - f_prev)/(x - x_prev));

            A[j*tsf_base.getN()+j-1] = (x - x_prev) / 6.0;
            A[j*tsf_base.getN()+j] = (x_next - x_prev) / 3.0;
            A[j*tsf_base.getN()+j+1] = (x_next - x) / 6.0;
        }

        X = A.getInverse()*B;

        for (size_t j = 0; j < tsf_base.getN(); j++)
        {
            secondDerivatives[i*tsf_base.getN()+j] = X[j];
        }

    }

    tsf_splined.reserve(tsf_base.getM(), resolution);
    double interpolationStepLength = (x_position[x_position.getN() -1] - x_position[0])/((double) (resolution - 1));

    // Calculate the interpolation values given the second derivatives
    for (size_t i = 0; i < tsf_base.getM(); i++)
    {
        for (size_t j = 0; j < resolution; j++)
        {
            // x is the position of the current interpolation point
            double x = x_position[0] + j * interpolationStepLength;

            // k is the index of the data point succeeding the interpoaltion point in x
            size_t k = 0;
            for (int l = 0; l < x_position.getN(); l++)
            {
                if (x <= x_position[l])
                {
                    k = l;
                    break;
                }
            }
            if ( k >= tsf_base.getN()) k = tsf_base.getN() - 1;
            if (k <= 0) k = 1;

            double x_k = x_position[k-1];//(k) * stepLength;
            double x_k_next = x_position[k];// (k + 1) * stepLength;

            double f_k = tsf_base[i*tsf_base.getN()+k-1];
            double f_k_next = tsf_base[i*tsf_base.getN()+k];

            double f_dd_k = secondDerivatives[i*tsf_base.getN()+k-1];
            double f_dd_k_next = secondDerivatives[i*tsf_base.getN()+k];

            double a = (x_k_next - x)/(x_k_next - x_k);
            double b = 1.0 - a;
            double c = (a*a*a - a)*(x_k_next - x)*(x_k_next - x)/6.0;
            double d = (b*b*b - b)*(x_k_next - x)*(x_k_next - x)/6.0;
            tsf_splined[i*resolution+j] = a*f_k + b*f_k_next + c*f_dd_k + d*f_dd_k_next;

            if (tsf_splined[i*resolution+j] < 0.0) tsf_splined[i*resolution+j] = 0.0;
            if (tsf_splined[i*resolution+j] > 1.0) tsf_splined[i*resolution+j] = 1.0;
        }
    }
}


void TransferFunction::setPreIntegrated()
{
    size_t resolution = tsf_splined.getN();

    tsf_preintegrated.set(tsf_splined.getM(), resolution, 0.0);

    double stepLength = 1.0/((double) (resolution - 1));

    tsf_preintegrated[0*resolution] = 0;
    tsf_preintegrated[1*resolution] = 0;
    tsf_preintegrated[2*resolution] = 0;
    tsf_preintegrated[3*resolution] = 0;

    for (size_t j = 1; j < resolution; j++)
    {
        double R = tsf_splined[0*resolution+j];
        double G = tsf_splined[1*resolution+j];
        double B = tsf_splined[2*resolution+j];
        double A = tsf_splined[3*resolution+j];

        double R_prev = tsf_splined[0*resolution+j-1];
        double G_prev = tsf_splined[1*resolution+j-1];
        double B_prev = tsf_splined[2*resolution+j-1];
        double A_prev = tsf_splined[3*resolution+j-1];

        tsf_preintegrated[0*resolution+j] = tsf_preintegrated[0*resolution+j-1] + stepLength*0.5*(R*A + R_prev*A_prev);
        tsf_preintegrated[1*resolution+j] = tsf_preintegrated[1*resolution+j-1] + stepLength*0.5*(G*A + G_prev*A_prev);
        tsf_preintegrated[2*resolution+j] = tsf_preintegrated[2*resolution+j-1] + stepLength*0.5*(B*A + B_prev*A_prev);
        tsf_preintegrated[3*resolution+j] = tsf_preintegrated[3*resolution+j-1] + stepLength*0.5*(A + A_prev);
    }
}
