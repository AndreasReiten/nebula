#ifndef RECONSTRUCTIONWIDGET_H
#define RECONSTRUCTIONWIDGET_H

#include <QMainWindow>

namespace Ui {
class ReconstructionWidget;
}

class ReconstructionWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit ReconstructionWidget(QWidget *parent = 0);
    ~ReconstructionWidget();

private:
    Ui::ReconstructionWidget *ui;

    void loadSettings();
    void writeSettings();
};

#endif // RECONSTRUCTIONWIDGET_H
