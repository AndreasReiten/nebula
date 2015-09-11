#ifndef RECONSTRUCTIONWIDGET_H
#define RECONSTRUCTIONWIDGET_H

#include <QMainWindow>
#include <QFileInfo>
#include <QSqlQuery>
#include <QDebug>
#include <QMap>
#include <QTimer>

#include "file/filetreeview.h"
#include "image/imagepreview.h"
#include "sql/customsqlquerymodel.h"
#include "worker/worker.h"
#include "sql/sqlqol.h"

namespace Ui {
class ReconstructionWidget;
}

class ReconstructionWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit ReconstructionWidget(QWidget *parent = 0);
    ~ReconstructionWidget();

signals:
    void message(QString);
    void message(QString, int);
    void analyze(QString);
    void setPlaneMarkers(QString);
    void setSelection(QString);
    void saveImage(QString);
    void takeImageScreenshot(QString);
    void fileChanged(QString);

    void populateInterpolationTreeProxySignal();


public slots:
    void setProgressBarFormat(QString str);
    void setProgressBarFormat_2(QString str);
    void takeImageScreenshotFunction();
    void saveImageFunction();
    void displayPopup(QString title, QString text);
    void saveProject();
    void loadProject();
    void sortItems(int column, Qt::SortOrder order);
    void refreshSelectionModel();
    void itemSelected(const QModelIndex & current, const QModelIndex & previous);
    void next();
    void previous();
    void batchNext();
    void batchPrevious();
    void clearRunnables();
//    void pollProgress();

    void populateInterpolationTreeProxySlot();

private slots:
    void on_sanityButton_clicked();
    void on_deactivateFileButton_clicked();
    void on_activateFileButton_clicked();
    void on_clearButton_clicked();
    void on_execQueryButton_clicked();
    void on_addFilesButton_clicked();
    void querySelectionModel(QString str);

private:
    Ui::ReconstructionWidget *p_ui;

    void loadSettings();
    void writeSettings();
    void initSql();

    FileSelectionModel * fileTreeModel;

    QSqlQuery * upsert_file_query;
    QString display_query;
    QMap<QString,QPair<int, QString>> column_map;
    CustomSqlQueryModel * selection_model;

    // Workers
    QThread * voxelizeThread;
    VoxelizeWorker * voxelizeWorker;

    QString p_working_dir;
    QString p_screenshot_dir;
    int p_current_row;

//    QTimer * progressPollTimer;
};

#endif // RECONSTRUCTIONWIDGET_H
