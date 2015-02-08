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

#include "utils/math/qxmathlib.h"
#include "utils/opencl/qxopencllib.h"
#include "utils/opengl/qxopengllib.h"
#include "utils/file/qxfilelib.h"
#include "utils/image/qximagelib.h"
#include "utils/svo/qxsvolib.h"
#include "utils/texthighlighter.h"
#include "utils/volumerender.h"
#include "utils/worker.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();


private slots:
    void displayPopup(QString title, QString text);
    void transferSet();
    
    // Toolchain button stuff
//    void anyButtonStart();

//    void setFileButtonFinish();
//    void allInOneButtonFinish();
//    void readFileButtonFinish();
//    void projectFileButtonFinish();
//    void voxelizeButtonFinish();

    void takeVolumeScreenshot();
    void setCurrentSvoLevel(int value);
    void setTab(int tab);

    void openSvo();
    void saveSvo();
    void saveLoadedSvo();

    void about();
    void aboutOpenCL();
    void aboutOpenGL();


    void print(QString str);
    void setGeneralProgressFormat(QString str);
    void setMemoryUsageFormat(QString str);
    void openUnitcellFile();
    void initWorkers();

//    void runProjectFileThread();
//    void runAllInOneThread();
//    void setImage(ImageInfo image);
//    void setSeriesSelection(Selection area);
    void loadPaths();
//    void removeImage();
//    void setFrame(int value);
//    void nextFrame();
//    void prevFrame();
//    void batchForward();
//    void batchBackward();
//    void nextSeries();
//    void prevSeries();
    
    void setHeader(QString path);
    
    void saveProject();
    void loadProject();
    
//    void setSelection(Selection rect);
    
    // File selection
//    void setFilesFromSelectionModel();
    
    
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
    
signals:
    void testToWindow();

    void changedDetector(int value);
    void changedFormat(int value);
    void changedPaths(QStringList strlist);
    void captureFrameBuffer(QString path);
    void changedUB();
    void pathRemoved(QString path);
//    void pathChanged(QString path);
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
    /* UI elements for UB matrix */
    QDockWidget * unitCellDock;
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
    QDockWidget * fileHeaderDock;
    QPlainTextEdit * fileHeaderEdit;
    
    // Buttons
    QPushButton *reconstructButton;
    QPushButton *setFileButton;
    QPushButton *readFileButton;
    QPushButton *projectFileButton;
    QPushButton *voxelizeButton;
    QPushButton *saveSvoButton;
    QPushButton *killButton;
    QPushButton *functionToggleButton;
    QPushButton *loadParButton;
    QPushButton *unitcellButton;
    QPushButton * loadPathsPushButton;
    QPushButton * removeCurrentPushButton;


    // OpenCL
    OpenCLContext * context_cl;

    // QThreads
//    QThread * readScriptThread;
//    QThread * setFileThread;
//    QThread * readFileThread;
//    QThread * projectFileThread;
    QThread * voxelizeThread;
//    QThread * reconstructThread;
    QThread * displayFileThread;

    // Workers
//    ReadScriptWorker * readScriptWorker;
//    SetFileWorker * setFileWorker;
//    ReadFileWorker * readFileWorker;
//    ProjectFileWorker * projectFileWorker;
//    ReconstructWorker * reconstructWorker;
    VoxelizeWorker * voxelizeWorker;

    // File browser
//    QWidget * fileBrowserWidget;
    FileSelectionModel * fileSelectionModel;
    FileTreeView *fileTreeView;
    void setFiles(QMap<QString, QStringList> folder_map);
    
    
    // Image browser widget
    QMainWindow * imageMainWindow;
//    QWidget * imageCentralWidget;
    
//    QLineEdit * pathLineEdit;
    
    QWidget * imageDisplayWidget;
    
//    QPushButton * imageBatchPrevButton;
//    QPushButton * imagePrevButton;
    
//    QPushButton * imageBatchNextButton;
//    QPushButton * imageNextButton;

//    QPushButton * nextSeriesButton;
//    QPushButton * prevSeriesButton;

//    QSpinBox * imageSpinBox;

    // Actions
    QAction *shadowAct;
    QAction *integrate3DAct;
    QAction *integrate2DAct;
    QAction *logIntegrate2DAct;
    QAction *sliceAct;
    QAction *scalebarAct;
    QAction *saveSVOAct;
    QAction *saveLoadedSvoAct;
    QAction *dataStructureAct;
    QAction *backgroundAct;
    QAction *projectionAct;
    QAction *screenshotAct;
    QAction *openSvoAct;
    QAction *exitAct;
    QAction *aboutAct;
    QAction *aboutQtAct;
    QAction *aboutOpenCLAct;
    QAction *aboutOpenGLAct;
    QAction *orthoGridAct;
    QAction *alignLabXtoSliceXAct;
    QAction *alignLabYtoSliceYAct;
    QAction *alignLabZtoSliceZAct;
    QAction *alignSliceToLabAct;
    QAction *rotateRightAct;
    QAction *rotateLeftAct;
    QAction *rotateUpAct;
    QAction *rotateDownAct;
    QAction *rulerAct;
    QAction *markAct;
    QAction *labFrameAct;
    QAction *rollCW;
    QAction *rollCCW;
    QAction *integrateCountsAct;
    QAction * imageScreenshotAct;
    QAction * saveImageAct;
    
    
    SparseVoxelOcttree svo_inprocess;
    SparseVoxelOcttree svo_loaded;

    void closeEvent(QCloseEvent *event);
    void initActions();
    void initConnects();
    void initGUI();
    void initMenus();

    void setStartConditions();

    void readSettings();
    void writeSettings();
    bool maybeSave();


    QString strippedName(const QString &fullFileName);
//    QString curFile;
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
    
//    QComboBox *formatComboBox;
    QComboBox *activeAngleComboBox;

    QSpinBox * svoLevelSpinBox;

//    QDoubleSpinBox *noiseCorrectionMinDoubleSpinBox;
//    QDoubleSpinBox *noiseCorrectionMaxDoubleSpinBox;
//    QDoubleSpinBox *postCorrectionMinDoubleSpinBox;
//    QDoubleSpinBox *postCorrectionMaxDoubleSpinBox;
    
    QDoubleSpinBox *omegaCorrectionSpinBox;
    QDoubleSpinBox *kappaCorrectionSpinBox;
    QDoubleSpinBox *phiCorrectionSpinBox;
    QDoubleSpinBox *beamXOverrideSpinBox;
    QDoubleSpinBox *beamYOverrideSpinBox;


    QDoubleSpinBox * funcParamASpinBox;
    QDoubleSpinBox * funcParamBSpinBox;
    QDoubleSpinBox * funcParamCSpinBox;
    QDoubleSpinBox * funcParamDSpinBox;

    QDoubleSpinBox * volumeRenderDataMinSpinBox;
    QDoubleSpinBox * volumeRenderDataMaxSpinBox;
    QDoubleSpinBox * volumeRenderAlphaSpinBox;
    QDoubleSpinBox * volumeRenderBrightnessSpinBox;

    QCheckBox * beamOverrideCheckBox;
    
    QComboBox * volumeRenderTsfComboBox;
    QComboBox * volumeRenderTsfAlphaComboBox;
    QComboBox * volumeRenderViewModeComboBox;

    QSlider * qualitySlider;
    
    QCheckBox * volumeRenderLogCheckBox;

    QElapsedTimer timer;
    Highlighter *error_highlighter;

    QTabWidget *tabWidget;

    
    QDockWidget *fileDockWidget;
    QDockWidget *voxelizeDockWidget;
    QWidget *fileControlsWidget;
    QWidget *voxelizeWidget;

    QWidget *toolChainWidget;
    QWidget *mainWidget;
//    QWidget *topWidget;
    QWidget *botWidget;
    QWidget *graphicsWidget;
    QWidget *functionWidget;
    QWidget *unitcellWidget;
    QWidget *setFilesWidget;
    
    
    // Corrections dock widget
    QWidget * correctionWidget;
    QDockWidget * correctionDock;
    QPushButton * traceSetPushButton;
    QCheckBox * traceTextureCheckBox;
    QDoubleSpinBox * correctionNoiseDoubleSpinBox;
    QSpinBox * correctionClutterSpinBox;
    QSpinBox * correctionMedianSpinBox;
    QSpinBox * correctionPlaneSpinBox;
    QCheckBox * correctionNoiseCheckBox;
    QCheckBox * correctionPlaneCheckBox;
    QCheckBox * correctionClutterCheckBox;
    QCheckBox * correctionMedianCheckBox;
    QCheckBox * correctionLorentzCheckBox;
    QCheckBox * correctionPolarizationCheckBox;
    QCheckBox * correctionFluxCheckBox;
    QCheckBox * correctionExposureCheckBox;
    
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
    ImagePreviewWorker * imageOpenGLWidget;

    // OpenGL rendering widgets
    SharedContextWindow *sharedContextWindow;
    VolumeRenderWindow *volumeRenderWindow;
    QWidget *volumeRenderWidget;

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
    QComboBox * imagePreviewTsfTextureComboBox;
    QComboBox * imagePreviewTsfAlphaComboBox;

    QDoubleSpinBox * imagePreviewDataMinDoubleSpinBox;
    QDoubleSpinBox * imagePreviewDataMaxDoubleSpinBox;

    QCheckBox * imagePreviewLogCheckBox;
    
    QWidget * imageSettingsWidget;
    QDockWidget * imageSettingsDock;
    
    /* Image corrections widget */
//    QCheckBox * autoBackgroundCorrectionCheckBox;
    
//    QCheckBox * correctionLorentzCheckBox;
    
    
    // Image Preview Widget
//    ImagePreviewWindow * imagePreviewWindow;
//    int display_file;


    QDockWidget *outputDockWidget;
    QDockWidget *graphicsDockWidget;
    QDockWidget *functionDockWidget;

    QString working_dir;
    QString screenshot_dir;

    QPlainTextEdit *errorTextEdit;
//    QProgressBar *genericProgressBar;
    QProgressBar *memoryUsageProgressBar;

    QMenuBar * mainMenu;
    QMenu *reduceMenu;
    QMenu *svoMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;
//    QToolBar *fileSelectionToolBar;
    QToolBar *viewToolBar;

    // Main resources
//    QStringList file_paths;
//    QList<DetectorFile> files;
//    ImageSeries image_folder;
//    SeriesSet series_set;

    Matrix<float> reduced_pixels;

    // Related to file treatment
//    float threshold_reduce_low;
//    float threshold_reduce_high;
//    float threshold_project_low;
//    float threshold_project_high;

    // Related to Voxelize
//    int brick_inner_dimension;
//    int brick_outer_dimension;

    QGridLayout * mainLayout;


};
#endif
