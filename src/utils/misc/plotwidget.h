#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QWidget>
#include "../math/qxmathlib.h"

class PlotWidget : public QWidget
{
        Q_OBJECT
    public:
        explicit PlotWidget(QWidget * parent = 0);
        ~PlotWidget();

    signals:

    public slots:
        void plot(double xmin, double xmax, double ymin, double ymax, Matrix<double> &data);
        void setLog(bool value);

    protected:
        void paintEvent(QPaintEvent * event);

    private:
        bool isLog;

        double p_x_min;
        double p_x_max;
        double p_y_min;
        double p_y_max;

        Matrix<double> p_data;
};

#endif // PLOTWIDGET_H
