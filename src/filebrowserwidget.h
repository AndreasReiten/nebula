#ifndef FILEBROWSERWIDGET_H
#define FILEBROWSERWIDGET_H

#include <QMainWindow>

namespace Ui {
class FileBrowserWidget;
}

class FileBrowserWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit FileBrowserWidget(QWidget *parent = 0);
    ~FileBrowserWidget();

private:
    Ui::FileBrowserWidget *ui;
};

#endif // FILEBROWSERWIDGET_H
