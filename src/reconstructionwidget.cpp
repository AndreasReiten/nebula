#include "reconstructionwidget.h"
#include "ui_reconstructionwidget.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlError>

ReconstructionWidget::ReconstructionWidget(QWidget *parent) :
    QMainWindow(parent),
    p_ui(new Ui::ReconstructionWidget),
    p_current_row(0)
{
    p_ui->setupUi(this);

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


    loadSettings();

    p_ui->fileSqlView->resizeColumnsToContents();

    //

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
//    ui->imageOpenGLWidget->setReducedPixels(&reduced_pixels);
    connect(p_ui->omegaSpinBox, SIGNAL(valueChanged(double)), p_ui->imageOpenGLWidget, SLOT(setOffsetOmega(double)));
    connect(p_ui->kappaSpinBox, SIGNAL(valueChanged(double)), p_ui->imageOpenGLWidget, SLOT(setOffsetKappa(double)));
    connect(p_ui->phiSpinBox, SIGNAL(valueChanged(double)), p_ui->imageOpenGLWidget, SLOT(setOffsetPhi(double)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(message(QString, int)), p_ui->reconstructionStatusBar, SLOT(showMessage(QString,int)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(message(QString)), p_ui->reconstructionStatusBar, SLOT(showMessage(QString)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(changedMemoryUsage(int)), p_ui->progressBar, SLOT(setValue(int)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(changedFormatMemoryUsage(QString)), this, SLOT(setProgressBarFormat(QString)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(changedFormatMemoryUsage(QString)), this, SLOT(setProgressBarFormat_2(QString)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(changedRangeMemoryUsage(int, int)), p_ui->progressBar, SLOT(setRange(int, int)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(progressTaskActive(bool)), p_ui->progressBar, SLOT(setVisible(bool)));
    connect(p_ui->reconstructButton, SIGNAL(clicked()), p_ui->imageOpenGLWidget, SLOT(reconstruct()));
//    connect(killButton, SIGNAL(clicked()), ui->imageOpenGLWidget, SLOT(killProcess()), Qt::DirectConnection);
//    connect(ui->imageOpenGLWidget->worker(), SIGNAL(pathChanged(QString)), this, SLOT(setHeader(QString)));
//    connect(ui->imageOpenGLWidget->worker(), SIGNAL(pathChanged(QString)), this, SLOT(setGeneralProgressFormat(QString)));
    connect(p_ui->imageOpenGLWidget->worker(), SIGNAL(progressRangeChanged(int, int)), p_ui->progressBar_2, SLOT(setRange(int, int)));
    connect(p_ui->imageOpenGLWidget->worker(), SIGNAL(progressChanged(int)), p_ui->progressBar_2, SLOT(setValue(int)));
    connect(p_ui->lorentzCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setCorrectionLorentz(bool)));
    connect(p_ui->viewModeComboBox, SIGNAL(currentIndexChanged(int)), p_ui->imageOpenGLWidget, SLOT(setMode(int)));
//    connect(p_ui->nextSeriesButton, SIGNAL(clicked()), p_ui->imageOpenGLWidget, SLOT(nextSeries()));
//    connect(p_ui->prevSeriesButton, SIGNAL(clicked()), p_ui->imageOpenGLWidget, SLOT(prevSeries()));
//    connect(p_ui->deactivateFileButton, SIGNAL(clicked()), p_ui->imageOpenGLWidget, SLOT(removeCurrentImage()));
    connect(this, SIGNAL(setPlaneMarkers(QString)), p_ui->imageOpenGLWidget, SLOT(applyPlaneMarker(QString)));
    connect(this, SIGNAL(analyze(QString)), p_ui->imageOpenGLWidget, SLOT(analyze(QString)));
//    connect(ui->imageOpenGLWidget, SIGNAL(pathChanged(QString)), this, SLOT(setHeader(QString)));
//    connect(ui->imageOpenGLWidget, SIGNAL(pathChanged(QString)), this, SLOT(setGeneralProgressFormat(QString)));
//    connect(ui->imageSpinBox, SIGNAL(valueChanged(int)), ui->imageOpenGLWidget, SLOT(setFrameByIndex(int)));
//    connect(ui->imageOpenGLWidget, SIGNAL(imageRangeChanged(int, int)), this, SLOT(setImageRange(int, int)));
//    connect(ui->imageOpenGLWidget, SIGNAL(currentIndexChanged(int)), imageSpinBox, SLOT(setValue(int)));
//    connect(this, SIGNAL(setChanged(SeriesSet)), ui->imageOpenGLWidget, SLOT(setSet(SeriesSet)));
    connect(p_ui->traceButton, SIGNAL(clicked()), p_ui->imageOpenGLWidget, SLOT(traceSeriesSlot()));
    connect(p_ui->bgSampleSpinBox, SIGNAL(valueChanged(int)), p_ui->imageOpenGLWidget, SLOT(setLsqSamples(int)));
    connect(p_ui->showTraceCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(showTraceTexture(bool)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(progressChanged(int)), p_ui->progressBar_2, SLOT(setValue(int)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(progressRangeChanged(int, int)), p_ui->progressBar_2, SLOT(setRange(int, int)));
    connect(p_ui->flatBgCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setCorrectionNoise(bool)));
    connect(p_ui->planarBgCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setCorrectionPlane(bool)));
//    connect(correctionClutterCheckBox, SIGNAL(toggled(bool)), ui->imageOpenGLWidget, SLOT(setCorrectionClutter(bool)));
//    connect(correctionMedianCheckBox, SIGNAL(toggled(bool)), ui->imageOpenGLWidget, SLOT(setCorrectionMedian(bool)));
    connect(p_ui->polarizationCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setCorrectionPolarization(bool)));
    connect(p_ui->fluxCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setCorrectionFlux(bool)));
    connect(p_ui->expTimeCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setCorrectionExposure(bool)));
    connect(p_ui->pixelProjectionCheckBox, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(setCorrectionPixelProjection(bool)));
//    connect(centerImageAction, SIGNAL(triggered()), ui->imageOpenGLWidget, SLOT(centerImage()));
    connect(p_ui->actionWeightcenter, SIGNAL(toggled(bool)), p_ui->imageOpenGLWidget, SLOT(showWeightCenter(bool)));
    connect(p_ui->flatBgSpinBox, SIGNAL(valueChanged(double)), p_ui->imageOpenGLWidget, SLOT(setNoise(double)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(noiseLowChanged(double)), p_ui->flatBgSpinBox, SLOT(setValue(double)));
    connect(this, SIGNAL(saveImage(QString)), p_ui->imageOpenGLWidget, SLOT(saveImage(QString)));
    connect(this, SIGNAL(takeImageScreenshot(QString)), p_ui->imageOpenGLWidget, SLOT(takeScreenShot(QString)));
//    connect(ui->imageOpenGLWidget, SIGNAL(resultFinished(QString)), outputPlainTextEdit, SLOT(setPlainText(QString)));
//    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(setTab(int)));
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
//    connect(p_ui->fileSqlView, SIGNAL(activated(QModelIndex)), this, SLOT(itemClicked(QModelIndex)));
//    connect(p_ui->fileSqlView, SIGNAL(entered(QModelIndex)), this, SLOT(itemClicked(QModelIndex)));
    connect(p_ui->nextImageButton, SIGNAL(clicked()), this, SLOT(next()));
    connect(p_ui->nextImageBatchButton, SIGNAL(clicked()), this, SLOT(batchNext()));
    connect(p_ui->prevImageButton, SIGNAL(clicked()), this, SLOT(previous()));
    connect(p_ui->prevImageBatchButton, SIGNAL(clicked()), this, SLOT(batchPrevious()));
    connect(p_ui->fileSqlView, SIGNAL(clicked(QModelIndex)), selection_model, SLOT(indexChanged(QModelIndex)));
    connect(this, SIGNAL(fileChanged(QString)), p_ui->imageOpenGLWidget, SLOT(setFilePath(QString)));
    connect(p_ui->actionCenter, SIGNAL(triggered()), p_ui->imageOpenGLWidget, SLOT(centerCurrentImage()));

    //### voxelizeWorker ###
    voxelizeThread = new QThread;
    voxelizeWorker = new VoxelizeWorker();
    voxelizeWorker->moveToThread(voxelizeThread);
//    voxelizeWorker->setSVOFile(&svo_inprocess);
//    voxelizeWorker->setReducedPixels(&reduced_pixels);
    voxelizeWorker->initializeCLKernel();

    voxelizeWorker->moveToThread(voxelizeThread);
//    connect(ui->treeLevelSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setCurrentSvoLevel(int)));
    connect(voxelizeThread, SIGNAL(started()), voxelizeWorker, SLOT(process()));
    connect(voxelizeWorker, SIGNAL(finished()), voxelizeThread, SLOT(quit()));
    connect(voxelizeWorker, SIGNAL(popup(QString, QString)), this, SLOT(displayPopup(QString, QString)));
    connect(voxelizeWorker, SIGNAL(changedMemoryUsage(int)), p_ui->progressBar, SLOT(setValue(int)));
    connect(voxelizeWorker, SIGNAL(message(QString)), p_ui->reconstructionStatusBar, SLOT(showMessage(QString)));
    connect(voxelizeWorker, SIGNAL(message(QString, int)), p_ui->reconstructionStatusBar, SLOT(showMessage(QString,int)));
    connect(voxelizeWorker, SIGNAL(changedFormatMemoryUsage(QString)), this, SLOT(setProgressBarFormat(QString)));
    connect(voxelizeWorker, SIGNAL(changedRangeMemoryUsage(int, int)), p_ui->progressBar, SLOT(setRange(int, int)));
    connect(voxelizeWorker, SIGNAL(changedGenericProgress(int)), p_ui->progressBar_2, SLOT(setValue(int)));
    connect(voxelizeWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setProgressBarFormat_2(QString)));
    connect(voxelizeWorker, SIGNAL(changedRangeGenericProcess(int, int)), p_ui->progressBar_2, SLOT(setRange(int, int)));
    connect(p_ui->imageOpenGLWidget, SIGNAL(qSpaceInfoChanged(float, float, float)), voxelizeWorker, SLOT(setQSpaceInfo(float, float, float)));
    connect(p_ui->generateTreeButton, SIGNAL(clicked()), voxelizeThread, SLOT(start()));
    connect(voxelizeWorker, SIGNAL(progressTaskActive(bool)), p_ui->progressBar, SLOT(setVisible(bool)));
//    connect(killButton, SIGNAL(clicked()), voxelizeWorker, SLOT(killProcess()), Qt::DirectConnection);
    connect(p_ui->selectionComboBox, SIGNAL(currentIndexChanged(QString)), p_ui->imageOpenGLWidget, SLOT(setApplicationMode(QString)));
    connect(p_ui->applySelectionButton, SIGNAL(clicked(bool)), p_ui->imageOpenGLWidget, SLOT(applySelection()));

    /////////////////////////
    loadSettings();
//    p_ui->imageOpenGLWidget->centerCurrentImage();
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
//    qDebug() <<

//    if (p_ui->fileSqlView->selectionModel()->selectedIndexes().isEmpty()) return;
//    QModelIndex index = p_ui->fileSqlView->selectionModel()->selectedIndexes().first();
    QString fpath = current.sibling(current.row(), 0).data().toString();
    p_current_row = current.row();
//    QStringList paths;




//    qDebug() << a.row() << b.row();

//    emit message(QString::number(selected.indexes().size()) + " " + QString::number(deselected.indexes().size())+ " " + QString::number(p_ui->fileSqlView->selectionModel()->selectedIndexes().size()));
//    emit message(QString::number(a.row()) +" "+ QString::number(b.row()) +" "+ QString::number(p_ui->fileSqlView->selectionModel()->selectedIndexes().size()));

//    for


//    p_current_row = a.row();
//    QString fpath = a.sibling(a.row(), 0).data(Qt::DisplayRole).toString();
//    if (p_current_file.filePath() != fpath)
//    {
//        p_current_file = QFileInfo(fpath);

        emit fileChanged(fpath);
//    }
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
//    if (QSqlDatabase::database().isOpen()) p_db.close();
//    p_db = QSqlDatabase::addDatabase("QSQLITE");

//    if (p_db.open())
//    {
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
//    }
//    else
//    {
//        qDebug() << "Database error" << QSqlDatabase::database().lastError();
//    }

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

void ReconstructionWidget::setProgressBarFormat_2(QString str)
{
    p_ui->progressBar_2->setFormat(str);
}

//void ReconstructionWidget::applyAnalytics()
//{
//    emit analyze(p_action_apply_mode);
//}

//void ReconstructionWidget::applyPlaneMarker()
//{
//    emit setPlaneMarkers(p_action_apply_mode);
//}

//void ReconstructionWidget::applySelection()
//{
//    emit setSelection(p_action_apply_mode);
//}

//void ReconstructionWidget::setApplyMode(QString str)
//{
//    p_action_apply_mode = str;
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
//    msgBox.setInformativeText("This action is irreversible.");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    if (ret == QMessageBox::Yes)
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

    QSqlDatabase::database().transaction();

    foreach (const QString &value, paths)
    {
        // If valid file
        QFileInfo info(value);

        if (info.isFile())
        {
            // Add to relevant sql database. Guess database based on extension
            upsert_file_query->bindValue(":FilePath", info.filePath());
            upsert_file_query->bindValue(":Path", info.path());
            upsert_file_query->bindValue(":File", info.fileName());

            DetectorFile file(info.filePath());

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
    }

    QSqlDatabase::database().commit();

    querySelectionModel(display_query);

    p_ui->fileSqlView->resizeColumnsToContents();
}
