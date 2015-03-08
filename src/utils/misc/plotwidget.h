#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QWidget>
//#include <QTimer>
#include "../math/qxmathlib.h"

class PlotWidget : public QWidget
{
        Q_OBJECT
    public:
        explicit PlotWidget(QWidget * parent = 0);
        ~PlotWidget();

    signals:
        //        void paintRequest();

    public slots:
        void plot(double xmin, double xmax, double ymin, double ymax, const Matrix<double> &x_data, const Matrix<double> &y_data);
        void setLog(bool value);

    protected:
        void paintEvent(QPaintEvent * event);

    private:
        bool isLog;

        double p_x_min;
        double p_x_max;
        double p_y_min;
        double p_y_max;

        Matrix<double> p_x_data;
        Matrix<double> p_y_data;

        //        QTimer * paintTimer;
};

#endif // PLOTWIDGET_H
