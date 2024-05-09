#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QDir>
#include <QListWidget>

//bool manageBL(QDir outputDir, const QString input);
bool manageBL(QDir outputDir, QListWidget *list);
void sanitizeList(QListWidget *list);
void folderCheck(QDir dir);

#endif // FILEMANAGER_H
