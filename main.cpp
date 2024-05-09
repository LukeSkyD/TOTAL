#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    QString systemLanguage = QLocale::system().name();

    // Cerca il file di traduzione per la lingua attiva sul sistema
    if (translator.load(":/translations/Totale_" + systemLanguage + ".qm")) {
        a.installTranslator(&translator);
    } else {
        qDebug() << "Failed to load the translation file for" << systemLanguage;
    }

    MainWindow w;
    w.show();
    return a.exec();
}
