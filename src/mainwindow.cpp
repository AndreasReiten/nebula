#include "mainwindow.h"

MainWindow::MainWindow()
{
    std::cout << "Constructing MainWindow" << std::endl;
    
    // Stylesheet
    QFile styleFile( ":/src/stylesheets/gosutheme.qss" );
    styleFile.open( QFile::ReadOnly );
    QString style( styleFile.readAll() );
    styleFile.close();
    this->setStyleSheet(style);
    
    // Initialize
    QGLFormat glFormat(QGL::DoubleBuffer | QGL::DepthBuffer | QGL::Rgba | QGL::AlphaChannel | QGL::StencilBuffer | QGL::DirectRendering, 0);
    //~ glFormat.setSampleBuffers ( true );
    //~ glFormat.setSamples ( 8 );
        
    initGLCL = new InitGLCLWidget(glFormat);
    initGLCL->updateGL();
    initGLCL->hide();
    
    dataInstance = new VolumeDataSet(initGLCL->getCLDevice(), initGLCL->getCLContext(), initGLCL->getCLCommandQueue());
    
    createActions();
    
    createMenus();
    
    createInteractives();
    createConnects();
	setCentralWidget(mainWidget);
    readSettings();
    setCurrentFile("");
    init_emit();
    print("[NebulaX] Welcome to NebulaX alpha!");
    setWindowTitle(tr("NebulaX[*]"));

    graphicsDockWidget->hide();
    unitcellDockWidget->hide();
    functionDockWidget->hide();
    fileDockWidget->hide();
    toolChainWidget->show();
    outputDockWidget->show();
    
    std::cout << "Done Constructing MainWindow" << std::endl;
}

MainWindow::~MainWindow()
{
}

void MainWindow::init_emit()
{
    
    tabWidget->setCurrentIndex(0);
    //~ formatComboBox->setCurrentIndex(0);
    svoLevelSpinBox->setValue(7);
    
    treshLimA_DSB->setValue(10);
    treshLimB_DSB->setValue(1e9);
    treshLimC_DSB->setValue(10);
    treshLimD_DSB->setValue(1e9);
    
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


void MainWindow::newFile()
{
    if (maybeSave()) 
    {
        textEdit->clear();
        setCurrentFile("");
    }
}

void MainWindow::open()
{
    if (maybeSave()) 
    {
        QString fileName = QFileDialog::getOpenFileName(this);
        if (!fileName.isEmpty())
        {
            QFileInfo fileInfo = QFileInfo(fileName);
            if (fileInfo.size() < 5000000) loadFile(fileName);
            else print("\nFile is too large!");
            
        }
    }
}

void MainWindow::createActions()
{
    // Actions
    newAct = new QAction(QIcon(":/art/new.png"), tr("&New script"), this);
    openAct = new QAction(QIcon(":/art/open.png"), tr("&Open script"), this);
    saveAct = new QAction(QIcon(":/art/save.png"), tr("&Save script"), this);
	runScriptAct = new QAction(QIcon(":/art/forward.png"), tr("Run"), this);
    saveAsAct = new QAction(tr("Save script &As..."), this);
    exitAct = new QAction(tr("E&xit program"), this);
    aboutAct = new QAction(tr("&About NebulaX"), this);
    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutOpenCLAct = new QAction(tr("About OpenCL"), this);
    aboutOpenGLAct = new QAction(tr("About OpenGL"), this);
    aboutHDF5Act = new QAction(tr("About HDF"), this);
    openSVOAct = new QAction(QIcon(":/art/open.png"), tr("Load SVO"), this);
    logAct =  new QAction(QIcon(":/art/log.png"), tr("Toggle Logarithm"), this);
    dataStructureAct = new QAction(QIcon(":/art/datastructure.png"), tr("Toggle Data Structure"), this);
    backgroundAct = new QAction(QIcon(":/art/background.png"), tr("Toggle Background Color"), this);
    projectionAct = new QAction(QIcon(":/art/projection.png"), tr("Toggle Projection"), this);
    screenshotAct = new QAction(QIcon(":/art/screenshot.png"), tr("&Take Screenshot"), this);
    
    
    // Action Tips
    newAct->setStatusTip(tr("Create a new file"));
    openAct->setStatusTip(tr("Open an existing file"));
    saveAct->setStatusTip(tr("Save the document to disk"));
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
	runScriptAct->setStatusTip(tr("Run the script"));
    exitAct->setStatusTip(tr("Exit NebulaX"));
    aboutAct->setStatusTip(tr("About"));
    aboutQtAct->setStatusTip(tr("About Qt"));
    aboutOpenCLAct->setStatusTip(tr("About OpenCL"));
    aboutOpenGLAct->setStatusTip(tr("About OpenGL"));
    aboutHDF5Act->setStatusTip(tr("About HDF"));
    
    // Shortcuts
    newAct->setShortcuts(QKeySequence::New);
    openAct->setShortcuts(QKeySequence::Open);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    exitAct->setShortcuts(QKeySequence::Quit);
}

bool MainWindow::save()
    {
    if (curFile.isEmpty()) 
    {
        return saveAs();
    } 
    else 
    {
        return saveFile(curFile);
    }
}

bool MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty())
        return false;

    return saveFile(fileName);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About NebulaX"),
        tr("<h1>About NebulaX</h1> <b>NebulaX</b> is primarily a program to reduce, visualize, and analyze diffuse X-ray scattering. <br> <a href=\"www.github.org/\">github.org</a> <h1>Lisencing (LGPL)</h1> This program is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.\n This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details. \n You should have received a copy of the GNU Lesser General Public License along with this program.  If not, see <a href=\"http://www.gnu.org/licenses/\">http://www.gnu.org/licenses/</a> "));
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

void MainWindow::aboutHDF5()
{
    QMessageBox::about(this, tr("About HDF"),
        tr("<h1>About HDF</h1> <b>Hierarchical Data Format</b>  (HDF, HDF4, or HDF5) is the name of a set of file formats and libraries designed to store and organize large amounts of numerical data. Originally developed at the National Center for Supercomputing Applications, it is currently supported by the non-profit HDF Group, whose mission is to ensure continued development of HDF technologies, and the continued accessibility of data currently stored in HDF. <br> In keeping with this goal, the HDF format, libraries and associated tools are available under a liberal, BSD-like license for general use. <br> <a href=\"http://www.hdfgroup.org/\">www.hdfgroup.org/</a>"));
}

void MainWindow::documentWasModified()
{
    setWindowModified(textEdit->document()->isModified());
}
 
 
void MainWindow::toggleFullScreen()
{
    if (vrWidget->isFullScreen())
    {
        vrWidget->showNormal();
    }
    else
    {
        vrWidget->showFullScreen();
    }
}

void MainWindow::openUnitcellFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr(".par (*.par);; All Files (*)"));
    
    if ((fileName != "")) 
    {
        // Regular expressions to match data in .par files
        QString wavelengthRegExp("CRYSTALLOGRAPHY\\sWAVELENGTH\\D+(\\d+\\.\\d+)");
        
        QStringList UBRegExp = {
        "CRYSTALLOGRAPHY\\sUB\\D+([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+",
        "([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+",
        "([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+",
        "([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+",
        "([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+",
        "([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+",
        "([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+",
        "([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+",
        "([+-]?\\d+\\.\\d+E[+-]?\\d+)\\s+"};
        
        QStringList unitcellRegExp = {
        "CELL\\sINFORMATION\\D+(\\d+\\.\\d+)\\D+\\d+\\.\\d+\\D+",
        "(\\d+\\.\\d+)\\D+\\d+\\.\\d+\\D+",
        "(\\d+\\.\\d+)\\D+\\d+\\.\\d+\\D+",
        "(\\d+\\.\\d+)\\D+\\d+\\.\\d+\\D+",
        "(\\d+\\.\\d+)\\D+\\d+\\.\\d+\\D+",
        "(\\d+\\.\\d+)\\D+\\d+\\.\\d+\\D+"};
        
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
                //~ qDebug() << pos << " - "<< i << ": "<< value << " <->" << abc[i];
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
        float wavelength;
        pos = 0;
        QRegExp tmp(wavelengthRegExp);
        pos = tmp.indexIn(contents, pos);
        if (pos > -1) 
        {
            QString value = tmp.cap(1); 
            wavelength = value.toFloat();
            //~ qDebug() << pos << " - " << ": "<< value << " <->" << wavelength;
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
        
        vrWidget->setMatrixU(U.data());
        vrWidget->setMatrixB(B.data());
    }
}

void MainWindow::openSVO()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr(".h5 (*.h5);; All Files (*)"));
    
    if ((fileName != "")) 
    {
        /* HDF5 File structure
        * File ->
        *   /bricks -> (Data)
        *       n_bricks (Attribute)
        *       dim_brick (Attribute)
        * 
        *   /oct_index -> (Data)
        *       n_nodes (Attribute)
        *       n_levels (Attribute)
        *       extent (Attribute)
        * 
        *   /oct_brick -> (Data)
        *       brick_pool_power (Attribute)
        */
        
        hid_t file_id;
        hid_t dset_id, atrib_id, plist_id;
        herr_t status;
        hsize_t dims[5];
        
        size_t   nelmts;
        unsigned flags, filter_info;
        H5Z_filter_t filter_type;
        
        /* Open file */
        file_id = H5Fopen(fileName.toStdString().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        
        
        /* Get misc metadata */
        dset_id = H5Dopen(file_id, "/meta", H5P_DEFAULT);
        
        atrib_id = H5Aopen(dset_id, "hist_norm_len", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &dims[0] );
        status = H5Aclose(atrib_id);
        
        atrib_id = H5Aopen(dset_id, "hist_log10_len", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &dims[1] );
        status = H5Aclose(atrib_id);
        
        HIST_NORM.reserve(dims[0]);
        HIST_LOG.reserve(dims[1]);
        HIST_MINMAX.reserve(2);
        SVO_COMMENT.reserve(2000);
        
        atrib_id = H5Aopen(dset_id, "histogram_normal", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_DOUBLE, HIST_NORM.data() );
        status = H5Aclose(atrib_id);
        
        atrib_id = H5Aopen(dset_id, "histogram_log10", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_DOUBLE, HIST_LOG.data() );
        status = H5Aclose(atrib_id);
        
        atrib_id = H5Aopen(dset_id, "value_min_max", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_DOUBLE, HIST_MINMAX.data() );
        status = H5Aclose(atrib_id);
        
        status = H5Dread(dset_id, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, SVO_COMMENT.data());
        status = H5Dclose(dset_id);
        
        /* Get brick data */
        dset_id = H5Dopen(file_id, "/bricks", H5P_DEFAULT);
        
        atrib_id = H5Aopen(dset_id, "n_bricks", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &VIEW_N_BRICKS );
        status = H5Aclose(atrib_id);
        
        atrib_id = H5Aopen(dset_id, "dim_brick", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &VIEW_DIM_BRICKS );
        status = H5Aclose(atrib_id);
        
        VIEW_BRICKS.reserve(VIEW_N_BRICKS*VIEW_DIM_BRICKS*VIEW_DIM_BRICKS*VIEW_DIM_BRICKS);
        
        plist_id = H5Dget_create_plist(dset_id);
        nelmts = 0;
        filter_type = H5Pget_filter(plist_id, 0, &flags, &nelmts, NULL, 0, NULL, &filter_info);
        status = H5Dread(dset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, VIEW_BRICKS.data());
        
        status = H5Dclose(dset_id);
        status = H5Pclose (plist_id);
        
        
        /* Get octtree data */
        dset_id = H5Dopen(file_id, "/oct_index", H5P_DEFAULT);
        
        atrib_id = H5Aopen(dset_id, "n_nodes", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &dims[0] );
        status = H5Aclose(atrib_id);
        
        atrib_id = H5Aopen(dset_id, "n_levels", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &VIEW_LEVELS );
        status = H5Aclose(atrib_id);
        
        atrib_id = H5Aopen(dset_id, "extent", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_FLOAT, VIEW_EXTENT );
        status = H5Aclose(atrib_id);
        
        VIEW_OCT_INDEX.reserve(dims[0]);
        VIEW_OCT_BRICK.reserve(dims[0]);
        
        status = H5Dread(dset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, VIEW_OCT_INDEX.data());
        status = H5Dclose(dset_id);
        
        
        dset_id = H5Dopen(file_id, "/oct_brick", H5P_DEFAULT);
        
        atrib_id = H5Aopen(dset_id, "brick_pool_power", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &VIEW_BPP );
        status = H5Aclose(atrib_id);

        status = H5Dread(dset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, VIEW_OCT_BRICK.data());
        status = H5Dclose(dset_id);
        
        /* Close the file */
        status = H5Fclose(file_id);
        
        
        vrWidget->setOCT_INDEX(&(this->VIEW_OCT_INDEX), VIEW_LEVELS, VIEW_EXTENT);
        vrWidget->setOCT_BRICK(&(this->VIEW_OCT_BRICK), VIEW_BPP);
        vrWidget->setBRICKS(&(this->VIEW_BRICKS), VIEW_N_BRICKS, VIEW_DIM_BRICKS);
        vrWidget->setMeta(&(this->HIST_NORM), &(this->HIST_LOG), &(this->HIST_MINMAX), &(this->SVO_COMMENT));
        alphaSpinBox->setValue(0.1);
        brightnessSpinBox->setValue(2.0);
        dataMinSpinBox->setValue(this->HIST_MINMAX[0]);
        dataMaxSpinBox->setValue(this->HIST_MINMAX[1]);
        
        
        print("\nLoaded file: \""+fileName+"\"");
        
        tabWidget->setCurrentIndex(4);
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
            break;
            
        case 1:
            graphicsDockWidget->hide();
            unitcellDockWidget->hide();
            functionDockWidget->hide();
            toolChainWidget->show();
            fileDockWidget->show();
            outputDockWidget->show();
            //~ irWidget->makeCurrent();
            break;
            
        case 2:
            toolChainWidget->hide();
            fileDockWidget->hide();
            outputDockWidget->hide();
            graphicsDockWidget->show();
            unitcellDockWidget->show();
            functionDockWidget->show();
            //~ vrWidget->makeCurrent();
            break;
            
        default:
            std::cout << "Reverting to Default Tab" << std::endl;
            break;
    }
    //~ std::cout << "initCLGL: Alpha Channel = " << initGLCL->format().alpha() << " size = "<< initGLCL->format().alphaBufferSize()<< "sharing = "<< initGLCL->isSharing() <<std::endl;
    //~ std::cout << "vrWidget Alpha Channel = " << vrWidget->format().alpha() << " size = "<< vrWidget->format().alphaBufferSize()<< "sharing = "<< vrWidget->isSharing() <<std::endl;
    //~ std::cout << "irWidget Alpha Channel = " << irWidget->format().alpha() << " size = "<< irWidget->format().alphaBufferSize()<< "sharing = "<< irWidget->isSharing() << std::endl;
}

void MainWindow::createConnects()
{
    /* this <-> vrWidget */
    connect(this->tsfAlphaComboBox, SIGNAL(activated(int)), vrWidget, SLOT(setTsfAlphaStyle(int)));
    connect(dataStructureAct, SIGNAL(triggered()), vrWidget, SLOT(toggleDataStructure()));
    connect(backgroundAct, SIGNAL(triggered()), vrWidget, SLOT(toggleBackground()));
    connect(screenshotAct, SIGNAL(triggered()), vrWidget, SLOT(takeScreenshot()));
    connect(logAct, SIGNAL(triggered()), vrWidget, SLOT(toggleLog()));
    connect(projectionAct, SIGNAL(triggered()), vrWidget, SLOT(togglePerspective()));
    //~ connect(this->graphicsBackgroundButton, SIGNAL(clicked()), vrWidget, SLOT(toggleBackground()));
    //~ connect(this->screenshotButton, SIGNAL(clicked()), vrWidget, SLOT(takeScreenshot()));
    connect(this->tsfComboBox, SIGNAL(activated(int)), vrWidget, SLOT(setTsf(int)));
    connect(this->dataMinSpinBox, SIGNAL(valueChanged(double)), vrWidget, SLOT(setTsfMin(double)));
    connect(this->dataMaxSpinBox, SIGNAL(valueChanged(double)), vrWidget, SLOT(setTsfMax(double)));
    connect(this->alphaSpinBox, SIGNAL(valueChanged(double)), vrWidget, SLOT(setTsfAlpha(double)));
    connect(this->brightnessSpinBox, SIGNAL(valueChanged(double)), vrWidget, SLOT(setTsfBrightness(double)));
    //~ connect(this->graphicsLogarithmButton, SIGNAL(clicked()), vrWidget, SLOT(toggleLog()));
    //~ connect(this->graphicsDataStructureButton, SIGNAL(clicked()), vrWidget, SLOT(toggleDataStructure()));
    //~ connect(this->graphicsPerspectiveButton, SIGNAL(clicked()), vrWidget, SLOT(togglePerspective()));
    connect(this->functionToggleButton, SIGNAL(clicked()), vrWidget, SLOT(toggleFunctionView()));
    connect(this->funcParamASpinBox, SIGNAL(valueChanged(double)), vrWidget, SLOT(setFuncParamA(double)));
    connect(this->funcParamBSpinBox, SIGNAL(valueChanged(double)), vrWidget, SLOT(setFuncParamB(double)));
    connect(this->funcParamCSpinBox, SIGNAL(valueChanged(double)), vrWidget, SLOT(setFuncParamC(double)));
    connect(this->funcParamDSpinBox, SIGNAL(valueChanged(double)), vrWidget, SLOT(setFuncParamD(double)));
    connect(vrWidget, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(this->unitcellButton, SIGNAL(clicked()), vrWidget, SLOT(toggleUnitcellView()));
    connect(this->hklEdit, SIGNAL(textChanged(const QString)), vrWidget, SLOT(setHklFocus(const QString)));
    connect(vrWidget, SIGNAL(changedAlphaValue(double)), this->alphaSpinBox, SLOT(setValue(double)));
    connect(vrWidget, SIGNAL(changedBrightnessValue(double)), this->brightnessSpinBox, SLOT(setValue(double)));
    connect(vrWidget, SIGNAL(changedDataMinValue(double)), this->dataMinSpinBox, SLOT(setValue(double)));
    connect(vrWidget, SIGNAL(changedDataMaxValue(double)), this->dataMaxSpinBox, SLOT(setValue(double)));
    connect(vrWidget, SIGNAL(changedFuncParamA(double)), this->funcParamASpinBox, SLOT(setValue(double)));
    connect(vrWidget, SIGNAL(changedFuncParamB(double)), this->funcParamBSpinBox, SLOT(setValue(double)));
    connect(vrWidget, SIGNAL(changedFuncParamC(double)), this->funcParamCSpinBox, SLOT(setValue(double)));
    connect(vrWidget, SIGNAL(changedFuncParamD(double)), this->funcParamDSpinBox, SLOT(setValue(double)));

    /* this <-> dataInstance */
    connect(this->svoLevelSpinBox, SIGNAL(valueChanged(int)), dataInstance, SLOT(setSvoLevels(int)));
    connect(saveSVOButton, SIGNAL(clicked()), dataInstance, SLOT(saveSVO()));
    connect(formatComboBox, SIGNAL(currentIndexChanged(int)), dataInstance, SLOT(setFormat(int)));
    connect(dataInstance, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGenericProgressFormat(QString)));
    connect(treshLimA_DSB, SIGNAL(valueChanged(double)), dataInstance, SLOT(setLowTresholdReduce(double)));
    connect(treshLimB_DSB, SIGNAL(valueChanged(double)), dataInstance, SLOT(setHighTresholdReduce(double)));
    connect(treshLimC_DSB, SIGNAL(valueChanged(double)), dataInstance, SLOT(setLowTresholdProject(double)));
    connect(treshLimD_DSB, SIGNAL(valueChanged(double)), dataInstance, SLOT(setHighTresholdProject(double)));
    connect(dataInstance, SIGNAL(changedGenericProgress(int)), mainProgress, SLOT(setValue(int)));
    connect(dataInstance, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(this->imageForwardButton, SIGNAL(clicked()), dataInstance, SLOT(incrementDisplayFrame1()));
    connect(this->imageFastForwardButton, SIGNAL(clicked()), dataInstance, SLOT(incrementDisplayFrame5()));
    connect(this->imageBackButton, SIGNAL(clicked()), dataInstance, SLOT(decrementDisplayFrame1()));
    connect(this->imageFastBackButton, SIGNAL(clicked()), dataInstance, SLOT(decrementDisplayFrame5()));
    connect(this->imageNumberSpinBox, SIGNAL(valueChanged(int)), dataInstance, SLOT(setDisplayFrame(int)));
    connect(dataInstance, SIGNAL(displayFrameChanged(int)), this->imageNumberSpinBox, SLOT(setValue(int)));
    connect(this , SIGNAL(changedPaths(QStringList)), dataInstance, SLOT(setPaths(QStringList)));
    /* irWidget <-> dataInstance */
    connect(dataInstance, SIGNAL(changedRawImage(PilatusFile *)), irWidget, SLOT(setRawImage(PilatusFile *)));
    connect(dataInstance, SIGNAL(changedCorrectedImage(PilatusFile *)), irWidget, SLOT(setCorrectedImage(PilatusFile *)));
    connect(dataInstance, SIGNAL(repaintRequest()), irWidget, SLOT(repaint()));
    
    /* this <-> this */
    connect(textEdit->document(), SIGNAL(contentsChanged()), this, SLOT(documentWasModified()));
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(setTab(int)));
    connect(openSVOAct, SIGNAL(triggered()), this, SLOT(openSVO()));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));
	connect(runScriptAct, SIGNAL(triggered()), this, SLOT(runReadScript()));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
    connect(aboutOpenCLAct, SIGNAL(triggered()), this, SLOT(aboutOpenCL()));
    connect(aboutOpenGLAct, SIGNAL(triggered()), this, SLOT(aboutOpenGL()));
    connect(aboutHDF5Act, SIGNAL(triggered()), this, SLOT(aboutHDF5()));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(readScriptButton, SIGNAL(clicked()), this, SLOT(runReadScript()));
    connect(setFilesButton, SIGNAL(clicked()), this, SLOT(runSetFiles()));
    connect(readFilesButton, SIGNAL(clicked()), this, SLOT(runReadFiles()));
    connect(allInOneButton, SIGNAL(clicked()), this, SLOT(runAllInOne()));
    connect(projectFilesButton, SIGNAL(clicked()), this, SLOT(runProjectFiles()));
    connect(generateSvoButton, SIGNAL(clicked()), this, SLOT(runGenerateSvo()));
    connect(loadParButton, SIGNAL(clicked()), this, SLOT(openUnitcellFile()));
}

void MainWindow::setGenericProgressFormat(QString str)
{
    mainProgress->setFormat(str);
}

void MainWindow::previewSVO()
{
    if (!(dataInstance->getOCT_INDEX()->size() > 0) && !(dataInstance->getOCT_BRICK()->size() > 0) && !(dataInstance->getBRICKS()->size() > 0))
    {
        print("\nWarning: No data to preview!");
        return;
    }
    else 
    {
        print("\nPreview enabled in 3D View tab.");
        vrWidget->setOCT_INDEX(dataInstance->getOCT_INDEX(), dataInstance->getLEVELS(), dataInstance->getExtent());
        vrWidget->setOCT_BRICK(dataInstance->getOCT_BRICK(), dataInstance->getBPP());
        vrWidget->setBRICKS(dataInstance->getBRICKS(), dataInstance->getN_BRICKS(), dataInstance->getBRICK_DIM_TOT());
        
        tabWidget->setCurrentIndex(4);
    }
}



void MainWindow::createMenus()
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
    helpMenu->addAction(aboutHDF5Act);
    
    mainMenu->addMenu(scriptMenu);
    mainMenu->addMenu(viewMenu);
    mainMenu->addSeparator();
    mainMenu->addMenu(helpMenu);
}



void MainWindow::createInteractives()
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
          
        setFilesButton = new QPushButton;
        setFilesButton->setIcon(QIcon(":/art/proceed.png"));
        setFilesButton->setIconSize(QSize(24,24));
        setFilesButton->setText("Set ");
        setFilesButton->setEnabled(false);

        readFilesButton = new QPushButton;
        readFilesButton->setIcon(QIcon(":/art/proceed.png"));
        readFilesButton->setIconSize(QSize(24,24));
        readFilesButton->setText("Read ");
        readFilesButton->setEnabled(false);

        projectFilesButton = new QPushButton;
        projectFilesButton->setIcon(QIcon(":/art/proceed.png"));
        projectFilesButton->setIconSize(QSize(24,24));
        projectFilesButton->setText("Correction and Projection ");
        projectFilesButton->setEnabled(false);

        generateSvoButton = new QPushButton;
        generateSvoButton->setIcon(QIcon(":/art/proceed.png"));
        generateSvoButton->setText("Voxelize ");
        generateSvoButton->setIconSize(QSize(32,32));
        generateSvoButton->setEnabled(false);
        generateSvoButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        allInOneButton = new QPushButton;
        allInOneButton->setIcon(QIcon(":/art/fast_proceed.png"));
        allInOneButton->setText("All of Above (reduced memory consumption) ");
        allInOneButton->setIconSize(QSize(24,24));
        allInOneButton->setEnabled(false);

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
        toolChainLayout->addWidget(setFilesButton,0,1,1,1);
        toolChainLayout->addWidget(readFilesButton,0,2,1,1);
        toolChainLayout->addWidget(projectFilesButton,0,3,1,1);
        toolChainLayout->addWidget(generateSvoButton,0,4,2,1);
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
        scriptWidget = new QWidget;
        scriptWidget->setObjectName("scriptWidget");
        
        // Script text edit
        textEdit = new QPlainTextEdit;
        script_highlighter = new Highlighter(textEdit->document());
        scriptHelp = "/* Add file paths using this Javascript window. \nDo this by appedning paths to the variable 'files'. */ \n\n files = ";
        textEdit->setPlainText(scriptHelp);
        
        // Toolbar
        scriptToolBar = new QToolBar(tr("Script"));
        scriptToolBar->addAction(newAct);
        scriptToolBar->addAction(openAct);
        scriptToolBar->addAction(saveAct);
                
        // Layout 
        QGridLayout * scriptLayout = new QGridLayout;
        scriptLayout->setSpacing(0);
        scriptLayout->setMargin(0);
        scriptLayout->setContentsMargins(0,0,0,0);
        scriptLayout->addWidget(scriptToolBar,0,0,1,2);
        scriptLayout->addWidget(textEdit,1,0,1,2);
        
        scriptWidget->setLayout(scriptLayout);
    }
    
    
    /* Image Widget */
    {
        imageWidget = new QWidget;
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

        //~ std::cout << "Before Set: INITGLCL Render: Alpha Channel = " << initGLCL->format().alpha() << std::endl;
        
        irWidget = new ImageRenderGLWidget(initGLCL->getCLDevice(), initGLCL->getCLContext(), initGLCL->getCLCommandQueue(), initGLCL->format(), 0, initGLCL);

        QGridLayout * imageLayout = new QGridLayout;
        imageLayout->setSpacing(0);
        imageLayout->setMargin(0);
        imageLayout->setContentsMargins(0,0,0,0);
        imageLayout->setColumnStretch(0,1);
        imageLayout->setColumnStretch(6,1);
        imageLayout->addWidget(irWidget,0,0,1,7);
        imageLayout->addWidget(imageFastBackButton,1,1,1,1);
        imageLayout->addWidget(imageBackButton,1,2,1,1);
        imageLayout->addWidget(imageNumberSpinBox,1,3,1,1);
        imageLayout->addWidget(imageForwardButton,1,4,1,1);
        imageLayout->addWidget(imageFastForwardButton,1,5,1,1);
        
        imageWidget->setLayout(imageLayout);
    }
    
    
    /*      3D View widget      */
    {
        //~ std::cout << "Before Set: INITGLCL Render: Alpha Channel = " << initGLCL->format().alpha() << std::endl;
        viewWidget = new QWidget;
        vrWidget = new VolumeRenderGLWidget(initGLCL->getCLDevice(), initGLCL->getCLContext(), initGLCL->getCLCommandQueue(), initGLCL->format(), 0, initGLCL);
        //~ vrWidget->setCursor(Qt::SizeAllCursor);
        
        // Toolbar
        viewToolBar = new QToolBar(tr("3D View"));
        viewToolBar->addAction(openSVOAct);
        viewToolBar->addSeparator();
        viewToolBar->addAction(projectionAct);
        viewToolBar->addAction(backgroundAct);
        viewToolBar->addAction(logAct);
        viewToolBar->addAction(dataStructureAct);
        viewToolBar->addAction(screenshotAct);
    
        // Layout  
        viewLayout = new QGridLayout;
        viewLayout->setSpacing(0);
        viewLayout->setMargin(0);
        viewLayout->setContentsMargins(0,0,0,0);
        viewLayout->setAlignment(Qt::AlignTop);
        viewLayout->addWidget(viewToolBar,0,0,1,1);
        viewLayout->addWidget(vrWidget,1,0,1,1);
        viewWidget->setLayout(viewLayout);
    }
    
    /*
     * QDockWidgets
     * */
    
    /* Graphics dock widget */
    {
        QLabel * l3= new QLabel(QString("Texture "));
        QLabel * l4= new QLabel(QString("Min: "));
        QLabel * l5= new QLabel(QString("Max: "));
        QLabel * l6= new QLabel(QString("Alpha: "));
        QLabel * l7= new QLabel(QString("Brightness: "));
        
        //~ graphicsBackgroundButton = new QPushButton(tr("Toggle Background"));
        //~ graphicsDataStructureButton = new QPushButton(tr("Show Data Structure"));
        //~ graphicsLogarithmButton = new QPushButton(tr("Toggle Logarithmic"));
        //~ graphicsPerspectiveButton = new QPushButton(tr("Toggle Perspective"));
        //~ screenshotButton = new QPushButton(tr("Take Screenshot"));
        
        
        dataMinSpinBox = new QDoubleSpinBox;
        dataMinSpinBox->setDecimals(2);
        dataMinSpinBox->setRange(0, 1e9);
        dataMinSpinBox->setSingleStep(0.1);
        dataMinSpinBox->setAccelerated(1);
        
        dataMaxSpinBox = new QDoubleSpinBox;
        dataMaxSpinBox->setDecimals(1);
        dataMaxSpinBox->setRange(0, 1e9);
        dataMaxSpinBox->setSingleStep(0.1);
        dataMaxSpinBox->setAccelerated(1);
        
        alphaSpinBox = new QDoubleSpinBox;
        alphaSpinBox->setDecimals(3);
        alphaSpinBox->setRange(0.001, 5);
        alphaSpinBox->setSingleStep(0.01);
        alphaSpinBox->setAccelerated(1);
        
        brightnessSpinBox = new QDoubleSpinBox;
        brightnessSpinBox->setDecimals(3);
        brightnessSpinBox->setRange(0.001, 5);
        brightnessSpinBox->setSingleStep(0.01);
        brightnessSpinBox->setAccelerated(1);
        
        tsfComboBox = new QComboBox;
        tsfComboBox->addItem(trUtf8("Hot"));
        tsfComboBox->addItem(trUtf8("Winter"));
        tsfComboBox->addItem(trUtf8("Ice"));
        tsfComboBox->addItem(trUtf8("Rainbow"));
        tsfComboBox->addItem(trUtf8("Hsv"));
        tsfComboBox->addItem(trUtf8("Binary"));
        tsfComboBox->addItem(trUtf8("Yranib"));
        tsfComboBox->addItem(trUtf8("Galaxy"));
        tsfComboBox->addItem(trUtf8("White"));
        tsfComboBox->addItem(trUtf8("Black"));

        tsfAlphaComboBox = new QComboBox;
        tsfAlphaComboBox->addItem(trUtf8("Uniform"));
        tsfAlphaComboBox->addItem(trUtf8("Linear"));
        tsfAlphaComboBox->addItem(trUtf8("Exponential"));
        
        
        graphicsDockWidget = new QDockWidget(tr("View Settings"), this);
        graphicsDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);    
        graphicsWidget = new QWidget;
        
        QGridLayout * graphicsLayout = new QGridLayout;
        graphicsLayout->setSpacing(0);
        graphicsLayout->setMargin(0);
        graphicsLayout->setContentsMargins(0,0,0,0);
        graphicsLayout->addWidget(l3,4,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        graphicsLayout->addWidget(tsfComboBox,4,2,1,1);
        graphicsLayout->addWidget(tsfAlphaComboBox,4,3,1,1);
        graphicsLayout->addWidget(l4,5,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        graphicsLayout->addWidget(dataMinSpinBox,5,2,1,2);
        graphicsLayout->addWidget(l5,6,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        graphicsLayout->addWidget(dataMaxSpinBox,6,2,1,2);
        graphicsLayout->addWidget(l6,7,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        graphicsLayout->addWidget(alphaSpinBox,7,2,1,2);
        graphicsLayout->addWidget(l7,8,0,1,2,Qt::AlignHCenter | Qt::AlignVCenter);
        graphicsLayout->addWidget(brightnessSpinBox,8,2,1,2);
        //~ graphicsLayout->addWidget(graphicsBackgroundButton,9,0,1,8);
        //~ graphicsLayout->addWidget(graphicsPerspectiveButton,10,0,1,8);
        //~ graphicsLayout->addWidget(graphicsLogarithmButton,11,0,1,8);
        //~ graphicsLayout->addWidget(graphicsDataStructureButton,12,0,1,8);
        //~ graphicsLayout->addWidget(screenshotButton,13,0,1,8);
        
        graphicsWidget->setLayout(graphicsLayout);
        graphicsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        graphicsWidget->setMaximumHeight(graphicsLayout->minimumSize().rheight());
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
        unitcellWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        unitcellWidget->setMaximumHeight(unitcellLayout->minimumSize().rheight());
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
        treshLimA_DSB->setRange(1, 1e9);
        treshLimA_DSB->setSingleStep(1);
        treshLimA_DSB->setAccelerated(1);
        treshLimA_DSB->setDecimals(2);
        treshLimA_DSB->setFocusPolicy(Qt::ClickFocus);
        
        treshLimB_DSB = new QDoubleSpinBox;
        treshLimB_DSB->setRange(1, 1e9);
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
        svoLevelSpinBox->setRange(2, 15);
        
        // Buttons
        saveSVOButton = new QPushButton;
        saveSVOButton->setIcon(QIcon(":/art/save.png"));
        saveSVOButton->setText("Save Octtree");
        
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
        fileLayout->addWidget(saveSVOButton,4,0,1,8);
        
        fileControlsWidget->setLayout(fileLayout);
        fileControlsWidget->setMaximumHeight(fileLayout->minimumSize().rheight());
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
        functionWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        functionWidget->setMaximumHeight(functionLayout->minimumSize().rheight());
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
        mainProgress = new QProgressBar;
        mainProgress->setRange( 0, 100 );
        mainProgress->hide();
        
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
        botLayout->addWidget(mainProgress, 1, 0, 1, 1);
        
        botWidget->setLayout(botLayout);
        outputDockWidget->setWidget(botWidget);
        viewMenu->addAction(outputDockWidget->toggleViewAction());
        this->addDockWidget(Qt::BottomDockWidgetArea, outputDockWidget);
    }

    
    /*      Tab widget      */
    tabWidget = new QTabWidget(mainWidget);

    // Add tabs
    tabWidget->addTab(scriptWidget, tr("Script Editor"));
    tabWidget->addTab(imageWidget, tr("Ewald's Projection"));
    tabWidget->addTab(viewWidget, tr("3D View"));
    
    // Put into main layout
    mainLayout->setMargin(0);
    mainLayout->setContentsMargins(3,3,3,3);
    mainLayout->addWidget(topWidget,0,0,1,1);
    mainLayout->addWidget(tabWidget,1,0,1,1);
	mainWidget->setLayout(mainLayout); 

    /* Script engine */
	rawFilesQs = engine.newVariant(rawFiles);
	engine.globalObject().setProperty("files", rawFilesQs);    
}

void MainWindow::runAllInOne()
{
    /*################################################################*/    
	/* STEP ONE, TWO, and THREE - All in one go to minimize intermediate
     * memory consumption between stages */
    
    mainProgress->show();
    tabWidget->setCurrentIndex(1);
    allInOneButton->setEnabled(false);
    readFilesButton->setEnabled(false);
    projectFilesButton->setEnabled(false);
    
    #ifndef QT_NO_CURSOR
        QApplication::setOverrideCursor(Qt::WaitCursor);
    #endif

    int STATUS_OK = dataInstance->funcAllInOne();
    
    #ifndef QT_NO_CURSOR
        QApplication::restoreOverrideCursor();
    #endif

    allInOneButton->setEnabled(true);
    if (STATUS_OK)
    {
        generateSvoButton->setEnabled(true);
    }
    mainProgress->hide();
}


void MainWindow::runReadFiles()
{
    /*################################################################*/    
	/* STEP TWO - READING FILE CONTENTS */
    mainProgress->show();
    tabWidget->setCurrentIndex(1);
    readFilesButton->setEnabled(false);
    
    #ifndef QT_NO_CURSOR
        QApplication::setOverrideCursor(Qt::WaitCursor);
    #endif
    int STATUS_OK = dataInstance->funcReadFiles();
    #ifndef QT_NO_CURSOR
        QApplication::restoreOverrideCursor();
    #endif

    readFilesButton->setEnabled(true);
    if (STATUS_OK)
    {
        projectFilesButton->setEnabled(true);
        generateSvoButton->setEnabled(false);
    }
    mainProgress->hide();
}

void MainWindow::runSetFiles()
{
	/*################################################################*/    
	/* STEP ONE - HEADER RETRIEVEAL*/
    mainProgress->show();
    tabWidget->setCurrentIndex(1);
    setFilesButton->setEnabled(false);
    
    #ifndef QT_NO_CURSOR
        QApplication::setOverrideCursor(Qt::WaitCursor);
    #endif

    int STATUS_OK = dataInstance->funcSetFiles();
    
    #ifndef QT_NO_CURSOR
        QApplication::restoreOverrideCursor();
    #endif

    setFilesButton->setEnabled(true);
    if (STATUS_OK)
    {
        readFilesButton->setEnabled(true);
        projectFilesButton->setEnabled(false);
        generateSvoButton->setEnabled(false);
    }
    mainProgress->hide();
}

void MainWindow::runProjectFiles()
{
    /*################################################################*/    
	/* STEP THREE - EWALD PROJECTION */
    
    mainProgress->show();
    tabWidget->setCurrentIndex(1);
    projectFilesButton->setEnabled(false);
    
    #ifndef QT_NO_CURSOR
        QApplication::setOverrideCursor(Qt::WaitCursor);
    #endif

    int STATUS_OK = dataInstance->funcProjectFiles();
    
    #ifndef QT_NO_CURSOR
        QApplication::restoreOverrideCursor();
    #endif

    projectFilesButton->setEnabled(true);
    if (STATUS_OK)
    {
        generateSvoButton->setEnabled(true);
    }
    mainProgress->hide();
}

void MainWindow::runGenerateSvo()
{
    /*################################################################*/    
	/* STEP FOUR - GENERATING A SPARSE VOXEL OCTREE*/
    mainProgress->show();
    tabWidget->setCurrentIndex(1);
    generateSvoButton->setEnabled(false);
    
    #ifndef QT_NO_CURSOR
        QApplication::setOverrideCursor(Qt::WaitCursor);
    #endif

    int STATUS_OK = dataInstance->funcGenerateSvo();
    
    #ifndef QT_NO_CURSOR
        QApplication::restoreOverrideCursor();
    #endif

    generateSvoButton->setEnabled(true);
    mainProgress->hide();
}

void MainWindow::print(QString str)
{
    info.append(str);
    size_t chars_max = 65536;
    size_t removable = info.size()- chars_max;
    if (removable > 0) info.remove(0, removable);
    
    errorTextEdit->setPlainText(info);
    errorTextEdit->moveCursor(QTextCursor::End) ;
    errorTextEdit->ensureCursorVisible();
}


void MainWindow::runReadScript()
{
	/*################################################################*/    
	/* STEP ZERO - FINDING FILES */
	
	// Set the corresponding tab
    tabWidget->setCurrentIndex(0);
    
	// Evaluate the script input
    engine.evaluate("var files = [];");
	engine.evaluate(textEdit->toPlainText());
	if (engine.hasUncaughtException() == true)
	{
		// Exceptions
		print( "\n[Script] Error: Uncaught exception in line " + QString::number(engine.uncaughtExceptionLineNumber()) + "\n[Script] " + engine.uncaughtException().toString());
	}	
	else
	{
		// Store evaluated file paths in a list
		#ifndef QT_NO_CURSOR
			QApplication::setOverrideCursor(Qt::WaitCursor);
		#endif
		
		rawFiles = engine.globalObject().property("files").toVariant().toStringList();
        print( "\n[Script] Script ran successfully and could register "+QString::number(rawFiles.size())+" files...");
        int n = rawFiles.removeDuplicates();
        if (n > 0) print( "\n[Script] Removed "+QString::number(n)+" duplicates...");
		
        size_t n_files = rawFiles.size();
        
        for (int i = 0; i < rawFiles.size(); i++)
		{
			if(i >= rawFiles.size()) break;
            
            QString fileName = rawFiles[i];
            
            QFileInfo curFile(fileName);
            
            if (!curFile.exists())
			{
				print( "\n[Script]  Warning: \"" + fileName + "\" - missing or no access!");
                rawFiles.removeAt(i);
                i--;
            }
        }
        emit changedPaths(rawFiles);
		print("\n[Script] "+ QString::number(rawFiles.size())+" of "+QString::number(n_files)+" files successfully found ("+QString::number(n_files-rawFiles.size())+"  missing or no access)");
        
        if (rawFiles.size() > 0) 
        {
            setFilesButton->setEnabled(true);
            allInOneButton->setEnabled(true);
            readFilesButton->setEnabled(false);
            projectFilesButton->setEnabled(false);
            generateSvoButton->setEnabled(false);
        }
        
		#ifndef QT_NO_CURSOR
			QApplication::restoreOverrideCursor();
		#endif
	}
    
}

void MainWindow::readSettings()
{
    QSettings settings("Norwegian University of Science and Technology", "NebulaX");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    svoDir = settings.value("svoDir", "").toString();
    svoDir = settings.value("scriptDir", "").toString();
    resize(size);
    move(pos);
}

void MainWindow::writeSettings()
{
    QSettings settings("Norwegian University of Science and Technology", "NebulaX");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("svoDir", svoDir);
    settings.setValue("scriptDir", svoDir);
}

bool MainWindow::maybeSave()
{
    if (textEdit->document()->isModified()) 
    {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("NebulaX"),
            tr("The script has been modified.\n"
            "Do you want to save your changes?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return save();
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
        QMessageBox::warning(this, tr("NebulaX"),
            tr("Cannot read file %1:\n%2.")
            .arg(fileName)
            .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    #ifndef QT_NO_CURSOR
        QApplication::setOverrideCursor(Qt::WaitCursor);
    #endif
        textEdit->setPlainText(in.readAll());
    #ifndef QT_NO_CURSOR
        QApplication::restoreOverrideCursor();
    #endif

    setCurrentFile(fileName);
    //statusBar()->showMessage(tr("File loaded"), 2000);
}

bool MainWindow::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) 
    {
        QMessageBox::warning(this, tr("NebulaX"),
            tr("Cannot write file %1:\n%2.")
            .arg(fileName)
            .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    #ifndef QT_NO_CURSOR
        QApplication::setOverrideCursor(Qt::WaitCursor);
    #endif
        out << textEdit->toPlainText();
    #ifndef QT_NO_CURSOR
        QApplication::restoreOverrideCursor();
    #endif

    setCurrentFile(fileName);
    //statusBar()->showMessage(tr("File saved"), 2000);
    return true;
}



void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile = fileName;
    textEdit->document()->setModified(false);
    setWindowModified(false);

    QString shownName = curFile;
    if (curFile.isEmpty())
        shownName = "untitled.txt";
    setWindowFilePath(shownName);
}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}
