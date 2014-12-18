#include "mainwindow.h"

#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

#include <QThread>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QTimer>
#include <QLineEdit>
#include <QGroupBox>
#include <QMessageBox>
#include <QFile>
#include <QToolButton>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QStatusBar>
#include <QSettings>
#include <QTextStream>
#include <QSlider>
#include <QList>
#include <QApplication>

MainWindow::MainWindow() :
    hasPendingChanges(0),
    batch_size(10)
{
    // Set some default values
//    display_file = 0;
    reduced_pixels.set(0,0);
    
    // Set stylesheet
    QFile styleFile( ":/src/stylesheets/plain.qss" );
    styleFile.open( QFile::ReadOnly );
    QString style( styleFile.readAll() );
    styleFile.close();
    this->setStyleSheet(style);

    // Set the OpenCL context
    context_cl = new OpenCLContext;

    // Set the format of the rendering context
    QSurfaceFormat format_gl;
    format_gl.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format_gl.setSwapInterval(1);
    format_gl.setSamples(16);
    format_gl.setRedBufferSize(8);
    format_gl.setGreenBufferSize(8);
    format_gl.setBlueBufferSize(8);
    format_gl.setAlphaBufferSize(8);
    
    sharedContextWindow = new SharedContextWindow();
    sharedContextWindow->setFormat(format_gl);
    sharedContextWindow->setOpenCLContext(context_cl);
    sharedContextWindow->show();

    sharedContextWindow->initializeWorker();
    sharedContextWindow->hide();
    
    
    this->initActions();
    
    this->initMenus();
    
    this->initGUI();
    
    this->initConnects();
    
    this->initWorkers();
    
    
    
    setCentralWidget(mainWidget);
    readSettings();
    print("[Nebula] Welcome to Nebula!");
    setWindowTitle(tr("Nebula[*]"));
    
    // Set start conditions
    setStartConditions();

//    graphicsDockWidget->hide();
//    unitCellDock->hide();
//    functionDockWidget->hide();
//    fileDockWidget->hide();
//    toolChainWidget->show();
//    outputDockWidget->show();

}

MainWindow::~MainWindow()
{
//    setFileThread->quit();
//    setFileThread->wait(1000);
    
//    readFileThread->quit();
//    readFileThread->wait(1000);
    
//    projectFileThread->quit();
//    projectFileThread->wait(1000);
    
    voxelizeThread->quit();
    voxelizeThread->wait(1000);
    
//    reconstructThread->quit();
//    reconstructThread->wait(1000);
    
    displayFileThread->quit();
    displayFileThread->wait(1000);
}

void MainWindow::setCurrentSvoLevel(int value)
{
    svo_inprocess.setLevels(value);
}

void MainWindow::displayPopup(QString title, QString text)
{
    QMessageBox::warning(this, title, text);
}


void MainWindow::initWorkers()
{
//    readScriptThread = new QThread;
//    setFileThread = new QThread;
//    readFileThread = new QThread;
//    projectFileThread = new QThread;
    voxelizeThread = new QThread;
//    reconstructThread = new QThread;
    displayFileThread = new QThread;

    //### setFileWorker ###
//    setFileWorker = new SetFileWorker();
//    setFileWorker->setFilePaths(&file_paths);
//    setFileWorker->setFiles(&files);
//    setFileWorker->setSVOFile(&svo_inprocess);
//    setFileWorker->setOpenCLContext(context_cl);
//    setFileWorker->setSVOFile(&svo_inprocess);

//    setFileWorker->moveToThread(setFileThread);
////    connect(setFileButton, SIGNAL(clicked()), this, SLOT(setFilesFromSelectionModel()));
//    connect(setFileThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
//    connect(setFileWorker, SIGNAL(finished()), this, SLOT(setFileButtonFinish()));
//    connect(setFileThread, SIGNAL(started()), setFileWorker, SLOT(process()));
//    connect(setFileWorker, SIGNAL(abort()), setFileThread, SLOT(quit()));
//    connect(setFileWorker, SIGNAL(finished()), setFileThread, SLOT(quit()));
//    connect(setFileWorker, SIGNAL(changedFile(QString)), this, SLOT(setHeader(QString)));
//    connect(setFileWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
//    connect(setFileWorker, SIGNAL(changedGenericProgress(int)), genericProgressBar, SLOT(setValue(int)));
//    connect(setFileWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
//    connect(setFileButton, SIGNAL(clicked()), setFileThread, SLOT(start()));
//    connect(killButton, SIGNAL(clicked()), setFileWorker, SLOT(killProcess()), Qt::DirectConnection);


//    //### readFileWorker ###
//    readFileWorker = new ReadFileWorker();
//    readFileWorker->setFilePaths(&file_paths);
//    readFileWorker->setFiles(&files);
//    readFileWorker->setSVOFile(&svo_inprocess);

//    readFileWorker->moveToThread(readFileThread);
//    connect(readFileThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
//    connect(readFileWorker, SIGNAL(finished()), this, SLOT(readFileButtonFinish()));
//    connect(readFileThread, SIGNAL(started()), readFileWorker, SLOT(process()));
//    connect(readFileWorker, SIGNAL(abort()), readFileThread, SLOT(quit()));
//    connect(readFileWorker, SIGNAL(finished()), readFileThread, SLOT(quit()));
//    connect(readFileWorker, SIGNAL(changedFile(QString)), this, SLOT(setHeader(QString)));
//    connect(readFileWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));

//    connect(readFileWorker, SIGNAL(changedMemoryUsage(int)), memoryUsageProgressBar, SLOT(setValue(int)));
//    connect(readFileWorker, SIGNAL(changedFormatMemoryUsage(QString)), this, SLOT(setMemoryUsageFormat(QString)));
//    connect(readFileWorker, SIGNAL(changedRangeMemoryUsage(int,int)), memoryUsageProgressBar, SLOT(setRange(int,int)));

//    connect(readFileWorker, SIGNAL(changedGenericProgress(int)), genericProgressBar, SLOT(setValue(int)));
//    connect(readFileWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
//    connect(readFileWorker, SIGNAL(changedRangeGenericProcess(int,int)), genericProgressBar, SLOT(setRange(int,int)));

//    connect(readFileWorker, SIGNAL(changedTabWidget(int)), tabWidget, SLOT(setCurrentIndex(int)));
//    connect(readFileButton, SIGNAL(clicked()), readFileThread, SLOT(start()));
//    connect(killButton, SIGNAL(clicked()), readFileWorker, SLOT(killProcess()), Qt::DirectConnection);


//    //### projectFileWorker ###
//    projectFileWorker = new ProjectFileWorker();
//    projectFileWorker->setFilePaths(&file_paths);
//    projectFileWorker->setSVOFile(&svo_inprocess);
//    projectFileWorker->setFiles(&files);
//    projectFileWorker->setOpenCLContext(context_cl);
//    projectFileWorker->setReducedPixels(&reduced_pixels);
//    projectFileWorker->initializeCLKernel();
//    connect(this->activeAngleComboBox, SIGNAL(currentIndexChanged(int)), projectFileWorker, SLOT(setActiveAngle(int)));
//    connect(this->omegaCorrectionSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setOffsetOmega(double)));
//    connect(this->kappaCorrectionSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setOffsetKappa(double)));
//    connect(this->phiCorrectionSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setOffsetPhi(double)));
////    connect(this->noiseCorrectionMinDoubleSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setNoiseLow(double)));
////    connect(this->noiseCorrectionMaxDoubleSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setNoiseHigh(double)));
////    connect(this->postCorrectionMinDoubleSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setThldProjectLow(double)));
////    connect(this->postCorrectionMaxDoubleSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setThldProjectHigh(double)));
//    connect(imagePreviewWindow->worker(), SIGNAL(noiseLowChanged(double)), projectFileWorker, SLOT(setNoiseLow(double)));
    

//    projectFileWorker->moveToThread(projectFileThread);
//    connect(projectFileThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
//    connect(projectFileWorker, SIGNAL(finished()), this, SLOT(projectFileButtonFinish()));
//    connect(projectFileThread, SIGNAL(started()), projectFileWorker, SLOT(process()));
//    connect(projectFileWorker, SIGNAL(finished()), projectFileThread, SLOT(quit()));
//    connect(projectFileWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));

//    connect(projectFileWorker, SIGNAL(changedMemoryUsage(int)), memoryUsageProgressBar, SLOT(setValue(int)));
//    connect(projectFileWorker, SIGNAL(changedFormatMemoryUsage(QString)), this, SLOT(setMemoryUsageFormat(QString)));
//    connect(projectFileWorker, SIGNAL(changedRangeMemoryUsage(int,int)), memoryUsageProgressBar, SLOT(setRange(int,int)));

//    connect(projectFileWorker, SIGNAL(changedGenericProgress(int)), genericProgressBar, SLOT(setValue(int)));
//    connect(projectFileWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
//    connect(projectFileWorker, SIGNAL(changedRangeGenericProcess(int,int)), genericProgressBar, SLOT(setRange(int,int)));

//    connect(projectFileWorker, SIGNAL(changedTabWidget(int)), tabWidget, SLOT(setCurrentIndex(int)));
//    connect(projectFileButton, SIGNAL(clicked()), this, SLOT(runProjectFileThread()));
//    connect(killButton, SIGNAL(clicked()), projectFileWorker, SLOT(killProcess()), Qt::DirectConnection);
////    connect(imagePreviewWindow->worker(), SIGNAL(selectionChanged(Selection)), projectFileWorker, SLOT(setSelection(Selection)));
//    connect(this, SIGNAL(selectionChanged(Selection)), projectFileWorker, SLOT(setSelection(Selection)));


    //### allInOneWorker ###
//    reconstructWorker = new ReconstructWorker();
//    reconstructWorker->setFilePaths(&file_paths);
//    reconstructWorker->setSVOFile(&svo_inprocess);
//    reconstructWorker->setOpenCLContext(context_cl);
    imagePreviewWindow->worker()->setReducedPixels(&reduced_pixels);
//    imagePreviewWindow->worker()->initializeCLKernel();
//    connect(allInOneButton, SIGNAL(clicked()), this, SLOT(setFilesFromSelectionModel()));
    connect(this->activeAngleComboBox, SIGNAL(currentIndexChanged(int)), imagePreviewWindow->worker(), SLOT(setActiveAngle(int)));
    connect(this->omegaCorrectionSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setOffsetOmega(double)));
    connect(this->kappaCorrectionSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setOffsetKappa(double)));
    connect(this->phiCorrectionSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setOffsetPhi(double)));
//    connect(this->noiseCorrectionMinDoubleSpinBox, SIGNAL(valueChanged(double)), multiWorker, SLOT(setNoiseLow(double)));
//    connect(this->noiseCorrectionMaxDoubleSpinBox, SIGNAL(valueChanged(double)), multiWorker, SLOT(setNoiseHigh(double)));
//    connect(this->postCorrectionMinDoubleSpinBox, SIGNAL(valueChanged(double)), multiWorker, SLOT(setThldProjectLow(double)));
//    connect(this->postCorrectionMaxDoubleSpinBox, SIGNAL(valueChanged(double)), multiWorker, SLOT(setThldProjectHigh(double)));
//    connect(imagePreviewWindow->worker(), SIGNAL(noiseLowChanged(double)), reconstructWorker, SLOT(setNoiseLow(double)));

//    reconstructWorker->moveToThread(reconstructThread);
//    connect(reconstructThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
//    connect(reconstructWorker, SIGNAL(finished()), this, SLOT(allInOneButtonFinish()));
//    connect(reconstructThread, SIGNAL(started()), this, SLOT(transferSet()), Qt::DirectConnection);
//    connect(reconstructThread, SIGNAL(started()), reconstructWorker, SLOT(process()));
//    connect(reconstructWorker, SIGNAL(finished()), reconstructThread, SLOT(quit()));
    connect(imagePreviewWindow->worker(), SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));

    connect(imagePreviewWindow->worker(), SIGNAL(changedMemoryUsage(int)), memoryUsageProgressBar, SLOT(setValue(int)));
    connect(imagePreviewWindow->worker(), SIGNAL(changedFormatMemoryUsage(QString)), this, SLOT(setMemoryUsageFormat(QString)));
    connect(imagePreviewWindow->worker(), SIGNAL(changedRangeMemoryUsage(int,int)), memoryUsageProgressBar, SLOT(setRange(int,int)));

    connect(imagePreviewWindow->worker(), SIGNAL(changedGenericProgress(int)), genericProgressBar, SLOT(setValue(int)));
    connect(imagePreviewWindow->worker(), SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(imagePreviewWindow->worker(), SIGNAL(changedRangeGenericProcess(int,int)), genericProgressBar, SLOT(setRange(int,int)));
//    connect(this, SIGNAL(setPulled(SeriesSet)), reconstructWorker, SLOT(setSet(SeriesSet)));

//    connect(reconstructWorker, SIGNAL(changedFile(QString)), this, SLOT(setHeader(QString)));
//    connect(reconstructWorker, SIGNAL(changedTabWidget(int)), tabWidget, SLOT(setCurrentIndex(int)));
    connect(reconstructButton, SIGNAL(clicked()), imagePreviewWindow->worker(), SLOT(reconstruct()));
    connect(killButton, SIGNAL(clicked()), imagePreviewWindow->worker(), SLOT(killProcess()), Qt::DirectConnection);
//    connect(imagePreviewWindow->worker(), SIGNAL(selectionChanged(Selection)), multiWorker, SLOT(setSelection(Selection)));
//    connect(this, SIGNAL(selectionChanged(Selection)), reconstructWorker, SLOT(setSelection(Selection)));


    //### voxelizeWorker ###
    voxelizeWorker = new VoxelizeWorker();
    voxelizeWorker->setOpenCLContext(context_cl);
    voxelizeWorker->moveToThread(voxelizeThread);
    voxelizeWorker->setSVOFile(&svo_inprocess);
    voxelizeWorker->setReducedPixels(&reduced_pixels);
    voxelizeWorker->initializeCLKernel();

    voxelizeWorker->moveToThread(voxelizeThread);
//    connect(voxelizeThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
//    connect(voxelizeWorker, SIGNAL(finished()), this, SLOT(voxelizeButtonFinish()));
    connect(svoLevelSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setCurrentSvoLevel(int)));
    connect(voxelizeThread, SIGNAL(started()), voxelizeWorker, SLOT(process()));
    connect(voxelizeWorker, SIGNAL(finished()), voxelizeThread, SLOT(quit()));
    connect(voxelizeWorker, SIGNAL(popup(QString,QString)), this, SLOT(displayPopup(QString,QString)));
    connect(voxelizeWorker, SIGNAL(changedMemoryUsage(int)), memoryUsageProgressBar, SLOT(setValue(int)));
    connect(voxelizeWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));

    connect(voxelizeWorker, SIGNAL(changedMemoryUsage(int)), memoryUsageProgressBar, SLOT(setValue(int)));
    connect(voxelizeWorker, SIGNAL(changedFormatMemoryUsage(QString)), this, SLOT(setMemoryUsageFormat(QString)));
    connect(voxelizeWorker, SIGNAL(changedRangeMemoryUsage(int,int)), memoryUsageProgressBar, SLOT(setRange(int,int)));

    connect(voxelizeWorker, SIGNAL(changedGenericProgress(int)), genericProgressBar, SLOT(setValue(int)));
    connect(voxelizeWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(voxelizeWorker, SIGNAL(changedRangeGenericProcess(int,int)), genericProgressBar, SLOT(setRange(int,int)));

    connect(voxelizeButton, SIGNAL(clicked()), voxelizeThread, SLOT(start()));
    connect(killButton, SIGNAL(clicked()), voxelizeWorker, SLOT(killProcess()), Qt::DirectConnection);
    connect(imagePreviewWindow->worker(), SIGNAL(qSpaceInfoChanged(float,float,float)), voxelizeWorker, SLOT(setQSpaceInfo(float,float,float)));
//    connect(setFileWorker, SIGNAL(qSpaceInfoChanged(float,float,float)), voxelizeWorker, SLOT(setQSpaceInfo(float,float,float)));
}

//void MainWindow::anyButtonStart()
//{
//    setFileButton->setDisabled(true);
//    reconstructButton->setDisabled(true);
//    readFileButton->setDisabled(true);
//    projectFileButton->setDisabled(true);
//    voxelizeButton->setDisabled(true);
//}

//void MainWindow::setFileButtonFinish()
//{
//    setFileButton->setDisabled(false);
//    reconstructButton->setDisabled(false);
//    readFileButton->setDisabled(false);
//    projectFileButton->setDisabled(true);
//    voxelizeButton->setDisabled(true);
//}

//void MainWindow::allInOneButtonFinish()
//{
//    setFileButton->setDisabled(false);
//    reconstructButton->setDisabled(false);
//    readFileButton->setDisabled(true);
//    projectFileButton->setDisabled(true);
//    voxelizeButton->setDisabled(false);
//}

//void MainWindow::readFileButtonFinish()
//{
//    setFileButton->setDisabled(false);
//    reconstructButton->setDisabled(false);
//    readFileButton->setDisabled(false);
//    projectFileButton->setDisabled(false);
//    voxelizeButton->setDisabled(true);
//}

//void MainWindow::projectFileButtonFinish()
//{
//    setFileButton->setDisabled(false);
//    reconstructButton->setDisabled(false);
//    readFileButton->setDisabled(false);
//    projectFileButton->setDisabled(false);
//    voxelizeButton->setDisabled(false);
//}

//void MainWindow::voxelizeButtonFinish()
//{
//    setFileButton->setDisabled(false);
//    reconstructButton->setDisabled(false);
//    readFileButton->setDisabled(false);
//    projectFileButton->setDisabled(false);
//    voxelizeButton->setDisabled(false);
//}

//void MainWindow::setImage(ImageInfo image)
//{
//    if (!series_set.isEmpty())
//    {
//        *series_set.current()->current() = image;

//        pathLineEdit->setText(series_set.current()->current()->path());

//        setHeader(series_set.current()->current()->path());

//        hasPendingChanges = true;
//    }
//}


//void MainWindow::setSeriesSelection(Selection area)
//{
//    if (!series_set.isEmpty())
//    {
//        series_set.current()->setSelection(area);
//        hasPendingChanges = true;
//    }
//}

void MainWindow::loadPaths()
{
    QMessageBox confirmationMsgBox;
    
    confirmationMsgBox.setWindowTitle("Nebula");
    confirmationMsgBox.setIcon(QMessageBox::Question);
    confirmationMsgBox.setText("Unsaved changes will be lost.");
    confirmationMsgBox.setInformativeText("Save?");
    confirmationMsgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    confirmationMsgBox.setDefaultButton(QMessageBox::Save);
    
    int ret = QMessageBox::Discard;
    
    if (hasPendingChanges) ret = confirmationMsgBox.exec();
    
    switch (ret) 
    {
        case QMessageBox::Save:
            // Save was clicked
            saveProject();
            setFiles(fileSelectionModel->getPaths());
            break;
        case QMessageBox::Discard:
            // Discard was clicked
            setFiles(fileSelectionModel->getPaths());
            break;
        case QMessageBox::Cancel:
            // Cancel was clicked
            break;
        default:
            // should never be reached
            break;
    }
}

void MainWindow::setHeader(QString path)
{
    DetectorFile file(path);
    fileHeaderEdit->setPlainText(file.getHeaderText());
}

void MainWindow::setFiles(QMap<QString, QStringList> folder_map)
{
    SeriesSet set;
    
    QMap<QString, QStringList>::const_iterator i = folder_map.constBegin();
    while (i != folder_map.constEnd())
    {
        ImageSeries folder;
        folder.setPath(i.key());

        QStringList image_strings(i.value());
        QStringList::const_iterator j = image_strings.constBegin();

        while (j != image_strings.constEnd())
        {
            ImageInfo image;

            image.setPath(*j);

            folder << image;
            ++j;
        }
            
        set << folder;

        ++i;
    }
    
    if (!set.isEmpty()) 
    {
//        file_paths = set.paths();
        emit setChanged(set);
        imageSpinBox->setRange(0,set.current()->size()-1);
    }
}


//void MainWindow::removeImage()
//{
//    if (!series_set.isEmpty())
//    {
//        emit pathRemoved(series_set.current()->current()->path());
        
//        series_set.current()->removeCurrent();

////        if (image_folder.size() == 0) image_folder.removeCurrent();
        
//        if (!series_set.isEmpty())
//        {
//            emit imageChanged(*series_set.current()->next());
//        }
//    }
//}

//void MainWindow::setFrame(int value)
//{
//    if (!series_set.isEmpty())
//    {
//        emit imageChanged(*series_set.current()->at(value));
//    }
//}



//void MainWindow::nextFrame()
//{
//    imageSpinBox->setValue(imageSpinBox->value()+1);
//}
//void MainWindow::prevFrame()
//{
//    imageSpinBox->setValue(imageSpinBox->value()-1);
//}
//void MainWindow::batchForward()
//{
//    imageSpinBox->setValue(imageSpinBox->value()+batch_size);
//}
//void MainWindow::batchBackward()
//{
//    imageSpinBox->setValue(imageSpinBox->value()-batch_size);
//}


//void MainWindow::nextSeries()
//{
//    if (!series_set.isEmpty())
//    {
//        series_set.current()->saveCurrentIndex();
//        series_set.next();
//        series_set.current()->loadSavedIndex();

//        imageSpinBox->setRange(0,series_set.current()->size()-1);
//        imageSpinBox->setValue(series_set.current()->i());

//        emit imageChanged(*series_set.current()->current());
//    }
//}

//void MainWindow::prevSeries()
//{
//    if (!series_set.isEmpty())
//    {
//        series_set.current()->saveCurrentIndex();
//        series_set.previous();
//        series_set.current()->loadSavedIndex();

//        imageSpinBox->setRange(0,series_set.current()->size()-1);
//        imageSpinBox->setValue(series_set.current()->i());

//        emit imageChanged(*series_set.current()->current());
//    }
//}

//void MainWindow::runProjectFileThread()
//{
//    tabWidget->setCurrentIndex(1);
    
    // Creation settings
//    svo_inprocess.creation_date = QDateTime::currentDateTime();
//    svo_inprocess.creation_noise_cutoff_low = noiseCorrectionMinDoubleSpinBox->value();
//    svo_inprocess.creation_noise_cutoff_high = noiseCorrectionMaxDoubleSpinBox->value();
//    svo_inprocess.creation_post_cutoff_low = postCorrectionMinDoubleSpinBox->value();
//    svo_inprocess.creation_post_cutoff_high = postCorrectionMaxDoubleSpinBox->value();
//    svo_inprocess.creation_correction_omega = omegaCorrectionSpinBox->value();
//    svo_inprocess.creation_correction_kappa = kappaCorrectionSpinBox->value();
//    svo_inprocess.creation_correction_phi = phiCorrectionSpinBox->value();
//    svo_inprocess.creation_file_paths = file_paths;
    
//    projectFileThread->start();
//}

//void MainWindow::runAllInOneThread()
//{
//    tabWidget->setCurrentIndex(1);
    
    // Creation settings
//    svo_inprocess.creation_date = QDateTime::currentDateTime();
//    svo_inprocess.creation_noise_cutoff_low = noiseCorrectionMinDoubleSpinBox->value();
//    svo_inprocess.creation_noise_cutoff_high = noiseCorrectionMaxDoubleSpinBox->value();
//    svo_inprocess.creation_post_cutoff_low = postCorrectionMinDoubleSpinBox->value();
//    svo_inprocess.creation_post_cutoff_high = postCorrectionMaxDoubleSpinBox->value();
//    svo_inprocess.creation_correction_omega = omegaCorrectionSpinBox->value();
//    svo_inprocess.creation_correction_kappa = kappaCorrectionSpinBox->value();
//    svo_inprocess.creation_correction_phi = phiCorrectionSpinBox->value();
//    svo_inprocess.creation_file_paths = file_paths;
    
    
//    reconstructThread->start();
//}

//void MainWindow::setFilesFromSelectionModel()
//{
//    file_paths = series_set.paths();
//}

void MainWindow::setStartConditions()
{
    tabWidget->setCurrentIndex(0);

    svoLevelSpinBox->setValue(11);

    volumeRenderDataMinSpinBox->setValue(1.0);
    volumeRenderDataMaxSpinBox->setValue(10);
    volumeRenderAlphaSpinBox->setValue(1.0);
    volumeRenderBrightnessSpinBox->setValue(2.0);
    volumeRenderTsfAlphaComboBox->setCurrentIndex(2);
    volumeRenderViewModeComboBox->setCurrentIndex(1);
    volumeRenderViewModeComboBox->setCurrentIndex(0);
    volumeRenderTsfComboBox->setCurrentIndex(1);
    volumeRenderLogCheckBox->setChecked(true);
    
    //
    
    batchSizeSpinBox->setValue(10);
    
    correctionPlaneCheckBox->setChecked(true);
    correctionPlaneCheckBox->setChecked(false);
    
    correctionNoiseDoubleSpinBox->setValue(1);
    correctionNoiseDoubleSpinBox->setValue(0);

    selectionModeComboBox->setCurrentIndex(1);
    selectionModeComboBox->setCurrentIndex(0);

    correctionPlaneSpinBox->setValue(10);
    
    //
    
    
    imagePreviewTsfTextureComboBox->setCurrentIndex(1);
    imagePreviewTsfAlphaComboBox->setCurrentIndex(2);
    imagePreviewDataMinDoubleSpinBox->setValue(10);
    imagePreviewDataMinDoubleSpinBox->setValue(1);
    imagePreviewDataMaxDoubleSpinBox->setValue(1000);
    imagePreviewLogCheckBox->setChecked(true);
    correctionLorentzCheckBox->setChecked(true);
    imageModeComboBox->setCurrentIndex(1);
    imageModeComboBox->setCurrentIndex(0);
    
    
    funcParamASpinBox->setValue(13.5);
    funcParamBSpinBox->setValue(10.5);
    funcParamCSpinBox->setValue(10.0);
    funcParamDSpinBox->setValue(0.005);

    qualitySlider->setValue(20);
    
    fileFilter->setText("*.cbf");
    
    activeAngleComboBox->setCurrentIndex(2);
    omegaCorrectionSpinBox->setValue(1.0);
    kappaCorrectionSpinBox->setValue(1.0);
    phiCorrectionSpinBox->setValue(1.0);

    omegaCorrectionSpinBox->setValue(0.1);
    kappaCorrectionSpinBox->setValue(0.1);
    phiCorrectionSpinBox->setValue(0.1);
    
    omegaCorrectionSpinBox->setValue(0.0);
    kappaCorrectionSpinBox->setValue(0.0);
    phiCorrectionSpinBox->setValue(0.0);
    
    alphaNormSpinBox->setValue(90);
    betaNormSpinBox->setValue(90);
    gammaNormSpinBox->setValue(90);
    
    aNormSpinBox->setValue(1);
    bNormSpinBox->setValue(1);
    cNormSpinBox->setValue(1);
    
    
    
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

void MainWindow::saveProject()
{
//    QFileDialog dialog;
    
//    qDebug() << "Got here";
    
//    dialog.setDefaultSuffix("txt");
    
//    qDebug() << "Got here";
    
//    QString file_name = dialog.getSaveFileName();
    QString file_name = QFileDialog::getSaveFileName(this, tr("Save project"), working_dir, tr(".qt (*.qt);; All Files (*)"));
    
//    QString file_name = "/home/natt/Workdir/BUGTEST.qt";
    
//    qDebug() << file_name;
    
    if (file_name != "")
    {
        QFileInfo info(file_name);
        working_dir = info.absoluteDir().path();
        
//        qDebug() << working_dir;
        
        QFile file(file_name);
        if (file.open(QIODevice::WriteOnly))
        {
//            qDebug() << "Got here";
            
            QDataStream out(&file);
            
            out << imagePreviewWindow->worker()->set();
            out << imageModeComboBox->currentText();
            out << imagePreviewTsfTextureComboBox->currentText();
            out << imagePreviewTsfAlphaComboBox->currentText();
            out << (double) imagePreviewDataMinDoubleSpinBox->value();
            out << (double) imagePreviewDataMaxDoubleSpinBox->value();
            out << (bool) imagePreviewLogCheckBox->isChecked();
            out << (bool) correctionLorentzCheckBox->isChecked();  
            out << (double) correctionNoiseDoubleSpinBox->value();
            
//            qDebug() << "Got here";
            
            file.close();
        }
    }
    
    
    hasPendingChanges = false;
//    qDebug() << "Got here too";
//    {
//        QFileDialog dialog;
//    }
    
}

void MainWindow::transferSet()
{
    SeriesSet set = imagePreviewWindow->worker()->set();
    
//    qDebug() << "Main" << set << set.begin()->begin()->selection();
    
    emit setPulled(set);
}

void MainWindow::loadProject()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Open project"), working_dir, tr(".qt (*.qt);; All Files (*)"));

    if (file_name != "")
    {
        QFileInfo info(file_name);
        working_dir = info.absoluteDir().path();
        
        QFile file(file_name);
        if (file.open(QIODevice::ReadOnly))
        {
            SeriesSet set;
            
            QString mode;
            QString tsfTexture;
            QString tsfAlpha;
            
            double dataMin;
            double dataMax;
            bool log;
            bool lorentzCorrection;
            double noise;
            
            QDataStream in(&file);
            
            in >> set >> mode >> tsfTexture >> tsfAlpha >> dataMin >> dataMax >> log >> lorentzCorrection >> noise;
            
            emit setChanged(set);            
            
            imageModeComboBox->setCurrentText(mode);
            imagePreviewTsfTextureComboBox->setCurrentText(tsfTexture);
            imagePreviewTsfAlphaComboBox->setCurrentText(tsfAlpha);
            imagePreviewDataMinDoubleSpinBox->setValue(dataMin);
            imagePreviewDataMaxDoubleSpinBox->setValue(dataMax);
            imagePreviewLogCheckBox->setChecked(log);
            correctionLorentzCheckBox->setChecked(lorentzCorrection);
            correctionNoiseDoubleSpinBox->setValue(noise);
            
            file.close();
        }
    }
}



//void MainWindow::setSelection(Selection rect)
//{
//    if (!series_set.isEmpty())
//    {
//        series_set.current()->current()->setSelection(rect);
        
//        hasPendingChanges = true;
//    }
//}



void MainWindow::initActions()
{


    // Actions
    exitAct = new QAction(tr("E&xit program"), this);
    aboutAct = new QAction(tr("&About Nebula"), this);
    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutOpenCLAct = new QAction(tr("About OpenCL"), this);
    aboutOpenGLAct = new QAction(tr("About OpenGL"), this);
    openSvoAct = new QAction(QIcon(":/art/open.png"), tr("Open SVO"), this);
    saveSVOAct = new QAction(QIcon(":/art/saveScript.png"), tr("Save SVO"), this);
    saveLoadedSvoAct = new QAction(QIcon(":/art/save.png"), tr("Save current SVO"), this);
    dataStructureAct = new QAction(QIcon(":/art/datastructure.png"), tr("Toggle data structure"), this);
    dataStructureAct->setCheckable(true);
    backgroundAct = new QAction(QIcon(":/art/background.png"), tr("Toggle background color"), this);
    backgroundAct->setCheckable(true);
    projectionAct = new QAction(QIcon(":/art/projection.png"), tr("Toggle projection"), this);
    projectionAct->setCheckable(true);
    projectionAct->setChecked(true);
    screenshotAct = new QAction(QIcon(":/art/screenshot.png"), tr("&Take screenshot"), this);
    scalebarAct = new QAction(QIcon(":/art/scalebar.png"), tr("&Toggle scalebars"), this);
    scalebarAct->setCheckable(true);
    scalebarAct->setChecked(true);
    sliceAct = new QAction(QIcon(":/art/slice.png"), tr("&Toggle slicing"), this);
    sliceAct->setCheckable(true);
    sliceAct->setChecked(false);
    integrate2DAct = new QAction(QIcon(":/art/integrate.png"), tr("&Toggle 3D->1D integration"), this);
    integrate2DAct->setCheckable(true);
    integrate3DAct = new QAction(QIcon(":/art/integrate.png"), tr("&Toggle 3D->2D integration"), this);
    integrate3DAct->setCheckable(true);
    integrate3DAct->setChecked(true);
    logIntegrate2DAct = new QAction(QIcon(":/art/log.png"), tr("&Toggle logarithmic"), this);
    logIntegrate2DAct->setCheckable(true);
    shadowAct = new QAction(QIcon(":/art/shadow.png"), tr("&Toggle shadows"), this);
    shadowAct->setCheckable(true);
    orthoGridAct = new QAction(QIcon(":/art/grid.png"), tr("&Toggle orthonormal grid"), this);
    orthoGridAct->setCheckable(true);
    
    rulerAct = new QAction(QIcon(":/art/ruler.png"), tr("&Toggle ruler"), this);
    rulerAct->setCheckable(true);
    
    markAct = new QAction(QIcon(":/art/marker.png"), tr("&Add marker"), this);
    labFrameAct = new QAction(QIcon(":/art/labframe.png"), tr("&View lab frame"), this);
    labFrameAct->setCheckable(true);
    labFrameAct->setChecked(true);
    
    alignLabXtoSliceXAct = new QAction(QIcon(":/art/align_x.png"), tr("Align lab frame to slice frame x"), this);
    alignLabYtoSliceYAct = new QAction(QIcon(":/art/align_y.png"), tr("Align lab frame to slice frame y"), this);
    alignLabZtoSliceZAct = new QAction(QIcon(":/art/align_z.png"), tr("Align lab frame to slice frame z"), this);
    alignSliceToLabAct = new QAction(QIcon(":/art/align_slice_frame_to_lab_frame"), tr("Align slice frame to lab frame"), this);
    
    rotateRightAct = new QAction(QIcon(":/art/rotate_right.png"), tr("Rotate right"), this);
    rotateLeftAct = new QAction(QIcon(":/art/rotate_left.png"), tr("Rotate left"), this);
    rotateUpAct = new QAction(QIcon(":/art/rotate_up.png"), tr("Rotate up"), this);
    rotateDownAct = new QAction(QIcon(":/art/rotate_down.png"), tr("Rotate down"), this);
    rollCW = new QAction(QIcon(":/art/roll_cw.png"), tr("Roll clockwise"), this);
    rollCCW = new QAction(QIcon(":/art/roll_ccw.png"), tr("Roll counterclockwise"), this);
    integrateCountsAct = new QAction(QIcon(":/art/integrate_counts.png"), tr("Integrate intensity in the view box"), this);
    integrateCountsAct->setCheckable(true);
    integrateCountsAct->setChecked(false);
    
    imageScreenshotAct = new QAction(QIcon(":/art/screenshot.png"), tr("Take screenshot"), this);
    imageScreenshotAct->setCheckable(false);
    
    saveImageAct = new QAction(QIcon(":/art/screenshot.png"), tr("Save image"), this);
    saveImageAct->setCheckable(false);
    
    // Action Tips
    exitAct->setStatusTip(tr("Exit Nebula"));
    aboutAct->setStatusTip(tr("About"));
    aboutQtAct->setStatusTip(tr("About Qt"));
    aboutOpenCLAct->setStatusTip(tr("About OpenCL"));
    aboutOpenGLAct->setStatusTip(tr("About OpenGL"));

    // Shortcuts
    exitAct->setShortcuts(QKeySequence::Quit);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Nebula"),
        tr("<h1>About Nebula</h1> <b>Nebula</b> is primarily a program to reduce, visualize, and analyze diffuse X-ray scattering. <br> <a href=\"www.github.org/\">github.org</a> <h1>Lisencing (LGPL)</h1> This program is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.\n This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details. \n You should have received a copy of the GNU Lesser General Public License along with this program.  If not, see <a href=\"http://www.gnu.org/licenses/\">http://www.gnu.org/licenses/</a> "));
}

void MainWindow::aboutOpenCL()
{
    QMessageBox::about(this, tr("About OpenCL"),
        tr("<h1>About OpenCL</h1> <b>OpenCL</b> is the first open, royalty-free standard for cross-platform, parallel programming of modern processors found in personal computers, servers and handheld/embedded devices. OpenCL (Open Computing Language) greatly improves speed and responsiveness for a wide spectrum of applications in numerous market categories from gaming and entertainment to scientific and medical software. <br> <a href=\"https://www.khronos.org/opencl/\">https://www.khronos.org/opencl</a>"));
}

void MainWindow::aboutOpenGL()
{
    QMessageBox::about(this, tr("About OpenGL"),
        tr("<h1>About OpenGL</h1> <b>OpenGL</b>  is the most widely adopted 2D and 3D graphics API in the industry, bringing thousands of applications to a wide variety of computer platforms. It is window-system and operating-system independent as well as network-transparent. OpenGL enables developers of software for PC, workstation, and supercomputing hardware to create high-performance, visually compelling graphics software applications, in markets such as CAD, content creation, energy, entertainment, game development, manufacturing, medical, and virtual reality. OpenGL exposes all the features of the latest graphics hardware.<br> <a href=\"https://www.khronos.org/opengl\">www.khronos.org/opengl</a>"));
}

void MainWindow::openUnitcellFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"), "", tr(".par (*.par);; All files (*)"));

    if ((fileName != ""))
    {
        // Regular expressions to match data in .par files
        QString wavelengthRegExp("CRYSTALLOGRAPHY\\sWAVELENGTH\\D+(\\d+\\.\\d+)");

        QStringList UBRegExp;
        UBRegExp.append("CRYSTALLOGRAPHY\\sUB\\D+([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+");
        UBRegExp.append("([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+");
        UBRegExp.append("([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+");
        UBRegExp.append("([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+");
        UBRegExp.append("([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+");
        UBRegExp.append("([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+");
        UBRegExp.append("([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+");
        UBRegExp.append("([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+");
        UBRegExp.append("([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+");

        QStringList unitcellRegExp;
        unitcellRegExp.append("CELL\\sINFORMATION\\D+(\\d+\\.\\d+)\\D+\\d+\\.\\d+\\D+");
        unitcellRegExp.append("(\\d+\\.\\d+)\\D+\\d+\\.\\d+\\D+");
        unitcellRegExp.append("(\\d+\\.\\d+)\\D+\\d+\\.\\d+\\D+");
        unitcellRegExp.append("(\\d+\\.\\d+)\\D+\\d+\\.\\d+\\D+");
        unitcellRegExp.append("(\\d+\\.\\d+)\\D+\\d+\\.\\d+\\D+");
        unitcellRegExp.append("(\\d+\\.\\d+)\\D+\\d+\\.\\d+\\D+");

        // Open file
        QFile f(fileName);
        if(!f.open(QIODevice::ReadOnly)) qDebug() << "open FAILED";
        QByteArray contents(f.readAll());

        // Find a, b, c, alpha, beta, gamma
        Matrix<float> abc(1,6);
        int pos = 0;
        for(int i = 0; i < unitcellRegExp.size(); i++)
        {
            QRegExp tmp(unitcellRegExp.at(i));
            pos = tmp.indexIn(contents, pos);
            if (pos > -1)
            {
                pos += tmp.matchedLength();;
                QString value = tmp.cap(1);
                abc[i] = value.toFloat();
            }
        }
        float a = abc[0];
        float b = abc[1];
        float c = abc[2];
        float alpha = abc[3]*pi/180;
        float beta = abc[4]*pi/180;
        float gamma = abc[5]*pi/180;

        // Set the values in the UI

        QString value;
        value = QString::number( a, 'g', 4 );
        this->a->setText(value);
        value = QString::number( b, 'g', 4 );
        this->b->setText(value);
        value = QString::number( c, 'g', 4 );
        this->c->setText(value);
        value = QString::number( 180/pi*alpha, 'g', 4 );
        this->alpha->setText(value);
        value = QString::number( 180/pi*beta, 'g', 4 );
        this->beta->setText(value);
        value = QString::number( 180/pi*gamma, 'g', 4 );
        this->gamma->setText(value);

        value = QString::number( 1/a, 'g', 4 );
        this->aStar->setText(value);
        value = QString::number( 1/b, 'g', 4 );
        this->bStar->setText(value);
        value = QString::number( 1/c, 'g', 4 );
        this->cStar->setText(value);
        value = QString::number( 180/pi*std::acos((std::cos(beta)*std::cos(gamma) - std::cos(alpha))/(std::sin(beta)*std::sin(gamma))), 'g', 4 );
        this->alphaStar->setText(value);
        value = QString::number( 180/pi*std::acos((std::cos(alpha)*std::cos(gamma) - std::cos(beta))/(std::sin(alpha)*std::sin(gamma))), 'g', 4 );
        this->betaStar->setText(value);
        value = QString::number( 180/pi*std::acos((std::cos(alpha)*std::cos(beta) - std::cos(gamma))/(std::sin(alpha)*std::sin(beta))), 'g', 4 );
        this->gammaStar->setText(value);

        // Find wavelength
        float wavelength = 0.0;
        pos = 0;
        QRegExp tmp(wavelengthRegExp);
        pos = tmp.indexIn(contents, pos);
        if (pos > -1)
        {
            QString value = tmp.cap(1);
            wavelength = value.toFloat();
        }

        // Find UB matrix
        Matrix<float> UB(3,3);
        pos = 0;
        for(int i = 0; i < UBRegExp.size(); i++)
        {
            QRegExp tmp(UBRegExp.at(i));
            pos = tmp.indexIn(contents, pos);
            if (pos > -1)
            {
                pos += tmp.matchedLength();;
                QString value = tmp.cap(1);
                UB[i] = value.toFloat()/wavelength;
            }
        }

        // Math to find U
        float sa = std::sin(alpha);
        float ca = std::cos(alpha);
        float cb = std::cos(beta);
        float cg = std::cos(gamma);
        float V = (a*b*c) * std::sqrt(1.0 - ca*ca - cb*cb - cg*cg + 2.0*ca*cb*cg);

        Matrix<float> B(3,3);
        B[0] = b*c*sa/V;
        B[1] = a*c*(ca*cb-cg)/(V*sa);
        B[2] = a*b*(ca*cg-cb)/(V*sa);
        B[3] = 0;
        B[4] = 1.0/(b*sa);
        B[5] = -ca/(c*sa);
        B[6] = 0;
        B[7] = 0;
        B[8] = 1.0/c;



        Matrix<float> U(3,3);
        U = UB * B.inverse();
    }
}



void MainWindow::setTab(int tab)
{
//    toolChainWidget->hide();
//    fileHeaderDock->hide();
//    outputDockWidget->hide();
//    fileDockWidget->hide();
//    graphicsDockWidget->hide();
//    unitCellDock->hide();
//    functionDockWidget->hide();
//    svoHeaderDock->hide();
    
//    if ((tab==0) || (tab==1)) toolChainWidget->show();
//    else toolChainWidget->hide();
    
    if ((tab==0) || (tab==1)) fileHeaderDock->show();
    else fileHeaderDock->hide();
    
    if ((tab==0) || (tab==1)) outputDockWidget->show();
    else outputDockWidget->hide();
    
//    if (tab==1) fileDockWidget->show();
//    else fileDockWidget->hide();
    
//    if (tab==2) graphicsDockWidget->show();
//    else graphicsDockWidget->hide();
    
//    if (tab==2) unitCellDock->show();
//    else unitCellDock->hide();
     
//    if (tab==2) functionDockWidget->show();
//    else functionDockWidget->hide();
    
//    if (tab==2) svoHeaderDock->show();
//    else svoHeaderDock->hide();
}


void MainWindow::initConnects()
{
    /* this <-> volumeRenderWidget */
    connect(this->qualitySlider, SIGNAL(valueChanged(int)), volumeRenderWindow->worker(), SLOT(setQuality(int)));
    connect(this->qualitySlider, SIGNAL(sliderReleased()), volumeRenderWindow->worker(), SLOT(refreshTexture()));
    connect(this->scalebarAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(setScalebar()));
    connect(this->sliceAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(setSlicing()));
    connect(this->integrate2DAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(setIntegration2D()));
    connect(this->integrate3DAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(setIntegration3D()));
    connect(this->shadowAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(setShadow()));
    connect(this->orthoGridAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(setOrthoGrid()));
    connect(this->projectionAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(setProjection()));
    connect(this->backgroundAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(setBackground()));
    connect(this->logIntegrate2DAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(setLogarithmic2D()));
    connect(this->dataStructureAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(setDataStructure()));
    connect(this->volumeRenderTsfComboBox, SIGNAL(currentIndexChanged(int)), volumeRenderWindow->worker(), SLOT(setTsfColor(int)));
    connect(this->volumeRenderViewModeComboBox, SIGNAL(currentIndexChanged(int)), volumeRenderWindow->worker(), SLOT(setViewMode(int)));
    connect(this->volumeRenderTsfAlphaComboBox, SIGNAL(currentIndexChanged(int)), volumeRenderWindow->worker(), SLOT(setTsfAlpha(int)));
    connect(this->volumeRenderDataMinSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->worker(), SLOT(setDataMin(double)));
    connect(this->volumeRenderDataMaxSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->worker(), SLOT(setDataMax(double)));
    connect(this->volumeRenderAlphaSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->worker(), SLOT(setAlpha(double)));
    connect(this->volumeRenderBrightnessSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->worker(), SLOT(setBrightness(double)));
    connect(this->functionToggleButton, SIGNAL(clicked()), volumeRenderWindow->worker(), SLOT(setModel()));
    connect(this->funcParamASpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->worker(), SLOT(setModelParam0(double)));
    connect(this->funcParamBSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->worker(), SLOT(setModelParam1(double)));
    connect(this->funcParamCSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->worker(), SLOT(setModelParam2(double)));
    connect(this->funcParamDSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->worker(), SLOT(setModelParam3(double)));
    connect(volumeRenderWindow->worker(), SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(this, SIGNAL(captureFrameBuffer(QString)), volumeRenderWindow->worker(), SLOT(takeScreenShot(QString)));
    connect(this->alignLabXtoSliceXAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(alignLabXtoSliceX()));
    connect(this->alignLabYtoSliceYAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(alignLabYtoSliceY()));
    connect(this->alignLabZtoSliceZAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(alignLabZtoSliceZ()));
    connect(this->alignSliceToLabAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(alignSliceToLab()));
    connect(this->rotateLeftAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(rotateLeft()));
    connect(this->rotateRightAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(rotateRight()));
    connect(this->rotateUpAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(rotateUp()));
    connect(this->rotateDownAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(rotateDown()));
    connect(this->rollCW, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(rollCW()));
    connect(this->rollCCW, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(rollCCW()));
    connect(this->rulerAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(toggleRuler()));
    connect(this->markAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(addMarker()));
    connect(this->labFrameAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(setLabFrame()));
    connect(this->rotateCellButton, SIGNAL(clicked()), volumeRenderWindow->worker(), SLOT(setURotation()));
    connect(this->toggleCellButton, SIGNAL(clicked()), volumeRenderWindow->worker(), SLOT(setUnitcell()));
    connect(this->hSpinBox, SIGNAL(valueChanged(int)), volumeRenderWindow->worker(), SLOT(setHCurrent(int)));
    connect(this->kSpinBox, SIGNAL(valueChanged(int)), volumeRenderWindow->worker(), SLOT(setKCurrent(int)));
    connect(this->lSpinBox, SIGNAL(valueChanged(int)), volumeRenderWindow->worker(), SLOT(setLCurrent(int)));
    connect(this->integrateCountsAct, SIGNAL(triggered()), volumeRenderWindow->worker(), SLOT(setCountIntegration()));
    
    /* this <-> this */
    connect(this->aNormSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->worker(), SLOT(setUB_a(double)));
    connect(this->bNormSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->worker(), SLOT(setUB_b(double)));
    connect(this->cNormSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->worker(), SLOT(setUB_c(double)));
    connect(this->alphaNormSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->worker(), SLOT(setUB_alpha(double)));
    connect(this->betaNormSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->worker(), SLOT(setUB_beta(double)));
    connect(this->gammaNormSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->worker(), SLOT(setUB_gamma(double)));
    connect(this->screenshotAct, SIGNAL(triggered()), this, SLOT(takeVolumeScreenshot()));
//    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(setTab(int)));
    connect(openSvoAct, SIGNAL(triggered()), this, SLOT(openSvo()));
    connect(saveSVOAct, SIGNAL(triggered()), this, SLOT(saveSvo()));
    connect(saveLoadedSvoAct, SIGNAL(triggered()), this, SLOT(saveLoadedSvo()));
    connect(saveSvoButton, SIGNAL(clicked()), this, SLOT(saveSvo()));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
    connect(aboutOpenCLAct, SIGNAL(triggered()), this, SLOT(aboutOpenCL()));
    connect(aboutOpenGLAct, SIGNAL(triggered()), this, SLOT(aboutOpenGL()));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    
    /*this <-> misc*/
    connect(fileFilter, SIGNAL(textChanged(QString)), fileSelectionModel, SLOT(setStringFilter(QString)));
//    connect(imagePreviewWindow->worker(), SIGNAL(resultFinished(QString)), outputPlainTextEdit, SLOT(setPlainText(QString)));
    connect(fileTreeView, SIGNAL(fileChanged(QString)), this, SLOT(setHeader(QString)));
    
    // KK
//    connect(fileFilter, SIGNAL(textChanged(QString)), fileSelectionModel, SLOT(setStringFilter(QString)));
    connect(loadPathsPushButton, SIGNAL(clicked()), this, SLOT(loadPaths()));
    connect(batchSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setBatchSize(int)));
//    connect(imagePrevtsfTextureComboBox, SIGNAL(currentIndexChanged(int)), imagePreviewWindow->worker(), SLOT(setTsfTexture(int)));
//    connect(tsfAlphaComboBox, SIGNAL(currentIndexChanged(int)), imagePreviewWindow->worker(), SLOT(setTsfAlpha(int)));
//    connect(dataMinDoubleSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setDataMin(double)));
//    connect(dataMaxDoubleSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setDataMax(double)));
//    connect(logCheckBox, SIGNAL(toggled(bool)), imagePreviewWindow->worker(), SLOT(setLog(bool)));
    connect(correctionLorentzCheckBox, SIGNAL(toggled(bool)), imagePreviewWindow->worker(), SLOT(setCorrectionLorentz(bool)));
    connect(imageModeComboBox, SIGNAL(currentIndexChanged(int)), imagePreviewWindow->worker(), SLOT(setMode(int)));
    connect(saveProjectAction, SIGNAL(triggered()), this, SLOT(saveProject()));
    connect(loadProjectAction, SIGNAL(triggered()), this, SLOT(loadProject()));
    connect(nextFramePushButton, SIGNAL(clicked()), this, SLOT(nextFrame()));
    connect(previousFramePushButton, SIGNAL(clicked()), this, SLOT(previousFrame()));
    connect(batchForwardPushButton, SIGNAL(clicked()), this, SLOT(batchForward()));
    connect(batchBackwardPushButton, SIGNAL(clicked()), this, SLOT(batchBackward()));
    connect(nextSeriesPushButton, SIGNAL(clicked()), imagePreviewWindow->worker(), SLOT(nextSeries()));
    connect(prevSeriesPushButton, SIGNAL(clicked()), imagePreviewWindow->worker(), SLOT(prevSeries()));
    connect(removeCurrentPushButton, SIGNAL(clicked()), imagePreviewWindow->worker(), SLOT(removeCurrentImage()));
    connect(imagePreviewWindow->worker(), SIGNAL(pathRemoved(QString)), fileSelectionModel, SLOT(removeFile(QString)));
    connect(this, SIGNAL(setSelection(QString)), imagePreviewWindow->worker(), SLOT(applySelection(QString)));
    connect(this, SIGNAL(setPlaneMarkers(QString)), imagePreviewWindow->worker(), SLOT(applyPlaneMarker(QString)));
    connect(this, SIGNAL(analyze(QString)), imagePreviewWindow->worker(), SLOT(analyze(QString)));
    connect(imagePreviewWindow->worker(), SIGNAL(pathChanged(QString)), this, SLOT(setHeader(QString)));
    connect(imagePreviewWindow->worker(), SIGNAL(pathChanged(QString)), pathLineEdit, SLOT(setText(QString)));
    connect(imageSpinBox, SIGNAL(valueChanged(int)), imagePreviewWindow->worker(), SLOT(setFrameByIndex(int)));
    connect(imagePreviewWindow->worker(), SIGNAL(imageRangeChanged(int,int)), this, SLOT(setImageRange(int, int)));
    connect(imagePreviewWindow->worker(), SIGNAL(currentIndexChanged(int)), imageSpinBox, SLOT(setValue(int)));
    connect(this, SIGNAL(setChanged(SeriesSet)), imagePreviewWindow->worker(), SLOT(setSet(SeriesSet)));
    connect(traceSetPushButton, SIGNAL(clicked()), imagePreviewWindow->worker(), SLOT(traceSet()));
    connect(correctionPlaneSpinBox, SIGNAL(valueChanged(int)), imagePreviewWindow->worker(), SLOT(setLsqSamples(int)));
    connect(traceTextureCheckBox,SIGNAL(toggled(bool)),imagePreviewWindow->worker(),SLOT(toggleTraceTexture(bool)));
    connect(imagePreviewWindow->worker(), SIGNAL(progressChanged(int)), generalProgressBar, SLOT(setValue(int)));
    connect(imagePreviewWindow->worker(), SIGNAL(progressRangeChanged(int,int)), generalProgressBar, SLOT(setRange(int,int)));
    connect(imagePreviewWindow->worker(), SIGNAL(visibilityChanged(bool)), generalProgressBar, SLOT(setHidden(bool)));
    connect(correctionNoiseCheckBox,SIGNAL(toggled(bool)),imagePreviewWindow->worker(),SLOT(setCorrectionNoise(bool)));
    connect(correctionPlaneCheckBox,SIGNAL(toggled(bool)),imagePreviewWindow->worker(),SLOT(setCorrectionPlane(bool)));
    connect(correctionClutterCheckBox,SIGNAL(toggled(bool)),imagePreviewWindow->worker(),SLOT(setCorrectionClutter(bool)));
    connect(correctionMedianCheckBox,SIGNAL(toggled(bool)),imagePreviewWindow->worker(),SLOT(setCorrectionMedian(bool)));
    connect(correctionPolarizationCheckBox,SIGNAL(toggled(bool)),imagePreviewWindow->worker(),SLOT(setCorrectionPolarization(bool)));
    connect(correctionFluxCheckBox,SIGNAL(toggled(bool)),imagePreviewWindow->worker(),SLOT(setCorrectionFlux(bool)));
    connect(correctionExposureCheckBox,SIGNAL(toggled(bool)),imagePreviewWindow->worker(),SLOT(setCorrectionExposure(bool)));
    connect(centerImageAction, SIGNAL(triggered()), imagePreviewWindow->worker(), SLOT(centerImage()));
    connect(showWeightCenterAction, SIGNAL(toggled(bool)), imagePreviewWindow->worker(), SLOT(showWeightCenter(bool)));
    connect(integratePushButton,SIGNAL(clicked()),this,SLOT(applyAnalytics()));
    connect(applyPlaneMarkerPushButton,SIGNAL(clicked()),this,SLOT(applyPlaneMarker()));
    connect(applySelectionPushButton,SIGNAL(clicked()),this,SLOT(applySelection()));
    connect(selectionModeComboBox,SIGNAL(currentTextChanged(QString)),this,SLOT(setApplyMode(QString)));
    connect(correctionNoiseDoubleSpinBox,SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setNoise(double)));
//    connect(noiseCorrectionMaxDoubleSpinBox,SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setThresholdNoiseHigh(double)));
//    connect(postCorrectionMinDoubleSpinBox,SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setThresholdPostCorrectionLow(double)));
//    connect(postCorrectionMaxDoubleSpinBox,SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setThresholdPostCorrectionHigh(double)));
    connect(imagePreviewWindow->worker(), SIGNAL(noiseLowChanged(double)), correctionNoiseDoubleSpinBox, SLOT(setValue(double)));
    connect(saveImageAct, SIGNAL(triggered()), this, SLOT(saveImageFunction()));
    connect(this, SIGNAL(saveImage(QString)), imagePreviewWindow->worker(),SLOT(saveImage(QString)));
    connect(imageScreenshotAct, SIGNAL(triggered()), this, SLOT(takeImageScreenshotFunction()));
    connect(this, SIGNAL(takeImageScreenshot(QString)), imagePreviewWindow->worker(),SLOT(takeScreenShot(QString)));
    connect(imagePreviewWindow->worker(), SIGNAL(resultFinished(QString)), outputPlainTextEdit, SLOT(setPlainText(QString)));
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(setTab(int)));
}

void MainWindow::setGenericProgressFormat(QString str)
{
    genericProgressBar->setFormat(str);
}

void MainWindow::setMemoryUsageFormat(QString str)
{
    memoryUsageProgressBar->setFormat(str);
}

void MainWindow::saveSvo()
{
    if (svo_inprocess.index.size() > 0)
    {
//        QFileDialog dialog;
//        dialog.setDefaultSuffix("svo");
        QString file_name = QFileDialog::getSaveFileName(this, tr("Save file"), working_dir, tr(".svo (*.svo);; All files (*)"));

        if (file_name != "")
        {
            QFileInfo info(file_name);
            working_dir = info.absoluteDir().path();

            // View settings
            svo_inprocess.view_mode = 0;
            svo_inprocess.view_tsf_style = 2;
            svo_inprocess.view_tsf_texture = 1;
            svo_inprocess.view_data_min = 0;
            svo_inprocess.view_data_max = 100;
            svo_inprocess.view_alpha = 0.05;
            svo_inprocess.view_brightness = 2.0;
            
            svo_inprocess.save(file_name);
        }
    }
}


void MainWindow::saveLoadedSvo()
{
    if (svo_loaded.getBrickNumber() > 0)
    {
//        QFileDialog dialog;
//        dialog.setDefaultSuffix("svo");
        QString file_name = QFileDialog::getSaveFileName(this, tr("Save file"), working_dir, tr(".svo (*.svo);; All files (*)"));

        if (file_name != "")
        {
            QFileInfo info(file_name);
            working_dir = info.absoluteDir().path();

            // View settings
            svo_loaded.view_mode = volumeRenderViewModeComboBox->currentIndex();
            svo_loaded.view_tsf_style = imagePreviewTsfAlphaComboBox->currentIndex();
            svo_loaded.view_tsf_texture = volumeRenderTsfComboBox->currentIndex();
            svo_loaded.view_data_min = volumeRenderDataMinSpinBox->value();
            svo_loaded.view_data_max = volumeRenderDataMaxSpinBox->value();
            svo_loaded.view_alpha = volumeRenderAlphaSpinBox->value();
            svo_loaded.view_brightness = volumeRenderBrightnessSpinBox->value();
            
            svo_loaded.setUB(volumeRenderWindow->worker()->getUBMatrix());
            svo_loaded.setMetaData(svoHeaderEdit->toPlainText());
            svo_loaded.save(file_name);
        }
    }
}


void MainWindow::openSvo()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Open file"), working_dir, tr(".svo (*.svo);; All files (*)"));

    if ((file_name != ""))
    {
        QFileInfo info(file_name);
        working_dir = info.absoluteDir().path();

        svo_loaded.open(file_name);
        volumeRenderWindow->worker()->setSvo(&(svo_loaded));
        
        volumeRenderViewModeComboBox->setCurrentIndex(svo_loaded.view_mode);
        imagePreviewTsfAlphaComboBox->setCurrentIndex(svo_loaded.view_tsf_style);
        volumeRenderTsfComboBox->setCurrentIndex(svo_loaded.view_tsf_texture);
        volumeRenderAlphaSpinBox->setValue(svo_loaded.view_alpha);
        volumeRenderBrightnessSpinBox->setValue(svo_loaded.view_brightness);
        volumeRenderDataMinSpinBox->setValue(svo_loaded.view_data_min);
        volumeRenderDataMaxSpinBox->setValue(svo_loaded.view_data_max);
        
        UBMatrix<double> UB;
        
        UB = svo_loaded.getUB();
        
        if (UB.size() == 3*3)
        {
            volumeRenderWindow->worker()->setUBMatrix(UB);
        
            alphaNormSpinBox->setValue(UB.alpha()*180.0/pi);
            betaNormSpinBox->setValue(UB.beta()*180.0/pi);
            gammaNormSpinBox->setValue(UB.gamma()*180.0/pi);
            
            aNormSpinBox->setValue(UB.a());
            bNormSpinBox->setValue(UB.b());
            cNormSpinBox->setValue(UB.c());
        }
        
        svoHeaderEdit->setDocumentTitle(file_name);
        svoHeaderEdit->setPlainText(svo_loaded.getMetaData());

        print("\n["+QString(this->metaObject()->className())+"] Loaded file: \""+file_name+"\"");
        
        setWindowTitle(tr("Nebula[*] (")+file_name+")");
    }
}



void MainWindow::initMenus()
{
    mainMenu = new QMenuBar;
    viewMenu = new QMenu(tr("V&iew"));
    helpMenu = new QMenu(tr("&Help"));

    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
    helpMenu->addAction(aboutOpenCLAct);
    helpMenu->addAction(aboutOpenGLAct);

    mainMenu->addMenu(viewMenu);
    mainMenu->addSeparator();
    mainMenu->addMenu(helpMenu);
}



void MainWindow::initGUI()
{
    /* Top Widget */
//    {
//        topWidget = new QWidget;

        // Layout
//        QGridLayout * topLayout = new QGridLayout;
//        gridLayout->setHorizontalSpacing(5);
//        gridLayout->setContentsMargins(5,5,5,5);
//        topLayout->addWidget(mainMenu,0,0,1,1);
//        topWidget->setLayout(topLayout);
//    }

    
    /*      File Select Widget       */
    {
        setFilesWidget = new QWidget;

        // Toolbar
//        fileSelectionToolBar = new QToolBar(tr("File selection toolbar"));
        fileFilter = new QLineEdit;
//        fileSelectionToolBar->addWidget(fileSelectionFilter);
        
        // File browser
//        fileBrowserWidget = new QWidget;
        fileSelectionModel  = new FileSelectionModel;
        fileSelectionModel->setRootPath(QDir::rootPath());

        fileTreeView = new FileTreeView;
        fileTreeView->setModel(fileSelectionModel);
        
        loadPathsPushButton = new QPushButton;//(QIcon(":/art/download.png"),"Load selected files"); //QIcon(":/art/rotate_down.png"),
        loadPathsPushButton->setIcon(QIcon(":/art/download.png"));
        loadPathsPushButton->setIconSize(QSize(86,86));
//        connect(loadPathsPushButton, SIGNAL(clicked()), this, SLOT(loadPaths()));
        
//        connect(fileTreeView, SIGNAL(fileChanged(QString)), this, SLOT(setHeader(QString)));
        
//        {
//            QGridLayout * gridLayout = new QGridLayout;
//            gridLayout->setHorizontalSpacing(5);
//            gridLayout->setVerticalSpacing(0);
//            gridLayout->setContentsMargins(5,5,5,5);
//            gridLayout->addWidget(fileSelectionTree,0,0,1,1);
    
//            fileBrowserWidget->setLayout(gridLayout);
//        }
        
        // Layout
        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(5,5,5,5);
        gridLayout->addWidget(fileFilter,0,0,1,2);
        gridLayout->addWidget(fileTreeView,2,0,1,2);
        gridLayout->addWidget(loadPathsPushButton,3,0,1,2);
        setFilesWidget->setLayout(gridLayout);
    }

    /*      3D View widget      */
    {
        QSurfaceFormat format_gl;
        format_gl.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        format_gl.setSwapInterval(1);
        format_gl.setSamples(16);
        format_gl.setRedBufferSize(8);
        format_gl.setGreenBufferSize(8);
        format_gl.setBlueBufferSize(8);
        format_gl.setAlphaBufferSize(8);

        volumeRenderWindow = new VolumeRenderWindow();
        volumeRenderWindow->setMultiThreading(true);
        volumeRenderWindow->setSharedWindow(sharedContextWindow);
        volumeRenderWindow->setFormat(format_gl);
        volumeRenderWindow->setOpenCLContext(context_cl);
        volumeRenderWindow->initializeWorker();

        volumeRenderWidget = QWidget::createWindowContainer(volumeRenderWindow);
        volumeRenderWidget->setFocusPolicy(Qt::TabFocus);

        // Toolbar
        viewToolBar = new QToolBar(tr("3D view"));
        viewToolBar->addAction(openSvoAct);
        viewToolBar->addAction(saveLoadedSvoAct);
        
        viewToolBar->addSeparator();
        viewToolBar->addAction(projectionAct);
        viewToolBar->addAction(scalebarAct);
        viewToolBar->addAction(labFrameAct);
        viewToolBar->addAction(dataStructureAct);
        
        viewToolBar->addSeparator();
        
        viewToolBar->addAction(integrate2DAct);
        viewToolBar->addAction(logIntegrate2DAct);
        viewToolBar->addSeparator();
        
        viewToolBar->addAction(alignLabXtoSliceXAct);
        viewToolBar->addAction(alignLabYtoSliceYAct);
        viewToolBar->addAction(alignLabZtoSliceZAct);
        
        viewToolBar->addAction(rotateLeftAct);
        viewToolBar->addAction(rotateRightAct);
        viewToolBar->addAction(rotateDownAct);
        viewToolBar->addAction(rotateUpAct);
        viewToolBar->addAction(rollCW);
        viewToolBar->addAction(rollCCW);
        viewToolBar->addAction(alignSliceToLabAct);
        viewToolBar->addSeparator();
        
        viewToolBar->addAction(backgroundAct);
        viewToolBar->addAction(screenshotAct);
        
        // Volume render QMainWindow
        volumeRenderMainWindow = new QMainWindow;
        volumeRenderMainWindow->setCentralWidget(volumeRenderWidget);
        volumeRenderMainWindow->addToolBar(Qt::TopToolBarArea, viewToolBar);
    }

    /*
     * QDockWidgets
     * */
    
    /* Image browser widget */
    {
        QSurfaceFormat format_gl;
        format_gl.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        format_gl.setSwapInterval(1);
        format_gl.setSamples(16);
        format_gl.setRedBufferSize(8);
        format_gl.setGreenBufferSize(8);
        format_gl.setBlueBufferSize(8);
        format_gl.setAlphaBufferSize(8);
        
        imagePreviewWindow = new ImagePreviewWindow();
        imagePreviewWindow->setMultiThreading(true);
        imagePreviewWindow->setSharedWindow(sharedContextWindow);
        imagePreviewWindow->setFormat(format_gl);
        imagePreviewWindow->setOpenCLContext(context_cl);
        imagePreviewWindow->initializeWorker();
        
        imageDisplayWidget = QWidget::createWindowContainer(imagePreviewWindow);
        imageDisplayWidget->setFocusPolicy(Qt::TabFocus);
        
        imageMainWindow = new QMainWindow;
        imageMainWindow->setAnimated(false);

        // Toolbar
        
        
//        imageBatchPrevButton = new QPushButton;
//        imageBatchPrevButton->setIcon(QIcon(":art/fast_back.png"));
//        imageBatchPrevButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
//        connect(imageBatchPrevButton, SIGNAL(clicked()), this, SLOT(batchBackward()));

//        imagePrevButton = new QPushButton;
//        imagePrevButton->setIcon(QIcon(":art/back.png"));
//        imagePrevButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
//        connect(imagePrevButton, SIGNAL(clicked()), this, SLOT(prevFrame()));

//        imageBatchNextButton = new QPushButton;
//        imageBatchNextButton->setIcon(QIcon(":art/fast_forward.png"));
//        imageBatchNextButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
//        connect(imageBatchNextButton, SIGNAL(clicked()), this, SLOT(batchForward()));

//        imageNextButton = new QPushButton;
//        imageNextButton->setIcon(QIcon(":art/forward.png"));
//        imageNextButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
//        connect(imageNextButton, SIGNAL(clicked()), this, SLOT(nextFrame()));

//        nextSeriesButton = new QPushButton;
//        nextSeriesButton->setIcon(QIcon(":art/forward.png"));
//        nextSeriesButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
//        connect(nextSeriesButton, SIGNAL(clicked()), this, SLOT(nextSeries()));

//        prevSeriesButton = new QPushButton;
//        prevSeriesButton->setIcon(QIcon(":art/back.png"));
//        prevSeriesButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
//        connect(prevSeriesButton, SIGNAL(clicked()), this, SLOT(prevSeries()));

//        imageSpinBox = new QSpinBox;
//        imageSpinBox->setRange(0,1);
//        connect(imageSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setFrame(int)));        
        
//        removeCurrentPushButton = new QPushButton(QIcon(":/art/kill.png"),"Remove frame");
//        connect(removeCurrentPushButton, SIGNAL(clicked()), this, SLOT(removeImage()));
//        connect(this, SIGNAL(pathRemoved(QString)), fileSelectionModel, SLOT(removeFile(QString)));

        
    
        saveProjectAction = new QAction(QIcon(":/art/save.png"), tr("Save project"), this);
        loadProjectAction = new QAction(QIcon(":/art/open.png"), tr("Load project"), this);

        squareAreaSelectAlphaAction = new QAction(QIcon(":/art/select.png"), tr("Toggle pixel selection"), this);
        squareAreaSelectAlphaAction->setCheckable(true);
        squareAreaSelectAlphaAction->setChecked(false);

        squareAreaSelectBetaAction = new QAction(QIcon(":/art/select2.png"), tr("Toggle background selection"), this);
        squareAreaSelectBetaAction->setCheckable(true);
        squareAreaSelectBetaAction->setChecked(false);

        centerImageAction = new QAction(QIcon(":/art/center.png"), tr("Center image"), this);
        centerImageAction->setCheckable(false);

        showWeightCenterAction = new QAction(QIcon(":/art/weight_center.png"), tr("Toggle weight center visual"), this);
        showWeightCenterAction->setCheckable(true);
        showWeightCenterAction->setChecked(true);
        
        imageToolBar = new QToolBar("Image");
        imageToolBar->addAction(saveProjectAction);
        imageToolBar->addAction(loadProjectAction);
        imageToolBar->addAction(centerImageAction);
        imageToolBar->addAction(showWeightCenterAction);
        imageToolBar->addSeparator();
        imageToolBar->addAction(imageScreenshotAct);
        imageToolBar->addAction(saveImageAct);
//        imageToolBar->addWidget(pathLineEdit);
    
//        connect(showWeightCenterAction, SIGNAL(toggled(bool)), imagePreviewWindow->worker(), SLOT(showWeightCenter(bool)));
//        connect(squareAreaSelectAlphaAction, SIGNAL(toggled(bool)), imagePreviewWindow->worker(), SLOT(setSelectionAlphaActive(bool)));
//        connect(squareAreaSelectBetaAction, SIGNAL(toggled(bool)), imagePreviewWindow->worker(), SLOT(setSelectionBetaActive(bool)));
        connect(imagePreviewWindow->worker(), SIGNAL(selectionAlphaChanged(bool)), squareAreaSelectAlphaAction, SLOT(setChecked(bool)));
        connect(imagePreviewWindow->worker(), SIGNAL(selectionBetaChanged(bool)), squareAreaSelectBetaAction, SLOT(setChecked(bool)));

        imageMainWindow->addToolBar(Qt::TopToolBarArea, imageToolBar);

        // Buttons
//        setFileButton = new QPushButton;
//        setFileButton->setIcon(QIcon(":/art/proceed.png"));
//        setFileButton->setIconSize(QSize(24,24));
//        setFileButton->setText("Set ");

//        readFileButton = new QPushButton;
//        readFileButton->setIcon(QIcon(":/art/proceed.png"));
//        readFileButton->setIconSize(QSize(24,24));
//        readFileButton->setText("Read ");
//        readFileButton->setEnabled(false);

//        projectFileButton = new QPushButton;
//        projectFileButton->setIcon(QIcon(":/art/proceed.png"));
//        projectFileButton->setIconSize(QSize(24,24));
//        projectFileButton->setText("Project ");
//        projectFileButton->setEnabled(false);

//        allInOneButton = new QPushButton;
//        allInOneButton->setIcon(QIcon(":/art/fast_proceed.png"));
//        allInOneButton->setText("All (reduced memory consumption) ");
//        allInOneButton->setIconSize(QSize(24,24));

//        killButton = new QPushButton;
//        killButton->setIcon(QIcon(":/art/kill.png"));
//        killButton->setText("Kill ");
//        killButton->setIconSize(QSize(24,24));
//        killButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

//        toolChainWidget = new QWidget;
//        QGridLayout * gridLayout = new QGridLayout;
//        gridLayout->setHorizontalSpacing(5);
//        gridLayout->setVerticalSpacing(2);
//        gridLayout->setContentsMargins(5,5,5,5);
//        gridLayout->setColumnStretch(1,1);
//        gridLayout->setColumnStretch(2,1);
//        gridLayout->setColumnStretch(3,1);
//        gridLayout->setColumnStretch(4,1);
//        gridLayout->addWidget(setFileButton,0,1,1,1);
//        gridLayout->addWidget(readFileButton,0,2,1,1);
//        gridLayout->addWidget(projectFileButton,0,3,1,1);
//        gridLayout->addWidget(killButton,0,4,2,1);
//        gridLayout->addWidget(allInOneButton,1,1,1,3);
//        toolChainWidget->setLayout(gridLayout);
        
//        QGridLayout * imageLayout = new QGridLayout;
//        imageLayout->setRowStretch(1,1);
//        imageLayout->addWidget(toolChainWidget,0,0,1,8);
//        imageLayout->addWidget(imageDisplayWidget,1,0,1,8);
//        imageLayout->addWidget(imageBatchPrevButton,2,0,1,2);
//        imageLayout->addWidget(imagePrevButton,2,2,1,1);
//        imageLayout->addWidget(imageSpinBox,2,3,1,2);
//        imageLayout->addWidget(imageNextButton,2,5,1,1);
//        imageLayout->addWidget(imageBatchNextButton,2,6,1,2);
//        imageLayout->addWidget(prevSeriesButton,3,0,1,2);
//        imageLayout->addWidget(nextSeriesButton,3,6,1,2);
//        imageLayout->addWidget(removeCurrentPushButton,3,2,1,4);
        
//        imageCentralWidget = new QWidget;
//        imageCentralWidget->setLayout(imageLayout);
        imageMainWindow->setCentralWidget(imageDisplayWidget);
    }
    
    /* Image browser display widget */
    {
        imageModeComboBox = new QComboBox;
        imageModeComboBox->addItem("Normal");
        imageModeComboBox->addItem("Variance");
        imageModeComboBox->addItem("Skewness");
    
        imagePreviewTsfTextureComboBox = new QComboBox;
        imagePreviewTsfTextureComboBox->addItem(trUtf8("Rainbow"));
        imagePreviewTsfTextureComboBox->addItem(trUtf8("Hot"));
        imagePreviewTsfTextureComboBox->addItem(trUtf8("Hsv"));
        imagePreviewTsfTextureComboBox->addItem(trUtf8("Galaxy"));
        imagePreviewTsfTextureComboBox->addItem(trUtf8("Binary"));
        imagePreviewTsfTextureComboBox->addItem(trUtf8("Yranib"));
    
        imagePreviewTsfAlphaComboBox = new QComboBox;
        imagePreviewTsfAlphaComboBox->addItem("Linear");
        imagePreviewTsfAlphaComboBox->addItem("Exponential");
        imagePreviewTsfAlphaComboBox->addItem("Uniform");
        imagePreviewTsfAlphaComboBox->addItem("Opaque");
    
        imagePreviewDataMinDoubleSpinBox = new QDoubleSpinBox;
        imagePreviewDataMinDoubleSpinBox->setRange(-1e9,1e9);
        imagePreviewDataMinDoubleSpinBox->setAccelerated(true);
        imagePreviewDataMinDoubleSpinBox->setPrefix("Data min: ");
    
        imagePreviewDataMaxDoubleSpinBox = new QDoubleSpinBox;
        imagePreviewDataMaxDoubleSpinBox->setRange(-1e9,1e9);
        imagePreviewDataMaxDoubleSpinBox->setAccelerated(true);
        imagePreviewDataMaxDoubleSpinBox->setPrefix("Data max: ");
        
        imagePreviewLogCheckBox = new QCheckBox("Log");
    
        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(5,5,5,5);
        gridLayout->setRowStretch(5,1);
        gridLayout->addWidget(imageModeComboBox,0,1,1,2);
        gridLayout->addWidget(imagePreviewTsfTextureComboBox,1,1,1,1);
        gridLayout->addWidget(imagePreviewTsfAlphaComboBox,1,2,1,1);
        gridLayout->addWidget(imagePreviewDataMinDoubleSpinBox,2,1,1,2);
        gridLayout->addWidget(imagePreviewDataMaxDoubleSpinBox,3,1,1,2);
        gridLayout->addWidget(imagePreviewLogCheckBox,4,1,1,1);
        
        imageSettingsWidget = new QWidget;
        imageSettingsWidget->setLayout(gridLayout);
    
        imageSettingsDock =  new QDockWidget("Display settings");
        imageSettingsDock->setWidget(imageSettingsWidget);
//        imageSettingsDock->setFixedHeight(imageSettingsWidget->minimumSizeHint().height()*1.2);
        imageMainWindow->addDockWidget(Qt::LeftDockWidgetArea, imageSettingsDock);
        
        connect(imagePreviewTsfTextureComboBox, SIGNAL(currentIndexChanged(int)), imagePreviewWindow->worker(), SLOT(setTsfTexture(int)));
        connect(imagePreviewTsfAlphaComboBox, SIGNAL(currentIndexChanged(int)), imagePreviewWindow->worker(), SLOT(setTsfAlpha(int)));
        connect(imagePreviewDataMinDoubleSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setDataMin(double)));
        connect(imagePreviewDataMaxDoubleSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setDataMax(double)));
        connect(imagePreviewLogCheckBox, SIGNAL(toggled(bool)), imagePreviewWindow->worker(), SLOT(setLog(bool)));
//        connect(imageModeComboBox, SIGNAL(currentIndexChanged(int)), imagePreviewWindow->worker(), SLOT(setMode(int)));
//        connect(saveProjectAction, SIGNAL(triggered()), this, SLOT(saveProject()));
//        connect(loadProjectAction, SIGNAL(triggered()), this, SLOT(loadProject()));
//        connect(this, SIGNAL(imageChanged(ImageInfo)), imagePreviewWindow->worker(), SLOT(setFrame(ImageInfo)));
//        connect(centerImageAction, SIGNAL(triggered()), imagePreviewWindow->worker(), SLOT(centerImage()));
        connect(this, SIGNAL(centerImage()), imagePreviewWindow->worker(), SLOT(centerImage()));
        
//        connect(imagePreviewWindow->worker(), SIGNAL(imageChanged(ImageInfo)), this, SLOT(setImage(ImageInfo)));
//        connect(imagePreviewWindow->worker(), SIGNAL(selectionChanged(Selection)), this, SLOT(setSeriesSelection(Selection)));
        
    }
    
    // Navigation dock widget
    {
        pathLineEdit = new QLineEdit("/path/to/file");
        pathLineEdit->setReadOnly(true);
        connect(this, SIGNAL(pathChanged(QString)), pathLineEdit, SLOT(setText(QString)));
        
        nextFramePushButton = new QPushButton(QIcon(":/art/forward.png"),"Next");
        previousFramePushButton = new QPushButton(QIcon(":/art/back.png"),"Prev");
        batchForwardPushButton = new QPushButton(QIcon(":/art/fast_forward.png"), "Skip");
        batchBackwardPushButton = new QPushButton(QIcon(":/art/fast_back.png"), "Skip");
        nextSeriesPushButton = new QPushButton(QIcon(":/art/next_series.png"),"Next series");
        nextSeriesPushButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        prevSeriesPushButton = new QPushButton(QIcon(":/art/prev_series.png"),"Prev series");
        prevSeriesPushButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        
        removeCurrentPushButton = new QPushButton(QIcon(":/art/kill.png"),"Remove frame");
        
        imageSpinBox = new QSpinBox;
        imageSpinBox->setPrefix("Frame: ");
        
        batchSizeSpinBox = new QSpinBox;
        batchSizeSpinBox->setPrefix("Skip size: ");
        
//        connect(batchSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setBatchSize(int)));
        
        generalProgressBar = new QProgressBar;
        generalProgressBar->hide();
        generalProgressBar->setFormat("%v of %m");
    
        QGridLayout * gridLayout = new QGridLayout;
        
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(5,5,5,5);
        gridLayout->setRowStretch(5,1);
        gridLayout->addWidget(batchBackwardPushButton,0,0,1,1);
        gridLayout->addWidget(previousFramePushButton,0,1,1,1);
        gridLayout->addWidget(imageSpinBox,0,2,1,2);
        gridLayout->addWidget(nextFramePushButton,0,4,1,1);
        gridLayout->addWidget(batchForwardPushButton,0,5,1,1);
        gridLayout->addWidget(prevSeriesPushButton,1,0,2,2);
        gridLayout->addWidget(batchSizeSpinBox,1,2,1,2);
        gridLayout->addWidget(nextSeriesPushButton,1,4,2,2);
        gridLayout->addWidget(removeCurrentPushButton, 2, 2, 1 , 2);
        gridLayout->addWidget(pathLineEdit,3,0,1,6);
        gridLayout->addWidget(generalProgressBar, 4, 0, 1 , 6);
        
        navigationWidget = new QWidget;
        navigationWidget->setLayout(gridLayout);
    
        navigationDock =  new QDockWidget("Navigation");
    //    navigationDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
        navigationDock->setWidget(navigationWidget);
//        navigationDock->setFixedHeight(navigationWidget->minimumSizeHint().height()*1.2);
        imageMainWindow->addDockWidget(Qt::BottomDockWidgetArea, navigationDock);
    }
    
    // Operations dock widget
    {
        applyPlaneMarkerPushButton  = new QPushButton(QIcon(":/art/lsqplane.png"),"Apply markers");
        applySelectionPushButton  = new QPushButton(QIcon(":/art/select.png"),"Apply selection");
        integratePushButton = new QPushButton(QIcon(":/art/proceed.png"),"Analyze frames");
    
        selectionModeComboBox = new QComboBox;
        selectionModeComboBox->addItem("Series");
        selectionModeComboBox->addItem("Set");
    
        QGridLayout *gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(5,5,5,5);
        gridLayout->setRowStretch(4,1);
        gridLayout->addWidget(selectionModeComboBox, 0, 0, 1, 2);
        gridLayout->addWidget(applyPlaneMarkerPushButton, 1, 0, 1 , 2);
        gridLayout->addWidget(applySelectionPushButton, 2, 0, 1 , 2);
        gridLayout->addWidget(integratePushButton, 3, 0, 1 , 2);
    
        selectionWidget = new QWidget;
        selectionWidget->setLayout(gridLayout);
    
        selectionDock =  new QDockWidget("Frame-by-frame operations");
//        selectionDock->setFixedHeight(selectionWidget->minimumSizeHint().height()*1.2);
        selectionDock->setWidget(selectionWidget);
        imageMainWindow->addDockWidget(Qt::RightDockWidgetArea, selectionDock); 
    }
    
    // Corrections dock widget
    {
        traceSetPushButton = new QPushButton("Trace set");
        traceTextureCheckBox = new QCheckBox("Show");
        
        correctionNoiseDoubleSpinBox = new QDoubleSpinBox;
        correctionNoiseDoubleSpinBox->setRange(0,1e4);
        
        correctionClutterSpinBox = new QSpinBox;
        correctionClutterSpinBox->setRange(0,100);
        correctionClutterSpinBox->setSuffix(" units");
                
        correctionMedianSpinBox = new QSpinBox;
        correctionMedianSpinBox->setRange(0,100);
        correctionMedianSpinBox->setPrefix("n x n: ");
    
        correctionPlaneSpinBox = new QSpinBox;
        correctionPlaneSpinBox->setRange(3,20);
        correctionPlaneSpinBox->setPrefix("Samples: ");
                
        correctionNoiseCheckBox = new QCheckBox("Flat b/g subtract");
        correctionPlaneCheckBox = new QCheckBox("Planar b/g subtract");
        correctionClutterCheckBox = new QCheckBox("Clutter removal");
        correctionMedianCheckBox = new QCheckBox("Median filter");
        correctionLorentzCheckBox = new QCheckBox("Lorentz correction");
        correctionPolarizationCheckBox = new QCheckBox("Polarization correction");
        correctionFluxCheckBox = new QCheckBox("Flux normalization");
        correctionExposureCheckBox = new QCheckBox("Exposure time normalization");
        
        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(5,5,5,5);
        gridLayout->setRowStretch(9,1);
        gridLayout->addWidget(traceSetPushButton,0,0,1,1);
        gridLayout->addWidget(traceTextureCheckBox,0,1,1,1);
        gridLayout->addWidget(correctionPlaneCheckBox,1,0,1,1);
        gridLayout->addWidget(correctionPlaneSpinBox,1,1,1,1);
        gridLayout->addWidget(correctionNoiseCheckBox,2,0,1,1);
        gridLayout->addWidget(correctionNoiseDoubleSpinBox,2,1,1,1);
        gridLayout->addWidget(correctionClutterCheckBox,3,0,1,1);
        gridLayout->addWidget(correctionClutterSpinBox,3,1,1,1);
        gridLayout->addWidget(correctionMedianCheckBox,4,0,1,1);
        gridLayout->addWidget(correctionMedianSpinBox,4,1,1,1);
        gridLayout->addWidget(correctionLorentzCheckBox,5,0,1,2);
        gridLayout->addWidget(correctionPolarizationCheckBox,6,0,1,2);
        gridLayout->addWidget(correctionFluxCheckBox,7,0,1,2);
        gridLayout->addWidget(correctionExposureCheckBox,8,0,1,2);
        
        
        correctionWidget = new QWidget;
        correctionWidget->setLayout(gridLayout);
        
        correctionDock =  new QDockWidget("Frame-by-frame corrections");
    //    correctionDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
        correctionDock->setWidget(correctionWidget);
//        correctionDock->setFixedHeight(correctionWidget->minimumSizeHint().height()*1.2);
        imageMainWindow->addDockWidget(Qt::LeftDockWidgetArea, correctionDock);
        
        
        // // //
        
//        noiseCorrectionMinDoubleSpinBox = new QDoubleSpinBox;
//        noiseCorrectionMinDoubleSpinBox->setRange(-1e6,1e6);
//        noiseCorrectionMinDoubleSpinBox->setPrefix("Noise: ");
        
//        noiseCorrectionMaxDoubleSpinBox = new QDoubleSpinBox;
//        noiseCorrectionMaxDoubleSpinBox->setRange(-1e6,1e6);
        
//        postCorrectionMinDoubleSpinBox = new QDoubleSpinBox;
//        postCorrectionMinDoubleSpinBox->setRange(-1e6,1e6);
//        postCorrectionMinDoubleSpinBox->setPrefix("PCT: ");
        
//        postCorrectionMaxDoubleSpinBox = new QDoubleSpinBox;
//        postCorrectionMaxDoubleSpinBox->setRange(-1e6,1e6);
        
//        correctionLorentzCheckBox = new QCheckBox("Lorentz correction");
//        autoBackgroundCorrectionCheckBox = new QCheckBox("Automatic background subtraction");
        
//        connect(correctionLorentzCheckBox, SIGNAL(toggled(bool)), imagePreviewWindow->worker(), SLOT(setCorrection(bool)));
//        connect(autoBackgroundCorrectionCheckBox, SIGNAL(toggled(bool)), imagePreviewWindow->worker(), SLOT(setAutoBackgroundCorrection(bool)));
//        connect(imagePreviewWindow->worker(), SIGNAL(noiseLowChanged(double)), noiseCorrectionMinDoubleSpinBox, SLOT(setValue(double)));
        
        
//        QGridLayout * correctionLayout = new QGridLayout;
//        correctionLayout->addWidget(noiseCorrectionMinDoubleSpinBox,0,0,1,2);
//        correctionLayout->addWidget(correctionLorentzCheckBox,1,0,1,2);
        
//        correctionWidget = new QWidget;
//        correctionWidget->setLayout(correctionLayout);
        
//        correctionDock =  new QDockWidget("Background");
//        correctionDock->setWidget(correctionWidget);
//        correctionDock->setFixedHeight(correctionWidget->minimumSizeHint().height()*1.2);
//        imageMainWindow->addDockWidget(Qt::RightDockWidgetArea, correctionDock);
        
        
//        connect(this->noiseCorrectionMinDoubleSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setThresholdNoiseLow(double)));
//        connect(this->noiseCorrectionMaxDoubleSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setThresholdNoiseHigh(double)));
//        connect(this->postCorrectionMinDoubleSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setThresholdPostCorrectionLow(double)));
//        connect(this->postCorrectionMaxDoubleSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->worker(), SLOT(setThresholdPostCorrectionHigh(double)));
    }
    
    
    
    
    /* Graphics dock widget */
    {
        QLabel * label_texture= new QLabel(QString("Texture "));
        QLabel * label_quality = new QLabel(QString("Texture quality: "));
        QLabel * label_mode = new QLabel(QString("View mode: "));

        volumeRenderDataMinSpinBox = new QDoubleSpinBox;
        volumeRenderDataMinSpinBox->setDecimals(2);
        volumeRenderDataMinSpinBox->setRange(0, 1e9);
        volumeRenderDataMinSpinBox->setSingleStep(1);
        volumeRenderDataMinSpinBox->setAccelerated(1);
        volumeRenderDataMinSpinBox->setPrefix("Data min: ");

        volumeRenderDataMaxSpinBox = new QDoubleSpinBox;
        volumeRenderDataMaxSpinBox->setDecimals(1);
        volumeRenderDataMaxSpinBox->setRange(0, 1e9);
        volumeRenderDataMaxSpinBox->setSingleStep(1);
        volumeRenderDataMaxSpinBox->setAccelerated(1);
        volumeRenderDataMaxSpinBox->setPrefix("Data max: ");

        volumeRenderAlphaSpinBox = new QDoubleSpinBox;
        volumeRenderAlphaSpinBox->setDecimals(4);
        volumeRenderAlphaSpinBox->setRange(0, 10);
        volumeRenderAlphaSpinBox->setSingleStep(0.1);
        volumeRenderAlphaSpinBox->setAccelerated(1);
        volumeRenderAlphaSpinBox->setPrefix("Alpha: ");

        volumeRenderBrightnessSpinBox = new QDoubleSpinBox;
        volumeRenderBrightnessSpinBox->setDecimals(4);
        volumeRenderBrightnessSpinBox->setRange(0, 10);
        volumeRenderBrightnessSpinBox->setSingleStep(0.1);
        volumeRenderBrightnessSpinBox->setAccelerated(1);
        volumeRenderBrightnessSpinBox->setPrefix("Brightness: ");
        
        volumeRenderViewModeComboBox = new QComboBox;
        volumeRenderViewModeComboBox->addItem(trUtf8("Integrate"));
        volumeRenderViewModeComboBox->addItem(trUtf8("Blend"));
        volumeRenderViewModeComboBox->addItem(trUtf8("Slice"));
        
        volumeRenderTsfComboBox = new QComboBox;
        volumeRenderTsfComboBox->addItem(trUtf8("Rainbow"));
        volumeRenderTsfComboBox->addItem(trUtf8("Hot"));
        volumeRenderTsfComboBox->addItem(trUtf8("Hsv"));
        volumeRenderTsfComboBox->addItem(trUtf8("Galaxy"));
        volumeRenderTsfComboBox->addItem(trUtf8("Binary"));
        volumeRenderTsfComboBox->addItem(trUtf8("Yranib"));

        volumeRenderTsfAlphaComboBox = new QComboBox;
        volumeRenderTsfAlphaComboBox->addItem(trUtf8("Linear"));
        volumeRenderTsfAlphaComboBox->addItem(trUtf8("Exponential"));
        volumeRenderTsfAlphaComboBox->addItem(trUtf8("Uniform"));
        
        volumeRenderLogCheckBox = new QCheckBox("Log");
        connect(volumeRenderLogCheckBox, SIGNAL(toggled(bool)), volumeRenderWindow->worker(), SLOT(setLog(bool)));
        
        
        qualitySlider = new QSlider(Qt::Horizontal);
        qualitySlider->setRange(1,100);
        qualitySlider->setToolTip("Set texture resolution");
        qualitySlider->setTickPosition(QSlider::NoTicks);

        graphicsDockWidget = new QDockWidget(tr("Display settings"), this);
        graphicsWidget = new QWidget;

        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(5,5,5,5);
        gridLayout->setRowStretch(8,1);
        gridLayout->addWidget(label_mode,0,0,1,2);
        gridLayout->addWidget(volumeRenderViewModeComboBox,0,2,1,2);
        gridLayout->addWidget(label_texture,1,0,1,2);
        gridLayout->addWidget(volumeRenderTsfComboBox,1,2,1,1);
        gridLayout->addWidget(volumeRenderTsfAlphaComboBox,1,3,1,1);
        gridLayout->addWidget(volumeRenderDataMinSpinBox,2,0,1,4);
        gridLayout->addWidget(volumeRenderDataMaxSpinBox,3,0,1,4);
        gridLayout->addWidget(volumeRenderAlphaSpinBox,4,0,1,4);
        gridLayout->addWidget(volumeRenderBrightnessSpinBox,5,0,1,4);
        gridLayout->addWidget(label_quality,6,0,1,2);
        gridLayout->addWidget(qualitySlider,6,2,1,2);
        gridLayout->addWidget(volumeRenderLogCheckBox,7,0,1,2);
                    
        graphicsWidget->setLayout(gridLayout);
        graphicsDockWidget->setFixedHeight(graphicsWidget->minimumSizeHint().height()*1.1);
        graphicsDockWidget->setWidget(graphicsWidget);
        viewMenu->addAction(graphicsDockWidget->toggleViewAction());
        volumeRenderMainWindow->addDockWidget(Qt::LeftDockWidgetArea, graphicsDockWidget);
    }
    
    /* Unitcell dock widget */
    {
        unitCellDock = new QDockWidget(tr("UB matrix"), this);
        unitCellWidget = new QWidget;
        
        // Real space unit cell
        aNormSpinBox = new QDoubleSpinBox;
        aNormSpinBox->setPrefix("a: ");
        bNormSpinBox = new QDoubleSpinBox;
        bNormSpinBox->setPrefix("b: ");
        cNormSpinBox = new QDoubleSpinBox;
        cNormSpinBox->setPrefix("c: ");
        
        alphaNormSpinBox = new QDoubleSpinBox;
        alphaNormSpinBox->setPrefix("α: ");
        alphaNormSpinBox->setRange(0,180);
        alphaNormSpinBox->setSuffix(" °");
        betaNormSpinBox = new QDoubleSpinBox;
        betaNormSpinBox->setPrefix("β: ");
        betaNormSpinBox->setRange(0,180);
        betaNormSpinBox->setSuffix(" °");
        gammaNormSpinBox = new QDoubleSpinBox;
        gammaNormSpinBox->setPrefix("γ: ");
        gammaNormSpinBox->setRange(0,180);
        gammaNormSpinBox->setSuffix(" °");
        
        // Reciprocal space unit cell
        aStarSpinBox = new QDoubleSpinBox;
        bStarSpinBox = new QDoubleSpinBox;
        cStarSpinBox = new QDoubleSpinBox;
        
        alphaStarSpinBox = new QDoubleSpinBox;
        betaStarSpinBox = new QDoubleSpinBox;
        gammaStarSpinBox = new QDoubleSpinBox;
        
        // Rotation
        phiSpinBox = new QDoubleSpinBox;
        kappaSpinBox = new QDoubleSpinBox;
        omegaSpinBox = new QDoubleSpinBox;
        
        // Positioning
        hSpinBox = new QSpinBox;
        hSpinBox->setRange(-1e3,1e3);
        hSpinBox->setPrefix("h: ");
        kSpinBox = new QSpinBox;
        kSpinBox->setRange(-1e3,1e3);
        kSpinBox->setPrefix("k: ");
        lSpinBox = new QSpinBox;
        lSpinBox->setRange(-1e3,1e3);
        lSpinBox->setPrefix("l: ");
        
        alignAlongAStarButton = new QPushButton("Align a*");
        alignAlongBStarButton = new QPushButton("Align b*");
        alignAlongCStarButton = new QPushButton("Align c*");

        connect(alignAlongAStarButton, SIGNAL(clicked()), volumeRenderWindow->worker(), SLOT(alignAStartoZ()));
        connect(alignAlongBStarButton, SIGNAL(clicked()), volumeRenderWindow->worker(), SLOT(alignBStartoZ()));
        connect(alignAlongCStarButton, SIGNAL(clicked()), volumeRenderWindow->worker(), SLOT(alignCStartoZ()));

        alignSlicetoAStarPushButton = new QPushButton("Slice a*");
        alignSlicetoBStarPushButton = new QPushButton("Slice b*");
        alignSlicetoCStarPushButton = new QPushButton("Slice c*");
        
        connect(alignSlicetoAStarPushButton, SIGNAL(clicked()), volumeRenderWindow->worker(), SLOT(alignSlicetoAStar()));
        connect(alignSlicetoBStarPushButton, SIGNAL(clicked()), volumeRenderWindow->worker(), SLOT(alignSlicetoBStar()));
        connect(alignSlicetoCStarPushButton, SIGNAL(clicked()), volumeRenderWindow->worker(), SLOT(alignSlicetoCStar()));
        
        helpCellOverlayButton = new QPushButton("Help Cell");
        rotateCellButton = new QPushButton("Rotation");
        toggleCellButton = new QPushButton("Toggle");
        
        rotateCellButton->setCheckable(true);
        rotateCellButton->setChecked(false);
        
        helpCellOverlayButton->setCheckable(true);
        helpCellOverlayButton->setChecked(true);
        
        connect(helpCellOverlayButton, SIGNAL(clicked()), volumeRenderWindow->worker(), SLOT(setMiniCell()));
        
        QGridLayout * gridLayout = new QGridLayout; 
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(5,5,5,5);
        gridLayout->setRowStretch(9,1);
        
        gridLayout->addWidget(aNormSpinBox,0,0,1,2);
        gridLayout->addWidget(bNormSpinBox,0,2,1,2);
        gridLayout->addWidget(cNormSpinBox,0,4,1,2);
        
        gridLayout->addWidget(alphaNormSpinBox,1,0,1,2);
        gridLayout->addWidget(betaNormSpinBox,1,2,1,2);
        gridLayout->addWidget(gammaNormSpinBox,1,4,1,2);
        
        gridLayout->addWidget(hSpinBox,4,0,1,2);
        gridLayout->addWidget(kSpinBox,4,2,1,2);
        gridLayout->addWidget(lSpinBox,4,4,1,2);
        
        gridLayout->addWidget(alignAlongAStarButton,5,0,1,2);
        gridLayout->addWidget(alignAlongBStarButton,5,2,1,2);
        gridLayout->addWidget(alignAlongCStarButton,5,4,1,2);

        gridLayout->addWidget(alignSlicetoAStarPushButton,6,0,1,2);
        gridLayout->addWidget(alignSlicetoBStarPushButton,6,2,1,2);
        gridLayout->addWidget(alignSlicetoCStarPushButton,6,4,1,2);
        
        gridLayout->addWidget(helpCellOverlayButton,7,0,1,3);
        gridLayout->addWidget(rotateCellButton,7,3,1,3);
        
        gridLayout->addWidget(toggleCellButton,8,0,1,6);
        
        unitCellWidget->setLayout(gridLayout);
        
        unitCellDock->setWidget(unitCellWidget);
        
        unitCellDock->setFixedHeight(unitCellWidget->minimumSizeHint().height()*1.15);
        
        viewMenu->addAction(unitCellDock->toggleViewAction());
        volumeRenderMainWindow->addDockWidget(Qt::LeftDockWidgetArea, unitCellDock);
    }

    
    /* SVO metadata text edit */
    {
        svoHeaderDock = new QDockWidget(tr("File info/notes"), this);
        svoHeaderEdit = new QPlainTextEdit;
        
        svoHeaderDock->setWidget(svoHeaderEdit);
        
        volumeRenderMainWindow->addDockWidget(Qt::LeftDockWidgetArea, svoHeaderDock);

//        svoHeaderDock->hide();
        
        connect(this, SIGNAL(pathChanged(QString)), this, SLOT(setHeader(QString)));
        
    }
    
    /* File Controls Widget */
    {
        fileControlsWidget = new QWidget;

        QLabel * labelE = new QLabel(QString("Active angle:"));

        activeAngleComboBox = new QComboBox;
        activeAngleComboBox->addItem("Phi");
        activeAngleComboBox->addItem("Kappa");
        activeAngleComboBox->addItem("Omega");
        activeAngleComboBox->addItem("Given by file");

        omegaCorrectionSpinBox = new QDoubleSpinBox;
        omegaCorrectionSpinBox->setRange(-180, 180);
        omegaCorrectionSpinBox->setDecimals(3);
        omegaCorrectionSpinBox->setPrefix("Δω: ");
        omegaCorrectionSpinBox->setSuffix(" °");
        
        kappaCorrectionSpinBox = new QDoubleSpinBox;
        kappaCorrectionSpinBox->setRange(-180, 180);
        kappaCorrectionSpinBox->setDecimals(3);
        kappaCorrectionSpinBox->setPrefix("Δκ: ");
        kappaCorrectionSpinBox->setSuffix(" °");
                                     
        phiCorrectionSpinBox = new QDoubleSpinBox;
        phiCorrectionSpinBox->setRange(-180, 180);
        phiCorrectionSpinBox->setDecimals(3);
        phiCorrectionSpinBox->setPrefix("Δφ: ");
        phiCorrectionSpinBox->setSuffix(" °");
                                     
        

        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(5,5,5,5);
        gridLayout->setRowStretch(4,1);
        gridLayout->addWidget(labelE,0,0,1,4);
        gridLayout->addWidget(activeAngleComboBox,0,4,1,4);
        gridLayout->addWidget(omegaCorrectionSpinBox,1,0,1,8);
        gridLayout->addWidget(kappaCorrectionSpinBox,2,0,1,8);
        gridLayout->addWidget(phiCorrectionSpinBox,3,0,1,8);
        
        fileControlsWidget->setLayout(gridLayout);
        
        fileDockWidget = new QDockWidget(tr("Reconstruction corrections"), this);
        fileDockWidget->setWidget(fileControlsWidget);
        fileDockWidget->setMaximumHeight(fileControlsWidget->minimumSizeHint().height()*1.1);
        viewMenu->addAction(fileDockWidget->toggleViewAction());
        imageMainWindow->addDockWidget(Qt::LeftDockWidgetArea, fileDockWidget);
    }
    
    /*Voxelize dock widget*/
    {
        setFileButton = new QPushButton;
        setFileButton->setIcon(QIcon(":/art/proceed.png"));
//        setFileButton->setIconSize(QSize(24,24));
        setFileButton->setText("Set ");

        readFileButton = new QPushButton;
        readFileButton->setIcon(QIcon(":/art/proceed.png"));
//        readFileButton->setIconSize(QSize(24,24));
        readFileButton->setText("Read ");
        readFileButton->setEnabled(false);

        projectFileButton = new QPushButton;
        projectFileButton->setIcon(QIcon(":/art/proceed.png"));
//        projectFileButton->setIconSize(QSize(24,24));
        projectFileButton->setText("Project ");
        projectFileButton->setEnabled(false);

        reconstructButton = new QPushButton;
        reconstructButton->setIcon(QIcon(":/art/fast_proceed.png"));
        reconstructButton->setText("Reconstruct frames");
//        allInOneButton->setIconSize(QSize(24,24));

        killButton = new QPushButton;
        killButton->setIcon(QIcon(":/art/kill.png"));
        killButton->setText("Kill ");
//        killButton->setIconSize(QSize(24,24));
        
        svoLevelSpinBox = new QSpinBox;
        svoLevelSpinBox->setRange(1, 15);
        svoLevelSpinBox->setPrefix("Octtree levels: ");

        voxelizeButton = new QPushButton;
        voxelizeButton->setIcon(QIcon(":/art/proceed.png"));
        voxelizeButton->setText("Generate tree");
//        voxelizeButton->setEnabled(false);
        
        saveSvoButton = new QPushButton;
        saveSvoButton->setIcon(QIcon(":/art/save.png"));
        saveSvoButton->setText("Save tree");
        
        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setContentsMargins(5,5,5,5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setRowStretch(4,1);
        gridLayout->addWidget(reconstructButton,0,0,1,1);
        gridLayout->addWidget(killButton,0,1,1,1);
        gridLayout->addWidget(svoLevelSpinBox,1,0,1,2);
        gridLayout->addWidget(voxelizeButton,2,0,1,2);
        gridLayout->addWidget(saveSvoButton,3,0,1,2);
        
        
        voxelizeWidget = new QWidget;
        voxelizeWidget->setLayout(gridLayout);
        
        voxelizeDockWidget = new QDockWidget("Reconstruction operations");
        voxelizeDockWidget->setWidget(voxelizeWidget);
//        voxelizeDockWidget->setFixedHeight(voxelizeWidget->minimumSizeHint().height()*1.1);
        viewMenu->addAction(voxelizeDockWidget->toggleViewAction());
        imageMainWindow->addDockWidget(Qt::RightDockWidgetArea, voxelizeDockWidget);
    }
    

    
    /* Function dock widget */
    {
        functionToggleButton = new QPushButton(tr("Toggle"));
        funcParamASpinBox = new QDoubleSpinBox;
        funcParamASpinBox->setDecimals(3);
        funcParamASpinBox->setRange(0, 100);
        funcParamASpinBox->setSingleStep(0.01);
        funcParamASpinBox->setAccelerated(1);
        funcParamASpinBox->setPrefix("Var 1: ");

        funcParamBSpinBox = new QDoubleSpinBox;
        funcParamBSpinBox->setDecimals(3);
        funcParamBSpinBox->setRange(0, 100);
        funcParamBSpinBox->setSingleStep(0.01);
        funcParamBSpinBox->setAccelerated(1);
        funcParamBSpinBox->setPrefix("Var 2: ");

        funcParamCSpinBox = new QDoubleSpinBox;
        funcParamCSpinBox->setDecimals(3);
        funcParamCSpinBox->setRange(0, 100);
        funcParamCSpinBox->setSingleStep(0.01);
        funcParamCSpinBox->setAccelerated(1);
        funcParamCSpinBox->setPrefix("Var 3: ");

        funcParamDSpinBox = new QDoubleSpinBox;
        funcParamDSpinBox->setDecimals(3);
        funcParamDSpinBox->setRange(0, 100);
        funcParamDSpinBox->setSingleStep(0.01);
        funcParamDSpinBox->setAccelerated(1);
        funcParamDSpinBox->setPrefix("Var 4: ");

        functionDockWidget = new QDockWidget(tr("Model settings"), this);
        functionWidget = new QWidget;

        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(0);
        gridLayout->setContentsMargins(5,5,5,5);
        gridLayout->setRowStretch(6,1);
        gridLayout->addWidget(funcParamASpinBox,1,0,1,4);
        gridLayout->addWidget(funcParamBSpinBox,2,0,1,4);
        gridLayout->addWidget(funcParamCSpinBox,3,0,1,4);
        gridLayout->addWidget(funcParamDSpinBox,4,0,1,4);
        gridLayout->addWidget(functionToggleButton,5,0,1,4);
        functionWidget->setLayout(gridLayout);
        
        functionDockWidget->setWidget(functionWidget);
        functionDockWidget->setFixedHeight(functionWidget->minimumSizeHint().height());
        viewMenu->addAction(functionDockWidget->toggleViewAction());
        volumeRenderMainWindow->addDockWidget(Qt::RightDockWidgetArea, functionDockWidget);
        functionDockWidget->hide();
    }



    /* Output Widget */
    {
        outputDockWidget = new QDockWidget(tr("Message log"), this);
        botWidget = new QWidget;

        // Progress Bar
        genericProgressBar = new QProgressBar;
        genericProgressBar->setRange( 0, 100);
        
        memoryUsageProgressBar = new QProgressBar;
        memoryUsageProgressBar->setRange( 0, 1);
        

        // Text output
        errorTextEdit = new QPlainTextEdit;
        error_highlighter = new Highlighter(errorTextEdit->document());
        errorTextEdit->setReadOnly(true);

        // Layout
        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(0);
        gridLayout->setContentsMargins(5,5,5,5);
        gridLayout->addWidget(errorTextEdit, 0, 0, 1, 1);
        gridLayout->addWidget(genericProgressBar, 1, 0, 1, 1);
        gridLayout->addWidget(memoryUsageProgressBar, 2, 0, 1, 1);

        botWidget->setLayout(gridLayout);
        outputDockWidget->setWidget(botWidget);
        viewMenu->addAction(outputDockWidget->toggleViewAction());
        this->addDockWidget(Qt::BottomDockWidgetArea, outputDockWidget);
    }

    // File header dock widget
    {
        fileHeaderEdit = new QPlainTextEdit;
        fileHeaderEdit->setReadOnly(true);

        fileHeaderDock = new QDockWidget(tr("Frame header info"), this);
        fileHeaderDock->setWidget(fileHeaderEdit);
        this->addDockWidget(Qt::RightDockWidgetArea, fileHeaderDock);
    }

    
    /* Text output widget */
    outputPlainTextEdit = new QPlainTextEdit("Certain output may be written in plain text here");
    outputPlainTextEdit->setReadOnly(true);
    
    /*      Tab widget      */
    tabWidget = new QTabWidget;

    // Add tabs
    tabWidget->addTab(setFilesWidget, tr("File selection"));
    tabWidget->addTab(imageMainWindow, tr("Reconstruction"));
    tabWidget->addTab(volumeRenderMainWindow, tr("Visualization"));
    tabWidget->addTab(outputPlainTextEdit, "Text output");

    // Put into main layout
//    mainLayout->setContentsMargins(3,3,3,3);
    mainLayout = new QGridLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
//    mainLayout->addWidget(topWidget,0,0,1,1);
    mainLayout->addWidget(tabWidget,1,0,1,1);
    
    mainWidget = new QWidget(this);
    mainWidget->setLayout(mainLayout);
}

void MainWindow::applyAnalytics()
{
    emit analyze(apply_mode);
}

void MainWindow::applyPlaneMarker()
{
    emit setPlaneMarkers(apply_mode);
}

void MainWindow::applySelection()
{
    emit setSelection(apply_mode);
}

void MainWindow::setApplyMode(QString str)
{
//    qDebug() << str;

    apply_mode = str;
//    integration_mode = str;
}

void MainWindow::nextFrame()
{
    imageSpinBox->setValue(imageSpinBox->value()+1);    
}

void MainWindow::previousFrame()
{
    imageSpinBox->setValue(imageSpinBox->value()-1);
}

void MainWindow::batchForward()
{
    imageSpinBox->setValue(imageSpinBox->value()+batch_size);
}
void MainWindow::batchBackward()
{
    imageSpinBox->setValue(imageSpinBox->value()-batch_size);
}

void MainWindow::setBatchSize(int value)
{
    batch_size = value;
}

void MainWindow::setImageRange(int low, int high)
{
    imageSpinBox->setRange(low, high);
}

void MainWindow::takeImageScreenshotFunction()
{
    // Move this to imagepreview?
    QString format = "jpg";
    QDateTime dateTime = dateTime.currentDateTime();
    QString initialPath = screenshot_dir + QString("/screenshot_"+dateTime.toString("yyyy_MM_dd_hh_mm_ss")) +"."+ format;

    QString file_name = QFileDialog::getSaveFileName(this, tr("Save as"), initialPath,
                                                tr("%1 files (*.%2);;All files (*)")
                                                .arg(format.toUpper())
                                                .arg(format));
    if (file_name !="")
    {
        QFileInfo info(file_name);
        screenshot_dir = info.absoluteDir().path();

        emit takeImageScreenshot(file_name);
    }
}

void MainWindow::saveImageFunction()
{
    // Move this to imagepreview?
    QString format = "jpg";
    QDateTime dateTime = dateTime.currentDateTime();
    QString initialPath = screenshot_dir + QString("/image_"+dateTime.toString("yyyy_MM_dd_hh_mm_ss")) +"."+ format;

    QString file_name = QFileDialog::getSaveFileName(this, tr("Save as"), initialPath,
                                                tr("%1 files (*.%2);;All files (*)")
                                                .arg(format.toUpper())
                                                .arg(format));
    if (file_name !="")
    {
        QFileInfo info(file_name);
        screenshot_dir = info.absoluteDir().path();

        emit saveImage(file_name);
    }
}

void MainWindow::print(QString str)
{
    info.append(str);
    size_t chars_max = 65536;
    size_t removable = info.size()- chars_max;
    if (removable > 0) info.remove(0, removable);

    errorTextEdit->setPlainText(info);
    errorTextEdit->moveCursor(QTextCursor::End);
    errorTextEdit->ensureCursorVisible();
}




void MainWindow::readSettings()
{
    QSettings settings("Norwegian University of Science and Technology", "Nebula");
    QPoint pos = settings.value("position", QPoint(0, 0)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    working_dir = settings.value("working_dir").toString();
    screenshot_dir = settings.value("screenshot_dir").toString();
    resize(size);
    move(pos);
}

void MainWindow::writeSettings()
{
    QSettings settings("Norwegian University of Science and Technology", "Nebula");
    settings.setValue("position", pos());
    settings.setValue("size", size());
    settings.setValue("working_dir", working_dir);
    settings.setValue("screenshot_dir", screenshot_dir);
}

void MainWindow::takeVolumeScreenshot()
{
    QString format = "jpg";
    QDateTime dateTime = dateTime.currentDateTime();
    QString initialPath = screenshot_dir + QString("/screenshot_"+dateTime.toString("yyyy_MM_dd_hh_mm_ss")) +"."+ format;

    QString file_name = QFileDialog::getSaveFileName(this, tr("Save as"), initialPath,
                                                tr("%1 files (*.%2);;All files (*)")
                                                .arg(format.toUpper())
                                                .arg(format));
    if (file_name !="")
    {
        QFileInfo info(file_name);
        screenshot_dir = info.absoluteDir().path();

        emit captureFrameBuffer(file_name);
    }
}

