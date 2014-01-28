#include "filetreeview.h"

FileTreeView::FileTreeView(QWidget *parent) :
    QTreeView(parent)
{
    connect(this, SIGNAL(expanded(const QModelIndex &)), this, SLOT(itemChanged(const QModelIndex &)));
    connect(this, SIGNAL(collapsed(const QModelIndex &)), this, SLOT(itemChanged(const QModelIndex &)));
    connect(this, SIGNAL(clicked(const QModelIndex &)), this, SLOT(itemChanged(const QModelIndex &)));
}

void FileTreeView::itemChanged(const QModelIndex & item)
{
    if (item.column() == 0) resizeColumnToContents(item.column());
}

FileSourceModel::FileSourceModel(QWidget *parent) :
    QFileSystemModel(parent)
{
    setReadOnly(true);
    setNameFilterDisables(false);
    setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files | QDir::Drives);
}

Qt::ItemFlags FileSourceModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags f = QFileSystemModel::flags(index);
	if (index.column() == 0) 
    { // make the first column checkable
		f |= Qt::ItemIsUserCheckable;
	}
	return f;
}

QVariant FileSourceModel::data(const QModelIndex& index, int role) const
{
    // Returns state depending on role (for example, is this index checked or not?)
	if (index.isValid() && (index.column() == 0) && (role == Qt::CheckStateRole)) 
    {
        // If an index has multiple children, of which at least one is unchecked, set the index check flag to partially checked
//        if (isDir(index))
//        {
//            qDebug() << filePath(index);
        if (directories.contains(filePath(index.parent()))) return Qt::Checked;
        else if (directories.contains(filePath(index))) return Qt::Checked;
        else if (data(index.parent(), Qt::CheckStateRole) == Qt::Checked) return Qt::Checked;
        else return Qt::Unchecked;
        
//            if (hasChildren(index))
//            {
////                if (canFetchMore(index)) fetchMore(index); 
                
//                int contained = 0;
//                for (int i = 0; i < rowCount(index); i++)
//                {
//                    if (directories.contains(filePath(index.child(i,0)))) contained++;
//                }
                
//                qDebug() << "children" << rowCount(index) << filePath(index);
                
//                if (contained == 0) return Qt::Unchecked; 
//                else if (contained == rowCount(index)) return Qt::Checked; 
//                else return Qt::PartiallyChecked;
                
                
//            }
//            else if (directories.contains(filePath(index)))
//            {
//                qDebug() << "check" << filePath(index);
//                return Qt::Checked;
//            } 
//            else 
//            {
//                qDebug() << "uncheck" << filePath(index);
//                return Qt::Unchecked;
//            }
//        }
//        else 
//        {
//            if (directories.contains(filePath(index)))
//            {
//                return Qt::Checked;
//            } 
//            else 
//            {
//                return Qt::Unchecked;
//            }
//        }
	}
	else return QFileSystemModel::data(index, role);
}

bool FileSourceModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    // Set data depending on role and corresponding value
	if ((index.isValid()) && (index.column() == 0) && (role == Qt::CheckStateRole)) 
    {
		// store checked paths, remove unchecked paths
		if (isDir(index)) 
        {
			if (value.toInt() == Qt::Checked)
            {
                addPath(index);
            }
            else
            {
                removePath(index);
            }
			directories.sort();
            files.sort();
		}
//        qDebug() << files;
//        qDebug() << directories;
		return true;
	}
	return QFileSystemModel::setData(index, value, role);
}


bool FileSourceModel::addPath(QModelIndex index)
{
    directories << filePath(index);
    if (fileInfo(index).isFile()) files << filePath(index);
    
    
    
    if (hasChildren(index))
    {
        for (int i = 0; i < rowCount(index); i++)
        {
            QModelIndex child = index.child(i,0);
            if (child.isValid())
            {
                directories << filePath(child);
                if (fileInfo(index).isFile()) files.removeAll(filePath(index));
//                emit dataChanged(index, child);
            }
        }
    }
    
    emit dataChanged(index.parent(), index.child(rowCount(index)-1,0));
}

bool FileSourceModel::removePath(QModelIndex index)
{
    directories.removeAll(filePath(index));
    if (fileInfo(index).isFile()) files.removeAll(filePath(index));
    
    emit dataChanged(index.parent(), index);
    
    if (hasChildren(index))
    {
        for (int i = 0; i < rowCount(index); i++)
        {
            QModelIndex child = index.child(i,0);
            if (child.isValid())
            {
                removePath(child);
            }
        }
    }
}
