#ifndef MARKER_H
#define MARKER_H

#include <QOpenGLFunctions>
#include "matrix.h"
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
//    GLuint getVbo();
    Matrix<float> &  getVerts();
    bool getTagged();
    Matrix<double> &getCenter();
    
private:
    bool isTagged;
    Matrix<double> xyz;
    
    Matrix<float> tagged_color;
    Matrix<float> untagged_color;
    
    Matrix<float> vertices;
//    GLuint vbo;
};

#endif // MARKER_H
