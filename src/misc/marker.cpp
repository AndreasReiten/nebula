#include "marker.h"

Marker::Marker()
{
    isTagged = false;

    xyz.set(1, 3, 0);
    xyz[0] = 0.0;
    xyz[1] = 0.0;
    xyz[2] = 0.0;

    tagged_color.set(1, 4, 0);
    tagged_color[0] = 1.0;
    tagged_color[1] = 0.2;
    tagged_color[2] = 0.7;
    tagged_color[3] = 1.0;

    untagged_color.set(1, 4, 0);
    untagged_color[0] = 1.0;
    untagged_color[1] = 0.2;
    untagged_color[2] = 0.7;
    untagged_color[3] = 0.3;
}

Marker::Marker(double x, double y, double z)
{
    isTagged = false;

    xyz.set(1, 3, 0);
    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;

    tagged_color.set(1, 4, 0);
    tagged_color[0] = 0.3;
    tagged_color[1] = 0.3;
    tagged_color[2] = 1.0;
    tagged_color[3] = 0.9;

    untagged_color.set(1, 4, 0);
    untagged_color[0] = 1.0;
    untagged_color[1] = 0.3;
    untagged_color[2] = 0.3;
    untagged_color[3] = 0.8;
}

Marker::~Marker()
{
}

float * Marker::getColor()
{
    if (isTagged)
    {
        return tagged_color.data();
    }
    else
    {
        return untagged_color.data();
    }
}

Matrix<double> &Marker::getCenter()
{
    return xyz;
}

double Marker::getDistance(double x, double y, double z)
{
    Matrix<double> xyz2(1, 3);
    xyz2[0] = x;
    xyz2[1] = y;
    xyz2[2] = z;

    return vecLength(xyz - xyz2);
}

void Marker::setTagged(bool value)
{
    isTagged = value;
}

bool Marker::getTagged()
{
    return isTagged;
}

Matrix<float> &Marker::getVerts()
{
    float l = 0.1;

    vertices.set(6, 3, 0);

    vertices[0] = xyz[0] - l;
    vertices[1] = xyz[1];
    vertices[2] = xyz[2];

    vertices[3] = xyz[0] + l;
    vertices[4] = xyz[1];
    vertices[5] = xyz[2];

    vertices[6] = xyz[0];
    vertices[7] = xyz[1] - l;
    vertices[8] = xyz[2];

    vertices[9] = xyz[0];
    vertices[10] = xyz[1] + l;
    vertices[11] = xyz[2];

    vertices[12] = xyz[0];
    vertices[13] = xyz[1];
    vertices[14] = xyz[2] - l;

    vertices[15] = xyz[0];
    vertices[16] = xyz[1];
    vertices[17] = xyz[2] + l;

    return vertices;
}
