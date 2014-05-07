#ifndef FILETREEVIEW_H
#define FILETREEVIEW_H

/*
 * These two classes supply a directory tree structure that can be used to select files.
 * */

#include <QTreeView>
#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDebug>
#include <QList>
#include <QStringList>

class FileTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit FileTreeView(QWidget *parent = 0);

public slots:
    void itemChanged(const QModelIndex & item);
//    void itemClicked(const QModelIndex & index);
signals:
    void fileChanged(QString path);
};


class FileSelectionModel : public QFileSystemModel
{
    Q_OBJECT
    
public slots:
    void setStringFilter(QString str);
    
public:
    explicit FileSelectionModel(QWidget *parent = 0);
	Qt::ItemFlags flags(const QModelIndex& index) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);    
    QStringList getFiles();
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    
private:
    QStringList files;
    
    QList<QModelIndex> indices;
    void addIndex(QModelIndex index);
    void removeIndex(QModelIndex index);
    void refreshFiles();
};

#endif // FILETREEVIEW_H
