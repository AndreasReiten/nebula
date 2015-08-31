#include "customsqlquerymodel.h"

#include <QBrush>

#include <QByteArray>
#include <QDataStream>
#include <QCryptographicHash>

#include <QDebug>

QVariant CustomSqlQueryModel::data ( const QModelIndex & index, int role ) const
{
//    if (role == Qt::ForegroundRole)
//    {
//        QString tmp = data(index,Qt::DisplayRole).toString();

//        qint64 value = std::abs(hash(tmp)) % 255;

//        return QBrush(QColor(155,value,255-value));
//    }

    return QSqlQueryModel::data(index,role);
}

qint64 hash(const QString & str)
{
  QByteArray hash = QCryptographicHash::hash(
    QByteArray::fromRawData((const char*)str.utf16(), str.length()*2),
    QCryptographicHash::Md5
  );
  Q_ASSERT(hash.size() == 16);
  QDataStream stream(&hash, QIODevice::ReadWrite);
  qint64 a, b;
  stream >> a >> b;
  return a ^ b;
}
