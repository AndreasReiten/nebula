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

QVariant FileSelectionModel::data(const QModelIndex& index, int role) const
{
    // Returns state depending on role (for example, is this index checked or not?)
    if (index.isValid() && (index.column() == 0) && (role == Qt::CheckStateRole))
    {
        QVariant state;
        if (other.contains(filePath(index)))
        {
//            if (rowCount(index))
//            {
                int checked_files = 0;
                int checked_other = 0;
                int total_files = 0;
                int total_other = 0;
                for (int i = 0; i < rowCount(index); i++)
                {
                    QModelIndex child = index.child(i,0);

                    if (fileInfo(child).isFile() && fileInfo(child).isReadable() && fileInfo(child).exists())
                    {
                        if (child.data(Qt::CheckStateRole) == Qt::Checked) checked_files++;
                        total_files++;
                    }
                    else
                    {
                        if (child.data(Qt::CheckStateRole) == Qt::Checked) checked_other++;
                        total_other++;
                    }
                }
                if (checked_other + checked_files == 0) state = Qt::Unchecked;
                else if ((checked_files == total_files) && (checked_files > 0)) state = Qt::Checked;
                else state = Qt::PartiallyChecked;
//            }
        }
        else if (paths.contains(filePath(index))) state = Qt::Checked;
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
    if (fileInfo(index).isFile() && fileInfo(index).isReadable() && fileInfo(index).exists()) paths << filePath(index);
    else other << filePath(index);

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

//Do not maintain marked. Populate it on demand and use only locally. Also add a function to removeIndex based on path

void FileSelectionModel::removeIndex(QModelIndex index)
{
    paths.removeAll(filePath(index)); // If the index of an item changes it cannot be removed. This can occur if the number of items in a folder change.
    other.removeAll(filePath(index));

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
//    paths.clear();

//    QModelIndex ix = index(0,0);

//    findMarkedIndices(ix);

    return paths;
}

void FileSelectionModel::removeFile(QString path)
{
    paths.removeAll(path);

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


// A stricktly speaking unneccesary function
//void FileSelectionModel::findMarkedIndices(QModelIndex &index)
//{
//    QMap<int, QVariant> m = itemData(index);

//    if ((m[Qt::CheckStateRole] == Qt::Checked) && fileInfo(index).isFile() && fileInfo(index).isReadable() && fileInfo(index).exists()) paths << filePath(index);

//    if (hasChildren(index))
//    {
//        for (int i = 0; i < rowCount(index); i++)
//        {
//            QModelIndex child = index.child(i,0);
//            if (child.isValid())
//            {
//                findMarkedIndices(child);
//            }
//        }
//    }
//}

//void FileSelectionModel::refreshFiles()
//{
//    files.clear();
//    for (int i = 0; i < marked.size(); i++)
//    {
//        if (fileInfo(marked.at(i)).isFile() && fileInfo(marked.at(i)).isReadable() && fileInfo(marked.at(i)).exists()) files << filePath(marked.at(i));
//    }
//}
