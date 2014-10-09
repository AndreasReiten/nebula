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

#include "../lib/qxlib/qxlib.h"
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
//    void setHeader(QString path);
    void displayPopup(QString title, QString text);
//    void omitFile();

//    void test();


    // Toolchain button stuff
    void anyButtonStart();

//    void readScriptButtonFinish();
    void setFileButtonFinish();
    void allInOneButtonFinish();
    void readFileButtonFinish();
    void projectFileButtonFinish();
    void voxelizeButtonFinish();

    void takeScreenshot();
//    void saveScriptAs();
    void setCurrentSvoLevel(int value);
    void setTab(int tab);

//    void newScriptFile();
//    void openScript();
//    void saveScript();
    void openSvo();
    void saveSvo();
    void saveLoadedSvo();


//    void documentWasModified();

    void about();
    void aboutOpenCL();
    void aboutOpenGL();


    void print(QString str);
    void setGenericProgressFormat(QString str);
    void setMemoryUsageFormat(QString str);
    void openUnitcellFile();
    void initializeWorkers();

    void runProjectFileThread();
    void runAllInOneThread();
    void setImage(ImageInfo image);
    void loadPaths();
    void removeImage();
    void setFrame(int value);
    void nextFrame();
    void previousFrame();
    void batchForward();
    void batchBackward();
//    void nextFolder();
//    void previousFolder();
    
    void setHeader(QString path);
    
    void saveProject();
    void loadProject();
    
    void setSelection(Selection rect);
    
    // File selection
    void setFilesFromSelectionModel();
    
    
signals:
    void testToWindow();

    void changedDetector(int value);
    void changedFormat(int value);
    void changedPaths(QStringList strlist);
    void captureFrameBuffer(QString path);
    void changedUB();
    void pathRemoved(QString path);
    void pathChanged(QString path);
    void imageChanged(ImageInfo image);
    void centerImage();
    
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
    
    QPushButton * helpCellOverlayButton;
    QPushButton * rotateCellButton;
    QPushButton * toggleCellButton;
    
    /* File selection filter */
    QLineEdit * fileSelectionFilter;
    
    /* Header dock widget */
    QDockWidget * fileHeaderDock;
    QPlainTextEdit * fileHeaderEdit;
    
    /* Header strings */
    QString current_svo_path;
//    QString current_script_path;

    // Buttons
    QPushButton *allInOneButton;
//    QPushButton *readScriptButton;
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
    QThread * setFileThread;
    QThread * readFileThread;
    QThread * projectFileThread;
    QThread * voxelizeThread;
    QThread * allInOneThread;
    QThread * displayFileThread;

    // Workers
//    ReadScriptWorker * readScriptWorker;
    SetFileWorker * setFileWorker;
    ReadFileWorker * readFileWorker;
    ProjectFileWorker * projectFileWorker;
    MultiWorker * allInOneWorker;
    VoxelizeWorker * voxelizeWorker;

    // File browser
    QWidget * fileBrowserWidget;
    FileSelectionModel * fileSelectionModel;
    FileTreeView *fileSelectionTree;
    void setFiles(QMap<QString, QStringList> folder_map);
    
    
    // Image browser widget
    QMainWindow * imageMainWindow;
    QWidget * imageCentralWidget;
    
    QLineEdit * pathLineEdit;
    
    QWidget * imageDisplayWidget;
    
    QPushButton * imageFastBackButton;
    QPushButton * imageSlowBackButton;
    
    QPushButton * imageFastForwardButton;
    QPushButton * imageSlowForwardButton;
    QSpinBox * imageSpinBox;

    // Actions
    QAction *shadowAct;
    QAction *integrate3DAct;
    QAction *integrate2DAct;
    QAction *logIntegrate2DAct;
    QAction *sliceAct;
    QAction *scalebarAct;
    QAction *saveSVOAct;
    QAction *saveLoadedSvoAct;
//    QAction *log3DAct;
    QAction *dataStructureAct;
    QAction *backgroundAct;
    QAction *projectionAct;
    QAction *screenshotAct;
    QAction *openSvoAct;
//    QAction *newAct;
//    QAction *openAct;
//    QAction *saveAct;
//    QAction *scriptingAct;
//    QAction *runScriptAct;
//    QAction *saveAsAct;
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
    
    
    int current_svo;

    SparseVoxelOcttree svo_inprocess;
    SparseVoxelOcttree svo_loaded;

    void closeEvent(QCloseEvent *event);
    void initializeActions();
    void initializeConnects();
    void initializeInteractives();
    void initializeMenus();

    void setStartConditions();

    void readSettings();
    void writeSettings();
    bool maybeSave();
//    void loadFile(const QString &fileName);
//    bool saveFile(const QString &fileName);
//    void setCurrentFile(const QString &fileName);


    QString strippedName(const QString &fullFileName);
//    QString scriptHelp;
    QString curFile;
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
    
    QComboBox *formatComboBox;
    QComboBox *activeAngleComboBox;

    QSpinBox * svoLevelSpinBox;

    QDoubleSpinBox *noiseCorrectionMinDoubleSpinBox;
    QDoubleSpinBox *noiseCorrectionMaxDoubleSpinBox;
    QDoubleSpinBox *postCorrectionMinDoubleSpinBox;
    QDoubleSpinBox *postCorrectionMaxDoubleSpinBox;
    
    QDoubleSpinBox *omegaCorrectionSpinBox;
    QDoubleSpinBox *kappaCorrectionSpinBox;
    QDoubleSpinBox *phiCorrectionSpinBox;


    QDoubleSpinBox * funcParamASpinBox;
    QDoubleSpinBox * funcParamBSpinBox;
    QDoubleSpinBox * funcParamCSpinBox;
    QDoubleSpinBox * funcParamDSpinBox;

    QDoubleSpinBox * volumeRenderDataMinSpinBox;
    QDoubleSpinBox * volumeRenderDataMaxSpinBox;
    QDoubleSpinBox * volumeRenderAlphaSpinBox;
    QDoubleSpinBox * volumeRenderBrightnessSpinBox;

    QComboBox * volumeRenderTsfComboBox;
    QComboBox * volumeRenderTsfAlphaComboBox;
    QComboBox * volumeRenderViewModeComboBox;

    QSlider * qualitySlider;
    
    QCheckBox * volumeRenderLogCheckBox;

    QElapsedTimer timer;
//    Highlighter *script_highlighter;
    Highlighter *error_highlighter;

    QTabWidget *tabWidget;

    
    QDockWidget *fileDockWidget;
    QWidget *fileControlsWidget;

    QWidget *toolChainWidget;
    QWidget *mainWidget;
    QWidget *topWidget;
    QWidget *botWidget;
    QWidget *graphicsWidget;
    QWidget *functionWidget;
    QWidget *unitcellWidget;
    QWidget *setFilesWidget;
//    QWidget *viewWidget;

protected:
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
    QComboBox * tsfTextureComboBox;
    QComboBox * tsfAlphaComboBox;

    QDoubleSpinBox * dataMinDoubleSpinBox;
    QDoubleSpinBox * dataMaxDoubleSpinBox;

    QCheckBox * logCheckBox;
    
    QWidget * imageSettingsWidget;
    QDockWidget * imageSettingsDock;
    
    /* Image corrections widget */
    QCheckBox * autoBackgroundCorrectionCheckBox;
    QWidget * correctionWidget;
    QDockWidget * correctionDock;
    QCheckBox * correctionLorentzCheckBox;
    
    
    // Image Preview Widget
    ImagePreviewWindow * imagePreviewWindow;
    int display_file;


    QDockWidget *outputDockWidget;
    QDockWidget *graphicsDockWidget;
    QDockWidget *functionDockWidget;

    QString svoDir;

//    QPlainTextEdit *scriptTextEdit;
    QPlainTextEdit *errorTextEdit;
    QProgressBar *genericProgressBar;
    QProgressBar *memoryUsageProgressBar;



    QMenuBar * mainMenu;
    QMenu *reduceMenu;
    QMenu *svoMenu;
//    QMenu *scriptMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;
    QToolBar *fileSelectionToolBar;
    QToolBar *viewToolBar;


//    QScriptEngine engine;
//    QScriptValue rawFilesQs;

    // Main resources
    QStringList file_paths;
    QList<DetectorFile> files;
    ImageFolder image_folder;
    Matrix<float> reduced_pixels;

    // Related to file treatment
    float threshold_reduce_low;
    float threshold_reduce_high;
    float threshold_project_low;
    float threshold_project_high;

    // Related to Voxelize
    int brick_inner_dimension;
    int brick_outer_dimension;

    QGridLayout * mainLayout;


};
#endif
