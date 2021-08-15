#include "mainwindow.h"
#include "./ui_mainwindow.h"

// Public Functions
MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->btnExportAll->setEnabled(false);
    ui->btnExportSelected->setEnabled(false);
}
MainWindow::~MainWindow()
{
    if (_ExportThread != NULL && _ExportThread->isRunning())
        _ExportThread->terminate();
    if (_LoadResourceThread != NULL && _LoadResourceThread->isRunning())
        _LoadResourceThread->terminate();

    delete ui;
}
void MainWindow::ThrowFatalError(std::string errorMessage, std::string errorDetail)
{
    const QString qErrorMessage = QString::fromStdString(errorMessage);
    const QString qErrorDetail = QString::fromStdString(errorDetail);

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(qErrorMessage);
    msgBox.setInformativeText(qErrorDetail);
    msgBox.exec();
    return;
}
void MainWindow::ThrowError(std::string errorMessage, std::string errorDetail)
{
    const QString qErrorMessage = QString::fromStdString(errorMessage);
    const QString qErrorDetail = QString::fromStdString(errorDetail);

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(qErrorMessage);
    msgBox.setInformativeText(qErrorDetail);
    msgBox.exec();
    return;
}

// Private Functions
void MainWindow::PopulateGUIResourceTable()
{
    // Clear any existing contents
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);

    // Load resource file data
    HAYDEN::ResourceFile resourceFile = SAM.GetResourceFile();
    for (int i = 0; i < resourceFile.resourceEntries.size(); i++)
    {
        // Temporary, probably? Skip anything that isn't TGA, LWO, MD6 for now.
        if (resourceFile.resourceEntries[i].version != 67 &&
            resourceFile.resourceEntries[i].version != 31 &&
            resourceFile.resourceEntries[i].version != 21)
            continue;

        int row_count = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row_count);

        // Set Resource Name
        std::string resourceName = resourceFile.resourceEntries[i].name;
        QString qResourceName = QString::fromStdString(resourceName);
        QTableWidgetItem *tableResourceName = new QTableWidgetItem(qResourceName);

        // Set Resource Type
        std::string resourceType = resourceFile.resourceEntries[i].type;
        QString qResourceType = QString::fromStdString(resourceType);
        QTableWidgetItem *tableResourceType = new QTableWidgetItem(qResourceType);

        // Set Resource Version
        std::string resourceVersion = std::to_string(resourceFile.resourceEntries[i].version);
        QString qResourceVersion = QString::fromStdString(resourceVersion);
        QTableWidgetItem *tableResourceVersion = new QTableWidgetItem(qResourceVersion);

        // Set Resource Status
        QTableWidgetItem *tableResourceStatus = new QTableWidgetItem("Loaded");

        // Populate Table Row
        ui->tableWidget->setItem(row_count, 0, tableResourceName);
        ui->tableWidget->setItem(row_count, 1, tableResourceType);
        ui->tableWidget->setItem(row_count, 2, tableResourceVersion);
        ui->tableWidget->setItem(row_count, 3, tableResourceStatus);
    }

    QHeaderView* tableHeader = ui->tableWidget->horizontalHeader();
    tableHeader->setSectionResizeMode(0, QHeaderView::Stretch);
    return;
}
int MainWindow::ConfirmExportAll()
{
    const QString qErrorMessage = "Do you really want to export *everything* in this resource file?";
    const QString qErrorDetail = "For some .resource files this can take up to 20-30 minutes, and require 25GB+ of free space.";

    QMessageBox msgBox;
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(qErrorMessage);
    msgBox.setInformativeText(qErrorDetail);
    int result = 0;
    result = msgBox.exec();
    return result;
}
int MainWindow::ShowLoadStatus()
{
    _LoadStatusBox.setStandardButtons(QMessageBox::Cancel);
    _LoadStatusBox.setText("Loading resource, please wait...");
    int result = 0;
    result = _LoadStatusBox.exec();
    return result;
}
int MainWindow::ShowExportStatus()
{
    _ExportStatusBox.setStandardButtons(QMessageBox::Cancel);
    _ExportStatusBox.setText("Export in progress, please wait...");
    int result = 0;
    result = _ExportStatusBox.exec();
    return result;
}

// Private Slots
void MainWindow::on_btnExportAll_clicked()
{
    if (!_ResourceFileIsLoaded)
    {
        ThrowError("No .resource file is currently loaded.", "Please select a file using the \"Load Resource\" button first.");
        return;
    }
    if (ConfirmExportAll() == 0x0400) // OK
    {
        // Grey out/disable the buttons
        ui->btnExportAll->setEnabled(false);
        ui->btnExportSelected->setEnabled(false);
        ui->btnLoadResource->setEnabled(false);
        ui->tableWidget->setEnabled(false);

        _ExportThread = QThread::create(&HAYDEN::SAMUEL::ExportAll, &SAM, _ExportPath);

        connect(_ExportThread, &QThread::finished, this, [this]() {   

            // Update status to "Exported"
            int rowCount = ui->tableWidget->rowCount();
            for (int64 i = 0; i < rowCount; i++)
            {
                QTableWidgetItem *tableItem = ui->tableWidget->item(i,3);
                tableItem->setText("Exported");
            }

            // Re enable the GUI
            ui->btnExportAll->setEnabled(true);
            ui->btnExportSelected->setEnabled(true);
            ui->btnLoadResource->setEnabled(true);
            ui->tableWidget->setEnabled(true);

            // Close progress box if open
            if (_ExportStatusBox.isVisible())
                _ExportStatusBox.close();
        });

        _ExportThread->start();

        if (ShowExportStatus() == 0x00400000 && _ExportThread->isRunning()) // CANCEL
            _ExportThread->terminate();
    }
    return;
}
void MainWindow::on_btnExportSelected_clicked()
{
    // Get selected items in QList
    QList<QTableWidgetItem *> itemExportQList = ui->tableWidget->selectedItems();

    // Abort if no selection
    if (itemExportQList.size() == 0)
    {
        ThrowError("No items were selected for export.");
        return;
    }

    // Convert to vector, get text for selected rows
    std::vector<std::vector<std::string>> itemExportRows;
    for (int64 i = 0; i < itemExportQList.size(); i+=4)
    {
        std::vector<std::string> rowText = {
            itemExportQList[i]->text().toStdString(),
            itemExportQList[i+1]->text().toStdString(),
            itemExportQList[i+2]->text().toStdString()
        };
        itemExportRows.push_back(rowText);

        // Update status to "Exported"
        itemExportQList[i+3]->setText("Exported");
    }

    // This vector gets passed to file exporter, so it only exports files from this list.
    SAM.ExportSelected(_ExportPath, itemExportRows);
    return;
}
void MainWindow::on_btnLoadResource_clicked()
{
    const QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
    {
        _ApplicationPath = QCoreApplication::applicationFilePath().toStdString();
        _ExportPath = fs::absolute(_ApplicationPath).replace_filename("exports").string();
        _ResourcePath = fileName.toStdString();

        // Load BasePath and PackageMapSpec Data into SAMUEL
        if (!SAM.Init(_ResourcePath))
        {
            ThrowError(SAM.GetLastErrorMessage(), SAM.GetLastErrorDetail());
            return;
        }

        // Grey out/disable the buttons
        ui->btnExportAll->setEnabled(false);
        ui->btnExportSelected->setEnabled(false);
        ui->btnLoadResource->setEnabled(false);
        ui->tableWidget->setEnabled(false);
        ui->tableWidget->setSortingEnabled(false);

        // Load resource data in separate thread
        _LoadResourceThread = QThread::create(&HAYDEN::SAMUEL::LoadResource, &SAM, _ResourcePath);

        connect(_LoadResourceThread, &QThread::finished, this, [this]()
        {
            // Close progress box if open
            if (_LoadStatusBox.isVisible())
                _LoadStatusBox.close();

            // Failed to load resource, display error
            if (SAM.GetResourceErrorCode() == 1)
                ThrowError(SAM.GetLastErrorMessage(), SAM.GetLastErrorDetail());

            // Loaded resource successfully
            if (SAM.GetResourceErrorCode() == 0)
            {
                // Populate the GUI
                PopulateGUIResourceTable();

                // Enable Export buttons
                ui->btnExportAll->setEnabled(true);
                ui->btnExportSelected->setEnabled(true);
                _ResourceFileIsLoaded = 1;
            }

            // Re enable the GUI
            ui->btnLoadResource->setEnabled(true);
            ui->tableWidget->setEnabled(true);
            ui->tableWidget->setSortingEnabled(true);          
        });

        _LoadResourceThread->start();

        if (ShowLoadStatus() == 0x00400000 && _LoadResourceThread->isRunning()) // CANCEL
            _LoadResourceThread->terminate();
    }
}
