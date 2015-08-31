#ifndef CUSTOMSQLQUERYMODEL_H
#define CUSTOMSQLQUERYMODEL_H

#include <QObject>
#include <QSqlQueryModel>


qint64 hash(const QString & str);

class CustomSqlQueryModel : public QSqlQueryModel
{
    Q_OBJECT
public:
    CustomSqlQueryModel(QObject * parent = 0)
    : QSqlQueryModel(parent) {;}

    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
};

#endif // CUSTOMSQLQUERYMODEL_H
