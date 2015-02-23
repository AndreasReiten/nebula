#ifndef LINE_H
#define LINE_H

#include <QTableView>
#include <QStandardItemModel>

#include "../math/qxmathlib.h"
#include "../opencl/qxopencllib.h"
#include "../file/qxfilelib.h"
#include "../svo/qxsvolib.h"

class LineView : public QTableView
{
    Q_OBJECT
public:
    explicit LineView(QWidget *parent = 0);
};


class LineModel : public QStandardItemModel
{
    Q_OBJECT
    
public:
    explicit LineModel(QWidget *parent = 0);
    
    Qt::ItemFlags flags(const QModelIndex& index) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
};

class Line
{
    // The length of the line is given by: vecLen(position_a - position_b) + offset_a + offset_b
    
public:
    Line();
    Line(Matrix<double> pos_a, Matrix<double> pos_b);
    Line(double x0, double y0, double z0, double x1, double y1, double z1);
    ~Line();
    
    void setPositionA(Matrix<double> pos);
    void setPositionB(Matrix<double> pos);
    void setTagged(bool value);
    void setComment(QString str);
    void setOffsetA(double value);
    void setOffsetB(double value);    
    
    Matrix<double> effectivePosA();
    Matrix<double> effectivePosB();
    double length();
    double distancePointToLine(Matrix<double> pos);
    const Matrix<double> & positionA() const;
    const Matrix<double> & positionB() const;
    const double offsetA() const;
    const double offsetB() const;
    Matrix<float> & vertices();
    const QString comment() const; 
    const bool tagged() const;
        
private:
    bool p_is_tagged;
    bool p_verts_computed;
    
    double p_offset_a;
    double p_offset_b;
    QString p_comment;
    
    Matrix<double> p_position_a;
    Matrix<double> p_position_b;
    Matrix<float> p_vertices;
    
    void computeVertices();
};

Q_DECLARE_METATYPE(Line);

QDebug operator<<(QDebug dbg, const Line &line);

QDataStream &operator<<(QDataStream &out, const Line &line);
QDataStream &operator>>(QDataStream &in, Line &line);



class Marker
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

#endif // LINE_H
