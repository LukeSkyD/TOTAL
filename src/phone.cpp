#include <QMessageBox>
#include <QProcess>
#include <QDebug>
#include <QThread>
#include <QObject>
#include <QCoreApplication>
#include "phone.h"

char blFlags = 0;
char setFlags = 0;
unsigned int devId = 0;
QString fastbootPath = "";
QString protocol = "";
QString device = "";
QString modified = "";

const devices device_list[] = {
    {0, "Nothing", "Phone (1)", "A063", "spacewar",
        {"boot", "vendor_boot", "dtbo"},
        {"abl", "aop", "bluetooth", "cpucp", "devcfg", "dsp", "dtbo", "featenabler", "hyp", "imagefv", "keymaster", "modem", "multiimgoem", "qupfw", "shrm", "tz", "uefisecapp", "xbl", "xbl_config"},
        {"system", "system_ext", "product", "vendor", "odm"},
        {"vbmeta", "vbmeta_system"}
    },
    {1, "Nothing", "Phone (2)", "A065", "taro",
        {"boot", "vendor_boot", "dtbo", "recovery"},
        {"abl", "aop", "aop_config", "bluetooth", "cpucp", "devcfg", "dsp", "featenabler", "hyp", "imagefv", "keymaster", "modem", "multiimgoem", "multiimgqti", "qupfw", "qweslicstore", "shrm", "tz", "uefi", "uefisecapp", "xbl", "xbl_config", "xbl_ramdump"},
        {"system", "system_ext", "product", "vendor", "vendor_dlkm", "odm"},
        {"vbmeta", "vbmeta_system", "vbmeta_vendor"}
    },
    {2, "Nothing", "Phone (2a)", "A142", "k6886v1_64",
        {"boot", "dtbo", "init_boot", "vendor_boot"},
        {"apusys", "audio_dsp", "ccu", "connsys_bt", "connsys_gnss", "connsys_wifi", "dpm", "gpueb", "gz", "lk", "logo", "mcf_ota", "mcupm", "md1img", "mvpu_algo", "pi_img", "scp", "spmfw", "sspm", "tee", "vcp"},
        {"odm", "vendor", "system_ext", "system"},
        {"vbmeta", "vbmeta_system", "vbmeta_vendor"}
    }
};

void checkFastboot(QString path)
{
    QProcess process;
    process.startCommand(path + "fastboot --version");
    process.waitForFinished();
    QString output = process.readAllStandardOutput();

    if (output.contains("Installed as")) {
        QStringList list = output.split("Installed as")[1].split("fastboot.exe");
        fastbootPath = list[0];
        setFlags = 0x01;
        return;
    }
    QMessageBox::warning(nullptr, QObject::tr("Error"), QObject::tr("Fastboot not found in the specified path."));
    setFlags = 0x00;
    return;
}


// old deviceCheck implementatio single thread
void deviceCheck(bool atBoot){
    QProcess process;
    QString output = "";
    QString error = "";
    QStringList list;
    QString model = "";

    //start an adb server
    process.startCommand(fastbootPath + "adb start-server");
    process.waitForFinished();

    //check for the device, first with adb, then fastboot, then edl
    // check the model of the phone with ro.product.odm.model
    process.startCommand(fastbootPath + "adb shell getprop ro.product.odm.model");
    process.waitForFinished();
    output = process.readAllStandardOutput();

    if (output != ""){
        list = output.split("\n");
        model = list.length() > 2 ? list[2].trimmed() : list[0].trimmed();

        //check if the model is in device_list.adb_model
        for (int i = 0; i < sizeof(*device_list)/sizeof(devices); i++){
            if (model == device_list[i].adb_model){
                protocol = "adb";
                setFlags |= 0x12;
                device = device_list[i].vendor + " " + device_list[i].device_name;
                devId = device_list[i].id;

                //check if rooted
                process.startCommand(fastbootPath + "adb shell su");
                process.waitForFinished();
                output = process.readAllStandardOutput();
                if (output!=""){
                    modified = " (rooted)";
                    setFlags |= 0x04;
                }
                return;
            }
        }
    }


    if (protocol == "") {
        // check if user has booted fastbootd instead of bootloader. fastbootd returns vendor-fingerprint
        process.startCommand(fastbootPath + "fastboot getvar vendor-fingerprint");
        if (process.waitForFinished(250 + (1250*(!atBoot)))){
            output = process.readAllStandardOutput();
            error = process.readAllStandardError();

            if (!(output.contains("Nothing") || error.contains("Nothing"))){
                // check for bootloader mode
                process.startCommand(fastbootPath + "fastboot getvar product");
                process.waitForFinished(250);
                output = process.readAllStandardOutput().split('\n')[0].trimmed().toLower();
                error = process.readAllStandardError().split('\n')[0].trimmed().toLower();
                model = output != "" ? ( output.split(':')[1].trimmed()) : ( error != "" ? ( error.split(":")[1].trimmed()) : "" );

                //check if the model is in device_list.bl_model
                for (int i = 0; i < sizeof(device_list)/sizeof(devices); i++){
                    if (model == device_list[i].bl_model){
                        protocol = "bl";
                        setFlags |= 0x22;
                        device = device_list[i].vendor + " " + device_list[i].device_name;
                        devId = device_list[i].id;
                        // check if unlocked
                        process.startCommand(fastbootPath + "fastboot getvar unlocked");
                        process.waitForFinished();
                        output = process.readAllStandardOutput();
                        output = output == "" ? process.readAllStandardError() : output;
                        if (output.contains("yes")){
                            modified = " (unlocked)";
                            setFlags |= 0x08;
                        }
                        break;
                    }
                }
            } else {
                protocol = "fd";
            }
        }
    }
    /*
    else {
    //edl mode check to be implemented
        ui->lDevice->setText("Device connected in EDL mode");
    }*/
}


