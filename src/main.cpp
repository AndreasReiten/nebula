/* Copyright 2013 Andreas Reiten
 * This file is part of Nebula.
 *
 * Nebula is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation (version 3).
 *
 * Nebula is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nebula.  If not, see <http://www.gnu.org/licenses/>. */

#include <QCoreApplication>
#include <QApplication>
#include <QIcon>
#include "mainwindow.h"
#include "utils/tools.h"

const int verbosity = 1;

/* This is the top level GUI implementation */
int main(int argc, char **argv)
{
    // Initialize the log file
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = QString(dateTime.toString("dd/MM/yyyy hh:mm:ss"));
    if (verbosity == 1) writeToLogAndPrint("### RIV LOG "+dateTimeString+" ###", "riv.log", 0);
    if (verbosity == 1) writeToLogAndPrint(Q_FUNC_INFO, "riv.log", 1);

//    ~if (Q_OS_LINUX) QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
//    ~std::cout << "Qt::AA_X11InitThreads = " << QCoreApplication::testAttribute(Qt::AA_X11InitThreads) << std::endl;
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/art/app.png"));

    app.setOrganizationName("Norwegian University of Science and Technology");
    app.setApplicationName("Nebula");

    MainWindow window;
    window.show();

    return app.exec();
}
