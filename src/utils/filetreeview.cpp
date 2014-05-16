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

    QFileSystemModel mod;

    if ((item.isValid()) &&  mod.fileInfo(item).isFile() && mod.fileInfo(item).isReadable() && mod.fileInfo(item).exists()) emit fileChanged(mod.filePath(item));

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
    Q_UNUSED(parent);
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

/* This function is used to set the data of an index, such as wether it should be
 * shown as marked or not (Qt::CheckStateRole)
 * */
QVariant FileSelectionModel::data(const QModelIndex& index, int role) const
{
    // Return a state depending on role
    if (index.isValid() && (index.column() == 0) && (role == Qt::CheckStateRole))
    {
        QVariant state;

        // If the index is not a file and has been added to other_paths
        if (other_paths.contains(filePath(index))) state = Qt::Checked;
        // Else if the index is a file and has been added to file paths
        else if (file_paths.contains(filePath(index))) state = Qt::Checked;
        else state = Qt::Unchecked;
        
        return state;
    }

    else return QFileSystemModel::data(index, role);
}

/* This function is called whenever the data of an index is changed.
 * Emits the signal dataChanged() which calls data().
 * */
bool FileSelectionModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    // Set data depending on role and corresponding value
    if ((index.isValid()) && (index.column() == 0) && (role == Qt::CheckStateRole))
    {
        // Store if checked, remove if unchecked
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


void FileSelectionModel::addIndex(QModelIndex index)
{
    if (fileInfo(index).isFile() && fileInfo(index).isReadable() && fileInfo(index).exists())
    {
        file_paths << filePath(index);
        file_paths.removeDuplicates();
    }
    else
    {
        other_paths << filePath(index);
        other_paths.removeDuplicates();
    }

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

void FileSelectionModel::removeIndex(QModelIndex index)
{
    file_paths.removeAll(filePath(index));
    other_paths.removeAll(filePath(index));

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
    return file_paths;
}

void FileSelectionModel::removeFile(QString path)
{
    file_paths.removeAll(path);

    QModelIndex ix = index(0,0);
    updateAll(ix);
}

void FileSelectionModel::updateAll(QModelIndex &index)
{
    emit dataChanged(index, index);

    if (hasChildren(index))
    {
        for (int i = 0; i < rowCount(index); i++)
        {
            QModelIndex child = index.child(i,0);
            if (child.isValid())
            {
                updateAll(child);
            }
        }
    }
}
