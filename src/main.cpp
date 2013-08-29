/* Copyright 2013 Andreas Reiten
 * This file is part of NebulaX.
 * 
 * NebulaX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation (version 3).
 * 
 * NebulaX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with NebulaX.  If not, see <http://www.gnu.org/licenses/>. */

#include <QApplication>
#include "mainwindow.h"

/* This is the top level GUI implementation */ 
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
	app.setOrganizationName("Norwegian University of Science and Technology");
    app.setApplicationName("NebulaX");
	
    MainWindow window;
    window.show();

    return app.exec();
}
