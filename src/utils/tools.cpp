#include "tools.h"

QString timeString(size_t ms)
{
    size_t hours = ms/(1000*60*60);
    size_t minutes = (ms - (1000*60*60)*hours)/(1000*60);
    size_t seconds = (ms - (1000*60*60)*hours - minutes*(1000*60))/(1000);
    size_t milliseconds = (ms - (1000*60*60)*hours - minutes*(1000*60) - seconds*1000);

    QString time("");
    if (hours > 0) time += QString::number(hours)+"h ";
    if (minutes > 0) time += QString::number(minutes)+"m ";
    if (seconds > 0) time += QString::number(seconds)+"s ";
    time += QString::number(milliseconds)+"ms";

    return time;
}

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



QString openResource(const char * path)
{
    QFile file( path );
    file.open( QFile::ReadOnly );
    QString qsrc(file.readAll());
    file.close();

    return qsrc;
}

//QByteArray openFile(const char * path)
//{
//    std::ifstream in(path, std::ios::in | std::ios::binary);
//    std::string contents;

//    if (in)
//    {
//        in.seekg(0, std::ios::end);
//        contents.resize(in.tellg());
//        in.seekg(0, std::ios::beg);
//        in.read(&contents[0], contents.size());
//        in.close();
//    }
//    else
//    {
//        qDebug(QString("Could not open file: " + QString(path)).toStdString().c_str());
//    }

//    QByteArray ba = QString(contents.c_str()).toUtf8();

//    return ba;
//}

