#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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


#include <hdf5.h>
/* GL and CL*/
#ifdef _WIN32
    #define GLEW_STATIC
#endif
#include <GL/glew.h>
#include <CL/opencl.h>
#include <CL/cl_gl.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

#include "utils/text_highlighter.h"
#include "utils/vol_data_set.h"
#include "utils/miniarray.h"
#include "utils/glclinit.h"
#include "utils/volumerender.h"
#include "utils/imagerender.h"
#include "utils/worker.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();


protected:
    VolumeDataSet * dataInstance;

private slots:
    //void setGlobalDetector(int value);
    //void setGlobalFormat(int value);
    void setTab(int tab);
    void openSVO();
    void toggleFullScreen();
    void newFile();
    void open();
    bool save();
    bool saveAs();
    void about();
    void documentWasModified();
    void aboutOpenCL();
    void aboutOpenGL();
    void aboutHDF5();

    void runAllInOne();
    void runReadScript();
    void runSetFiles();
    void runReadFiles();
    void runProjectFiles();
    void runGenerateSvo();

    void previewSVO();
    void print(QString str);
    void setGenericProgressFormat(QString str);
    void openUnitcellFile();
    void initializeThreads();
    void errorString(QString str);

signals:
    void changedDetector(int value);
    void changedFormat(int value);
    void changedActiveAngle(int value);
    void changedPaths(QStringList strlist);

private:
    MiniArray<float> VIEW_BRICKS;
    MiniArray<unsigned int> VIEW_OCT_INDEX;
    MiniArray<unsigned int> VIEW_OCT_BRICK;
    MiniArray<double> HIST_NORM;
    MiniArray<double> HIST_LOG;
    MiniArray<double> HIST_MINMAX;
    MiniArray<char> SVO_COMMENT;

    size_t VIEW_LEVELS, VIEW_BPP, VIEW_DIM_BRICKS, VIEW_N_BRICKS;
    float VIEW_EXTENT[8];

    cl_device device;
    cl_context context;
    cl_command_queue queue;
    cl_uint num_devices;
    cl_uint num_platforms;
    cl_int err;

    int function_success;

    const char * cl_error_cstring(cl_int error);
    void closeEvent(QCloseEvent *event);
    void initializeActions();
    void initializeConnects();
    void initializeInteractives();
    void initializeMenus();
    //void createStatusBar();

    void init_emit();
    int init_cl_device();
    int init_cl_base();
	void setFileTree();
    void readSettings();
    void writeSettings();
    bool maybeSave();
    void loadFile(const QString &fileName);
    bool saveFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);

    //~ QCheckBox * fastMoveCheckBox;
    QGLFormat glFormat;

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
    ContextGLWidget * initGLCL;
    VolumeRenderGLWidget *vrWidget;
    ImageRenderGLWidget *irWidget;



    QDockWidget *outputDockWidget;
    QDockWidget *graphicsDockWidget;
    QDockWidget *functionDockWidget;
    QDockWidget *unitcellDockWidget;

    QString svoDir;
    QString scriptDir;

    QPlainTextEdit *textEdit;
	QPlainTextEdit *errorTextEdit;
    QProgressBar *progressBar;

    QPushButton *readScriptButton;
    QPushButton *setFilesButton;
    QPushButton *readFilesButton;
    QPushButton *projectFilesButton;
    QPushButton *generateSvoButton;
    QPushButton *saveSVOButton;
    //~ QPushButton *previewSVOButton;
    //~ QPushButton *graphicsBackgroundButton;
    //~ QPushButton *graphicsDataStructureButton;
    //~ QPushButton *graphicsLogarithmButton;
    //~ QPushButton *graphicsPerspectiveButton;
    //~ QPushButton *screenshotButton;
    QPushButton *functionToggleButton;

    QMenuBar * mainMenu;
    QMenu *reduceMenu;
    QMenu *svoMenu;
	QMenu *scriptMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;
	QToolBar *scriptToolBar;
    QToolBar *viewToolBar;

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
    QAction *aboutHDF5Act;

	QScriptValue rawFilesValue;
	QScriptEngine engine;
	QScriptValue rawFilesQs;
	QStringList rawFiles;

    QGridLayout * mainLayout;
    QGridLayout * viewLayout;

    QThread * setFileThread;
    QThread * readFileThread;
    QThread * projectFileThread;
    QThread * voxelizeThread;
    QThread * allInOneThread;

    SetFileWorker * setFileWorker;
    ReadFileWorker * readFileWorker;
    ProjectFileWorker * projectFileWorker;
    AllInOneWorker * allInOneWorker;
    VoxelizeWorker * voxelizeWorker;
};
#endif
