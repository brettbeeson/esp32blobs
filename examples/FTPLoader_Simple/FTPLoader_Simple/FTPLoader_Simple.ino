#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "FTPUploader.h"
#include <dirent.h>
#include <glob.h>
#include "FtpClient.h"
#include <WiFiManager.h>          // https://github.com/Brunez3BD/WIFIMANAGER-ESP32
#include "FS.h"
#include "SD_MMC.h"

#define MAX_BUF 100

#include "ArduinoLogRemoteDebug.h"

//FTPUploader ftp("13.236.175.255", "timelapse", "U88magine!");
FTPUploader ftp("monitor.phisaver.com", "timelapse", "U88magine!");

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); while (!Serial) {};
  Log.begin (LOG_LEVEL_VERBOSE, &Serial, false /* prefix */); // SILENT|FATAL|ERROR|WARNING|NOTICE|TRACE|VERBOSE
  // Use this instead of direct
  // esp_vfs_fat_sdmmc_mount
  // which doesn't (?) allow use for FS,File Arduino classes
  // SD_MMC.begin() does, and also linux/posix FILE* operations
  // can be used at /sdcard
  // SD_MMC.begin("/")  fails
  if (SD_MMC.begin()) {
    Serial.println("Started filesystem");
  } else {
    Serial.println("Failed to start filesystem");
    while (1) {};
  }

  Serial.println(FTPUploader::slash("/", "/tl/1/2", ""));
  Serial.println(FTPUploader::slash("/", "/tl/1", ""));
  Serial.println(FTPUploader::slash("/", "/tl", "/"));
  Serial.println(FTPUploader::slash("/", "tl", "/"));
  Serial.println(FTPUploader::slash("/", "tl/", "/"));
  Serial.println(FTPUploader::slash("", "/tl/2/", ""));
  Serial.println(FTPUploader::slash("/", "tl", "/"));

  
  WiFiManager wifiManager;
  if (!wifiManager.autoConnect("TimeLapseBlob")) {
    Serial.println("Failed to connect and hit timeout\n");
    while (1) {};
  }
  Serial.println("Ready to upload");

  try {
    //listFilesPosix();
    ftp.begin();
    ftp.uploadFile(SD_MMC, "index.htm");
    ftp.uploadFiles(SD_MMC, "/tl/", "/tl/", "", false /* remove */, true /* overwrite */, true /* sort */);
  } catch (const std::exception &e) {
    LOG_E("%s", e.what());
  }
}


void loop() {
}

/**
 * Faster to use Posix function, compared to File class
 */
void listFilesPosix() {
  
  DIR *folder;
  struct dirent *entry;
  int entries = 0;
  int files=0;
  folder = opendir("/sdcard/tl");
  while  (entry = readdir(folder))   {
    entries++;
    if (entry->d_type==1) files++;
    Serial.printf("File %3d: %s Type:%d\n",
           files,
           entry->d_name,
           entry->d_type
          );
  }
  Serial.printf("entries %d: files:%d\n",entries,files);
  closedir(folder);
}
