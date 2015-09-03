#include "filebrowserwidget.h"
#include "ui_filebrowserwidget.h"
#include "sql/sqlqol.h"

#include "file/fileformat.h"

#include <QSettings>
#include <QMessageBox>

FileBrowserWidget::FileBrowserWidget(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FileBrowserWidget)
{
    // Prepare column to sql table translation map
    column_map["Path"] = QPair<int,QString>(0, "Path");
    column_map["File"] = QPair<int,QString>(1, "File");
    column_map["*"] = QPair<int,QString>(2, "Active");
    column_map["ω"] = QPair<int,QString>(3, "Omega");
    column_map["κ"] = QPair<int,QString>(4, "Kappa");
    column_map["φ"] = QPair<int,QString>(5, "Phi");
    column_map["Start angle"] = QPair<int,QString>(6, "StartAngle");
    column_map["Increment"] = QPair<int,QString>(7, "AngleIncrement");
    column_map["DDist"] = QPair<int,QString>(8, "DetectorDistance");
    column_map["Beam x"] = QPair<int,QString>(9, "BeamX");
    column_map["Beam y"] = QPair<int,QString>(10, "BeamY");
    column_map["Flux"] = QPair<int,QString>(11, "Flux");
    column_map["T exp"] = QPair<int,QString>(12, "ExposureTime");
    column_map["λ"] = QPair<int,QString>(13, "Wavelength");
    column_map["Detector"] = QPair<int,QString>(14, "Detector");
    column_map["Px size x"] = QPair<int,QString>(15, "PixelSizeX");
    column_map["Px size y"] = QPair<int,QString>(16, "PixelSizeY");

    // Prep UI
    ui->setupUi(this);

    fileTreeModel  = new FileSelectionModel;
    fileTreeModel->setRootPath(QDir::rootPath());
    ui->fileTreeView->setModel(fileTreeModel);
    ui->fileTreeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    ui->addFilesButton->setIcon(QIcon(":/art/download.png"));

//    headerHighlighter = new Highlighter(ui->headerEdit->document());

    this->setAnimated(false);
    connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), fileTreeModel, SLOT(setStringFilter(QString)));
    connect(ui->fileTreeView, SIGNAL(fileChanged(QString)), this, SLOT(setHeader(QString)));

    ui->filterLineEdit->setText("*.cbf");

    // Open database and initialize tables
    initSql();

    // Init sql query model/view
    selection_model = new CustomSqlQueryModel(ui->selectionView);
    selection_model->setQuery(display_query, p_db);

    QMapIterator<QString, QPair<int,QString>> i(column_map);
    while (i.hasNext()) {
        i.next();
        selection_model->setHeaderData(i.value().first, Qt::Horizontal, i.key());
    }

    ui->selectionView->horizontalHeader()->setSortIndicatorShown(true);
    ui->selectionView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->selectionView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    connect(ui->addFilesButton, SIGNAL(clicked(bool)), ui->selectionView, SLOT(resizeColumnsToContents()));

    ui->selectionView->setModel(selection_model);
//    ui->selectionView->verticalHeader()->setVisible(false);
    connect(ui->selectionView->horizontalHeader(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(sortItems(int,Qt::SortOrder)));

    loadSettings();

    ui->selectionView->resizeColumnsToContents();
}

void FileBrowserWidget::setHeader(QString path)
{
//    DetectorFile file(path);
//    ui->headerEdit->setPlainText(file.getHeaderText());
}

void FileBrowserWidget::querySelectionModel(QString str)
{
    selection_model->setQuery(str, p_db);
    if (selection_model->lastError().isValid())
    {
        qDebug() << selection_model->lastError();
        ui->statusBar->showMessage(selection_model->lastError().text());
    }
}

void FileBrowserWidget::sortItems(int column,Qt::SortOrder order)
{
    QMap<Qt::SortOrder,QString> order_map;
    order_map[Qt::AscendingOrder] = "ASC";
    order_map[Qt::DescendingOrder] = "DESC";

    display_query = ("SELECT Path, File, Active, Omega, Kappa, Phi, StartAngle, AngleIncrement, DetectorDistance, BeamX, BeamY, Flux, ExposureTime, Wavelength, Detector, PixelSizeX, PixelSizeY FROM cbf ORDER BY "+
                        column_map[selection_model->headerData(column, Qt::Horizontal).toString()].second+
                        " "+order_map[order]+
                        ", File ASC");

    querySelectionModel(display_query);
}

void FileBrowserWidget::initSql()
{
    // Database
    if (p_db.isOpen()) p_db.close();
    p_db = QSqlDatabase::addDatabase("QSQLITE", "browser_db");

    p_db.setDatabaseName(QDir::currentPath() + "/browser.sqlite3");

    if (p_db.open())
    {
        QSqlQuery query(p_db);
        if (!query.exec("CREATE TABLE IF NOT EXISTS cbf ("
                        "FilePath TEXT PRIMARY KEY NOT NULL, "
                        "Path TEXT,"
                        "File TEXT,"
                        "Active INTEGER, "
                        "Omega REAL, "
                        "Kappa REAL, "
                        "Phi REAL, "
                        "StartAngle REAL,"
                        "AngleIncrement REAL,"
                        "DetectorDistance REAL,"
                        "BeamX REAL,"
                        "BeamY REAL,"
                        "Flux REAL,"
                        "ExposureTime REAL,"
                        "Wavelength REAL,"
                        "Detector TEXT,"
                        "PixelSizeX REAL,"
                        "PixelSizeY REAL"
                        ");"))
        {
            qDebug() << sqlQueryError(query);
        }
    }
    else
    {
        qDebug() << "Database error" << p_db.lastError();
    }

    // Queries
    upsert_file_query = new QSqlQuery(p_db);
    upsert_file_query->prepare("INSERT OR IGNORE INTO cbf (FilePath, Path, File, Active, Omega, Kappa, Phi, StartAngle, AngleIncrement, DetectorDistance, BeamX, BeamY, Flux, ExposureTime, Wavelength, Detector, PixelSizeX, PixelSizeY) VALUES (:FilePath, :Path, :File, :Active, :Omega, :Kappa, :Phi, :StartAngle, :AngleIncrement, :DetectorDistance, :BeamX, :BeamY, :Flux, :ExposureTime, :Wavelength, :Detector, :PixelSizeX, :PixelSizeY);");

    display_query = "SELECT Path, File, Active, Omega, Kappa, Phi, StartAngle, AngleIncrement, DetectorDistance, BeamX, BeamY, Flux, ExposureTime, Wavelength, Detector, PixelSizeX, PixelSizeY FROM cbf ORDER BY Path ASC, File ASC";
}

FileBrowserWidget::~FileBrowserWidget()
{
    writeSettings();
    p_db.close();
    delete ui;
}

void FileBrowserWidget::on_addFilesButton_clicked()
{
    QStringList paths(fileTreeModel->selected());

    p_db.transaction();

    foreach (const QString &value, paths)
    {
        // If valid file
        QFileInfo info(value);

        if (info.isFile())
        {
            // Add to relevant sql database. Guess database based on extension
            upsert_file_query->bindValue(":FilePath", info.filePath());
            upsert_file_query->bindValue(":Path", info.path());
            upsert_file_query->bindValue(":File", info.fileName());

            DetectorFile file(info.filePath());

            upsert_file_query->bindValue(":Active", 1);
            upsert_file_query->bindValue(":Omega", file.omega() * 180.0 / pi);
            upsert_file_query->bindValue(":Kappa", file.kappa() * 180.0 / pi);
            upsert_file_query->bindValue(":Phi", file.phi() * 180.0 / pi);
            upsert_file_query->bindValue(":StartAngle", file.startAngle() * 180.0 / pi);
            upsert_file_query->bindValue(":AngleIncrement", file.angleIncrement() * 180.0 / pi);
            upsert_file_query->bindValue(":DetectorDistance", file.detectorDist());
            upsert_file_query->bindValue(":BeamX", file.beamX());
            upsert_file_query->bindValue(":BeamY", file.beamY());
            upsert_file_query->bindValue(":Flux", file.flux());
            upsert_file_query->bindValue(":ExposureTime", file.expTime());
            upsert_file_query->bindValue(":Wavelength", file.wavelength());
            upsert_file_query->bindValue(":Detector", file.detector());
            upsert_file_query->bindValue(":PixelSizeX", file.pixSizeX());
            upsert_file_query->bindValue(":PixelSizeY", file.pixSizeY());

            if (!upsert_file_query->exec()) qDebug() << sqlQueryError(*upsert_file_query);
        }
    }

    p_db.commit();

    querySelectionModel(display_query);
}

void FileBrowserWidget::on_clearButton_clicked()
{
    QMessageBox msgBox;
    msgBox.setText("Clear all files?");
    msgBox.setInformativeText("This action is irreversible.");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    if (ret == QMessageBox::Yes)
    {
        QSqlQuery query("DELETE FROM cbf", p_db);
        if (!query.exec()) qDebug() << sqlQueryError(query);
        querySelectionModel(display_query);
    }
}

void FileBrowserWidget::loadSettings()
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    this->restoreState(settings.value("FileBrowserWidget/state").toByteArray());
//    this->ui->splitter_2->restoreState(settings.value("FileBrowserWidget/splitter_2/state").toByteArray());
    this->ui->splitter->restoreState(settings.value("FileBrowserWidget/splitter/state").toByteArray());
}

void FileBrowserWidget::writeSettings()
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setValue("FileBrowserWidget/state", this->saveState());
//    settings.setValue("FileBrowserWidget/splitter_2/state", this->ui->splitter_2->saveState());
    settings.setValue("FileBrowserWidget/splitter/state", this->ui->splitter->saveState());
}

void FileBrowserWidget::on_execQueryButton_clicked()
{
    QSqlQuery query(ui->sqlTextEdit->toPlainText(), p_db);
    if (query.lastError().isValid())
    {
        qDebug() << query.lastError();
        ui->statusBar->showMessage(query.lastError().text());
    }
}


void FileBrowserWidget::on_reconstructButton_clicked()
{
    qDebug() << "Set sql table as source for reconstruction. Ask to merge with reconstruction db or just use this.";
}
