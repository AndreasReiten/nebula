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
    reduced_pixels.set(0, 0);

    // Set stylesheet
    QFile styleFile( ":/src/stylesheets/plain.qss" );
    styleFile.open( QFile::ReadOnly );
    QString style( styleFile.readAll() );
    styleFile.close();
    this->setStyleSheet(style);

    this->initActions();

    this->initMenus();

    this->initGUI();

    this->initWorkers();

    this->initConnects();

    setCentralWidget(mainWidget);
    readSettings();
    print("[Nebula] Welcome to Nebula!");
    setWindowTitle("Nebula[*]");

    // Set start conditions
    setStartConditions();
}

MainWindow::~MainWindow()
{
    voxelizeThread->quit();
    voxelizeThread->wait();
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
    voxelizeThread = new QThread;

    imageOpenGLWidget->setReducedPixels(&reduced_pixels);

    connect(this->activeAngleComboBox, SIGNAL(currentIndexChanged(QString)), imageOpenGLWidget, SLOT(setActiveAngle(QString)));
    connect(this->omegaCorrectionSpinBox, SIGNAL(valueChanged(double)), imageOpenGLWidget, SLOT(setOffsetOmega(double)));
    connect(this->kappaCorrectionSpinBox, SIGNAL(valueChanged(double)), imageOpenGLWidget, SLOT(setOffsetKappa(double)));
    connect(this->phiCorrectionSpinBox, SIGNAL(valueChanged(double)), imageOpenGLWidget, SLOT(setOffsetPhi(double)));
    connect(imageOpenGLWidget, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));

    connect(imageOpenGLWidget, SIGNAL(changedMemoryUsage(int)), memoryUsageProgressBar, SLOT(setValue(int)));
    connect(imageOpenGLWidget, SIGNAL(changedFormatMemoryUsage(QString)), this, SLOT(setMemoryUsageFormat(QString)));
    connect(imageOpenGLWidget, SIGNAL(changedRangeMemoryUsage(int, int)), memoryUsageProgressBar, SLOT(setRange(int, int)));
    connect(imageOpenGLWidget, SIGNAL(showProgressBar(bool)), memoryUsageProgressBar, SLOT(setVisible(bool)));

    connect(reconstructButton, SIGNAL(clicked()), imageOpenGLWidget, SLOT(reconstruct()));
    connect(killButton, SIGNAL(clicked()), imageOpenGLWidget, SLOT(killProcess()), Qt::DirectConnection);


    //### voxelizeWorker ###
    voxelizeWorker = new VoxelizeWorker();
    voxelizeWorker->moveToThread(voxelizeThread);
    voxelizeWorker->setSVOFile(&svo_inprocess);
    voxelizeWorker->setReducedPixels(&reduced_pixels);
    voxelizeWorker->initializeCLKernel();

    voxelizeWorker->moveToThread(voxelizeThread);
    connect(svoLevelSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setCurrentSvoLevel(int)));
    connect(voxelizeThread, SIGNAL(started()), voxelizeWorker, SLOT(process()));
    connect(voxelizeWorker, SIGNAL(finished()), voxelizeThread, SLOT(quit()));
    connect(voxelizeWorker, SIGNAL(popup(QString, QString)), this, SLOT(displayPopup(QString, QString)));
    connect(voxelizeWorker, SIGNAL(changedMemoryUsage(int)), memoryUsageProgressBar, SLOT(setValue(int)));
    connect(voxelizeWorker, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));

    connect(voxelizeWorker, SIGNAL(changedFormatMemoryUsage(QString)), this, SLOT(setMemoryUsageFormat(QString)));
    connect(voxelizeWorker, SIGNAL(changedRangeMemoryUsage(int, int)), memoryUsageProgressBar, SLOT(setRange(int, int)));

    connect(voxelizeWorker, SIGNAL(changedGenericProgress(int)), generalProgressBar, SLOT(setValue(int)));
    connect(voxelizeWorker, SIGNAL(changedFormatGenericProgress(QString)), this, SLOT(setGeneralProgressFormat(QString)));
    connect(voxelizeWorker, SIGNAL(changedRangeGenericProcess(int, int)), generalProgressBar, SLOT(setRange(int, int)));

    connect(voxelizeButton, SIGNAL(clicked()), voxelizeThread, SLOT(start()));
    connect(killButton, SIGNAL(clicked()), voxelizeWorker, SLOT(killProcess()), Qt::DirectConnection);
    connect(imageOpenGLWidget, SIGNAL(qSpaceInfoChanged(float, float, float)), voxelizeWorker, SLOT(setQSpaceInfo(float, float, float)));
}

void MainWindow::loadBrowserPaths()
{
    QMessageBox confirmationMsgBox;

    confirmationMsgBox.setWindowTitle("Nebula");
    confirmationMsgBox.setIcon(QMessageBox::Question);
    confirmationMsgBox.setText("Unsaved changes will be lost.");
    confirmationMsgBox.setInformativeText("Save?");
    confirmationMsgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    confirmationMsgBox.setDefaultButton(QMessageBox::Save);

    int ret = QMessageBox::Discard;

    if (hasPendingChanges)
    {
        ret = confirmationMsgBox.exec();
    }

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
    fileHeaderEditOne->setPlainText(file.getHeaderText());
    fileHeaderEditTwo->setPlainText(file.getHeaderText());
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
        emit setChanged(set);
        imageSpinBox->setRange(0, set.current()->size() - 1);
    }
}


void MainWindow::setStartConditions()
{
    tabWidget->setCurrentIndex(0);

    svoLevelSpinBox->setValue(10);

    volumeRenderDataMinSpinBox->setValue(1.0);
    volumeRenderDataMaxSpinBox->setValue(10);
    volumeRenderAlphaSpinBox->setValue(1.0);
    volumeRenderBrightnessSpinBox->setValue(2.0);
    volumeRenderTsfAlphaComboBox->setCurrentIndex(2);
    volumeRenderViewModeComboBox->setCurrentIndex(1);
    volumeRenderViewModeComboBox->setCurrentIndex(0);
    volumeRenderTsfComboBox->setCurrentIndex(1);
    volumeRenderLogCheckBox->setChecked(true);

    batchSizeSpinBox->setValue(10);

    correctionPlaneCheckBox->setChecked(true);
    correctionPlaneCheckBox->setChecked(false);

    correctionNoiseDoubleSpinBox->setValue(1);
    correctionNoiseDoubleSpinBox->setValue(0);

    selectionModeComboBox->setCurrentIndex(1);
    selectionModeComboBox->setCurrentIndex(0);

    correctionPlaneSpinBox->setValue(10);

    imagePreviewTsfTextureComboBox->setCurrentIndex(1);
    imagePreviewTsfAlphaComboBox->setCurrentIndex(2);
    imagePreviewDataMinDoubleSpinBox->setValue(0.01);
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

void MainWindow::closeEvent(QCloseEvent * event)
{
    writeSettings();
    event->accept();
}

void MainWindow::saveProject()
{
    QString file_name = QFileDialog::getSaveFileName(this, "Save project", working_dir);

    if (file_name != "")
    {
        QFileInfo info(file_name);
        working_dir = info.absoluteDir().path();

        QFile file(file_name);

        if (file.open(QIODevice::WriteOnly))
        {
            QDataStream out(&file);

            out << imageOpenGLWidget->set();
            out << imageModeComboBox->currentText();
            out << imagePreviewTsfTextureComboBox->currentText();
            out << imagePreviewTsfAlphaComboBox->currentText();
            out << (double) imagePreviewDataMinDoubleSpinBox->value();
            out << (double) imagePreviewDataMaxDoubleSpinBox->value();
            out << (bool) imagePreviewLogCheckBox->isChecked();
            out << (bool) correctionLorentzCheckBox->isChecked();
            out << (double) correctionNoiseDoubleSpinBox->value();

            file.close();
        }
    }


    hasPendingChanges = false;
}

void MainWindow::transferSet()
{
    SeriesSet set = imageOpenGLWidget->set();

    emit setPulled(set);
}

void MainWindow::loadProject()
{
    QString file_name = QFileDialog::getOpenFileName(this, "Open project", working_dir);

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

void MainWindow::initActions()
{


    // Actions
    exitAct = new QAction("E&xit program", this);
    aboutAct = new QAction("&About Nebula", this);
    aboutQtAct = new QAction("About &Qt", this);
    aboutOpenCLAct = new QAction("About OpenCL", this);
    aboutOpenGLAct = new QAction("About OpenGL", this);
    openSvoAct = new QAction(QIcon(":/art/open.png"), "Open SVO", this);
    saveSVOAct = new QAction(QIcon(":/art/saveScript.png"), "Save SVO", this);
    saveLoadedSvoAct = new QAction(QIcon(":/art/save.png"), "Save current SVO", this);
    dataStructureAct = new QAction(QIcon(":/art/datastructure.png"), "Toggle data structure", this);
    dataStructureAct->setCheckable(true);
    backgroundAct = new QAction(QIcon(":/art/background.png"), "Toggle background color", this);
    backgroundAct->setCheckable(true);
    projectionAct = new QAction(QIcon(":/art/projection.png"), "Toggle projection", this);
    projectionAct->setCheckable(true);
    projectionAct->setChecked(true);
    screenshotAct = new QAction(QIcon(":/art/screenshot.png"), "&Take screenshot", this);
    scalebarAct = new QAction(QIcon(":/art/scalebar.png"), "&Toggle scalebars", this);
    scalebarAct->setCheckable(true);
    scalebarAct->setChecked(true);
    sliceAct = new QAction(QIcon(":/art/slice.png"), "&Toggle slicing", this);
    sliceAct->setCheckable(true);
    sliceAct->setChecked(false);
    integrate2DAct = new QAction(QIcon(":/art/integrate.png"), "&Toggle 3D->1D integration", this);
    integrate2DAct->setCheckable(true);
    integrate3DAct = new QAction(QIcon(":/art/integrate.png"), "&Toggle 3D->2D integration", this);
    integrate3DAct->setCheckable(true);
    integrate3DAct->setChecked(true);
    logIntegrate2DAct = new QAction(QIcon(":/art/log.png"), "&Toggle logarithmic", this);
    logIntegrate2DAct->setCheckable(true);
    shadowAct = new QAction(QIcon(":/art/shadow.png"), "&Toggle shadows", this);
    shadowAct->setCheckable(true);
    orthoGridAct = new QAction(QIcon(":/art/grid.png"), "&Toggle orthonormal grid", this);
    orthoGridAct->setCheckable(true);

    rulerAct = new QAction(QIcon(":/art/ruler.png"), "&Toggle ruler", this);
    rulerAct->setCheckable(true);

    markAct = new QAction(QIcon(":/art/marker.png"), "&Add marker", this);
    labFrameAct = new QAction(QIcon(":/art/labframe.png"), "&View lab frame", this);
    labFrameAct->setCheckable(true);
    labFrameAct->setChecked(true);

    alignLabXtoSliceXAct = new QAction(QIcon(":/art/align_x.png"), "Align lab frame to slice frame x", this);
    alignLabYtoSliceYAct = new QAction(QIcon(":/art/align_y.png"), "Align lab frame to slice frame y", this);
    alignLabZtoSliceZAct = new QAction(QIcon(":/art/align_z.png"), "Align lab frame to slice frame z", this);
    alignSliceToLabAct = new QAction(QIcon(":/art/align_slice_frame_to_lab_frame"), "Align slice frame to lab frame", this);

    rotateRightAct = new QAction(QIcon(":/art/rotate_right.png"), "Rotate right", this);
    rotateLeftAct = new QAction(QIcon(":/art/rotate_left.png"), "Rotate left", this);
    rotateUpAct = new QAction(QIcon(":/art/rotate_up.png"), "Rotate up", this);
    rotateDownAct = new QAction(QIcon(":/art/rotate_down.png"), "Rotate down", this);
    rollCW = new QAction(QIcon(":/art/roll_cw.png"), "Roll clockwise", this);
    rollCCW = new QAction(QIcon(":/art/roll_ccw.png"), "Roll counterclockwise", this);
    integrateCountsAct = new QAction(QIcon(":/art/integrate_counts.png"), "Integrate intensity in the view box", this);
    integrateCountsAct->setCheckable(true);
    integrateCountsAct->setChecked(false);

    imageScreenshotAct = new QAction(QIcon(":/art/screenshot.png"), "Take screenshot", this);
    imageScreenshotAct->setCheckable(false);

    saveImageAct = new QAction(QIcon(":/art/screenshot.png"), "Save image", this);
    saveImageAct->setCheckable(false);

    // Action Tips
    exitAct->setStatusTip("Exit Nebula");
    aboutAct->setStatusTip("About");
    aboutQtAct->setStatusTip("About Qt");
    aboutOpenCLAct->setStatusTip("About OpenCL");
    aboutOpenGLAct->setStatusTip("About OpenGL");

    // Shortcuts
    exitAct->setShortcuts(QKeySequence::Quit);
}

void MainWindow::about()
{
    QMessageBox::about(this, "About Nebula",
                       "<h1>About Nebula</h1> <b>Nebula</b> is primarily a program to reduce, visualize, and analyze diffuse X-ray scattering. <br> <a href=\"www.github.org/\">github.org</a> <h1>Lisencing (GPL v. 2)</h1> This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program; if not, see <a href=\"https://www.gnu.org/licenses/\">http://www.gnu.org/licenses/</a>. Linking Nebula statically or dynamically with other modules is making a combined work based on Nebula. Thus, the terms and conditions of the GNU General Public License cover the whole combination.");
}

void MainWindow::aboutOpenCL()
{
    QMessageBox::about(this, "About OpenCL",
                       "<h1>About OpenCL</h1> <b>OpenCL</b> is the first open, royalty-free standard for cross-platform, parallel programming of modern processors found in personal computers, servers and handheld/embedded devices. OpenCL (Open Computing Language) greatly improves speed and responsiveness for a wide spectrum of applications in numerous market categories from gaming and entertainment to scientific and medical software. <br> <a href=\"https://www.khronos.org/opencl/\">https://www.khronos.org/opencl</a>");
}

void MainWindow::aboutOpenGL()
{
    QMessageBox::about(this, "About OpenGL",
                       "<h1>About OpenGL</h1> <b>OpenGL</b>  is the most widely adopted 2D and 3D graphics API in the industry, bringing thousands of applications to a wide variety of computer platforms. It is window-system and operating-system independent as well as network-transparent. OpenGL enables developers of software for PC, workstation, and supercomputing hardware to create high-performance, visually compelling graphics software applications, in markets such as CAD, content creation, energy, entertainment, game development, manufacturing, medical, and virtual reality. OpenGL exposes all the features of the latest graphics hardware.<br> <a href=\"https://www.khronos.org/opengl\">www.khronos.org/opengl</a>");
}

void MainWindow::loadUnitcellFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open file");

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

        if (!f.open(QIODevice::ReadOnly))
        {
            qDebug() << "open FAILED";
        }

        QByteArray contents(f.readAll());

        // Find a, b, c, alpha, beta, gamma
        Matrix<float> abc(1, 6);
        int pos = 0;

        for (int i = 0; i < unitcellRegExp.size(); i++)
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
        float alpha = abc[3] * pi / 180;
        float beta = abc[4] * pi / 180;
        float gamma = abc[5] * pi / 180;

        // Set the values in the UI

        QString value;
        value = QString::number( a, 'g', 4 );
        this->a->setText(value);
        value = QString::number( b, 'g', 4 );
        this->b->setText(value);
        value = QString::number( c, 'g', 4 );
        this->c->setText(value);
        value = QString::number( 180 / pi * alpha, 'g', 4 );
        this->alpha->setText(value);
        value = QString::number( 180 / pi * beta, 'g', 4 );
        this->beta->setText(value);
        value = QString::number( 180 / pi * gamma, 'g', 4 );
        this->gamma->setText(value);

        value = QString::number( 1 / a, 'g', 4 );
        this->aStar->setText(value);
        value = QString::number( 1 / b, 'g', 4 );
        this->bStar->setText(value);
        value = QString::number( 1 / c, 'g', 4 );
        this->cStar->setText(value);
        value = QString::number( 180 / pi * std::acos((std::cos(beta) * std::cos(gamma) - std::cos(alpha)) / (std::sin(beta) * std::sin(gamma))), 'g', 4 );
        this->alphaStar->setText(value);
        value = QString::number( 180 / pi * std::acos((std::cos(alpha) * std::cos(gamma) - std::cos(beta)) / (std::sin(alpha) * std::sin(gamma))), 'g', 4 );
        this->betaStar->setText(value);
        value = QString::number( 180 / pi * std::acos((std::cos(alpha) * std::cos(beta) - std::cos(gamma)) / (std::sin(alpha) * std::sin(beta))), 'g', 4 );
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
        Matrix<float> UB(3, 3);
        pos = 0;

        for (int i = 0; i < UBRegExp.size(); i++)
        {
            QRegExp tmp(UBRegExp.at(i));
            pos = tmp.indexIn(contents, pos);

            if (pos > -1)
            {
                pos += tmp.matchedLength();;
                QString value = tmp.cap(1);
                UB[i] = value.toFloat() / wavelength;
            }
        }

        // Math to find U
        float sa = std::sin(alpha);
        float ca = std::cos(alpha);
        float cb = std::cos(beta);
        float cg = std::cos(gamma);
        float V = (a * b * c) * std::sqrt(1.0 - ca * ca - cb * cb - cg * cg + 2.0 * ca * cb * cg);

        Matrix<float> B(3, 3);
        B[0] = b * c * sa / V;
        B[1] = a * c * (ca * cb - cg) / (V * sa);
        B[2] = a * b * (ca * cg - cb) / (V * sa);
        B[3] = 0;
        B[4] = 1.0 / (b * sa);
        B[5] = -ca / (c * sa);
        B[6] = 0;
        B[7] = 0;
        B[8] = 1.0 / c;



        Matrix<float> U(3, 3);
        U = UB * B.inverse();
    }
}



void MainWindow::setTab(int tab)
{
}


void MainWindow::initConnects()
{
    /* this <-> volumeRenderWidget */
    connect(this->qualitySlider, SIGNAL(valueChanged(int)), volumeOpenGLWidget, SLOT(setQuality(int)));
    connect(this->qualitySlider, SIGNAL(sliderReleased()), volumeOpenGLWidget, SLOT(refreshTexture()));
    connect(this->scalebarAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(setScalebar()));
    connect(this->sliceAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(setSlicing()));
    connect(this->integrate2DAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(setIntegration2D()));
    connect(this->integrate3DAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(setIntegration3D()));
    connect(this->shadowAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(setShadow()));
    connect(this->orthoGridAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(setOrthoGrid()));
    connect(this->projectionAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(setProjection()));
    connect(this->backgroundAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(setBackground()));
    connect(this->logIntegrate2DAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(setLogarithmic2D()));
    connect(this->dataStructureAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(setDataStructure()));
    connect(this->volumeRenderTsfComboBox, SIGNAL(currentIndexChanged(int)), volumeOpenGLWidget, SLOT(setTsfColor(int)));
    connect(this->volumeRenderViewModeComboBox, SIGNAL(currentIndexChanged(int)), volumeOpenGLWidget, SLOT(setViewMode(int)));
    connect(this->volumeRenderTsfAlphaComboBox, SIGNAL(currentIndexChanged(int)), volumeOpenGLWidget, SLOT(setTsfAlpha(int)));
    connect(this->volumeRenderDataMinSpinBox, SIGNAL(valueChanged(double)), volumeOpenGLWidget, SLOT(setDataMin(double)));
    connect(this->volumeRenderDataMaxSpinBox, SIGNAL(valueChanged(double)), volumeOpenGLWidget, SLOT(setDataMax(double)));
    connect(this->volumeRenderAlphaSpinBox, SIGNAL(valueChanged(double)), volumeOpenGLWidget, SLOT(setAlpha(double)));
    connect(this->volumeRenderBrightnessSpinBox, SIGNAL(valueChanged(double)), volumeOpenGLWidget, SLOT(setBrightness(double)));
    connect(this->functionToggleButton, SIGNAL(clicked()), volumeOpenGLWidget, SLOT(setModel()));
    connect(this->funcParamASpinBox, SIGNAL(valueChanged(double)), volumeOpenGLWidget, SLOT(setModelParam0(double)));
    connect(this->funcParamBSpinBox, SIGNAL(valueChanged(double)), volumeOpenGLWidget, SLOT(setModelParam1(double)));
    connect(this->funcParamCSpinBox, SIGNAL(valueChanged(double)), volumeOpenGLWidget, SLOT(setModelParam2(double)));
    connect(this->funcParamDSpinBox, SIGNAL(valueChanged(double)), volumeOpenGLWidget, SLOT(setModelParam3(double)));
    connect(volumeOpenGLWidget, SIGNAL(changedMessageString(QString)), this, SLOT(print(QString)));
    connect(this, SIGNAL(captureFrameBuffer(QString)), volumeOpenGLWidget, SLOT(takeScreenShot(QString)));
    connect(this->alignLabXtoSliceXAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(alignLabXtoSliceX()));
    connect(this->alignLabYtoSliceYAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(alignLabYtoSliceY()));
    connect(this->alignLabZtoSliceZAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(alignLabZtoSliceZ()));
    connect(this->alignSliceToLabAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(alignSliceToLab()));
    connect(this->rotateLeftAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(rotateLeft()));
    connect(this->rotateRightAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(rotateRight()));
    connect(this->rotateUpAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(rotateUp()));
    connect(this->rotateDownAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(rotateDown()));
    connect(this->rollCW, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(rollCW()));
    connect(this->rollCCW, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(rollCCW()));
    connect(this->rulerAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(toggleRuler()));
    connect(this->markAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(addMarker()));
    connect(this->labFrameAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(setLabFrame()));
    connect(this->toggleHklButton, SIGNAL(clicked()), volumeOpenGLWidget, SLOT(toggleHkl()));
    connect(this->toggleCellButton, SIGNAL(clicked()), volumeOpenGLWidget, SLOT(setUnitcell()));
    connect(this->hSpinBox, SIGNAL(valueChanged(int)), volumeOpenGLWidget, SLOT(setHCurrent(int)));
    connect(this->kSpinBox, SIGNAL(valueChanged(int)), volumeOpenGLWidget, SLOT(setKCurrent(int)));
    connect(this->lSpinBox, SIGNAL(valueChanged(int)), volumeOpenGLWidget, SLOT(setLCurrent(int)));
    connect(this->integrateCountsAct, SIGNAL(triggered()), volumeOpenGLWidget, SLOT(setCountIntegration()));
    connect(this->helpCellOverlayButton, SIGNAL(toggled(bool)), volumeOpenGLWidget, SLOT(setMiniCell(bool)));
    connect(this->rotateCellButton, SIGNAL(toggled(bool)), volumeOpenGLWidget, SLOT(setURotation(bool)));

    connect(insertLinePushButton, SIGNAL(clicked()), volumeOpenGLWidget, SLOT(addLine()));
    connect(removeLinePushButton, SIGNAL(clicked()), lineModel, SLOT(removeMarkedLine()));
    connect(volumeOpenGLWidget, SIGNAL(linesChanged()), lineModel, SLOT(refresh()));

    //    /* this <-> this */
    connect(this->aNormSpinBox, SIGNAL(valueChanged(double)), volumeOpenGLWidget, SLOT(setUB_a(double)));
    connect(this->bNormSpinBox, SIGNAL(valueChanged(double)), volumeOpenGLWidget, SLOT(setUB_b(double)));
    connect(this->cNormSpinBox, SIGNAL(valueChanged(double)), volumeOpenGLWidget, SLOT(setUB_c(double)));
    connect(this->alphaNormSpinBox, SIGNAL(valueChanged(double)), volumeOpenGLWidget, SLOT(setUB_alpha(double)));
    connect(this->betaNormSpinBox, SIGNAL(valueChanged(double)), volumeOpenGLWidget, SLOT(setUB_beta(double)));
    connect(this->gammaNormSpinBox, SIGNAL(valueChanged(double)), volumeOpenGLWidget, SLOT(setUB_gamma(double)));
    connect(this->screenshotAct, SIGNAL(triggered()), this, SLOT(takeVolumeScreenshot()));
    connect(openSvoAct, SIGNAL(triggered()), this, SLOT(loadSvo()));
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
    connect(fileTreeView, SIGNAL(fileChanged(QString)), this, SLOT(setHeader(QString)));
    connect(imageOpenGLWidget->worker(), SIGNAL(pathChanged(QString)), this, SLOT(setHeader(QString)));
    connect(imageOpenGLWidget->worker(), SIGNAL(pathChanged(QString)), this, SLOT(setGeneralProgressFormat(QString)));
    connect(imageOpenGLWidget->worker(), SIGNAL(progressRangeChanged(int, int)), generalProgressBar, SLOT(setRange(int, int)));
    connect(imageOpenGLWidget->worker(), SIGNAL(progressChanged(int)), generalProgressBar, SLOT(setValue(int)));

    // KK
    connect(loadPathsPushButton, SIGNAL(clicked()), this, SLOT(loadBrowserPaths()));
    connect(batchSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setBatchSize(int)));
    connect(correctionLorentzCheckBox, SIGNAL(toggled(bool)), imageOpenGLWidget, SLOT(setCorrectionLorentz(bool)));
    connect(imageModeComboBox, SIGNAL(currentIndexChanged(int)), imageOpenGLWidget, SLOT(setMode(int)));
    connect(saveProjectAction, SIGNAL(triggered()), this, SLOT(saveProject()));
    connect(loadProjectAction, SIGNAL(triggered()), this, SLOT(loadProject()));
    connect(nextFramePushButton, SIGNAL(clicked()), this, SLOT(nextFrame()));
    connect(previousFramePushButton, SIGNAL(clicked()), this, SLOT(previousFrame()));
    connect(batchForwardPushButton, SIGNAL(clicked()), this, SLOT(batchForward()));
    connect(batchBackwardPushButton, SIGNAL(clicked()), this, SLOT(batchBackward()));
    connect(nextSeriesPushButton, SIGNAL(clicked()), imageOpenGLWidget, SLOT(nextSeries()));
    connect(prevSeriesPushButton, SIGNAL(clicked()), imageOpenGLWidget, SLOT(prevSeries()));
    connect(removeCurrentPushButton, SIGNAL(clicked()), imageOpenGLWidget, SLOT(removeCurrentImage()));
    connect(imageOpenGLWidget, SIGNAL(pathRemoved(QString)), fileSelectionModel, SLOT(removeFile(QString)));
    connect(this, SIGNAL(setSelection(QString)), imageOpenGLWidget, SLOT(applySelection(QString)));
    connect(this, SIGNAL(setPlaneMarkers(QString)), imageOpenGLWidget, SLOT(applyPlaneMarker(QString)));
    connect(this, SIGNAL(analyze(QString)), imageOpenGLWidget, SLOT(analyze(QString)));
    connect(imageOpenGLWidget, SIGNAL(pathChanged(QString)), this, SLOT(setHeader(QString)));
    connect(imageOpenGLWidget, SIGNAL(pathChanged(QString)), this, SLOT(setGeneralProgressFormat(QString)));
    connect(imageSpinBox, SIGNAL(valueChanged(int)), imageOpenGLWidget, SLOT(setFrameByIndex(int)));
    connect(imageOpenGLWidget, SIGNAL(imageRangeChanged(int, int)), this, SLOT(setImageRange(int, int)));
    connect(imageOpenGLWidget, SIGNAL(currentIndexChanged(int)), imageSpinBox, SLOT(setValue(int)));
    connect(this, SIGNAL(setChanged(SeriesSet)), imageOpenGLWidget, SLOT(setSet(SeriesSet)));
    connect(traceSeriesPushButton, SIGNAL(clicked()), imageOpenGLWidget, SLOT(traceSeriesSlot()));
    connect(correctionPlaneSpinBox, SIGNAL(valueChanged(int)), imageOpenGLWidget, SLOT(setLsqSamples(int)));
    connect(traceTextureCheckBox, SIGNAL(toggled(bool)), imageOpenGLWidget, SLOT(toggleTraceTexture(bool)));
    connect(imageOpenGLWidget, SIGNAL(progressChanged(int)), generalProgressBar, SLOT(setValue(int)));
    connect(imageOpenGLWidget, SIGNAL(progressRangeChanged(int, int)), generalProgressBar, SLOT(setRange(int, int)));
    connect(correctionNoiseCheckBox, SIGNAL(toggled(bool)), imageOpenGLWidget, SLOT(setCorrectionNoise(bool)));
    connect(correctionPlaneCheckBox, SIGNAL(toggled(bool)), imageOpenGLWidget, SLOT(setCorrectionPlane(bool)));
    connect(correctionClutterCheckBox, SIGNAL(toggled(bool)), imageOpenGLWidget, SLOT(setCorrectionClutter(bool)));
    connect(correctionMedianCheckBox, SIGNAL(toggled(bool)), imageOpenGLWidget, SLOT(setCorrectionMedian(bool)));
    connect(correctionPolarizationCheckBox, SIGNAL(toggled(bool)), imageOpenGLWidget, SLOT(setCorrectionPolarization(bool)));
    connect(correctionFluxCheckBox, SIGNAL(toggled(bool)), imageOpenGLWidget, SLOT(setCorrectionFlux(bool)));
    connect(correctionExposureCheckBox, SIGNAL(toggled(bool)), imageOpenGLWidget, SLOT(setCorrectionExposure(bool)));
    connect(centerImageAction, SIGNAL(triggered()), imageOpenGLWidget, SLOT(centerImage()));
    connect(showWeightCenterAction, SIGNAL(toggled(bool)), imageOpenGLWidget, SLOT(showWeightCenter(bool)));
    connect(integratePushButton, SIGNAL(clicked()), this, SLOT(applyAnalytics()));
    connect(applyPlaneMarkerPushButton, SIGNAL(clicked()), this, SLOT(applyPlaneMarker()));
    connect(applySelectionPushButton, SIGNAL(clicked()), this, SLOT(applySelection()));
    connect(selectionModeComboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(setApplyMode(QString)));
    connect(correctionNoiseDoubleSpinBox, SIGNAL(valueChanged(double)), imageOpenGLWidget, SLOT(setNoise(double)));
    connect(imageOpenGLWidget, SIGNAL(noiseLowChanged(double)), correctionNoiseDoubleSpinBox, SLOT(setValue(double)));
    connect(saveImageAct, SIGNAL(triggered()), this, SLOT(saveImageFunction()));
    connect(this, SIGNAL(saveImage(QString)), imageOpenGLWidget, SLOT(saveImage(QString)));
    connect(imageScreenshotAct, SIGNAL(triggered()), this, SLOT(takeImageScreenshotFunction()));
    connect(this, SIGNAL(takeImageScreenshot(QString)), imageOpenGLWidget, SLOT(takeScreenShot(QString)));
    connect(imageOpenGLWidget, SIGNAL(resultFinished(QString)), outputPlainTextEdit, SLOT(setPlainText(QString)));
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(setTab(int)));
    connect(voxelizeWorker, SIGNAL(showProgressBar(bool)), memoryUsageProgressBar, SLOT(setVisible(bool)));
    connect(beamOverrideCheckBox, SIGNAL(toggled(bool)), imageOpenGLWidget, SLOT(setBeamOverrideActive(bool)));
    connect(beamXOverrideSpinBox, SIGNAL(valueChanged(double)), imageOpenGLWidget, SLOT(setBeamXOverride(double)));
    connect(beamYOverrideSpinBox, SIGNAL(valueChanged(double)), imageOpenGLWidget, SLOT(setBeamYOverride(double)));

    connect(lineView, SIGNAL(doubleClicked(QModelIndex)), lineModel, SLOT(highlight(QModelIndex)));
    connect(lineView, SIGNAL(doubleClicked(QModelIndex)), volumeOpenGLWidget, SLOT(update()));
    connect(lineModel, SIGNAL(lineChanged(int)), volumeOpenGLWidget, SLOT(refreshLine(int)));
    connect(lineModel, SIGNAL(lineChanged(int)), volumeOpenGLWidget, SLOT(update()));
    connect(lineView, SIGNAL(clicked(QModelIndex)), lineView, SLOT(setCurrentIndex(QModelIndex)));
    connect(lineView, SIGNAL(clicked(QModelIndex)), lineView, SLOT(edit(QModelIndex)));
    connect(volumeOpenGLWidget->worker(), SIGNAL(lineIntegralResolved()), this, SLOT(setLineIntegralPlot()));
}

void MainWindow::setGeneralProgressFormat(QString str)
{
    generalProgressBar->setFormat(str);
}

void MainWindow::setMemoryUsageFormat(QString str)
{
    memoryUsageProgressBar->setFormat(str);
}

void MainWindow::saveSvo()
{
    if (svo_inprocess.index()->size() > 0)
    {
        QString file_name = QFileDialog::getSaveFileName(this, "Save file", working_dir);

        if (file_name != "")
        {
            QFileInfo info(file_name);
            working_dir = info.absoluteDir().path();

            // View settings
            svo_inprocess.setViewMode(0);
            svo_inprocess.setViewTsfStyle(2);
            svo_inprocess.setViewTsfTexture(1);
            svo_inprocess.setViewDataMin(0);
            svo_inprocess.setViewDataMax(100);
            svo_inprocess.setViewAlpha(0.05);
            svo_inprocess.setViewBrightness(2.0);

            svo_inprocess.save(file_name);
        }
    }
}


void MainWindow::saveLoadedSvo()
{
    if (svo_loaded.brickNumber() > 0)
    {
        QString file_name = QFileDialog::getSaveFileName(this, "Save file", working_dir);

        if (file_name != "")
        {
            QFileInfo info(file_name);
            working_dir = info.absoluteDir().path();

            // View settings
            svo_loaded.setViewMode(volumeRenderViewModeComboBox->currentIndex());
            svo_loaded.setViewTsfStyle(imagePreviewTsfAlphaComboBox->currentIndex());
            svo_loaded.setViewTsfTexture(volumeRenderTsfComboBox->currentIndex());
            svo_loaded.setViewDataMin(volumeRenderDataMinSpinBox->value());
            svo_loaded.setViewDataMax(volumeRenderDataMaxSpinBox->value());
            svo_loaded.setViewAlpha(volumeRenderAlphaSpinBox->value());
            svo_loaded.setViewBrightness(volumeRenderBrightnessSpinBox->value());

            svo_loaded.setUB(volumeOpenGLWidget->getUBMatrix());
            svo_loaded.setMetaData(svoHeaderEdit->toPlainText());
            svo_loaded.save(file_name);
        }
    }
}


void MainWindow::loadSvo()
{
    QString file_name = QFileDialog::getOpenFileName(this, "Open file", working_dir);

    if ((file_name != ""))
    {
        QFileInfo info(file_name);
        working_dir = info.absoluteDir().path();

        svo_loaded.open(file_name);
        volumeOpenGLWidget->setSvo(&(svo_loaded));
        lineModel->setLines(svo_loaded.lines());

        volumeRenderViewModeComboBox->setCurrentIndex(svo_loaded.viewMode());
        imagePreviewTsfAlphaComboBox->setCurrentIndex(svo_loaded.viewTsfStyle());
        volumeRenderTsfComboBox->setCurrentIndex(svo_loaded.viewTsfTexture());
        volumeRenderAlphaSpinBox->setValue(svo_loaded.viewAlpha());
        volumeRenderBrightnessSpinBox->setValue(svo_loaded.viewBrightness());
        volumeRenderDataMinSpinBox->setValue(svo_loaded.viewDataMin());
        volumeRenderDataMaxSpinBox->setValue(svo_loaded.viewDataMax());

        qDebug() << svo_loaded.lines()->size();

        UBMatrix<double> UB;

        UB = svo_loaded.UB();

        if (UB.size() == 3 * 3)
        {
            volumeOpenGLWidget->setUBMatrix(UB);

            alphaNormSpinBox->setValue(UB.alpha() * 180.0 / pi);
            betaNormSpinBox->setValue(UB.beta() * 180.0 / pi);
            gammaNormSpinBox->setValue(UB.gamma() * 180.0 / pi);

            aNormSpinBox->setValue(UB.a());
            bNormSpinBox->setValue(UB.b());
            cNormSpinBox->setValue(UB.c());
        }

        svoHeaderEdit->setDocumentTitle(file_name);
        svoHeaderEdit->setPlainText(svo_loaded.metaData());

        print("\n[" + QString(this->metaObject()->className()) + "] Loaded file: \"" + file_name + "\"");

        setWindowTitle("Nebula[*] (" + file_name + ")");
    }
}



void MainWindow::initMenus()
{
    mainMenu = new QMenuBar;
    viewMenu = new QMenu("V&iew");
    helpMenu = new QMenu("&Help");

    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
    helpMenu->addAction(aboutOpenCLAct);
    helpMenu->addAction(aboutOpenGLAct);

    mainMenu->addMenu(viewMenu);
    mainMenu->addSeparator();
    mainMenu->addMenu(helpMenu);

    this->setMenuBar(mainMenu);
}



void MainWindow::initGUI()
{
    /*      File Select Widget       */
    {
        setFilesWidget = new QWidget;

        // Toolbar
        fileFilter = new QLineEdit;

        // File browser
        fileSelectionModel  = new FileSelectionModel;
        fileSelectionModel->setRootPath(QDir::rootPath());

        fileTreeView = new FileTreeView;
        fileTreeView->setModel(fileSelectionModel);

        loadPathsPushButton = new QPushButton;//(QIcon(":/art/download.png"),"Load selected files"); //QIcon(":/art/rotate_down.png"),
        loadPathsPushButton->setIcon(QIcon(":/art/download.png"));
        loadPathsPushButton->setIconSize(QSize(86, 86));

        // Layout
        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->addWidget(fileFilter, 0, 0, 1, 2);
        gridLayout->addWidget(fileTreeView, 2, 0, 1, 2);
        gridLayout->addWidget(loadPathsPushButton, 3, 0, 1, 2);

        setFilesWidget->setLayout(gridLayout);

        fileHeaderEditOne = new QPlainTextEdit;
        headerHighlighterOne = new Highlighter(fileHeaderEditOne->document());
        fileHeaderEditOne->setReadOnly(true);

        fileHeaderDockOne = new QDockWidget("Frame header info", this);
        fileHeaderDockOne->setWidget(fileHeaderEditOne);

        browserMainWindow = new QMainWindow;
        browserMainWindow->setCentralWidget(setFilesWidget);
        browserMainWindow->addDockWidget(Qt::RightDockWidgetArea, fileHeaderDockOne);
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

        volumeOpenGLWidget = new VolumeOpenGLWidget();
        volumeOpenGLWidget->setFormat(format_gl);
        volumeOpenGLWidget->setMouseTracking(true);


        // Toolbar
        viewToolBar = new QToolBar("3D view");
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
        volumeRenderMainWindow->setCentralWidget(volumeOpenGLWidget);
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

        imageOpenGLWidget = new ImageOpenGLWidget();

        imageOpenGLWidget->setFormat(format_gl);
        imageOpenGLWidget->setMouseTracking(true);

        reconstructionMainWindow = new QMainWindow;
        reconstructionMainWindow->setAnimated(false);

        saveProjectAction = new QAction(QIcon(":/art/save.png"), "Save project", this);
        loadProjectAction = new QAction(QIcon(":/art/open.png"), "Load project", this);

        squareAreaSelectAlphaAction = new QAction(QIcon(":/art/select.png"), "Toggle pixel selection", this);
        squareAreaSelectAlphaAction->setCheckable(true);
        squareAreaSelectAlphaAction->setChecked(false);

        squareAreaSelectBetaAction = new QAction(QIcon(":/art/select2.png"), "Toggle background selection", this);
        squareAreaSelectBetaAction->setCheckable(true);
        squareAreaSelectBetaAction->setChecked(false);

        centerImageAction = new QAction(QIcon(":/art/center.png"), "Center image", this);
        centerImageAction->setCheckable(false);

        showWeightCenterAction = new QAction(QIcon(":/art/weight_center.png"), "Toggle weight center visual", this);
        showWeightCenterAction->setCheckable(true);
        showWeightCenterAction->setChecked(false);

        imageToolBar = new QToolBar("Image");
        imageToolBar->addAction(saveProjectAction);
        imageToolBar->addAction(loadProjectAction);
        imageToolBar->addAction(centerImageAction);
        imageToolBar->addAction(showWeightCenterAction);
        imageToolBar->addSeparator();
        imageToolBar->addAction(imageScreenshotAct);
        imageToolBar->addAction(saveImageAct);

        connect(imageOpenGLWidget, SIGNAL(selectionAlphaChanged(bool)), squareAreaSelectAlphaAction, SLOT(setChecked(bool)));
        connect(imageOpenGLWidget, SIGNAL(selectionBetaChanged(bool)), squareAreaSelectBetaAction, SLOT(setChecked(bool)));

        reconstructionMainWindow->addToolBar(Qt::TopToolBarArea, imageToolBar);

        reconstructionMainWindow->setCentralWidget(imageOpenGLWidget);
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
        imagePreviewDataMinDoubleSpinBox->setRange(-1e9, 1e9);
        imagePreviewDataMinDoubleSpinBox->setAccelerated(true);
        imagePreviewDataMinDoubleSpinBox->setPrefix("Data min: ");

        imagePreviewDataMaxDoubleSpinBox = new QDoubleSpinBox;
        imagePreviewDataMaxDoubleSpinBox->setRange(-1e9, 1e9);
        imagePreviewDataMaxDoubleSpinBox->setAccelerated(true);
        imagePreviewDataMaxDoubleSpinBox->setPrefix("Data max: ");

        imagePreviewLogCheckBox = new QCheckBox("Log");

        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->setRowStretch(5, 1);
        gridLayout->addWidget(imageModeComboBox, 0, 1, 1, 2);
        gridLayout->addWidget(imagePreviewTsfTextureComboBox, 1, 1, 1, 1);
        gridLayout->addWidget(imagePreviewTsfAlphaComboBox, 1, 2, 1, 1);
        gridLayout->addWidget(imagePreviewDataMinDoubleSpinBox, 2, 1, 1, 2);
        gridLayout->addWidget(imagePreviewDataMaxDoubleSpinBox, 3, 1, 1, 2);
        gridLayout->addWidget(imagePreviewLogCheckBox, 4, 1, 1, 1);

        imageSettingsWidget = new QWidget;
        imageSettingsWidget->setLayout(gridLayout);

        imageSettingsDock =  new QDockWidget("Display settings");
        imageSettingsDock->setWidget(imageSettingsWidget);
        reconstructionMainWindow->addDockWidget(Qt::LeftDockWidgetArea, imageSettingsDock);

        connect(imagePreviewTsfTextureComboBox, SIGNAL(currentIndexChanged(int)), imageOpenGLWidget, SLOT(setTsfTexture(int)), Qt::QueuedConnection);
        connect(imagePreviewTsfAlphaComboBox, SIGNAL(currentIndexChanged(int)), imageOpenGLWidget, SLOT(setTsfAlpha(int)));
        connect(imagePreviewDataMinDoubleSpinBox, SIGNAL(valueChanged(double)), imageOpenGLWidget, SLOT(setDataMin(double)));
        connect(imagePreviewDataMaxDoubleSpinBox, SIGNAL(valueChanged(double)), imageOpenGLWidget, SLOT(setDataMax(double)));
        connect(imagePreviewLogCheckBox, SIGNAL(toggled(bool)), imageOpenGLWidget, SLOT(setLog(bool)));
        connect(this, SIGNAL(centerImage()), imageOpenGLWidget, SLOT(centerImage()));

    }

    // Navigation dock widget
    {
        nextFramePushButton = new QPushButton(QIcon(":/art/forward.png"), "Next");
        previousFramePushButton = new QPushButton(QIcon(":/art/back.png"), "Prev");
        batchForwardPushButton = new QPushButton(QIcon(":/art/fast_forward.png"), "Skip");
        batchBackwardPushButton = new QPushButton(QIcon(":/art/fast_back.png"), "Skip");
        nextSeriesPushButton = new QPushButton(QIcon(":/art/next_series.png"), "Next series");
        nextSeriesPushButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        prevSeriesPushButton = new QPushButton(QIcon(":/art/prev_series.png"), "Prev series");
        prevSeriesPushButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

        removeCurrentPushButton = new QPushButton(QIcon(":/art/kill.png"), "Remove frame");

        imageSpinBox = new QSpinBox;
        imageSpinBox->setPrefix("Frame: ");

        batchSizeSpinBox = new QSpinBox;
        batchSizeSpinBox->setPrefix("Skip size: ");

        generalProgressBar = new QProgressBar;

        memoryUsageProgressBar = new QProgressBar;
        memoryUsageProgressBar->setRange( 0, 1);
        memoryUsageProgressBar->setVisible(false);

        QGridLayout * gridLayout = new QGridLayout;

        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->setRowStretch(5, 1);
        gridLayout->addWidget(batchBackwardPushButton, 0, 0, 1, 1);
        gridLayout->addWidget(previousFramePushButton, 0, 1, 1, 1);
        gridLayout->addWidget(imageSpinBox, 0, 2, 1, 2);
        gridLayout->addWidget(nextFramePushButton, 0, 4, 1, 1);
        gridLayout->addWidget(batchForwardPushButton, 0, 5, 1, 1);
        gridLayout->addWidget(prevSeriesPushButton, 1, 0, 2, 2);
        gridLayout->addWidget(batchSizeSpinBox, 1, 2, 1, 2);
        gridLayout->addWidget(nextSeriesPushButton, 1, 4, 2, 2);
        gridLayout->addWidget(removeCurrentPushButton, 2, 2, 1 , 2);
        gridLayout->addWidget(generalProgressBar, 3, 0, 1 , 6);
        gridLayout->addWidget(memoryUsageProgressBar, 4, 0, 1 , 6);

        navigationWidget = new QWidget;
        navigationWidget->setLayout(gridLayout);

        navigationDock =  new QDockWidget("Navigation");
        navigationDock->setWidget(navigationWidget);
        reconstructionMainWindow->addDockWidget(Qt::BottomDockWidgetArea, navigationDock);
    }

    // Operations dock widget
    {
        applyPlaneMarkerPushButton  = new QPushButton(QIcon(":/art/lsqplane.png"), "Apply markers");
        applySelectionPushButton  = new QPushButton(QIcon(":/art/select.png"), "Apply selection");
        integratePushButton = new QPushButton(QIcon(":/art/proceed.png"), "Analyze frames");

        selectionModeComboBox = new QComboBox;
        selectionModeComboBox->addItem("Series");
        selectionModeComboBox->addItem("Set");

        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->setRowStretch(4, 1);
        gridLayout->addWidget(selectionModeComboBox, 0, 0, 1, 2);
        gridLayout->addWidget(applyPlaneMarkerPushButton, 1, 0, 1 , 2);
        gridLayout->addWidget(applySelectionPushButton, 2, 0, 1 , 2);
        gridLayout->addWidget(integratePushButton, 3, 0, 1 , 2);

        selectionWidget = new QWidget;
        selectionWidget->setLayout(gridLayout);

        selectionDock =  new QDockWidget("Frame-by-frame operations");
        selectionDock->setWidget(selectionWidget);
        reconstructionMainWindow->addDockWidget(Qt::RightDockWidgetArea, selectionDock);
    }

    // Corrections dock widget
    {
        traceSeriesPushButton = new QPushButton("Generate trace");
        traceTextureCheckBox = new QCheckBox("Show");

        correctionNoiseDoubleSpinBox = new QDoubleSpinBox;
        correctionNoiseDoubleSpinBox->setRange(0, 1e4);

        correctionClutterSpinBox = new QSpinBox;
        correctionClutterSpinBox->setRange(0, 100);
        correctionClutterSpinBox->setSuffix(" units");

        correctionMedianSpinBox = new QSpinBox;
        correctionMedianSpinBox->setRange(0, 100);
        correctionMedianSpinBox->setPrefix("n x n: ");

        correctionPlaneSpinBox = new QSpinBox;
        correctionPlaneSpinBox->setRange(3, 20);
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
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->setRowStretch(9, 1);
        gridLayout->addWidget(traceSeriesPushButton, 0, 0, 1, 1);
        gridLayout->addWidget(traceTextureCheckBox, 0, 1, 1, 1);
        gridLayout->addWidget(correctionPlaneCheckBox, 1, 0, 1, 1);
        gridLayout->addWidget(correctionPlaneSpinBox, 1, 1, 1, 1);
        gridLayout->addWidget(correctionNoiseCheckBox, 2, 0, 1, 1);
        gridLayout->addWidget(correctionNoiseDoubleSpinBox, 2, 1, 1, 1);
        gridLayout->addWidget(correctionClutterCheckBox, 3, 0, 1, 1);
        gridLayout->addWidget(correctionClutterSpinBox, 3, 1, 1, 1);
        gridLayout->addWidget(correctionMedianCheckBox, 4, 0, 1, 1);
        gridLayout->addWidget(correctionMedianSpinBox, 4, 1, 1, 1);
        gridLayout->addWidget(correctionLorentzCheckBox, 5, 0, 1, 2);
        gridLayout->addWidget(correctionPolarizationCheckBox, 6, 0, 1, 2);
        gridLayout->addWidget(correctionFluxCheckBox, 7, 0, 1, 2);
        gridLayout->addWidget(correctionExposureCheckBox, 8, 0, 1, 2);


        correctionWidget = new QWidget;
        correctionWidget->setLayout(gridLayout);

        correctionDock =  new QDockWidget("Frame-by-frame corrections");
        correctionDock->setWidget(correctionWidget);
        reconstructionMainWindow->addDockWidget(Qt::LeftDockWidgetArea, correctionDock);
    }




    /* Graphics dock widget */
    {
        QLabel * label_texture = new QLabel(QString("Texture "));
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
        connect(volumeRenderLogCheckBox, SIGNAL(toggled(bool)), volumeOpenGLWidget, SLOT(setLog(bool)));


        qualitySlider = new QSlider(Qt::Horizontal);
        qualitySlider->setRange(1, 100);
        qualitySlider->setToolTip("Set texture resolution");
        qualitySlider->setTickPosition(QSlider::NoTicks);

        graphicsDockWidget = new QDockWidget("Display settings", this);
        graphicsWidget = new QWidget;

        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->setRowStretch(8, 1);
        gridLayout->addWidget(label_mode, 0, 0, 1, 2);
        gridLayout->addWidget(volumeRenderViewModeComboBox, 0, 2, 1, 2);
        gridLayout->addWidget(label_texture, 1, 0, 1, 2);
        gridLayout->addWidget(volumeRenderTsfComboBox, 1, 2, 1, 1);
        gridLayout->addWidget(volumeRenderTsfAlphaComboBox, 1, 3, 1, 1);
        gridLayout->addWidget(volumeRenderDataMinSpinBox, 2, 0, 1, 4);
        gridLayout->addWidget(volumeRenderDataMaxSpinBox, 3, 0, 1, 4);
        gridLayout->addWidget(volumeRenderAlphaSpinBox, 4, 0, 1, 4);
        gridLayout->addWidget(volumeRenderBrightnessSpinBox, 5, 0, 1, 4);
        gridLayout->addWidget(label_quality, 6, 0, 1, 2);
        gridLayout->addWidget(qualitySlider, 6, 2, 1, 2);
        gridLayout->addWidget(volumeRenderLogCheckBox, 7, 0, 1, 2);

        graphicsWidget->setLayout(gridLayout);
        //        graphicsDockWidget->setFixedHeight(graphicsWidget->minimumSizeHint().height()*1.1);
        graphicsDockWidget->setWidget(graphicsWidget);
        viewMenu->addAction(graphicsDockWidget->toggleViewAction());
        volumeRenderMainWindow->addDockWidget(Qt::LeftDockWidgetArea, graphicsDockWidget);
    }

    /* PlotWidget */
    {
        plotDockWidget = new QDockWidget("Plot");

        plotWidget = new PlotWidget;

        plotDockWidget->setWidget(plotWidget);
        viewMenu->addAction(plotDockWidget->toggleViewAction());
        volumeRenderMainWindow->addDockWidget(Qt::LeftDockWidgetArea, plotDockWidget);
    }

    /* Unitcell dock widget */
    {
        unitCellDockWidget = new QDockWidget("UB matrix", this);
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
        alphaNormSpinBox->setRange(0, 180);
        alphaNormSpinBox->setSuffix(" °");
        betaNormSpinBox = new QDoubleSpinBox;
        betaNormSpinBox->setPrefix("β: ");
        betaNormSpinBox->setRange(0, 180);
        betaNormSpinBox->setSuffix(" °");
        gammaNormSpinBox = new QDoubleSpinBox;
        gammaNormSpinBox->setPrefix("γ: ");
        gammaNormSpinBox->setRange(0, 180);
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
        hSpinBox->setRange(-1e3, 1e3);
        hSpinBox->setPrefix("h: ");
        kSpinBox = new QSpinBox;
        kSpinBox->setRange(-1e3, 1e3);
        kSpinBox->setPrefix("k: ");
        lSpinBox = new QSpinBox;
        lSpinBox->setRange(-1e3, 1e3);
        lSpinBox->setPrefix("l: ");

        alignAlongAStarButton = new QPushButton("Align a*");
        alignAlongBStarButton = new QPushButton("Align b*");
        alignAlongCStarButton = new QPushButton("Align c*");

        connect(alignAlongAStarButton, SIGNAL(clicked()), volumeOpenGLWidget, SLOT(alignAStartoZ()));
        connect(alignAlongBStarButton, SIGNAL(clicked()), volumeOpenGLWidget, SLOT(alignBStartoZ()));
        connect(alignAlongCStarButton, SIGNAL(clicked()), volumeOpenGLWidget, SLOT(alignCStartoZ()));

        alignSlicetoAStarPushButton = new QPushButton("Slice a*");
        alignSlicetoBStarPushButton = new QPushButton("Slice b*");
        alignSlicetoCStarPushButton = new QPushButton("Slice c*");

        connect(alignSlicetoAStarPushButton, SIGNAL(clicked()), volumeOpenGLWidget, SLOT(alignSlicetoAStar()));
        connect(alignSlicetoBStarPushButton, SIGNAL(clicked()), volumeOpenGLWidget, SLOT(alignSlicetoBStar()));
        connect(alignSlicetoCStarPushButton, SIGNAL(clicked()), volumeOpenGLWidget, SLOT(alignSlicetoCStar()));

        helpCellOverlayButton = new QPushButton("Help Cell");
        rotateCellButton = new QPushButton("Rotation");
        toggleHklButton = new QPushButton("Toggle hkl");
        toggleCellButton = new QPushButton("Toggle Cell");

        rotateCellButton->setCheckable(true);
        rotateCellButton->setChecked(false);

        helpCellOverlayButton->setCheckable(true);
        helpCellOverlayButton->setChecked(true);

        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->setRowStretch(10, 1);

        gridLayout->addWidget(aNormSpinBox, 0, 0, 1, 2);
        gridLayout->addWidget(bNormSpinBox, 0, 2, 1, 2);
        gridLayout->addWidget(cNormSpinBox, 0, 4, 1, 2);

        gridLayout->addWidget(alphaNormSpinBox, 1, 0, 1, 2);
        gridLayout->addWidget(betaNormSpinBox, 1, 2, 1, 2);
        gridLayout->addWidget(gammaNormSpinBox, 1, 4, 1, 2);

        gridLayout->addWidget(hSpinBox, 4, 0, 1, 2);
        gridLayout->addWidget(kSpinBox, 4, 2, 1, 2);
        gridLayout->addWidget(lSpinBox, 4, 4, 1, 2);

        gridLayout->addWidget(alignAlongAStarButton, 5, 0, 1, 2);
        gridLayout->addWidget(alignAlongBStarButton, 5, 2, 1, 2);
        gridLayout->addWidget(alignAlongCStarButton, 5, 4, 1, 2);

        gridLayout->addWidget(alignSlicetoAStarPushButton, 6, 0, 1, 2);
        gridLayout->addWidget(alignSlicetoBStarPushButton, 6, 2, 1, 2);
        gridLayout->addWidget(alignSlicetoCStarPushButton, 6, 4, 1, 2);

        gridLayout->addWidget(helpCellOverlayButton, 7, 0, 1, 3);
        gridLayout->addWidget(rotateCellButton, 7, 3, 1, 3);

        gridLayout->addWidget(toggleHklButton, 8, 0, 1, 6);
        gridLayout->addWidget(toggleCellButton, 9, 0, 1, 6);

        unitCellWidget->setLayout(gridLayout);

        unitCellDockWidget->setWidget(unitCellWidget);

        //        unitCellDockWidget->setFixedHeight(unitCellWidget->minimumSizeHint().height()*1.15);

        viewMenu->addAction(unitCellDockWidget->toggleViewAction());
        volumeRenderMainWindow->addDockWidget(Qt::LeftDockWidgetArea, unitCellDockWidget);
    }


    /* SVO metadata text edit */
    {
        svoHeaderDock = new QDockWidget("File info/notes", this);
        svoHeaderEdit = new QPlainTextEdit;

        svoHeaderDock->setWidget(svoHeaderEdit);

        volumeRenderMainWindow->addDockWidget(Qt::LeftDockWidgetArea, svoHeaderDock);
    }

    /* File Controls Widget */
    {
        fileControlsWidget = new QWidget;

        beamOverrideCheckBox = new QCheckBox;

        QLabel * beamOverrideLabel = new QLabel(QString("Beam center override:"));

        beamXOverrideSpinBox = new QDoubleSpinBox;
        beamXOverrideSpinBox->setRange(-5000, 5000);
        beamXOverrideSpinBox->setDecimals(2);
        beamXOverrideSpinBox->setPrefix("x: ");
        beamXOverrideSpinBox->setSuffix(" px");

        beamYOverrideSpinBox = new QDoubleSpinBox;
        beamYOverrideSpinBox->setRange(-5000, 5000);
        beamYOverrideSpinBox->setDecimals(2);
        beamYOverrideSpinBox->setPrefix("y: ");
        beamYOverrideSpinBox->setSuffix(" px");

        QLabel * label = new QLabel(QString("Active angle:"));

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
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->setRowStretch(4, 1);
        gridLayout->addWidget(beamOverrideCheckBox, 0, 0, 1, 1);
        gridLayout->addWidget(beamOverrideLabel, 0, 1, 1, 7);
        gridLayout->addWidget(beamXOverrideSpinBox, 1, 0, 1, 4);
        gridLayout->addWidget(beamYOverrideSpinBox, 1, 4, 1, 4);
        gridLayout->addWidget(label, 2, 0, 1, 4);
        gridLayout->addWidget(activeAngleComboBox, 2, 4, 1, 4);
        gridLayout->addWidget(omegaCorrectionSpinBox, 3, 0, 1, 8);
        gridLayout->addWidget(kappaCorrectionSpinBox, 4, 0, 1, 8);
        gridLayout->addWidget(phiCorrectionSpinBox, 5, 0, 1, 8);

        fileControlsWidget->setLayout(gridLayout);

        fileDockWidget = new QDockWidget("Reconstruction corrections", this);
        fileDockWidget->setWidget(fileControlsWidget);
        fileDockWidget->setMaximumHeight(fileControlsWidget->minimumSizeHint().height() * 1.1);
        viewMenu->addAction(fileDockWidget->toggleViewAction());
        reconstructionMainWindow->addDockWidget(Qt::LeftDockWidgetArea, fileDockWidget);



    }

    /*Voxelize dock widget*/
    {
        setFileButton = new QPushButton;
        setFileButton->setIcon(QIcon(":/art/proceed.png"));
        setFileButton->setText("Set ");

        readFileButton = new QPushButton;
        readFileButton->setIcon(QIcon(":/art/proceed.png"));
        readFileButton->setText("Read ");
        readFileButton->setEnabled(false);

        projectFileButton = new QPushButton;
        projectFileButton->setIcon(QIcon(":/art/proceed.png"));
        projectFileButton->setText("Project ");
        projectFileButton->setEnabled(false);

        reconstructButton = new QPushButton;
        reconstructButton->setIcon(QIcon(":/art/fast_proceed.png"));
        reconstructButton->setText("Reconstruct frames");

        killButton = new QPushButton;
        killButton->setIcon(QIcon(":/art/kill.png"));
        killButton->setText("Kill ");

        svoLevelSpinBox = new QSpinBox;
        svoLevelSpinBox->setRange(1, 15);
        svoLevelSpinBox->setPrefix("Octree levels: ");

        voxelizeButton = new QPushButton;
        voxelizeButton->setIcon(QIcon(":/art/proceed.png"));
        voxelizeButton->setText("Generate octree");

        saveSvoButton = new QPushButton;
        saveSvoButton->setIcon(QIcon(":/art/save.png"));
        saveSvoButton->setText("Save");

        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setRowStretch(4, 1);
        gridLayout->addWidget(reconstructButton, 0, 0, 1, 1);
        gridLayout->addWidget(killButton, 0, 1, 1, 1);
        gridLayout->addWidget(svoLevelSpinBox, 1, 0, 1, 2);
        gridLayout->addWidget(voxelizeButton, 2, 0, 1, 1);
        gridLayout->addWidget(saveSvoButton, 2, 1, 1, 1);


        voxelizeWidget = new QWidget;
        voxelizeWidget->setLayout(gridLayout);

        voxelizeDockWidget = new QDockWidget("Reconstruction operations");
        voxelizeDockWidget->setWidget(voxelizeWidget);
        viewMenu->addAction(voxelizeDockWidget->toggleViewAction());
        reconstructionMainWindow->addDockWidget(Qt::RightDockWidgetArea, voxelizeDockWidget);
    }

    /* Header dock widget */
    {
        fileHeaderEditTwo = new QPlainTextEdit;
        headerHighlighterTwo = new Highlighter(fileHeaderEditTwo->document());
        fileHeaderEditTwo->setReadOnly(true);

        fileHeaderDockTwo = new QDockWidget("Frame header info", this);
        fileHeaderDockTwo->setWidget(fileHeaderEditTwo);
        viewMenu->addAction(fileDockWidget->toggleViewAction());
        reconstructionMainWindow->addDockWidget(Qt::RightDockWidgetArea, fileHeaderDockTwo);
    }



    /* Function dock widget */
    {
        functionToggleButton = new QPushButton("Toggle");
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

        functionDockWidget = new QDockWidget("Model settings", this);
        functionWidget = new QWidget;

        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(0);
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->setRowStretch(6, 1);
        gridLayout->addWidget(funcParamASpinBox, 1, 0, 1, 4);
        gridLayout->addWidget(funcParamBSpinBox, 2, 0, 1, 4);
        gridLayout->addWidget(funcParamCSpinBox, 3, 0, 1, 4);
        gridLayout->addWidget(funcParamDSpinBox, 4, 0, 1, 4);
        gridLayout->addWidget(functionToggleButton, 5, 0, 1, 4);
        functionWidget->setLayout(gridLayout);

        functionDockWidget->setWidget(functionWidget);
        //        functionDockWidget->setFixedHeight(functionWidget->minimumSizeHint().height());
        viewMenu->addAction(functionDockWidget->toggleViewAction());
        volumeRenderMainWindow->addDockWidget(Qt::RightDockWidgetArea, functionDockWidget);
        functionDockWidget->hide();
    }

    /* Line dock widget */
    {
        lineModel = new LineModel();

        lineView = new QTableView;
        lineView->setModel(lineModel);

        insertLinePushButton = new QPushButton("Insert");
        removeLinePushButton = new QPushButton("Remove");

        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(0);
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->setRowStretch(4, 1);
        gridLayout->addWidget(lineView, 0, 0, 1, 4);
        gridLayout->addWidget(insertLinePushButton, 1, 0, 1, 2);
        gridLayout->addWidget(removeLinePushButton, 1, 2, 1, 2);

        lineWidget = new QWidget;
        lineWidget->setLayout(gridLayout);

        lineDockWidget = new QDockWidget("Lines");
        lineDockWidget->setWidget(lineWidget);
        viewMenu->addAction(lineDockWidget->toggleViewAction());
        volumeRenderMainWindow->addDockWidget(Qt::BottomDockWidgetArea, lineDockWidget);
    }

    /* Output Widget */
    {
        outputDockWidget = new QDockWidget("Message log", this);
        botWidget = new QWidget;

        // Text output
        errorTextEdit = new QPlainTextEdit;
        msgLogHighlighter = new Highlighter(errorTextEdit->document());

        errorTextEdit->setReadOnly(true);

        // Layout
        QGridLayout * gridLayout = new QGridLayout;
        gridLayout->setHorizontalSpacing(5);
        gridLayout->setVerticalSpacing(0);
        gridLayout->setContentsMargins(5, 5, 5, 5);
        gridLayout->addWidget(errorTextEdit, 0, 0, 1, 1);

        botWidget->setLayout(gridLayout);
        outputDockWidget->setWidget(botWidget);
        viewMenu->addAction(outputDockWidget->toggleViewAction());
        this->addDockWidget(Qt::BottomDockWidgetArea, outputDockWidget);
    }

    /* Text output widget */
    outputPlainTextEdit = new QPlainTextEdit("Output in plain text.");
    textResultHighlighter = new Highlighter(outputPlainTextEdit->document());
    outputPlainTextEdit->setReadOnly(true);

    /*      Tab widget      */
    tabWidget = new QTabWidget;

    // Add tabs
    tabWidget->addTab(browserMainWindow, "Browse");
    tabWidget->addTab(reconstructionMainWindow, "Reconstruct");
    tabWidget->addTab(volumeRenderMainWindow, "Visualize");
    tabWidget->addTab(outputPlainTextEdit, "Text output");

    // Put into main layout
    mainLayout = new QGridLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(tabWidget, 1, 0, 1, 1);

    mainWidget = new QWidget(this);
    mainWidget->setLayout(mainLayout);

    // Tabify docks
    volumeRenderMainWindow->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    volumeRenderMainWindow->tabifyDockWidget(unitCellDockWidget, svoHeaderDock);
    volumeRenderMainWindow->tabifyDockWidget(unitCellDockWidget, plotDockWidget);

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
    apply_mode = str;
}

void MainWindow::nextFrame()
{
    imageSpinBox->setValue(imageSpinBox->value() + 1);
}

void MainWindow::previousFrame()
{
    imageSpinBox->setValue(imageSpinBox->value() - 1);
}

void MainWindow::batchForward()
{
    imageSpinBox->setValue(imageSpinBox->value() + batch_size);
}
void MainWindow::batchBackward()
{
    imageSpinBox->setValue(imageSpinBox->value() - batch_size);
}

void MainWindow::setBatchSize(int value)
{
    batch_size = value;
}

void MainWindow::setImageRange(int low, int high)
{
    imageSpinBox->setRange(low, high);
}

void MainWindow::setLineIntegralPlot()
{
    Matrix<double> y_data = volumeOpenGLWidget->worker()->getLineIntegralData();
    
    Matrix<double> x_data(y_data.m(), y_data.n());
    
    for (int i = 0; i < x_data.size(); i++)
    {
        x_data[i] = volumeOpenGLWidget->worker()->getLineIntegralXmin() +  i*(volumeOpenGLWidget->worker()->getLineIntegralXmax() - volumeOpenGLWidget->worker()->getLineIntegralXmin())/(x_data.size() - 1);
    }
    
    plotWidget->plot(volumeOpenGLWidget->worker()->getLineIntegralXmin(),
                     volumeOpenGLWidget->worker()->getLineIntegralXmax(),
                     volumeOpenGLWidget->worker()->getLineIntegralYmin(),
                     volumeOpenGLWidget->worker()->getLineIntegralYmax(), 
                     x_data, y_data);
}

void MainWindow::takeImageScreenshotFunction()
{
    // Move this to imagepreview?
    QString format = "jpg";
    QDateTime dateTime = dateTime.currentDateTime();
    QString initialPath = screenshot_dir + QString("/screenshot_" + dateTime.toString("yyyy_MM_dd_hh_mm_ss")) + "." + format;

    QString file_name = QFileDialog::getSaveFileName(this, "Save as", initialPath,
                        QString("%1 files (*.%2);;All files (*)")
                        .arg(format.toUpper())
                        .arg(format));

    if (file_name != "")
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
    QString initialPath = screenshot_dir + QString("/image_" + dateTime.toString("yyyy_MM_dd_hh_mm_ss")) + "." + format;

    QString file_name = QFileDialog::getSaveFileName(this, "Save as", initialPath,
                        QString("%1 files (*.%2);;All files (*)")
                        .arg(format.toUpper())
                        .arg(format));

    if (file_name != "")
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
    size_t removable = info.size() - chars_max;

    if (removable > 0)
    {
        info.remove(0, removable);
    }

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
    QString initialPath = screenshot_dir + QString("/screenshot_" + dateTime.toString("yyyy_MM_dd_hh_mm_ss")) + "." + format;

    QString file_name = QFileDialog::getSaveFileName(this, "Save as", initialPath,
                        QString("%1 files (*.%2);;All files (*)")
                        .arg(format.toUpper())
                        .arg(format));

    if (file_name != "")
    {
        QFileInfo info(file_name);
        screenshot_dir = info.absoluteDir().path();

        emit captureFrameBuffer(file_name);
    }
}

