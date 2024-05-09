#include <QDir>
#include <QFile>
#include <QMessageBox>
#include "extractor.h"
#include "fileManager.h"
#include "phone.h"

void sanitizeList(QListWidget *list){
    // Sanitize the list of files:
    // - delete all the files terminating with .00X and adds back only the first file .001
    for (int i = 0; i < list->count(); i++) {
        QString path = list->item(i)->text();

        if (path.contains(".7z.00")) {
            if (!path.endsWith(".001")) {
                // remove from the list
                list->takeItem(i);
                i--;

                // check .001 file exist in the list, if not add it
                bool found = false;
                for (int j = 0; j < list->count(); j++) {
                    if (list->item(j)->text() == path.left(path.length() - 1) + "1") {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    // check if the file exists
                    if (QFile::exists(path.left(path.length() - 1) + "1")) {
                        list->addItem(path.left(path.length() - 1) + "1");
                    } else {
                        QMessageBox::warning(nullptr, QObject::tr("Error"), path.left(path.length() - 1) + "1" + QObject::tr("\nFile not found."));
                        return;
                    }
                }
            }
        }
    }
}

void folderCheck(QDir dir){
    // Check with QDir if extTemp in the program diretory is already present, if not create it
    if (!dir.exists()) {
        dir.mkdir(".");

    } else {
        // we delete every file inside the directory
        dir.setNameFilters(QStringList() << "*.*");
        dir.setFilter(QDir::Files);
        foreach(QString dirFile, dir.entryList()) {
            dir.remove(dirFile);
        }
    }
}


bool manageBL(QDir outputDir, QListWidget *list){
    for (int i =0; i < list->count(); i++){
        QStringList extractFiles;
        QString workingOn = list->item(i)->text();

        if (workingOn.endsWith(".001")){
            // merges files
            QString fileName = workingOn.split('/').last().remove(".001");
            QString inputFile = workingOn.left(workingOn.lastIndexOf(".001")).replace("/", "\\") + ".*";
            QString outputFile = outputDir.absolutePath().replace('/', "\\") + "\\" + fileName;
            qDebug() << outputFile;

            QProcess process;
            process.start(WIN32 ? "cmd" : "cat", WIN32 ? QStringList() << "/c" << "copy" << "/B" << inputFile << outputFile : QStringList() << inputFile << " > " << outputFile);
            process.waitForFinished();

            workingOn = outputFile.replace('\\', '/');
            qDebug() << workingOn;
        }

        if (workingOn.contains(".7z")){
            extractFiles = extractArchive(workingOn, outputDir.absolutePath());
            if (workingOn != list->item(i)->text())
                QFile::remove(workingOn);

            for (const QString& file : extractFiles){
                if (file.contains(".zip") || file.contains(".img")){
                    workingOn = file;
                    break;
                }
            }
        }

        if (workingOn.contains(".zip")){
            extractFiles = extractArchive(workingOn, outputDir.absolutePath());
            if (workingOn != list->item(i)->text())
                QFile::remove(workingOn);

            // Search for payload.bin inside extractedFiles
            for (const QString& file : extractFiles){
                if (file.contains("payload.bin")){
                    // Check for /META-INF/com/android/metadata just extracted in the same folder if it has any line starting with "PRE_OTA_VERSION"
                    //if so it's an incremental package and should be deleted from extTemp, if not set workingOn to payload.bin
                    QFile properties(outputDir.absolutePath() + "/META-INF/com/android/metadata");
                    if (properties.open(QIODevice::ReadOnly)){
                        QTextStream stream(&properties);
                        while (!stream.atEnd()){
                            QString line = stream.readLine();
                            if (line.startsWith("pre-build")){
                                QFile::remove(outputDir.absolutePath() + "/payload.bin");
                                QFile::remove(outputDir.absolutePath() + "/apex_info.pb");
                                QFile::remove(outputDir.absolutePath() + "/payload_properties.txt");
                                QFile::remove(outputDir.absolutePath() + "/care_map.pb");
                                QDir(outputDir.absolutePath() + "/META-INF").removeRecursively();
                                properties.close();
                                return false;
                            }
                        }
                    }

                    // It's a FULL OTA, set payload.bin to workingOn
                    workingOn = file;
                    properties.close();
                    QFile::remove(outputDir.absolutePath() + "/apex_info.pb");
                    QFile::remove(outputDir.absolutePath() + "/payload_properties.txt");
                    QFile::remove(outputDir.absolutePath() + "/care_map.pb");
                    QDir(outputDir.absolutePath() + "/META-INF").removeRecursively();
                    break;
                }
            }
        }

        if (workingOn.contains(".bin")){
            extractBin(workingOn, outputDir);

            if (workingOn != list->item(i)->text())
                QFile::remove(workingOn);
        }


        if (workingOn == list->item(i)->text()){
            // scan the folder for any .img file and hard link them to the output directory
            QStringList filters = workingOn.split('/');
            filters.removeLast();
            QDir workingOnDir(filters.join('/'));

            filters.clear();
            filters << "*.img";

            QStringList imgFiles = workingOnDir.entryList(filters, QDir::Files);
            for (const QString& imgFile : imgFiles)
                !QFileInfo::exists(outputDir.absolutePath() + "/" + imgFile) && QFile::link((workingOnDir.absolutePath() + "/" + imgFile), outputDir.absolutePath() + "/" + imgFile);
        }

        // check if every device partition is present from device_list checking for devId
        for (int i = 0; i < sizeof(*device_list)/sizeof(devices); i++){
            if (devId == device_list[i].id){
                // check if all the partitions are present
                for (const QString& partition : device_list[i].boot_partitions){
                    if (!QFileInfo::exists(outputDir.absolutePath() + "/" + partition + ".img"))
                        break;
                }
                for (const QString& partition : device_list[i].firmware_partitions){
                    if (!QFileInfo::exists(outputDir.absolutePath() + "/" + partition + ".img"))
                        break;
                }
                for (const QString& partition : device_list[i].logical_partitions){
                    if (!QFileInfo::exists(outputDir.absolutePath() + "/" + partition + ".img"))
                        break;
                }
                for (const QString& partition : device_list[i].vbmeta_partitions){
                    if (!QFileInfo::exists(outputDir.absolutePath() + "/" + partition + ".img"))
                        break;
                }

                return true;
            }
        }
    }

    return false;
}
