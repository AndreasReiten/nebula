#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QLabel>
#include <QImage>
#include <QScrollArea>

#include "../math/qxmathlib.h"

class PlotLineWidget : public QWidget
{
        Q_OBJECT
    public:
        explicit PlotLineWidget(QWidget * parent = 0);
        ~PlotLineWidget();

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
};

class PlotSurfaceWidget : public QScrollArea
{
        Q_OBJECT
    public:
        explicit PlotSurfaceWidget(QWidget * parent = 0);
        ~PlotSurfaceWidget();

    public slots:
        void plot(const Matrix<double> &data);
        void setLog(bool value);

    private:
        void resizeEvent(QResizeEvent * event);
        
        bool isLog;

        QLabel * p_label;
        Matrix<uchar> p_data;
};

#endif // PLOTWIDGET_H
