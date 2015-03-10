#include "linemodel.h"

LineModel::LineModel(QWidget * parent) :
    QAbstractTableModel(parent)
{
    p_lines = new QList<Line>;

    p_columns = 13;
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
            case 1:
                return p_lines->at(index.row()).comment();

            case 2:
                return p_lines->at(index.row()).positionA()[0];

            case 3:
                return p_lines->at(index.row()).positionA()[1];

            case 4:
                return p_lines->at(index.row()).positionA()[2];

            case 5:
                return p_lines->at(index.row()).positionB()[0];

            case 6:
                return p_lines->at(index.row()).positionB()[1];

            case 7:
                return p_lines->at(index.row()).positionB()[2];

            case 8:
                return p_lines->at(index.row()).offsetA();

            case 9:
                return p_lines->at(index.row()).offsetB();

            case 10:
                return p_lines->at(index.row()).length();

            case 11:
                return p_lines->at(index.row()).prismSideA();

            case 12:
                return p_lines->at(index.row()).prismSideB();
        }
    }
    if (role == Qt::DecorationRole)
    {
        switch (index.column())
        {
            case 0:
                return QIcon(":/art/goto.png");
        }
    }
    else if (role == Qt::BackgroundRole)
    {
        if (p_lines->at(index.row()).tagged())
        {
            return QColor(150, 150, 255, 100);
        }
    }
    else if ((index.column() == 0) && (role == Qt::CheckStateRole))
    {
        if (p_lines->at(index.row()).tagged())
        {
            return Qt::Checked;
        }
        else
        {
            return Qt::Unchecked;
        }
    }
    else if (role == Qt::EditRole)
    {
        switch (index.column())
        {
            case 0:
                return "";

            case 1:
                return p_lines->at(index.row()).comment();

            case 2:
                return QString::number(p_lines->at(index.row()).positionA()[0]);

            case 3:
                return QString::number(p_lines->at(index.row()).positionA()[1]);

            case 4:
                return QString::number(p_lines->at(index.row()).positionA()[2]);

            case 5:
                return QString::number(p_lines->at(index.row()).positionB()[0]);

            case 6:
                return QString::number(p_lines->at(index.row()).positionB()[1]);

            case 7:
                return QString::number(p_lines->at(index.row()).positionB()[2]);

            case 8:
                return QString::number(p_lines->at(index.row()).offsetA());

            case 9:
                return QString::number(p_lines->at(index.row()).offsetB());

            case 10:
                return QString::number(p_lines->at(index.row()).length());

            case 11:
                return QString::number(p_lines->at(index.row()).prismSideA());

            case 12:
                return QString::number(p_lines->at(index.row()).prismSideB());
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
                case 1:
                    return QString("Comment");

                case 2:
                    return QString("x0");

                case 3:
                    return QString("y0");

                case 4:
                    return QString("z0");

                case 5:
                    return QString("x1");

                case 6:
                    return QString("y1");

                case 7:
                    return QString("z1");

                case 8:
                    return QString("Offset0");

                case 9:
                    return QString("Offset1");

                case 10:
                    return QString("Length");

                case 11:
                    return QString("Side0");

                case 12:
                    return QString("Side1");
            }
        }
    }

    return QVariant();
}

bool LineModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole)
    {
        switch (index.column())
        {
            case 1:
            {
                (*p_lines)[index.row()].setComment(value.toString());
                break;
            }

            case 2:
            {
                Matrix<double> pos = (*p_lines)[index.row()].positionA();
                pos[0] = value.toDouble();
                (*p_lines)[index.row()].setPositionA(pos);
                break;
            }

            case 3:
            {
                Matrix<double> pos = (*p_lines)[index.row()].positionA();
                pos[1] = value.toDouble();
                (*p_lines)[index.row()].setPositionA(pos);
                break;
            }

            case 4:
            {
                Matrix<double> pos = (*p_lines)[index.row()].positionA();
                pos[2] = value.toDouble();
                (*p_lines)[index.row()].setPositionA(pos);
                break;
            }

            case 5:
            {
                Matrix<double> pos = (*p_lines)[index.row()].positionB();
                pos[0] = value.toDouble();
                (*p_lines)[index.row()].setPositionB(pos);
                break;
            }

            case 6:
            {
                Matrix<double> pos = (*p_lines)[index.row()].positionB();
                pos[1] = value.toDouble();
                (*p_lines)[index.row()].setPositionB(pos);
                break;
            }

            case 7:
            {
                Matrix<double> pos = (*p_lines)[index.row()].positionB();
                pos[2] = value.toDouble();
                (*p_lines)[index.row()].setPositionB(pos);
                break;
            }

            case 8:
            {
                (*p_lines)[index.row()].setOffsetA(value.toDouble());
                break;
            }

            case 9:
            {
                (*p_lines)[index.row()].setOffsetB(value.toDouble());
                break;
            }

            case 10:
            {
                break;
            }

            case 11:
            {
                (*p_lines)[index.row()].setPrismSideA(value.toDouble());
                break;
            }

            case 12:
            {
                (*p_lines)[index.row()].setPrismSideB(value.toDouble());
                break;
            }
        }

        emit lineChanged(index.row());
    }
    else if ((index.column() == 0) && (role == Qt::CheckStateRole))
    {
        if (value == Qt::Checked)
        {
            (*p_lines)[index.row()].setTagged(true);
//            lineChecked(index.row());
        }
        else
        {
            (*p_lines)[index.row()].setTagged(false);
        }

        emit dataChanged(index, index.sibling(index.row(), p_columns - 1));
    }

    return true;
}

void LineModel::setLines(QList<Line> * list)
{
    p_lines = list;

    refresh();
}

Qt::ItemFlags LineModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(index);

    if (index.column() == 0)
    {
        f |= Qt::ItemIsUserCheckable;
    }

    if ((index.column() != 10) && (index.column() != 0))
    {
        f |= Qt::ItemIsEditable | Qt::ItemIsEnabled ;
    }
    else
    {
        f |= Qt::ItemIsEnabled ;
    }

    return f;
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
