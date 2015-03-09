#include "plotwidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QLinearGradient>

PlotWidget::PlotWidget(QWidget * parent) :
    QWidget(parent),
    isLog(false)
{
    p_x_min = 0;
    p_x_max = 1;
    p_y_min = 0;
    p_y_max = 1.9;

    double buf_x[] = {0.0, 0.1, 0.2, 0.4, 0.7, 0.9};

    p_x_data.setDeep(1, 6, buf_x);

    double buf_y[] = { 0.0, 0.1, 0.2, 0.4, 1.2, 1.9};

    p_y_data.setDeep(1, 6, buf_y);

    //    paintTimer = new QTimer;

    //    connect(this, SIGNAL(paintRequest()), paintTimer, SLOT(start()));
}

PlotWidget::~PlotWidget()
{

}

void PlotWidget::plot(double xmin, double xmax, double ymin, double ymax, const Matrix<double> &x_data, const Matrix<double> &y_data)
{
    p_x_min = xmin;
    p_x_max = xmax;
    p_y_min = ymin;
    p_y_max = ymax;

    p_x_data = x_data;

    p_y_data = y_data;


    //    qDebug() << p_x_min << p_x_max << p_y_min << p_y_max;
    //    p_x_data.print(0, "Data X");
    //    p_y_data.print(0, "Data Y");

    update();
}

void PlotWidget::setLog(bool value)
{
    isLog = value;
}

void PlotWidget::paintEvent(QPaintEvent * event)
{
    // Paint the graph as a QPolygon inside rect
    QRectF rect(this->rect() - QMargins(20, 20, 20, 20));

    QPainter painter(this);

    QPolygonF polygon;
    polygon << rect.bottomLeft();

    for (size_t i = 0; i < p_y_data.n(); i++)
    {
        polygon << QPointF(
                    ((p_x_data[i] - p_x_min) / (p_x_max - p_x_min))* rect.width() + rect.left() ,
                    (rect.height() - ((p_y_data[i] - p_y_min) / (p_y_max - p_y_min))* rect.height()) + rect.top());
    }

    polygon << rect.bottomRight();

    painter.setRenderHint(QPainter::Antialiasing);

    QBrush brush(Qt::gray, Qt::CrossPattern);

    painter.setBrush(brush);

    painter.drawRect(rect);

    //    QLinearGradient lgrad(QPointF(0,0), QPointF(rect.top(),rect.bottom()));
    //                lgrad.setColorAt(0.0, Qt::blue);
    //                lgrad.setColorAt(1.0, Qt::transparent);

    QBrush lgrad_brush(Qt::black);
    painter.setBrush(lgrad_brush);

    painter.drawPolygon(polygon);
}
