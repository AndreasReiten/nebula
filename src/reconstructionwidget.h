#ifndef RECONSTRUCTIONWIDGET_H
#define RECONSTRUCTIONWIDGET_H

#include <QMainWindow>
#include <QFileInfo>

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


public slots:
    void setProgressBarFormat(QString str);
    void setProgressBarFormat_2(QString str);
    void setApplyMode(QString str);
    void applyAnalytics();
    void applyPlaneMarker();
    void applySelection();
    void takeImageScreenshotFunction();
    void saveImageFunction();
    void displayPopup(QString title, QString text);
    void saveProject();
    void loadProject();
    void sortItems(int column, Qt::SortOrder order);
    void refreshSelectionModel();
    void itemClicked(const QModelIndex & index);

private slots:
    void on_sanityButton_clicked();

    void on_deactivateFileButton_clicked();

    void on_activateFileButton_clicked();

private:
    Ui::ReconstructionWidget *p_ui;

    void loadSettings();
    void writeSettings();
    void initSql();

//    QSqlDatabase p_db;
    QString display_query;
    QMap<QString,QPair<int, QString>> column_map;
    CustomSqlQueryModel * selection_model;

    // Workers
    QThread * voxelizeThread;
    VoxelizeWorker * voxelizeWorker;

    QString p_action_apply_mode;
    QString p_working_dir;
    QString p_screenshot_dir;
    QFileInfo p_current_file;
};

#endif // RECONSTRUCTIONWIDGET_H
