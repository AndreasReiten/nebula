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
 * Remove hard memory requirements in cases where information about max RAM is known (case for vRAM).
 * (Low priority) Radial selectivity. I. e. confining viewable area to a spherical shell
 * (Time consuming) A solid linear texture edit (tsf function)
 * (Done) Area selection prior to reduction to limit data size.
 * (Done) Align slice frame to normal to a*, b* and c*.
 * (Done) Persistent screenshot dir.
 * (Done, quick fixed) Going from slice mode to integrate mode is bugged.
 * (Done on GUI level) Remove PCT threshold altogether. It can no longer be justified if the early box termination based on variance becomes successful. This could also be where the problem with voxelize lies.
 * (Somewhat time consuming) Too much VRAM is being used and without proper warnigns when too much is used
 * (Somewhat time consuming) Make native save/load (<< and >>)functions for svo's and possibly reconstruction projects
 * (Done, but early MSD features are still very flawed) Vincinity check for bricks. Bricks can not be made MSD early if some voxels are filled, but others are zero. Would help smoothen surface features. Basically check for voxels of value zero in each brick. It is also flawed in that it disallows MSD for cases where a brick completely envelops a small feature.
 * (Difficult, and variance should be found from raw data, not interpolated) Proper setting of transparent voxels. There are artefacts.
 * Other interpolation modes than IDW to get rid of blobbyness. IDW great unless sparsely populated.
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
