#ifndef CUSTOMSQLQUERYMODEL_H
#define CUSTOMSQLQUERYMODEL_H

#include <QObject>
#include <QSqlQueryModel>


qint64 hash(const QString & str);

class CustomSqlQueryModel : public QSqlQueryModel
{
    Q_OBJECT
public slots:
    void indexChanged(const QModelIndex & index);

public:
    CustomSqlQueryModel(QObject * parent = 0)
    : QSqlQueryModel(parent) {;}

    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;

private:
    QString p_match_string;
};

#endif // CUSTOMSQLQUERYMODEL_H
