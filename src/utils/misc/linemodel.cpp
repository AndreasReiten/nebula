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
    
    lines = new QList<Line>;
}

int LineModel::rowCount(const QModelIndex & /*parent*/) const
{
    return lines->size();
}

int LineModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}

QVariant LineModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        switch (index.column()) 
        {
        case 0:
            return lines->at(index.row()).comment();
        case 1:
            return lines->at(index.row()).length();
        default:
            return QString("...");
        }
    }
    return QVariant();
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
            if (value.toString() != "")
            (*lines)[index.row()].setComment(value.toString());
            break;
        }
    }
    return true;
}

void LineModel::setLines(QList<Line> * list)
{
    lines = list;
}

Qt::ItemFlags LineModel::flags(const QModelIndex & /*index*/) const
{
    return Qt::ItemIsSelectable |  Qt::ItemIsEditable | Qt::ItemIsEnabled ;
}

void LineModel::removeMarkedLine()
{
    for (int i = 0; i < lines->size(); i++) 
    {
        if (lines->at(i).tagged() == true)
        {
            lines->removeAt(i);
            i--;
        }
    }
}

