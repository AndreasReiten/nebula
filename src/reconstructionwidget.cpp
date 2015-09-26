#include "reconstructionwidget.h"
#include "ui_reconstructionwidget.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QThreadPool>

ReconstructionWidget::ReconstructionWidget(QWidget *parent) :
    QMainWindow(parent),
    p_ui(new Ui::ReconstructionWidget),
    p_current_row(0)
{
    p_ui->setupUi(this);
    p_ui->progressBar->hide();

    // Prepare column to sql table translation map
    column_map["Path"] = QPair<int,QString>(0, "FilePath");
    column_map["Path"] = QPair<int,QString>(1, "Path");
    column_map["File"] = QPair<int,QString>(2, "File");
    column_map["*"] = QPair<int,QString>(3, "Active");
    column_map["ω"] = QPair<int,QString>(4, "Omega");
    column_map["κ"] = QPair<int,QString>(5, "Kappa");
    column_map["φ"] = QPair<int,QString>(6, "Phi");
    column_map["Start angle"] = QPair<int,QString>(7, "StartAngle");
    column_map["Increment"] = QPair<int,QString>(8, "AngleIncrement");
    column_map["DDist"] = QPair<int,QString>(9, "DetectorDistance");
    column_map["Beam x"] = QPair<int,QString>(10, "BeamX");
    column_map["Beam y"] = QPair<int,QString>(11, "BeamY");
    column_map["Flux"] = QPair<int,QString>(12, "Flux");
    column_map["T exp"] = QPair<int,QString>(13, "ExposureTime");
    column_map["λ"] = QPair<int,QString>(14, "Wavelength");
    column_map["Detector"] = QPair<int,QString>(15, "Detector");
    column_map["Px size x"] = QPair<int,QString>(16, "PixelSizeX");
    column_map["Px size y"] = QPair<int,QString>(17, "PixelSizeY");


    // Prep file browser
    fileTreeModel  = new FileSelectionModel;
    fileTreeModel->setRootPath(QDir::rootPath());
    p_ui->fileTreeView->setModel(fileTreeModel);
    p_ui->fileTreeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(p_ui->filterLineEdit, SIGNAL(textChanged(QString)), fileTreeModel, SLOT(setStringFilter(QString)));

    // Open database and initialize tables
    initSql();

    // Prep file filter
    connect(p_ui->filterLineEdit, SIGNAL(textChanged(QString)), fileTreeModel, SLOT(setStringFilter(QString)));
    p_ui->filterLineEdit->setText("*.cbf");

    // Init sql query model/view
    selection_model = new CustomSqlQueryModel(p_ui->fileSqlView);
    selection_model->setQuery(display_query, QSqlDatabase::database());

    QMapIterator<QString, QPair<int,QString>> i(column_map);
    while (i.hasNext())
    {
        i.next();
        selection_model->setHeaderData(i.value().first, Qt::Horizontal, i.key());
    }

    p_ui->fileSqlView->horizontalHeader()->setSortIndicatorShown(true);
    p_ui->fileSqlView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    p_ui->fileSqlView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    p_ui->fileSqlView->setModel(selection_model);
    connect(p_ui->fileSqlView->horizontalHeader(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(sortItems(int,Qt::SortOrder)));

    p_ui->fileSqlView->setColumnHidden(0, true);
    p_ui->fileSqlView->setColumnHidden(3, true);
    p_ui->fileSqlView->resizeColumnsToContents();

    QSurfaceFormat format_gl;
    format_gl.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format_gl.setSwapInterval(1);
    format_gl.setSamples(16);
    format_gl.setRedBufferSize(8);
    format_gl.setGreenBufferSize(8);
    format_gl.setBlueBufferSize(8);
    format_gl.setAlphaBufferSize(8);

    p_ui->imageOpenGLWidget->setFormat(format_gl);
    p_ui->imageOpenGLWidget->setMouseTracking(true);

    ////////////////////
    connect(p_ui->rgbComboBox, SIGNAL(currentTextChanged(QString)), p_ui->imageOpenGLWidget, SLOT(setRgb(QString)));
    connect(p_ui->alphaComboBox, SIGNAL(currentIndexChanged(QString)), p_ui->imageOpenGLWidget, SLOT(setAlpha(QString)));
    connect(p_ui->dataMinSpinBox, SIGNAL(valueChanged(double)), p_ui->imageOpenGLWidget, SLOT(setDataMin(double)));
    connect(p_ui->dataMaxSpinBox, SIGNAL(valueChanged(double)), p_ui->imageOpenGLWidget, SLOT(setDataMax(double)));
    connect(p_ui->logCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setLog(bool)));
    connect(p_ui->omegaSpinBox, SIGNAL(valueChanged(double)), p_ui->imageOpenGLWidget, SLOT(setOffsetOmega(double)));
    connect(p_ui->kappaSpinBox, SIGNAL(valueChanged(double)), p_ui->imageOpenGLWidget, SLOT(setOffsetKappa(double)));
    connect(p_ui->phiSpinBox, SIGNAL(valueChanged(double)), p_ui->imageOpenGLWidget, SLOT(setOffsetPhi(double)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(message(QString, int)), p_ui->reconstructionStatusBar, SLOT(showMessage(QString,int)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(message(QString)), p_ui->reconstructionStatusBar, SLOT(showMessage(QString)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(progressTaskActive(bool)), p_ui->progressBar, SLOT(setVisible(bool)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(progressTaskActive(bool)), p_ui->reconstructButton, SLOT(setDisabled(bool)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(progressChanged(int)), p_ui->progressBar, SLOT(setValue(int)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(progressRangeChanged(int, int)), p_ui->progressBar, SLOT(setRange(int, int)));
    connect(p_ui->lorentzCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setCorrectionLorentz(bool)));
    connect(p_ui->viewModeComboBox, SIGNAL(currentIndexChanged(int)), p_ui->imageOpenGLWidget, SLOT(setMode(int)));
    connect(this, SIGNAL(setPlaneMarkers(QString)), p_ui->imageOpenGLWidget, SLOT(applyPlaneMarker(QString)));
    connect(this, SIGNAL(analyze(QString)), p_ui->imageOpenGLWidget, SLOT(analyze(QString)));
    connect(p_ui->traceButton, SIGNAL(clicked()), p_ui->imageOpenGLWidget, SLOT(traceSeriesSlot()));
    connect(p_ui->bgSampleSpinBox, SIGNAL(valueChanged(int)), p_ui->imageOpenGLWidget, SLOT(setLsqSamples(int)));
    connect(p_ui->showTraceCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(showTraceTexture(bool)));
    connect(p_ui->flatBgCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setCorrectionNoise(bool)));
    connect(p_ui->planarBgCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setCorrectionPlane(bool)));
    connect(p_ui->polarizationCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setCorrectionPolarization(bool)));
    connect(p_ui->fluxCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setCorrectionFlux(bool)));
    connect(p_ui->expTimeCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setCorrectionExposure(bool)));
    connect(p_ui->pixelProjectionCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setCorrectionPixelProjection(bool)));
    connect(p_ui->actionWeightcenter, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(showWeightCenter(bool)));
    connect(p_ui->flatBgSpinBox, SIGNAL(valueChanged(double)), p_ui->imageOpenGLWidget, SLOT(setNoise(double)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(noiseLowChanged(double)), p_ui->flatBgSpinBox, SLOT(setValue(double)));
    connect(this, SIGNAL(saveImage(QString)), p_ui->imageOpenGLWidget, SLOT(saveImage(QString)));
    connect(this, SIGNAL(takeImageScreenshot(QString)), p_ui->imageOpenGLWidget, SLOT(takeScreenShot(QString)));
    connect(p_ui->beamCenterCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setBeamOverrideActive(bool)));
    connect(p_ui->xCenterSpinBox, SIGNAL(valueChanged(double)), p_ui->imageOpenGLWidget, SLOT(setBeamXOverride(double)));
    connect(p_ui->yCenterSpinBox, SIGNAL(valueChanged(double)), p_ui->imageOpenGLWidget, SLOT(setBeamYOverride(double)));
    connect(p_ui->actionEwaldCircle, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(showEwaldCircle(bool)));
    connect(p_ui->actionTooltip, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(showImageTooltip(bool)));
    connect(p_ui->actionSave, SIGNAL(triggered()), this, SLOT(saveProject()));
    connect(p_ui->actionOpen, SIGNAL(triggered()), this, SLOT(loadProject()));
    connect(p_ui->actionImageScreenshot, SIGNAL(triggered()), this, SLOT(saveImageFunction()));
    connect(p_ui->actionFrameScreenshot, SIGNAL(triggered()), this, SLOT(takeImageScreenshotFunction()));
    connect(this, SIGNAL(message(QString)), p_ui->reconstructionStatusBar, SLOT(showMessage(QString)));
    connect(this, SIGNAL(message(QString, int)), p_ui->reconstructionStatusBar, SLOT(showMessage(QString,int)));
    connect(p_ui->fileSqlView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(itemSelected(QModelIndex,QModelIndex)));
    connect(p_ui->nextImageButton, SIGNAL(clicked()), this, SLOT(next()));
    connect(p_ui->nextImageBatchButton, SIGNAL(clicked()), this, SLOT(batchNext()));
    connect(p_ui->prevImageButton, SIGNAL(clicked()), this, SLOT(previous()));
    connect(p_ui->prevImageBatchButton, SIGNAL(clicked()), this, SLOT(batchPrevious()));
    connect(p_ui->fileSqlView, SIGNAL(clicked(QModelIndex)), selection_model, SLOT(indexChanged(QModelIndex)));
    connect(this, SIGNAL(fileChanged(QString)), p_ui->imageOpenGLWidget, SLOT(setFilePath(QString)));
    connect(p_ui->actionCenter, SIGNAL(triggered()), p_ui->imageOpenGLWidget, SLOT(centerCurrentImage()));

    connect(p_ui->reconstructButton, SIGNAL(clicked()), this, SLOT(growInterpolationTree_start()));
    connect(this, SIGNAL(growInterpolationTreeProxySignal()), p_ui->imageOpenGLWidget, SLOT(growInterpolationTree()));
    connect(p_ui->imageOpenGLWidget->growInterpolationTreeWatcher(), SIGNAL(progressRangeChanged(int,int)), p_ui->progressBar, SLOT(setRange(int,int)));
    connect(p_ui->imageOpenGLWidget->growInterpolationTreeWatcher(), SIGNAL(progressTextChanged(QString)), p_ui->reconstructionStatusBar, SLOT(showMessage(QString)));
    connect(p_ui->imageOpenGLWidget->growInterpolationTreeWatcher(), SIGNAL(progressValueChanged(int)), p_ui->progressBar, SLOT(setValue(int)));
//    connect(p_ui->imageOpenGLWidget->interpolationTreeWatcher(), SIGNAL(finished()), this, SLOT(growInterpolationTree_finished()));
    connect(p_ui->stopButton, SIGNAL(clicked()), p_ui->imageOpenGLWidget->growInterpolationTreeWatcher(), SLOT(cancel()));
    connect(p_ui->pauseButton, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget->growInterpolationTreeWatcher(), SLOT(setPaused(bool)));

    connect(p_ui->imageOpenGLWidget->fertilizeInterpolationTreeWatcher(), SIGNAL(progressRangeChanged(int,int)), p_ui->progressBar, SLOT(setRange(int,int)));
    connect(p_ui->imageOpenGLWidget->fertilizeInterpolationTreeWatcher(), SIGNAL(progressTextChanged(QString)), p_ui->reconstructionStatusBar, SLOT(showMessage(QString)));
    connect(p_ui->imageOpenGLWidget->fertilizeInterpolationTreeWatcher(), SIGNAL(progressValueChanged(int)), p_ui->progressBar, SLOT(setValue(int)));
    connect(p_ui->imageOpenGLWidget->fertilizeInterpolationTreeWatcher(), SIGNAL(finished()), this, SLOT(interpolationTree_finished()));
    connect(p_ui->stopButton, SIGNAL(clicked()), p_ui->imageOpenGLWidget->fertilizeInterpolationTreeWatcher(), SLOT(cancel()));
    connect(p_ui->pauseButton, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget->fertilizeInterpolationTreeWatcher(), SLOT(setPaused(bool)));

    connect(p_ui->voxelizeButton, SIGNAL(clicked()), this, SLOT(growVoxelTree_start()));
    connect(this, SIGNAL(growVoxelTreeProxySignal()), p_ui->imageOpenGLWidget, SLOT(growVoxelTree()));
    connect(p_ui->imageOpenGLWidget->voxelTreeWatcher(), SIGNAL(progressRangeChanged(int,int)), p_ui->progressBar, SLOT(setRange(int,int)));
    connect(p_ui->imageOpenGLWidget->voxelTreeWatcher(), SIGNAL(progressTextChanged(QString)), p_ui->reconstructionStatusBar, SLOT(showMessage(QString)));
    connect(p_ui->imageOpenGLWidget->voxelTreeWatcher(), SIGNAL(progressValueChanged(int)), p_ui->progressBar, SLOT(setValue(int)));
    connect(p_ui->imageOpenGLWidget->voxelTreeWatcher(), SIGNAL(finished()), this, SLOT(growVoxelTree_finished()));
    connect(p_ui->stopButton, SIGNAL(clicked()), p_ui->imageOpenGLWidget->voxelTreeWatcher(), SLOT(cancel()));
    connect(p_ui->pauseButton, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget->voxelTreeWatcher(), SLOT(setPaused(bool)));

    //### voxelizeWorker ###
    voxelizeThread = new QThread;
    voxelizeWorker = new VoxelizeWorker();
    voxelizeWorker->moveToThread(voxelizeThread);
//    voxelizeWorker->setSVOFile(&svo_inprocess);
//    voxelizeWorker->setReducedPixels(&reduced_pixels);
    voxelizeWorker->initializeCLKernel();

    voxelizeWorker->moveToThread(voxelizeThread);
    connect(voxelizeThread, SIGNAL(started()), voxelizeWorker, SLOT(process()));
    connect(voxelizeWorker, SIGNAL(finished()), voxelizeThread, SLOT(quit()));
    connect(voxelizeWorker, SIGNAL(popup(QString, QString)), this, SLOT(displayPopup(QString, QString)));
    connect(voxelizeWorker, SIGNAL(changedMemoryUsage(int)), p_ui->progressBar, SLOT(setValue(int)));
    connect(voxelizeWorker, SIGNAL(message(QString)), p_ui->reconstructionStatusBar, SLOT(showMessage(QString)));
    connect(voxelizeWorker, SIGNAL(message(QString, int)), p_ui->reconstructionStatusBar, SLOT(showMessage(QString,int)));
    connect(voxelizeWorker, SIGNAL(changedFormatMemoryUsage(QString)), this, SLOT(setProgressBarFormat(QString)));
    connect(voxelizeWorker, SIGNAL(changedRangeMemoryUsage(int, int)), p_ui->progressBar, SLOT(setRange(int, int)));
//    connect(voxelizeWorker, SIGNAL(changedGenericProgress(int)), p_ui->progressBar_2, SLOT(setValue(int)));
//    connect(voxelizeWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setProgressBarFormat_2(QString)));
//    connect(voxelizeWorker, SIGNAL(changedRangeGenericProcess(int, int)), p_ui->progressBar_2, SLOT(setRange(int, int)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(qSpaceInfoChanged(float, float, float)), voxelizeWorker, SLOT(setQSpaceInfo(float, float, float)));
//    connect(p_ui->generateTreeButton, SIGNAL(clicked()), voxelizeThread, SLOT(start()));
    connect(voxelizeWorker, SIGNAL(progressTaskActive(bool)), p_ui->progressBar, SLOT(setVisible(bool)));
    connect(p_ui->selectionComboBox, SIGNAL(currentIndexChanged(QString)), p_ui->imageOpenGLWidget, SLOT(setApplicationMode(QString)));
    connect(p_ui->applySelectionButton, SIGNAL(clicked(bool)), p_ui->imageOpenGLWidget, SLOT(applySelection()));

    loadSettings();
}

void ReconstructionWidget::growInterpolationTree_start()
{
    p_ui->reconstructButton->setDisabled(true);
    p_ui->voxelizeButton->setDisabled(true);
    p_ui->progressBar->show();

    emit growInterpolationTreeProxySignal();
}

void ReconstructionWidget::interpolationTree_finished()
{
    p_ui->reconstructButton->setDisabled(false);
    p_ui->voxelizeButton->setDisabled(false);
    p_ui->progressBar->hide();
    p_ui->progressBar->setValue(0);
}

void ReconstructionWidget::growVoxelTree_start()
{
    p_ui->reconstructButton->setDisabled(true);
    p_ui->voxelizeButton->setDisabled(true);
    p_ui->progressBar->show();

    emit growVoxelTreeProxySignal();
}

void ReconstructionWidget::growVoxelTree_finished()
{
    p_ui->reconstructButton->setDisabled(false);
    p_ui->voxelizeButton->setDisabled(false);
    p_ui->progressBar->hide();
    p_ui->progressBar->setValue(0);
}

void ReconstructionWidget::next()
{
    p_current_row++;
    if(p_current_row < 0) p_current_row = 0;
    if(p_current_row >= selection_model->rowCount()) p_current_row = selection_model->rowCount()-1;
    p_ui->fileSqlView->selectRow(p_current_row);
}

void ReconstructionWidget::previous()
{
    p_current_row--;
    if(p_current_row < 0) p_current_row = 0;
    if(p_current_row >= selection_model->rowCount()) p_current_row = selection_model->rowCount()-1;
    p_ui->fileSqlView->selectRow(p_current_row);
}

void ReconstructionWidget::batchNext()
{
    p_current_row += 10;
    if(p_current_row < 0) p_current_row = 0;
    if(p_current_row >= selection_model->rowCount()) p_current_row = selection_model->rowCount()-1;
    p_ui->fileSqlView->selectRow(p_current_row);
}

void ReconstructionWidget::batchPrevious()
{
    p_current_row -= 10;
    if(p_current_row < 0) p_current_row = 0;
    if(p_current_row >= selection_model->rowCount()) p_current_row = selection_model->rowCount()-1;
    p_ui->fileSqlView->selectRow(p_current_row);
}

void ReconstructionWidget::itemSelected(const QModelIndex & current, const QModelIndex & previous)
{
    QString fpath = current.sibling(current.row(), 0).data().toString();
    p_current_row = current.row();
    emit fileChanged(fpath);
}

void ReconstructionWidget::displayPopup(QString title, QString text)
{
    QMessageBox::warning(this, title, text);
}

void ReconstructionWidget::sortItems(int column,Qt::SortOrder order)
{
    QMap<Qt::SortOrder,QString> order_map;
    order_map[Qt::AscendingOrder] = "ASC";
    order_map[Qt::DescendingOrder] = "DESC";

    display_query = ("SELECT * FROM cbf_table ORDER BY "+
                        column_map[selection_model->headerData(column, Qt::Horizontal).toString()].second+
                        " "+order_map[order]+
                        ", File ASC");

    refreshSelectionModel();
}

void ReconstructionWidget::refreshSelectionModel()
{
    selection_model->setQuery(display_query, QSqlDatabase::database());
    if (selection_model->lastError().isValid())
    {
        qDebug() << selection_model->lastError();
    }
}

void ReconstructionWidget::initSql()
{
    // Database
    QSqlQuery query(QSqlDatabase::database());
    if (!query.exec("CREATE TABLE IF NOT EXISTS cbf_table ("
                    "FilePath TEXT PRIMARY KEY NOT NULL, "
                    "Path TEXT,"
                    "File TEXT,"
                    "Active INTEGER, "
                    "Omega REAL, "
                    "Kappa REAL, "
                    "Phi REAL, "
                    "StartAngle REAL,"
                    "AngleIncrement REAL,"
                    "DetectorDistance REAL,"
                    "BeamX REAL,"
                    "BeamY REAL,"
                    "Flux REAL,"
                    "ExposureTime REAL,"
                    "Wavelength REAL,"
                    "Detector TEXT,"
                    "PixelSizeX REAL,"
                    "PixelSizeY REAL"
                    ");"))
    {
        qDebug() << sqlQueryError(query);
    }

    // Queries
    upsert_file_query = new QSqlQuery(QSqlDatabase::database());
    upsert_file_query->prepare("INSERT OR IGNORE INTO cbf_table (FilePath, Path, File, Active, Omega, Kappa, Phi, StartAngle, AngleIncrement, DetectorDistance, BeamX, BeamY, Flux, ExposureTime, Wavelength, Detector, PixelSizeX, PixelSizeY) VALUES (:FilePath, :Path, :File, :Active, :Omega, :Kappa, :Phi, :StartAngle, :AngleIncrement, :DetectorDistance, :BeamX, :BeamY, :Flux, :ExposureTime, :Wavelength, :Detector, :PixelSizeX, :PixelSizeY);");
    display_query = "SELECT * FROM cbf_table ORDER BY Path ASC, File ASC";
}

void ReconstructionWidget::takeImageScreenshotFunction()
{
    // Move this to imagepreview?
    QString format = "png";
    QDateTime dateTime = dateTime.currentDateTime();
    QString initialPath = p_screenshot_dir + QString("/screenshot_" + dateTime.toString("yyyy_MM_dd_hh_mm_ss")) + "." + format;

    QString file_name = QFileDialog::getSaveFileName(this, "Save as", initialPath,
                        QString("%1 files (*.%2);;All files (*)")
                        .arg(format.toUpper())
                        .arg(format));

    if (file_name != "")
    {
        QFileInfo info(file_name);
        p_screenshot_dir = info.absoluteDir().path();

        emit takeImageScreenshot(file_name);
    }
}

void ReconstructionWidget::saveImageFunction()
{
    // Move this to imagepreview?
    QString format = "png";
    QDateTime dateTime = dateTime.currentDateTime();
    QString initialPath = p_screenshot_dir + QString("/image_" + dateTime.toString("yyyy_MM_dd_hh_mm_ss")) + "." + format;

    QString file_name = QFileDialog::getSaveFileName(this, "Save as", initialPath,
                        QString("%1 files (*.%2);;All files (*)")
                        .arg(format.toUpper())
                        .arg(format));

    if (file_name != "")
    {
        QFileInfo info(file_name);
        p_screenshot_dir = info.absoluteDir().path();

        emit saveImage(file_name);
    }
}


void ReconstructionWidget::setProgressBarFormat(QString str)
{
    p_ui->progressBar->setFormat(str);
}

//void ReconstructionWidget::setProgressBarFormat_2(QString str)
//{
//    p_ui->progressBar_2->setFormat(str);
//}

void ReconstructionWidget::saveProject()
{
    QString file_name = QFileDialog::getSaveFileName(this, "Save project", p_working_dir,"Text files (*.txt);;All files (*)");

    if (file_name != "")
    {
        QFileInfo info(file_name);
        p_working_dir = info.absoluteDir().path();

        QFile file(file_name);

        if (file.open(QIODevice::WriteOnly))
        {
            QDataStream out(&file);
            file.close();
        }
    }
}

void ReconstructionWidget::loadProject()
{
    QString file_name = QFileDialog::getOpenFileName(this, "Open project", p_working_dir,"Text files (*.txt);;All files (*)");

    if (file_name != "")
    {
        QFileInfo info(file_name);
        p_working_dir = info.absoluteDir().path();

        QFile file(file_name);

        if (file.open(QIODevice::ReadOnly))
        {
            QDataStream in(&file);
            file.close();
        }
    }
}

ReconstructionWidget::~ReconstructionWidget()
{
    voxelizeThread->quit();
    voxelizeThread->wait();
//    p_db.close();
    writeSettings();
    delete p_ui;
}

void ReconstructionWidget::loadSettings()
{
//    qDebug() << "loadsettings";
    QSettings settings("settings.ini", QSettings::IniFormat);
    p_working_dir = settings.value("ReconstructionWidget/working_dir", QDir::homePath()).toString();
    p_screenshot_dir = settings.value("ReconstructionWidget/screenshot_dir", QDir::homePath()).toString();
    this->restoreState(settings.value("ReconstructionWidget/state").toByteArray());
    p_ui->splitter->restoreState(settings.value("ReconstructionWidget/splitter/state").toByteArray());
    p_ui->toolBox->setCurrentIndex(settings.value("ReconstructionWidget/toolBox/currentIndex").toInt());
}

void ReconstructionWidget::writeSettings()
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setValue("ReconstructionWidget/working_dir", p_working_dir);
    settings.setValue("ReconstructionWidget/screenshot_dir", p_screenshot_dir);
    settings.setValue("ReconstructionWidget/state", this->saveState());
    settings.setValue("ReconstructionWidget/toolBox/currentIndex", p_ui->toolBox->currentIndex());
    settings.setValue("ReconstructionWidget/splitter/state", this->p_ui->splitter->saveState());
}



void ReconstructionWidget::on_sanityButton_clicked()
{
    QSqlQuery query("SELECT * FROM cbf_table", QSqlDatabase::database());
    if (query.lastError().isValid())
    {
        qDebug() << query.lastError();
    }

    int n_insane_files = 0;
    QFileInfo insane_file;

    emit message("Checking files...");

    while (query.next())
    {
        // If valid file
        QFileInfo info(query.value(0).toString());

        if (!info.isFile())
        {
            n_insane_files++;
            insane_file = info;
            emit message(info.filePath()+" was not found.");
        }
    }

    if (n_insane_files > 0)
    {
        emit message(QString::number(n_insane_files)+" files were not found ("+insane_file.path()+").");
    }
    else
    {
        emit message("All files checked out.");
    }
}

void ReconstructionWidget::on_deactivateFileButton_clicked()
{
    QSqlQuery query(QSqlDatabase::database());
    query.prepare("UPDATE cbf_table SET Active = :Active WHERE FilePath = :FilePath");

    QModelIndexList selected = p_ui->fileSqlView->selectionModel()->selectedIndexes();

    QSqlDatabase::database().transaction();

    for (int i = 0; i < selected.size(); i++)
    {
        query.bindValue(":Active", 0);
        query.bindValue(":FilePath", selected[i].sibling(selected[i].row(), 0).data().toString());

        if (!query.exec()) qDebug() << sqlQueryError(query);
    }
    QSqlDatabase::database().commit();

    refreshSelectionModel();
}

void ReconstructionWidget::on_activateFileButton_clicked()
{
    QSqlQuery query(QSqlDatabase::database());
    query.prepare("UPDATE cbf_table SET Active = :Active WHERE FilePath = :FilePath");

    QModelIndexList selected = p_ui->fileSqlView->selectionModel()->selectedIndexes();

    QSqlDatabase::database().transaction();

    for (int i = 0; i < selected.size(); i++)
    {
        query.bindValue(":Active", 1);
        query.bindValue(":FilePath", selected[i].sibling(selected[i].row(), 0).data().toString());

        if (!query.exec()) qDebug() << sqlQueryError(query);
    }
    QSqlDatabase::database().commit();

    refreshSelectionModel();
}

void ReconstructionWidget::querySelectionModel(QString str)
{
    selection_model->setQuery(str, QSqlDatabase::database());
    if (selection_model->lastError().isValid())
    {
        qDebug() << selection_model->lastError();
        p_ui->reconstructionStatusBar->showMessage(selection_model->lastError().text());
    }
}

void ReconstructionWidget::on_clearButton_clicked()
{
    QMessageBox msgBox;
    msgBox.setText("Remove files?");
    QPushButton * yes_button = msgBox.addButton(trUtf8("Yes, remove files"), QMessageBox::YesRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.exec();

//    qDebug() << ret << QMessageBox::Yes << QMessageBox::No << QMessageBox::YesRole;

    if (msgBox.clickedButton() == yes_button)
    {
        QSqlQuery query(QSqlDatabase::database());
        query.prepare("DELETE FROM cbf_table WHERE FilePath = :FilePath");

        QModelIndexList selected = p_ui->fileSqlView->selectionModel()->selectedIndexes();

        QSqlDatabase::database().transaction();

        for (int i = 0; i < selected.size(); i++)
        {
            query.bindValue(":FilePath", selected[i].sibling(selected[i].row(), 0).data().toString());

            if (!query.exec()) qDebug() << sqlQueryError(query);
        }
        QSqlDatabase::database().commit();

        refreshSelectionModel();
    }
}

void ReconstructionWidget::on_execQueryButton_clicked()
{
    QSqlQuery query(p_ui->sqlTextEdit->toPlainText(), QSqlDatabase::database());
    if (query.lastError().isValid())
    {
        qDebug() << query.lastError();
        p_ui->reconstructionStatusBar->showMessage(query.lastError().text());
    }
}

void ReconstructionWidget::on_addFilesButton_clicked()
{
    QStringList paths(fileTreeModel->selected());

    QList<DetectorFile> files;
    foreach (const QString &path, paths)
    {
        DetectorFile file(path);

        if (file.isValid()) files << file;
    }

    QFutureWatcher<void> future_watcher;
    future_watcher.setFuture(QtConcurrent::map(files, &DetectorFile::readHeader));
    future_watcher.waitForFinished();

    QSqlDatabase::database().transaction();

    foreach (const DetectorFile &file, files)
    {
        // Add to relevant sql database. Guess database based on extension
        upsert_file_query->bindValue(":FilePath", file.filePath());
        upsert_file_query->bindValue(":Path", file.dir());
        upsert_file_query->bindValue(":File", file.fileName());
        upsert_file_query->bindValue(":Active", 1);
        upsert_file_query->bindValue(":Omega", file.omega() * 180.0 / pi);
        upsert_file_query->bindValue(":Kappa", file.kappa() * 180.0 / pi);
        upsert_file_query->bindValue(":Phi", file.phi() * 180.0 / pi);
        upsert_file_query->bindValue(":StartAngle", file.startAngle() * 180.0 / pi);
        upsert_file_query->bindValue(":AngleIncrement", file.angleIncrement() * 180.0 / pi);
        upsert_file_query->bindValue(":DetectorDistance", file.detectorDist());
        upsert_file_query->bindValue(":BeamX", file.beamX());
        upsert_file_query->bindValue(":BeamY", file.beamY());
        upsert_file_query->bindValue(":Flux", file.flux());
        upsert_file_query->bindValue(":ExposureTime", file.expTime());
        upsert_file_query->bindValue(":Wavelength", file.wavelength());
        upsert_file_query->bindValue(":Detector", file.detector());
        upsert_file_query->bindValue(":PixelSizeX", file.pixSizeX());
        upsert_file_query->bindValue(":PixelSizeY", file.pixSizeY());

        if (!upsert_file_query->exec()) qDebug() << sqlQueryError(*upsert_file_query);
    }

    QSqlDatabase::database().commit();

    querySelectionModel(display_query);

    p_ui->fileSqlView->resizeColumnsToContents();
}

void ReconstructionWidget::on_pushButton_clicked()
{
    QElapsedTimer t;

    int ins = 42;

    size_t size = 1000000;

    QVector<int> vec;
    QList<int> list;
    QVector<int> vec_res;
    vec_res.reserve(size);
    std::vector<int> stdvec;
    std::vector<int> stdvec_res;
    stdvec_res.reserve(size);

    t.start();

    for (int i = 0; i < size; i++)
    {
        vec << ins;
    }
    qDebug() << "Vec" << t.elapsed();

    t.restart();
    for (int i = 0; i < size; i++)
    {
        vec_res << ins;
    }
    qDebug() << "Vec reserved" <<  t.elapsed();

    t.restart();
    for (int i = 0; i < size; i++)
    {
        list << ins;
    }
    qDebug() << "List" << t.elapsed();

    t.restart();

    for (int i = 0; i < size; i++)
    {
        stdvec.push_back( ins);
    }
    qDebug() << "Std Vec" << t.elapsed();

    t.restart();
    for (int i = 0; i < size; i++)
    {
        stdvec_res.push_back(ins);
    }
    qDebug() << "Std Vec reserved" <<  t.elapsed();

    emit message("Done!");
}
