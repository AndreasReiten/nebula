#ifndef FILETREEVIEW_H
#define FILETREEVIEW_H

#include <QTreeView>
#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDebug>

class FileTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit FileTreeView(QWidget *parent = 0);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
//    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
//    void dragMoveEvent(QDragMoveEvent *event);
    
signals:

public slots:
    void itemChanged(const QModelIndex & item);

};

class FileSourceModel : public QFileSystemModel
{
    Q_OBJECT
public:
    explicit FileSourceModel(QWidget *parent = 0);
    
protected:
    Qt::DropActions supportedDropActions() const;
//    Qt::DropActions supportedDragActions() const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
//    bool canDropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) const;
    
    
signals:

public slots:

};

//class FileDisplayModel : public QStandardItemModel
//{
//    Q_OBJECT
//public:
//    explicit FileDisplayModel(QWidget *parent = 0);
    
//protected:
//    Qt::DropActions supportedDropActions() const;
////    Qt::DropActions supportedDragActions() const;
//    Qt::ItemFlags flags(const QModelIndex &index) const;   
//    QStringList mimeTypes() const;
    
//    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

//signals:

//public slots:

//};

#endif // FILETREEVIEW_H
