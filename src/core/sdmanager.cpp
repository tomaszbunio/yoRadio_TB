//v0.9.686
#include "options.h"
#if SDC_CS!=255
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <vector>
#include <algorithm>
#include "vfs_api.h"
#include "sd_diskio.h"
//#define USE_SD
#include "config.h"
#include "sdmanager.h"
#include "display.h"
#include "player.h"

#if defined(SD_SPIPINS) || SD_HSPI
SPIClass  SDSPI(HSPI);
#define SDREALSPI SDSPI
#else
  #define SDREALSPI SPI
#endif

#ifndef SDSPISPEED
  #define SDSPISPEED 20000000
#endif

#ifndef SD_INDEX_YIELD_EVERY
  #define SD_INDEX_YIELD_EVERY 16
#endif
#ifndef SD_INDEX_PROGRESS_EVERY
  #define SD_INDEX_PROGRESS_EVERY 16
#endif

SDManager sdman(FSImplPtr(new VFSImpl()));

bool SDManager::start(){
  ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  vTaskDelay(10);
  if(!ready) ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  vTaskDelay(20);
  if(!ready) ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  vTaskDelay(50);
  if(!ready) ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  return ready;
}

void SDManager::stop(){
  end();
  ready = false;
}
#include "diskio_impl.h"
bool SDManager::cardPresent() {

  if(!ready) return false;
  if(sectorSize()<1) {
    return false;
  }
  uint8_t buff[sectorSize()] = { 0 };
  bool bread = readRAW(buff, 1);
  if(sectorSize()>0 && !bread) return false;
  return bread;
}

bool SDManager::_checkNoMedia(const char* path){
  if (path[strlen(path) - 1] == '/')
    snprintf(config.tmpBuf, sizeof(config.tmpBuf), "%s%s", path, ".nomedia");
  else
    snprintf(config.tmpBuf, sizeof(config.tmpBuf), "%s/%s", path, ".nomedia");
  bool nm = exists(config.tmpBuf);
  return nm;
}

bool SDManager::_endsWith (const char* base, const char* str) {
  int slen = strlen(str) - 1;
  const char *p = base + strlen(base) - 1;
  while(p > base && isspace(*p)) p--;
  p -= slen;
  if (p < base) return false;
  return (strncmp(p, str, slen) == 0);
}

void SDManager::listSD(File &plSDfile, File &plSDindex, const char* dirname, uint8_t levels) {
    File root = sdman.open(dirname);
    if (!root) {
        Serial.println("##[ERROR]#\tFailed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("##[ERROR]#\tNot a directory");
        return;
    }

    std::vector<String> dirs;
    std::vector<String> files;

    uint16_t scanCounter = 0;
    while (true) {
        if ((scanCounter++ % SD_INDEX_YIELD_EVERY) == 0) {
            vTaskDelay(1);
            player.loop();
        }
        bool isDir;
        String fileName = root.getNextFileName(&isDir);
        if (fileName.isEmpty()) break;
        if (isDir) {
            if (levels && !_checkNoMedia(fileName.c_str())) {
                dirs.push_back(fileName);
            }
        } else {
            String fnLower = fileName;
            fnLower.toLowerCase();
            if (fnLower.endsWith(".mp3") || fnLower.endsWith(".m4a") || fnLower.endsWith(".aac") ||
                fnLower.endsWith(".wav") || fnLower.endsWith(".flac")) {
                files.push_back(fileName);
            }
        }
    }
    root.close();

    std::sort(dirs.begin(), dirs.end());
    std::sort(files.begin(), files.end());

    for (auto& dir : dirs) {
        listSD(plSDfile, plSDindex, dir.c_str(), levels - 1);
    }

    for (auto& fp : files) {
        const char* fn = strrchr(fp.c_str(), '/');
        fn = fn ? fn + 1 : fp.c_str();
        uint32_t pos = plSDfile.position();
        plSDfile.printf("%s\t%s\t0\n", fn, fp.c_str());
        plSDindex.write((uint8_t*)&pos, 4);
        _sdFCount++;
        if ((_sdFCount % SD_INDEX_PROGRESS_EVERY) == 0) {
            Serial.print(".");
            if (display.mode() == SDCHANGE) display.putRequest(SDFILEINDEX, _sdFCount);
            if ((_sdFCount % (64 * SD_INDEX_PROGRESS_EVERY)) == 0) Serial.println();
        }
    }
}

void SDManager::indexSDPlaylist() {
  _sdFCount = 0;
  if(exists(PLAYLIST_SD_PATH)) remove(PLAYLIST_SD_PATH);
  if(exists(INDEX_SD_PATH)) remove(INDEX_SD_PATH);
  File playlist = open(PLAYLIST_SD_PATH, "w", true);
  if (!playlist) {
    return;
  }
  File index = open(INDEX_SD_PATH, "w", true);
  listSD(playlist, index, "/", SD_MAX_LEVELS);
  index.flush();
  index.close();
  playlist.flush();
  playlist.close();
  if (display.mode() == SDCHANGE) display.putRequest(SDFILEINDEX, _sdFCount);
  Serial.println();
  delay(50);
}
#endif
