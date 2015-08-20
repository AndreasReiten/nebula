#include "imagemarker.h"

ImageMarker::ImageMarker()
{
    p_x = 0;
    p_y = 0;
    p_w = 0;
    p_h = 0;
}

ImageMarker::ImageMarker(double x, double y, double w, double h)
{
    p_x = x;
    p_y = y;
    p_w = w;
    p_h = h;
}

ImageMarker::~ImageMarker()
{

}

void ImageMarker::setX(double value)
{
    p_x = value;
}

void ImageMarker::setY(double value)
{
    p_y = value;
}

void ImageMarker::setW(double value)
{
    p_w = value;
}

void ImageMarker::setH(double value)
{
    p_h = value;
}

double ImageMarker::x()
{
    return p_x;
}

double ImageMarker::y()
{
    return p_y;
}

double ImageMarker::w()
{
    return p_w;
}

double ImageMarker::h()
{
    return p_h;
}

