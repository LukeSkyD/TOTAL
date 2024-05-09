#ifndef PHONE_H
#define PHONE_H

#include <QStringList>
#include <QThread>
#include <QProcess>


void checkFastboot(QString path);
void deviceCheck(bool atBoot);

struct devices {
    unsigned int id;
    QString vendor;
    QString device_name;
    QString adb_model;
    QString bl_model;
    QStringList boot_partitions;
    QStringList firmware_partitions;
    QStringList logical_partitions;
    QStringList vbmeta_partitions;
};

extern QString protocol;
extern QString device;
extern QString modified;

extern const devices device_list[];

// bootloaderFlags (blFlags)
// 0x01 - Wipe, 0x02 - Flash to both slots, 0x04 - Flash logical partitions, 0x08 - Disable AVB, 0x10 - Disable partition integrity check, 0x20 - Path correctly set
// settingsFlags (setFlags)
// 0x01 Fastboot correctly set, 0x02 Found device, 0x04 - Rooted, 0x08 - Unlocked, 0x10 - ADB mode, 0x20 - Fastboot mode, 0x40 - EDL mode
// devId (devId)
// 0x01 NP1, 0x02 NP2, 0x03 NP2a
extern char blFlags;
extern char setFlags;
extern unsigned int devId;
extern QString fastbootPath;


class DeviceCheckThread : public QThread {
    Q_OBJECT
public:
    explicit DeviceCheckThread(bool atBoot) : atBoot(atBoot) {}

signals:
    void deviceChecked(QString protocol, QString device, QString modified, int setFlags, int devId);

protected:
    void run() override {
        QProcess process;
        QString output = "";
        QString error = "";
        QStringList list;
        QString model = "";

        //start an adb server
        process.start(fastbootPath + "adb start-server");
        process.waitForFinished();

        //check for the device, first with adb, then fastboot, then edl
        // check the model of the phone with ro.product.odm.model
        process.start(fastbootPath + "adb shell getprop ro.product.odm.model");
        process.waitForFinished();
        output = process.readAllStandardOutput();

        if (output != ""){
            list = output.split("\n");
            model = list.length() > 2 ? list[2].trimmed() : list[0].trimmed();

            //check if the model is in device_list.adb_model
            for (int i = 0; i < sizeof(*device_list)/sizeof(devices); i++){
                if (model == device_list[i].adb_model){
                    emit deviceChecked("adb", device_list[i].vendor + " " + device_list[i].device_name, modified, 0x12, device_list[i].id);

                    //check if rooted
                    process.start(fastbootPath + "adb shell su");
                    process.waitForFinished();
                    output = process.readAllStandardOutput();
                    if (output!=""){
                        modified = " (rooted)";
                        emit deviceChecked("adb", device_list[i].vendor + " " + device_list[i].device_name, modified, 0x04, device_list[i].id);
                    }
                    return;
                }
            }
        }

        if (protocol == "") {
            // check if user has booted fastbootd instead of bootloader. fastbootd returns vendor-fingerprint
            process.start(fastbootPath + "fastboot getvar vendor-fingerprint");
            if (process.waitForFinished(250 + (1250*(!atBoot)))){
                output = process.readAllStandardOutput();
                error = process.readAllStandardError();

                if (!(output.contains("Nothing") || error.contains("Nothing"))){
                    // check for bootloader mode
                    process.start(fastbootPath + "fastboot getvar product");
                    process.waitForFinished(250);
                    output = process.readAllStandardOutput().split('\n')[0].trimmed().toLower();
                    error = process.readAllStandardError().split('\n')[0].trimmed().toLower();
                    model = output != "" ? ( output.split(':')[1].trimmed()) : ( error != "" ? ( error.split(":")[1].trimmed()) : "" );

                    //check if the model is in device_list.bl_model
                    for (int i = 0; i < sizeof(*device_list)/sizeof(devices); i++){
                        if (model == device_list[i].bl_model){
                            emit deviceChecked("bl", device_list[i].vendor + " " + device_list[i].device_name, modified, 0x22, device_list[i].id);

                            // check if unlocked
                            process.start(fastbootPath + "fastboot getvar unlocked");
                            process.waitForFinished();
                            output = process.readAllStandardOutput();
                            output = output == "" ? process.readAllStandardError() : output;
                            if (output.contains("yes")){
                                modified = " (unlocked)";
                                emit deviceChecked("bl", device_list[i].vendor + " " + device_list[i].device_name, modified, 0x08, device_list[i].id);
                            }
                            break;
                        }
                    }
                } else {
                    emit deviceChecked("fd", "", "", 0, -1);
                }
            }
        }
    }

private:
    bool atBoot;
    QString fastbootPath = "";
    QString modified = "";
    QString protocol = "";
};


#endif // PHONE_H
