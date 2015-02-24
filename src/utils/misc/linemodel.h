#ifndef LINEMODEL_H
#define LINEMODEL_H

#include "line.h"
//class LineView : public QTableView
//{
//    Q_OBJECT
//public:
//    explicit LineView(QWidget *parent = 0);
//};

#include <QAbstractTableModel>

class LineModel : public QAbstractTableModel
{
    Q_OBJECT
    
public:
    explicit LineModel(QWidget *parent = 0);
    
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex & index) const;
         
public slots:
    void setLines(QList<Line> * list);
    void removeMarkedLine();
    
    
private:
    QList<Line> * lines;
    
signals:
    void linesChanged(QList<Line> lines);
};

#endif
