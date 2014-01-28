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
#include <QList>
#include <QStringList>

class FileTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit FileTreeView(QWidget *parent = 0);

public slots:
    void itemChanged(const QModelIndex & item);
};


class FileSelectionModel : public QFileSystemModel
{
    Q_OBJECT
    
public slots:
    void setStringFilter(QString str);
    
public:
    explicit FileSelectionModel(QWidget *parent = 0);
//	inline QStringList getCheckedDirectories() const { return directories; }
	Qt::ItemFlags flags(const QModelIndex& index) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);    
    QStringList getFiles();
    
    
private:
//	QStringList directories;
    QStringList files;
    
    QList<QModelIndex> indices;
//    bool addPath(QModelIndex index);
//    bool removePath(QModelIndex index);
    bool addIndex(QModelIndex index);
    bool removeIndex(QModelIndex index);
    void refreshFiles();
};

#endif // FILETREEVIEW_H
