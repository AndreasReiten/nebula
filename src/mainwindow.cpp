#include "mainwindow.h"

MainWindow::MainWindow() 
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
    initializeEmit();
    print("[Nebula] Welcome to Nebula!");
    setWindowTitle(tr("Nebula[*]"));

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
//    setFileWorker->setQSpaceInfo(&suggested_search_radius_low, &suggested_search_radius_high, &suggested_q);
    setFileWorker->setOpenCLContext(context_cl);

    setFileWorker->moveToThread(setFileThread);
    connect(setFileButton, SIGNAL(clicked()), this, SLOT(setFilesFromSelectionModel()), Qt::DirectConnection);
    connect(setFileThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
    connect(setFileWorker, SIGNAL(finished()), this, SLOT(setFileButtonFinish()));
    connect(setFileThread, SIGNAL(started()), setFileWorker, SLOT(process()));
    connect(setFileWorker, SIGNAL(abort()), setFileThread, SLOT(quit()));
    connect(setFileWorker, SIGNAL(finished()), setFileThread, SLOT(quit()));
    connect(setFileWorker, SIGNAL(changedFile(QString)), this, SLOT(updateFileHeader(QString)));
    connect(setFileWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(setFileWorker, SIGNAL(changedGenericProgress(int)), progressBar, SLOT(setValue(int)));
    connect(setFileWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
//    connect(setFileWorker, SIGNAL(changedTabWidget(int)), tabWidget, SLOT(setCurrentIndex(int)));
    connect(setFileButton, SIGNAL(clicked()), setFileThread, SLOT(start()));
    connect(killButton, SIGNAL(clicked()), setFileWorker, SLOT(killProcess()), Qt::DirectConnection);


    //### readFileWorker ###
    readFileWorker = new ReadFileWorker();
    readFileWorker->setFilePaths(&file_paths);
    readFileWorker->setFiles(&files);

    readFileWorker->moveToThread(readFileThread);
    connect(readFileThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
    connect(readFileWorker, SIGNAL(finished()), this, SLOT(readFileButtonFinish()));
    connect(readFileThread, SIGNAL(started()), readFileWorker, SLOT(process()));
    connect(readFileWorker, SIGNAL(abort()), readFileThread, SLOT(quit()));
    connect(readFileWorker, SIGNAL(finished()), readFileThread, SLOT(quit()));
    connect(readFileWorker, SIGNAL(changedFile(QString)), this, SLOT(updateFileHeader(QString)));
    connect(readFileWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(readFileWorker, SIGNAL(changedGenericProgress(int)), progressBar, SLOT(setValue(int)));
    connect(readFileWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(readFileWorker, SIGNAL(changedTabWidget(int)), tabWidget, SLOT(setCurrentIndex(int)));
    connect(readFileButton, SIGNAL(clicked()), readFileThread, SLOT(start()));
    connect(killButton, SIGNAL(clicked()), readFileWorker, SLOT(killProcess()), Qt::DirectConnection);


    //### projectFileWorker ###
    projectFileWorker = new ProjectFileWorker();
    projectFileWorker->setOpenCLContext(context_cl);
    projectFileWorker->setFilePaths(&file_paths);
    projectFileWorker->setFiles(&files);
    projectFileWorker->setReducedPixels(&reduced_pixels);
    projectFileWorker->initializeCLKernel();
    connect(this->activeAngleComboBox, SIGNAL(currentIndexChanged(int)), projectFileWorker, SLOT(setActiveAngle(int)), Qt::QueuedConnection);
    connect(this->omegaCorrectionSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setOffsetOmega(double)), Qt::QueuedConnection);
    connect(this->kappaCorrectionSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setOffsetKappa(double)), Qt::QueuedConnection);
    connect(this->phiCorrectionSpinBox, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setOffsetPhi(double)), Qt::QueuedConnection);
    connect(this->reduceThresholdLow, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setReduceThresholdLow(double)), Qt::QueuedConnection);
    connect(this->reduceThresholdHigh, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setReduceThresholdHigh(double)), Qt::QueuedConnection);
    connect(this->projectThresholdLow, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setProjectThresholdLow(double)), Qt::QueuedConnection);
    connect(this->projectThresholdHigh, SIGNAL(valueChanged(double)), projectFileWorker, SLOT(setProjectThresholdHigh(double)), Qt::QueuedConnection);

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
//    allInOneWorker->setQSpaceInfo(&suggested_search_radius_low, &suggested_search_radius_high, &suggested_q);
    allInOneWorker->setOpenCLContext(context_cl);
    allInOneWorker->setReducedPixels(&reduced_pixels);
    allInOneWorker->initializeCLKernel();
    connect(allInOneButton, SIGNAL(clicked()), this, SLOT(setFilesFromSelectionModel()), Qt::DirectConnection);
    connect(this->activeAngleComboBox, SIGNAL(currentIndexChanged(int)), allInOneWorker, SLOT(setActiveAngle(int)), Qt::QueuedConnection);
    connect(this->omegaCorrectionSpinBox, SIGNAL(valueChanged(double)), allInOneWorker, SLOT(setOffsetOmega(double)), Qt::QueuedConnection);
    connect(this->kappaCorrectionSpinBox, SIGNAL(valueChanged(double)), allInOneWorker, SLOT(setOffsetKappa(double)), Qt::QueuedConnection);
    connect(this->phiCorrectionSpinBox, SIGNAL(valueChanged(double)), allInOneWorker, SLOT(setOffsetPhi(double)), Qt::QueuedConnection);
    connect(this->reduceThresholdLow, SIGNAL(valueChanged(double)), allInOneWorker, SLOT(setReduceThresholdLow(double)), Qt::QueuedConnection);
    connect(this->reduceThresholdHigh, SIGNAL(valueChanged(double)), allInOneWorker, SLOT(setReduceThresholdHigh(double)), Qt::QueuedConnection);
    connect(this->projectThresholdLow, SIGNAL(valueChanged(double)), allInOneWorker, SLOT(setProjectThresholdLow(double)), Qt::QueuedConnection);
    connect(this->projectThresholdHigh, SIGNAL(valueChanged(double)), allInOneWorker, SLOT(setProjectThresholdHigh(double)), Qt::QueuedConnection);

    allInOneWorker->moveToThread(allInOneThread);
    connect(allInOneThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
    connect(allInOneWorker, SIGNAL(finished()), this, SLOT(allInOneButtonFinish()));
    connect(allInOneThread, SIGNAL(started()), allInOneWorker, SLOT(process()));
    connect(allInOneWorker, SIGNAL(finished()), allInOneThread, SLOT(quit()));
    connect(allInOneWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(allInOneWorker, SIGNAL(changedGenericProgress(int)), progressBar, SLOT(setValue(int)));
    connect(allInOneWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(allInOneWorker, SIGNAL(changedFile(QString)), this, SLOT(updateFileHeader(QString)));
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
    connect(projectFileWorker, SIGNAL(qSpaceInfoChanged(float,float,float)), voxelizeWorker, SLOT(setQSpaceInfo(float,float,float)));
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

void MainWindow::incrementDisplayFile1()
{
    imageSpinBox->setValue(imageSpinBox->value()+1);
}
void MainWindow::incrementDisplayFile10()
{
    imageSpinBox->setValue(imageSpinBox->value()+10);
}
void MainWindow::decrementDisplayFile1()
{
    imageSpinBox->setValue(imageSpinBox->value()-1);
}
void MainWindow::decrementDisplayFile10()
{
    imageSpinBox->setValue(imageSpinBox->value()-10);
}
void MainWindow::setDisplayFile(int value)
{
    if ((value >= 0) && (value < file_paths.size()))
    {
        emit imagePreviewChanged(file_paths[value]);
        emit updateFileHeader(file_paths[value]);
        imageLabel->setText(file_paths[value]);
    }
    else
    {
        print("\n[Nebula] File does not exist");
    }
}

void MainWindow::updateFileHeader(QString path)
{
    DetectorFile file(path, NULL);
    fileHeaderEdit->setPlainText(file.getHeaderText());
}

void MainWindow::runProjectFileThread()
{
    tabWidget->setCurrentIndex(1);
    projectFileThread->start();
}

void MainWindow::runAllInOneThread()
{
    tabWidget->setCurrentIndex(1);
    allInOneThread->start();
}

void MainWindow::setFilesFromSelectionModel()
{
    if (!scriptingAct->isChecked()) file_paths = fileSelectionModel->getFiles();
}

void MainWindow::initializeEmit()
{
    tabWidget->setCurrentIndex(0);
    svoLevelSpinBox->setValue(11);

    reduceThresholdLow->setValue(10);
    reduceThresholdHigh->setValue(1e9);
    projectThresholdLow->setValue(10);
    projectThresholdHigh->setValue(1e9);

    dataMinSpinBox->setValue(1);
    dataMaxSpinBox->setValue(1000);
    alphaSpinBox->setValue(1.0);
    brightnessSpinBox->setValue(2.0);

    funcParamASpinBox->setValue(13.5);
    funcParamBSpinBox->setValue(10.5);
    funcParamCSpinBox->setValue(10.0);
    funcParamDSpinBox->setValue(0.005);

    qualitySlider->setValue(100);
    
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
    alignSliceToLabAct = new QAction(QIcon(":/art/placeholder.png"), tr("Align slice frame to lab frame"), this);
    
    rotateRightAct = new QAction(QIcon(":/art/rotate_right.png"), tr("Rotate right"), this);
    rotateLeftAct = new QAction(QIcon(":/art/rotate_left.png"), tr("Rotate left"), this);
    rotateUpAct = new QAction(QIcon(":/art/rotate_up.png"), tr("Rotate up"), this);
    rotateDownAct = new QAction(QIcon(":/art/rotate_down.png"), tr("Rotate down"), this);
    rollCW = new QAction(QIcon(":/art/roll_cw.png"), tr("Roll clockwise"), this);
    rollCCW = new QAction(QIcon(":/art/roll_ccw.png"), tr("Roll counterclockwise"), this);
    
    
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

void MainWindow::omitFile()
{
    int value = imageSpinBox->value();

    emit omitFile(file_paths[value]);

    if (value < file_paths.size())
    {
        file_paths.removeAt(value);



        // Feature/Bug: Should also un-select the file in question in the file browser, or use a "load files" explicit button. Also, the file_paths array is superfluous
    }

    if (value < files.size())
    {
        files.removeAt(value);

        print("\nRemoved file "+QString::number(value)+"of"+QString::number(files.size()));
    }

    if (value >= file_paths.size()) value = file_paths.size() - 1;
    emit imagePreviewChanged(file_paths[value]);
    emit updateFileHeader(file_paths[value]);
    imageLabel->setText(file_paths[value]);
}

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
        U = UB * B.getInverse();
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
    
    if (tab==1) file_paths = fileSelectionModel->getFiles();


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
    connect(this->tsfComboBox, SIGNAL(currentIndexChanged(int)), volumeRenderWindow->getWorker(), SLOT(setTsfColor(int)));
    connect(this->tsfAlphaComboBox, SIGNAL(currentIndexChanged(int)), volumeRenderWindow->getWorker(), SLOT(setTsfAlpha(int)));
    connect(this->dataMinSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setDataMin(double)));
    connect(this->dataMaxSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setDataMax(double)));
    connect(this->alphaSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setAlpha(double)));
    connect(this->brightnessSpinBox, SIGNAL(valueChanged(double)), volumeRenderWindow->getWorker(), SLOT(setBrightness(double)));
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

        alphaSpinBox->setValue(0.05);
        brightnessSpinBox->setValue(2.0);
        dataMinSpinBox->setValue(svo_loaded.getMinMax()->at(0));
        dataMaxSpinBox->setValue(svo_loaded.getMinMax()->at(1));
        
        UBMatrix<double> UB;
        
        UB = svo_loaded.getUB();
        
        if (UB.size() == 3*3)
        {
            volumeRenderWindow->getWorker()->setUBMatrix(UB);
        
            UB.print(2,"UB loaded");
        
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

        connect(fileSelectionTree, SIGNAL(fileChanged(QString)), this, SLOT(updateFileHeader(QString)));
        connect(this, SIGNAL(omitFile(QString)), fileSelectionModel, SLOT(removeFile(QString)));

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

        setFilesWidget->setLayout(scriptLayout);
    }

    /*      3D View widget      */
    {
        QSurfaceFormat format_gl;
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
        volumeRenderWindow->setAnimating(true);
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
        viewToolBar->addAction(sliceAct);
        
        viewToolBar->addAction(shadowAct);
        viewToolBar->addAction(dataStructureAct);
        
        viewToolBar->addAction(integrate3DAct);
        viewToolBar->addAction(log3DAct);
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
        imagePreviewWindow->setAnimating(true);
        imagePreviewWindow->initializeWorker();
        
        imageDisplayWidget = QWidget::createWindowContainer(imagePreviewWindow);
        imageDisplayWidget->setFocusPolicy(Qt::TabFocus);
        
        connect(this, SIGNAL(imagePreviewChanged(QString)), imagePreviewWindow->getWorker(), SLOT(setImageFromPath(QString)));

        imageWidget = new QWidget;

        imageLabel = new QLabel("---");
        
        imageFastBackButton = new QPushButton;
        imageFastBackButton->setIcon(QIcon(":art/fast_back.png"));
        imageFastBackButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        connect(imageFastBackButton, SIGNAL(clicked()), this, SLOT(decrementDisplayFile10()));

        imageSlowBackButton = new QPushButton;
        imageSlowBackButton->setIcon(QIcon(":art/back.png"));
        imageSlowBackButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        connect(imageSlowBackButton, SIGNAL(clicked()), this, SLOT(decrementDisplayFile1()));

        imageSpinBox = new QSpinBox;
        imageSpinBox->setRange(0,100000);
        
        connect(imageSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setDisplayFile(int)));

        imageFastForwardButton = new QPushButton;
        imageFastForwardButton->setIcon(QIcon(":art/fast_forward.png"));
        imageFastForwardButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        connect(imageFastForwardButton, SIGNAL(clicked()), this, SLOT(incrementDisplayFile10()));

        imageSlowForwardButton = new QPushButton;
        imageSlowForwardButton->setIcon(QIcon(":art/forward.png"));
        imageSlowForwardButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        connect(imageSlowForwardButton, SIGNAL(clicked()), this, SLOT(incrementDisplayFile1()));

        
        omitFrameButton = new QPushButton("Remove");
        omitFrameButton->setIcon(QIcon(":art/kill.png"));
        omitFrameButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        connect(omitFrameButton, SIGNAL(clicked()), this, SLOT(omitFile()));


        imageModeCB = new QComboBox;
        imageModeCB->addItem("Raw");
        imageModeCB->addItem("Corrected");
        connect(imageModeCB, SIGNAL(currentIndexChanged(int)), imagePreviewWindow->getWorker(), SLOT(setMode(int)));
                
        QGridLayout * imageLayout = new QGridLayout;
        imageLayout->setRowStretch(1,1);
        imageLayout->addWidget(imageLabel,0,0,1,8);
        imageLayout->addWidget(imageDisplayWidget,1,0,1,8);
        imageLayout->addWidget(imageFastBackButton,2,0,1,2);
        imageLayout->addWidget(imageSlowBackButton,2,2,1,1);
        imageLayout->addWidget(imageSpinBox,2,3,1,2);
        imageLayout->addWidget(imageSlowForwardButton,2,5,1,1);
        imageLayout->addWidget(imageFastForwardButton,2,6,1,2);
        imageLayout->addWidget(imageModeCB,3,0,1,4);
        imageLayout->addWidget(omitFrameButton,3,4,1,4);
        
        imageWidget->setLayout(imageLayout);
    }
    
    /* Graphics dock widget */
    {
        QLabel * label_texture= new QLabel(QString("Texture "));
        QLabel * label_data_min= new QLabel(QString("Min: "));
        QLabel * label_data_max= new QLabel(QString("Max: "));
        QLabel * label_alpha= new QLabel(QString("Alpha: "));
        QLabel * label_brightness = new QLabel(QString("Brightness: "));
        QLabel * label_quality = new QLabel(QString("Performance: "));

        dataMinSpinBox = new QDoubleSpinBox;
        dataMinSpinBox->setDecimals(2);
        dataMinSpinBox->setRange(0, 1e9);
        dataMinSpinBox->setSingleStep(1);
        dataMinSpinBox->setAccelerated(1);

        dataMaxSpinBox = new QDoubleSpinBox;
        dataMaxSpinBox->setDecimals(1);
        dataMaxSpinBox->setRange(0, 1e9);
        dataMaxSpinBox->setSingleStep(1);
        dataMaxSpinBox->setAccelerated(1);

        alphaSpinBox = new QDoubleSpinBox;
        alphaSpinBox->setDecimals(4);
        alphaSpinBox->setRange(0, 10);
        alphaSpinBox->setSingleStep(0.1);
        alphaSpinBox->setAccelerated(1);

        brightnessSpinBox = new QDoubleSpinBox;
        brightnessSpinBox->setDecimals(4);
        brightnessSpinBox->setRange(0, 10);
        brightnessSpinBox->setSingleStep(0.1);
        brightnessSpinBox->setAccelerated(1);

        tsfComboBox = new QComboBox;
        tsfComboBox->addItem(trUtf8("Rainbow"));
        tsfComboBox->addItem(trUtf8("Hot"));
        tsfComboBox->addItem(trUtf8("Hsv"));
        tsfComboBox->addItem(trUtf8("Galaxy"));
        tsfComboBox->addItem(trUtf8("Binary"));
        tsfComboBox->addItem(trUtf8("Yranib"));

        tsfAlphaComboBox = new QComboBox;
        tsfAlphaComboBox->addItem(trUtf8("Linear"));
        tsfAlphaComboBox->addItem(trUtf8("Exponential"));
        tsfAlphaComboBox->addItem(trUtf8("Uniform"));

        qualitySlider = new QSlider(Qt::Horizontal);
        qualitySlider->setRange(1,100);
        qualitySlider->setToolTip("Set quality versus performance");
        qualitySlider->setTickPosition(QSlider::NoTicks);

        graphicsDockWidget = new QDockWidget(tr("View Settings"), this);
        graphicsDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        graphicsWidget = new QWidget;

        QGridLayout * graphicsLayout = new QGridLayout;

        graphicsLayout->addWidget(label_texture,0,0,1,2);
        graphicsLayout->addWidget(tsfComboBox,0,2,1,1);
        graphicsLayout->addWidget(tsfAlphaComboBox,0,3,1,1);
        graphicsLayout->addWidget(label_data_min,1,0,1,2);
        graphicsLayout->addWidget(dataMinSpinBox,1,2,1,2);
        graphicsLayout->addWidget(label_data_max,2,0,1,2);
        graphicsLayout->addWidget(dataMaxSpinBox,2,2,1,2);
        graphicsLayout->addWidget(label_alpha,3,0,1,2);
        graphicsLayout->addWidget(alphaSpinBox,3,2,1,2);
        graphicsLayout->addWidget(label_brightness,4,0,1,2);
        graphicsLayout->addWidget(brightnessSpinBox,4,2,1,2);
        graphicsLayout->addWidget(label_quality,5,0,1,2);
        graphicsLayout->addWidget(qualitySlider,5,2,1,2);

        graphicsWidget->setLayout(graphicsLayout);
        graphicsDockWidget->setFixedSize(graphicsWidget->minimumSizeHint());
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
        bNormSpinBox = new QDoubleSpinBox;
        cNormSpinBox = new QDoubleSpinBox;
    
        alphaNormSpinBox = new QDoubleSpinBox;
        alphaNormSpinBox->setRange(0,180);
        betaNormSpinBox = new QDoubleSpinBox;
        betaNormSpinBox->setRange(0,180);
        gammaNormSpinBox = new QDoubleSpinBox;
        gammaNormSpinBox->setRange(0,180);
        
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
        kSpinBox = new QSpinBox;
        kSpinBox->setRange(-1e3,1e3);
        lSpinBox = new QSpinBox;
        lSpinBox->setRange(-1e3,1e3);
        
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
        
        QLabel * aLabel = new QLabel("<i>a<i>");
        QLabel * bLabel = new QLabel("<i>b<i>");
        QLabel * cLabel = new QLabel("<i>c<i>");
        QLabel * alphaLabel = new QLabel(trUtf8("<i>α<i>"));
        QLabel * betaLabel = new QLabel(trUtf8( "<i>β<i>"));
        QLabel * gammaLabel = new QLabel(trUtf8( "<i>γ<i>"));

        QLabel * hLabel = new QLabel("<i>h<i>");
        QLabel * kLabel = new QLabel("<i>k<i>");
        QLabel * lLabel = new QLabel("<i>l<i>");
        
        QGridLayout * unitCellLayout = new QGridLayout; 
        
        unitCellLayout->addWidget(aLabel,0,0,1,1);
        unitCellLayout->addWidget(aNormSpinBox,0,1,1,1);
        unitCellLayout->addWidget(bLabel,0,2,1,1);
        unitCellLayout->addWidget(bNormSpinBox,0,3,1,1);
        unitCellLayout->addWidget(cLabel,0,4,1,1);
        unitCellLayout->addWidget(cNormSpinBox,0,5,1,1);
        
        unitCellLayout->addWidget(alphaLabel,1,0,1,1);
        unitCellLayout->addWidget(alphaNormSpinBox,1,1,1,1);
        unitCellLayout->addWidget(betaLabel,1,2,1,1);
        unitCellLayout->addWidget(betaNormSpinBox,1,3,1,1);
        unitCellLayout->addWidget(gammaLabel,1,4,1,1);
        unitCellLayout->addWidget(gammaNormSpinBox,1,5,1,1);
        
        unitCellLayout->addWidget(hLabel,4,0,1,1);
        unitCellLayout->addWidget(hSpinBox,4,1,1,1);
        unitCellLayout->addWidget(kLabel,4,2,1,1);
        unitCellLayout->addWidget(kSpinBox,4,3,1,1);
        unitCellLayout->addWidget(lLabel,4,4,1,1);
        unitCellLayout->addWidget(lSpinBox,4,5,1,1);
        
        unitCellLayout->addWidget(alignAlongAStarButton,5,0,1,2);
        unitCellLayout->addWidget(alignAlongBStarButton,5,2,1,2);
        unitCellLayout->addWidget(alignAlongCStarButton,5,4,1,2);
        
        unitCellLayout->addWidget(helpCellOverlayButton,6,0,1,3);
        unitCellLayout->addWidget(rotateCellButton,6,3,1,3);
        
        unitCellLayout->addWidget(toggleCellButton,7,0,1,6);
        
        unitCellWidget->setLayout(unitCellLayout);
        
        unitCellDock->setWidget(unitCellWidget);
        
        unitCellDock->setFixedSize(unitCellWidget->minimumSizeHint());
        
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
    }
    
    /* File Controls Widget */
    {
        fileControlsWidget = new QWidget;

        // Labels
        QLabel * labelA = new QLabel(QString("Detector file format:"));
        QLabel * labelB = new QLabel(QString("Pre correction threshold:"));
        QLabel * labelC = new QLabel(QString("Post correction threshold:"));
        QLabel * labelD = new QLabel(QString("Octtree levels: "));
        QLabel * labelE = new QLabel(QString("Active angle:"));
        QLabel * labelF = new QLabel("<i>ω</i>:");
        QLabel * labelG = new QLabel("<i>κ</i>:");
        QLabel * labelH = new QLabel("<i>φ</i>:");
        QLabel * labelI = new QLabel("Correction:");
        
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
        reduceThresholdLow = new QDoubleSpinBox;
        reduceThresholdLow->setRange(0, 1e9);
        reduceThresholdLow->setSingleStep(1);
        reduceThresholdLow->setAccelerated(1);
        reduceThresholdLow->setDecimals(2);
        reduceThresholdLow->setFocusPolicy(Qt::ClickFocus);

        reduceThresholdHigh = new QDoubleSpinBox;
        reduceThresholdHigh->setRange(0, 1e9);
        reduceThresholdHigh->setSingleStep(1);
        reduceThresholdHigh->setAccelerated(1);
        reduceThresholdHigh->setDecimals(2);
        reduceThresholdHigh->setFocusPolicy(Qt::ClickFocus);

        projectThresholdLow = new QDoubleSpinBox;
        projectThresholdLow->setRange(0, 1e9);
        projectThresholdLow->setSingleStep(1);
        projectThresholdLow->setAccelerated(1);
        projectThresholdLow->setDecimals(2);
        projectThresholdLow->setFocusPolicy(Qt::ClickFocus);

        projectThresholdHigh = new QDoubleSpinBox;
        projectThresholdHigh->setRange(0, 1e9);
        projectThresholdHigh->setSingleStep(1);
        projectThresholdHigh->setAccelerated(1);
        projectThresholdHigh->setDecimals(2);
        projectThresholdHigh->setFocusPolicy(Qt::ClickFocus);
        
        connect(this->reduceThresholdLow, SIGNAL(valueChanged(double)), imagePreviewWindow->getWorker(), SLOT(setThresholdAlow(double)),Qt::QueuedConnection);
        connect(this->reduceThresholdHigh, SIGNAL(valueChanged(double)), imagePreviewWindow->getWorker(), SLOT(setThresholdAhigh(double)),Qt::QueuedConnection);
        connect(this->projectThresholdLow, SIGNAL(valueChanged(double)), imagePreviewWindow->getWorker(), SLOT(setThresholdBlow(double)),Qt::QueuedConnection);
        connect(this->projectThresholdHigh, SIGNAL(valueChanged(double)), imagePreviewWindow->getWorker(), SLOT(setThresholdBhigh(double)),Qt::QueuedConnection);
        
        omegaCorrectionSpinBox = new QDoubleSpinBox;
        omegaCorrectionSpinBox->setRange(-180, 180);
        omegaCorrectionSpinBox->setDecimals(3);
        
        kappaCorrectionSpinBox = new QDoubleSpinBox;
        kappaCorrectionSpinBox->setRange(-180, 180);
        kappaCorrectionSpinBox->setDecimals(3);
        
        phiCorrectionSpinBox = new QDoubleSpinBox;
        phiCorrectionSpinBox->setRange(-180, 180);
        phiCorrectionSpinBox->setDecimals(3);
        
        svoLevelSpinBox = new QSpinBox;
        svoLevelSpinBox->setRange(1, 15);
        
        
        
        

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
        reconstructLayout->addWidget(labelB,2,0,1,4);
        reconstructLayout->addWidget(reduceThresholdLow,2,4,1,2);
        reconstructLayout->addWidget(reduceThresholdHigh,2,6,1,2);
        reconstructLayout->addWidget(labelC,3,0,1,4);
        reconstructLayout->addWidget(projectThresholdLow,3,4,1,2);
        reconstructLayout->addWidget(projectThresholdHigh,3,6,1,2);
        reconstructLayout->addWidget(labelD,4,0,1,4);
        reconstructLayout->addWidget(svoLevelSpinBox,4,4,1,4);
        reconstructLayout->addWidget(labelI,5,0,1,2);
        reconstructLayout->addWidget(labelF,5,2,1,1);
        reconstructLayout->addWidget(omegaCorrectionSpinBox,5,3,1,1);
        reconstructLayout->addWidget(labelG,5,4,1,1);
        reconstructLayout->addWidget(kappaCorrectionSpinBox,5,5,1,1);
        reconstructLayout->addWidget(labelH,5,6,1,1);
        reconstructLayout->addWidget(phiCorrectionSpinBox,5,7,1,1);
        reconstructLayout->addWidget(voxelizeButton,8,0,1,8);
        reconstructLayout->addWidget(saveSvoButton,9,0,1,8);
        fileControlsWidget->setLayout(reconstructLayout);
        fileDockWidget = new QDockWidget(tr("Data Reduction Settings"), this);
        fileDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        fileDockWidget->setWidget(fileControlsWidget);
        fileDockWidget->setFixedSize(fileControlsWidget->minimumSizeHint());
        viewMenu->addAction(fileDockWidget->toggleViewAction());
        this->addDockWidget(Qt::BottomDockWidgetArea, fileDockWidget);
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
        QLabel * p0= new QLabel(QString("Variable 1: "));
        QLabel * p1= new QLabel(QString("Variable 2: "));
        QLabel * p2= new QLabel(QString("Variable 3: "));
        QLabel * p3= new QLabel(QString("Variable 4: "));

        functionToggleButton = new QPushButton(tr("Toggle"));
        funcParamASpinBox = new QDoubleSpinBox;
        funcParamASpinBox->setDecimals(3);
        funcParamASpinBox->setRange(0, 100);
        funcParamASpinBox->setSingleStep(0.01);
        funcParamASpinBox->setAccelerated(1);

        funcParamBSpinBox = new QDoubleSpinBox;
        funcParamBSpinBox->setDecimals(3);
        funcParamBSpinBox->setRange(0, 100);
        funcParamBSpinBox->setSingleStep(0.01);
        funcParamBSpinBox->setAccelerated(1);

        funcParamCSpinBox = new QDoubleSpinBox;
        funcParamCSpinBox->setDecimals(3);
        funcParamCSpinBox->setRange(0, 100);
        funcParamCSpinBox->setSingleStep(0.01);
        funcParamCSpinBox->setAccelerated(1);

        funcParamDSpinBox = new QDoubleSpinBox;
        funcParamDSpinBox->setDecimals(3);
        funcParamDSpinBox->setRange(0, 100);
        funcParamDSpinBox->setSingleStep(0.01);
        funcParamDSpinBox->setAccelerated(1);

        functionDockWidget = new QDockWidget(tr("Model Settings"), this);
        functionWidget = new QWidget;

        QGridLayout * functionLayout = new QGridLayout;
        functionLayout->addWidget(p0,1,0,1,2);
        functionLayout->addWidget(funcParamASpinBox,1,2,1,2);
        functionLayout->addWidget(p1,2,0,1,2);
        functionLayout->addWidget(funcParamBSpinBox,2,2,1,2);
        functionLayout->addWidget(p2,3,0,1,2);
        functionLayout->addWidget(funcParamCSpinBox,3,2,1,2);
        functionLayout->addWidget(p3,4,0,1,2);
        functionLayout->addWidget(funcParamDSpinBox,4,2,1,2);
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

