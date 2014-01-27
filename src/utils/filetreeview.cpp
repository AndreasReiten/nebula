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
    setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
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
        if (isDir(index))
        {
            if (hasChildren(index))
            {
                int contained = 0;
                for (int i = 0; i < rowCount(index); i++)
                {
                    if (directories.contains(filePath(index.child(i,0)))) contained++;
                }
                if (contained == 0) return Qt::Unchecked; 
                else if (contained == rowCount(index)) return Qt::Checked; 
                else return Qt::PartiallyChecked;
            }
            else if (directories.contains(filePath(index)))
            {
                return Qt::Checked;
            } 
            else 
            {
                return Qt::Unchecked;
            }
        }
        else 
        {
            if (files.contains(filePath(index)))
            {
                return Qt::Checked;
            } 
            else 
            {
                return Qt::Unchecked;
            }
        }
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
			if (value.toInt() == Qt::Checked) addPath(index);
            else removePath(index);
            
			directories.sort();
		}
        else 
        {
            if (value.toInt() == Qt::Checked) files << filePath(index);
            else files.removeAll(filePath(index));
			files.sort();
            qDebug() << files;
        }
		return true;
	}
	return QFileSystemModel::setData(index, value, role);
}


bool FileSourceModel::addPath(QModelIndex index)
{
    directories << filePath(index);
    if (hasChildren(index))
    {
        for (int i = 0; i < rowCount(index); i++)
        {
            QModelIndex child = index.child(i,0);
            if (child.isValid())
            {
//                Need dir check here and func below
                directories << filePath(child);
                emit dataChanged(index, child);
            }
        }
    }
    emit dataChanged(index, index.parent());
}

bool FileSourceModel::removePath(QModelIndex index)
{
    directories.removeAll(filePath(index));
    if (hasChildren(index))
    {
        for (int i = 0; i < rowCount(index); i++)
        {
            QModelIndex child = index.child(i,0);
            if (child.isValid())
            {
                directories.removeAll(filePath(child));
                emit dataChanged(index, child);
                removePath(child);
            }
        }
    }
    emit dataChanged(index, index.parent());
}
