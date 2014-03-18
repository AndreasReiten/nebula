#include "marker.h"

Marker::Marker()
{
    isTagged = false;
    
    xyz.set(1,3,0);
    xyz[0] = 0.0;
    xyz[1] = 0.0;
    xyz[2] = 0.0;
    
    tagged_color.set(1,4,0);
    tagged_color[0] = 1.0;
    tagged_color[1] = 0.2;
    tagged_color[2] = 0.7;
    tagged_color[3] = 1.0;
    
    untagged_color.set(1,4,0);
    untagged_color[0] = 1.0;
    untagged_color[1] = 0.2;
    untagged_color[2] = 0.7;
    untagged_color[3] = 0.3;
    
//    glGenBuffers(1, &vbo);
    
//    generateVbo();  
}

Marker::Marker(double x, double y, double z)
{
    isTagged = false;
    
    xyz.set(1,3,0);
    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;
    
    
    tagged_color.set(1,4,0);
    tagged_color[0] = 0.7;
    tagged_color[1] = 0.2;
    tagged_color[2] = 1.0;
    tagged_color[3] = 0.7;
    
    untagged_color.set(1,4,0);
    untagged_color[0] = 1.0;
    untagged_color[1] = 0.2;
    untagged_color[2] = 0.7;
    untagged_color[3] = 0.6;
    
//    glGenBuffers(1, &vbo);
    
//    generateVbo();
}

Marker::~Marker()
{
//    glDeleteBuffers(1, &vbo);
}

//GLuint Marker::getVbo()
//{
//    return vbo;
//}

float * Marker::getColor()
{
    if (isTagged) return tagged_color.data();
    else return untagged_color.data();
}

double Marker::getDistance(double x, double y, double z)
{
    Matrix<double> xyz2(1,3);
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

Matrix<float> & Marker::getVerts()
{
    float l = 0.1;
    
    vertices.set(6,3,0);
    
    vertices[0] = xyz[0]-l;
    vertices[1] = xyz[1];
    vertices[2] = xyz[2];
    
    vertices[3] = xyz[0]+l;
    vertices[4] = xyz[1];
    vertices[5] = xyz[2];
    
    vertices[6] = xyz[0];
    vertices[7] = xyz[1]-l;
    vertices[8] = xyz[2];
    
    vertices[9] = xyz[0];
    vertices[10] = xyz[1]+l;
    vertices[11] = xyz[2];
    
    vertices[12] = xyz[0];
    vertices[13] = xyz[1];
    vertices[14] = xyz[2]-l;
    
    vertices[15] = xyz[0];
    vertices[16] = xyz[1];
    vertices[17] = xyz[2]+l;
    
    return vertices;
//    glBindBuffer(GL_ARRAY_BUFFER, vbo);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT)*vbo_dummy.size(), vbo_dummy.data(), GL_STATIC_DRAW);
//    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
