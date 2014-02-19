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

FileSelectionModel::FileSelectionModel(QWidget *parent) :
    QFileSystemModel(parent)
{
    setReadOnly(true);
    setNameFilterDisables(false);
    setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files | QDir::Drives);
}

int FileSelectionModel::columnCount(const QModelIndex& parent) const
{
    return QFileSystemModel::columnCount();
}

void FileSelectionModel::setStringFilter(QString str)
{
    setNameFilters(str.split(",", QString::SkipEmptyParts));
}

Qt::ItemFlags FileSelectionModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags f = QFileSystemModel::flags(index);
    
	// Make the first column checkable
    if (index.column() == 0) 
    { 
		f |= Qt::ItemIsUserCheckable;
	}
	return f;
}

QVariant FileSelectionModel::data(const QModelIndex& index, int role) const
{
    // Returns state depending on role (for example, is this index checked or not?)
	if (index.isValid() && (index.column() == 0) && (role == Qt::CheckStateRole)) 
    {
        QVariant state;
        if (indices.contains(index) && (rowCount(index) > 1))
        {
            int contained = 0;
            for (int i = 0; i < rowCount(index); i++)
            {
                if (indices.contains(index.child(i,0))) contained++;
            }
            
            if (contained == rowCount(index)) state = Qt::Checked;
            else state = Qt::PartiallyChecked;
        }
        else if (indices.contains(index)) state = Qt::Checked;
        else state = Qt::Unchecked;
        
        return state;
	}

	else return QFileSystemModel::data(index, role);
}

bool FileSelectionModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    // Set data depending on role and corresponding value
	if ((index.isValid()) && (index.column() == 0) && (role == Qt::CheckStateRole)) 
    {
		// Store checked indices, remove unchecked indices
        if (value.toInt() == Qt::Checked)
        {
            addIndex(index);
        }
        else
        {
            removeIndex(index);
        }
		return true;
	}
	return QFileSystemModel::setData(index, value, role);
}


bool FileSelectionModel::addIndex(QModelIndex index)
{
    indices << index;
    
    if (hasChildren(index))
    {
        for (int i = 0; i < rowCount(index); i++)
        {
            QModelIndex child = index.child(i,0);
            if (child.isValid())
            {
                addIndex(child);
            }
        }
    }
    
    emit dataChanged(index.parent(), index.child(rowCount(index)-1,0));
}

bool FileSelectionModel::removeIndex(QModelIndex index)
{
    indices.removeAll(index); // If the index of an item changes it cannot be removed. This can occur if the number of items in a folder change.
    
    if (hasChildren(index))
    {
        for (int i = 0; i < rowCount(index); i++)
        {
            QModelIndex child = index.child(i,0);
            if (child.isValid())
            {
                removeIndex(child);
            }
        }
    }
    
    emit dataChanged(index.parent(), index.child(rowCount(index)-1,0));
}

QStringList FileSelectionModel::getFiles()
{
    refreshFiles();
    return files;
}

void FileSelectionModel::refreshFiles()
{
    files.clear();
    for (int i = 0; i < indices.size(); i++)
    {
        if (fileInfo(indices.at(i)).isFile() && fileInfo(indices.at(i)).isReadable() && fileInfo(indices.at(i)).exists()) files << filePath(indices.at(i));  
    }
}
