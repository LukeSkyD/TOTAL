#ifndef EXTRACTOR_H
#define EXTRACTOR_H

/*#include <archive.h>
#include <archive_entry.h>*/
#include <QString>
#include <QDir>
#include <QProcess>
#include <QRunnable>
#include <QThreadPool>
#include "updMetadata/update_metadata.pb.h"


QStringList extractArchive(const QString input, const QString output);
void extractBin(const QString input, QDir output);
void extractPartition(QString input, QString output, const chromeos_update_engine::PartitionUpdate &partition, long long dataOffset, uint32_t block_size);

#endif // EXTRACTOR_H
