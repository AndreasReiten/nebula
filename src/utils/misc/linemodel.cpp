#include "linemodel.h"

//LineView::LineView(QWidget *parent) :
//    QTableView(parent)
//{

//}

LineModel::LineModel(QWidget *parent) :
    QAbstractTableModel(parent)
{
//    lines << Line(0,0,0,1,1,1);
//    lines << Line(2,-2,2,1,1,1);
//    lines << Line(1,1,1,1,1,1);
    
    p_lines = new QList<Line>;

    p_columns = 10;
}

int LineModel::rowCount(const QModelIndex & /*parent*/) const
{
    return p_lines->size();
}

int LineModel::columnCount(const QModelIndex & /*parent*/) const
{
    return p_columns;
}

QVariant LineModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        switch (index.column()) 
        {
        case 0:
            return p_lines->at(index.row()).comment();
        case 1:
            return p_lines->at(index.row()).positionA()[0];
        case 2:
            return p_lines->at(index.row()).positionA()[1];
        case 3:
            return p_lines->at(index.row()).positionA()[2];
        case 4:
            return p_lines->at(index.row()).positionB()[0];
        case 5:
            return p_lines->at(index.row()).positionB()[1];
        case 6:
            return p_lines->at(index.row()).positionB()[2];
        case 7:
            return p_lines->at(index.row()).offsetA();
        case 8:
            return p_lines->at(index.row()).offsetB();
        case 9:
            return p_lines->at(index.row()).length();
        }
    }
    else if (role == Qt::BackgroundRole)
    {
        if (p_lines->at(index.row()).tagged()) return QColor(255,25,167,100);
    }
    else if(role == Qt::EditRole)
    {
        switch (index.column()) 
        {
        case 0:
            return p_lines->at(index.row()).comment();
        case 1:
            return QString::number(p_lines->at(index.row()).positionA()[0]);
        case 2:
            return QString::number(p_lines->at(index.row()).positionA()[1]);
        case 3:
            return QString::number(p_lines->at(index.row()).positionA()[2]);
        case 4:
            return QString::number(p_lines->at(index.row()).positionB()[0]);
        case 5:
            return QString::number(p_lines->at(index.row()).positionB()[1]);
        case 6:
            return QString::number(p_lines->at(index.row()).positionB()[2]);
        case 7:
            return QString::number(p_lines->at(index.row()).offsetA());
        case 8:
            return QString::number(p_lines->at(index.row()).offsetB());
        case 9:
            return p_lines->at(index.row()).length();
        }
    }
    return QVariant();(1.0,0.1,0.7,0.9);
}

QVariant LineModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) 
        {
            switch (section)
            {
            case 0:
                return QString("Comment");
            case 1:
                return QString("x0");
            case 2:
                return QString("y0");
            case 3:
                return QString("z0");
            case 4:
                return QString("x1");
            case 5:
                return QString("y1");
            case 6:
                return QString("z1");
            case 7:
                return QString("Offset0");
            case 8:
                return QString("Offset1");
            case 9:
                return QString("Length");
            }
        }
    }
    return QVariant();
}

bool LineModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role == Qt::EditRole)
    {
        switch (index.column()) 
        {
        case 0:
        {
            (*p_lines)[index.row()].setComment(value.toString());
            break;
        }
        case 1:
        {
            Matrix<double> pos = (*p_lines)[index.row()].positionA();
            pos[0] = value.toDouble();
            (*p_lines)[index.row()].setPositionA(pos);
            break;
        }
        case 2:
        {
            Matrix<double> pos = (*p_lines)[index.row()].positionA();
            pos[1] = value.toDouble();
            (*p_lines)[index.row()].setPositionA(pos);
            break;
        }
        case 3:
        {
            Matrix<double> pos = (*p_lines)[index.row()].positionA();
            pos[2] = value.toDouble();
            (*p_lines)[index.row()].setPositionA(pos);
            break;
        }
        case 4:
        {
            Matrix<double> pos = (*p_lines)[index.row()].positionA();
            pos[0] = value.toDouble();
            (*p_lines)[index.row()].setPositionA(pos);
            break;
        }
        case 5:
        {
            Matrix<double> pos = (*p_lines)[index.row()].positionA();
            pos[1] = value.toDouble();
            (*p_lines)[index.row()].setPositionA(pos);
            break;
        }
        case 6:
        {
            Matrix<double> pos = (*p_lines)[index.row()].positionA();
            pos[2] = value.toDouble();
            (*p_lines)[index.row()].setPositionA(pos);
            break;
        }
        case 7:
        {
            (*p_lines)[index.row()].setOffsetA(value.toDouble());
            break;
        }
        case 8:
        {
            (*p_lines)[index.row()].setOffsetB(value.toDouble());
            break;
        }
        }
        
        emit lineChanged(index.row());
    }
    return true;
}

void LineModel::setLines(QList<Line> * list)
{
    p_lines = list;
}

Qt::ItemFlags LineModel::flags(const QModelIndex & index) const
{
    if (index.column() < p_columns - 1) return Qt::ItemIsSelectable |  Qt::ItemIsEditable | Qt::ItemIsEnabled ;
    else return Qt::ItemIsEnabled ;
}

void LineModel::removeMarkedLine()
{
    for (int i = 0; i < p_lines->size(); i++)
    {
        if (p_lines->at(i).tagged() == true)
        {
            p_lines->removeAt(i);
            i--;
        }
    }
    
    refresh();
}

void LineModel::refresh()
{
    beginResetModel();
    endResetModel();
}

void LineModel::highlight(QModelIndex index)
{
    bool tag = (*p_lines)[index.row()].tagged();
    (*p_lines)[index.row()].setTagged(!tag);
    refresh();
}
