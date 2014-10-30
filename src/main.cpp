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

/* Notes:
 * Radial selectivity. I. e. confining viewable area to a spherical shell 
 * A solid linear texture edit (tsf function) 
 * Area selection prior to reduction to limit data size.
 * Proper setting of transparent voxels. There are artefacts.
 * Align slice frame to normal to a*, b* and c*. 
 * Persistent screenshot dir.
 * Going from slice mode to integrate mode is bugged.
 * Remove PCT threshold altogether. It can no longer be justified if the early box termination based on variance becomes successful. This could also be where the problem with voxelize lies.
 * */

#include <QApplication>
#include <QIcon>
#include <QDateTime>
#include <QString>
#include <QDebug>
#include <QByteArray>

#include "mainwindow.h"

void writeToLogAndPrint(QString text, QString file, bool append)
{
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = QString("["+dateTime.toString("hh:mm:ss")+"] ");

    std::ofstream myfile (file.toStdString().c_str(), std::ios::out | ((append == true) ? std::ios::app : std::ios::trunc));
    if (myfile.is_open())
    {
        myfile << dateTimeString.toStdString().c_str() << text.toStdString().c_str() << std::endl;
        std::cout << "[Log]"<< dateTimeString.toStdString().c_str() << text.toStdString().c_str() << std::endl;
    }
    else std::cout << "Unable to open log file" << std::endl;
}

void appOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        writeToLogAndPrint("Debug: "+
                    QString(localMsg.constData())+" ("+
                    QString(context.file)+":"+
                    QString::number(context.line)+", " +
                    QString(context.function)+")", "nebula.log", 1);
        break;
    case QtWarningMsg:
        writeToLogAndPrint("Warning: "+
                           QString(localMsg.constData())+" ("+
                           QString(context.file)+":"+
                           QString::number(context.line)+", " +
                           QString(context.function)+")", "nebula.log", 1);
        break;
    case QtCriticalMsg:
        writeToLogAndPrint("Critical: "+
                           QString(localMsg.constData())+" ("+
                           QString(context.file)+":"+
                           QString::number(context.line)+", " +
                           QString(context.function)+")", "nebula.log", 1);
        break;
    case QtFatalMsg:
        writeToLogAndPrint("Fatal: "+
                           QString(localMsg.constData())+" ("+
                           QString(context.file)+":"+
                           QString::number(context.line)+", " +
                           QString(context.function)+")", "nebula.log", 1);
        abort();
    }
}


/* This is the top level of the GUI application*/
int main(int argc, char **argv)
{
    // Initialize the log file
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = QString(dateTime.toString("dd/MM/yyyy hh:mm:ss"));
    writeToLogAndPrint("### NEBULA LOG "+dateTimeString+" ###", "nebula.log", 0);

    // Handle Qt messages
    qInstallMessageHandler(appOutput);

    // Register custom objects for signal/slot transfer
    qRegisterMetaType<ImageInfo>();
    qRegisterMetaType<ImageSeries>();
    qRegisterMetaType<SeriesSet>();
    qRegisterMetaType<Selection>();

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/art/app.png"));

    app.setOrganizationName("Norwegian University of Science and Technology");
    app.setApplicationName("Nebula");

    MainWindow window;
    window.show();

    return app.exec();
}
