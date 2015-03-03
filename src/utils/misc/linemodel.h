#ifndef LINEMODEL_H
#define LINEMODEL_H

#include "line.h"

#include <QAbstractTableModel>
#include <QColor>

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
    void refresh();
    void highlight(QModelIndex index);
    
private:
    QList<Line> * p_lines;
    int p_columns;
    
signals:
    void linesChanged(QList<Line> p_lines);
    void lineChanged(int value);
};

#endif
