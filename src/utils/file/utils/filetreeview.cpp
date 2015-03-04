#include "filetreeview.h"

#include <QDebug>
#include <QMap>

FileTreeView::FileTreeView(QWidget * parent) :
    QTreeView(parent)
{
    connect(this, SIGNAL(expanded(const QModelIndex &)), this, SLOT(itemChanged(const QModelIndex &)));
    connect(this, SIGNAL(collapsed(const QModelIndex &)), this, SLOT(itemChanged(const QModelIndex &)));
    connect(this, SIGNAL(clicked(const QModelIndex &)), this, SLOT(itemChanged(const QModelIndex &)));
    connect(this, SIGNAL(activated(const QModelIndex &)), this, SLOT(itemChanged(const QModelIndex &)));
}

void FileTreeView::itemChanged(const QModelIndex &item)
{
    if (item.column() == 0)
    {
        resizeColumnToContents(item.column());
    }

    QFileSystemModel * mod = qobject_cast<QFileSystemModel *> (this->model());

    if ((item.isValid()) &&  mod->fileInfo(item).isFile() && mod->fileInfo(item).isReadable() && mod->fileInfo(item).exists())
    {
        emit fileChanged(mod->filePath(item));
    }
}


FileSelectionModel::FileSelectionModel(QWidget * parent) :
    QFileSystemModel(parent)
{
    setReadOnly(true);
    setNameFilterDisables(false);
    setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files | QDir::Drives);
}

int FileSelectionModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return QFileSystemModel::columnCount();
}

void FileSelectionModel::setStringFilter(QString str)
{
    setNameFilters(str.split(",", QString::SkipEmptyParts));
}

Qt::ItemFlags FileSelectionModel::flags(const QModelIndex &index) const
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
QVariant FileSelectionModel::data(const QModelIndex &index, int role) const
{
    // Return a state depending on role
    if ((index.column() == 0) && (role == Qt::CheckStateRole))
    {
        return checklist.contains(index) ? Qt::Checked : Qt::Unchecked;
    }

    return QFileSystemModel::data(index, role);
}

/* This function is called whenever the data of an index is changed.
 * Emits the signal dataChanged() which calls data().
 * */
bool FileSelectionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // Set data depending on role and corresponding value
    if (role == Qt::CheckStateRole)
    {
        if (value == Qt::Checked)
        {
            addIndex(index);
        }
        else
        {
            removeIndex(index);
        }

        emit dataChanged(index, index);
        return true;
    }

    return QFileSystemModel::setData(index, value, role);
}

void FileSelectionModel::addIndex(QModelIndex index)
{
    checklist.insert(index);

    if (hasChildren(index))
    {
        for (int i = 0; i < rowCount(index); i++)
        {
            QModelIndex child = index.child(i, 0);

            if (child.isValid())
            {
                addIndex(child);
            }
        }
    }

    emit dataChanged(index.parent(), index.child(rowCount(index) - 1, 0));
}

void FileSelectionModel::removeIndex(QModelIndex index)
{
    checklist.remove(index);

    if (hasChildren(index))
    {
        for (int i = 0; i < rowCount(index); i++)
        {
            QModelIndex child = index.child(i, 0);

            if (child.isValid())
            {
                removeIndex(child);
            }
        }
    }

    emit dataChanged(index.parent(), index.child(rowCount(index) - 1, 0));
}

void FileSelectionModel::removeFile(QString path)
{
    QPersistentModelIndex persistent_index = index(path);
    checklist.remove(persistent_index);
}

QMap<QString, QStringList>  FileSelectionModel::getPaths()
{
    QMap<QString, QStringList> paths;

    foreach (const QPersistentModelIndex &value, checklist)
    {
        QModelIndex index = value;

        if (index.isValid() && fileInfo(index).isFile())
        {
            paths[fileInfo(index).dir().absolutePath()] << filePath(index);
        }
    }

    QMap<QString, QStringList>::const_iterator i = paths.constBegin();

    while (i != paths.constEnd())
    {
        paths[i.key()].sort();
        ++i;
    }

    return paths;
}


