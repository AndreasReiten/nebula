#ifndef MARKER_H
#define MARKER_H

#include <QOpenGLFunctions>
#include "lib/qxlib/qxlib.h"
#include "tools.h"

class Marker : protected QOpenGLFunctions
{
public:
    Marker();
    Marker(double x, double y, double z);
    
    ~Marker();
    
    float *getColor();
    double getDistance(double x, double y, double z);
    void setTagged(bool value);
    Matrix<float> &  getVerts();
    bool getTagged();
    Matrix<double> &getCenter();
    
private:
    bool isTagged;
    Matrix<double> xyz;
    
    Matrix<float> tagged_color;
    Matrix<float> untagged_color;
    
    Matrix<float> vertices;
};

#endif // MARKER_H
