#include "filebrowserwidget.h"
#include "ui_filebrowserwidget.h"
#include "sql/sqlqol.h"

#include "file/fileformat.h"

#include <QSettings>
#include <QMessageBox>

FileBrowserWidget::FileBrowserWidget(QWidget *parent) :
    QMainWindow(parent),
    p_ui(new Ui::FileBrowserWidget)
{
    // Prepare column to sql table translation map
    column_map["Path"] = QPair<int,QString>(0, "FilePath");
    column_map["Path"] = QPair<int,QString>(1, "Path");
    column_map["File"] = QPair<int,QString>(2, "File");
    column_map["*"] = QPair<int,QString>(3, "Active");
    column_map["ω"] = QPair<int,QString>(4, "Omega");
    column_map["κ"] = QPair<int,QString>(5, "Kappa");
    column_map["φ"] = QPair<int,QString>(6, "Phi");
    column_map["Start angle"] = QPair<int,QString>(7, "StartAngle");
    column_map["Increment"] = QPair<int,QString>(8, "AngleIncrement");
    column_map["DDist"] = QPair<int,QString>(9, "DetectorDistance");
    column_map["Beam x"] = QPair<int,QString>(10, "BeamX");
    column_map["Beam y"] = QPair<int,QString>(11, "BeamY");
    column_map["Flux"] = QPair<int,QString>(12, "Flux");
    column_map["T exp"] = QPair<int,QString>(13, "ExposureTime");
    column_map["λ"] = QPair<int,QString>(14, "Wavelength");
    column_map["Detector"] = QPair<int,QString>(15, "Detector");
    column_map["Px size x"] = QPair<int,QString>(16, "PixelSizeX");
    column_map["Px size y"] = QPair<int,QString>(17, "PixelSizeY");

    // Prep UI
    p_ui->setupUi(this);

    fileTreeModel  = new FileSelectionModel;
    fileTreeModel->setRootPath(QDir::rootPath());
    p_ui->fileTreeView->setModel(fileTreeModel);
    p_ui->fileTreeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    p_ui->addFilesButton->setIcon(QIcon(":/art/download.png"));

    this->setAnimated(false);
    connect(p_ui->filterLineEdit, SIGNAL(textChanged(QString)), fileTreeModel, SLOT(setStringFilter(QString)));

    p_ui->filterLineEdit->setText("*.cbf");

    // Open database and initialize tables
    initSql();

    // Init sql query model/view
    selection_model = new CustomSqlQueryModel(p_ui->selectionView);
    selection_model->setQuery(display_query, QSqlDatabase::database());

    QMapIterator<QString, QPair<int,QString>> i(column_map);
    while (i.hasNext()) {
        i.next();
        selection_model->setHeaderData(i.value().first, Qt::Horizontal, i.key());
    }

    p_ui->selectionView->horizontalHeader()->setSortIndicatorShown(true);
    p_ui->selectionView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    p_ui->selectionView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    connect(p_ui->addFilesButton, SIGNAL(clicked(bool)), p_ui->selectionView, SLOT(resizeColumnsToContents()));

    p_ui->selectionView->setModel(selection_model);
    connect(p_ui->selectionView->horizontalHeader(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(sortItems(int,Qt::SortOrder)));

    p_ui->selectionView->setColumnHidden(0, true);
    p_ui->selectionView->setColumnHidden(3, true);

    loadSettings();

    p_ui->selectionView->resizeColumnsToContents();
}

void FileBrowserWidget::querySelectionModel(QString str)
{
    selection_model->setQuery(str, QSqlDatabase::database());
    if (selection_model->lastError().isValid())
    {
        qDebug() << selection_model->lastError();
        p_ui->browserStatusBar->showMessage(selection_model->lastError().text());
    }
}

void FileBrowserWidget::sortItems(int column,Qt::SortOrder order)
{
    QMap<Qt::SortOrder,QString> order_map;
    order_map[Qt::AscendingOrder] = "ASC";
    order_map[Qt::DescendingOrder] = "DESC";

    display_query = ("SELECT * FROM browser_table_cbf ORDER BY "+
                        column_map[selection_model->headerData(column, Qt::Horizontal).toString()].second+
                        " "+order_map[order]+
                        ", File ASC");

    querySelectionModel(display_query);
}

void FileBrowserWidget::initSql()
{
    // Database
//    if (p_db.isOpen()) p_db.close();
//    p_db = QSqlDatabase::addDatabase("QSQLITE");

//    if (p_db.open())
//    {
        QSqlQuery query(QSqlDatabase::database());
        if (!query.exec("CREATE TABLE IF NOT EXISTS browser_table_cbf ("
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
//    }
//    else
//    {
//        qDebug() << "Database error" << QSqlDatabase::database().lastError();
//    }

    // Queries
    upsert_file_query = new QSqlQuery(QSqlDatabase::database());
    upsert_file_query->prepare("INSERT OR IGNORE INTO browser_table_cbf (FilePath, Path, File, Active, Omega, Kappa, Phi, StartAngle, AngleIncrement, DetectorDistance, BeamX, BeamY, Flux, ExposureTime, Wavelength, Detector, PixelSizeX, PixelSizeY) VALUES (:FilePath, :Path, :File, :Active, :Omega, :Kappa, :Phi, :StartAngle, :AngleIncrement, :DetectorDistance, :BeamX, :BeamY, :Flux, :ExposureTime, :Wavelength, :Detector, :PixelSizeX, :PixelSizeY);");

    display_query = "SELECT * FROM browser_table_cbf ORDER BY Path ASC, File ASC";
}

FileBrowserWidget::~FileBrowserWidget()
{
    writeSettings();
    QSqlDatabase::database().close();
    delete p_ui;
}

void FileBrowserWidget::on_addFilesButton_clicked()
{
    QStringList paths(fileTreeModel->selected());

    QSqlDatabase::database().transaction();

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

    QSqlDatabase::database().commit();

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
        QSqlQuery query("DELETE FROM browser_table_cbf", QSqlDatabase::database());
        if (query.lastError().isValid()) qDebug() << sqlQueryError(query);
        querySelectionModel(display_query);
    }
}

void FileBrowserWidget::loadSettings()
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    this->restoreState(settings.value("FileBrowserWidget/state").toByteArray());
//    this->ui->splitter_2->restoreState(settings.value("FileBrowserWidget/splitter_2/state").toByteArray());
    this->p_ui->splitter->restoreState(settings.value("FileBrowserWidget/splitter/state").toByteArray());
}

void FileBrowserWidget::writeSettings()
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    settings.setValue("FileBrowserWidget/state", this->saveState());
//    settings.setValue("FileBrowserWidget/splitter_2/state", this->ui->splitter_2->saveState());
    settings.setValue("FileBrowserWidget/splitter/state", this->p_ui->splitter->saveState());
}

void FileBrowserWidget::on_execQueryButton_clicked()
{
    QSqlQuery query(p_ui->sqlTextEdit->toPlainText(), QSqlDatabase::database());
    if (query.lastError().isValid())
    {
        qDebug() << query.lastError();
        p_ui->browserStatusBar->showMessage(query.lastError().text());
    }
}

const Ui::FileBrowserWidget & FileBrowserWidget::ui() const
{
    return *p_ui;
}
