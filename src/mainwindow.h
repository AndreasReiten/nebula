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

#include "utils/contextcl.h"
#include "utils/sharedcontext.h"
#include "utils/texthighlighter.h"
#include "utils/miniarray.h"
#include "utils/volumerender.h"
//#include "utils/imagerender.h"
//#include "utils/worker.h"
#include "utils/sparsevoxelocttree.h"
#include "utils/tools.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();


protected:

private slots:
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

    void runReadScript();

    void print(QString str);
    void setGenericProgressFormat(QString str);
    void openUnitcellFile();
    void initializeThreads();
//    void writeLog(QString str);

    void setReduceThresholdLow(double value);
    void setReduceThresholdHigh(double value);
    void setProjectThresholdLow(double value);
    void setProjectThresholdHigh(double value);
    void runProjectFileThread();
    void runAllInOneThread();
    void runDisplayFileThread(int value);
    void incrementDisplayFile1();
    void incrementDisplayFile10();
    void decrementDisplayFile1();
    void decrementDisplayFile10();
    void setDisplayFile(int value);

signals:
    void changedDetector(int value);
    void changedFormat(int value);
    void changedActiveAngle(int value);
    void changedPaths(QStringList strlist);

private:
    ContextCL * context_cl;

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

    QPushButton * loadParButton;
    QPushButton * unitcellButton;

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

    QDoubleSpinBox *treshLimA_DSB;
    QDoubleSpinBox *treshLimB_DSB;
    QDoubleSpinBox *treshLimC_DSB;
    QDoubleSpinBox *treshLimD_DSB;


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


    QElapsedTimer timer;
    Highlighter *script_highlighter;
    Highlighter *error_highlighter;

    QTabWidget *tabWidget;

    QAction *scalebarAct;
    QPushButton *allInOneButton;
    QPushButton *imageForwardButton;
    QPushButton *imageFastForwardButton;
    QPushButton *imageBackButton;
    QPushButton *imageFastBackButton;
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
    QWidget *scriptWidget;
    QWidget *viewWidget;

protected:
    SharedContextWindow *sharedContextWindow;
    VolumeRenderWindow *volumeRenderWindow;
    QWidget *volumeRenderWidget;
//    ImageRenderGLWidget *imageRenderWidget;

    QDockWidget *outputDockWidget;
    QDockWidget *graphicsDockWidget;
    QDockWidget *functionDockWidget;
    QDockWidget *unitcellDockWidget;

    QString svoDir;
//    QString scriptDir;

    QPlainTextEdit *textEdit;
    QPlainTextEdit *errorTextEdit;
    QProgressBar *progressBar;

    QPushButton *readScriptButton;
    QPushButton *setFilesButton;
    QPushButton *readFilesButton;
    QPushButton *projectFilesButton;
    QPushButton *generateSvoButton;
    QPushButton *saveSVOButton;
    QPushButton *killButton;
    QPushButton *functionToggleButton;

    QMenuBar * mainMenu;
    QMenu *reduceMenu;
    QMenu *svoMenu;
    QMenu *scriptMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;
    QToolBar *scriptToolBar;
    QToolBar *viewToolBar;

    QAction *saveSVOAct;
    QAction *logAct;
    QAction *dataStructureAct;
    QAction *backgroundAct;
    QAction *projectionAct;
    QAction *screenshotAct;
    QAction *openSVOAct;
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *runScriptAct;
    QAction *saveAsAct;
    QAction *exitAct;
    QAction *aboutAct;
    QAction *aboutQtAct;
    QAction *aboutOpenCLAct;
    QAction *aboutOpenGLAct;

    QScriptValue rawFilesValue;
    QScriptEngine engine;
    QScriptValue rawFilesQs;

    // Utility
    int verbosity;

    // Main resources
    QStringList file_paths;
//    QList<PilatusFile> files;
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

    QThread * setFileThread;
    QThread * readFileThread;
    QThread * projectFileThread;
    QThread * voxelizeThread;
    QThread * allInOneThread;
    QThread * displayFileThread;

    QGridLayout * mainLayout;

    int display_file;

//    SetFileWorker * setFileWorker;
//    ReadFileWorker * readFileWorker;
//    ProjectFileWorker * projectFileWorker;
//    AllInOneWorker * allInOneWorker;
//    VoxelizeWorker * voxelizeWorker;
//    DisplayFileWorker * displayFileWorker;
};
#endif
