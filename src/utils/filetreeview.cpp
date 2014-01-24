#include "filetreeview.h"

FileTreeView::FileTreeView(QWidget *parent) :
    QTreeView(parent)
{
    connect(this, SIGNAL(expanded(const QModelIndex &)), this, SLOT(itemChanged(const QModelIndex &)));
    connect(this, SIGNAL(collapsed(const QModelIndex &)), this, SLOT(itemChanged(const QModelIndex &)));
    connect(this, SIGNAL(clicked(const QModelIndex &)), this, SLOT(itemChanged(const QModelIndex &)));
}

void FileTreeView::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug("Drag enter event");
    if (event->mimeData()->hasFormat("text/path"))
    {
        qDebug("Accepted");
        event->acceptProposedAction();
    }   
}

void FileTreeView::dropEvent(QDropEvent *event)
{
    qDebug("Drop event");
    
    QByteArray encodedData = event->mimeData()->data("text/path");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    
    while (!stream.atEnd()) 
    {
        QString text;
        stream >> text;
        
        qDebug() << text;
    }
    
    event->setDropAction(Qt::CopyAction);
    event->acceptProposedAction();
}


void FileTreeView::itemChanged(const QModelIndex & item)
{
    resizeColumnToContents(item.column());
}

FileSourceModel::FileSourceModel(QWidget *parent) :
    QFileSystemModel(parent)
{
    
}

Qt::DropActions FileSourceModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags FileSourceModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled;
}

QStringList FileSourceModel::mimeTypes() const
{
    QStringList types;
    types << "text/path";
    return types;
}

QMimeData *FileSourceModel::mimeData(const QModelIndexList &indexes) const
{
    qDebug();
    
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    foreach (const QModelIndex &index, indexes) 
    {
        if (index.isValid() && (index.column() == 0)) 
        {
            stream << filePath(index);
        }
    }

    mimeData->setData("text/path", encodedData);
    return mimeData;
}

