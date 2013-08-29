#include <QApplication>
#include "mainwindow.h"

/* This is the top level GUI implementation */ 
// valgrind --tool=memcheck --show-reachable=yes
// Show each frame briefly upon loading and possibly on demand?


int main(int argc, char **argv)
{
    QApplication app(argc, argv);
	app.setOrganizationName("Norwegian University of Science and Technology");
    app.setApplicationName("RIV");
	
    MainWindow window;
    window.show();

    return app.exec();
}
