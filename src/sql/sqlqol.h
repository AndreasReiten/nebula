#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QMapIterator>
#include <QVariant>

inline QString sqlQueryError(QSqlQuery & query)
{
    QMapIterator<QString, QVariant> i(query.boundValues());

    QString values;

    while (i.hasNext())
    {
        i.next();
        values += " ["+QString(i.key().toUtf8().data())+"]:("
             + QString(i.value().toString().toUtf8().data())+")";
    }

    return QString("SQL query failed: "+query.lastQuery()+" Error msg: "+query.lastError().text()+" Values:"+values);
}
