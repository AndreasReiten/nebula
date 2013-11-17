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
#include <QTreeWidgetItem>
#include <QDebug>

class FileTreeView : public QTreeWidget
{
    Q_OBJECT
public:
    explicit FileTreeView(QWidget *parent = 0);
    
protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void startDrag(Qt::DropActions supportedActions);
    
signals:

public slots:
    void showDirectory(QTreeWidgetItem* item, int);  

private:
    void addChildren(QTreeWidgetItem* item,QString filePath);
    
};

#endif // FILETREEVIEW_H
