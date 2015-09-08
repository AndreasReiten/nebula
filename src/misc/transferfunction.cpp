#include "transferfunction.h"

TransferFunction::TransferFunction():
    p_has_rgb(false),
    p_has_alpha(false),
    p_rgb_str("Hot"),
    p_alpha_str("Opaque")
//    uuuh("Opaque")
{
}



Matrix<double> * TransferFunction::getSplined()
{
    return &tsf_splined;
}


Matrix<double> * TransferFunction::getPreIntegrated()
{
    return &tsf_preintegrated;
}

void TransferFunction::setAlpha(QString alpha)
{
    p_alpha_str = alpha;
    //    uuuh = alpha;
    // Compute the alpha
    p_alpha = p_x;

    if (alpha == "Linear")
    {
        // Linearly increasing alpha
        for (size_t i = 0; i < p_rgb.n(); i++)
        {
            p_alpha[i] = p_x[i] / p_x[p_x.n()-1];
        }
    }
    else if (alpha == "Exponential")
    {
        // Exponentially increasing data
        for (size_t i = 0; i < p_rgb.n(); i++)
        {
            p_alpha[i] = std::exp(-(1.0 - p_x[i] / p_x[p_x.n()-1]) * 3.0);
        }

        p_alpha[0] = 0;
    }
    else if (alpha == "Uniform")
    {
        // Uniform alpha except for the first vertex
        for (size_t i = 0; i < p_rgb.n(); i++)
        {
            p_alpha[i] = 1.0;
        }

        p_alpha[0] = 0;
    }
    else if (alpha == "Opaque")
    {
        // Opaque
        for (size_t i = 0; i < p_rgb.n(); i++)
        {
            p_alpha[i] = 1.0;
        }
    }
    else
    {
        // Opaque
        for (size_t i = 0; i < p_rgb.n(); i++)
        {
            p_alpha[i] = 1.0;
        }
        qDebug() << "Could not recognize alpha" << alpha;
    }

    p_has_alpha = true;
}

void TransferFunction::setRgb(QString rgb)
{
    p_rgb_str = rgb;
    // The format is [x, r, g, b, a]
    // Todo: make each component independent

    double buf_hot[] =
    {
        0.00,  0.00,  0.00,  0.00,
        1.00,  0.70,  0.00,  0.00,
        2.00,  1.00,  0.00,  0.00,
        3.00,  1.00,  0.50,  0.00,
        4.00,  1.00,  1.00,  0.00,
        5.00,  1.00,  1.00,  0.50,
        6.00,  1.00,  1.00,  1.00
    };
    Matrix<double> hot;
    hot.setDeep(7, 4, buf_hot);

    double buf_galaxy[] =
    {
        0.00,  0.00,  0.00,  1.00,
        1.00,  1.00,  0.00,  1.00,
        2.00,  1.00,  0.00,  0.00,
        3.00,  1.00,  0.50,  0.00,
        4.00,  1.00,  1.00,  0.00,
        5.00,  1.00,  1.00,  0.50,
        6.00,  1.00,  1.00,  1.00
    };
    Matrix<double> galaxy;
    galaxy.setDeep(7, 4, buf_galaxy);

    double buf_hsv[] =
    {
        0.00,  1.00,  0.00,  0.00,
        1.00,  1.00,  0.00,  1.00,
        2.00,  0.00,  0.00,  1.00,
        3.00,  0.00,  1.00,  1.00,
        4.00,  0.00,  1.00,  0.00,
        5.00,  1.00,  1.00,  0.00,
        6.00,  1.00,  0.00,  0.00
    };
    Matrix<double> hsv;
    hsv.setDeep(7, 4, buf_hsv);

    double buf_binary[] =
    {
        0.00,  0.00,  0.00,  0.00,
        1.00,  0.14,  0.14,  0.14,
        2.00,  0.29,  0.29,  0.29,
        3.00,  0.43,  0.43,  0.43,
        4.00,  0.57,  0.57,  0.57,
        5.00,  0.71,  0.71,  0.71,
        6.00,  0.86,  0.86,  0.86,
        7.00,  1.00,  1.00,  1.00
    };
    Matrix<double> binary;
    binary.setDeep(8, 4, buf_binary);

    double buf_yranib[] =
    {
        0.00,  1.00,  1.00,  1.00,
        1.00,  0.86,  0.86,  0.86,
        2.00,  0.71,  0.71,  0.71,
        3.00,  0.57,  0.57,  0.57,
        4.00,  0.43,  0.43,  0.43,
        5.00,  0.29,  0.29,  0.29,
        6.00,  0.14,  0.14,  0.14,
        7.00,  0.00,  0.00,  0.00
    };
    Matrix<double> yranib;
    yranib.setDeep(8, 4, buf_yranib);

    double buf_rainbow[] =
    {
        0.00,  1.00,  0.00,  0.00,
        1.00,  1.00,  1.00,  0.00,
        2.00,  0.00,  1.00,  0.00,
        3.00,  0.00,  1.00,  1.00,
        4.00,  0.00,  0.00,  1.00,
        5.00,  1.00,  0.00,  1.00,
    };
    Matrix<double> rainbow;
    rainbow.setDeep(6, 4, buf_rainbow);

    Matrix<double> choice;

    if (rgb == "Rainbow")
    {
        choice = rainbow.colmajor();
    }
    else if (rgb == "Hot")
    {
        choice = hot.colmajor();
    }
    else if (rgb == "Hsv")
    {
        choice = hsv.colmajor();
    }
    else if (rgb == "Galaxy")
    {
        choice = galaxy.colmajor();
    }
    else if (rgb == "Binary")
    {
        choice = binary.colmajor();
    }
    else if (rgb == "Yranib")
    {
        choice = yranib.colmajor();
    }
    else
    {
        choice = rainbow.colmajor();
        qDebug() << "Could not recognize rgb" << rgb;
    }

    p_x.setDeep(1, choice.n(), choice.data()); // The x position of rgb points
    p_rgb.setDeep(choice.m() - 1, choice.n(), choice.data() + choice.n()); // The rgb points

    p_has_rgb = true;

//    qDebug() << p_alpha_str;
    setAlpha(p_alpha_str);
}

void TransferFunction::setColorScheme(QString rgb, QString alpha)
{
    setRgb(rgb);
    setAlpha(alpha);
}

Matrix<double> * TransferFunction::getThumb()
{
    return &p_rgb;
}

void TransferFunction::setSpline(size_t resolution)
{
    /* Cubic spline interpolation for each row oversampled at resolution > n.
     * The goal of cubic spline interpolation is to get an interpolation formula
     * that is continuous in both the first and second derivatives, both within
     * the intervals and at the interpolating nodes. */

    // Calculate the second derivative for the function in all points
    if (!p_has_rgb) setRgb("Hot");
    if (!p_has_alpha) setAlpha("Opaque");

//    qDebug() << p_alpha_str;
//    qDebug() << p_rgb_str;

    Matrix<double> rgba(p_rgb);
    rgba.resize(4,p_rgb.n());

//    rgba.print();
//    p_rgb.print();
//    p_alpha.print();

    for (int i = 0; i < p_rgb.n(); i++)
    {
       rgba[p_rgb.size()+i] = p_alpha[i];
    }

//    rgba.print(2);

    Matrix<double> secondDerivatives(rgba.m(), rgba.n());

    for (size_t i = 0; i < rgba.m(); i++)
    {
        Matrix<double> A(rgba.n(), rgba.n(), 0.0);
        Matrix<double> X(rgba.n(), 1, 0.0);
        Matrix<double> B(rgba.n(), 1, 0.0);

        // Set the boundary conditions
        A[0] = 1.0;
        A[(rgba.n()) * (rgba.n()) - 1] = 1.0;
        B[0] = 0.0;
        B[rgba.n() - 1] = 0.0;

        for (size_t j = 1; j < rgba.n() - 1; j++)
        {
            double x_prev = p_x[j - 1];
            double x = p_x[j];
            double x_next = p_x[j + 1];

            double f_prev = rgba[i * rgba.n() + j - 1];
            double f = rgba[i * rgba.n() + j];
            double f_next = rgba[i * rgba.n() + j + 1];

            B[j] = ((f_next - f) / (x_next - x) - (f - f_prev) / (x - x_prev));

            A[j * rgba.n() + j - 1] = (x - x_prev) / 6.0;
            A[j * rgba.n() + j] = (x_next - x_prev) / 3.0;
            A[j * rgba.n() + j + 1] = (x_next - x) / 6.0;
        }

        X = A.inverse() * B;

        for (size_t j = 0; j < rgba.n(); j++)
        {
            secondDerivatives[i * rgba.n() + j] = X[j];
        }

    }

    tsf_splined.reserve(rgba.m(), resolution);
    double interpolationStepLength = (p_x[p_x.n() - 1] - p_x[0]) / ((double) (resolution - 1));

    // Calculate the interpolation values given the second derivatives
    for (size_t i = 0; i < rgba.m(); i++)
    {
        for (size_t j = 0; j < resolution; j++)
        {
            // x is the position of the current interpolation point
            double x = p_x[0] + j * interpolationStepLength;

            // k is the index of the data point succeeding the interpoaltion point in x
            size_t k = 0;

            for (size_t l = 0; l < p_x.n(); l++)
            {
                if (x <= p_x[l])
                {
                    k = l;
                    break;
                }
            }

            if ( k >= rgba.n())
            {
                k = rgba.n() - 1;
            }

            if (k <= 0)
            {
                k = 1;
            }

            double x_k = p_x[k - 1];
            double x_k_next = p_x[k];

            double f_k = rgba[i * rgba.n() + k - 1];
            double f_k_next = rgba[i * rgba.n() + k];

            double f_dd_k = secondDerivatives[i * rgba.n() + k - 1];
            double f_dd_k_next = secondDerivatives[i * rgba.n() + k];

            double a = (x_k_next - x) / (x_k_next - x_k);
            double b = 1.0 - a;
            double c = (a * a * a - a) * (x_k_next - x) * (x_k_next - x) / 6.0;
            double d = (b * b * b - b) * (x_k_next - x) * (x_k_next - x) / 6.0;
            tsf_splined[i * resolution + j] = a * f_k + b * f_k_next + c * f_dd_k + d * f_dd_k_next;

            if (tsf_splined[i * resolution + j] < 0.0)
            {
                tsf_splined[i * resolution + j] = 0.0;
            }

            if (tsf_splined[i * resolution + j] > 1.0)
            {
                tsf_splined[i * resolution + j] = 1.0;
            }
        }
    }
}


void TransferFunction::setPreIntegrated()
{
    size_t resolution = tsf_splined.n();

    tsf_preintegrated.set(tsf_splined.m(), resolution, 0.0);

    double step_length = 1.0 / ((double) (resolution - 1));

    tsf_preintegrated[0 * resolution] = 0;
    tsf_preintegrated[1 * resolution] = 0;
    tsf_preintegrated[2 * resolution] = 0;
    tsf_preintegrated[3 * resolution] = 0;

    for (size_t j = 1; j < resolution; j++)
    {
        double R = tsf_splined[0 * resolution + j];
        double G = tsf_splined[1 * resolution + j];
        double B = tsf_splined[2 * resolution + j];
        double A = tsf_splined[3 * resolution + j];

        double R_prev = tsf_splined[0 * resolution + j - 1];
        double G_prev = tsf_splined[1 * resolution + j - 1];
        double B_prev = tsf_splined[2 * resolution + j - 1];
        double A_prev = tsf_splined[3 * resolution + j - 1];

        tsf_preintegrated[0 * resolution + j] = tsf_preintegrated[0 * resolution + j - 1] + step_length * 0.5 * (R * A + R_prev * A_prev);
        tsf_preintegrated[1 * resolution + j] = tsf_preintegrated[1 * resolution + j - 1] + step_length * 0.5 * (G * A + G_prev * A_prev);
        tsf_preintegrated[2 * resolution + j] = tsf_preintegrated[2 * resolution + j - 1] + step_length * 0.5 * (B * A + B_prev * A_prev);
        tsf_preintegrated[3 * resolution + j] = tsf_preintegrated[3 * resolution + j - 1] + step_length * 0.5 * (A + A_prev);
    }
}
