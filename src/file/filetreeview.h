#ifndef FILETREEVIEW_H
#define FILETREEVIEW_H

/*
 * These two classes supply a directory tree structure that can be used to select files.
 * */

#include <QTreeView>
#include <QFileSystemModel>
#include <QSet>
#include <QMap>
#include <QFileInfoList>
#include <QDir>

class FileTreeView : public QTreeView
{
        Q_OBJECT
    public:
        explicit FileTreeView(QWidget * parent = 0);

    public slots:
        void itemChanged(const QModelIndex &item);
    signals:
        void fileChanged(QString path);
};


class FileSelectionModel : public QFileSystemModel
{
        Q_OBJECT

    public slots:
        void setStringFilter(QString str);
        void removeFile(QString path); // Make an index from the path. Convert index to a persistent one. Look it up in the QSet. Kill.

    public:
        explicit FileSelectionModel(QWidget * parent = 0);
        Qt::ItemFlags flags(const QModelIndex &index) const;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
        int columnCount(const QModelIndex &parent = QModelIndex()) const;
        QMap<QString, QStringList> getPaths();

    private:
        QSet<QPersistentModelIndex> checklist;
        void removeIndex(QModelIndex index);
        void addIndex(QModelIndex index);
};

#endif // FILETREEVIEW_H
