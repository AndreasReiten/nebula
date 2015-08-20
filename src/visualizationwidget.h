#ifndef VISUALIZATIONWIDGET_H
#define VISUALIZATIONWIDGET_H

#include <QMainWindow>

namespace Ui {
class VisualizationWidget;
}

class VisualizationWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit VisualizationWidget(QWidget *parent = 0);
    ~VisualizationWidget();

private:
    Ui::VisualizationWidget *ui;
};

#endif // VISUALIZATIONWIDGET_H
