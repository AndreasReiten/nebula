#include "filetreeview.h"

FileTreeView::FileTreeView(QWidget *parent) :
    QTreeWidget(parent)
{
    // Configure header item
    QTreeWidgetItem* headerItem = new QTreeWidgetItem();
    headerItem->setText(0,QString("File Name"));
    headerItem->setText(1,QString("Size (Bytes)"));
    headerItem->setText(2,QString("Path"));
    this->setHeaderItem(headerItem);
    
    // Initialize items
    QDir* rootDir = new QDir("/");
    QFileInfoList filesList = rootDir->entryInfoList();
    
    
    foreach(QFileInfo fileInfo, filesList)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0,fileInfo.fileName());
        
        if(fileInfo.isFile())
        {  
            item->setText(1,QString::number(fileInfo.size()));
            item->setIcon(0,*(new QIcon(":/art/file.png")));
        }
        
        if(fileInfo.isDir())
        {
            item->setIcon(0,*(new QIcon(":/art/folder.png")));
            addChildren(item,fileInfo.filePath());
        } 
        
        item->setText(2,fileInfo.filePath());
        this->addTopLevelItem(item);	  
    }	
    
//    connect(this , SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(showDirectory(QTreeWidgetItem*, int)));
}

void FileTreeView::addChildren(QTreeWidgetItem* item,QString filePath)
{
	QDir* rootDir = new QDir(filePath);
	QFileInfoList filesList = rootDir->entryInfoList();	  
	
	foreach(QFileInfo fileInfo, filesList)
	{
	    QTreeWidgetItem* child = new QTreeWidgetItem();
	    child->setText(0,fileInfo.fileName());
	    
	    
	    if(fileInfo.isFile())
	    {  
	      child->setText(1,QString::number(fileInfo.size()));
	    }
	    
	    if(fileInfo.isDir())
	    {
	      child->setIcon(0,*(new QIcon("folder.jpg")));
	      child->setText(2,fileInfo.filePath());
	    }  
	    
	    item->addChild(child);
	}
}

void FileTreeView::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug("Drag enter event");
    event->acceptProposedAction();
}

void FileTreeView::dragLeaveEvent(QDragLeaveEvent *event)
{
    qDebug("Drag leave event");
    event->accept();
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

void FileTreeView::startDrag(Qt::DropActions supportedActions)
{
    
}

void FileTreeView::showDirectory(QTreeWidgetItem* item, int )
{
    QDir* rootDir = new QDir(item->text(2));
    QFileInfoList filesList = rootDir->entryInfoList();	  
    
    foreach(QFileInfo fileInfo, filesList)
    {
        QTreeWidgetItem* child = new QTreeWidgetItem();
        child->setText(0,fileInfo.fileName());	  
        
        if(fileInfo.isFile())
        {  
            child->setText(1,QString::number(fileInfo.size()));
            child->setIcon(0,*(new QIcon("file.jpg")));
        }
        
        if(fileInfo.isDir())
        {
            child->setIcon(0,*(new QIcon("folder.jpg")));
            child->setText(2,fileInfo.filePath());
        } 
        
        item->addChild(child);
        
        resizeColumnToContents(0);
    }
} 

//FileSourceModel::FileSourceModel(QWidget *parent) :
//    QFileSystemModel(parent)
//{
    
//}

//bool FileSourceModel::canDropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) const
//{
//    qDebug();
    
//    return true;
//}

//Qt::DropActions FileSourceModel::supportedDropActions() const
//{
//    qDebug();
    
//    return Qt::CopyAction | Qt::MoveAction;
//}

//Qt::ItemFlags FileSourceModel::flags(const QModelIndex &index) const
//{
////    qDebug();
    
//    Qt::ItemFlags defaultFlags = QFileSystemModel::flags(index);

//    if (index.isValid())
//        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
//    else
//    {
//        qDebug() << "invalid index";
//        return Qt::ItemIsDropEnabled | defaultFlags;
//    }
//}

//QStringList FileSourceModel::mimeTypes() const
//{
//    qDebug();
    
//    QStringList types;
//    types << "path";
//    return types;
//}

//QMimeData *FileSourceModel::mimeData(const QModelIndexList &indexes) const
//{
//    qDebug();
    
//    QMimeData *mimeData = new QMimeData();
//    QByteArray encodedData;

//    QDataStream stream(&encodedData, QIODevice::WriteOnly);

//    foreach (const QModelIndex &index, indexes) {
//        if (index.isValid()) 
//        {
//            QString text = filePath(index); //data(index, Qt::DisplayRole).toString();
//            stream << text;
//        }
//    }

//    mimeData->setData("path", encodedData);
//    return mimeData;
//}

//bool FileSourceModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
//{
//    qDebug() << "Dropping payload!";
    
//    if (action == Qt::IgnoreAction)
//        return true;

//    if (!data->hasFormat("path"))
//        return false;

//    if (column > 0)
//        return false;
    
//    int beginRow;
    
//    if (row != -1)
//        beginRow = row;

//    else if (parent.isValid())
//        beginRow = parent.row();

//    else
//        beginRow = rowCount(QModelIndex());
    
//    QByteArray encodedData = data->data("path");
//    QDataStream stream(&encodedData, QIODevice::ReadOnly);
//    QStringList newItems;
//    int rows = 0;

//    while (!stream.atEnd()) 
//    {
//        QString text;
//        stream >> text;
//        qDebug() << text;
//        newItems << text;
//        ++rows;
//    }
    
//    insertRows(beginRow, rows, QModelIndex());
//    foreach (const QString &text, newItems) {
//        QModelIndex idx = index(beginRow, 0, QModelIndex());
//        setData(idx, text);
//        beginRow++;
//    }

//    return true;
//}




//FileDisplayModel::FileDisplayModel(QWidget *parent) :
//    QStandardItemModel(parent)
//{
    
//}

//Qt::DropActions FileDisplayModel::supportedDropActions() const
//{
//    qDebug();
    
//    return Qt::CopyAction | Qt::MoveAction;
//}

//Qt::ItemFlags FileDisplayModel::flags(const QModelIndex &index) const
//{
////    qDebug();
    
//    Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);

//    if (index.isValid())
//        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
//    else
//    {
//        qDebug() << "invalid index";
//        return Qt::ItemIsDropEnabled | defaultFlags;
//    }
//}

//QStringList FileDisplayModel::mimeTypes() const
//{
//    QStringList types;
//    types << "path";
//    return types;
//}

//bool FileDisplayModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
//{
//    qDebug() << "Dropping payload!";
    
//    if (action == Qt::IgnoreAction)
//        return true;

//    if (!data->hasFormat("path"))
//        return false;

//    if (column > 0)
//        return false;
    
//    int beginRow;
    
//    if (row != -1)
//        beginRow = row;

//    else if (parent.isValid())
//        beginRow = parent.row();

//    else
//        beginRow = rowCount(QModelIndex());
    
//    QByteArray encodedData = data->data("path");
//    QDataStream stream(&encodedData, QIODevice::ReadOnly);
//    QStringList newItems;
//    int rows = 0;

//    while (!stream.atEnd()) 
//    {
//        QString text;
//        stream >> text;
//        qDebug() << text;
//        newItems << text;
//        ++rows;
//    }
    
//    insertRows(beginRow, rows, QModelIndex());
//    foreach (const QString &text, newItems) {
//        QModelIndex idx = index(beginRow, 0, QModelIndex());
//        setData(idx, text);
//        beginRow++;
//    }

//    return true;
//}
