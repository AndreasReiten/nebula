#ifndef FILEBROWSERWIDGET_H
#define FILEBROWSERWIDGET_H

#include <QMainWindow>
#include <QSqlQuery>
#include <QMap>
#include "file/filetreeview.h"
#include "sql/customsqlquerymodel.h"
#include "misc/texthighlighter.h"

namespace Ui {
class FileBrowserWidget;
}

class FileBrowserWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit FileBrowserWidget(QSqlDatabase db, QWidget *parent = 0);
    ~FileBrowserWidget();

public slots:
    void querySelectionModel(QString str);
    void setHeader(QString path);
    void sortItems(int column, Qt::SortOrder order);

private slots:
    void on_addFilesButton_clicked();
    void on_clearButton_clicked();
    void on_execQueryButton_clicked();
    void on_reconstructButton_clicked();

private:
    Ui::FileBrowserWidget *ui;

    FileSelectionModel * fileTreeModel;
    CustomSqlQueryModel * selectionModel;

    Highlighter * headerHighlighter;

    void initSql();
    QSqlDatabase p_db;
    QString display_query;
    QSqlQuery * upsert_file_query;

    CustomSqlQueryModel * selection_model;

    void loadSettings();
    void writeSettings();

    QMap<QString,QPair<int, QString>> column_map;
};

#endif // FILEBROWSERWIDGET_H
