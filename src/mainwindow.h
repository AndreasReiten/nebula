#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <fstream>

#include <QDockWidget>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QProgressBar>
#include <QMainWindow>
#include <QPushButton>
#include <QComboBox>
#include <QGridLayout>
#include <QFileDialog>
#include <QTableView>

#include "filebrowserwidget.h"

#include "math/matrix.h"
#include "opencl/contextcl.h"
#include "file/filetreeview.h"
#include "image/imagepreview.h"
#include "svo/sparsevoxeloctree.h"
#include "misc/texthighlighter.h"
#include "misc/box.h"
#include "misc/line.h"
#include "misc/plotwidget.h"
#include "misc/linemodel.h"
#include "volume/volumerender.h"
#include "worker/worker.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
        Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private slots:
    void loadSvoMetaData();
    void saveLoadedSvoMetaData();

    void saveLineAsTextProxy();
    void saveSurfaceAsTextProxy();

    void displayPopup(QString title, QString text);
    void transferSet();

    void takeVolumeScreenshot();
    void setCurrentSvoLevel(int value);
    void setTab(int tab);

    void loadSvo();
    void saveSvo();
    void saveLoadedSvo();

    void about();
    void aboutOpenCL();
    void aboutOpenGL();

    void print(QString str);
    void setGeneralProgressFormat(QString str);
    void setMemoryUsageFormat(QString str);
    void loadUnitcellFile();
    void initWorkers();

    void loadBrowserPaths();
    void setHeader(QString path);

    void saveProject();
    void loadProject();

    void applyAnalytics();
    void applyPlaneMarker();
    void applySelection();
    void nextFrame();
    void previousFrame();
    void batchForward();
    void batchBackward();
    void takeImageScreenshotFunction();
    void saveImageFunction();
    void setApplyMode(QString str);
    void setBatchSize(int value);
    void setImageRange(int low, int high);
    void setLineIntegralPlot();
    void setPlaneIntegralPlot();

signals:
    void saveLineAsText(QString str);
    void saveSurfaceAsText(QString str);
    void testToWindow();

    void changedDetector(int value);
    void changedFormat(int value);
    void changedPaths(QStringList strlist);
    void captureFrameBuffer(QString path);
    void changedUB();
    void pathRemoved(QString path);
    void imageChanged(ImageInfo image);
    void selectionChanged(Selection area);
    void centerImage();
    void analyze(QString str);
    void setPlaneMarkers(QString str);
    void setSelection(QString str);
    void takeImageScreenshot(QString str);
    void saveImage(QString str);
    void setChanged(SeriesSet set);
    void setPulled(SeriesSet set);

private:
    Ui::MainWindow *ui;
    FileBrowserWidget *fileBrowserWidget;
//    QMainWindow * fileBrowserWidget;

    QAction * loadSvoMetadataAct;
    QAction * saveLoadedSvoMetadataAct;


    // PlotWidget
    QDockWidget * plotLineDockWidget;
    PlotLineWidget * plotLineWidget;
    QAction * plotLineSaveAsImageAction;
    QAction * plotLineSaveAsTextAction;

    QSpinBox * plotLineABResSpinBox;
    QSpinBox * plotLineCResSpinBox;

    QToolBar * plotLineToolBar;
    QWidget * plotLineWidgetContainter;
    QCheckBox * plotLineLogCheckBox;

    QDockWidget * plotSurfaceDockWidget;
    PlotSurfaceWidget * plotSurfaceWidget;
    QAction * plotSurfaceSaveAsImageAction;
    QAction * plotSurfaceSaveAsTextAction;

    QSpinBox * plotSurfaceABResSpinBox;
    QSpinBox * plotSurfaceCResSpinBox;

    QToolBar * plotSurfaceToolBar;
    QWidget * plotSurfaceWidgetContainter;
    QCheckBox * plotSurfaceLogCheckBox;

    // Line dock widget
    QDockWidget * lineDockWidget;
    QWidget * lineWidget;
    QTableView * lineView;
    LineModel * lineModel;
    QPushButton * insertLinePushButton;
    QPushButton * snapLinePosAPushButton;
    QPushButton * snapLinePosBPushButton;
    QPushButton * snapLineCenterPushButton;
    QPushButton * setLinePosAPushButton;
    QPushButton * setLinePosBPushButton;
    QPushButton * setLineCenterPushButton;

    QPushButton * alignLineToAPushButton;
    QPushButton * alignLineToBPushButton;
    QPushButton * alignLineToCPushButton;



    QPushButton * copyLinePushButton;
    QPushButton * translateLinePushButton;
    QPushButton * setTranslateLineAPushButton;
    QPushButton * setTranslateLineBPushButton;

    QPushButton * removeLinePushButton;

    /* UI elements for UB matrix */
    QDockWidget * unitCellDockWidget;
    QWidget * unitCellWidget;

    void loadParFile();

    // SVO metadata editor
    QPlainTextEdit * svoHeaderEdit;
    QDockWidget * svoHeaderDock;


    // Real space unit cell
    QDoubleSpinBox * aNormSpinBox;
    QDoubleSpinBox * bNormSpinBox;
    QDoubleSpinBox * cNormSpinBox;

    QDoubleSpinBox * alphaNormSpinBox;
    QDoubleSpinBox * betaNormSpinBox;
    QDoubleSpinBox * gammaNormSpinBox;

    // Reciprocal space unit cell
    QDoubleSpinBox * aStarSpinBox;
    QDoubleSpinBox * bStarSpinBox;
    QDoubleSpinBox * cStarSpinBox;

    QDoubleSpinBox * alphaStarSpinBox;
    QDoubleSpinBox * betaStarSpinBox;
    QDoubleSpinBox * gammaStarSpinBox;

    // Rotation
    QDoubleSpinBox * phiSpinBox;
    QDoubleSpinBox * kappaSpinBox;
    QDoubleSpinBox * omegaSpinBox;

    // Positioning
    QSpinBox * hSpinBox;
    QSpinBox * kSpinBox;
    QSpinBox * lSpinBox;

    QPushButton * alignAlongAStarButton;
    QPushButton * alignAlongBStarButton;
    QPushButton * alignAlongCStarButton;

    QPushButton * alignSlicetoAStarPushButton;
    QPushButton * alignSlicetoBStarPushButton;
    QPushButton * alignSlicetoCStarPushButton;

    QPushButton * helpCellOverlayButton;
    QPushButton * rotateCellButton;
    QPushButton * toggleCellButton;
    QPushButton * toggleHklButton;

    /* File selection filter */
    QLineEdit * fileFilter;

    /* Header dock widget */
    QDockWidget * fileHeaderDockOne;
    QPlainTextEdit * fileHeaderEditOne;

    QDockWidget * fileHeaderDockTwo;
    QPlainTextEdit * fileHeaderEditTwo;

    // Buttons
    QPushButton * reconstructButton;
    QPushButton * setFileButton;
    QPushButton * readFileButton;
    QPushButton * projectFileButton;
    QPushButton * voxelizeButton;
    QPushButton * saveSvoButton;
    QPushButton * killButton;
    QPushButton * functionToggleButton;
    QPushButton * loadParButton;
    QPushButton * unitcellButton;
    QPushButton * loadPathsPushButton;
    QPushButton * removeCurrentPushButton;


    QThread * voxelizeThread;

    // Workers
    VoxelizeWorker * voxelizeWorker;

    // File browser
    FileSelectionModel * fileSelectionModel;
    FileTreeView * fileTreeView;
    void setFiles(QMap<QString, QStringList> folder_map);


    // Image browser widget
    QMainWindow * reconstructionMainWindow;

    // Actions
    QAction * shadowAct;
    QAction * integrate3DAct;
    QAction * integrate2DAct;
    QAction * logIntegrate2DAct;
    QAction * sliceAct;
    QAction * scalebarAct;
//        QAction * saveSVOAct;
    QAction * saveLoadedSvoAct;
    QAction * dataStructureAct;
    QAction * backgroundAct;
    QAction * projectionAct;
    QAction * screenshotAct;
    QAction * openSvoAct;
    QAction * exitAct;
    QAction * aboutAct;
    QAction * aboutQtAct;
    QAction * aboutOpenCLAct;
    QAction * aboutOpenGLAct;
    QAction * orthoGridAct;
    QAction * alignLabXtoSliceXAct;
    QAction * alignLabYtoSliceYAct;
    QAction * alignLabZtoSliceZAct;
    QAction * alignSliceToLabAct;
    QAction * rotateRightAct;
    QAction * rotateLeftAct;
    QAction * rotateUpAct;
    QAction * rotateDownAct;
    QAction * rulerAct;
    QAction * markAct;
    QAction * labFrameAct;
    QAction * rollCW;
    QAction * rollCCW;
    QAction * integrateCountsAct;
    QAction * imageScreenshotAct;
    QAction * saveImageAct;


    SparseVoxelOctree svo_inprocess;
    SparseVoxelOctree svo_loaded;

    void closeEvent(QCloseEvent * event);
    void initActions();
    void initConnects();
    void initGUI();
    void initMenus();

    void setStartConditions();

    void readSettings();
    void writeSettings();
    bool maybeSave();
    void setDarkTheme();


    QString strippedName(const QString &fullFileName);
    QString info;

    QLineEdit * hklEdit;

    bool hasPendingChanges;
    size_t batch_size;


    QLabel * alpha;
    QLabel * beta;
    QLabel * gamma;
    QLabel * a;
    QLabel * b;
    QLabel * c;

    QLabel * alphaStar;
    QLabel * betaStar;
    QLabel * gammaStar;
    QLabel * aStar;
    QLabel * bStar;
    QLabel * cStar;

    // Volume render widget
    QMainWindow * volumeRenderMainWindow;

    QComboBox * activeAngleComboBox;

    QSpinBox * svoLevelSpinBox;

    QDoubleSpinBox * omegaCorrectionSpinBox;
    QDoubleSpinBox * kappaCorrectionSpinBox;
    QDoubleSpinBox * phiCorrectionSpinBox;
    QDoubleSpinBox * beamXOverrideSpinBox;
    QDoubleSpinBox * beamYOverrideSpinBox;


    QDoubleSpinBox * funcParamASpinBox;
    QDoubleSpinBox * funcParamBSpinBox;
    QDoubleSpinBox * funcParamCSpinBox;
    QDoubleSpinBox * funcParamDSpinBox;

    QDoubleSpinBox * volumeDataMinSpinBox;
    QDoubleSpinBox * volumeDataMaxSpinBox;
    QDoubleSpinBox * volumeAlphaSpinBox;
    QDoubleSpinBox * volumeBrightnessSpinBox;

    QCheckBox * beamOverrideCheckBox;

    QComboBox * volumeTsfTextureComboBox;
    QComboBox * volumeTsfAlphaComboBox;
    QComboBox * volumeViewModeComboBox;

    QSlider * qualitySlider;

    QCheckBox * volumeRenderLogCheckBox;

    QElapsedTimer timer;
    Highlighter * msgLogHighlighter;
    Highlighter * textResultHighlighter;
    Highlighter * headerHighlighterOne;
    Highlighter * headerHighlighterTwo;

    QTabWidget * tabWidget;

    QDockWidget * fileDockWidget;
    QDockWidget * voxelizeDockWidget;
    QWidget * fileControlsWidget;
    QWidget * voxelizeWidget;

    QWidget * toolChainWidget;
    QWidget * mainWidget;
    QWidget * botWidget;
    QWidget * graphicsWidget;
    QWidget * functionWidget;
    QWidget * unitcellWidget;
    QWidget * setFilesWidget;

    // Corrections dock widget
    QWidget * correctionWidget;
    QDockWidget * correctionDock;
    QPushButton * traceSeriesPushButton;
    QCheckBox * traceTextureCheckBox;
    QDoubleSpinBox * correctionFlatDoubleSpinBox;
    QSpinBox * correctionClutterSpinBox;
    QSpinBox * correctionMedianSpinBox;
    QSpinBox * correctionPlaneSpinBox;
    QCheckBox * correctionFlatCheckBox;
    QCheckBox * correctionPlaneCheckBox;
    QCheckBox * correctionClutterCheckBox;
    QCheckBox * correctionMedianCheckBox;
    QCheckBox * correctionLorentzCheckBox;
    QCheckBox * correctionPolarizationCheckBox;
    QCheckBox * correctionFluxCheckBox;
    QCheckBox * correctionExposureCheckBox;
    QCheckBox * correctionPixelProjectionCheckBox;

    // Navigation dock widget
    QWidget * navigationWidget;
    QDockWidget * navigationDock;
    QProgressBar * generalProgressBar;
    QSpinBox * batchSizeSpinBox;
    QSpinBox * imageSpinBox;
    QPushButton * nextFramePushButton;
    QPushButton * previousFramePushButton;
    QPushButton * batchForwardPushButton;
    QPushButton * batchBackwardPushButton;
    QPushButton * nextSeriesPushButton;
    QPushButton * prevSeriesPushButton;

    // Operations dock widget
    QPushButton * applyPlaneMarkerPushButton;
    QPushButton * applySelectionPushButton;
    QPushButton * integratePushButton;
    QComboBox * selectionModeComboBox;
    QGridLayout * selectionLayout;
    QWidget * selectionWidget;
    QDockWidget * selectionDock;

    // Misc
    QString apply_mode;
    QPlainTextEdit * outputPlainTextEdit;

protected:
    ImageOpenGLWidget * imageOpenGLWidget;

    // OpenGL rendering widgets
    VolumeOpenGLWidget * volumeOpenGLWidget;

    /* Image browser toolbar */
    QToolBar * imageToolBar;

    QAction * saveProjectAction;
    QAction * loadProjectAction;
    QAction * squareAreaSelectAlphaAction;
    QAction * squareAreaSelectBetaAction;
    QAction * centerImageAction;
    QAction * showWeightCenterAction;


    /* Image browser view settings widget */
    QComboBox * imageModeComboBox;
    QComboBox * imageTsfTextureComboBox;
    QComboBox * imageTsfAlphaComboBox;

    QDoubleSpinBox * imageDataMinDoubleSpinBox;
    QDoubleSpinBox * imageDataMaxDoubleSpinBox;

    QCheckBox * imageLogCheckBox;

    QWidget * imageSettingsWidget;
    QDockWidget * imageSettingsDock;

    QDockWidget * outputDockWidget;
    QDockWidget * graphicsDockWidget;
    QDockWidget * functionDockWidget;

    QString working_dir;
    QString screenshot_dir;

    QPlainTextEdit * errorTextEdit;
    QProgressBar * memoryUsageProgressBar;

    QMenuBar * mainMenu;
    QMenu * reduceMenu;
    QMenu * svoMenu;
    QMenu * viewMenu;
    QMenu * helpMenu;
    QToolBar * viewToolBar;

    Matrix<float> reduced_pixels;

    QGridLayout * mainLayout;
};
#endif // MAINWINDOW_H
