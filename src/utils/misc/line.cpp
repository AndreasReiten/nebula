#include "line.h"

Line::Line()
{
    p_is_tagged = false;
    p_verts_computed = false;
    p_comment = "<No comment>";
    p_position_a.set(3, 1, 0);
    p_position_b.set(3, 1, 0);
    p_offset_a = 0;
    p_offset_b = 0;

    p_vertices.set(3, 10, 0);
    p_a.set(3, 1, 0);
    p_b.set(3, 1, 0);
    p_c.set(3, 1, 0);

    p_prism_side_a = 0.01;
    p_prism_side_b = 0.01;

    computePrism();
    computeVertices();
}

Line::Line(const Line &other)
{
    p_a = other.aVec();
    p_b = other.bVec();
    p_c = other.cVec();

    p_is_tagged = other.tagged();
    p_verts_computed = false;

    p_offset_a = other.offsetA();
    p_offset_b = other.offsetB();
    p_comment = other.comment();

    p_position_a = other.positionA();
    p_position_b = other.positionB();

    p_prism_side_a = other.prismSideA();
    p_prism_side_b = other.prismSideB();

    p_vertices.set(3, 10, 0);

    computePrism();
    computeVertices();
}

Line::Line(Matrix<double> pos_a, Matrix<double> pos_b)
{
    p_is_tagged = false;
    p_verts_computed = false;
    p_comment = "<No comment>";
    p_position_a = pos_a;
    p_position_b = pos_b;
    p_offset_a = 0;
    p_offset_b = 0;

    p_vertices.set(3, 10, 0);
    p_a.set(3, 1, 0);
    p_b.set(3, 1, 0);
    p_c.set(3, 1, 0);

    p_prism_side_a = 0.01;
    p_prism_side_b = 0.01;

    computePrism();
    computeVertices();
}
Line::Line(double x0, double y0, double z0, double x1, double y1, double z1)
{
    p_is_tagged = false;
    p_verts_computed = false;
    p_comment = "<No comment>";
    p_position_a.set(3, 1);
    p_position_b.set(3, 1);

    p_position_a[0] = x0;
    p_position_a[1] = y0;
    p_position_a[2] = z0;

    p_position_b[0] = x1;
    p_position_b[1] = y1;
    p_position_b[2] = z1;

    p_offset_a = 0;
    p_offset_b = 0;

    p_vertices.set(3, 10, 0);
    p_a.set(3, 1, 0);
    p_b.set(3, 1, 0);
    p_c.set(3, 1, 0);

    p_prism_side_a = 0.01;
    p_prism_side_b = 0.01;

    computePrism();
    computeVertices();
}
Line::~Line()
{

}

void Line::setCenter(Matrix<double> pos)
{
    Matrix<double> middle(3, 1);
    middle[0] = p_position_a[0] + 0.5 * (p_position_b[0] - p_position_a[0]);
    middle[1] = p_position_a[1] + 0.5 * (p_position_b[1] - p_position_a[1]);
    middle[2] = p_position_a[2] + 0.5 * (p_position_b[2] - p_position_a[2]);

    Matrix<double> translation(3, 1);
    translation[0] = - middle[0] + pos[0];
    translation[1] = - middle[1] + pos[1];
    translation[2] = - middle[2] + pos[2];

    translation.print(4);

    p_position_a += translation;
    p_position_b += translation;

    computePrism();
    computeVertices();
}

void Line::alignWithVec(Matrix<double> vec)
{
    Matrix<double> new_vec = vecNormalize(vec)*vecLength(p_position_b-p_position_a);

    Matrix<double> middle(3, 1);
    middle[0] = p_position_a[0] + 0.5 * (p_position_b[0] - p_position_a[0]);
    middle[1] = p_position_a[1] + 0.5 * (p_position_b[1] - p_position_a[1]);
    middle[2] = p_position_a[2] + 0.5 * (p_position_b[2] - p_position_a[2]);

    p_position_a = middle - 0.5*new_vec;
    p_position_b = middle + 0.5*new_vec;

    computePrism();
    computeVertices();
}

void Line::setPrismSideA(double value)
{
    p_prism_side_a = value;

    computePrism();
    computeVertices();
}
void Line::setPrismSideB(double value)
{
    p_prism_side_b = value;

    computePrism();
    computeVertices();
}

double Line::prismSideA() const
{
    return p_prism_side_a;
}

double Line::prismSideB() const
{
    return p_prism_side_b;
}

Matrix<double> Line::basePos()
{
    Matrix<double> eff_pos_a = effectivePosA();

    Matrix<double> base_pos = eff_pos_a - 0.5 * p_a - 0.5 * p_b;

    return base_pos;
}

const Matrix<double> Line::aVec() const
{
    return p_a;
}

const Matrix<double> Line::bVec() const
{
    return p_b;
}

const Matrix<double> Line::cVec() const
{
    return p_c;
}

void Line::setPositionA(Matrix<double> pos)
{
    p_position_a = pos;

    computePrism();
    computeVertices();
}
void Line::setPositionB(Matrix<double> pos)
{
    p_position_b = pos;

    computePrism();
    computeVertices();
}
void Line::setTagged(bool value)
{
    p_is_tagged = value;
}
void Line::setComment(QString str)
{
    p_comment = str;

    computePrism();
    computeVertices();
}

void Line::setOffsetA(double value)
{
    p_offset_a = value;

    computePrism();
    computeVertices();
}

void Line::setOffsetB(double value)
{
    p_offset_b = value;

    computePrism();
    computeVertices();
}

void Line::computePrism()
{
    // These vectors span the rectangular integration prism
    p_c = effectivePosB() - effectivePosA();

    p_a[0] = p_c[2];
    p_a[1] = 0;
    p_a[2] = -p_c[0];

    p_a = vecNormalize(p_a) * p_prism_side_a;

    p_b = vecNormalize(vecCross(p_c, p_a)) * p_prism_side_b; // The arguments of the cross product are not permutable
}

double Line::length() const
{
    return vecLength(p_position_a - p_position_b) + p_offset_a + p_offset_b;
}


const Matrix<double> Line::effectivePosA() const
{
    return p_position_a + vecNormalize(p_position_a - p_position_b) * p_offset_a;
}

const Matrix<double> Line::effectivePosB() const
{
    return p_position_b + vecNormalize(p_position_b - p_position_a) * p_offset_b;
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
const Matrix<double> &Line::positionA() const
{
    return p_position_a;
}
const Matrix<double> &Line::positionB() const
{
    return p_position_b;
}

double Line::offsetA() const
{
    return p_offset_a;
}
double Line::offsetB() const
{
    return p_offset_b;
}

const Matrix<float> &Line::vertices() const
{
    return p_vertices;
}
bool Line::tagged() const
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

    Matrix<double> base_pos = basePos();

    p_vertices[0] = eff_pos_a[0];
    p_vertices[1] = eff_pos_a[1];
    p_vertices[2] = eff_pos_a[2];

    p_vertices[3] = eff_pos_b[0];
    p_vertices[4] = eff_pos_b[1];
    p_vertices[5] = eff_pos_b[2];

    p_vertices[6] = base_pos[0];
    p_vertices[7] = base_pos[1];
    p_vertices[8] = base_pos[2];

    p_vertices[9] = base_pos[0] + p_a[0];
    p_vertices[10] = base_pos[1] + p_a[1];
    p_vertices[11] = base_pos[2] + p_a[2];

    p_vertices[12] = base_pos[0] + p_b[0];
    p_vertices[13] = base_pos[1] + p_b[1];
    p_vertices[14] = base_pos[2] + p_b[2];

    p_vertices[15] = base_pos[0] + p_c[0];
    p_vertices[16] = base_pos[1] + p_c[1];
    p_vertices[17] = base_pos[2] + p_c[2];

    p_vertices[18] = base_pos[0] + p_a[0] + p_b[0];
    p_vertices[19] = base_pos[1] + p_a[1] + p_b[1];
    p_vertices[20] = base_pos[2] + p_a[2] + p_b[2];

    p_vertices[21] = base_pos[0] + p_b[0] + p_c[0];
    p_vertices[22] = base_pos[1] + p_b[1] + p_c[1];
    p_vertices[23] = base_pos[2] + p_b[2] + p_c[2];

    p_vertices[24] = base_pos[0] + p_c[0] + p_a[0];
    p_vertices[25] = base_pos[1] + p_c[1] + p_a[1];
    p_vertices[26] = base_pos[2] + p_c[2] + p_a[2];

    p_vertices[27] = base_pos[0] + p_a[0] + p_b[0] + p_c[0];
    p_vertices[28] = base_pos[1] + p_a[1] + p_b[1] + p_c[1];
    p_vertices[29] = base_pos[2] + p_a[2] + p_b[2] + p_c[2];

    p_verts_computed = true;
}

GLuint * Line::vbo()
{
    return &p_vbo;
}

QDebug operator<<(QDebug dbg, const Line &line)
{
    dbg.nospace() << "# " << line.comment() << "\n" << line.prismSideA() << " # sida A\n" << line.prismSideB() << " # side B\n" << line.length() << " # side C\n";
    return dbg.maybeSpace();
}

QDataStream &operator<<(QDataStream &out, const Line &line)
{
    out << line.comment() << line.tagged() << line.positionA() << line.positionB() << line.offsetA() << line.offsetB()  << line.prismSideA() << line.prismSideB();

    return out;
}

QTextStream &operator<<(QTextStream &out, const Line &line)
{
    out << "# " << line.comment() << "\n" << line.prismSideA() << " # sida A\n" << line.prismSideB() << " # side B\n" << line.length() << " # side C\n";

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
    double prism_side_a;
    double prism_side_b;

    in >> comment >> isTagged >> position_a >> position_b >> offset_a >> offset_b >> prism_side_a >> prism_side_b;

    line.setComment(comment);
    line.setTagged(isTagged);
    line.setPositionA(position_a);
    line.setPositionB(position_b);
    line.setOffsetA(offset_a);
    line.setOffsetB(offset_b);
    line.setPrismSideA(prism_side_a);
    line.setPrismSideB(prism_side_b);
    return in;
}


