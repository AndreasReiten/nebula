#include "visualizationwidget.h"
#include "ui_visualizationwidget.h"

VisualizationWidget::VisualizationWidget(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::VisualizationWidget)
{
    ui->setupUi(this);
}

VisualizationWidget::~VisualizationWidget()
{
    delete ui;
}
