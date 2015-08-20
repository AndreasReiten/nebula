#include "filebrowserwidget.h"
#include "ui_filebrowserwidget.h"

FileBrowserWidget::FileBrowserWidget(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FileBrowserWidget)
{
    ui->setupUi(this);
}

FileBrowserWidget::~FileBrowserWidget()
{
    delete ui;
}
