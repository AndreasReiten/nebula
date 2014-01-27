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
#include <QStringList>

class FileTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit FileTreeView(QWidget *parent = 0);

public slots:
    void itemChanged(const QModelIndex & item);
};


class FileSourceModel : public QFileSystemModel
{
    Q_OBJECT
    
public:
    explicit FileSourceModel(QWidget *parent = 0);
	inline QStringList getCheckedDirectories() const { return directories; }
	Qt::ItemFlags flags(const QModelIndex& index) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);    
    bool addPath(QModelIndex index);
    bool removePath(QModelIndex index);
    
private:
	QStringList directories;
    QStringList files;
};

#endif // FILETREEVIEW_H
