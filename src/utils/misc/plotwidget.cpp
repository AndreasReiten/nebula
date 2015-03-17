#include "plotwidget.h"

#include <QPainter>
#include <QPaintEvent>

PlotLineWidget::PlotLineWidget(QWidget * parent) :
    QWidget(parent),
    isLog(false)
{
    p_x_min = 0;
    p_x_max = 1;
    p_y_min = 0;
    p_y_max = 1;

    double buf_x[] = {0.0, 0.1, 0.2, 0.3, 0.4, 0.5};

    p_x_data.setDeep(1, 6, buf_x);

    double buf_y[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    p_y_data.setDeep(1, 6, buf_y);
}

PlotLineWidget::~PlotLineWidget()
{

}

void PlotLineWidget::plot(double xmin, double xmax, double ymin, double ymax, const Matrix<double> &x_data, const Matrix<double> &y_data)
{
    p_x_min = xmin;
    p_x_max = xmax;
    p_y_min = ymin;
    p_y_max = ymax;

    p_x_data = x_data;

    p_y_data = y_data;

    update();
}

void PlotLineWidget::setLog(bool value)
{
    isLog = value;

    update();
}



void PlotLineWidget::saveAsImage()
{

}

void PlotLineWidget::paintEvent(QPaintEvent * event)
{
    // Paint the graph as a QPolygon inside rect
    QRectF rect(this->rect() - QMargins(20, 20, 20, 20));

    QPainter painter(this);

    QPolygonF polygon;
    polygon << rect.bottomLeft();

//    qDebug() << p_x_min << p_x_max << p_y_min << p_y_max;

    for (size_t i = 0; i < p_y_data.n(); i++)
    {
        if (isLog)
        {
            polygon << QPointF(
                        ((p_x_data[i] - p_x_min) / (p_x_max - p_x_min))* rect.width() + rect.left() ,
                        (rect.height() - (log10((p_y_data[i] - p_y_min) / (p_y_max - p_y_min)*1e2+1)/log10(1e2))* rect.height()) + rect.top());

//            qDebug() << (log10((p_y_data[i] - p_y_min) / (p_y_max - p_y_min)*1e2+1)/log10(1e2));
        }
        else
        {
            polygon << QPointF(
                        ((p_x_data[i] - p_x_min) / (p_x_max - p_x_min))* rect.width() + rect.left() ,
                        (rect.height() - ((p_y_data[i] - p_y_min) / (p_y_max - p_y_min))* rect.height()) + rect.top());
        }
    }

    polygon << rect.bottomRight();

    painter.setRenderHint(QPainter::Antialiasing);

    QBrush brush(Qt::gray, Qt::CrossPattern);

    painter.setBrush(brush);

    painter.drawRect(rect);

    QBrush lgrad_brush(Qt::black);
    painter.setBrush(lgrad_brush);

    painter.drawPolygon(polygon);
}


PlotSurfaceWidget::PlotSurfaceWidget(QWidget * parent) :
    QScrollArea(parent),
    isLog(true)
{
    p_data.set(2, 2 * 4);
    p_data[0] = 255; // B
    p_data[1] = 255; // G
    p_data[2] = 255; // R
    p_data[3] = 255; // A

    p_data[4] = 255;
    p_data[5] = 255;
    p_data[6] = 255;
    p_data[7] = 255;

    p_data[8] = 255;
    p_data[9] = 255;
    p_data[10] = 255;
    p_data[11] = 255;

    p_data[12] = 255;
    p_data[13] = 255;
    p_data[14] = 255;
    p_data[15] = 255;

    p_label = new QLabel;
    p_label->setBackgroundRole(QPalette::Base);
    p_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    p_label->setScaledContents(true);

    this->setBackgroundRole(QPalette::Dark);
    this->setWidget(p_label);

    QImage image(p_data.data(), p_data.m(), p_data.n() / 4, QImage::Format_RGB32);

    p_label->setPixmap(QPixmap::fromImage(image));
    p_label->adjustSize();


    fitToWindow();
}

PlotSurfaceWidget::~PlotSurfaceWidget()
{

}

void PlotSurfaceWidget::fitToWindow()
{
    double scale_factor = std::min((double) this->size().width() / (double) p_label->pixmap()->width(), (double) this->size().height() / (double) p_label->pixmap()->height());

    p_label->resize(scale_factor * p_label->pixmap()->size());
}

void PlotSurfaceWidget::resizeEvent(QResizeEvent * event)
{
    fitToWindow();
}


void PlotSurfaceWidget::saveAsImage()
{
    QString file_name = QFileDialog::getSaveFileName(this, "Save as text");

    QImage image(p_data.data(), p_data.m(), p_data.n() / 4, QImage::Format_RGB32);

    image.mirrored(0, 1).save(file_name);
}

void PlotSurfaceWidget::plot(const Matrix<double> &data)
{
    double log_scaling = 1e6;
    double log_offset = 1e-6;

    p_raw_data = data;

    p_data.resize(p_raw_data.m(), p_raw_data.n() * 4);

    double min, max;

    min = std::max(p_raw_data.min(), log_offset);
    max = p_raw_data.max();
//    p_raw_data.minmax(&min, &max);

//    qDebug() << min << max;

    for (int i = 0; i < p_raw_data.size(); i++)
    {
        if (isLog)
        {

//             (log10(((p_raw_data[i] - min) / (max - min))*log_scaling+log_offset)/log10(log_scaling));
//            ((log10(p_raw_data[i]+log_offset) - log10(min)) / (log10(max) - log10(min)))

            p_data[i * 4 + 0] = 255 * std::max(0.0,(log10(((p_raw_data[i] - min) / (max - min))*log_scaling+log_offset)/log10(log_scaling)));
            p_data[i * 4 + 1] = 255 * std::max(0.0,(log10(((p_raw_data[i] - min) / (max - min))*log_scaling+log_offset)/log10(log_scaling)));
            p_data[i * 4 + 2] = 255 * std::max(0.0,(log10(((p_raw_data[i] - min) / (max - min))*log_scaling+log_offset)/log10(log_scaling)));
            p_data[i * 4 + 3] = 255;
        }
        else
        {
            p_data[i * 4 + 0] = 255 * (p_raw_data[i] / max);
            p_data[i * 4 + 1] = 255 * (p_raw_data[i] / max);
            p_data[i * 4 + 2] = 255 * (p_raw_data[i] / max);
            p_data[i * 4 + 3] = 255;
        }


    }

    QImage image(p_data.data(), p_data.m(), p_data.n() / 4, QImage::Format_RGB32);

    p_label->setPixmap(QPixmap::fromImage(image));
    p_label->adjustSize();

    fitToWindow();
}

void PlotSurfaceWidget::setLog(bool value)
{
    isLog = value;

    plot(p_raw_data);
}
