#include "bootloader.h"
#include "phone.h"

void bootloader_flasher(QDir outputDir){
    // remembering fastbootPath from phone.cpp, we set active slot to a with fastboot --set-active=a
    QProcess process;
    process.start(fastbootPath + "fastboot --set-active=a");
    process.waitForFinished();

    // Flashing each QStringList boot_partitions, if both slot bit is set, we flash to both slots a and b
    for (QString partition : device_list[devId-1].boot_partitions){
        if (blFlags & 0x02){
            for (QString slot : {"_a", "_b"}){
                process.start(fastbootPath + "fastboot flash " + partition + slot + " " + outputDir.absolutePath() + "/" + partition + ".img");
                process.waitForFinished();
            }
        } else {
            process.start(fastbootPath + "fastboot flash " + partition + " " + outputDir.absolutePath() + "/" + partition + ".img");
            process.waitForFinished();
        }
    }

    // Firmware Flash which is different from 1 and 2 to 2a
    if (devId == 0x01 || devId == 0x02){
        // if the phone is a phone 1 or 2, reboot to fastboot
        process.start(fastbootPath + "fastboot reboot fastboot");
        process.waitForFinished();

        // flashing each QStringList firmware_partitions
        for (QString partition : device_list[devId-1].firmware_partitions){
            process.start(fastbootPath + "fastboot flash --slot=" + ((blFlags & 0x02) ? "all"  : "a") + partition + " " + outputDir.absolutePath() + "/" + partition + ".img");
            process.waitForFinished();
        }

        // Flashing vbmeta with avb on or off depending on the flag
        process.start(fastbootPath + "fastboot flash --slot=" + ((blFlags & 0x02) ? "all"  : "a") + "vbmeta " + ((blFlags & 0x08) ? "--disable-verity --disable-verification " : "") + outputDir.absolutePath() + "/vbmeta.img");

    } else {
        // phone 2a
        // flashing firmware partitions
        for (QString partition : device_list[devId-1].firmware_partitions){
            if (blFlags & 0x02){
                for (QString slot : {"_a", "_b"}){
                    process.start(fastbootPath + "fastboot flash " + partition + slot + " " + outputDir.absolutePath() + "/" + partition + ".img");
                    process.waitForFinished();
                }
            } else {
                process.start(fastbootPath + "fastboot flash " + partition + "_a " + outputDir.absolutePath() + "/" + partition + ".img");
                process.waitForFinished();
            }
        }
        // flashing preloader_a and preloader_b
        if (blFlags & 0x02){
            for (QString slot : {"_a", "_b"}){
                process.start(fastbootPath + "fastboot flash preloader" + slot + " " + outputDir.absolutePath() + "/preloader_raw.img");
                process.waitForFinished();
            }
        } else {
            process.start(fastbootPath + "fastboot flash preloader_a " + outputDir.absolutePath() + "/preloader_raw.img");
            process.waitForFinished();
        }
        // flashing vbmeta
        if (blFlags & 0x02) {
            for (QString partition : device_list[devId-1].vbmeta_partitions){
                for (QString slot : {"_a", "_b"}){
                    process.start(fastbootPath + "fastboot flash " + partition + slot + " " + ((blFlags & 0x08) ? "--disable-verity --disable-verification " : "") + outputDir.absolutePath() + "/" + partition + ".img");
                    process.waitForFinished();
                }
            }
        } else {
            for (QString partition : device_list[devId-1].vbmeta_partitions){
                process.start(fastbootPath + "fastboot flash " + partition + "_a " + ((blFlags & 0x08) ? "--disable-verity --disable-verification " : "") + outputDir.absolutePath() + "/" + partition + ".img");
                process.waitForFinished();
            }
        }
        // reboot to fastboot
        process.start(fastbootPath + "fastboot reboot fastboot");
        process.waitForFinished();
    }


    // flashing logical partitions
    if (blFlags & 0x04) {
        // check for super.img in the outputDir
        if (!QFileInfo::exists(outputDir.absolutePath() + "/super.img")){
            if (QFileInfo::exists(outputDir.absolutePath() + "/super_empty.img")){
                // call wipeSuperPartition
                process.start(fastbootPath + "fastboot wipe-super " + outputDir.absolutePath() + "/super_empty.img");
                process.waitForFinished();
            } else {
                // call resizeLogicalPartition
                // for phone 1 and 2
                if (devId == 0x01 || devId == 0x02){
                    for (QString partition : device_list[devId-1].logical_partitions){
                        for (QString slot : {"_a", "_b"}){
                            process.start(fastbootPath + "fastboot delete-logical-partition " + partition + slot + "-cow");
                            process.waitForFinished();
                            process.start(fastbootPath + "fastboot delete-logical-partition " + partition + slot);
                            process.waitForFinished();
                            process.start(fastbootPath + "fastboot create-logical-partition " + partition + slot + ", 1");
                            process.waitForFinished();
                        }
                    }
                } else {
                    for (QString partition : device_list[devId-1].logical_partitions){
                        for (QString slot : {"_a", "_b"}){
                            process.start(fastbootPath + "fastboot delete-logical-partition " + partition + slot + "-cow");
                            process.waitForFinished();
                            process.start(fastbootPath + "fastboot delete-logical-partition " + partition + slot);
                            process.waitForFinished();
                        }
                        process.start(fastbootPath + "fastboot create-logical-partition " + partition + "_a" + ", 1");
                        process.waitForFinished();
                    }
                }
            }
            // flash each logical partition
            for (QString partition : device_list[devId-1].logical_partitions){
                process.start(fastbootPath + "fastboot flash " + partition + "_a " + outputDir.absolutePath() + "/" + partition + ".img");
                process.waitForFinished();
            }
        } else {
            // flash super.img
            process.start(fastbootPath + "fastboot flash super " + outputDir.absolutePath() + "/super.img");
            process.waitForFinished();
        }
    }

    // reboot to bootloader
    process.start(fastbootPath + "fastboot reboot bootloader");
    process.waitForFinished();

    // wipe data if the wipe flag is set
    if (blFlags & 0x01){
        process.start(fastbootPath + "fastboot -w");
        process.waitForFinished();
    }

    // reboot to system
    process.start(fastbootPath + "fastboot reboot");
    process.waitForFinished();

}

void bootloader_flasher2(QDir outputDir){
    // remembering fastbootPath from phone.cpp, we set active slot to a with fastboot --set-active=a
    QProcess process;
    process.start(fastbootPath + "fastboot --set-active=a");
    process.waitForFinished();

    // Flashing each QStringList boot_partitions, if both slot bit is set, we flash to both slots a and b
    for (QString partition : device_list[devId-1].boot_partitions){
        process.start(fastbootPath + "fastboot flash " + ((blFlags & 0x02) ? "--slot=all " : "") + partition + " " + outputDir.absolutePath() + "/" + partition + ".img");
        process.waitForFinished();
    }

    // Firmware Flash which is different from 1 and 2 to 2a
    if (devId == 0x01 || devId == 0x02){
        // if the phone is a phone 1 or 2, reboot to fastboot
        process.start(fastbootPath + "fastboot reboot fastboot");
        process.waitForFinished();

        // flashing each QStringList firmware_partitions
        for (QString partition : device_list[devId-1].firmware_partitions){
            process.start(fastbootPath + "fastboot flash --slot=" + ((blFlags & 0x02) ? "all"  : "a") + partition + " " + outputDir.absolutePath() + "/" + partition + ".img");
            process.waitForFinished();
        }

        // Flashing vbmeta with avb on or off depending on the flag
        process.start(fastbootPath + "fastboot flash --slot=" + ((blFlags & 0x02) ? "all"  : "a") + "vbmeta " + ((blFlags & 0x08) ? "--disable-verity --disable-verification " : "") + outputDir.absolutePath() + "/vbmeta.img");

    } else {
        // phone 2a
        // flashing firmware partitions
        for (QString partition : device_list[devId-1].firmware_partitions){
            process.start(fastbootPath + "fastboot flash " + ((blFlags & 0x02) ? "--slot=all " : "") + partition + " " + outputDir.absolutePath() + "/" + partition + ".img");
            process.waitForFinished();
        }
        // flashing preloader_a and preloader_b
        process.start(fastbootPath + "fastboot flash " + ((blFlags & 0x02) ? "--slot=all " : "") + "preloader " + outputDir.absolutePath() + "/preloader_raw.img");
        process.waitForFinished();
        // flashing vbmeta
        for (QString partition : device_list[devId-1].vbmeta_partitions){
            process.start(fastbootPath + "fastboot flash " + ((blFlags & 0x02) ? "--slot=all " : "") + partition + " " + ((blFlags & 0x08) ? "--disable-verity --disable-verification " : "") + outputDir.absolutePath() + "/" + partition + ".img");
            process.waitForFinished();
        }
        // reboot to fastboot
        process.start(fastbootPath + "fastboot reboot fastboot");
        process.waitForFinished();
    }


    // flashing logical partitions
    if (blFlags & 0x04) {
        // check for super.img in the outputDir
        if (!QFileInfo::exists(outputDir.absolutePath() + "/super.img")){
            if (QFileInfo::exists(outputDir.absolutePath() + "/super_empty.img")){
                // call wipeSuperPartition
                process.start(fastbootPath + "fastboot wipe-super " + outputDir.absolutePath() + "/super_empty.img");
                process.waitForFinished();
            } else {
                // call resizeLogicalPartition
                // for phone 1 and 2
                if (devId == 0x01 || devId == 0x02){
                    for (QString partition : device_list[devId-1].logical_partitions){
                        process.start(fastbootPath + "fastboot delete-logical-partition " + ((blFlags & 0x02) ? "--slot=all " : "") + partition + "-cow");
                        process.waitForFinished();
                        process.start(fastbootPath + "fastboot delete-logical-partition " + ((blFlags & 0x02) ? "--slot=all " : "") + partition);
                        process.waitForFinished();
                        process.start(fastbootPath + "fastboot create-logical-partition " + ((blFlags & 0x02) ? "--slot=all " : "") + partition + ", 1");
                        process.waitForFinished();
                    }
                } else {
                    for (QString partition : device_list[devId-1].logical_partitions){
                        process.start(fastbootPath + "fastboot delete-logical-partition " + ((blFlags & 0x02) ? "--slot=all " : "") + partition + "-cow");
                        process.waitForFinished();
                        process.start(fastbootPath + "fastboot delete-logical-partition " + ((blFlags & 0x02) ? "--slot=all " : "") + partition);
                        process.waitForFinished();
                        process.start(fastbootPath + "fastboot create-logical-partition " + partition + "_a" + ", 1");
                        process.waitForFinished();
                    }
                }
            }
            // flash each logical partition
            for (QString partition : device_list[devId-1].logical_partitions){
                process.start(fastbootPath + "fastboot flash " + partition + "_a " + outputDir.absolutePath() + "/" + partition + ".img");
                process.waitForFinished();
            }
        } else {
            // flash super.img
            process.start(fastbootPath + "fastboot flash super " + outputDir.absolutePath() + "/super.img");
            process.waitForFinished();
        }
    }

    // reboot to bootloader
    process.start(fastbootPath + "fastboot reboot bootloader");
    process.waitForFinished();

    // wipe data if the wipe flag is set
    if (blFlags & 0x01){
        process.start(fastbootPath + "fastboot -w");
        process.waitForFinished();
    }

    // reboot to system
    process.start(fastbootPath + "fastboot reboot");
    process.waitForFinished();

}
