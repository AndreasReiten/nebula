#include "reconstructionwidget.h"
#include "ui_reconstructionwidget.h"

#include <QSettings>

ReconstructionWidget::ReconstructionWidget(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ReconstructionWidget)
{
    ui->setupUi(this);

    loadSettings();
}

ReconstructionWidget::~ReconstructionWidget()
{
    writeSettings();
    delete ui;
}

void ReconstructionWidget::loadSettings()
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    this->restoreState(settings.value("ReconstructionWidget/state").toByteArray());
    this->ui->splitter_2->restoreState(settings.value("ReconstructionWidget/splitter_2/state").toByteArray());
    this->ui->splitter->restoreState(settings.value("ReconstructionWidget/splitter/state").toByteArray());
}

void ReconstructionWidget::writeSettings()
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setValue("ReconstructionWidget/state", this->saveState());
    settings.setValue("ReconstructionWidget/splitter_2/state", this->ui->splitter_2->saveState());
    settings.setValue("ReconstructionWidget/splitter/state", this->ui->splitter->saveState());
}
