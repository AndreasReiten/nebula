#include "mainwindow.h"

MainWindow::MainWindow() :
    isInScriptMode(true)
{
    //     Set default values
    current_svo = 0;
    display_file = 0;
    svo_loaded.append(SparseVoxelOcttree());

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
//    format_gl.setVersion(4, 3);
    format_gl.setSamples(16);
    format_gl.setRedBufferSize(8);
    format_gl.setGreenBufferSize(8);
    format_gl.setBlueBufferSize(8);
    format_gl.setAlphaBufferSize(8);
//    format_gl.setSwapBehavior(QSurfaceFormat::DoubleBuffer);


    sharedContextWindow = new SharedContextWindow();
    sharedContextWindow->setFormat(format_gl);
    sharedContextWindow->setOpenCLContext(context_cl);
    sharedContextWindow->resize(2, 2);
    sharedContextWindow->show();
    sharedContextWindow->preInitialize();
    sharedContextWindow->hide();

    this->initializeActions();
    this->initializeMenus();
    this->initializeInteractives();
    this->initializeConnects();
    this->initializeThreads();

    setCentralWidget(mainWidget);
    readSettings();
    setCurrentFile("");
    initializeEmit();
    print("[Nebula] Welcome to Nebula alpha!");
    setWindowTitle(tr("Nebula[*]"));

    graphicsDockWidget->hide();
    unitcellDockWidget->hide();
    functionDockWidget->hide();
    fileDockWidget->hide();
    toolChainWidget->show();
    outputDockWidget->show();

}

MainWindow::~MainWindow()
{
//
}

void MainWindow::setCurrentSvoLevel(int value)
{
    svo_inprocess.setLevels(value);
}

void MainWindow::initializeThreads()
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
    setFileWorker->setQSpaceInfo(&suggested_search_radius_low, &suggested_search_radius_high, &suggested_q);
    setFileWorker->setOpenCLContext(context_cl);
    setFileWorker->setOpenCLBuffers(imageRenderWorker->getAlphaImgCLGL(), imageRenderWorker->getBetaImgCLGL(), imageRenderWorker->getGammaImgCLGL(), imageRenderWorker->getTsfImgCLGL());

    setFileWorker->moveToThread(setFileThread);
    connect(setFileThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
    connect(setFileWorker, SIGNAL(finished()), this, SLOT(setFileButtonFinish()));
    connect(setFileThread, SIGNAL(started()), setFileWorker, SLOT(process()));
    connect(setFileWorker, SIGNAL(abort()), setFileThread, SLOT(quit()));
    connect(setFileWorker, SIGNAL(finished()), setFileThread, SLOT(quit()));
    connect(setFileWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(setFileWorker, SIGNAL(changedGenericProgress(int)), progressBar, SLOT(setValue(int)));
    connect(setFileWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(setFileWorker, SIGNAL(changedTabWidget(int)), tabWidget, SLOT(setCurrentIndex(int)));
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
    projectFileWorker->setReduceThresholdLow(&threshold_reduce_low);
    projectFileWorker->setReduceThresholdHigh(&threshold_reduce_high);
    projectFileWorker->setProjectThresholdLow(&threshold_project_low);
    projectFileWorker->setProjectThresholdHigh(&threshold_project_high);

    projectFileWorker->moveToThread(projectFileThread);
    connect(projectFileThread, SIGNAL(started()), imageRenderWindow, SLOT(stopAnimating()));
    connect(projectFileThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
    connect(projectFileWorker, SIGNAL(updateRequest()), imageRenderWindow, SLOT(renderNow()), Qt::BlockingQueuedConnection);
    connect(projectFileWorker, SIGNAL(finished()), this, SLOT(projectFileButtonFinish()));
    connect(projectFileThread, SIGNAL(started()), projectFileWorker, SLOT(process()));
    connect(projectFileWorker, SIGNAL(finished()), projectFileThread, SLOT(quit()));
    connect(projectFileWorker, SIGNAL(finished()), imageRenderWindow, SLOT(startAnimating()));
    connect(projectFileWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(projectFileWorker, SIGNAL(changedGenericProgress(int)), progressBar, SLOT(setValue(int)));
    connect(projectFileWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(projectFileWorker, SIGNAL(changedTabWidget(int)), tabWidget, SLOT(setCurrentIndex(int)));
    connect(projectFileWorker, SIGNAL(changedImageSize(int,int)), imageRenderWorker, SLOT(setImageSize(int,int)), Qt::BlockingQueuedConnection);
    connect(projectFileButton, SIGNAL(clicked()), this, SLOT(runProjectFileThread()));
    connect(killButton, SIGNAL(clicked()), projectFileWorker, SLOT(killProcess()), Qt::DirectConnection);
//    connect(projectFileWorker, SIGNAL(aquireSharedBuffers()), imageRenderWorker, SLOT(aquireSharedBuffers()), Qt::BlockingQueuedConnection);
//    connect(projectFileWorker, SIGNAL(releaseSharedBuffers()), imageRenderWorker, SLOT(releaseSharedBuffers()), Qt::BlockingQueuedConnection);

    //### allInOneWorker ###
    allInOneWorker = new AllInOneWorker();
    allInOneWorker->setFilePaths(&file_paths);
    allInOneWorker->setSVOFile(&svo_inprocess);
    allInOneWorker->setQSpaceInfo(&suggested_search_radius_low, &suggested_search_radius_high, &suggested_q);
    allInOneWorker->setOpenCLContext(context_cl);
    allInOneWorker->setOpenCLBuffers(imageRenderWorker->getAlphaImgCLGL(), imageRenderWorker->getBetaImgCLGL(), imageRenderWorker->getGammaImgCLGL(), imageRenderWorker->getTsfImgCLGL());
    allInOneWorker->setReducedPixels(&reduced_pixels);
    allInOneWorker->initializeCLKernel();
    allInOneWorker->setReduceThresholdLow(&threshold_reduce_low);
    allInOneWorker->setReduceThresholdHigh(&threshold_reduce_high);
    allInOneWorker->setProjectThresholdLow(&threshold_project_low);
    allInOneWorker->setProjectThresholdHigh(&threshold_project_high);

    allInOneWorker->moveToThread(allInOneThread);
    connect(allInOneThread, SIGNAL(started()), imageRenderWindow, SLOT(stopAnimating()));
    connect(allInOneThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
    connect(allInOneWorker, SIGNAL(updateRequest()), imageRenderWindow, SLOT(renderNow()), Qt::BlockingQueuedConnection);
    connect(allInOneWorker, SIGNAL(finished()), this, SLOT(allInOneButtonFinish()));
    connect(allInOneThread, SIGNAL(started()), allInOneWorker, SLOT(process()));
    connect(allInOneWorker, SIGNAL(finished()), allInOneThread, SLOT(quit()));
    connect(allInOneWorker, SIGNAL(finished()), imageRenderWindow, SLOT(startAnimating()));
    connect(allInOneWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(allInOneWorker, SIGNAL(changedGenericProgress(int)), progressBar, SLOT(setValue(int)));
    connect(allInOneWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(allInOneWorker, SIGNAL(changedTabWidget(int)), tabWidget, SLOT(setCurrentIndex(int)));
    connect(allInOneWorker, SIGNAL(changedImageSize(int,int)), imageRenderWorker, SLOT(setImageSize(int,int)), Qt::BlockingQueuedConnection);
    connect(allInOneButton, SIGNAL(clicked()), this, SLOT(runAllInOneThread()));
    connect(killButton, SIGNAL(clicked()), allInOneWorker, SLOT(killProcess()), Qt::DirectConnection);
//    connect(allInOneWorker, SIGNAL(aquireSharedBuffers()), imageRenderWorker, SLOT(aquireSharedBuffers()), Qt::BlockingQueuedConnection);
//    connect(allInOneWorker, SIGNAL(releaseSharedBuffers()), imageRenderWorker, SLOT(releaseSharedBuffers()), Qt::BlockingQueuedConnection);


    //### voxelizeWorker ###
    voxelizeWorker = new VoxelizeWorker();
    voxelizeWorker->setOpenCLContext(context_cl);
    voxelizeWorker->moveToThread(voxelizeThread);
    voxelizeWorker->setSVOFile(&svo_inprocess);
    voxelizeWorker->setQSpaceInfo(&suggested_search_radius_low, &suggested_search_radius_high, &suggested_q);
    voxelizeWorker->setReducedPixels(&reduced_pixels);
    voxelizeWorker->initializeCLKernel();

    voxelizeWorker->moveToThread(voxelizeThread);
    connect(voxelizeThread, SIGNAL(started()), this, SLOT(anyButtonStart()));
    connect(voxelizeWorker, SIGNAL(finished()), this, SLOT(voxelizeButtonFinish()));
    connect(svoLevelSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setCurrentSvoLevel(int)), Qt::DirectConnection);
    connect(voxelizeThread, SIGNAL(started()), voxelizeWorker, SLOT(process()));
    connect(voxelizeWorker, SIGNAL(finished()), voxelizeThread, SLOT(quit()));
    connect(voxelizeWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(voxelizeWorker, SIGNAL(changedGenericProgress(int)), progressBar, SLOT(setValue(int)));
    connect(voxelizeWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(voxelizeButton, SIGNAL(clicked()), voxelizeThread, SLOT(start()));
    connect(killButton, SIGNAL(clicked()), voxelizeWorker, SLOT(killProcess()), Qt::DirectConnection);


    //### displayFileWorker ###
    displayFileWorker = new DisplayFileWorker();
    displayFileWorker->setOpenCLContext(context_cl);
    displayFileWorker->setOpenCLBuffers(imageRenderWorker->getAlphaImgCLGL(), imageRenderWorker->getBetaImgCLGL(), imageRenderWorker->getGammaImgCLGL(), imageRenderWorker->getTsfImgCLGL());
    displayFileWorker->setFilePaths(&file_paths);
    displayFileWorker->setFiles(&files);
    displayFileWorker->initializeCLKernel();
    displayFileWorker->setReduceThresholdLow(&threshold_reduce_low);
    displayFileWorker->setReduceThresholdHigh(&threshold_reduce_high);
    displayFileWorker->setProjectThresholdLow(&threshold_project_low);
    displayFileWorker->setProjectThresholdHigh(&threshold_project_high);

    displayFileWorker->moveToThread(displayFileThread);
    connect(displayFileThread, SIGNAL(started()), displayFileWorker, SLOT(process()));
    connect(displayFileWorker, SIGNAL(finished()), displayFileThread, SLOT(quit()));
    connect(displayFileWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(displayFileWorker, SIGNAL(changedImageSize(int,int)), imageRenderWorker, SLOT(setImageSize(int,int)), Qt::BlockingQueuedConnection);
    connect(killButton, SIGNAL(clicked()), displayFileWorker, SLOT(killProcess()), Qt::DirectConnection);
//    connect(displayFileWorker, SIGNAL(aquireSharedBuffers()), imageRenderWorker, SLOT(aquireSharedBuffers()), Qt::BlockingQueuedConnection);
//    connect(displayFileWorker, SIGNAL(releaseSharedBuffers()), imageRenderWorker, SLOT(releaseSharedBuffers()), Qt::BlockingQueuedConnection);
    connect(this->imageForwardButton, SIGNAL(clicked()), this, SLOT(incrementDisplayFile1()));
    connect(this->imageFastForwardButton, SIGNAL(clicked()), this, SLOT(incrementDisplayFile10()));
    connect(this->imageBackButton, SIGNAL(clicked()), this, SLOT(decrementDisplayFile1()));
    connect(this->imageFastBackButton, SIGNAL(clicked()), this, SLOT(decrementDisplayFile10()));
    connect(this->imageNumberSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setDisplayFile(int)));
}

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
    int value = display_file + 1;
    runDisplayFileThread(value);
}
void MainWindow::incrementDisplayFile10()
{
    int value = display_file + 10;
    runDisplayFileThread(value);
}
void MainWindow::decrementDisplayFile1()
{
    int value = display_file - 1;
    runDisplayFileThread(value);
}
void MainWindow::decrementDisplayFile10()
{
    int value = display_file - 10;
    runDisplayFileThread(value);
}
void MainWindow::setDisplayFile(int value)
{
    runDisplayFileThread(value);
}

void MainWindow::runDisplayFileThread(int value)
{
    if (value < 0) value = 0;
    if (value >= file_paths.size()) value = file_paths.size() - 1;

    if ((file_paths.size() > 0 ) && (display_file != value))
    {
        display_file = value;
        imageNumberSpinBox->setValue(display_file);
        displayFileWorker->setDisplayFile(display_file);

        displayFileThread->start();
    }
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

void MainWindow::setReduceThresholdLow(double value)
{
    this->threshold_reduce_low = (float) value;
}
void MainWindow::setReduceThresholdHigh(double value)
{
    this->threshold_reduce_high = (float) value;
}
void MainWindow::setProjectThresholdLow(double value)
{
    this->threshold_project_low = (float) value;
}
void MainWindow::setProjectThresholdHigh(double value)
{
    this->threshold_project_high = (float) value;
}

void MainWindow::initializeEmit()
{
    tabWidget->setCurrentIndex(0);
    svoLevelSpinBox->setValue(9);

    treshLimA_DSB->setValue(10);
    treshLimB_DSB->setValue(1e9);
    treshLimC_DSB->setValue(10);
    treshLimD_DSB->setValue(1e9);

    dataMinSpinBox->setValue(10);
    dataMaxSpinBox->setValue(1000);
    alphaSpinBox->setValue(0.5);
    brightnessSpinBox->setValue(2.0);

    funcParamASpinBox->setValue(13.5);
    funcParamBSpinBox->setValue(10.5);
    funcParamCSpinBox->setValue(10.0);
    funcParamDSpinBox->setValue(0.005);

    qualitySlider->setValue(100);
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

        }
    }
}

void MainWindow::initializeActions()
{


    // Actions
    scriptingAct = new QAction(QIcon(":/art/fast_proceed.png"), tr("&Toggle scripting mode"), this);
    newAct = new QAction(QIcon(":/art/new.png"), tr("&New script"), this);
    openAct = new QAction(QIcon(":/art/open.png"), tr("&Open script"), this);
    saveAct = new QAction(QIcon(":/art/save.png"), tr("&Save script"), this);
    runScriptAct = new QAction(QIcon(":/art/forward.png"), tr("Run"), this);
    saveAsAct = new QAction(QIcon(":/art/save.png"), tr("Save script &As..."), this);
    exitAct = new QAction(tr("E&xit program"), this);
    aboutAct = new QAction(tr("&About Nebula"), this);
    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutOpenCLAct = new QAction(tr("About OpenCL"), this);
    aboutOpenGLAct = new QAction(tr("About OpenGL"), this);
    //~aboutHDF5Act = new QAction(tr("About HDF"), this);
    openSVOAct = new QAction(QIcon(":/art/open.png"), tr("Open SVO"), this);
    saveSVOAct = new QAction(QIcon(":/art/saveScript.png"), tr("Save SVO"), this);
    logAct =  new QAction(QIcon(":/art/log.png"), tr("Toggle Logarithm"), this);
    dataStructureAct = new QAction(QIcon(":/art/datastructure.png"), tr("Toggle Data Structure"), this);
    backgroundAct = new QAction(QIcon(":/art/background.png"), tr("Toggle Background Color"), this);
    projectionAct = new QAction(QIcon(":/art/projection.png"), tr("Toggle Projection"), this);
    screenshotAct = new QAction(QIcon(":/art/screenshot.png"), tr("&Take Screenshot"), this);
    scalebarAct = new QAction(QIcon(":/art/scalebar.png"), tr("&Toggle Scalebars"), this);

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
    //~aboutHDF5Act->setStatusTip(tr("About HDF"));

    // Shortcuts
    newAct->setShortcuts(QKeySequence::New);
    openAct->setShortcuts(QKeySequence::Open);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    exitAct->setShortcuts(QKeySequence::Quit);
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

////~void MainWindow::aboutHDF5()
////~{
//    //~QMessageBox::about(this, tr("About HDF"),
//        //~tr("<h1>About HDF</h1> <b>Hierarchical Data Format</b>  (HDF, HDF4, or HDF5) is the name of a set of file formats and libraries designed to store and organize large amounts of numerical data. Originally developed at the National Center for Supercomputing Applications, it is currently supported by the non-profit HDF Group, whose mission is to ensure continued development of HDF technologies, and the continued accessibility of data currently stored in HDF. <br> In keeping with this goal, the HDF format, libraries and associated tools are available under a liberal, BSD-like license for general use. <br> <a href=\"http://www.hdfgroup.org/\">www.hdfgroup.org/</a>"));
////~}

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
        MiniArray<float> abc(6);
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

//        volumeRenderWidget->setMatrixU(U.data());
//        volumeRenderWidget->setMatrixB(B.data());
    }
}



void MainWindow::setTab(int tab)
{
    switch (tab)
    {
        case 0:
            graphicsDockWidget->hide();
            unitcellDockWidget->hide();
            functionDockWidget->hide();
            fileDockWidget->hide();
            toolChainWidget->show();
            outputDockWidget->show();
            setWindowTitle(tr("Nebula[*] @ SCRIPT: ")+current_script_path);
            break;

        case 1:
            graphicsDockWidget->hide();
            unitcellDockWidget->hide();
            functionDockWidget->hide();
            toolChainWidget->show();
            fileDockWidget->show();
            outputDockWidget->show();
            setWindowTitle(tr("Nebula[*] @ SCRIPT:")+current_script_path);
            break;

        case 2:
            toolChainWidget->hide();
            fileDockWidget->hide();
            outputDockWidget->hide();
            graphicsDockWidget->show();
            unitcellDockWidget->show();
            functionDockWidget->show();
            setWindowTitle(tr("Nebula[*] @ SVO:")+current_svo_path);
            break;

        default:
            std::cout << "Reverting to Default Tab" << std::endl;
            break;
    }
}


void MainWindow::toggleScriptView()
{
    isInScriptMode = !isInScriptMode;

    if (isInScriptMode)
    {
        scriptTextEdit->show();
        readScriptButton->show();
        fileBrowserWidget->hide();
    }
    else
    {
        scriptTextEdit->hide();
        readScriptButton->hide();
        fileBrowserWidget->show();
    }
}

void MainWindow::initializeConnects()
{
    /* this <-> volumeRenderWidget */
    connect(this->qualitySlider, SIGNAL(valueChanged(int)), volumeRenderWorker, SLOT(setQuality(int)));
    connect(this->scalebarAct, SIGNAL(triggered()), volumeRenderWorker, SLOT(setScalebar()));
    connect(this->projectionAct, SIGNAL(triggered()), volumeRenderWorker, SLOT(setProjection()));
    connect(this->backgroundAct, SIGNAL(triggered()), volumeRenderWorker, SLOT(setBackground()));
    connect(this->logAct, SIGNAL(triggered()), volumeRenderWorker, SLOT(setLogarithmic()));
    connect(this->dataStructureAct, SIGNAL(triggered()), volumeRenderWorker, SLOT(setDataStructure()));
    connect(this->tsfComboBox, SIGNAL(currentIndexChanged(int)), volumeRenderWorker, SLOT(setTsfColor(int)));
    connect(this->tsfAlphaComboBox, SIGNAL(currentIndexChanged(int)), volumeRenderWorker, SLOT(setTsfAlpha(int)));
    connect(this->dataMinSpinBox, SIGNAL(valueChanged(double)), volumeRenderWorker, SLOT(setDataMin(double)));
    connect(this->dataMaxSpinBox, SIGNAL(valueChanged(double)), volumeRenderWorker, SLOT(setDataMax(double)));
    connect(this->alphaSpinBox, SIGNAL(valueChanged(double)), volumeRenderWorker, SLOT(setAlpha(double)));
    connect(this->brightnessSpinBox, SIGNAL(valueChanged(double)), volumeRenderWorker, SLOT(setBrightness(double)));
    connect(this->unitcellButton, SIGNAL(clicked()), volumeRenderWorker, SLOT(setUnitcell()));
    connect(this->functionToggleButton, SIGNAL(clicked()), volumeRenderWorker, SLOT(setModel()));
    connect(this->funcParamASpinBox, SIGNAL(valueChanged(double)), volumeRenderWorker, SLOT(setModelParam0(double)));
    connect(this->funcParamBSpinBox, SIGNAL(valueChanged(double)), volumeRenderWorker, SLOT(setModelParam1(double)));
    connect(this->funcParamCSpinBox, SIGNAL(valueChanged(double)), volumeRenderWorker, SLOT(setModelParam2(double)));
    connect(this->funcParamDSpinBox, SIGNAL(valueChanged(double)), volumeRenderWorker, SLOT(setModelParam3(double)));


    /* this <-> this */
    connect(this->scriptingAct, SIGNAL(triggered()), this, SLOT(toggleScriptView()));
    connect(this->screenshotAct, SIGNAL(triggered()), this, SLOT(takeScreenshot()));
    connect(this->treshLimA_DSB, SIGNAL(valueChanged(double)), this, SLOT(setReduceThresholdLow(double)));
    connect(this->treshLimB_DSB, SIGNAL(valueChanged(double)), this, SLOT(setReduceThresholdHigh(double)));
    connect(this->treshLimC_DSB, SIGNAL(valueChanged(double)), this, SLOT(setProjectThresholdLow(double)));
    connect(this->treshLimD_DSB, SIGNAL(valueChanged(double)), this, SLOT(setProjectThresholdHigh(double)));
    connect(scriptTextEdit->document(), SIGNAL(contentsChanged()), this, SLOT(documentWasModified()));
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(setTab(int)));
    connect(openSVOAct, SIGNAL(triggered()), this, SLOT(openSvo()));
    connect(saveSVOAct, SIGNAL(triggered()), this, SLOT(saveSvo()));
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
    connect(loadParButton, SIGNAL(clicked()), this, SLOT(openUnitcellFile()));
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
        dialog.setDefaultSuffix(".svo");
        QString file_name = dialog.getSaveFileName(this, tr("Save File"), "", tr(".svo (*.svo);; All Files (*)"));

        if (file_name != "")
        {
            svo_inprocess.save(file_name);
        }
    }
    else
    {

    }
}

void MainWindow::openSvo()
{
    current_svo_path = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr(".svo (*.svo);; All Files (*)"));

    if ((current_svo_path != ""))
    {
        svo_loaded[current_svo].open(current_svo_path);
        volumeRenderWorker->setSvo(&(svo_loaded[current_svo]));

        alphaSpinBox->setValue(0.05);
        brightnessSpinBox->setValue(2.0);
        dataMinSpinBox->setValue(svo_loaded[current_svo].getMinMax()->at(0));
        dataMaxSpinBox->setValue(svo_loaded[current_svo].getMinMax()->at(1));

        print("\n["+QString(this->metaObject()->className())+"] Loaded file: \""+current_svo_path+"\"");
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

        setFileButton = new QPushButton;
        setFileButton->setIcon(QIcon(":/art/proceed.png"));
        setFileButton->setIconSize(QSize(24,24));
        setFileButton->setText("Set ");
//        setFileButton->setEnabled(false);

        readFileButton = new QPushButton;
        readFileButton->setIcon(QIcon(":/art/proceed.png"));
        readFileButton->setIconSize(QSize(24,24));
        readFileButton->setText("Read ");
        readFileButton->setEnabled(false);

        projectFileButton = new QPushButton;
        projectFileButton->setIcon(QIcon(":/art/proceed.png"));
        projectFileButton->setIconSize(QSize(24,24));
        projectFileButton->setText("Correction and Projection ");
        projectFileButton->setEnabled(false);

        voxelizeButton = new QPushButton;
        voxelizeButton->setIcon(QIcon(":/art/proceed.png"));
        voxelizeButton->setText("Voxelize ");
        voxelizeButton->setIconSize(QSize(32,32));
        voxelizeButton->setEnabled(false);
        voxelizeButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        allInOneButton = new QPushButton;
        allInOneButton->setIcon(QIcon(":/art/fast_proceed.png"));
        allInOneButton->setText("All of Above (reduced memory consumption) ");
        allInOneButton->setIconSize(QSize(24,24));
//        allInOneButton->setEnabled(false);

        killButton = new QPushButton;
        killButton->setIcon(QIcon(":/art/kill.png"));
        killButton->setText("Kill ");
        killButton->setIconSize(QSize(24,24));
        killButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        toolChainWidget = new QWidget;
        QGridLayout * toolChainLayout = new QGridLayout;
        toolChainLayout->setSpacing(0);
        toolChainLayout->setMargin(0);
        toolChainLayout->setContentsMargins(0,0,0,0);
        toolChainLayout->setColumnStretch(1,1);
        toolChainLayout->setColumnStretch(2,1);
        toolChainLayout->setColumnStretch(3,1);
        toolChainLayout->setColumnStretch(4,1);
        toolChainLayout->addWidget(readScriptButton,0,0,2,1);
        toolChainLayout->addWidget(setFileButton,0,1,1,1);
        toolChainLayout->addWidget(readFileButton,0,2,1,1);
        toolChainLayout->addWidget(projectFileButton,0,3,1,1);
        toolChainLayout->addWidget(voxelizeButton,0,4,2,1);
        toolChainLayout->addWidget(killButton,0,5,2,1);
        toolChainLayout->addWidget(allInOneButton,1,1,1,3);
        toolChainWidget->setLayout(toolChainLayout);

        // Layout
        QGridLayout * topLayout = new QGridLayout;
        topLayout->setSpacing(0);
        topLayout->setMargin(0);
        topLayout->setContentsMargins(0,0,0,0);
        topLayout->addWidget(mainMenu,0,0,1,1);
        topLayout->addWidget(toolChainWidget,1,0,1,1);
        topWidget->setLayout(topLayout);
    }


    /*      Script Widget       */
    {
        setFilesWidget = new QWidget;

        // Script text edit
        scriptTextEdit = new QPlainTextEdit;
        script_highlighter = new Highlighter(scriptTextEdit->document());
        scriptHelp = "/* Add file paths using this Javascript window. \nDo this by appedning paths to the variable 'files'. */ \n\n files = ";
        scriptTextEdit->setPlainText(scriptHelp);

        // Toolbar
        scriptToolBar = new QToolBar(tr("Script"));
        scriptToolBar->addAction(newAct);
        scriptToolBar->addAction(openAct);
        scriptToolBar->addAction(saveAct);
        scriptToolBar->addAction(scriptingAct);

        // File browser
        fileBrowserWidget = new QWidget;
        fileSystemModel  = new QFileSystemModel;
        fileSelectedModel = new QStandardItemModel;

        fileSystemTree = new FileTreeView;
        fileSystemTree->setDragEnabled(1);
        fileSystemTree->setAcceptDrops(0);
        fileSystemTree->setDropIndicatorShown(1);
        fileSystemTree->setSortingEnabled(1);

        fileSelectedTree = new FileTreeView;
        fileSelectedTree->setDragEnabled(0);
        fileSelectedTree->setAcceptDrops(1);
        fileSelectedTree->setDropIndicatorShown(1);
        fileSelectedTree->setSortingEnabled(1);

        fileSystemModel->setRootPath(QDir::rootPath());
//        fileSelectedModel->setRootPath(QDir::rootPath());
        fileSystemTree->setModel(fileSystemModel);
        fileSelectedTree->setModel(fileSelectedModel);

        QGridLayout * fileBrowserLayout = new QGridLayout;
        fileBrowserLayout->setSpacing(0);
        fileBrowserLayout->setMargin(0);
        fileBrowserLayout->setContentsMargins(0,0,0,0);
        fileBrowserLayout->addWidget(fileSystemTree,0,0,1,1);
        fileBrowserLayout->addWidget(fileSelectedTree,0,1,1,1);

        fileBrowserWidget->setLayout(fileBrowserLayout);

        toggleScriptView();

        // Layout
        QGridLayout * scriptLayout = new QGridLayout;
        scriptLayout->setSpacing(0);
        scriptLayout->setMargin(0);
        scriptLayout->setContentsMargins(0,0,0,0);
        scriptLayout->addWidget(scriptToolBar,0,0,1,2);
        scriptLayout->addWidget(scriptTextEdit,1,0,1,2);
        scriptLayout->addWidget(fileBrowserWidget,2,0,1,2);

        setFilesWidget->setLayout(scriptLayout);
    }


    /* Image Widget */
    {
        imageControlsWidget = new QWidget;

        imageForwardButton = new QPushButton;
        imageForwardButton->setIcon(QIcon(":/art/forward.png"));
        imageForwardButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

        imageFastForwardButton = new QPushButton;
        imageFastForwardButton->setIcon(QIcon(":art/fast_forward.png"));
        imageFastForwardButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

        imageBackButton = new QPushButton;
        imageBackButton->setIcon(QIcon(":/art/back.png"));
        imageBackButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

        imageFastBackButton = new QPushButton;
        imageFastBackButton->setIcon(QIcon(":/art/fast_back.png"));
        imageFastBackButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

        imageNumberSpinBox = new QSpinBox;
        imageNumberSpinBox->setMinimum(0);
        imageNumberSpinBox->setAccelerated(true);


        QSurfaceFormat format_gl;
//        format_gl.setVersion(4, 3);
        format_gl.setSamples(16);
        format_gl.setRedBufferSize(8);
        format_gl.setGreenBufferSize(8);
        format_gl.setBlueBufferSize(8);
        format_gl.setAlphaBufferSize(8);

        imageRenderWorker = new ImageRenderWorker();
        imageRenderWorker->setOpenCLContext(context_cl);
        imageRenderWorker->setSharedWindow(sharedContextWindow);

        imageRenderWindow = new ImageRenderWindow();
        imageRenderWindow->setOpenGLWorker(imageRenderWorker);
        imageRenderWindow->setSharedWindow(sharedContextWindow);
        imageRenderWindow->setFormat(format_gl);
        imageRenderWindow->setOpenCLContext(context_cl);
        imageRenderWindow->setAnimating(true);


        imageRenderWidget = QWidget::createWindowContainer(imageRenderWindow);
        imageRenderWidget->setFocusPolicy(Qt::TabFocus);

        QGridLayout * imageLayout = new QGridLayout;
        imageLayout->setSpacing(0);
        imageLayout->setMargin(0);
        imageLayout->setContentsMargins(0,0,0,0);
        imageLayout->setRowStretch(0,1);
        imageLayout->setColumnStretch(0,1);
        imageLayout->setColumnStretch(6,1);
        imageLayout->addWidget(imageRenderWidget,0,0,1,7);
        imageLayout->addWidget(imageFastBackButton,1,1,1,1);
        imageLayout->addWidget(imageBackButton,1,2,1,1);
        imageLayout->addWidget(imageNumberSpinBox,1,3,1,1);
        imageLayout->addWidget(imageForwardButton,1,4,1,1);
        imageLayout->addWidget(imageFastForwardButton,1,5,1,1);

        imageWidget = new QWidget;
        imageWidget->setLayout(imageLayout);
    }


    /*      3D View widget      */
    {
        QSurfaceFormat format_gl;
//        format_gl.setVersion(4, 3);
        format_gl.setSamples(16);
        format_gl.setRedBufferSize(8);
        format_gl.setGreenBufferSize(8);
        format_gl.setBlueBufferSize(8);
        format_gl.setAlphaBufferSize(8);

        volumeRenderWorker = new VolumeRenderWorker();
        volumeRenderWorker->setOpenCLContext(context_cl);
        volumeRenderWorker->setSharedWindow(sharedContextWindow);

        volumeRenderWindow = new VolumeRenderWindow();
        volumeRenderWindow->setOpenGLWorker(volumeRenderWorker);
        volumeRenderWindow->setSharedWindow(sharedContextWindow);
        volumeRenderWindow->setFormat(format_gl);
        volumeRenderWindow->setOpenCLContext(context_cl);
        volumeRenderWindow->setAnimating(true);

        volumeRenderWidget = QWidget::createWindowContainer(volumeRenderWindow);
        volumeRenderWidget->setFocusPolicy(Qt::TabFocus);

        // Toolbar
        viewToolBar = new QToolBar(tr("3D View"));
        viewToolBar->addAction(openSVOAct);
        viewToolBar->addSeparator();
        viewToolBar->addAction(projectionAct);
        viewToolBar->addAction(backgroundAct);
        viewToolBar->addAction(logAct);
        viewToolBar->addAction(dataStructureAct);
        viewToolBar->addAction(scalebarAct);
        viewToolBar->addAction(screenshotAct);

        // Layout
        QGridLayout * viewLayout = new QGridLayout;
        viewLayout->setSpacing(0);
        viewLayout->setMargin(0);
        viewLayout->setContentsMargins(0,0,0,0);
//        viewLayout->setAlignment(Qt::AlignTop);
        viewLayout->addWidget(viewToolBar,0,0,1,1);
        viewLayout->addWidget(volumeRenderWidget,1,0,1,1);

        viewWidget = new QWidget;
        viewWidget->setLayout(viewLayout);
    }

    /*
     * QDockWidgets
     * */

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
        graphicsLayout->setSpacing(0);
        graphicsLayout->setMargin(0);
        graphicsLayout->setContentsMargins(0,0,0,0);

        graphicsLayout->addWidget(label_texture,0,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        graphicsLayout->addWidget(tsfComboBox,0,2,1,1);
        graphicsLayout->addWidget(tsfAlphaComboBox,0,3,1,1);
        graphicsLayout->addWidget(label_data_min,1,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        graphicsLayout->addWidget(dataMinSpinBox,1,2,1,2);
        graphicsLayout->addWidget(label_data_max,2,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        graphicsLayout->addWidget(dataMaxSpinBox,2,2,1,2);
        graphicsLayout->addWidget(label_alpha,3,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        graphicsLayout->addWidget(alphaSpinBox,3,2,1,2);
        graphicsLayout->addWidget(label_brightness,4,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        graphicsLayout->addWidget(brightnessSpinBox,4,2,1,2);
        graphicsLayout->addWidget(label_quality,5,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        graphicsLayout->addWidget(qualitySlider,5,2,1,2);

        graphicsWidget->setLayout(graphicsLayout);
////        graphicsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
////        graphicsWidget->setMaximumHeight(graphicsLayout->minimumSize().rheight());
        graphicsDockWidget->setWidget(graphicsWidget);
        viewMenu->addAction(graphicsDockWidget->toggleViewAction());
        this->addDockWidget(Qt::RightDockWidgetArea, graphicsDockWidget);
    }

    /* Unitcell dock widget */
    {
        unitcellDockWidget = new QDockWidget(tr("Unitcell Settings"), this);
        unitcellDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        unitcellWidget = new QWidget;

        QLabel * aLabel = new QLabel("<i>a<i>");
        a = new QLabel(tr("-"));
        QLabel * bLabel = new QLabel("<i>b<i>");
        b = new QLabel(tr("-"));
        QLabel * cLabel = new QLabel("<i>c<i>");
        c = new QLabel(tr("-"));
        QLabel * alphaLabel = new QLabel(trUtf8("<i>α<i>"));
        alpha = new QLabel(tr("-"));
        QLabel * betaLabel = new QLabel(trUtf8( "<i>β<i>"));
        beta = new QLabel(tr("-"));
        QLabel * gammaLabel = new QLabel(trUtf8( "<i>γ<i>"));
        gamma = new QLabel(tr("-"));


        QLabel * aStarLabel = new QLabel("<i>a*<i>");
        aStar = new QLabel(tr("-"));
        QLabel * bStarLabel = new QLabel("<i>b*<i>");
        bStar = new QLabel(tr("-"));
        QLabel * cStarLabel = new QLabel("<i>c*<i>");
        cStar = new QLabel(tr("-"));
        QLabel * alphaStarLabel = new QLabel(trUtf8("<i>α*<i>"));
        alphaStar = new QLabel(tr("-"));
        QLabel * betaStarLabel = new QLabel(trUtf8( "<i>β*<i>"));
        betaStar = new QLabel(tr("-"));
        QLabel * gammaStarLabel = new QLabel(trUtf8( "<i>γ*<i>"));
        gammaStar = new QLabel(tr("-"));

        unitcellButton = new QPushButton(tr("Toggle Unitcell"));
        loadParButton = new QPushButton(tr("Load Unitcell File"));

        QLabel * hklEditLabel = new QLabel(trUtf8( "<i>hkl: <i>"));
        hklEdit = new QLineEdit;
        //~ hklEdit->setFixedWidth(100);
        hklEdit->setValidator( new QRegExpValidator(QRegExp("(?:\\D+)?(?:[-+]?\\d+)(?:\\D+)?(?:[-+]?\\d+)(?:\\D+)?(?:[-+]?\\d+)")) );

        QGridLayout * unitcellLayout = new QGridLayout;
        unitcellLayout->setSpacing(0);
        unitcellLayout->setMargin(0);
        unitcellLayout->setContentsMargins(0,0,0,0);
        unitcellLayout->addWidget(unitcellButton,0,0,1,4);
        unitcellLayout->addWidget(loadParButton,1,0,1,4);
        unitcellLayout->addWidget(hklEditLabel,2,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(hklEdit,2,2,1,2);
        unitcellLayout->addWidget(aLabel,3,0,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(a,3,1,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(alphaLabel,3,2,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(alpha,3,3,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(bLabel,4,0,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(b,4,1,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(betaLabel,4,2,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(beta,4,3,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(cLabel,5,0,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(c,5,1,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(gammaLabel,5,2,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(gamma,5,3,1,1,Qt::AlignHCenter | Qt::AlignVCenter);

        unitcellLayout->addWidget(aStarLabel,6,0,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(aStar,6,1,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(alphaStarLabel,6,2,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(alphaStar,6,3,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(bStarLabel,7,0,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(bStar,7,1,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(betaStarLabel,7,2,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(betaStar,7,3,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(cStarLabel,8,0,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(cStar,8,1,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(gammaStarLabel,8,2,1,1,Qt::AlignHCenter | Qt::AlignVCenter);
        unitcellLayout->addWidget(gammaStar,8,3,1,1,Qt::AlignHCenter | Qt::AlignVCenter);

        unitcellWidget->setLayout(unitcellLayout);
//        //~unitcellWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
////        unitcellWidget->setMaximumHeight(unitcellLayout->minimumSize().rheight());
        unitcellDockWidget->setWidget(unitcellWidget);
        viewMenu->addAction(unitcellDockWidget->toggleViewAction());
        this->addDockWidget(Qt::RightDockWidgetArea, unitcellDockWidget);
    }

    /* File Controls Widget */
    {
        fileControlsWidget = new QWidget;

        // Labels
        QLabel * labelA = new QLabel(QString("Detector File Format:"));
        QLabel * labelB = new QLabel(QString("Correction Threshold:"));
        QLabel * labelC = new QLabel(QString("Projection Threshold:"));
        QLabel * labelD = new QLabel(QString("Octtree Levels: "));

        // Combo boxes and their labels
        formatComboBox = new QComboBox;
        formatComboBox->addItem("PILATUS CBF 1.2");
        formatComboBox->addItem("[ your file format here ]");

        // Spin Boxes
        treshLimA_DSB = new QDoubleSpinBox;
        treshLimA_DSB->setRange(0, 1e9);
        treshLimA_DSB->setSingleStep(1);
        treshLimA_DSB->setAccelerated(1);
        treshLimA_DSB->setDecimals(2);
        treshLimA_DSB->setFocusPolicy(Qt::ClickFocus);

        treshLimB_DSB = new QDoubleSpinBox;
        treshLimB_DSB->setRange(0, 1e9);
        treshLimB_DSB->setSingleStep(1);
        treshLimB_DSB->setAccelerated(1);
        treshLimB_DSB->setDecimals(2);
        treshLimB_DSB->setFocusPolicy(Qt::ClickFocus);

        treshLimC_DSB = new QDoubleSpinBox;
        treshLimC_DSB->setRange(0, 1e9);
        treshLimC_DSB->setSingleStep(1);
        treshLimC_DSB->setAccelerated(1);
        treshLimC_DSB->setDecimals(2);
        treshLimC_DSB->setFocusPolicy(Qt::ClickFocus);

        treshLimD_DSB = new QDoubleSpinBox;
        treshLimD_DSB->setRange(0, 1e9);
        treshLimD_DSB->setSingleStep(1);
        treshLimD_DSB->setAccelerated(1);
        treshLimD_DSB->setDecimals(2);
        treshLimD_DSB->setFocusPolicy(Qt::ClickFocus);

        svoLevelSpinBox = new QSpinBox;
        svoLevelSpinBox->setRange(1, 15);

        // Buttons
        saveSvoButton = new QPushButton;
        saveSvoButton->setIcon(QIcon(":/art/saveScript.png"));
        saveSvoButton->setText("Save Octtree");

        QGridLayout * fileLayout = new QGridLayout;
        fileLayout->setSpacing(0);
        fileLayout->setMargin(0);
        fileLayout->setContentsMargins(0,0,0,0);
        fileLayout->addWidget(labelA,0,0,1,4,Qt::AlignHCenter | Qt::AlignVCenter);
        fileLayout->addWidget(formatComboBox,0,4,1,4);
        fileLayout->addWidget(labelB,1,0,1,4,Qt::AlignHCenter | Qt::AlignVCenter);
        fileLayout->addWidget(treshLimA_DSB,1,4,1,2);
        fileLayout->addWidget(treshLimB_DSB,1,6,1,2);
        fileLayout->addWidget(labelC,2,0,1,4,Qt::AlignHCenter | Qt::AlignVCenter);
        fileLayout->addWidget(treshLimC_DSB,2,4,1,2);
        fileLayout->addWidget(treshLimD_DSB,2,6,1,2);
        fileLayout->addWidget(labelD,3,0,1,4,Qt::AlignHCenter | Qt::AlignVCenter);
        fileLayout->addWidget(svoLevelSpinBox,3,4,1,4);
        fileLayout->addWidget(saveSvoButton,4,0,1,8);
        fileControlsWidget->setLayout(fileLayout);
//        fileControlsWidget->setMaximumHeight(fileLayout->minimumSize().rheight());
        fileDockWidget = new QDockWidget(tr("Data Reduction Settings"), this);
        fileDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        fileDockWidget->setWidget(fileControlsWidget);
        fileDockWidget->setMaximumWidth(fileLayout->minimumSize().rwidth());
        viewMenu->addAction(fileDockWidget->toggleViewAction());
        this->addDockWidget(Qt::BottomDockWidgetArea, fileDockWidget);
    }

    /* Function dock widget */
    {
        QLabel * p0= new QLabel(QString("Var 1: "));
        QLabel * p1= new QLabel(QString("Var 2: "));
        QLabel * p2= new QLabel(QString("Var 3: "));
        QLabel * p3= new QLabel(QString("Var 4: "));

        functionToggleButton = new QPushButton(tr("Toggle On/Off"));
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

        functionDockWidget = new QDockWidget(tr("Function Settings"), this);
        functionWidget = new QWidget;

        QGridLayout * functionLayout = new QGridLayout;
        functionLayout->setSpacing(0);
        functionLayout->setMargin(0);
        functionLayout->setContentsMargins(0,0,0,0);
        functionLayout->addWidget(functionToggleButton,0,0,1,4);
        functionLayout->addWidget(p0,1,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        functionLayout->addWidget(funcParamASpinBox,1,2,1,2);
        functionLayout->addWidget(p1,2,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        functionLayout->addWidget(funcParamBSpinBox,2,2,1,2);
        functionLayout->addWidget(p2,3,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        functionLayout->addWidget(funcParamCSpinBox,3,2,1,2);
        functionLayout->addWidget(p3,4,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        functionLayout->addWidget(funcParamDSpinBox,4,2,1,2);
        functionWidget->setLayout(functionLayout);
//        functionWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
//        functionWidget->setMaximumHeight(functionLayout->minimumSize().rheight());
        functionDockWidget->setWidget(functionWidget);
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
//        progressBar->hide();

        // Text output
        errorTextEdit = new QPlainTextEdit;
        error_highlighter = new Highlighter(errorTextEdit->document());
        errorTextEdit->setReadOnly(true);

        // Layout
        QGridLayout * botLayout = new QGridLayout;
        botLayout->setSpacing(0);
        botLayout->setMargin(0);
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
    tabWidget->addTab(setFilesWidget, tr("Script Editor"));
    tabWidget->addTab(imageWidget, tr("Ewald's Projection"));
    tabWidget->addTab(viewWidget, tr("3D View"));

    // Put into main layout
    mainLayout->setMargin(0);
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
    QScreen * screen = QGuiApplication::primaryScreen();
    if (screen)
    {
        QPixmap screenshot = screen->grabWindow(volumeRenderWindow->winId());

        QString format = "jpg";
        QDateTime dateTime = dateTime.currentDateTime();
        QString initialPath = QDir::currentPath() + QString("/screenshot_"+dateTime.toString("yyyy_MM_dd_hh_mm_ss")) +"."+ format;

        QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), initialPath,
                                                    tr("%1 Files (*.%2);;All Files (*)")
                                                    .arg(format.toUpper())
                                                    .arg(format));
        if (!fileName.isEmpty()) screenshot.save(fileName, format.toLatin1().constData(), 100);
        print(QString("\n Saved: "+fileName));
    }
}


void MainWindow::test()
{
    qDebug() << "Main test";
    QElapsedTimer timer;
    timer.start();
    emit testToWindow();
    qDebug() << timer.restart() << " A";
}
