#include "reconstructionwidget.h"
#include "ui_reconstructionwidget.h"

ReconstructionWidget::ReconstructionWidget(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ReconstructionWidget)
{
    ui->setupUi(this);
}

ReconstructionWidget::~ReconstructionWidget()
{
    delete ui;
}
