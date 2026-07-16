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

#ifndef SD_SPI_SCK
  #define SD_SPI_SCK SCK
#endif
#ifndef SD_SPI_MISO
  #define SD_SPI_MISO MISO
#endif
#ifndef SD_SPI_MOSI
  #define SD_SPI_MOSI MOSI
#endif

static void sdInitSpiBus() {
#if defined(SD_SPIPINS)
  SDREALSPI.begin(SD_SPIPINS);
#elif SD_HSPI
  SDREALSPI.begin();
#else
  SDREALSPI.begin(SD_SPI_SCK, SD_SPI_MISO, SD_SPI_MOSI, SDC_CS);
#endif
}

static void sdDeselectSharedDevices() {
  pinMode(SDC_CS, OUTPUT);
  digitalWrite(SDC_CS, HIGH);
  #if TFT_CS >= 0
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
  #endif
  #if TS_CS >= 0
    pinMode(TS_CS, OUTPUT);
    digitalWrite(TS_CS, HIGH);
  #endif
  delay(2);
}

#ifdef DEBUG_SD
#ifndef SD_DEBUG_SPI_SCK
  #define SD_DEBUG_SPI_SCK SD_SPI_SCK
#endif
#ifndef SD_DEBUG_SPI_MISO
  #define SD_DEBUG_SPI_MISO SD_SPI_MISO
#endif
#ifndef SD_DEBUG_SPI_MOSI
  #define SD_DEBUG_SPI_MOSI SD_SPI_MOSI
#endif
#if defined(SD_SPIPINS) || SD_HSPI
  #define SD_DEBUG_SPI_NAME "HSPI"
#else
  #define SD_DEBUG_SPI_NAME "FSPI"
#endif

static void sdDebugDeselectSharedDevices() {
  sdDeselectSharedDevices();
}

static uint8_t sdDebugTransfer(bool selectCard) {
  if (selectCard) digitalWrite(SDC_CS, LOW);
  SDREALSPI.beginTransaction(SPISettings(SDSPISPEED, MSBFIRST, SPI_MODE0));
  uint8_t value = SDREALSPI.transfer(0xFF);
  SDREALSPI.endTransaction();
  if (selectCard) digitalWrite(SDC_CS, HIGH);
  return value;
}

static uint8_t sdDebugCmd0() {
  sdDebugDeselectSharedDevices();
  sdInitSpiBus();
  SDREALSPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
  digitalWrite(SDC_CS, HIGH);
  for (uint8_t i = 0; i < 10; i++) SDREALSPI.transfer(0xFF);
  digitalWrite(SDC_CS, LOW);
  SDREALSPI.transfer(0x40);
  SDREALSPI.transfer(0x00);
  SDREALSPI.transfer(0x00);
  SDREALSPI.transfer(0x00);
  SDREALSPI.transfer(0x00);
  SDREALSPI.transfer(0x95);
  uint8_t resp = 0xFF;
  for (uint8_t i = 0; i < 10; i++) {
    resp = SDREALSPI.transfer(0xFF);
    if ((resp & 0x80) == 0) break;
  }
  digitalWrite(SDC_CS, HIGH);
  SDREALSPI.transfer(0xFF);
  SDREALSPI.endTransaction();
  return resp;
}

static void sdDebugPreflight() {
  sdDebugDeselectSharedDevices();
  delay(2);
  uint8_t idleByte = sdDebugTransfer(false);
  digitalWrite(SDC_CS, LOW);
  delay(1);
  uint8_t selectedByte = sdDebugTransfer(true);
  uint8_t postByte = sdDebugTransfer(false);
  SD_DEBUG_PRINTF("[SDDBG] bus preflight idle_byte=0x%02X selected_byte=0x%02X post_byte=0x%02X\n",
                  idleByte, selectedByte, postByte);
  sdInitSpiBus();
}
#endif

SDManager sdman(FSImplPtr(new VFSImpl()));

bool SDManager::start(){
  if (ready) return true;
  sdInitSpiBus();
  sdDeselectSharedDevices();
#ifdef DEBUG_SD
  SD_DEBUG_PRINTF("[SDDBG] start cs=%d speed=%lu spi=%s pins sck=%d miso=%d mosi=%d shared tft_cs=%d ts_cs=%d\n",
                  SDC_CS, (unsigned long)SDSPISPEED, SD_DEBUG_SPI_NAME,
                  SD_DEBUG_SPI_SCK, SD_DEBUG_SPI_MISO, SD_DEBUG_SPI_MOSI, TFT_CS, TS_CS);
  sdDebugPreflight();
  SD_DEBUG_PRINTF("[SDDBG] manual CMD0 cs=%d resp=0x%02X\n", SDC_CS, sdDebugCmd0());
  sdInitSpiBus();
  sdDeselectSharedDevices();
  ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  SD_DEBUG_PRINTF("[SDDBG] begin attempt 1 -> %s\n", ready ? "OK" : "FAIL");
  vTaskDelay(10);
  if(!ready) {
    sdDeselectSharedDevices();
    ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
    SD_DEBUG_PRINTF("[SDDBG] begin attempt 2 -> %s\n", ready ? "OK" : "FAIL");
  }
  vTaskDelay(20);
  if(!ready) {
    sdDeselectSharedDevices();
    ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
    SD_DEBUG_PRINTF("[SDDBG] begin attempt 3 -> %s\n", ready ? "OK" : "FAIL");
  }
  vTaskDelay(50);
  if(!ready) {
    sdDeselectSharedDevices();
    ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
    SD_DEBUG_PRINTF("[SDDBG] begin attempt 4 -> %s\n", ready ? "OK" : "FAIL");
  }
  return ready;
#else
  sdDeselectSharedDevices();
  ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  vTaskDelay(10);
  if(!ready) {
    sdDeselectSharedDevices();
    ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  }
  vTaskDelay(20);
  if(!ready) {
    sdDeselectSharedDevices();
    ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  }
  vTaskDelay(50);
  if(!ready) {
    sdDeselectSharedDevices();
    ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  }
  return ready;
#endif
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
        SD_DEBUG_PRINTLN("##[ERROR]#\tFailed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        SD_DEBUG_PRINTLN("##[ERROR]#\tNot a directory");
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
            SD_DEBUG_PRINT(".");
            if (display.mode() == SDCHANGE) display.putRequest(SDFILEINDEX, _sdFCount);
            if ((_sdFCount % (64 * SD_INDEX_PROGRESS_EVERY)) == 0) SD_DEBUG_PRINTLN();
        }
    }
}

void SDManager::indexSDPlaylist() {
  _sdFCount = 0;
  if(SPIFFS.exists(PLAYLIST_SD_PATH)) SPIFFS.remove(PLAYLIST_SD_PATH);
  if(SPIFFS.exists(INDEX_SD_PATH)) SPIFFS.remove(INDEX_SD_PATH);
  File playlist = SPIFFS.open(PLAYLIST_SD_PATH, "w", true);
  if (!playlist) {
    return;
  }
  File index = SPIFFS.open(INDEX_SD_PATH, "w", true);
  listSD(playlist, index, "/", SD_MAX_LEVELS);
  index.flush();
  index.close();
  playlist.flush();
  playlist.close();
  if (display.mode() == SDCHANGE) display.putRequest(SDFILEINDEX, _sdFCount);
  SD_DEBUG_PRINTLN();
  delay(50);
}
#endif