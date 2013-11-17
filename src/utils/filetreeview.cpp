#include "filetreeview.h"

FileTreeView::FileTreeView(QWidget *parent) :
    QTreeView(parent)
{
    
}

void FileTreeView::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug("Drag enter event");
    event->acceptProposedAction();
}

void FileTreeView::dragLeaveEvent(QDragLeaveEvent *event)
{
    qDebug("Drag leave event");
}

void FileTreeView::dropEvent(QDropEvent *event)
{
    qDebug("Drop event");
    event->acceptProposedAction();
}

void FileTreeView::dragMoveEvent(QDragMoveEvent *event)
{
//    qDebug("Drag move event");
    event->acceptProposedAction();
}



FileSourceModel::FileSourceModel(QWidget *parent) :
    QFileSystemModel(parent)
{
    
}

bool FileSourceModel::canDropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) const
{
    qDebug();
    
    return true;
}

Qt::DropActions FileSourceModel::supportedDropActions() const
{
    qDebug();
    
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags FileSourceModel::flags(const QModelIndex &index) const
{
//    qDebug();
    
    Qt::ItemFlags defaultFlags = QFileSystemModel::flags(index);

    if (index.isValid())
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    else
    {
        qDebug() << "invalid index";
        return Qt::ItemIsDropEnabled | defaultFlags;
    }
}

QStringList FileSourceModel::mimeTypes() const
{
    qDebug();
    
    QStringList types;
    types << "path";
    return types;
}

QMimeData *FileSourceModel::mimeData(const QModelIndexList &indexes) const
{
    qDebug();
    
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    foreach (const QModelIndex &index, indexes) {
        if (index.isValid()) 
        {
            QString text = filePath(index); //data(index, Qt::DisplayRole).toString();
            stream << text;
        }
    }

    mimeData->setData("path", encodedData);
    return mimeData;
}

bool FileSourceModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    qDebug() << "Dropping payload!";
    
    if (action == Qt::IgnoreAction)
        return true;

    if (!data->hasFormat("path"))
        return false;

    if (column > 0)
        return false;
    
    int beginRow;
    
    if (row != -1)
        beginRow = row;

    else if (parent.isValid())
        beginRow = parent.row();

    else
        beginRow = rowCount(QModelIndex());
    
    QByteArray encodedData = data->data("path");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QStringList newItems;
    int rows = 0;

    while (!stream.atEnd()) 
    {
        QString text;
        stream >> text;
        qDebug() << text;
        newItems << text;
        ++rows;
    }
    
    insertRows(beginRow, rows, QModelIndex());
    foreach (const QString &text, newItems) {
        QModelIndex idx = index(beginRow, 0, QModelIndex());
        setData(idx, text);
        beginRow++;
    }

    return true;
}




FileDisplayModel::FileDisplayModel(QWidget *parent) :
    QStandardItemModel(parent)
{
    
}

Qt::DropActions FileDisplayModel::supportedDropActions() const
{
    qDebug();
    
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags FileDisplayModel::flags(const QModelIndex &index) const
{
//    qDebug();
    
    Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);

    if (index.isValid())
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    else
    {
        qDebug() << "invalid index";
        return Qt::ItemIsDropEnabled | defaultFlags;
    }
}

QStringList FileDisplayModel::mimeTypes() const
{
    QStringList types;
    types << "path";
    return types;
}

bool FileDisplayModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    qDebug() << "Dropping payload!";
    
    if (action == Qt::IgnoreAction)
        return true;

    if (!data->hasFormat("path"))
        return false;

    if (column > 0)
        return false;
    
    int beginRow;
    
    if (row != -1)
        beginRow = row;

    else if (parent.isValid())
        beginRow = parent.row();

    else
        beginRow = rowCount(QModelIndex());
    
    QByteArray encodedData = data->data("path");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QStringList newItems;
    int rows = 0;

    while (!stream.atEnd()) 
    {
        QString text;
        stream >> text;
        qDebug() << text;
        newItems << text;
        ++rows;
    }
    
    insertRows(beginRow, rows, QModelIndex());
    foreach (const QString &text, newItems) {
        QModelIndex idx = index(beginRow, 0, QModelIndex());
        setData(idx, text);
        beginRow++;
    }

    return true;
}
