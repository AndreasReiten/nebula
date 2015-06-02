#ifndef IMAGEMARKER_H
#define IMAGEMARKER_H


class ImageMarker
{
public:
    ImageMarker();
    ImageMarker(double x, double y, double w, double h);
    ~ImageMarker();

    void setX(double value);
    void setY(double value);
    void setW(double value);
    void setH(double value);

    double x();
    double y();
    double w();
    double h();

private:
    double p_x, p_y, p_w, p_h;
};

#endif // IMAGEMARKER_H
