#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qThread>
#include <QtConcurrent/QtConcurrent>
#include <QProgressDialog>
#include <QFuture>
#include <QFutureWatcher>

#include "src/phone.h"
#include "src/bootloader.h"
#include "src/fileManager.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    init();
    checkFastboot("");
    if (setFlags == 1) {
        ui->lPath->setText(fastbootPath);
        ui->bRefresh->setEnabled(true);
        deviceCheck(true);
        if (protocol != "") {
            ui->lDevice->setText(protocol.toUpper() + " - " + device + modified);
            ui->bBL->setEnabled(true);
        }
    } else {
        ui->bRefresh->setEnabled(false);
        ui->bBL->setEnabled(false);
        ui->bEDL->setEnabled(false);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init()
{
    blFlags=0x00;
    setFlags&=0x01;
    devId=0x00;
    protocol="";
    ui->stackedWidget->setCurrentIndex(0);
    ui->frameOptions->setHidden(true);
    ui->bRefresh->setEnabled(false);
    ui->bBL->setEnabled(false);
    ui->bEDL->setEnabled(false);

    if (setFlags & 0x01){
        ui->bRefresh->setEnabled(true);
        deviceCheck(true);
        if (protocol != "") {
            ui->bBL->setEnabled(true);
        } else {
            ui->lDevice->setText(tr("Not found"));
        }
    }
}

void MainWindow::on_bKit_clicked()
{
    QFileDialog dKit;
    dKit.setFileMode(QFileDialog::AnyFile);
    checkFastboot(dKit.getExistingDirectory(this, tr("Select ADB & Fastboot directory"), QDir::homePath())+"/");
    if (setFlags == 1) {
        ui->lPath->setText(fastbootPath);
        ui->bRefresh->setEnabled(true);
        deviceCheck(true);
        if (protocol != "") ui->lDevice->setText(protocol.toUpper() + " - " + device + modified);
    } else {
        ui->bRefresh->setEnabled(false);
    }
}

void MainWindow::on_bHOME_clicked()
{
    init();
}

void MainWindow::on_bRefresh_clicked()
{
    // when refresh is clicked a waiting box appears while the program is checking the device with QtConcurrent::run
    protocol="";
    QMessageBox msgBox;
    msgBox.setStandardButtons(QMessageBox::NoButton);
    msgBox.setText(QObject::tr("Checking for device"));
    msgBox.show();

    QEventLoop wait;
    QFutureWatcher<void> fw;

    // Start device check in a separate thread
    QObject::connect(&fw, &QFutureWatcher<void>::finished, &wait, &QEventLoop::quit);
    fw.setFuture(QtConcurrent::run(deviceCheck, false));
    wait.exec();
    QObject::disconnect(&fw, &QFutureWatcher<void>::finished, &wait, &QEventLoop::quit);
    // Close progress dialog when the work is done
    msgBox.close();

    if (protocol == ""){
        // show mesagge error to suggest to disconnect and reconnect the phone to the cable while keeping the phone on
        QMessageBox::warning(this, tr("No device found"), tr("No device found\nDisconnect and reconnect your phone."));
        ui->lDevice->setText(tr("Not found"));
        ui->bBL->setEnabled(false);
    } else {
        ui->lDevice->setText(protocol.toUpper() + " - " + device + modified);
        ui->bBL->setEnabled(true);
    }
}



void MainWindow::on_bBL_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}


void MainWindow::on_bUNOS_clicked()
{
    blFlags=0x04;
    ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::on_pRNOS_clicked()
{
    blFlags=0x17;
    ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::on_pCROM_clicked()
{
    blFlags=0x14;
    ui->frameOptions->setHidden(false);
    // allow user to select only wipe and disable avb
    ui->cbWipe->setCheckState(Qt::Unchecked);
    ui->cbAVB->setCheckState(Qt::Unchecked);

    ui->cb2Slo->setCheckState(Qt::Unchecked);
    ui->cbLogi->setCheckState(Qt::Checked);
    ui->cbDCheck->setCheckState(Qt::Checked);
    ui->cb2Slo->setDisabled(true);
    ui->cbLogi->setDisabled(true);
    ui->cbDCheck->setDisabled(true);

    ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::on_pMFL_clicked()
{
    blFlags=0x00;
    ui->frameOptions->setHidden(false);
    ui->cbWipe->setCheckState(Qt::Unchecked);
    ui->cb2Slo->setCheckState(Qt::Unchecked);
    ui->cbLogi->setCheckState(Qt::Unchecked);
    ui->cbAVB->setCheckState(Qt::Unchecked);
    ui->cbDCheck->setCheckState(Qt::Unchecked);
    ui->cb2Slo->setDisabled(false);
    ui->cbLogi->setDisabled(false);
    ui->cbDCheck->setDisabled(false);
    ui->stackedWidget->setCurrentIndex(2);
}


void MainWindow::on_cbWipe_stateChanged(int arg1)
{
    blFlags = arg1 ? (blFlags | 0x01) : (blFlags & 0xFE);
}

void MainWindow::on_cbLogi_stateChanged(int arg1)
{
    blFlags = arg1 ? (blFlags | 0x04) : (blFlags & 0xFB);
}

void MainWindow::on_cb2Slo_stateChanged(int arg1)
{
    blFlags = arg1 ? (blFlags | 0x02) : (blFlags & 0xFD);
}

void MainWindow::on_cbAVB_stateChanged(int arg1)
{
    blFlags = arg1 ? (blFlags | 0x08) : (blFlags & 0xF7);
}

void MainWindow::on_cbDCheck_stateChanged(int arg1)
{
    blFlags = arg1 ? (blFlags | 0x10) : (blFlags & 0xEF);
}

void MainWindow::on_bAdd_clicked()
{
    // Verifica se il numero di elementi nella lista è inferiore a 5
    if (ui->listWidget->count() < 5) {
        // Aggiungi un elemento alla lista
        QString filePath = QFileDialog::getOpenFileName(this, tr("Select the FW"), QDir::homePath());
        if (!filePath.isEmpty()) {
            ui->listWidget->addItem(filePath);
        }
    } else {
        // Avvisa l'utente che è stato raggiunto il limite massimo
        QMessageBox::warning(this, tr("Limit reached"), tr("You cannot add more than five elements."));
    }
}

void MainWindow::on_bRm_clicked()
{
    QListWidgetItem *item = ui->listWidget->currentItem();
    if (item) {
        delete item;
    }
}


void MainWindow::on_bBLConf_clicked()
{
    // a pop up is displayed to confirm the action while making a list of all the settings

    // first we checked the input files:
    // - if it's a zip file, we extract "payload_properties.txt" to check if the file contains the string "PRE_OTA_VERSION", if so it's an incremental and not supported, error message and back to add files
    // if not we extract payload.bin to a temp directory, do the next checks
    // - if it's a bin file, we start extracting all partitions to have .img files and to the next check
    // - if it's an .img file, get the path and check if all the partitions in the list of the phone are satisfied, if not error message and back to add files *

    // * this error is not shown and not considered if the flag has its bit set to blFlags 0x10

    // - if it's .7z could be Spike's or Reindex fastboot rom:
    // -- if it's Spike's
    // --- check that there are enough .7z.00X files to open the archive
    // --- check if those files before .7z have FULLOTA in their names, extract only that file and proceed to perform checks for zip and then bin files
    // --- if there is no FULLOTA string then extract everything and perform the check for the .img files
    // -- if it's Reindex
    // --- we search for vendor_boot.img to get the path and proceed to perform the check for the .img files

    // then we check the phone
    // -- if it's adb reboot to bootloader
    // --- message prompt to detach and reattach the usb cable after the phone is on
    // -- if it's fastbootd, reboot to bootloader
    // -- if it's bootloader, check if the phone is unlocked, if not error message and back to add files

    // then we check the flags which we'll continue to comment later now it's time to program

    // pop up to confirm the action
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Confirmation"), tr("You selected these settings:\nWipe: %1\nFlash to both slots: %2\nFlash logical partitions: %3\nDisable AVB: %4\nDisable partition integrity check: %5\nAre you sure you want to continue?").arg((blFlags & 0x01) ? tr("Yes") : tr("No")).arg((blFlags & 0x02) ? tr("Yes") : tr("No")).arg((blFlags & 0x04) ? tr("Yes") : tr("No")).arg((blFlags & 0x08) ? tr("Yes") : tr("No")).arg((blFlags & 0x10) ? tr("Yes") : tr("No")), QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::No) {
        return;
    }

    QProgressDialog *m_progressDialog = new QProgressDialog("Please wait...", QString(), 0, 0, this);
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setRange(0, 100);
    m_progressDialog->setValue(0);
    m_progressDialog->setCancelButton(nullptr);
    m_progressDialog->setWindowTitle("Flashing");
    m_progressDialog->setLabelText(tr("Sanitizing the list of files"));
    m_progressDialog->show();

    // Sanitize the list of files
    sanitizeList(ui->listWidget);

    if (ui->listWidget->count() == 0){
        QMessageBox::warning(this, tr("Error"), tr("No files selected."));
        // close the progress dialog
        m_progressDialog->close();
        return;
    }


    // check the phone first with adb if it's booted up prompt to reboot to bootloader
    // if it's fastbootd, reboot to bootloader
    // if it's bootloader, check if it's unlocked
    // if not error message and back to add files
    m_progressDialog->setLabelText(tr("Checking the device"));
    m_progressDialog->setValue(10);

    // start the check in a separate thread
    QFutureWatcher<void> fw2;
    QObject::connect(&fw2, &QFutureWatcher<void>::finished, m_progressDialog, &QProgressDialog::close);
    fw2.setFuture(QtConcurrent::run(deviceCheck, false));
    m_progressDialog->exec();

    QObject::disconnect(&fw2, &QFutureWatcher<void>::finished, m_progressDialog, &QProgressDialog::close);

    while (protocol != "bl"){
        if (protocol == "adb"){
            QMessageBox::information(this, tr("Reboot to bootloader"), tr("Press OK to automatically reboot to bootloader"));
            QProcess process;
            process.startCommand(fastbootPath + "adb reboot bootloader");
            process.waitForFinished();
        } else if (protocol == "fd"){
            QMessageBox::information(this, tr("Reboot to bootloader"), tr("Press OK to automatically reboot to bootloader"));
            QProcess process;
            process.startCommand(fastbootPath + "fastboot reboot bootloader");
            process.waitForFinished();
        } else if (protocol == ""){
            // if the protocol is not bl, qmessagebox to retry or cancel
            QMessageBox::StandardButton reply2;
            reply2 = QMessageBox::question(this, tr("Error"), tr("The device is not in bootloader mode, please reboot to bootloader and try again."), QMessageBox::Retry|QMessageBox::Cancel);
            if (reply2 == QMessageBox::Cancel) {
                m_progressDialog->close();
                return;
            }
        }
        deviceCheck(false);
    }
    if (!(blFlags & 0x08)){
        QMessageBox::warning(this, tr("Error"), tr("The bootloader is locked, unlock it and try again."));
        m_progressDialog->close();
        return;
    }


    m_progressDialog->setLabelText(tr("Checking the input files and eventually extracting\nThis could take a while"));
    m_progressDialog->setValue(20);

    // check the input files
    // let's start with spike's if it's a fullota
    // check if it's a fullota
    // if it's a fullota, extract only that file
    // if not, extract everything
    // it cycle through all the items added to search for OTA.7z.001 files

    //m_progressDialog->setLabelText("Checking the input files and eventually extracting\nThis could take a while, the process could hang");
    //m_progressDialog->show();
    //::processEvents();
    // if it finds one it extracts the file and it will treat it as a zip file

    // Check with QDir if extTemp in the program diretory is already present, if not create it
    QDir dir(QCoreApplication::applicationDirPath() + "/extTemp");
    folderCheck(dir);

    // prepare for multithread call, manageBL returns a boolean
    QFutureWatcher<bool> fw;
    QObject::connect(&fw, &QFutureWatcher<bool>::finished, m_progressDialog, &QProgressDialog::close);
    fw.setFuture(QtConcurrent::run(manageBL, dir, ui->listWidget));
    m_progressDialog->exec();

    QObject::disconnect(&fw, &QFutureWatcher<bool>::finished, m_progressDialog, &QProgressDialog::close);
    bool result = fw.result();

    // ignore file check if set to 0x10
    if (!result || (blFlags & 0x10)){
        QMessageBox::warning(this, tr("Error"), tr("The selected files do not contain the necessary partitions for your device."));
        m_progressDialog->close();
        return;
    }


    m_progressDialog->setLabelText(tr("Flashing the device"));
    m_progressDialog->setValue(75);

    // start the flashing process
    QFutureWatcher<void> fw3;
    QObject::connect(&fw3, &QFutureWatcher<void>::finished, m_progressDialog, &QProgressDialog::close);
    fw3.setFuture(QtConcurrent::run(bootloader_flasher, dir));
    m_progressDialog->exec();
    QObject::disconnect(&fw3, &QFutureWatcher<void>::finished, m_progressDialog, &QProgressDialog::close);

    QMessageBox::information(this, tr("Success"), tr("The device has been successfully flashed."));
    m_progressDialog->close();
    init();
}






void MainWindow::on_actionAbout_QT_triggered()
{
    QMessageBox::aboutQt(this, "About Qt");
}


void MainWindow::on_btnOptions_clicked()
{
    // check if a valid option is selected
    if (!ui->rbReboot->isChecked() && !ui->rbLUBL->isChecked() && !ui->rbLUCP->isChecked()){
        QMessageBox::warning(this, tr("Error"), tr("Please select a valid option."));
        return;
    }

    /*if (ui->rbReboot->isChecked(){
            // inside cbReboot we have what mode the user wants to reboot their phone
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, tr("Confirmation"), tr("You selected to reboot the phone in %1 mode, are you sure you want to continue?").arg(cbReboot), QMessageBox::Yes|QMessageBox::No);
            if (reply == QMessageBox::No) {
                return;
            }

        })*/



}


void MainWindow::on_bADB_clicked()
{
    // resets the radiobuttons
    ui->rbReboot->setChecked(false);
    ui->rbLUBL->setChecked(false);
    ui->rbLUCP->setChecked(false);

    ui->stackedWidget->setCurrentIndex(3);
}

