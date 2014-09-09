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
#include <QFileDialog>
#include <QToolButton>
#include <QtScript>
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
    //     Set some default values
    current_svo = 0;
    display_file = 0;
    
    //     Set stylesheet
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
    
    
    this->initializeActions();
    
    this->initializeMenus();
    
    this->initializeInteractives();
    
    this->initializeConnects();
    
    this->initializeWorkers();
    
    
    
    setCentralWidget(mainWidget);
    readSettings();
    setCurrentFile("");
    print("[Nebula] Welcome to Nebula!");
    setWindowTitle(tr("Nebula[*]"));
    
    // Set start conditions
    setStartConditions();

    graphicsDockWidget->hide();
    unitCellDock->hide();
    functionDockWidget->hide();
    fileDockWidget->hide();
    toolChainWidget->show();
    outputDockWidget->show();
}

MainWindow::~MainWindow()
{
    readScriptThread->quit();
    readScriptThread->wait(1000);
            
    setFileThread->quit();
    setFileThread->wait(1000);
    
    readFileThread->quit();
    readFileThread->wait(1000);
    
    projectFileThread->quit();
    projectFileThread->wait(1000);
    
    voxelizeThread->quit();
    voxelizeThread->wait(1000);
    
    allInOneThread->quit();
    allInOneThread->wait(1000);
    
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


void MainWindow::initializeWorkers()
{
    readScriptThread = new QThread;
    setFileThread = new QThread;
    readFileThread = new QThread;
    projectFileThread = new QThread;
    voxelizeThread = new QThread;
    allInOneThread = new QThread;
    displayFileThread = new QThread;

    // readScriptWorker
    readScriptWorker = new ReadScriptWorker();
    readScriptWorker->setFilePaths(&file_paths);
    readScriptWorker->setScriptEngine(&engine);
    readScriptWorker->setInput(scriptTextEdit);

    readScriptWorker->moveToThread(readScriptThread);
    connect(readScriptThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
    connect(readScriptWorker, SIGNAL(finished()), this, SLOT(readScriptButtonFinish()));
    connect(readScriptThread, SIGNAL(started()), readScriptWorker, SLOT(process()));
    connect(readScriptWorker, SIGNAL(abort()), readScriptThread, SLOT(quit()));
    connect(readScriptWorker, SIGNAL(finished()), readScriptThread, SLOT(quit()));
    connect(readScriptWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)), Qt::BlockingQueuedConnection);
    connect(readScriptWorker, SIGNAL(changedGenericProgress(int)), progressBar, SLOT(setValue(int)));
    connect(readScriptWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(readScriptWorker, SIGNAL(changedTabWidget(int)), tabWidget, SLOT(setCurrentIndex(int)));
    connect(readScriptButton, SIGNAL(clicked()), readScriptThread, SLOT(start()));
    connect(killButton, SIGNAL(clicked()), readScriptWorker, SLOT(killProcess()), Qt::DirectConnection);

    //### setFileWorker ###
    setFileWorker = new SetFileWorker();
    setFileWorker->setFilePaths(&file_paths);
    setFileWorker->setFiles(&files);
    setFileWorker->setSVOFile(&svo_inprocess);
    setFileWorker->setOpenCLContext(context_cl);
    setFileWorker->setSVOFile(&svo_inprocess);

    setFileWorker->moveToThread(setFileThread);
    connect(setFileButton, SIGNAL(clicked()), this, SLOT(setFilesFromSelectionModel()), Qt::DirectConnection);
    connect(setFileThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
    connect(setFileWorker, SIGNAL(finished()), this, SLOT(setFileButtonFinish()));
    connect(setFileThread, SIGNAL(started()), setFileWorker, SLOT(process()));
    connect(setFileWorker, SIGNAL(abort()), setFileThread, SLOT(quit()));
    connect(setFileWorker, SIGNAL(finished()), setFileThread, SLOT(quit()));
    connect(setFileWorker, SIGNAL(changedFile(QString)), this, SLOT(setHeader(QString)));
    connect(setFileWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(setFileWorker, SIGNAL(changedGenericProgress(int)), progressBar, SLOT(setValue(int)));
    connect(setFileWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(setFileButton, SIGNAL(clicked()), setFileThread, SLOT(start()));
    connect(killButton, SIGNAL(clicked()), setFileWorker, SLOT(killProcess()), Qt::DirectConnection);


    //### readFileWorker ###
    readFileWorker = new ReadFileWorker();
    readFileWorker->setFilePaths(&file_paths);
    readFileWorker->setFiles(&files);
    readFileWorker->setSVOFile(&svo_inprocess);

    readFileWorker->moveToThread(readFileThread);
    connect(readFileThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
    connect(readFileWorker, SIGNAL(finished()), this, SLOT(readFileButtonFinish()));
    connect(readFileThread, SIGNAL(started()), readFileWorker, SLOT(process()));
    connect(readFileWorker, SIGNAL(abort()), readFileThread, SLOT(quit()));
    connect(readFileWorker, SIGNAL(finished()), readFileThread, SLOT(quit()));
    connect(readFileWorker, SIGNAL(changedFile(QString)), this, SLOT(setHeader(QString)));
    connect(readFileWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(readFileWorker, SIGNAL(changedGenericProgress(int)), progressBar, SLOT(setValue(int)));
    connect(readFileWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(readFileWorker, SIGNAL(changedTabWidget(int)), tabWidget, SLOT(setCurrentIndex(int)));
    connect(readFileButton, SIGNAL(clicked()), readFileThread, SLOT(start()));
    connect(killButton, SIGNAL(clicked()), readFileWorker, SLOT(killProcess()), Qt::DirectConnection);


    //### projectFileWorker ###
    projectFileWorker = new ProjectFileWorker();
    projectFileWorker->setFilePaths(&file_paths);
    projectFileWorker->setSVOFile(&svo_inprocess);
    projectFileWorker->setFiles(&files);
    projectFileWorker->setOpenCLContext(context_cl);
    projectFileWorker->setReducedPixels(&reduced_pixels);
    projectFileWorker->initializeCLKernel();
    connect(this->activeAngleComboBox, SIGNAL(currentIndexChanged(int)), projectFileWorker, SLOT(setActiveAngle(int)), Qt::QueuedConnection);
    connect(this->omegaCorrectionSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setOffsetOmega(double)), Qt::QueuedConnection);
    connect(this->kappaCorrectionSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setOffsetKappa(double)), Qt::QueuedConnection);
    connect(this->phiCorrectionSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setOffsetPhi(double)), Qt::QueuedConnection);
    connect(this->noiseCorrectionMinDoubleSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setReduceThresholdLow(double)), Qt::QueuedConnection);
    connect(this->noiseCorrectionMaxDoubleSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setReduceThresholdHigh(double)), Qt::QueuedConnection);
    connect(this->postCorrectionMinDoubleSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setProjectThresholdLow(double)), Qt::QueuedConnection);
    connect(this->postCorrectionMaxDoubleSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setProjectThresholdHigh(double)), Qt::QueuedConnection);

    projectFileWorker->moveToThread(projectFileThread);
    connect(projectFileThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
    connect(projectFileWorker, SIGNAL(finished()), this, SLOT(projectFileButtonFinish()));
    connect(projectFileThread, SIGNAL(started()), projectFileWorker, SLOT(process()));
    connect(projectFileWorker, SIGNAL(finished()), projectFileThread, SLOT(quit()));
    connect(projectFileWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(projectFileWorker, SIGNAL(changedGenericProgress(int)), progressBar, SLOT(setValue(int)));
    connect(projectFileWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(projectFileWorker, SIGNAL(changedTabWidget(int)), tabWidget, SLOT(setCurrentIndex(int)));
    connect(projectFileButton, SIGNAL(clicked()), this, SLOT(runProjectFileThread()));
    connect(killButton, SIGNAL(clicked()), projectFileWorker, SLOT(killProcess()), Qt::DirectConnection);


    //### allInOneWorker ###
    allInOneWorker = new MultiWorker();
    allInOneWorker->setFilePaths(&file_paths);
    allInOneWorker->setSVOFile(&svo_inprocess);
    allInOneWorker->setOpenCLContext(context_cl);
    allInOneWorker->setReducedPixels(&reduced_pixels);
    allInOneWorker->initializeCLKernel();
    connect(allInOneButton, SIGNAL(clicked()), this, SLOT(setFilesFromSelectionModel()), Qt::DirectConnection);
    connect(this->activeAngleComboBox, SIGNAL(currentIndexChanged(int)), allInOneWorker, SLOT(setActiveAngle(int)), Qt::QueuedConnection);
    connect(this->omegaCorrectionSpinBox, SIGNAL(valueChanged(double)), allInOneWorker, SLOT(setOffsetOmega(double)), Qt::QueuedConnection);
    connect(this->kappaCorrectionSpinBox, SIGNAL(valueChanged(double)), allInOneWorker, SLOT(setOffsetKappa(double)), Qt::QueuedConnection);
    connect(this->phiCorrectionSpinBox, SIGNAL(valueChanged(double)), allInOneWorker, SLOT(setOffsetPhi(double)), Qt::QueuedConnection);
    connect(this->noiseCorrectionMinDoubleSpinBox, SIGNAL(valueChanged(double)), allInOneWorker, SLOT(setReduceThresholdLow(double)), Qt::QueuedConnection);
    connect(this->noiseCorrectionMaxDoubleSpinBox, SIGNAL(valueChanged(double)), allInOneWorker, SLOT(setReduceThresholdHigh(double)), Qt::QueuedConnection);
    connect(this->postCorrectionMinDoubleSpinBox, SIGNAL(valueChanged(double)), allInOneWorker, SLOT(setProjectThresholdLow(double)), Qt::QueuedConnection);
    connect(this->postCorrectionMaxDoubleSpinBox, SIGNAL(valueChanged(double)), allInOneWorker, SLOT(setProjectThresholdHigh(double)), Qt::QueuedConnection);

    allInOneWorker->moveToThread(allInOneThread);
    connect(allInOneThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
    connect(allInOneWorker, SIGNAL(finished()), this, SLOT(allInOneButtonFinish()));
    connect(allInOneThread, SIGNAL(started()), allInOneWorker, SLOT(process()));
    connect(allInOneWorker, SIGNAL(finished()), allInOneThread, SLOT(quit()));
    connect(allInOneWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(allInOneWorker, SIGNAL(changedGenericProgress(int)), progressBar, SLOT(setValue(int)));
    connect(allInOneWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(allInOneWorker, SIGNAL(changedFile(QString)), this, SLOT(setHeader(QString)));
    connect(allInOneWorker, SIGNAL(changedTabWidget(int)), tabWidget, SLOT(setCurrentIndex(int)));
    connect(allInOneButton, SIGNAL(clicked()), this, SLOT(runAllInOneThread()));
    connect(killButton, SIGNAL(clicked()), allInOneWorker, SLOT(killProcess()), Qt::DirectConnection);


    //### voxelizeWorker ###
    voxelizeWorker = new VoxelizeWorker();
    voxelizeWorker->setOpenCLContext(context_cl);
    voxelizeWorker->moveToThread(voxelizeThread);
    voxelizeWorker->setSVOFile(&svo_inprocess);
//    voxelizeWorker->setQSpaceInfo(&suggested_search_radius_low, &suggested_search_radius_high, &suggested_q);
    voxelizeWorker->setReducedPixels(&reduced_pixels);
    voxelizeWorker->initializeCLKernel();

    voxelizeWorker->moveToThread(voxelizeThread);
    connect(voxelizeThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
    connect(voxelizeWorker, SIGNAL(finished()), this, SLOT(voxelizeButtonFinish()));
    connect(svoLevelSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setCurrentSvoLevel(int)), Qt::DirectConnection);
    connect(voxelizeThread, SIGNAL(started()), voxelizeWorker, SLOT(process()));
    connect(voxelizeWorker, SIGNAL(finished()), voxelizeThread, SLOT(quit()));
    connect(voxelizeWorker, SIGNAL(popup(QString,QString)), this, SLOT(displayPopup(QString,QString)));
    connect(voxelizeWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(voxelizeWorker, SIGNAL(changedGenericProgress(int)), progressBar, SLOT(setValue(int)));
    connect(voxelizeWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(voxelizeButton, SIGNAL(clicked()), voxelizeThread, SLOT(start()));
    connect(killButton, SIGNAL(clicked()), voxelizeWorker, SLOT(killProcess()), Qt::DirectConnection);
    connect(allInOneWorker, SIGNAL(qSpaceInfoChanged(float,float,float)), voxelizeWorker, SLOT(setQSpaceInfo(float,float,float)));
    connect(setFileWorker, SIGNAL(qSpaceInfoChanged(float,float,float)), voxelizeWorker, SLOT(setQSpaceInfo(float,float,float)));
}

//void MainWindow::test()
//{
//    qDebug("Test");
//}

void MainWindow::anyButtonStart()
{
    readScriptButton->setDisabled(true);
    setFileButton->setDisabled(true);
    allInOneButton->setDisabled(true);
    readFileButton->setDisabled(true);
    projectFileButton->setDisabled(true);
    voxelizeButton->setDisabled(true);
}

void MainWindow::readScriptButtonFinish()
{
    readScriptButton->setDisabled(false);
    setFileButton->setDisabled(false);
    allInOneButton->setDisabled(false);
    readFileButton->setDisabled(true);
    projectFileButton->setDisabled(true);
    voxelizeButton->setDisabled(true);
}

void MainWindow::setFileButtonFinish()
{
    readScriptButton->setDisabled(false);
    setFileButton->setDisabled(false);
    allInOneButton->setDisabled(false);
    readFileButton->setDisabled(false);
    projectFileButton->setDisabled(true);
    voxelizeButton->setDisabled(true);
}

void MainWindow::allInOneButtonFinish()
{
    readScriptButton->setDisabled(false);
    setFileButton->setDisabled(false);
    allInOneButton->setDisabled(false);
    readFileButton->setDisabled(true);
    projectFileButton->setDisabled(true);
    voxelizeButton->setDisabled(false);
}

void MainWindow::readFileButtonFinish()
{
    readScriptButton->setDisabled(false);
    setFileButton->setDisabled(false);
    allInOneButton->setDisabled(false);
    readFileButton->setDisabled(false);
    projectFileButton->setDisabled(false);
    voxelizeButton->setDisabled(true);
}

void MainWindow::projectFileButtonFinish()
{
    readScriptButton->setDisabled(false);
    setFileButton->setDisabled(false);
    allInOneButton->setDisabled(false);
    readFileButton->setDisabled(false);
    projectFileButton->setDisabled(false);
    voxelizeButton->setDisabled(false);
}

void MainWindow::voxelizeButtonFinish()
{
    readScriptButton->setDisabled(false);
    setFileButton->setDisabled(false);
    allInOneButton->setDisabled(false);
    readFileButton->setDisabled(false);
    projectFileButton->setDisabled(false);
    voxelizeButton->setDisabled(false);
}

void MainWindow::loadPaths()
{
    QMessageBox confirmationMsgBox;
    
    confirmationMsgBox.setWindowTitle("framer");
    confirmationMsgBox.setIcon(QMessageBox::Question);
    confirmationMsgBox.setText("Unsaved changes will be lost.");
    confirmationMsgBox.setInformativeText("Save first?");
    confirmationMsgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    confirmationMsgBox.setDefaultButton(QMessageBox::Save);
    
    int ret = QMessageBox::Discard;
    
    if (hasPendingChanges) ret = confirmationMsgBox.exec();
    
    switch (ret) 
    {
        case QMessageBox::Save:
            // Save was clicked
            saveSvo();
//            tabWidget->setCurrentIndex(1);
            setFiles(fileSelectionModel->getPaths());
            break;
        case QMessageBox::Discard:
            // Discard was clicked
//            tabWidget->setCurrentIndex(1);
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
    folderSet.clear();
    
    QMap<QString, QStringList>::const_iterator i = folder_map.constBegin();
    while (i != folder_map.constEnd())
    {
        ImageFolder folder;
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
            
        folderSet << folder;

        ++i;
    }
    
    if (folderSet.size() > 0) 
    {
        emit imageChanged(*folderSet.current()->current());
        emit centerImage();
    }
}


void MainWindow::removeImage()
{
    if (folderSet.size() > 0)
    {
        emit pathRemoved(folderSet.current()->current()->path());
        
        folderSet.current()->removeCurrent();

        if (folderSet.current()->size() == 0) folderSet.removeCurrent();
        
//        qDebug() << folders.size() << folderSet.current()->size() << folderSet.current()->i();
        
        if (folderSet.size() > 0)
        {
            emit imageChanged(*folderSet.current()->next());
//            emit pathChanged(folderSet.current()->next()->path());
//            emit selectionChanged(folderSet.current()->current()->selection());
        }
    }
}

void MainWindow::nextFrame()
{
    if (folderSet.size() > 0)
    {
        emit imageChanged(*folderSet.current()->next());
    }
}
void MainWindow::previousFrame()
{
    if (folderSet.size() > 0)
    {
        emit imageChanged(*folderSet.current()->previous());
    }
}
void MainWindow::batchForward()
{
    if (folderSet.size() > 0)
    {
        for (size_t i = 0; i < batch_size; i++)
        {
            folderSet.current()->next();
        }
        
        emit imageChanged(*folderSet.current()->current());
    }
}
void MainWindow::batchBackward()
{
    if (folderSet.size() > 0)
    {
        for (size_t i = 0; i < batch_size; i++)
        {
            folderSet.current()->previous();
        }
        
        emit imageChanged(*folderSet.current()->current());
    }
}

void MainWindow::nextFolder()
{
    if (folderSet.size() > 0)
    {
        emit imageChanged(*folderSet.next()->current());
    }
}
void MainWindow::previousFolder()
{
    if (folderSet.size() > 0)
    {
        emit imageChanged(*folderSet.previous()->current());
    }
}

//void MainWindow::incrementDisplayFile1()
//{
//    imageSpinBox->setValue(imageSpinBox->value()+1);
//}
//void MainWindow::incrementDisplayFile10()
//{
//    imageSpinBox->setValue(imageSpinBox->value()+10);
//}
//void MainWindow::decrementDisplayFile1()
//{
//    imageSpinBox->setValue(imageSpinBox->value()-1);
//}
//void MainWindow::decrementDisplayFile10()
//{
//    imageSpinBox->setValue(imageSpinBox->value()-10);
//}
//void MainWindow::setDisplayFile(int value)
//{
//    if ((value >= 0) && (value < file_paths.size()))
//    {
//        emit imagePreviewChanged(file_paths[value]);
//        emit setHeader(file_paths[value]);
//        imageLabel->setText(file_paths[value]);
//    }
//    else
//    {
//        print("\n[Nebula] File #"+QString::number(value)+" could not be found");
//    }
//}

//void MainWindow::refreshDisplayFile()
//{
//    int value = imageSpinBox->value();
    
//    if ((value >= 0) && (value < file_paths.size()))
//    {
//        emit imagePreviewChanged(file_paths[value]);
//        emit setHeader(file_paths[value]);
//        imageLabel->setText(file_paths[value]);
//    }
//}

//void MainWindow::setHeader(QString path)
//{
//    DetectorFile file(path);
//    fileHeaderEdit->setPlainText(file.getHeaderText());
//}

void MainWindow::runProjectFileThread()
{
    tabWidget->setCurrentIndex(1);
    
    // Creation settings
    svo_inprocess.creation_date = QDateTime::currentDateTime();
    svo_inprocess.creation_noise_cutoff_low = noiseCorrectionMinDoubleSpinBox->value();
    svo_inprocess.creation_noise_cutoff_high = noiseCorrectionMaxDoubleSpinBox->value();
    svo_inprocess.creation_post_cutoff_low = postCorrectionMinDoubleSpinBox->value();
    svo_inprocess.creation_post_cutoff_high = postCorrectionMaxDoubleSpinBox->value();
    svo_inprocess.creation_correction_omega = omegaCorrectionSpinBox->value();
    svo_inprocess.creation_correction_kappa = kappaCorrectionSpinBox->value();
    svo_inprocess.creation_correction_phi = phiCorrectionSpinBox->value();
    svo_inprocess.creation_file_paths = file_paths;
    
    projectFileThread->start();
}

void MainWindow::runAllInOneThread()
{
    tabWidget->setCurrentIndex(1);
    
    // Creation settings
    svo_inprocess.creation_date = QDateTime::currentDateTime();
    svo_inprocess.creation_noise_cutoff_low = noiseCorrectionMinDoubleSpinBox->value();
    svo_inprocess.creation_noise_cutoff_high = noiseCorrectionMaxDoubleSpinBox->value();
    svo_inprocess.creation_post_cutoff_low = postCorrectionMinDoubleSpinBox->value();
    svo_inprocess.creation_post_cutoff_high = postCorrectionMaxDoubleSpinBox->value();
    svo_inprocess.creation_correction_omega = omegaCorrectionSpinBox->value();
    svo_inprocess.creation_correction_kappa = kappaCorrectionSpinBox->value();
    svo_inprocess.creation_correction_phi = phiCorrectionSpinBox->value();
    svo_inprocess.creation_file_paths = file_paths;
    
    
    allInOneThread->start();
}

void MainWindow::setFilesFromSelectionModel()
{
    if (!scriptingAct->isChecked()) file_paths = folderSet.paths();
}

void MainWindow::setStartConditions()
{
    tabWidget->setCurrentIndex(0);
    svoLevelSpinBox->setValue(11);

    noiseCorrectionMinDoubleSpinBox->setValue(5);
    noiseCorrectionMaxDoubleSpinBox->setValue(1e9);
    postCorrectionMinDoubleSpinBox->setValue(10);
    postCorrectionMaxDoubleSpinBox->setValue(1e9);

    volumeRenderDataMinSpinBox->setValue(1.0);
    volumeRenderDataMaxSpinBox->setValue(10);
    volumeRenderAlphaSpinBox->setValue(1.0);
    volumeRenderBrightnessSpinBox->setValue(2.0);
    
    volumeRenderViewModeComboBox->setCurrentIndex(0);
    tsfAlphaComboBox->setCurrentIndex(2);
    volumeRenderTsfComboBox->setCurrentIndex(1);
    
    tsfTextureComboBox->setCurrentIndex(1);
    tsfAlphaComboBox->setCurrentIndex(2);
    dataMinDoubleSpinBox->setValue(5);
    dataMaxDoubleSpinBox->setValue(1000);
    logCheckBox->setChecked(true);
    correctionLorentzCheckBox->setChecked(true);
    imageModeComboBox->setCurrentIndex(0);
    
    
    funcParamASpinBox->setValue(13.5);
    funcParamBSpinBox->setValue(10.5);
    funcParamCSpinBox->setValue(10.0);
    funcParamDSpinBox->setValue(0.005);

    qualitySlider->setValue(20);
    
    fileSelectionFilter->setText("*.cbf");
    
    activeAngleComboBox->setCurrentIndex(2);
    omegaCorrectionSpinBox->setValue(1.0);
    kappaCorrectionSpinBox->setValue(1.0);
    phiCorrectionSpinBox->setValue(1.0);
    
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
    if (maybeSave())
    {
        writeSettings();
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::saveProject()
{
    QFileDialog dialog;
    dialog.setDefaultSuffix("txt");
    QString path = dialog.getSaveFileName(this, tr("Save project"), "", tr(".qt (*.qt);; All Files (*)"));

    if (path != "")
    {
        QFile file(path);
        if (file.open(QIODevice::WriteOnly))
        {
            QDataStream out(&file);
            
            out << folderSet;
            out << tsfTextureComboBox->currentText();
            out << tsfAlphaComboBox->currentText();
            out << (double) dataMinDoubleSpinBox->value();
            out << (double) dataMaxDoubleSpinBox->value();
            out << (bool) logCheckBox->isChecked();
            out << (bool) correctionLorentzCheckBox->isChecked();  
            out << (bool) autoBackgroundCorrectionCheckBox->isChecked();  
            
            file.close();
        }
    }
    
    hasPendingChanges = false;
}

void MainWindow::loadProject()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open project"), "", tr(".qt (*.qt);; All Files (*)"));

    if (path != "")
    {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly))
        {
            QString tsfTexture;
            QString tsfAlpha;
            double dataMin;
            double dataMax;
            bool log;
            bool lorentzCorrection;
            bool autoBackgroundCorrection;
            
            QDataStream in(&file);
            
            in >> folderSet >> tsfTexture >> tsfAlpha >> dataMin >> dataMax >> log >> lorentzCorrection >> autoBackgroundCorrection;
            
            
            tsfTextureComboBox->setCurrentText(tsfTexture);
            tsfAlphaComboBox->setCurrentText(tsfAlpha);
            dataMinDoubleSpinBox->setValue(dataMin);
            dataMaxDoubleSpinBox->setValue(dataMax);
            logCheckBox->setChecked(log);
            correctionLorentzCheckBox->setChecked(lorentzCorrection);
            autoBackgroundCorrectionCheckBox->setChecked(autoBackgroundCorrection);
            
            file.close();

            if (folderSet.size() > 0)
            {
                emit imageChanged(*folderSet.current()->current());
                emit centerImage();
            }
        }
    }
}

//void MainWindow::saveProject()
//{
//    QFileDialog dialog;
//    dialog.setDefaultSuffix("txt");
//    QString path = dialog.getSaveFileName(this, tr("Save project"), "", tr(".qt (*.qt);; All Files (*)"));

//    if (path != "")
//    {
//        QFile file(path);
//        if (file.open(QIODevice::WriteOnly))
//        {
//            QDataStream out(&file);
            
//            out << folderSet;
//            out << tsfTextureComboBox->currentText();
//            out << tsfAlphaComboBox->currentText();
//            out << (double) dataMinDoubleSpinBox->value();
//            out << (double) dataMaxDoubleSpinBox->value();
//            out << (bool) imageLogCheckBox->isChecked();
//            out << (bool) correctionLorentzCheckBox->isChecked();            
            
//            file.close();
//        }
//    }
    
//    hasPendingChanges = false;
//}

//void MainWindow::loadProject()
//{
//    QString path = QFileDialog::getOpenFileName(this, tr("Open project"), "", tr(".qt (*.qt);; All Files (*)"));

//    if (path != "")
//    {
//        QFile file(path);
//        if (file.open(QIODevice::ReadOnly))
//        {
//            QString tsfTexture;
//            QString tsfAlpha;
//            double dataMin;
//            double dataMax;
//            bool log;
//            bool correction;
            
//            QDataStream in(&file);
            
//            in >> folderSet >> tsfTexture >> tsfAlpha >> dataMin >> dataMax >> log >> correction;
            
//            tsfTextureComboBox->setCurrentText(tsfTexture);
//            tsfAlphaComboBox->setCurrentText(tsfAlpha);
//            dataMinDoubleSpinBox->setValue(dataMin);
//            dataMaxDoubleSpinBox->setValue(dataMax);
//            imageLogCheckBox->setChecked(log);
//            correctionLorentzCheckBox->setChecked(correction);
            
//            file.close();

//            if (folderSet.size() > 0)
//            {
//                emit pathChanged(folderSet.current()->current()->path());
//                emit selectionChanged(folderSet.current()->current()->selection());
//            }
//        }
//    }
//}

void MainWindow::setSelection(Selection rect)
{
    if (folderSet.size() > 0)
    {
        folderSet.current()->current()->setSelection(rect);
        
        hasPendingChanges = true;
    }
}

void MainWindow::newScriptFile()
{
    if (maybeSave())
    {
        scriptTextEdit->clear();
        setCurrentFile("");
    }
}

void MainWindow::openScript()
{
    if (maybeSave())
    {
        current_script_path = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr(".txt (*.txt);; All Files (*)"));
        if (!current_script_path.isEmpty())
        {
            QFileInfo fileInfo = QFileInfo(current_script_path);
            if (fileInfo.size() < 5000000) loadFile(current_script_path);
            else print("\nFile is too large!");
            
            setWindowTitle(tr("Nebula[*] (")+current_script_path+")");
        }
    }
}

void MainWindow::initializeActions()
{


    // Actions
    scriptingAct = new QAction(QIcon(":/art/script.png"), tr("&Toggle scripting mode"), this);
    scriptingAct->setCheckable(true);
    scriptingAct->setChecked(false);
    newAct = new QAction(QIcon(":/art/new.png"), tr("&New script"), this);
    newAct->setVisible(false);
    openAct = new QAction(QIcon(":/art/open.png"), tr("&Open script"), this);
    openAct->setVisible(false);
    saveAct = new QAction(QIcon(":/art/save.png"), tr("&Save script"), this);
    saveAct->setVisible(false);
    runScriptAct = new QAction(QIcon(":/art/forward.png"), tr("Run"), this);
    saveAsAct = new QAction(QIcon(":/art/save.png"), tr("Save script &As..."), this);
    exitAct = new QAction(tr("E&xit program"), this);
    aboutAct = new QAction(tr("&About Nebula"), this);
    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutOpenCLAct = new QAction(tr("About OpenCL"), this);
    aboutOpenGLAct = new QAction(tr("About OpenGL"), this);
    openSvoAct = new QAction(QIcon(":/art/open.png"), tr("Open SVO"), this);
    saveSVOAct = new QAction(QIcon(":/art/saveScript.png"), tr("Save SVO"), this);
    saveLoadedSvoAct = new QAction(QIcon(":/art/save.png"), tr("Save Current SVO"), this);
    log3DAct =  new QAction(QIcon(":/art/log.png"), tr("Toggle logarithmic"), this);
    log3DAct->setCheckable(true);
    log3DAct->setChecked(true);
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
    labFrameAct = new QAction(QIcon(":/art/labframe.png"), tr("&View Lab Frame"), this);
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
    
    // Action Tips
    newAct->setStatusTip(tr("Create a new file"));
    openAct->setStatusTip(tr("Open an existing file"));
    saveAct->setStatusTip(tr("Save the document to disk"));
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    runScriptAct->setStatusTip(tr("Run the script"));
    exitAct->setStatusTip(tr("Exit Nebula"));
    aboutAct->setStatusTip(tr("About"));
    aboutQtAct->setStatusTip(tr("About Qt"));
    aboutOpenCLAct->setStatusTip(tr("About OpenCL"));
    aboutOpenGLAct->setStatusTip(tr("About OpenGL"));

    // Shortcuts
    newAct->setShortcuts(QKeySequence::New);
    openAct->setShortcuts(QKeySequence::Open);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    exitAct->setShortcuts(QKeySequence::Quit);
}

//void MainWindow::omitFile()
//{
//    int value = imageSpinBox->value();

//    emit omitFile(file_paths[value]);

//    if (value < file_paths.size())
//    {
//        file_paths.removeAt(value);



//        // Feature/Bug: Should also un-select the file in question in the file browser, or use a "load files" explicit button. Also, the file_paths array is superfluous
//    }

//    if (value < files.size())
//    {
//        files.removeAt(value);

//        print("\nRemoved file "+QString::number(value)+"of"+QString::number(files.size()));
//    }

//    if (value >= file_paths.size()) value = file_paths.size() - 1;
//    emit imagePreviewChanged(file_paths[value]);
//    emit setHeader(file_paths[value]);
//    imageLabel->setText(file_paths[value]);
//}

void MainWindow::saveScript()
{
    if (curFile.isEmpty())
    {
        saveScriptAs();
    }
    else
    {
        saveFile(curFile);
    }
}


void MainWindow::saveScriptAs()
{
    QString fileName = QFileDialog::getSaveFileName(this);
    if (!fileName.isEmpty()) saveFile(fileName);
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

void MainWindow::documentWasModified()
{
    setWindowModified(scriptTextEdit->document()->isModified());
}


void MainWindow::openUnitcellFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr(".par (*.par);; All Files (*)"));

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
    toolChainWidget->hide();
    fileHeaderDock->hide();
    outputDockWidget->hide();
    fileDockWidget->hide();
    graphicsDockWidget->hide();
    unitCellDock->hide();
    functionDockWidget->hide();
    svoHeaderDock->hide();
    
//    if (tab==1) file_paths = fileSelectionModel->getFiles();


    if ((tab==0) || (tab==1)) toolChainWidget->show();
    else toolChainWidget->hide();
    
    if ((tab==0) || (tab==1)) fileHeaderDock->show();
    else fileHeaderDock->hide();
    
    if ((tab==0) || (tab==1)) outputDockWidget->show();
    else outputDockWidget->hide();
    
    if (tab==1) fileDockWidget->show();
    else fileDockWidget->hide();
    
    if (tab==2) graphicsDockWidget->show();
    else graphicsDockWidget->hide();
    
    if (tab==2) unitCellDock->show();
    else unitCellDock->hide();
     
    if (tab==2) functionDockWidget->show();
    else functionDockWidget->hide();
    
    if (tab==2) svoHeaderDock->show();
    else svoHeaderDock->hide();
}


void MainWindow::initializeConnects()
{
    /* this <-> volumeRenderWidget */
    connect(this->qualitySlider, SIGNAL(valueChanged(int)), volumeRenderWindow->getWorker(), SLOT(setQuality(int)));
    connect(this->qualitySlider, SIGNAL(sliderReleased()), volumeRenderWindow->getWorker(), SLOT(refreshTexture()));
    connect(this->scalebarAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(setScalebar()));
    connect(this->sliceAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(setSlicing()));
    connect(this->integrate2DAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(setIntegration2D()));
    connect(this->integrate3DAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(setIntegration3D()));
    connect(this->shadowAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(setShadow()));
    connect(this->orthoGridAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(setOrthoGrid()));
    connect(this->projectionAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(setProjection()));
    connect(this->backgroundAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(setBackground()));
    connect(this->log3DAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(setLogarithmic()));
    connect(this->logIntegrate2DAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(setLogarithmic2D()));
    connect(this->dataStructureAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(setDataStructure()));
    connect(this->volumeRenderTsfComboBox, SIGNAL(currentIndexChanged(int)), volumeRenderWindow->getWorker(), SLOT(setTsfColor(int)));
    connect(this->volumeRenderViewModeComboBox, SIGNAL(currentIndexChanged(int)), volumeRenderWindow->getWorker(), SLOT(setViewMode(int)));
    connect(this->tsfAlphaComboBox, SIGNAL(currentIndexChanged(int)), volumeRenderWindow->getWorker(), SLOT(setTsfAlpha(int)));
    connect(this->volumeRenderDataMinSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setDataMin(double)));
    connect(this->volumeRenderDataMaxSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setDataMax(double)));
    connect(this->volumeRenderAlphaSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setAlpha(double)));
    connect(this->volumeRenderBrightnessSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setBrightness(double)));
    connect(this->functionToggleButton, SIGNAL(clicked()), volumeRenderWindow->getWorker(), SLOT(setModel()));
    connect(this->funcParamASpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setModelParam0(double)));
    connect(this->funcParamBSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setModelParam1(double)));
    connect(this->funcParamCSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setModelParam2(double)));
    connect(this->funcParamDSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setModelParam3(double)));
    connect(volumeRenderWindow->getWorker(), SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)), Qt::BlockingQueuedConnection);
    connect(this, SIGNAL(captureFrameBuffer(QString)), volumeRenderWindow->getWorker(), SLOT(takeScreenShot(QString)));
    connect(this->alignLabXtoSliceXAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(alignLabXtoSliceX()));
    connect(this->alignLabYtoSliceYAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(alignLabYtoSliceY()));
    connect(this->alignLabZtoSliceZAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(alignLabZtoSliceZ()));
    connect(this->alignSliceToLabAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(alignSliceToLab()));
    
    connect(this->rotateLeftAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(rotateLeft()));
    connect(this->rotateRightAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(rotateRight()));
    connect(this->rotateUpAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(rotateUp()));
    connect(this->rotateDownAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(rotateDown()));
    connect(this->rollCW, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(rollCW()));
    connect(this->rollCCW, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(rollCCW()));
    connect(this->rulerAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(toggleRuler()));
    connect(this->markAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(addMarker()));
    connect(this->labFrameAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(setLabFrame()));
    connect(this->rotateCellButton, SIGNAL(clicked()), volumeRenderWindow->getWorker(), SLOT(setURotation()));
    connect(this->toggleCellButton, SIGNAL(clicked()), volumeRenderWindow->getWorker(), SLOT(setUnitcell()));
    connect(this->hSpinBox, SIGNAL(valueChanged(int)), volumeRenderWindow->getWorker(), SLOT(setHCurrent(int)));
    connect(this->kSpinBox, SIGNAL(valueChanged(int)), volumeRenderWindow->getWorker(), SLOT(setKCurrent(int)));
    connect(this->lSpinBox, SIGNAL(valueChanged(int)), volumeRenderWindow->getWorker(), SLOT(setLCurrent(int)));
    connect(this->integrateCountsAct, SIGNAL(triggered()), volumeRenderWindow->getWorker(), SLOT(setCountIntegration()));
    
    /* this <-> this */
    connect(this->aNormSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setUB_a(double)), Qt::QueuedConnection);
    connect(this->bNormSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setUB_b(double)), Qt::QueuedConnection);
    connect(this->cNormSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setUB_c(double)), Qt::QueuedConnection);
    
    connect(this->alphaNormSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setUB_alpha(double)), Qt::QueuedConnection);
    connect(this->betaNormSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setUB_beta(double)), Qt::QueuedConnection);
    connect(this->gammaNormSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setUB_gamma(double)), Qt::QueuedConnection);
    
    
    connect(this->scriptingAct, SIGNAL(toggled(bool)), fileBrowserWidget, SLOT(setHidden(bool)));
    connect(this->scriptingAct, SIGNAL(toggled(bool)), fileSelectionFilter, SLOT(setDisabled(bool)));
    connect(this->scriptingAct, SIGNAL(toggled(bool)), scriptTextEdit, SLOT(setVisible(bool)));
    connect(this->scriptingAct, SIGNAL(toggled(bool)), readScriptButton, SLOT(setVisible(bool)));
    connect(this->scriptingAct, SIGNAL(toggled(bool)), newAct, SLOT(setVisible(bool)));
    connect(this->scriptingAct, SIGNAL(toggled(bool)), openAct, SLOT(setVisible(bool)));
    connect(this->scriptingAct, SIGNAL(toggled(bool)), saveAct, SLOT(setVisible(bool)));
    
    connect(this->screenshotAct, SIGNAL(triggered()), this, SLOT(takeScreenshot()));
    connect(scriptTextEdit->document(), SIGNAL(contentsChanged()), this, SLOT(documentWasModified()));
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(setTab(int)));
    connect(openSvoAct, SIGNAL(triggered()), this, SLOT(openSvo()));
    connect(saveSVOAct, SIGNAL(triggered()), this, SLOT(saveSvo()));
    connect(saveLoadedSvoAct, SIGNAL(triggered()), this, SLOT(saveLoadedSvo()));
    connect(saveSvoButton, SIGNAL(clicked()), this, SLOT(saveSvo()));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newScriptFile()));
    connect(openAct, SIGNAL(triggered()), this, SLOT(openScript()));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(saveScript()));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveScriptAs()));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
    connect(aboutOpenCLAct, SIGNAL(triggered()), this, SLOT(aboutOpenCL()));
    connect(aboutOpenGLAct, SIGNAL(triggered()), this, SLOT(aboutOpenGL()));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    
    /*this <-> misc*/
    connect(fileSelectionFilter, SIGNAL(textChanged(QString)), fileSelectionModel, SLOT(setStringFilter(QString)));
}

void MainWindow::setGenericProgressFormat(QString str)
{
    progressBar->setFormat(str);
}

void MainWindow::saveSvo()
{
    if (svo_inprocess.index.size() > 0)
    {
        QFileDialog dialog;
        dialog.setDefaultSuffix("svo");
        QString file_name = dialog.getSaveFileName(this, tr("Save File"), "", tr(".svo (*.svo);; All Files (*)"));

        if (file_name != "")
        {
            // View settings
            svo_inprocess.view_mode = 0;
            svo_inprocess.view_tsf_style = 2;
            svo_inprocess.view_tsf_texture = 1;
            svo_inprocess.view_data_min = 0;
            svo_inprocess.view_data_max = 100;
            svo_inprocess.view_alpha = 0.05;
            svo_inprocess.view_brightness = 2.0;
            
            qDebug() << "Get saved! Creation";
            
            svo_inprocess.save(file_name);
        }
    }
}


void MainWindow::saveLoadedSvo()
{
    if (svo_loaded.getBrickNumber() > 0)
    {
        QFileDialog dialog;
        dialog.setDefaultSuffix("svo");
        QString file_name = dialog.getSaveFileName(this, tr("Save File"), "", tr(".svo (*.svo);; All Files (*)"));

        if (file_name != "")
        {
            // View settings
            svo_loaded.view_mode = volumeRenderViewModeComboBox->currentIndex();
            svo_loaded.view_tsf_style = tsfAlphaComboBox->currentIndex();
            svo_loaded.view_tsf_texture = volumeRenderTsfComboBox->currentIndex();
            svo_loaded.view_data_min = volumeRenderDataMinSpinBox->value();
            svo_loaded.view_data_max = volumeRenderDataMaxSpinBox->value();
            svo_loaded.view_alpha = volumeRenderAlphaSpinBox->value();
            svo_loaded.view_brightness = volumeRenderBrightnessSpinBox->value();
            
            qDebug() << "Get saved!Soft" << svo_loaded.view_data_min;
            
            svo_loaded.setUB(volumeRenderWindow->getWorker()->getUBMatrix());
            svo_loaded.setMetaData(svoHeaderEdit->toPlainText());
            svo_loaded.save(file_name);
        }
    }
}


void MainWindow::openSvo()
{
    current_svo_path = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr(".svo (*.svo);; All Files (*)"));

    if ((current_svo_path != ""))
    {
        svo_loaded.open(current_svo_path);
        volumeRenderWindow->getWorker()->setSvo(&(svo_loaded));
        
        volumeRenderViewModeComboBox->setCurrentIndex(svo_loaded.view_mode);
        tsfAlphaComboBox->setCurrentIndex(svo_loaded.view_tsf_style);
        volumeRenderTsfComboBox->setCurrentIndex(svo_loaded.view_tsf_texture);
        volumeRenderAlphaSpinBox->setValue(svo_loaded.view_alpha);
        volumeRenderBrightnessSpinBox->setValue(svo_loaded.view_brightness);
        volumeRenderDataMinSpinBox->setValue(svo_loaded.view_data_min);
        volumeRenderDataMaxSpinBox->setValue(svo_loaded.view_data_max);
        
        UBMatrix<double> UB;
        
        UB = svo_loaded.getUB();
        
        if (UB.size() == 3*3)
        {
            volumeRenderWindow->getWorker()->setUBMatrix(UB);
        
//            UB.print(2,"UB loaded");
        
            alphaNormSpinBox->setValue(UB.alpha()*180.0/pi);
            betaNormSpinBox->setValue(UB.beta()*180.0/pi);
            gammaNormSpinBox->setValue(UB.gamma()*180.0/pi);
            
            aNormSpinBox->setValue(UB.a());
            bNormSpinBox->setValue(UB.b());
            cNormSpinBox->setValue(UB.c());
        }
        
        svoHeaderEdit->setDocumentTitle(current_svo_path);
        svoHeaderEdit->setPlainText(svo_loaded.getMetaData());

        print("\n["+QString(this->metaObject()->className())+"] Loaded file: \""+current_svo_path+"\"");
        
        setWindowTitle(tr("Nebula[*] (")+current_svo_path+")");
    }
}



void MainWindow::initializeMenus()
{
    mainMenu = new QMenuBar;
    scriptMenu = new QMenu(tr("&File"));
    viewMenu = new QMenu(tr("V&iew"));
    helpMenu = new QMenu(tr("&Help"));

    scriptMenu->addAction(newAct);
    scriptMenu->addAction(openAct);
    scriptMenu->addAction(saveAct);
    scriptMenu->addAction(saveAsAct);
    scriptMenu->addSeparator();
    scriptMenu->addAction(exitAct);

    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
    helpMenu->addAction(aboutOpenCLAct);
    helpMenu->addAction(aboutOpenGLAct);

    mainMenu->addMenu(scriptMenu);
    mainMenu->addMenu(viewMenu);
    mainMenu->addSeparator();
    mainMenu->addMenu(helpMenu);
}



void MainWindow::initializeInteractives()
{
    mainWidget = new QWidget(this);
    mainLayout = new QGridLayout;
    
    /* Top Widget */
    {
        topWidget = new QWidget(mainWidget);

        // Buttons
        readScriptButton = new QPushButton;
        readScriptButton->setIcon(QIcon(":/art/proceed.png"));
        readScriptButton->setIconSize(QSize(32,32));
        readScriptButton->setText("Run Script ");
        readScriptButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        readScriptButton->setVisible(false);
        
        setFileButton = new QPushButton;
        setFileButton->setIcon(QIcon(":/art/proceed.png"));
        setFileButton->setIconSize(QSize(24,24));
        setFileButton->setText("Set ");

        readFileButton = new QPushButton;
        readFileButton->setIcon(QIcon(":/art/proceed.png"));
        readFileButton->setIconSize(QSize(24,24));
        readFileButton->setText("Read ");
        readFileButton->setEnabled(false);

        projectFileButton = new QPushButton;
        projectFileButton->setIcon(QIcon(":/art/proceed.png"));
        projectFileButton->setIconSize(QSize(24,24));
        projectFileButton->setText("Project ");
        projectFileButton->setEnabled(false);

        allInOneButton = new QPushButton;
        allInOneButton->setIcon(QIcon(":/art/fast_proceed.png"));
        allInOneButton->setText("All of Above (reduced memory consumption) ");
        allInOneButton->setIconSize(QSize(24,24));

        killButton = new QPushButton;
        killButton->setIcon(QIcon(":/art/kill.png"));
        killButton->setText("Kill ");
        killButton->setIconSize(QSize(24,24));
        killButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        toolChainWidget = new QWidget;
        QGridLayout * toolChainLayout = new QGridLayout;
        toolChainLayout->setSpacing(0);
        toolChainLayout->setContentsMargins(0,0,0,0);
        toolChainLayout->setColumnStretch(1,1);
        toolChainLayout->setColumnStretch(2,1);
        toolChainLayout->setColumnStretch(3,1);
        toolChainLayout->setColumnStretch(4,1);
        toolChainLayout->addWidget(readScriptButton,0,0,2,1);
        toolChainLayout->addWidget(setFileButton,0,1,1,1);
        toolChainLayout->addWidget(readFileButton,0,2,1,1);
        toolChainLayout->addWidget(projectFileButton,0,3,1,1);
        toolChainLayout->addWidget(killButton,0,4,2,1);
        toolChainLayout->addWidget(allInOneButton,1,1,1,3);
        toolChainWidget->setLayout(toolChainLayout);

        // Layout
        QGridLayout * topLayout = new QGridLayout;
        topLayout->setSpacing(0);
        topLayout->setContentsMargins(0,0,0,0);
        topLayout->addWidget(mainMenu,0,0,1,1);
        topLayout->addWidget(toolChainWidget,1,0,1,1);
        topWidget->setLayout(topLayout);
    }

    
    /*      File Select Widget       */
    {
        setFilesWidget = new QWidget;

        // Script text edit
        scriptTextEdit = new QPlainTextEdit;
        scriptTextEdit->hide();
        script_highlighter = new Highlighter(scriptTextEdit->document());
        loadFile(":/default/example_script.txt");
        
        
        // Toolbar
        fileSelectionToolBar = new QToolBar(tr("File selection toolbar"));
        fileSelectionToolBar->addAction(scriptingAct);
        fileSelectionToolBar->addAction(newAct);
        fileSelectionToolBar->addAction(openAct);
        fileSelectionToolBar->addAction(saveAct);
        
        
        fileSelectionFilter = new QLineEdit;
        fileSelectionToolBar->addWidget(fileSelectionFilter);
        
        // File browser
        fileBrowserWidget = new QWidget;
        fileSelectionModel  = new FileSelectionModel;
        fileSelectionModel->setRootPath(QDir::rootPath());

        fileSelectionTree = new FileTreeView;
        fileSelectionTree->setModel(fileSelectionModel);
        
        loadPathsPushButton = new QPushButton;//(QIcon(":/art/download.png"),"Load selected files"); //QIcon(":/art/rotate_down.png"),
        loadPathsPushButton->setIcon(QIcon(":/art/download.png"));
        loadPathsPushButton->setIconSize(QSize(86,86));
        connect(loadPathsPushButton, SIGNAL(clicked()), this, SLOT(loadPaths()));
        
        connect(fileSelectionTree, SIGNAL(fileChanged(QString)), this, SLOT(setHeader(QString)));
//        connect(this, SIGNAL(omitFile(QString)), fileSelectionModel, SLOT(removeFile(QString)));

        QGridLayout * fileBrowserLayout = new QGridLayout;
        fileBrowserLayout->setSpacing(0);
        fileBrowserLayout->setContentsMargins(0,0,0,0);
        fileBrowserLayout->addWidget(fileSelectionTree,0,0,1,1);

        fileBrowserWidget->setLayout(fileBrowserLayout);

        // Layout
        QGridLayout * scriptLayout = new QGridLayout;
        scriptLayout->setSpacing(0);
        scriptLayout->setContentsMargins(0,0,0,0);
        scriptLayout->addWidget(fileSelectionToolBar,0,0,1,2);
        scriptLayout->addWidget(scriptTextEdit,1,0,1,2);
        scriptLayout->addWidget(fileBrowserWidget,2,0,1,2);
        scriptLayout->addWidget(loadPathsPushButton,3,0,1,2);
        setFilesWidget->setLayout(scriptLayout);
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
//        volumeRenderWindow->setAnimating(true);
        volumeRenderWindow->initializeWorker();

        volumeRenderWidget = QWidget::createWindowContainer(volumeRenderWindow);
        volumeRenderWidget->setFocusPolicy(Qt::TabFocus);

        // Toolbar
        viewToolBar = new QToolBar(tr("3D View"));
        viewToolBar->addAction(openSvoAct);
        viewToolBar->addAction(saveLoadedSvoAct);
        
        viewToolBar->addSeparator();
        viewToolBar->addAction(projectionAct);
        
        viewToolBar->addAction(markAct);
        viewToolBar->addAction(scalebarAct);
        viewToolBar->addAction(labFrameAct);
        
        viewToolBar->addAction(shadowAct);
        viewToolBar->addAction(dataStructureAct);
        
//        viewToolBar->addSeparator();
//        viewToolBar->addAction(integrateCountsAct);
//        viewToolBar->addAction(integrate3DAct);
//        viewToolBar->addAction(log3DAct);
        viewToolBar->addSeparator();
        
        viewToolBar->addAction(integrate2DAct);
        viewToolBar->addAction(logIntegrate2DAct);
        viewToolBar->addSeparator();
        
        viewToolBar->addAction(alignLabXtoSliceXAct);
        viewToolBar->addAction(alignLabYtoSliceYAct);
        viewToolBar->addAction(alignLabZtoSliceZAct);
        viewToolBar->addAction(alignSliceToLabAct);
        viewToolBar->addAction(rotateLeftAct);
        viewToolBar->addAction(rotateRightAct);
        viewToolBar->addAction(rotateDownAct);
        viewToolBar->addAction(rotateUpAct);
        viewToolBar->addAction(rollCW);
        viewToolBar->addAction(rollCCW);
        viewToolBar->addSeparator();
        
        viewToolBar->addAction(backgroundAct);
        viewToolBar->addAction(screenshotAct);
        

        // Layout
        QGridLayout * viewLayout = new QGridLayout;
        viewLayout->setSpacing(0);
        viewLayout->setContentsMargins(0,0,0,0);
        viewLayout->addWidget(viewToolBar,0,0,1,1);
        viewLayout->addWidget(volumeRenderWidget,1,0,1,1);

        viewWidget = new QWidget;
        viewWidget->setLayout(viewLayout);
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
//        imagePreviewWindow->setAnimating(true);
        imagePreviewWindow->initializeWorker();
        
        imageDisplayWidget = QWidget::createWindowContainer(imagePreviewWindow);
        imageDisplayWidget->setFocusPolicy(Qt::TabFocus);
        
        connect(this, SIGNAL(imageChanged(ImageInfo)), imagePreviewWindow->getWorker(), SLOT(setFrame(ImageInfo)));

        imageWidget = new QMainWindow;

//        imageLabel = new QLabel("---");
        // Toolbar
        pathLineEdit = new QLineEdit("/path/to/file");
        pathLineEdit->setReadOnly(true);
        connect(this, SIGNAL(pathChanged(QString)), pathLineEdit, SLOT(setText(QString)));
        
        imageFastBackButton = new QPushButton;
        imageFastBackButton->setIcon(QIcon(":art/fast_back.png"));
        imageFastBackButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        connect(imageFastBackButton, SIGNAL(clicked()), this, SLOT(batchBackward()));

        imageSlowBackButton = new QPushButton;
        imageSlowBackButton->setIcon(QIcon(":art/back.png"));
        imageSlowBackButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        connect(imageSlowBackButton, SIGNAL(clicked()), this, SLOT(previousFrame()));

//        imageSpinBox = new QSpinBox;
//        imageSpinBox->setRange(0,100000);
        
//        connect(imageSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setDisplayFile(int)));

        imageFastForwardButton = new QPushButton;
        imageFastForwardButton->setIcon(QIcon(":art/fast_forward.png"));
        imageFastForwardButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        connect(imageFastForwardButton, SIGNAL(clicked()), this, SLOT(batchForward()));

        imageSlowForwardButton = new QPushButton;
        imageSlowForwardButton->setIcon(QIcon(":art/forward.png"));
        imageSlowForwardButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        connect(imageSlowForwardButton, SIGNAL(clicked()), this, SLOT(nextFrame()));

        
//        omitFrameButton = new QPushButton("Remove");
//        omitFrameButton->setIcon(QIcon(":art/kill.png"));
//        omitFrameButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
//        connect(omitFrameButton, SIGNAL(clicked()), this, SLOT(omitFile()));
        removeCurrentPushButton = new QPushButton(QIcon(":/art/kill.png"),"Remove frame");
        connect(removeCurrentPushButton, SIGNAL(clicked()), this, SLOT(removeImage()));
        connect(this, SIGNAL(pathRemoved(QString)), fileSelectionModel, SLOT(removeFile(QString)));

//        imageModeCB = new QComboBox;
//        imageModeCB->addItem("Raw");
//        imageModeCB->addItem("Corrected");
//        connect(imageModeCB, SIGNAL(currentIndexChanged(int)), imagePreviewWindow->getWorker(), SLOT(setMode(int)));
//        connect(imageModeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(refreshDisplayFile()));
        
        imageToolBar = new QToolBar("Image");
    
        saveProjectAction = new QAction(QIcon(":/art/save.png"), tr("Save project"), this);
        loadProjectAction = new QAction(QIcon(":/art/open.png"), tr("Load project"), this);
        
        squareAreaSelectAction = new QAction(QIcon(":/art/select.png"), tr("Toggle pixel selection"), this);
        squareAreaSelectAction->setCheckable(true);
        squareAreaSelectAction->setChecked(false);
        
        centerImageAction = new QAction(QIcon(":/art/center.png"), tr("Center image"), this);
        centerImageAction->setCheckable(false);
    
        imageToolBar->addAction(saveProjectAction);
        imageToolBar->addAction(loadProjectAction);
        imageToolBar->addAction(centerImageAction);
        imageToolBar->addAction(squareAreaSelectAction);
        imageToolBar->addWidget(pathLineEdit);
    
        imageWidget->addToolBar(Qt::TopToolBarArea, imageToolBar);
        
        QGridLayout * imageLayout = new QGridLayout;
        imageLayout->setRowStretch(1,1);
//        imageLayout->addWidget(imageToolBar,0,0,1,8);
        imageLayout->addWidget(imageDisplayWidget,1,0,1,8);
        imageLayout->addWidget(imageFastBackButton,2,0,1,2);
        imageLayout->addWidget(imageSlowBackButton,2,2,1,1);
//        imageLayout->addWidget(imageSpinBox,2,3,1,2);
        imageLayout->addWidget(imageSlowForwardButton,2,5,1,1);
        imageLayout->addWidget(imageFastForwardButton,2,6,1,2);
//        imageLayout->addWidget(imageModeCB,3,0,1,4);
        imageLayout->addWidget(removeCurrentPushButton,3,0,1,8);
        
        imageCentralWidget = new QWidget;
        imageCentralWidget->setLayout(imageLayout);
        imageWidget->setCentralWidget(imageCentralWidget);
    }
    
    /* Image browser settings widget */
    {
        imageModeComboBox = new QComboBox;
        imageModeComboBox->addItem("Normal");
        imageModeComboBox->addItem("Variance");
        imageModeComboBox->addItem("Skewness");
    
        tsfTextureComboBox = new QComboBox;
        tsfTextureComboBox->addItem(trUtf8("Rainbow"));
        tsfTextureComboBox->addItem(trUtf8("Hot"));
        tsfTextureComboBox->addItem(trUtf8("Hsv"));
        tsfTextureComboBox->addItem(trUtf8("Galaxy"));
        tsfTextureComboBox->addItem(trUtf8("Binary"));
        tsfTextureComboBox->addItem(trUtf8("Yranib"));
    
        tsfAlphaComboBox = new QComboBox;
        tsfAlphaComboBox->addItem("Linear");
        tsfAlphaComboBox->addItem("Exponential");
        tsfAlphaComboBox->addItem("Uniform");
        tsfAlphaComboBox->addItem("Opaque");
    
        dataMinDoubleSpinBox = new QDoubleSpinBox;
        dataMinDoubleSpinBox->setRange(-1e9,1e9);
        dataMinDoubleSpinBox->setAccelerated(true);
        dataMinDoubleSpinBox->setPrefix("Data min: ");
    
        dataMaxDoubleSpinBox = new QDoubleSpinBox;
        dataMaxDoubleSpinBox->setRange(-1e9,1e9);
        dataMaxDoubleSpinBox->setAccelerated(true);
        dataMaxDoubleSpinBox->setPrefix("Data max: ");
        
        logCheckBox = new QCheckBox("Log");
    
        QGridLayout * settingsLayout = new QGridLayout;
        settingsLayout->addWidget(imageModeComboBox,0,1,1,2);
        settingsLayout->addWidget(tsfTextureComboBox,1,1,1,1);
        settingsLayout->addWidget(tsfAlphaComboBox,1,2,1,1);
        settingsLayout->addWidget(dataMinDoubleSpinBox,2,1,1,2);
        settingsLayout->addWidget(dataMaxDoubleSpinBox,3,1,1,2);
        settingsLayout->addWidget(logCheckBox,4,1,1,1);
        
    
    
        imageSettingsWidget = new QWidget;
        imageSettingsWidget->setLayout(settingsLayout);
    
    
        imageSettingsDock =  new QDockWidget("Display settings");
        imageSettingsDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
        imageSettingsDock->setWidget(imageSettingsWidget);
        imageSettingsDock->setFixedHeight(imageSettingsWidget->minimumSizeHint().height()*1.2);
        imageWidget->addDockWidget(Qt::RightDockWidgetArea, imageSettingsDock);
        
        connect(tsfTextureComboBox, SIGNAL(currentIndexChanged(int)), imagePreviewWindow->getWorker(), SLOT(setTsfTexture(int)));
        connect(tsfAlphaComboBox, SIGNAL(currentIndexChanged(int)), imagePreviewWindow->getWorker(), SLOT(setTsfAlpha(int)));
        connect(dataMinDoubleSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->getWorker(), SLOT(setDataMin(double)));
        connect(dataMaxDoubleSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->getWorker(), SLOT(setDataMax(double)));
        connect(logCheckBox, SIGNAL(toggled(bool)), imagePreviewWindow->getWorker(), SLOT(setLog(bool)));
        connect(imageModeComboBox, SIGNAL(currentIndexChanged(int)), imagePreviewWindow->getWorker(), SLOT(setMode(int)));
        connect(saveProjectAction, SIGNAL(triggered()), this, SLOT(saveProject()));
        connect(loadProjectAction, SIGNAL(triggered()), this, SLOT(loadProject()));
        connect(centerImageAction, SIGNAL(triggered()), imagePreviewWindow->getWorker(), SLOT(centerImage()));
        connect(this, SIGNAL(centerImage()), imagePreviewWindow->getWorker(), SLOT(centerImage()));
        connect(this, SIGNAL(selectionChanged(QRectF)), imagePreviewWindow->getWorker(), SLOT(setSelection(QRectF)));
        connect(imagePreviewWindow->getWorker(), SIGNAL(selectionChanged(QRectF)), this, SLOT(setSelection(QRectF)));
        connect(squareAreaSelectAction, SIGNAL(toggled(bool)), imagePreviewWindow->getWorker(), SLOT(setSelectionActive(bool)));
//        connect(loadPathsPushButton,SIGNAL(clicked()),imagePreviewWindow->getWorker(),SLOT(centerImage()));
//        connect(loadPathsPushButton,SIGNAL(clicked()),imagePreviewWindow->getWorker(),SLOT(centerImage()));
    }
    
    // Corrections dock widget
    {
        noiseCorrectionMinDoubleSpinBox = new QDoubleSpinBox;
        noiseCorrectionMinDoubleSpinBox->setRange(-1e6,1e6);
        noiseCorrectionMinDoubleSpinBox->setPrefix("Noise: ");
        
        noiseCorrectionMaxDoubleSpinBox = new QDoubleSpinBox;
        noiseCorrectionMaxDoubleSpinBox->setRange(-1e6,1e6);
        
        postCorrectionMinDoubleSpinBox = new QDoubleSpinBox;
        postCorrectionMinDoubleSpinBox->setRange(-1e6,1e6);
        postCorrectionMinDoubleSpinBox->setPrefix("PCT: ");
        
        postCorrectionMaxDoubleSpinBox = new QDoubleSpinBox;
        postCorrectionMaxDoubleSpinBox->setRange(-1e6,1e6);
        
        correctionLorentzCheckBox = new QCheckBox("Lorentz correction");
        autoBackgroundCorrectionCheckBox = new QCheckBox("Automatic background subtraction");
        
        connect(correctionLorentzCheckBox, SIGNAL(toggled(bool)), imagePreviewWindow->getWorker(), SLOT(setCorrection(bool)));
        
        QGridLayout * correctionLayout = new QGridLayout;
        correctionLayout->addWidget(noiseCorrectionMinDoubleSpinBox,0,0,1,1);
        correctionLayout->addWidget(noiseCorrectionMaxDoubleSpinBox,0,1,1,1);
        correctionLayout->addWidget(postCorrectionMinDoubleSpinBox,1,0,1,1);
        correctionLayout->addWidget(postCorrectionMaxDoubleSpinBox,1,1,1,1);
        correctionLayout->addWidget(correctionLorentzCheckBox,2,0,1,2);
        correctionLayout->addWidget(autoBackgroundCorrectionCheckBox,3,0,1,2);
        
        correctionWidget = new QWidget;
        correctionWidget->setLayout(correctionLayout);
        
        correctionDock =  new QDockWidget("Corrections");
        correctionDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
        correctionDock->setWidget(correctionWidget);
        correctionDock->setFixedHeight(correctionWidget->minimumSizeHint().height()*1.2);
        imageWidget->addDockWidget(Qt::RightDockWidgetArea, correctionDock);
        
        
        connect(this->noiseCorrectionMinDoubleSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->getWorker(), SLOT(setThresholdNoiseLow(double)),Qt::QueuedConnection);
        connect(this->noiseCorrectionMaxDoubleSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->getWorker(), SLOT(setThresholdNoiseHigh(double)),Qt::QueuedConnection);
        connect(this->postCorrectionMinDoubleSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->getWorker(), SLOT(setThresholdPostCorrectionLow(double)),Qt::QueuedConnection);
        connect(this->postCorrectionMaxDoubleSpinBox, SIGNAL(valueChanged(double)), imagePreviewWindow->getWorker(), SLOT(setThresholdPostCorrectionHigh(double)),Qt::QueuedConnection);
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

        qualitySlider = new QSlider(Qt::Horizontal);
        qualitySlider->setRange(1,100);
        qualitySlider->setToolTip("Set texture resolution");
        qualitySlider->setTickPosition(QSlider::NoTicks);

        graphicsDockWidget = new QDockWidget(tr("View Settings"), this);
        graphicsDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        graphicsWidget = new QWidget;

        QGridLayout * graphicsLayout = new QGridLayout;
        
        graphicsLayout->addWidget(label_mode,0,0,1,2);
        graphicsLayout->addWidget(volumeRenderViewModeComboBox,0,2,1,2);
        graphicsLayout->addWidget(label_texture,1,0,1,2);
        graphicsLayout->addWidget(volumeRenderTsfComboBox,1,2,1,1);
        graphicsLayout->addWidget(volumeRenderTsfAlphaComboBox,1,3,1,1);
        graphicsLayout->addWidget(volumeRenderDataMinSpinBox,2,0,1,4);
        graphicsLayout->addWidget(volumeRenderDataMaxSpinBox,3,0,1,4);
        graphicsLayout->addWidget(volumeRenderAlphaSpinBox,4,0,1,4);
        graphicsLayout->addWidget(volumeRenderBrightnessSpinBox,5,0,1,4);
        graphicsLayout->addWidget(label_quality,6,0,1,2);
        graphicsLayout->addWidget(qualitySlider,6,2,1,2);

        graphicsWidget->setLayout(graphicsLayout);
        graphicsDockWidget->setFixedHeight(graphicsWidget->minimumSizeHint().height()*1.1);
        graphicsDockWidget->setWidget(graphicsWidget);
        viewMenu->addAction(graphicsDockWidget->toggleViewAction());
        this->addDockWidget(Qt::RightDockWidgetArea, graphicsDockWidget);
    }
    
    /* Unitcell dock widget */
    {
        unitCellDock = new QDockWidget(tr("UB Matrix"), this);
        unitCellDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        unitCellWidget = new QWidget;
        
        // Real space unit cell
        aNormSpinBox = new QDoubleSpinBox;
        aNormSpinBox->setPrefix("a: ");
        aNormSpinBox->setSuffix(" °");
        bNormSpinBox = new QDoubleSpinBox;
        bNormSpinBox->setPrefix("b: ");
        bNormSpinBox->setSuffix(" °");
        cNormSpinBox = new QDoubleSpinBox;
        cNormSpinBox->setPrefix("c: ");
        cNormSpinBox->setSuffix(" °");
        
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
        
        connect(alignAlongAStarButton, SIGNAL(clicked()), volumeRenderWindow->getWorker(), SLOT(alignAStartoZ()));
        connect(alignAlongBStarButton, SIGNAL(clicked()), volumeRenderWindow->getWorker(), SLOT(alignBStartoZ()));
        connect(alignAlongCStarButton, SIGNAL(clicked()), volumeRenderWindow->getWorker(), SLOT(alignCStartoZ()));
        
        helpCellOverlayButton = new QPushButton("Help Cell");
        rotateCellButton = new QPushButton("Rotation");
        toggleCellButton = new QPushButton("Toggle");
        
        rotateCellButton->setCheckable(true);
        rotateCellButton->setChecked(false);
        
        helpCellOverlayButton->setCheckable(true);
        helpCellOverlayButton->setChecked(true);
        
        connect(helpCellOverlayButton, SIGNAL(clicked()), volumeRenderWindow->getWorker(), SLOT(setMiniCell()));
        
//        QLabel * aLabel = new QLabel("<i>a<i>");
//        QLabel * bLabel = new QLabel("<i>b<i>");
//        QLabel * cLabel = new QLabel("<i>c<i>");
//        QLabel * alphaLabel = new QLabel(trUtf8("<i>α<i>"));
//        QLabel * betaLabel = new QLabel(trUtf8( "<i>β<i>"));
//        QLabel * gammaLabel = new QLabel(trUtf8( "<i>γ<i>"));

//        QLabel * hLabel = new QLabel("<i>h<i>");
//        QLabel * kLabel = new QLabel("<i>k<i>");
//        QLabel * lLabel = new QLabel("<i>l<i>");
        
        QGridLayout * unitCellLayout = new QGridLayout; 
        
//        unitCellLayout->addWidget(aLabel,0,0,1,1);
        unitCellLayout->addWidget(aNormSpinBox,0,0,1,2);
//        unitCellLayout->addWidget(bLabel,0,2,1,1);
        unitCellLayout->addWidget(bNormSpinBox,0,2,1,2);
//        unitCellLayout->addWidget(cLabel,0,4,1,1);
        unitCellLayout->addWidget(cNormSpinBox,0,4,1,2);
        
//        unitCellLayout->addWidget(alphaLabel,1,0,1,1);
        unitCellLayout->addWidget(alphaNormSpinBox,1,0,1,2);
//        unitCellLayout->addWidget(betaLabel,1,2,1,1);
        unitCellLayout->addWidget(betaNormSpinBox,1,2,1,2);
//        unitCellLayout->addWidget(gammaLabel,1,4,1,1);
        unitCellLayout->addWidget(gammaNormSpinBox,1,4,1,2);
        
//        unitCellLayout->addWidget(hLabel,4,0,1,1);
        unitCellLayout->addWidget(hSpinBox,4,0,1,2);
//        unitCellLayout->addWidget(kLabel,4,2,1,1);
        unitCellLayout->addWidget(kSpinBox,4,2,1,2);
//        unitCellLayout->addWidget(lLabel,4,4,1,1);
        unitCellLayout->addWidget(lSpinBox,4,4,1,2);
        
        unitCellLayout->addWidget(alignAlongAStarButton,5,0,1,2);
        unitCellLayout->addWidget(alignAlongBStarButton,5,2,1,2);
        unitCellLayout->addWidget(alignAlongCStarButton,5,4,1,2);
        
        unitCellLayout->addWidget(helpCellOverlayButton,6,0,1,3);
        unitCellLayout->addWidget(rotateCellButton,6,3,1,3);
        
        unitCellLayout->addWidget(toggleCellButton,7,0,1,6);
        
        unitCellWidget->setLayout(unitCellLayout);
        
        unitCellDock->setWidget(unitCellWidget);
        
        unitCellDock->setFixedHeight(unitCellWidget->minimumSizeHint().height()*1.15);
        
        viewMenu->addAction(unitCellDock->toggleViewAction());
        this->addDockWidget(Qt::RightDockWidgetArea, unitCellDock);
    }

    
    /* SVO metadata text edit */
    {
        svoHeaderDock = new QDockWidget(tr("Metadata"), this);
        svoHeaderDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        svoHeaderEdit = new QPlainTextEdit;
        
        svoHeaderDock->setWidget(svoHeaderEdit);
        
        this->addDockWidget(Qt::RightDockWidgetArea, svoHeaderDock);

        svoHeaderDock->hide();
        
        connect(this, SIGNAL(pathChanged(QString)), this, SLOT(setHeader(QString)));
        
    }
    
    /* File Controls Widget */
    {
        fileControlsWidget = new QWidget;

        // Labels
        QLabel * labelA = new QLabel(QString("File format:"));
        QLabel * labelE = new QLabel(QString("Active angle:"));
        QLabel * labelJ = new QLabel("Voxelize settings");
        
        // Combo boxes and their labels
        formatComboBox = new QComboBox;
        formatComboBox->addItem("PILATUS CBF 1.2");
        formatComboBox->addItem("[ your file format here ]");
        
        activeAngleComboBox = new QComboBox;
        activeAngleComboBox->addItem("Phi");
        activeAngleComboBox->addItem("Kappa");
        activeAngleComboBox->addItem("Omega");
        activeAngleComboBox->addItem("Given by file");

        // Spin Boxes
//        noiseCorrectionMinDoubleSpinBox = new QDoubleSpinBox;
//        noiseCorrectionMinDoubleSpinBox->setRange(0, 1e9);
//        noiseCorrectionMinDoubleSpinBox->setSingleStep(1);
//        noiseCorrectionMinDoubleSpinBox->setAccelerated(1);
//        noiseCorrectionMinDoubleSpinBox->setDecimals(2);
//        noiseCorrectionMinDoubleSpinBox->setFocusPolicy(Qt::ClickFocus);
//        noiseCorrectionMinDoubleSpinBox->setPrefix("Noise cutoff: ");
        
//        noiseCorrectionMaxDoubleSpinBox = new QDoubleSpinBox;
//        noiseCorrectionMaxDoubleSpinBox->setRange(0, 1e9);
//        noiseCorrectionMaxDoubleSpinBox->setSingleStep(1);
//        noiseCorrectionMaxDoubleSpinBox->setAccelerated(1);
//        noiseCorrectionMaxDoubleSpinBox->setDecimals(2);
//        noiseCorrectionMaxDoubleSpinBox->setFocusPolicy(Qt::ClickFocus);

//        postCorrectionMinDoubleSpinBox = new QDoubleSpinBox;
//        postCorrectionMinDoubleSpinBox->setRange(0, 1e9);
//        postCorrectionMinDoubleSpinBox->setSingleStep(1);
//        postCorrectionMinDoubleSpinBox->setAccelerated(1);
//        postCorrectionMinDoubleSpinBox->setDecimals(2);
//        postCorrectionMinDoubleSpinBox->setFocusPolicy(Qt::ClickFocus);
//        postCorrectionMinDoubleSpinBox->setPrefix("Post corr. cutoff: ");

//        postCorrectionMaxDoubleSpinBox = new QDoubleSpinBox;
//        postCorrectionMaxDoubleSpinBox->setRange(0, 1e9);
//        postCorrectionMaxDoubleSpinBox->setSingleStep(1);
//        postCorrectionMaxDoubleSpinBox->setAccelerated(1);
//        postCorrectionMaxDoubleSpinBox->setDecimals(2);
//        postCorrectionMaxDoubleSpinBox->setFocusPolicy(Qt::ClickFocus);

        
        
//        connect(this->noiseCorrectionMinDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(refreshDisplayFile()));
//        connect(this->noiseCorrectionMaxDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(refreshDisplayFile()));
//        connect(this->postCorrectionMinDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(refreshDisplayFile()));
//        connect(this->postCorrectionMaxDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(refreshDisplayFile()));
//        QLabel * labelF = new QLabel("Δ<i>ω</i>:");
//        QLabel * labelG = new QLabel("Δ<i>κ</i>:");
//        QLabel * labelH = new QLabel("Δ<i>φ</i>:");
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
                                     
        svoLevelSpinBox = new QSpinBox;
        svoLevelSpinBox->setRange(1, 15);
        svoLevelSpinBox->setPrefix("Octtree levels: ");
        
        
        
        

        // Buttons
        voxelizeButton = new QPushButton;
        voxelizeButton->setIcon(QIcon(":/art/proceed.png"));
        voxelizeButton->setText("Voxelize");
        voxelizeButton->setEnabled(false);
        
        saveSvoButton = new QPushButton;
        saveSvoButton->setIcon(QIcon(":/art/save.png"));
        saveSvoButton->setText("Save Octtree");

        QGridLayout * reconstructLayout = new QGridLayout;
        reconstructLayout->addWidget(labelA,0,0,1,4);
        reconstructLayout->addWidget(formatComboBox,0,4,1,4);
        reconstructLayout->addWidget(labelE,1,0,1,4);
        reconstructLayout->addWidget(activeAngleComboBox,1,4,1,4);
//        reconstructLayout->addWidget(labelB,2,0,1,4);
//        reconstructLayout->addWidget(noiseCorrectionMinDoubleSpinBox,2,0,1,8);
//        reconstructLayout->addWidget(noiseCorrectionMaxDoubleSpinBox,2,6,1,2);
//        reconstructLayout->addWidget(labelC,3,0,1,4);
//        reconstructLayout->addWidget(postCorrectionMinDoubleSpinBox,3,0,1,8);
//        reconstructLayout->addWidget(postCorrectionMaxDoubleSpinBox,3,6,1,2);
//        reconstructLayout->addWidget(labelI,4,0,1,2);
//        reconstructLayout->addWidget(labelF,4,0,1,4);
        reconstructLayout->addWidget(omegaCorrectionSpinBox,4,0,1,8);
//        reconstructLayout->addWidget(labelG,5,0,1,4);
        reconstructLayout->addWidget(kappaCorrectionSpinBox,5,0,1,8);
//        reconstructLayout->addWidget(labelH,6,0,1,4);
        reconstructLayout->addWidget(phiCorrectionSpinBox,6,0,1,8);
        
        reconstructLayout->addWidget(labelJ,7,0,1,8, Qt::AlignHCenter);
//        reconstructLayout->addWidget(labelD,8,0,1,4);
        reconstructLayout->addWidget(svoLevelSpinBox,8,0,1,8);
        reconstructLayout->addWidget(voxelizeButton,9,0,1,8);
        reconstructLayout->addWidget(saveSvoButton,10,0,1,8);
        fileControlsWidget->setLayout(reconstructLayout);
        fileDockWidget = new QDockWidget(tr("Data Reduction Settings"), this);
        fileDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        fileDockWidget->setWidget(fileControlsWidget);
        fileDockWidget->setFixedHeight(fileControlsWidget->minimumSizeHint().height()*1.1);
        viewMenu->addAction(fileDockWidget->toggleViewAction());
        imageWidget->addDockWidget(Qt::RightDockWidgetArea, fileDockWidget);
    }
    
    // File header dock widget
    {
        fileHeaderEdit = new QPlainTextEdit;
        fileHeaderEdit->setReadOnly(true);
        
        fileHeaderDock = new QDockWidget(tr("Header Info"), this);
        fileHeaderDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        fileHeaderDock->setWidget(fileHeaderEdit);
        this->addDockWidget(Qt::RightDockWidgetArea, fileHeaderDock);
    }
    
    /* Function dock widget */
    {
//        QLabel * p0= new QLabel(QString("Variable 1: "));
//        QLabel * p1= new QLabel(QString("Variable 2: "));
//        QLabel * p2= new QLabel(QString("Variable 3: "));
//        QLabel * p3= new QLabel(QString("Variable 4: "));

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

        functionDockWidget = new QDockWidget(tr("Model Settings"), this);
        functionWidget = new QWidget;

        QGridLayout * functionLayout = new QGridLayout;
//        functionLayout->addWidget(p0,1,0,1,2);
        functionLayout->addWidget(funcParamASpinBox,1,0,1,4);
//        functionLayout->addWidget(p1,2,0,1,2);
        functionLayout->addWidget(funcParamBSpinBox,2,0,1,4);
//        functionLayout->addWidget(p2,3,0,1,2);
        functionLayout->addWidget(funcParamCSpinBox,3,0,1,4);
//        functionLayout->addWidget(p3,4,0,1,2);
        functionLayout->addWidget(funcParamDSpinBox,4,0,1,4);
        functionLayout->addWidget(functionToggleButton,5,0,1,4);
        functionWidget->setLayout(functionLayout);
        functionDockWidget->setWidget(functionWidget);
        functionDockWidget->setFixedHeight(functionWidget->minimumSizeHint().height());
        functionDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        viewMenu->addAction(functionDockWidget->toggleViewAction());
        this->addDockWidget(Qt::RightDockWidgetArea, functionDockWidget);
    }



    /* Output Widget */
    {
        outputDockWidget = new QDockWidget(tr("Output Terminal"), this);
        outputDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        botWidget = new QWidget;

        // Progress Bar
        progressBar = new QProgressBar;
        progressBar->setRange( 0, 100 );

        // Text output
        errorTextEdit = new QPlainTextEdit;
        error_highlighter = new Highlighter(errorTextEdit->document());
        errorTextEdit->setReadOnly(true);

        // Layout
        QGridLayout * botLayout = new QGridLayout;
        botLayout->setSpacing(0);
        botLayout->setContentsMargins(0,0,0,0);
        botLayout->addWidget(errorTextEdit, 0, 0, 1, 1);
        botLayout->addWidget(progressBar, 1, 0, 1, 1);

        botWidget->setLayout(botLayout);
        outputDockWidget->setWidget(botWidget);
        viewMenu->addAction(outputDockWidget->toggleViewAction());
        this->addDockWidget(Qt::BottomDockWidgetArea, outputDockWidget);
    }


    /*      Tab widget      */
    tabWidget = new QTabWidget(mainWidget);

    // Add tabs
    tabWidget->addTab(setFilesWidget, tr("File Selection"));
    tabWidget->addTab(imageWidget, tr("Reconstruction"));
    tabWidget->addTab(viewWidget, tr("Visualization"));

    // Put into main layout
    mainLayout->setContentsMargins(3,3,3,3);
    mainLayout->addWidget(topWidget,0,0,1,1);
    mainLayout->addWidget(tabWidget,1,0,1,1);
    mainWidget->setLayout(mainLayout);

    /* Script engine */
    rawFilesQs = engine.newVariant(file_paths);
    engine.globalObject().setProperty("files", rawFilesQs);
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
    QPoint pos = settings.value("pos", QPoint(0, 0)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    svoDir = settings.value("svoDir", "").toString();
    svoDir = settings.value("scriptDir", "").toString();
    resize(size);
    move(pos);
}

void MainWindow::writeSettings()
{
    QSettings settings("Norwegian University of Science and Technology", "Nebula");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("svoDir", svoDir);
    settings.setValue("scriptDir", svoDir);
}

bool MainWindow::maybeSave()
{
    if (scriptTextEdit->document()->isModified())
    {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("Nebula"),
            tr("The script has been modified.\n"
            "Do you want to saveScript your changes?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
        {
            saveScript();
            return true;
        }
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

void MainWindow::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this, tr("Nebula"),
            tr("Cannot read file %1:\n%2.")
            .arg(fileName)
            .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    #ifndef QT_NO_CURSOR
        QApplication::setOverrideCursor(Qt::WaitCursor);
    #endif
        scriptTextEdit->setPlainText(in.readAll());
    #ifndef QT_NO_CURSOR
        QApplication::restoreOverrideCursor();
    #endif

    setCurrentFile(fileName);
}

bool MainWindow::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox::warning(this, tr("Nebula"),
            tr("Cannot write file %1:\n%2.")
            .arg(fileName)
            .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    #ifndef QT_NO_CURSOR
        QApplication::setOverrideCursor(Qt::WaitCursor);
    #endif
        out << scriptTextEdit->toPlainText();
    #ifndef QT_NO_CURSOR
        QApplication::restoreOverrideCursor();
    #endif

    setCurrentFile(fileName);
    return true;
}



void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile = fileName;
    scriptTextEdit->document()->setModified(false);
    setWindowModified(false);

    QString shownName = curFile;
    if (curFile.isEmpty())
        shownName = "untitled.txt";
    setWindowFilePath(shownName);
}

void MainWindow::takeScreenshot()
{
    QString format = "jpg";
    QDateTime dateTime = dateTime.currentDateTime();
    QString initialPath = QDir::currentPath() + QString("/screenshot_"+dateTime.toString("yyyy_MM_dd_hh_mm_ss")) +"."+ format;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), initialPath,
                                                tr("%1 Files (*.%2);;All Files (*)")
                                                .arg(format.toUpper())
                                                .arg(format));
    emit captureFrameBuffer(fileName);
}

