#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

#include <QMainWindow>
#include <QApplication>
#include <QWidget>
#include <QThread>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QTimer>
#include <QDockWidget>
#include <QLineEdit>
#include <QGroupBox>
#include <QGridLayout>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QToolButton>
#include <QComboBox>
#include <QPushButton>
#include <QtScript>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QStatusBar>
#include <QSettings>
#include <QTextStream>
#include <QProgressBar>
#include <QSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QList>
#include <QFileSystemModel>
#include <QStandardItemModel>

#include "utils/contextcl.h"
#include "utils/sharedcontext.h"
#include "utils/texthighlighter.h"
#include "utils/volumerender.h"
#include "utils/imagepreview.h"
#include "utils/matrix.h"
#include "utils/worker.h"
#include "utils/sparsevoxelocttree.h"
#include "utils/tools.h"
#include "utils/filetreeview.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();


private slots:
    void updateFileHeader(QString path);
    void displayPopup(QString title, QString text);
    void omitFile();

    void test();


    // Toolchain button stuff
    void anyButtonStart();

    void readScriptButtonFinish();
    void setFileButtonFinish();
    void allInOneButtonFinish();
    void readFileButtonFinish();
    void projectFileButtonFinish();
    void voxelizeButtonFinish();

    void takeScreenshot();
    void saveScriptAs();
    void setCurrentSvoLevel(int value);
    void setTab(int tab);

    void newScriptFile();
    void openScript();
    void saveScript();
    void openSvo();
    void saveSvo();
    void saveLoadedSvo();


    void documentWasModified();

    void about();
    void aboutOpenCL();
    void aboutOpenGL();


    void print(QString str);
    void setGenericProgressFormat(QString str);
    void openUnitcellFile();
    void initializeWorkers();

    void runProjectFileThread();
    void runAllInOneThread();
    void incrementDisplayFile1();
    void incrementDisplayFile10();
    void decrementDisplayFile1();
    void decrementDisplayFile10();
    void setDisplayFile(int value);
    
    // File selection
    void setFilesFromSelectionModel();
    
    
    
signals:
    void testToWindow();

    void changedDetector(int value);
    void changedFormat(int value);
    void changedPaths(QStringList strlist);
    void captureFrameBuffer(QString path);
    void changedUB();
    void imagePreviewChanged(QString path);
    void omitFile(QString str);
    
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
    QString current_script_path;

    // Buttons
    QPushButton *allInOneButton;
    QPushButton *readScriptButton;
    QPushButton *setFileButton;
    QPushButton *readFileButton;
    QPushButton *projectFileButton;
    QPushButton *voxelizeButton;
    QPushButton *saveSvoButton;
    QPushButton *killButton;
    QPushButton *functionToggleButton;
    QPushButton * loadParButton;
    QPushButton * unitcellButton;


    // OpenCL
    OpenCLContext * context_cl;

    // QThreads
    QThread * readScriptThread;
    QThread * setFileThread;
    QThread * readFileThread;
    QThread * projectFileThread;
    QThread * voxelizeThread;
    QThread * allInOneThread;
    QThread * displayFileThread;

    // Workers
    ReadScriptWorker * readScriptWorker;
    SetFileWorker * setFileWorker;
    ReadFileWorker * readFileWorker;
    ProjectFileWorker * projectFileWorker;
    MultiWorker * allInOneWorker;
    VoxelizeWorker * voxelizeWorker;

    // File browser
    QWidget * fileBrowserWidget;
    FileSelectionModel * fileSelectionModel;
    FileTreeView *fileSelectionTree;
    
    // Image browser QDockWidget
    QDockWidget * imageDock;
    
    QWidget * imageWidget;
    
    QLabel * imageLabel;
    
    QWidget * imageDisplayWidget;
    
    QPushButton * imageFastBackButton;
    QPushButton * imageSlowBackButton;
    
    QSpinBox * imageSpinBox;
    
    QPushButton * imageFastForwardButton;
    QPushButton * imageSlowForwardButton;
    QPushButton * omitFrameButton;
    QComboBox * imageModeCB;

    // Actions
    QAction *shadowAct;
    QAction *integrate3DAct;
    QAction *integrate2DAct;
    QAction *logIntegrate2DAct;
    QAction *sliceAct;
    QAction *scalebarAct;
    QAction *saveSVOAct;
    QAction *saveLoadedSvoAct;
    QAction *log3DAct;
    QAction *dataStructureAct;
    QAction *backgroundAct;
    QAction *projectionAct;
    QAction *screenshotAct;
    QAction *openSvoAct;
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *scriptingAct;
    QAction *runScriptAct;
    QAction *saveAsAct;
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
    
    
    int current_svo;

    float suggested_search_radius_high;
    float suggested_search_radius_low;
    float suggested_q;

    SparseVoxelOcttree svo_inprocess;
    SparseVoxelOcttree svo_loaded;

    void closeEvent(QCloseEvent *event);
    void initializeActions();
    void initializeConnects();
    void initializeInteractives();
    void initializeMenus();

    void initializeEmit();

    void readSettings();
    void writeSettings();
    bool maybeSave();
    void loadFile(const QString &fileName);
    bool saveFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);


    QString strippedName(const QString &fullFileName);
    QString scriptHelp;
    QString curFile;
    QString info;

    QLineEdit * hklEdit;




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

    QComboBox *formatComboBox;
    QComboBox *activeAngleComboBox;

    QSpinBox * svoLevelSpinBox;

    QDoubleSpinBox *reduceThresholdLow;
    QDoubleSpinBox *reduceThresholdHigh;
    QDoubleSpinBox *projectThresholdLow;
    QDoubleSpinBox *projectThresholdHigh;
    
    QDoubleSpinBox *omegaCorrectionSpinBox;
    QDoubleSpinBox *kappaCorrectionSpinBox;
    QDoubleSpinBox *phiCorrectionSpinBox;


    QDoubleSpinBox * funcParamASpinBox;
    QDoubleSpinBox * funcParamBSpinBox;
    QDoubleSpinBox * funcParamCSpinBox;
    QDoubleSpinBox * funcParamDSpinBox;

    QDoubleSpinBox * dataMinSpinBox;
    QDoubleSpinBox * dataMaxSpinBox;
    QDoubleSpinBox * alphaSpinBox;
    QDoubleSpinBox * brightnessSpinBox;

    QComboBox * tsfComboBox;
    QComboBox * tsfAlphaComboBox;

    QSlider * qualitySlider;

    QElapsedTimer timer;
    Highlighter *script_highlighter;
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
    QWidget *viewWidget;

protected:
    // OpenGL rendering widgets
    SharedContextWindow *sharedContextWindow;
    VolumeRenderWindow *volumeRenderWindow;
    QWidget *volumeRenderWidget;

    // Image Preview Widget
    ImagePreviewWindow * imagePreviewWindow;
    int display_file;


    QDockWidget *outputDockWidget;
    QDockWidget *graphicsDockWidget;
    QDockWidget *functionDockWidget;

    QString svoDir;

    QPlainTextEdit *scriptTextEdit;
    QPlainTextEdit *errorTextEdit;
    QProgressBar *progressBar;



    QMenuBar * mainMenu;
    QMenu *reduceMenu;
    QMenu *svoMenu;
    QMenu *scriptMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;
    QToolBar *fileSelectionToolBar;
    QToolBar *viewToolBar;


    QScriptEngine engine;
    QScriptValue rawFilesQs;

    // Main resources
    QStringList file_paths;
    QList<DetectorFile> files;
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
