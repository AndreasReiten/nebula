#include "line.h"

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
}

Marker::Marker(double x, double y, double z)
{
    isTagged = false;
    
    xyz.set(1,3,0);
    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;
    
    tagged_color.set(1,4,0);
    tagged_color[0] = 0.3;
    tagged_color[1] = 0.3;
    tagged_color[2] = 1.0;
    tagged_color[3] = 0.9;
    
    untagged_color.set(1,4,0);
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
    if (isTagged) return tagged_color.data();
    else return untagged_color.data();
}

Matrix<double> & Marker::getCenter()
{
    return xyz;
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
}




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

double Line::length()
{
    return vecLength(p_position_a - p_position_b) + p_offset_a + p_offset_b;
}


Matrix<double> Line::effectivePosA()
{
    p_position_a + vecNormalize(p_position_a - p_position_b)*p_offset_a;
}

Matrix<double> Line::effectivePosB()
{
    p_position_b + vecNormalize(p_position_b - p_position_a)*p_offset_b;
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

LineView::LineView(QWidget *parent) :
    QTableView(parent)
{
    connect(this, SIGNAL(doubleClicked(QModelIndex &)), this, SLOT(itemChanged(const QModelIndex &)));
    connect(this, SIGNAL(clicked(const QModelIndex &)), this, SLOT(itemChanged(const QModelIndex &)));
}

Qt::ItemFlags LineModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = QStandardItemModel::flags(index);
    
    // Make the first column checkable
//    if (index.column() == 0)
//    {
//        f |= Qt::ItemIsUserCheckable;
//    }
    return f;
}

/* This function is used to set the data of an index
 * */
QVariant LineModel::data(const QModelIndex& index, int role) const
{
    // Return a state depending on role
//    if ((index.column() == 0) && (role == Qt::CheckStateRole)) return checklist.contains(index) ? Qt::Checked : Qt::Unchecked;

    return QStandardItemModel::data(index, role);
}

/* This function is called whenever the data of an index is changed.
 * Emits the signal dataChanged() which calls data().
 * */
bool LineModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    // Set data depending on role and corresponding value
//    if (role == Qt::CheckStateRole)
//    {
//        if (value == Qt::Checked)
//        {
//            addIndex(index);
//        }
//        else removeIndex(index);

//        emit dataChanged(index, index);
//        return true;
//    }
    return QStandardItemModel::setData(index, value, role);
}

int LineModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return QStandardItemModel::columnCount();
}
