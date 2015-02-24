#include "line.h"

Line::Line()
{
    p_comment = "<No comment>";
    p_position_a.set(3,1,0);
    p_position_b.set(3,1,0);
    p_offset_a = 0;
    p_offset_b = 0;
    
    p_vertices.set(3,2,0);
}

Line::Line(Matrix<double> pos_a, Matrix<double> pos_b)
{
    p_comment = "<No comment>";
    p_position_a = pos_a;
    p_position_b = pos_b;
    p_offset_a = 0;
    p_offset_b = 0;
    
    p_vertices.set(3,2,0);
}
Line::Line(double x0, double y0, double z0, double x1, double y1, double z1)
{
    p_comment = "<No comment>";
    p_position_a.set(3,1);
    p_position_b.set(3,1);

    p_position_a[0] = x0;
    p_position_a[1] = y0;
    p_position_a[2] = z0;
    
    p_position_b[0] = x1;
    p_position_b[1] = y1;
    p_position_b[2] = z1;
    
    p_offset_a = 0;
    p_offset_b = 0;
    
    p_vertices.set(3,2,0);
}
Line::~Line()
{
    
}

void Line::setPositionA(Matrix<double> pos)
{
    p_position_a = pos;
    
    computeVertices();
}
void Line::setPositionB(Matrix<double> pos)
{
    p_position_b = pos;
    
    computeVertices();
}
void Line::setTagged(bool value)
{
    p_is_tagged = value;
}
void Line::setComment(QString str)
{
    p_comment = str;
    
    computeVertices();
}

void Line::setOffsetA(double value)
{
    p_offset_a = value;
    
    computeVertices();
}

void Line::setOffsetB(double value)
{
    p_offset_b = value;
}

const double Line::length() const
{
    return vecLength(p_position_a - p_position_b) + p_offset_a + p_offset_b;
}


Matrix<double> Line::effectivePosA()
{
    return p_position_a + vecNormalize(p_position_a - p_position_b)*p_offset_a;
}

Matrix<double> Line::effectivePosB()
{
    return p_position_b + vecNormalize(p_position_b - p_position_a)*p_offset_b;
}

double Line::distancePointToLine(Matrix<double> pos)
{
    double l = length();
    
    Matrix<double> eff_pos_a = effectivePosA();
    Matrix<double> eff_pos_b = effectivePosB();
    
    if (l <= 0)
    {
        return vecLength(pos - eff_pos_a);
    }
    
    const double t = vecDot(pos - eff_pos_a, eff_pos_b - eff_pos_a) / l; // This is readily shown by drawing out the relevant vectors
    
    if (t < 0.0) 
    {
        return vecLength(pos - eff_pos_a);  // Beyond the 'eff_pos_a' end of the segment
    }
    else if (t > 1.0)
    {
        return vecLength(pos - eff_pos_b);  // Beyond the 'eff_pos_b' end of the segment
    }
    
    Matrix<double> projection = eff_pos_a + t * (eff_pos_b - eff_pos_a);  // Projection falls on the segment
    
    return vecLength(pos - projection);
}
const Matrix<double> & Line::positionA() const
{
    return p_position_a;
}
const Matrix<double> &Line::positionB() const
{
    return p_position_b;
}

const double Line::offsetA() const
{
    return p_offset_a;
}
const double Line::offsetB() const
{
    return p_offset_b;
}

Matrix<float> & Line::vertices()
{
    return vertices();
}
const bool Line::tagged() const
{
    return p_is_tagged;
}
    
const QString Line::comment() const
{
    return p_comment;
}

void Line::computeVertices()
{
    Matrix<double> eff_pos_a = effectivePosA();
    Matrix<double> eff_pos_b = effectivePosB();
    
    p_vertices[0] = eff_pos_a[0];
    p_vertices[1] = eff_pos_a[1];
    p_vertices[2] = eff_pos_a[2];
    
    p_vertices[3] = eff_pos_b[0];
    p_vertices[4] = eff_pos_b[1];
    p_vertices[5] = eff_pos_b[2];
    
    p_verts_computed = true;
}

QDebug operator<<(QDebug dbg, const Line &line)
{
    dbg.nospace() << "(...)";
    return dbg.maybeSpace();
}

QDataStream &operator<<(QDataStream &out, const Line &line)
{
    out << line.comment() << line.tagged() << line.positionA() << line.positionB() << line.offsetA() << line.offsetB();

    return out;
}

QDataStream &operator>>(QDataStream &in, Line &line)
{
    QString comment;
    bool isTagged;
    Matrix<double> position_a;
    Matrix<double> position_b;
    double offset_a;
    double offset_b;

    in >> comment >> isTagged >> position_a >> position_b >> offset_a >> offset_b;
    
    line.setComment(comment);
    line.setTagged(isTagged);
    line.setPositionA(position_a);
    line.setPositionB(position_b);
    line.setOffsetA(offset_a);
    line.setOffsetB(offset_b);
    return in;
}


