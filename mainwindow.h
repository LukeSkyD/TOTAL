#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QProcess>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QDebug>
#include <QMessageBox>
#include <archive.h>
#include <archive_entry.h>

#include <fcntl.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_bKit_clicked();
    void init();
    void on_bHOME_clicked();

    void on_bBL_clicked();

    void on_bUNOS_clicked();
    void on_pRNOS_clicked();
    void on_pCROM_clicked();
    void on_pMFL_clicked();

    void on_cbWipe_stateChanged(int arg1);
    void on_cbLogi_stateChanged(int arg1);
    void on_cb2Slo_stateChanged(int arg1);
    void on_cbAVB_stateChanged(int arg1);
    void on_cbDCheck_stateChanged(int arg1);

    void on_bAdd_clicked();

    void on_bRm_clicked();

    void on_bBLConf_clicked();

    void on_bRefresh_clicked();


    void on_actionAbout_QT_triggered();

    void on_btnOptions_clicked();

    void on_bADB_clicked();

private:
    Ui::MainWindow *ui;
    QProcess *fastboot;
    QProcess *adb;
};
#endif // MAINWINDOW_H
