#include "customsqlquerymodel.h"

#include <QBrush>
#include <QDebug>

QVariant CustomSqlQueryModel::data ( const QModelIndex & index, int role ) const
{
    if (role == Qt::ForegroundRole)
    {
        const QVariant value(data(index,Qt::DisplayRole));
        const QVariant left_value(data(index.sibling(index.row(),3),Qt::DisplayRole));

        if (left_value.toInt() == 0)
        {
            return QBrush(QColor(70,70,70));
        }
        if (value.toString() == p_match_string)
        {
            return QBrush(QColor(165,146,99));
        }
        else
        {
            return QBrush(QColor(200,200,200));
        }
    }

    return QSqlQueryModel::data(index,role);
}

void CustomSqlQueryModel::indexChanged(const QModelIndex & index)
{
    const QVariant value(data(index,Qt::DisplayRole));
    p_match_string = value.toString();

//    emit dataChanged(index(0,0), index(rowCount()-1,columnCount()-1));
    emit dataChanged(QModelIndex(), QModelIndex());
}


//qint64 hash(const QString & str)
//{
//  QByteArray hash = QCryptographicHash::hash(
//    QByteArray::fromRawData((const char*)str.utf16(), str.length()*2),
//    QCryptographicHash::Md5
//  );
//  Q_ASSERT(hash.size() == 16);
//  QDataStream stream(&hash, QIODevice::ReadWrite);
//  qint64 a, b;
//  stream >> a >> b;
//  return a ^ b;
//}
