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
//#include <QTreeView>

#include "utils/contextcl.h"
#include "utils/sharedcontext.h"
#include "utils/texthighlighter.h"
#include "utils/miniarray.h"
#include "utils/volumerender.h"
#include "utils/imagerender.h"
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
    void updateFileHeader(int value);
    void displayPopup(QString title, QString text);
    void toggleScriptView();

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


    void documentWasModified();

    void about();
    void aboutOpenCL();
    void aboutOpenGL();

//    void runReadScript();

    void print(QString str);
    void setGenericProgressFormat(QString str);
    void openUnitcellFile();
    void initializeThreads();

//    void setReduceThresholdLow(double value);
//    void setReduceThresholdHigh(double value);
//    void setProjectThresholdLow(double value);
//    void setProjectThresholdHigh(double value);
    void runProjectFileThread();
    void runAllInOneThread();
    void runDisplayFileThread(int value);
    void incrementDisplayFile1();
    void incrementDisplayFile10();
    void decrementDisplayFile1();
    void decrementDisplayFile10();
    void setDisplayFile(int value);

signals:
    void testToWindow();

    void changedDetector(int value);
    void changedFormat(int value);
    void changedActiveAngle(int value);
    void changedPaths(QStringList strlist);
    void captureFrameBuffer(QString path, float quality);

private:
    // Header dock widget
    QDockWidget * fileHeaderDock;
    QPlainTextEdit * fileHeaderEdit;
    
    // Header strings
    QString current_svo_path;
    QString current_script_path;

    // Buttons
    QPushButton *allInOneButton;
    QPushButton *imageForwardButton;
    QPushButton *imageFastForwardButton;
    QPushButton *imageBackButton;
    QPushButton *imageFastBackButton;
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
    AllInOneWorker * allInOneWorker;
    VoxelizeWorker * voxelizeWorker;
    DisplayFileWorker * displayFileWorker;

    // Boolean checks
    bool isInScriptMode;


    // File browser
    QWidget * fileBrowserWidget;
    FileSourceModel * fileSystemModel;
    FileSourceModel * fileSelectedModel;
    FileTreeView *fileSystemTree;
    FileTreeView *fileSelectedTree;
    
    
    // Actions
    QAction *shadowAct;
    QAction *integrate3DAct;
    QAction *integrate2DAct;
    QAction *logIntegrate2DAct;
    QAction *sliceAct;
    QAction *scalebarAct;
    QAction *saveSVOAct;
    QAction *log3DAct;
    QAction *dataStructureAct;
    QAction *backgroundAct;
    QAction *projectionAct;
    QAction *screenshotAct;
    QAction *openSVOAct;
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
    QAction *alignXAct;
    QAction *alignYAct;
    QAction *alignZAct;
    QAction *rotateRightAct;
    QAction *rotateLeftAct;
    QAction *rotateUpAct;
    QAction *rotateDownAct;
    QAction *rulerAct;
    
    
    int current_svo;

    float suggested_search_radius_high;
    float suggested_search_radius_low;
    float suggested_q;

    SparseVoxelOcttree svo_inprocess;
    QList<SparseVoxelOcttree> svo_loaded;

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

    QSpinBox * svoLevelSpinBox;

    QDoubleSpinBox *reduceThresholdLow;
    QDoubleSpinBox *reduceThresholdHigh;
    QDoubleSpinBox *projectThresholdLow;
    QDoubleSpinBox *projectThresholdHigh;


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

    
    QSpinBox *imageNumberSpinBox;
    QDockWidget *fileDockWidget;
    QWidget *fileControlsWidget;
    QWidget *imageWidget;
    QWidget *imageControlsWidget;

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
//    VolumeRenderWorker *volumeRenderWorker;
    VolumeRenderWindow *volumeRenderWindow;
    QWidget *volumeRenderWidget;

//    ImageRenderWorker *imageRenderWorker;
    ImageRenderWindow *imageRenderWindow;
    QWidget *imageRenderWidget;

    QDockWidget *outputDockWidget;
    QDockWidget *graphicsDockWidget;
    QDockWidget *functionDockWidget;
    QDockWidget *unitcellDockWidget;

    QString svoDir;
//    QString scriptDir;

    QPlainTextEdit *scriptTextEdit;
    QPlainTextEdit *errorTextEdit;
    QProgressBar *progressBar;



    QMenuBar * mainMenu;
    QMenu *reduceMenu;
    QMenu *svoMenu;
    QMenu *scriptMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;
    QToolBar *scriptToolBar;
    QToolBar *viewToolBar;


//    QScriptValue rawFilesValue;
    QScriptEngine engine;
    QScriptValue rawFilesQs;

    // Utility

    // Main resources
    QStringList file_paths;
    QList<PilatusFile> files;
//    QList<PilatusFile> background_files;
    MiniArray<float> reduced_pixels;

    // Related to file treatment
    float threshold_reduce_low;
    float threshold_reduce_high;
    float threshold_project_low;
    float threshold_project_high;

    // Related to Voxelize
    int brick_inner_dimension;
    int brick_outer_dimension;

    QGridLayout * mainLayout;

    int display_file;
};
#endif
