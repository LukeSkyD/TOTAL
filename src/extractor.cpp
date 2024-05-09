#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QThreadPool>
#include <QMessageBox>
#include <QRunnable>
#include <archive.h>
#include <archive_entry.h>
#include <lzma.h>
#include "extractor.h"
#include "updMetadata/update_metadata.pb.h"

void workerThread(const QString& inputPath, const QDir& outputDir, long long dataOffset, uint32_t block_size, std::atomic<int>& currentIndex, const std::vector<chromeos_update_engine::PartitionUpdate>& partitions);
void extractBin(const QString input, QDir outputDir);

class Worker : public QRunnable {
public:
    Worker(const QString& inputPath, const QDir& outputDir, long long dataOffset, uint32_t block_size, const QList<chromeos_update_engine::PartitionUpdate>& partitions)
        : inputPath(inputPath), outputDir(outputDir), dataOffset(dataOffset), block_size(block_size), partitions(partitions) {}

    void run() override {
        for (const auto& partition : partitions) {
            QString outputPath = outputDir.absoluteFilePath(QString::fromStdString(partition.partition_name()) + ".img");
            extractPartition(inputPath, outputPath, partition, dataOffset, block_size);
        }
    }

private:
    QString inputPath;
    QDir outputDir;
    long long dataOffset;
    uint32_t block_size;
    QList<chromeos_update_engine::PartitionUpdate> partitions;
};


static int copy_data(struct archive *in, struct archive *out)
{
    int r;
    const void *buff;
    size_t size;
    la_int64_t offset;

    for (;;) {
        r = archive_read_data_block(in, &buff, &size, &offset);
        if (r == ARCHIVE_EOF)
            return (ARCHIVE_OK);
        if (r < ARCHIVE_OK)
            return (r);
        r = archive_write_data_block(out, buff, size, offset);
        if (r < ARCHIVE_OK) {
            fprintf(stderr, "%s\n", archive_error_string(out));
            return (r);
        }
    }
}

QStringList extractArchive(QString input, const QString outputDir)
{

    struct archive *a;
    struct archive *ext;
    struct archive_entry *entry;
    QStringList extrFiles;
    int r;

    a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS);
    archive_write_disk_set_standard_lookup(ext);
    if (archive_read_open_filename(a, input.toStdString().c_str(), 10240))
        exit(1);

    qDebug() << input << " -> " << outputDir;
    for (;;) {
        r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF)
            break;
        if (r < ARCHIVE_OK)
            fprintf(stderr, "%s\n", archive_error_string(a));
        if (r < ARCHIVE_WARN)
            exit(1);

        // Update the pathname of the entry to include the output directory
        archive_entry_set_pathname(entry, (outputDir + '/' + archive_entry_pathname(entry)).toStdString().c_str());
        // Save the list of extracted files
        extrFiles << archive_entry_pathname(entry);

        r = archive_write_header(ext, entry);
        if (r < ARCHIVE_OK)
            fprintf(stderr, "%s\n", archive_error_string(ext));
        else if (archive_entry_size(entry) > 0) {
            r = copy_data(a, ext);
            if (r < ARCHIVE_OK)
                fprintf(stderr, "%s\n", archive_error_string(ext));
            if (r < ARCHIVE_WARN)
                exit(1);
        }
        r = archive_write_finish_entry(ext);
        if (r < ARCHIVE_OK)
            fprintf(stderr, "%s\n", archive_error_string(ext));
        if (r < ARCHIVE_WARN)
            exit(1);
    }
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    return extrFiles;
}

void extractPartition(QString input, QString output, const chromeos_update_engine::PartitionUpdate &partition, long long dataOffset, uint32_t block_size){
    /*if (!(partition.partition_name() == "abl" || partition.partition_name() == "boot"))
            continue;*/
    // open input file
    QFile input_file(input);
    QDataStream in(&input_file);
    if (!input_file.open(QIODevice::ReadOnly)){
        QMessageBox::critical(nullptr, QObject::tr("Error"), QObject::tr("Error opening payload.bin"));
        return;
    }

    // open output file
    QFile output_file(output);
    if (!output_file.open(QIODevice::WriteOnly)){
        QMessageBox::critical(nullptr, QObject::tr("Error"), QObject::tr("Error opening output file\nTry moving the program's folder to the desktop"));
        input_file.close();
        return;
    }
    QDataStream out(&output_file);


    qDebug("Working on %s", partition.partition_name().c_str());

    for (const auto& op: partition.operations()){
        // operation infos:
        const uint64_t opDataOffset = dataOffset + op.data_offset();
        const uint64_t dataLength = op.data_length();
        const uint64_t expectedUncompressedLength = op.dst_extents()[0].num_blocks() * block_size;

        // lzma stream definition
        lzma_stream strm = LZMA_STREAM_INIT;
        lzma_ret ret = lzma_stream_decoder(&strm, UINT64_MAX, LZMA_CONCATENATED);
        if (ret != LZMA_OK){
            qDebug("Error: LZMA stream initialization failed");
            input_file.close();
            output_file.close();
            return;
        }

        // data worked on
        char *inputData = new char[dataLength], *outputData = new char[expectedUncompressedLength], *buffer = new char[expectedUncompressedLength];
        // IO operations
        // Archive definitions
        struct archive* a = archive_read_new();
        archive_read_support_format_raw(a);
        archive_read_support_filter_bzip2(a);

        // archive_entry definition
        struct archive_entry* entry;
        // libarchive status
        int result = 0;

        // Input file seek
        input_file.seek(opDataOffset);
        // Read dataLength bytes from the file without qdatastream
        input_file.read(inputData, dataLength);

        // Seek the output file to the right position
        output_file.seek(op.dst_extents()[0].start_block() * block_size);

        long long writtenData = 0;
        long long bytesRead = 0;

        switch(op.type()){
        case op.REPLACE_XZ:
            // lzma stream initialization
            strm.next_in = (unsigned char *)inputData;
            strm.avail_in = dataLength;
            strm.next_out = (unsigned char *)outputData;
            strm.avail_out = expectedUncompressedLength;

            ret = lzma_code(&strm, LZMA_RUN);
            if (ret != LZMA_OK){
                qDebug("Error: LZMA decompression failed");
                archive_read_free(a);
                input_file.close();
                output_file.close();
                return;
            }

            // Writing the decompressed data to the output file
            out.writeRawData(outputData, expectedUncompressedLength);
            break;

        case op.REPLACE_BZ:
            // Archive initialization
            result = archive_read_open_memory(a, inputData, dataLength);
            if (result != ARCHIVE_OK) {
                std::cerr << "Error: Opening compressed data failed" << std::endl;
                std::cerr << archive_error_string(a) << std::endl;
                archive_read_free(a);
                input_file.close();
                output_file.close();
                return;
            }

            // Reading and decompressing the data
            while ((archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
                while ((bytesRead = archive_read_data(a, buffer, expectedUncompressedLength)) > 0) {
                    memcpy(outputData + writtenData, buffer, bytesRead);
                    writtenData += bytesRead;
                }
                if (bytesRead < 0) {
                    std::cerr << "Error: Decompressing data failed" << std::endl;
                    std::cerr << archive_error_string(a) << std::endl;
                    std::cerr << "Bytes offset from payload.bin: " << opDataOffset << std::endl;
                    std::cerr << "Data end: " << dataLength + opDataOffset << std::endl;
                    std::cerr << "expectedUncompressedLength: " << expectedUncompressedLength << std::endl;

                    archive_read_free(a);
                    input_file.close();
                    output_file.close();
                    return;
                }
            }

            // Archive closing
            archive_read_free(a);

            // Writing the decompressed data to the output file
            out.writeRawData(outputData, expectedUncompressedLength);
            break;

        case op.REPLACE:
            // Writing the data to the output file
            out.writeRawData(inputData, dataLength);
            break;

        case op.ZERO:
            // Writing zeros to the output file
            for (const auto& extent : op.dst_extents()){
                output_file.seek(extent.start_block()*block_size);
                outputData = new char[extent.num_blocks()*block_size];
                memset(outputData, 0, extent.num_blocks()*block_size);
                out.writeRawData(outputData, extent.num_blocks()*block_size);
            }
            break;

        default:
            QMessageBox::critical(nullptr, QObject::tr("Error"), QObject::tr("Unsupported payload"));
            input_file.close();
            output_file.close();
            return;
        }


        // check the hash of the operation
        /*if (op.has_data_sha256_hash()){
                        const char *expectedHash = op.data_sha256_hash().c_str();
                        // check the hash of the operation
                        const char *calculatedHash=QCryptographicHash::hash(outputData, QCryptographicHash::Sha256).toStdString().c_str();

                        if (calculatedHash != expectedHash){
                            qDebug("Error: Hash mismatch");
                            qDebug("Expected hash: %s", expectedHash);
                            qDebug("Calculated hash: %s", calculatedHash.toStdString().c_str());
                            return;
                        }
                    }*/
    }
}


void extractBin(const QString input, QDir outputDir){
    QFile file(input);
    QDataStream in(&file);

    if (!(file.open(QIODevice::ReadOnly))){
        QMessageBox::critical(nullptr, QObject::tr("Error"), input + QObject::tr("\nError opening file"));
        return;
    }

    // Header definitions
    uint32_t magic, manifestSignatureSize;
    uint64_t version, manifestSize;
    long long dataOffset;

    // Manifest structure definition
    chromeos_update_engine::DeltaArchiveManifest manifest;

    // Header checks
    // Read big endian data
    in.setByteOrder(QDataStream::BigEndian);

    // Magic & version check
    in >> magic;
    in >> version;

    if (magic != 0x43724155 || version != 0x00000002){
        file.close();
        QMessageBox::critical(nullptr, QObject::tr("Error"), QObject::tr("Invalid payload file"));
        return;
    }

    // Read manifest size and define manifest buffer
    in >> manifestSize;
    char* manifestBuffer = new char[manifestSize];

    // Read manifest signature size
    in >> manifestSignatureSize;

    // Set data offset
    dataOffset = 24 + manifestSignatureSize + manifestSize;

    // Read manifest data
    in.readRawData(manifestBuffer, manifestSize);

    // Close the file
    file.close();


    // Deserialize manifest
    if (!manifest.ParseFromArray(manifestBuffer, manifestSize)){
        file.close();
        QMessageBox::critical(nullptr, QObject::tr("Error"), QObject::tr("Error parsing payload manifest, could be a bad .bin file"));
        return;
    }

    // Define block size
    const uint32_t block_size = manifest.block_size();
    const auto& partitions = manifest.partitions();

    // Define max threads
    const int maxThreads = 4;

    // thread pool init
    QThreadPool pool;
    pool.setMaxThreadCount(maxThreads);

    // each thread extracts n
    for (int i = 0; i < partitions.size(); i += maxThreads) {
        int endIndex = qMin(i + maxThreads, partitions.size());
        QList<chromeos_update_engine::PartitionUpdate> subPartitions(partitions.begin() + i, partitions.begin() + endIndex);
        Worker* worker = new Worker(input, outputDir, dataOffset, block_size, subPartitions);
        pool.start(worker);
    }

    // Wait for all threads to finish
    pool.waitForDone();


}




