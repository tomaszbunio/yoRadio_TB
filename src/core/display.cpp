#include "Arduino.h"
#include "options.h"
#include "WiFi.h"
#include "time.h"
#include "config.h"
#include "display.h"
#include "presets.h"
#include "player.h"
#include "network.h"
#include "netserver.h"
#include "profiler.h"
#include "timekeeper.h"
#include "../pluginsManager/pluginsManager.h"
#include "../displays/dspcore.h"
#include "../displays/widgets/widgets.h"
#include "../displays/widgets/pages.h"
#include "../displays/tools/l10n.h"
#include <Fonts/FreeSans9pt7b.h>

#ifndef SD_FFT_TOP_OFFSET
    #define SD_FFT_TOP_OFFSET 11
#endif
#ifndef SD_FFT_BOTTOM_MARGIN
    #define SD_FFT_BOTTOM_MARGIN 20
#endif
#ifndef SD_FFT_INFO_ROW_EXTRA
    #define SD_FFT_INFO_ROW_EXTRA 10
#endif
#ifndef SD_FFT_UPDATE_MS
    #define SD_FFT_UPDATE_MS 50
#endif
#ifndef SD_FFT_BANDS
    #define SD_FFT_BANDS 16
#endif

Display display;
#ifdef USE_NEXTION
    #include "../displays/nextion.h"
Nextion nextion;
#endif

static constexpr bool kTouchEnabled = (TS_MODEL != TS_MODEL_UNDEFINED);

#ifdef DEBUG_MODE_SWITCH
static const char* dbgModeName(displayMode_e m) {
    switch (m) {
        case PLAYER: return "PLAYER";
        case STATIONS: return "STATIONS";
        case VOL: return "VOL";
        case LOST: return "LOST";
        case UPDATING: return "UPDATING";
        case SLEEPING: return "SLEEPING";
        case SCREENSAVER: return "SCREENSAVER";
        case SCREENBLANK: return "SCREENBLANK";
        case SD_PLAYER: return "SD_PLAYER";
        case SDCHANGE: return "SDCHANGE";
        case INFO: return "INFO";
        case SETTINGS: return "SETTINGS";
        case TIMEZONE: return "TIMEZONE";
        case WIFI: return "WIFI";
        case NUMBERS: return "NUMBERS";
        case PRESETS: return "PRESETS";
        default: return "UNKNOWN";
    }
}
#endif

#ifndef DEBUG_VOLUME_SCREEN
    #define DEBUG_VOLUME_SCREEN true
#endif

#if DEBUG_VOLUME_SCREEN
static const char* volDbgModeName(displayMode_e m) {
    switch (m) {
        case PLAYER: return "PLAYER";
        case VOL: return "VOL";
        case STATIONS: return "STATIONS";
        case PRESETS: return "PRESETS";
        case NUMBERS: return "NUMBERS";
        case LOST: return "LOST";
        case UPDATING: return "UPDATING";
        case INFO: return "INFO";
        case SETTINGS: return "SETTINGS";
        case TIMEZONE: return "TIMEZONE";
        case WIFI: return "WIFI";
        case CLEAR: return "CLEAR";
        case SLEEPING: return "SLEEPING";
        case SDCHANGE: return "SDCHANGE";
        case SCREENSAVER: return "SCREENSAVER";
        case SCREENBLANK: return "SCREENBLANK";
        case SD_PLAYER: return "SD_PLAYER";
        default: return "?";
    }
}
#endif

#ifndef CORE_STACK_SIZE
    #define CORE_STACK_SIZE 1024 * 4
#endif
#ifndef DSP_TASK_PRIORITY
    #define DSP_TASK_PRIORITY 2 //"task_prioritas"
#endif
#ifndef DSP_TASK_DELAY
    #define DSP_TASK_DELAY pdMS_TO_TICKS(10) // cap for 50 fps
#endif

#define DSP_QUEUE_TICKS 0

#ifndef DSQ_SEND_DELAY
    // #define DSQ_SEND_DELAY portMAX_DELAY
    #define DSQ_SEND_DELAY pdMS_TO_TICKS(200)
#endif

QueueHandle_t displayQueue;

static void loopDspTask(void* pvParameters) {
    while (true) {
#ifndef DUMMYDISPLAY
        if (displayQueue == NULL) { break; }
        if (timekeeper.loop0()) {
            display.loop();
    #ifndef NETSERVER_LOOP1
            netserver.loop();
    #endif
        }
#else
        timekeeper.loop0();
    #ifndef NETSERVER_LOOP1
        netserver.loop();
    #endif
#endif
        vTaskDelay(DSP_TASK_DELAY);
    }
    vTaskDelete(NULL);
}

void Display::_createDspTask() {
    xTaskCreatePinnedToCore(loopDspTask, "DspTask", CORE_STACK_SIZE, NULL, DSP_TASK_PRIORITY, NULL, DSP_TASK_CORE_ID); //"task_prioritas"
}

#ifndef DUMMYDISPLAY
//============================================================================================================================
DspCore dsp;
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

Page* pages[] = {new Page(), new Page(), new Page(), new Page(), new Page(), new Page()}; // "presets" +1 page; PG_SD_PLAYER=5

    #if !((DSP_MODEL == DSP_ST7735 && DTYPE == INITR_BLACKTAB) || DSP_MODEL == DSP_ST7789 || DSP_MODEL == DSP_ST7796 || DSP_MODEL == DSP_ILI9488 || DSP_MODEL == DSP_ILI9486 || \
          DSP_MODEL == DSP_ILI9341 || DSP_MODEL == DSP_ILI9225 || DSP_MODEL == DSP_ST7789_170 || DSP_MODEL == DSP_SSD1322)
        #undef BITRATE_FULL
        #define BITRATE_FULL false
    #endif

void returnPlayer() {
    display.putRequest(NEWMODE, PLAYER);
}

Display::~Display() {
    delete _pager;
    delete _footer;
    delete _plwidget;
    delete _nums;
    delete _clock;
    delete _meta;
    delete _title1;
    delete _title2;
    delete _plcurrent;
    delete _sdtitle;
    delete _sdartist;
    delete _sdalbum;
    delete _sdprogress;
    delete _sdfft;
}

void Display::init() {
    #ifdef STATION_LOGO_WIDGET
    // Inicjalizacja SPIFFS dla logo stacji (montuje system plików flash)
    _stationLogo.init();
    #endif
    Serial.print("##[BOOT]#\tdisplay.init\t");
    #ifdef USE_NEXTION
    nextion.begin();
    #endif
    #if LIGHT_SENSOR != 255
    analogSetAttenuation(ADC_0db);
    #endif
    _bootStep = 0;
    dsp.initDisplay();
    u8g2Fonts.begin(dsp);
    displayQueue = NULL;
    displayQueue = xQueueCreate(5, sizeof(requestParams_t));
    while (displayQueue == NULL) { ; }
    //_pager.begin();
    //_bootScreen();
    _pager = new Pager();
    _footer = new Page();
    _plwidget = new PlayListWidget();
    _nums = new NumWidget();
    _clock = new ClockWidget();
    _meta = new ScrollWidget();
    _title1 = new ScrollWidget();
    _plcurrent = new ScrollWidget();
    _createDspTask();  // po inicjalizacji wszystkich widgetów – DspTask nie trafi na _pager=nullptr
    Serial.println("done");
}

uint16_t Display::width() {
    return dsp.width();
}
uint16_t Display::height() {
    return dsp.height();
}
    #if TIME_SIZE > 19
        #if DSP_MODEL == DSP_SSD1322
            #define BOOT_PRG_COLOR WHITE
            #define BOOT_TXT_COLOR WHITE
            #define PINK           WHITE
        #elif DSP_MODEL == DSP_SSD1327
            #define BOOT_PRG_COLOR 0x07
            #define BOOT_TXT_COLOR 0x3f
            #define PINK           0x02
        #else
            #define BOOT_PRG_COLOR 0xE68B
            #define BOOT_TXT_COLOR 0xFFFF
            #define PINK           0xF97F
        #endif
    #endif

void Display::_bootScreen() {
    _boot = new Page();
    _boot->addWidget(new ProgressWidget(bootWdtConf, bootPrgConf, BOOT_PRG_COLOR, 0));
    _bootstring = (TextWidget*)&_boot->addWidget(new TextWidget(bootstrConf, 50, true, BOOT_TXT_COLOR, 0));
    _bootversion = (TextWidget*)&_boot->addWidget(new TextWidget(bootverConf, 32, false, BOOT_TXT_COLOR, 0));
    _bootversion->setText(FIRMWARE_VERSION);
    _pager->addPage(_boot);
    _pager->setPage(_boot, true);
    dsp.drawLogo(bootLogoTop);
    _bootStep = 1;
}

void Display::_buildPager() {
    _meta->init("*", metaConf, config.theme.meta, config.theme.metabg);
    _meta->setFbLabel("META");
    _title1->init("*", title1Conf, config.theme.title1, config.theme.background);
    _title1->setFbLabel("TITLE");
    _clock->init(clockConf, 0, 0);
    #if DSP_MODEL == DSP_NOKIA5110
    _plcurrent->init("*", playlistConf, 0, 1);
    #else
    _plcurrent->init("*", playlistConf, config.theme.plcurrent, config.theme.plcurrentbg);
    #endif
    _plwidget->init(_plcurrent);
    #if !defined(DSP_LCD)
    _plcurrent->moveTo({TFT_FRAMEWDT, (uint16_t)(_plwidget->currentTop()), (int16_t)playlistConf.width});
    #endif
    #ifndef HIDE_TITLE2
    _title2 = new ScrollWidget("*", title2Conf, config.theme.title2, config.theme.background);
    _title2->setFbLabel("TITLE2");
    #endif
    #if !defined(DSP_LCD) && DSP_MODEL != DSP_NOKIA5110
    _plbackground = new FillWidget(playlBGConf, config.theme.background);
        #if DSP_INVERT_TITLE || defined(DSP_OLED)
    _metabackground = new FillWidget(metaBGConf, config.theme.metafill);
        #else
    _metabackground = new FillWidget(metaBGConfInv, config.theme.metafill);
        #endif
    #endif
    #if DSP_MODEL == DSP_NOKIA5110
    _plbackground = new FillWidget(playlBGConf, 1);
        //_metabackground = new FillWidget(metaBGConf, 1);
    #endif
    #ifndef HIDE_VU
    _vuwidget = new VuWidget(vuConf, bandsConf, config.theme.vumax, config.theme.vumid, config.theme.vumin, config.theme.background);
    #endif
    #ifndef HIDE_VOLBAR
    _volbar = new SliderWidget(volbarConf, config.theme.volbarin, config.theme.background, VOLUME_CONTROL_STEPS, config.theme.volbarout); // "vol_step"
    #endif
    #ifndef HIDE_HEAPBAR
    _heapbar = new SliderWidget(heapbarConf, config.theme.buffer, config.theme.background, psramInit() ? 300000 : 1600 * config.store.abuff);
    #endif
    #ifndef HIDE_VOL
    _voltxt = new TextWidget(voltxtConf, 10, false, config.theme.vol, config.theme.background);
    #endif
    #ifndef HIDE_IP
    _volip = new TextWidget(iptxtConf, 30, false, config.theme.ip, config.theme.background);
    #endif
    #ifndef HIDE_RSSI
    _rssi = new TextWidget(rssiConf, 20, false, config.theme.rssi, config.theme.background); // 20 config.theme.background
    #endif
    _nums->init(numConf, 10, false, config.theme.digit, config.theme.background);
    #ifndef HIDE_WEATHER
    _weather = new ScrollWidget("\007", weatherConf, config.theme.weather, config.theme.background);
    #endif

    _chtxt = new TextWidget(chtxtConf, 12, false, config.theme.ch, config.theme.background);

    if (_volbar) { _footer->addWidget(_volbar); }
    if (_voltxt) { _footer->addWidget(_voltxt); }
    if (_volip) { _footer->addWidget(_volip); }
    if (_rssi) { _footer->addWidget(_rssi); }
    if (_heapbar) { _footer->addWidget(_heapbar); }
    if (_chtxt) {
        _footer->addWidget(_chtxt);
    }

    if (_metabackground) { pages[PG_PLAYER]->addWidget(_metabackground); }
    pages[PG_PLAYER]->addWidget(_meta);
    pages[PG_PLAYER]->addWidget(_title1);
    if (_title2) { pages[PG_PLAYER]->addWidget(_title2); }
    if (_weather) { pages[PG_PLAYER]->addWidget(_weather); }
    #ifdef SHOW_BITRATE_WIDGET
      #if BITRATE_FULL
      _fullbitrate = new BitrateWidget(fullbitrateConf, config.theme.bitrate, config.theme.background);
      pages[PG_PLAYER]->addWidget(_fullbitrate);
      #else
      _bitrate = new TextWidget(bitrateConf, 30, false, config.theme.bitrate, config.theme.background);
      pages[PG_PLAYER]->addWidget(_bitrate);
      #endif
    #endif // SHOW_BITRATE_WIDGET
    if (_vuwidget) { pages[PG_PLAYER]->addWidget(_vuwidget); }
    pages[PG_PLAYER]->addWidget(_clock);
    pages[PG_SCREENSAVER]->addWidget(_clock);
    pages[PG_PLAYER]->addPage(_footer);

    if (_metabackground) { pages[PG_DIALOG]->addWidget(_metabackground); }
    pages[PG_DIALOG]->addWidget(_meta);
    pages[PG_DIALOG]->addWidget(_nums);

    #if !defined(DSP_LCD) && DSP_MODEL != DSP_NOKIA5110
    pages[PG_DIALOG]->addPage(_footer);
    #endif
    #if !defined(DSP_LCD)
    if (_plbackground) {
        pages[PG_PLAYLIST]->addWidget(_plbackground);
        _plbackground->setHeight(_plwidget->itemHeight());
        _plbackground->moveTo({0, (uint16_t)(_plwidget->currentTop() - playlistConf.widget.textsize * 2), (int16_t)playlBGConf.width});
    }
    #endif
    pages[PG_PLAYLIST]->addWidget(_plcurrent);
    pages[PG_PLAYLIST]->addWidget(_plwidget);

    // SD_PLAYER page: shared clock + vumeter, plus SD-specific widgets
    _sdtitle = new ScrollWidget("*", sdTitleConf, config.theme.title1, config.theme.background);
    _sdtitle->setFbLabel("SDTITLE");
    _sdartist = new ScrollWidget("*", sdArtistConf, config.theme.title2, config.theme.background);
    _sdartist->setFbLabel("SDARTIST");
    _sdalbum = new ScrollWidget("*", sdAlbumConf, config.theme.title2, config.theme.background);
    _sdalbum->setFbLabel("SDALBUM");
    pages[PG_SD_PLAYER]->addWidget(_sdtitle);
    pages[PG_SD_PLAYER]->addWidget(_sdartist);
    pages[PG_SD_PLAYER]->addWidget(_sdalbum);
    _sdprogress = new SliderWidget(sdProgressConf, config.theme.volbarin, config.theme.background, 1000, config.theme.title1);
    pages[PG_SD_PLAYER]->addWidget(_sdprogress);
    if (!kTouchEnabled) {
        const uint8_t sdInfoTextSize = sdFileFormatConf.textsize;
        const uint16_t infoBottom = (uint16_t)(sdFileFormatConf.top + (sdInfoTextSize * CHARHEIGHT + SD_FFT_INFO_ROW_EXTRA));
        const uint16_t controlsBottom = (uint16_t)(SD_BTN_Y + SD_BTN_H + SD_BTN_GAP);
        const uint16_t fftTop = (uint16_t)(max(infoBottom, controlsBottom) > SD_FFT_TOP_OFFSET ? (max(infoBottom, controlsBottom) - SD_FFT_TOP_OFFSET) : 0);
        const uint16_t fftBottom = (uint16_t)(sdCurrentTimeConf.top > SD_FFT_BOTTOM_MARGIN ? sdCurrentTimeConf.top - SD_FFT_BOTTOM_MARGIN : 0);
        const uint16_t fftHeight = (fftBottom > fftTop) ? (uint16_t)(fftBottom - fftTop + 1) : 0;
        _sdfft = new SdFftWidget();
        _sdfft->init({sdProgressConf.widget.left, fftTop, 0, WA_LEFT}, sdProgressConf.width, fftHeight, SD_FFT_BANDS, config.theme.title2, config.theme.background, SD_FFT_UPDATE_MS);
        pages[PG_SD_PLAYER]->addWidget(_sdfft);
    }
    #ifdef SD_COVER_ART
    _sdcover.init(config.theme.background);
    #endif

    for (const auto& p : pages) { _pager->addPage(p); }
}

void Display::_apScreen() {
    if (_boot) { _pager->removePage(_boot); }
    #ifndef DSP_LCD
    _boot = new Page();
        #if DSP_MODEL != DSP_NOKIA5110
            #if DSP_INVERT_TITLE || defined(DSP_OLED)
    _boot->addWidget(new FillWidget(metaBGConf, config.theme.metafill));
            #else
    _boot->addWidget(new FillWidget(metaBGConfInv, config.theme.metafill));
            #endif
        #endif
    ScrollWidget* bootTitle = (ScrollWidget*)&_boot->addWidget(new ScrollWidget("*", apTitleConf, config.theme.meta, config.theme.metabg));
    bootTitle->setText("yoRadio AP Mode");
    TextWidget* apname = (TextWidget*)&_boot->addWidget(new TextWidget(apNameConf, 30, false, config.theme.title1, config.theme.background));
    apname->setText(LANG::apNameTxt);
    TextWidget* apname2 = (TextWidget*)&_boot->addWidget(new TextWidget(apName2Conf, 30, false, config.theme.clock, config.theme.background));
    apname2->setText(apSsid);
    TextWidget* appass = (TextWidget*)&_boot->addWidget(new TextWidget(apPassConf, 30, false, config.theme.title1, config.theme.background));
    appass->setText(LANG::apPassTxt);
    TextWidget* appass2 = (TextWidget*)&_boot->addWidget(new TextWidget(apPass2Conf, 30, false, config.theme.clock, config.theme.background));
    appass2->setText(apPassword);
    ScrollWidget* bootSett = (ScrollWidget*)&_boot->addWidget(new ScrollWidget("*", apSettConf, config.theme.title2, config.theme.background));
    bootSett->setText(config.ipToStr(WiFi.softAPIP()), LANG::apSettFmt);
    _pager->addPage(_boot);
    _pager->setPage(_boot);
    #else
    dsp.apScreen();
    #endif
}

void Display::_start() {
    if (_boot) { _pager->removePage(_boot); }
    #ifdef USE_NEXTION
    nextion.wake();
    #endif
    if (network.status != CONNECTED && network.status != SDREADY) {
        _apScreen();
    #ifdef USE_NEXTION
        nextion.apScreen();
    #endif
        _bootStep = 2;
        return;
    }
    #ifdef USE_NEXTION
    // nextion.putcmd("page player");
    nextion.start();
    #endif
    _buildPager();
    _mode = (config.getMode() == PM_SDCARD) ? SD_PLAYER : PLAYER;
    if (_mode == PLAYER) {
        config.setTitle(LANG::const_PlReady);
    }

    if (_heapbar) { _heapbar->lock(!config.store.audioinfo); }

    if (_weather) { _weather->lock(!config.store.showweather); }
    if (_weather && config.store.showweather) { _weather->setText(LANG::const_getWeather); }

    if (_vuwidget) { _vuwidget->lock(); }
    if (_rssi) { _setRSSI(WiFi.RSSI()); }
    if (_chtxt) {
        _chtxt->setText(config.lastStation(), "CH:%d");
    }
    #ifndef HIDE_IP
    if (_volip) { _volip->setText(config.ipToStr(WiFi.localIP()), iptxtFmt); }
    #endif
    if (_mode == SD_PLAYER) {
        _pager->setPage(pages[PG_SD_PLAYER]);
        _sdPlayerScreen();
        _drawSdControls(true);
    } else {
        _pager->setPage(pages[PG_PLAYER]);
        _volume();
        _station();
        _time(false);
        pm.on_display_player();
    }
    _bootStep = 2;
}

void Display::_showDialog(const char* title) {
    dsp.setScrollId(NULL);
    _pager->setPage(pages[PG_DIALOG]);
    #ifdef META_MOVE
    _meta->moveTo(metaMove);
    #endif
    _meta->setAlign(WA_CENTER);
    _meta->setText(title);
}

void Display::_swichMode(displayMode_e newmode) {
    #ifdef USE_NEXTION
    // nextion.swichMode(newmode);
    nextion.putRequest({NEWMODE, newmode});
    #endif
#ifdef DEBUG_MODE_SWITCH
    Serial.printf("[MODEDBG][Display] request=%s current=%s net=%d playMode=%d isSdPlayer=%d\n",
                  dbgModeName(newmode), dbgModeName(_mode), network.status, config.getMode(), config.isSdPlayer ? 1 : 0);
#endif
#if DEBUG_VOLUME_SCREEN
    if (newmode == VOL || _mode == VOL) {
        Serial.printf("[VOLDBG][%lu][Display::_swichMode] request=%s current=%s net=%d playMode=%d isSdPlayer=%d\n",
                      millis(), volDbgModeName(newmode), volDbgModeName(_mode),
                      network.status, config.getMode(), config.isSdPlayer ? 1 : 0);
    }
#endif
    if (newmode == _mode) {
        if (newmode == SD_PLAYER) {
            SD_DEBUG_PRINTLN("[SD_PLAYER] refresh current SD_PLAYER screen");
            config.isSdPlayer = true;
            _sdCurTimeBuf[0] = 0;
            _sdTotTimeBuf[0] = 0;
            _sdFmtBuf[0] = 0;
            _sdSrBuf[0] = 0;
            _sdBpsBuf[0] = 0;
            _sdPlaying = !player.isRunning();
            _pager->setPage(pages[PG_SD_PLAYER]);
            _sdPlayerScreen();
            _drawSdControls(true);
        }
        return;
    }
    if (network.status != CONNECTED && network.status != SDREADY) { return; }
    _mode = newmode;
#ifdef DEBUG_MODE_SWITCH
    Serial.printf("[MODEDBG][Display] switched=%s isSdPlayer=%d lastStation=%u currentPlItem=%u\n",
                  dbgModeName(_mode), config.isSdPlayer ? 1 : 0, config.lastStation(), currentPlItem);
#endif
    dsp.setScrollId(NULL);
    if (newmode == PLAYER) {
        if (player.isRunning()) {
            if (clockMove.width < 0) {
                _clock->moveBack();
            } else {
                _clock->moveTo(clockMove);
            }
        } else {
            _clock->moveBack();
        }
    #ifdef DSP_LCD
        dsp.clearDsp();
    #endif
        numOfNextStation = 0;
    #ifdef META_MOVE
        _meta->moveBack();
    #endif
        _meta->setAlign(metaConf.widget.align);
        //_meta->setText(config.station.name);
        // _nums->setText("");
        config.isScreensaver = false;
        if (_vuwidget) { _vuwidget->invalidate(); }
        _pager->setPage(pages[PG_PLAYER]);
        // Returning from SD_PLAYER can leave stale pixels in the audio buffer bar area.
        // Force full widget reset so next draw starts from a clean baseline.
        if (_heapbar) {
            _heapbar->lock(true);
            _heapbar->unlock();
            _heapbar->lock(!config.store.audioinfo);
            _heapbar->setValue((config.store.audioinfo && player.isRunning()) ? player.inBufferFilled() : 0);
        }
        // _station() po setPage() – inaczej setPage() nadpisuje logo stacji tłem strony
        _station();
        if (_chtxt) {
            _chtxt->setText(config.lastStation(), "CH:%d");
        }
        config.setDspOn(config.store.dspon, false);
        pm.on_display_player();
    }
    if (newmode == SCREENSAVER || newmode == SCREENBLANK) {
        config.isScreensaver = true;
        config.isSdPlayer = false;
        _pager->setPage(pages[PG_SCREENSAVER]);
        if (newmode == SCREENBLANK) {
            // dsp.clearClock();
            _clock->clear();
            config.setDspOn(false, false);
        }
    } else {
        config.screensaverTicks = SCREENSAVERSTARTUPDELAY;
        config.screensaverPlayingTicks = SCREENSAVERSTARTUPDELAY;
        config.isScreensaver = false;
        if (newmode == PLAYER || newmode == SD_PLAYER) config.isSdPlayer = (newmode == SD_PLAYER);
    #if PWR_AMP != 255 // "PWR_AMP"
        digitalWrite(PWR_AMP, HIGH);
    #endif
    }
    if (newmode == VOL) {
    #ifndef HIDE_IP
        _showDialog(LANG::const_DlgVolume);
    #else
        _showDialog(config.ipToStr(WiFi.localIP()));
    #endif
        if (_chtxt) {
            _chtxt->setText("v8.8");
        }
        _nums->setText(config.store.volume, numtxtFmt);
    }
    if (newmode == LOST) { _showDialog(LANG::const_DlgLost); }
    if (newmode == UPDATING) { _showDialog(LANG::const_DlgUpdate); }
    if (newmode == SLEEPING) { _showDialog("SLEEPING"); }
    if (newmode == SDCHANGE) { _showDialog(LANG::const_waitForSD); }
    if (newmode == INFO || newmode == SETTINGS || newmode == TIMEZONE || newmode == WIFI) { _showDialog(LANG::const_DlgNextion); }
    if (newmode == NUMBERS) { _showDialog(""); }
    #if DSP_MODEL == DSP_ILI9488
    if (newmode == PRESETS) {
        _pager->setPage(pages[PG_PRESETS], true);
        presets_drawScreen();
    }
    #endif
    if (newmode == STATIONS) {
        // Force reload of playlist items on every entry.
        // Without this, cached SD rows can remain visible after switching to WEB mode.
        if (_plwidget) { _plwidget->resetCache(); }
        _pager->setPage(pages[PG_PLAYLIST]);
        _plcurrent->setText("");
        currentPlItem = config.lastStation();
        _drawPlaylist();
    }
    if (newmode == SD_PLAYER) {
        SD_DEBUG_PRINTLN("[SD_PLAYER] switching to SD_PLAYER screen");
        #ifdef STATION_LOGO_WIDGET
        _stationLogo.clear();
        #endif
        _sdCurTimeBuf[0] = 0;
        _sdTotTimeBuf[0] = 0;
        _sdFmtBuf[0] = 0; _sdSrBuf[0] = 0; _sdBpsBuf[0] = 0;
        _sdPlaying = !player.isRunning();
        _pager->setPage(pages[PG_SD_PLAYER]);
        _sdPlayerScreen();
        _drawSdControls(true);
    }
}

void Display::resetQueue() {
    if (displayQueue != NULL) { xQueueReset(displayQueue); }
}

void Display::_applyTheme() {
  // Update widget colors without rebuilding pages (keeps current texts like weather)
  if (!_pager) return;

  // Important: ClockWidget may use an internal PS frame buffer (PSFBUFFER)
  // initialized with the background color. When theme background changes at
  // runtime, the buffer must be reset, otherwise parts of the clock area can
  // keep the previous background until a full reboot.
  if (_clock) {
    // Update clock widget colors first so _reset() uses the new background.
    _clock->setColors(config.theme.clock, config.theme.background, false);
    bool wasLocked = _clock->locked();
    if (wasLocked) _clock->unlock();
    // lock(true) calls ClockWidget::_reset(), which recreates the buffer using
    // current config.theme.background.
    _clock->lock(true);
    _clock->unlock();
    if (wasLocked) _clock->lock(true);
  }

  // Recolor widgets.
  // Use redraw=true for background/filled widgets so old colors don't linger
  // (without needing a full reboot).
  if (_meta)    _meta->setColors(config.theme.meta, config.theme.metabg, true);
  if (_title1)  _title1->setColors(config.theme.title1, config.theme.background, true);
  #ifndef HIDE_TITLE2
  if (_title2)  _title2->setColors(config.theme.title2, config.theme.background, true);
  #endif
  #ifndef HIDE_WEATHER
  if (_weather) _weather->setColors(config.theme.weather, config.theme.background, true);
  #endif

  if (_metabackground) _metabackground->setColors(0, config.theme.metafill, true);
  if (_plbackground)   _plbackground->setColors(0, config.theme.plcurrentfill, true);
  if (_plcurrent)      _plcurrent->setColors(config.theme.plcurrent, config.theme.plcurrentbg, true);

  #ifndef HIDE_VU
  if (_vuwidget) _vuwidget->setVuColors(config.theme.vumax, config.theme.vumid, config.theme.vumin, config.theme.background, false);
  #endif
  #ifndef HIDE_VOLBAR
  if (_volbar) {
    _volbar->setOutlineColor(config.theme.volbarout, false);
    _volbar->setColors(config.theme.volbarin, config.theme.background, true);
  }
  #endif
  #ifndef HIDE_HEAPBAR
  if (_heapbar) _heapbar->setColors(config.theme.buffer, config.theme.background, true);
  #endif
  #ifndef HIDE_VOL
  if (_voltxt) _voltxt->setColors(config.theme.vol, config.theme.background, true);
  #endif
  #ifndef HIDE_IP
  if (_volip)  _volip->setColors(config.theme.ip, config.theme.background, true);
  #endif
  #ifndef HIDE_RSSI
  if (_rssi)   _rssi->setColors(config.theme.rssi, config.theme.background, true);
  #endif
  if (_chtxt)  _chtxt->setColors(config.theme.ch, config.theme.background, true);
  if (_nums)   _nums->setColors(config.theme.digit, config.theme.background, true);
  if (_bitrate) _bitrate->setColors(config.theme.bitrate, config.theme.background, true);
  if (_fullbitrate) _fullbitrate->setColors(config.theme.bitrate, config.theme.background, true);
  #ifdef STATION_LOGO_WIDGET
  // Poinformuj widget logo o nowym kolorze tła – używany przy clear() i przy PNG mniejszym niż obszar
  _stationLogo.setBgColor(config.theme.background);
  #endif
  #ifdef SD_COVER_ART
  _sdcover.setBgColor(config.theme.background);
  #endif

  // Redraw current page using the new theme background
  if (_sdtitle)    _sdtitle->setColors(config.theme.title1, config.theme.background, true);
  if (_sdartist)   _sdartist->setColors(config.theme.title2, config.theme.background, true);
  if (_sdalbum)       _sdalbum->setColors(config.theme.title2, config.theme.background, true);
  if (_sdprogress)    _sdprogress->setColors(config.theme.volbarin, config.theme.background, true);
  if (_sdfft)         _sdfft->setColors(config.theme.title2, config.theme.background, true);

  Page *pg = pages[PG_DIALOG];
  if (_mode == PLAYER) pg = pages[PG_PLAYER];
  else if (_mode == SD_PLAYER) pg = pages[PG_SD_PLAYER];
  else if (_mode == STATIONS) pg = pages[PG_PLAYLIST];
  else if (_mode == PRESETS) pg = pages[PG_PRESETS];
  else if (_mode == SCREENSAVER || _mode == SCREENBLANK) pg = pages[PG_SCREENSAVER];

  _pager->setPage(pg, false);
  #ifndef HIDE_VU
  // setPage() clears player area; force a full VU redraw after the page is rebuilt.
  if (_vuwidget) _vuwidget->setVuColors(config.theme.vumax, config.theme.vumid, config.theme.vumin, config.theme.background, true);
  // setPage() clears player area; force one-shot redraw of L/R labels over VU.
  VuWidget::setLabelsDrawn(false);
  #endif
  // setPage() clears the screen – redraw station logo if on player page
  if (pg == pages[PG_PLAYER]) _station();
  #ifdef SD_COVER_ART
  if (pg == pages[PG_SD_PLAYER]) { _sdcover.redraw(); }
  #endif
  if (pg == pages[PG_SD_PLAYER]) { _sdCurTimeBuf[0] = 0; _sdTotTimeBuf[0] = 0; _sdFmtBuf[0] = 0; _sdSrBuf[0] = 0; _sdBpsBuf[0] = 0; _sdPlaying = !player.isRunning(); _drawSdTimers(true); _drawSdFileInfo(true); _drawSdControls(true); }

  // Force clock redraw (it uses config.theme directly)
  if (_clock && (pg == pages[PG_PLAYER] || pg == pages[PG_SCREENSAVER])) {
    _clock->draw(true);
  }
  Serial.printf("_applyTheme: _meta=%p _title1=%p _clock=%p _weather=%p\n", _meta, _title1, _clock, _weather);
}


void Display::loadSdCover() {
    #ifdef SD_COVER_ART
    if (config.getMode() != PM_SDCARD) return;
    _sdcover.reset();
    _sdcover.setTrack(config.station.url);
    #endif
}

void Display::resetPlaylistCache() {
    if (_plwidget) _plwidget->resetCache();
}

bool Display::isClockMMTap(uint16_t x, uint16_t y) {
    if (!_clock) return false;
    if (config.store.flipscreen) {
        x = (uint16_t)((int)dsp.width() - 1 - (int)x);
        y = (uint16_t)((int)dsp.height() - 1 - (int)y);
    }
    return _clock->isMMTap(x, y);
}

bool Display::isClockSecondsTap(uint16_t x, uint16_t y) {
    if (!_clock) return false;
    if (config.store.flipscreen) {
        x = (uint16_t)((int)dsp.width() - 1 - (int)x);
        y = (uint16_t)((int)dsp.height() - 1 - (int)y);
    }
    return _clock->isSecondsTap(x, y);
}

int8_t Display::isSdTransportTap(uint16_t x, uint16_t y) {
    if (!kTouchEnabled) return -1;
    if (config.store.flipscreen) {
        x = (uint16_t)((int)dsp.width() - 1 - (int)x);
        y = (uint16_t)((int)dsp.height() - 1 - (int)y);
    }
    const uint16_t bx   = sdProgressConf.widget.left;
    const uint16_t bw   = (sdProgressConf.width - 2 * SD_BTN_GAP) / 3;
    const uint16_t step = bw + SD_BTN_GAP;
    const uint16_t nextX = bx + 2 * step;
    const uint16_t backY = SD_BTN_Y - SD_BTN_H - SD_BTN_GAP;
    const uint16_t folderY = SD_BTN_Y + SD_BTN_H + SD_BTN_GAP;

    if (y >= SD_BTN_Y && y < (uint16_t)(SD_BTN_Y + SD_BTN_H)) {
        if (x >= bx            && x < bx + bw)            return 0;  // Prev
        if (x >= bx + step     && x < bx + step + bw)     return 1;  // Play/Pause
        if (x >= nextX         && x < nextX + bw)         return 2;  // Next
    }
    if (y >= backY && y < (uint16_t)(backY + SD_BTN_H)) {
        if (x >= nextX && x < nextX + bw) return 3;  // Back to radio
    }
    if (y >= folderY && y < (uint16_t)(folderY + SD_BTN_H)) {
        if (x >= bx            && x < bx + bw)        return 4;  // Previous folder
        if (x >= bx + step     && x < bx + step + bw) return 6;  // Shuffle
        if (x >= nextX         && x < nextX + bw)     return 5;  // Next folder
    }
    return -1;
}

void Display::flashSdButton(uint8_t btn) {
    if (!kTouchEnabled) return;
    if (_mode != SD_PLAYER || btn > 6) return;
    _drawSdButton(btn, true);
    delay(120);
    _drawSdButton(btn, false);
}

void Display::_sdPlayerScreen() {
    if (_sdtitle) {
        const char* txt = (strlen(config.station.title) > 0) ? config.station.title : config.station.name;
        _sdtitle->setText(txt);
    }
    if (_sdartist) {
        _sdartist->setText(config.station.sdArtist);
    }
    if (_sdalbum) {
        _sdalbum->setText(config.station.sdAlbum);
    }
    #ifdef SD_COVER_ART
    _sdcover.redraw();
    #endif
    if (_sdprogress) {
        uint32_t dur = player.getAudioFileDuration();
        uint32_t pos = player.getAudioCurrentTime();
        _sdprogress->setValue(dur > 0 ? (uint32_t)((uint64_t)pos * 1000 / dur) : 0);
    }
    _drawSdTimers();
    _drawSdFileInfo();
    _drawSdControls();
}

void Display::_drawSdTimers(bool force) {
    uint32_t dur = player.getAudioFileDuration();
    uint32_t pos = player.getAudioCurrentTime();
    char buf[8];

    // currentTime
    snprintf(buf, sizeof(buf), "%u:%02u", pos / 60, pos % 60);
    if (force || strcmp(buf, _sdCurTimeBuf) != 0) {
        uint8_t  tsz    = sdCurrentTimeConf.textsize;
        uint16_t cw     = tsz * CHARWIDTH;
        uint16_t ch     = tsz * CHARHEIGHT;
        uint16_t clearW = max((uint16_t)(strlen(_sdCurTimeBuf) * cw), (uint16_t)(strlen(buf) * cw));
        if (clearW == 0) clearW = strlen(buf) * cw;
        dsp.fillRect(sdCurrentTimeConf.left, sdCurrentTimeConf.top, clearW, ch, config.theme.background);
        dsp.setTextColor(config.theme.title2, config.theme.background);
        dsp.setFont();
        dsp.setTextSize(tsz);
        dsp.setCursor(sdCurrentTimeConf.left, sdCurrentTimeConf.top);
        dsp.print(buf);
        strlcpy(_sdCurTimeBuf, buf, sizeof(_sdCurTimeBuf));
    }

    // totalTime
    if (dur == 0) return;
    snprintf(buf, sizeof(buf), "%u:%02u", dur / 60, dur % 60);
    if (force || strcmp(buf, _sdTotTimeBuf) != 0) {
        uint8_t  tsz    = sdTotalTimeConf.textsize;
        uint16_t cw     = tsz * CHARWIDTH;
        uint16_t ch     = tsz * CHARHEIGHT;
        uint16_t newW   = strlen(buf) * cw;
        uint16_t oldW   = strlen(_sdTotTimeBuf) * cw;
        uint16_t clearW = max(oldW, newW);
        if (clearW == 0) clearW = newW;
        uint16_t clearX = dsp.width() - clearW - sdTotalTimeConf.left;
        uint16_t drawX  = dsp.width() - newW  - sdTotalTimeConf.left;
        dsp.fillRect(clearX, sdTotalTimeConf.top, clearW, ch, config.theme.background);
        dsp.setTextColor(config.theme.title2, config.theme.background);
        dsp.setFont();
        dsp.setTextSize(tsz);
        dsp.setCursor(drawX, sdTotalTimeConf.top);
        dsp.print(buf);
        strlcpy(_sdTotTimeBuf, buf, sizeof(_sdTotTimeBuf));
    }
}

void Display::_drawSdFileInfo(bool force) {
    static const char* const fmtNames[] = {"", "MP3", "AAC", "FLAC", "OGG", "WAV", "OGG", "OPUS"};
    const char* fmtStr = (config.configFmt <= BF_OPU) ? fmtNames[config.configFmt] : "";

    uint32_t sr = player.getSampleRate();
    char srBuf[12];
    if (sr == 0) {
        srBuf[0] = 0;
    } else if (sr % 1000 == 0) {
        snprintf(srBuf, sizeof(srBuf), "%ukHz", sr / 1000);
    } else {
        snprintf(srBuf, sizeof(srBuf), "%u.%ukHz", sr / 1000, (sr % 1000) / 100);
    }

    uint8_t bps = player.getBitsPerSample();
    char bpsBuf[8];
    if (bps == 0) {
        bpsBuf[0] = 0;
    } else {
        snprintf(bpsBuf, sizeof(bpsBuf), "%ubit", bps);
    }

    uint8_t tsz = sdFileFormatConf.textsize;
    bool dirty = force ||
                 strcmp(fmtStr, _sdFmtBuf) != 0 ||
                 strcmp(srBuf, _sdSrBuf) != 0 ||
                 strcmp(bpsBuf, _sdBpsBuf) != 0;
    if (!dirty) return;

    const uint16_t rowX = sdProgressConf.widget.left;
    const uint16_t rowW = sdProgressConf.width;
    const uint16_t rowY = sdFileFormatConf.top;
    const uint16_t vPad = 3;
    const uint16_t gap  = 4;
    const uint16_t boxW = (uint16_t)((rowW - 2 * gap) / 3);
    const uint8_t  rad  = 5;
    const uint16_t fg   = 0xFFFF; // white border/text

    dsp.setFont(&FreeSans9pt7b);
    dsp.setTextSize(tsz);

    const char* labels[3] = {fmtStr[0] ? fmtStr : "-", srBuf[0] ? srBuf : "-", bpsBuf[0] ? bpsBuf : "-"};
    uint16_t maxTxtH = 0;
    for (uint8_t i = 0; i < 3; i++) {
        int16_t x1, y1;
        uint16_t txtW, txtH;
        dsp.getTextBounds((char*)labels[i], 0, 0, &x1, &y1, &txtW, &txtH);
        if (txtH > maxTxtH) maxTxtH = txtH;
    }
    if (maxTxtH == 0) maxTxtH = (uint16_t)(tsz * CHARHEIGHT);
    const uint16_t rowH = (uint16_t)(maxTxtH + vPad * 2);

    // Clear only the new one-line badges area (do not overwrite SD control buttons).
    dsp.fillRect(rowX, rowY, rowW, rowH, config.theme.background);
    for (uint8_t i = 0; i < 3; i++) {
        const uint16_t x = (uint16_t)(rowX + i * (boxW + gap));
        dsp.drawRoundRect(x, rowY, boxW, rowH, rad, fg);

        int16_t x1, y1;
        uint16_t txtW, txtH;
        dsp.getTextBounds((char*)labels[i], 0, 0, &x1, &y1, &txtW, &txtH);
        const uint16_t tx = (txtW < boxW) ? (uint16_t)(x + (boxW - txtW) / 2) : (uint16_t)(x + 2);
        const uint16_t ty = (uint16_t)(rowY + vPad - y1);
        dsp.setTextColor(fg, config.theme.background);
        dsp.setCursor(tx, ty);
        dsp.print(labels[i]);
    }

    dsp.setFont();

    strlcpy(_sdFmtBuf, fmtStr, sizeof(_sdFmtBuf));
    strlcpy(_sdSrBuf,  srBuf,  sizeof(_sdSrBuf));
    strlcpy(_sdBpsBuf, bpsBuf, sizeof(_sdBpsBuf));
}

void Display::_drawSdControls(bool force) {
    if (!kTouchEnabled) return;
    bool playing = player.isRunning();
    bool snuffle = config.store.sdsnuffle;
    if (!force && playing == _sdPlaying && snuffle == _sdSnuffle) return;

    if (force) {
        _drawSdButton(0);
        _drawSdButton(2);
        _drawSdButton(3);
        _drawSdButton(4);
        _drawSdButton(5);
        _drawSdButton(6);
    } else if (snuffle != _sdSnuffle) {
        _drawSdButton(6);
    }
    _drawSdButton(1);

    _sdPlaying = playing;
    _sdSnuffle = snuffle;
    return;

    const uint16_t btnX   = sdProgressConf.widget.left;
    const uint16_t btnW   = (sdProgressConf.width - 2 * SD_BTN_GAP) / 3;
    const uint16_t btnY   = SD_BTN_Y;
    const uint16_t btnH   = SD_BTN_H;
    const uint8_t  btnR   = SD_BTN_R;
    const uint16_t fg     = config.theme.title2;
    const uint16_t bg     = config.theme.background;

    // 0=prev  1=play  2=pause  3=next  4=back to radio
    auto drawBtn = [&](uint16_t bx, uint16_t by, uint16_t bw, uint8_t iconType) {
        const int cx = bx + bw / 2;
        const int cy = by + btnH / 2;
        dsp.fillRoundRect(bx, by, bw, btnH, btnR, bg);
        dsp.drawRoundRect(bx, by, bw, btnH, btnR, fg);
        switch (iconType) {
            case 0: // Prev  |◀
                dsp.fillRect(cx - 12, cy - 11, 5, 22, fg);
                dsp.fillTriangle(cx - 5, cy, cx + 11, cy - 11, cx + 11, cy + 11, fg);
                break;
            case 1: // Play  ▶
                dsp.fillTriangle(cx - 12, cy - 12, cx - 12, cy + 12, cx + 12, cy, fg);
                break;
            case 2: // Pause ⏸
                dsp.fillRect(cx - 11, cy - 11, 8, 22, fg);
                dsp.fillRect(cx +  3, cy - 11, 8, 22, fg);
                break;
            case 3: // Next  ▶|
                dsp.fillTriangle(cx - 11, cy - 11, cx - 11, cy + 11, cx + 5, cy, fg);
                dsp.fillRect(cx +  7, cy - 11, 5, 22, fg);
                break;
            case 4: // Return
                dsp.drawFastHLine(cx - 10, cy, 20, fg);
                dsp.drawFastHLine(cx - 10, cy + 1, 20, fg);
                dsp.drawFastHLine(cx - 10, cy + 2, 20, fg);
                dsp.drawFastVLine(cx + 8, cy - 10, 11, fg);
                dsp.drawFastVLine(cx + 9, cy - 10, 11, fg);
                dsp.drawFastVLine(cx + 10, cy - 10, 11, fg);
                dsp.fillTriangle(cx - 10, cy + 1, cx - 1, cy - 8, cx - 1, cy + 10, fg);
                break;
        }
    };

    if (force) {
        drawBtn(btnX,                            btnY, btnW, 0);  // Prev
        drawBtn(btnX + 2 * (btnW + SD_BTN_GAP), btnY, btnW, 3);  // Next
        drawBtn(btnX + 2 * (btnW + SD_BTN_GAP), btnY - btnH - SD_BTN_GAP, btnW, 4);  // Back to radio
    }
    drawBtn(btnX + btnW + SD_BTN_GAP, btnY, btnW, playing ? 2 : 1);  // Play/Pause

    _sdPlaying = playing;
}

void Display::_drawSdButton(uint8_t btn, bool pressed) {
    if (!kTouchEnabled) return;
    const uint16_t btnX   = sdProgressConf.widget.left;
    const uint16_t btnW   = (sdProgressConf.width - 2 * SD_BTN_GAP) / 3;
    const uint16_t step   = btnW + SD_BTN_GAP;
    const uint16_t btnY   = SD_BTN_Y;
    const uint16_t btnH   = SD_BTN_H;
    const uint8_t  btnR   = SD_BTN_R;
    const uint16_t normal = config.theme.title2;
    const uint16_t bg     = config.theme.background;

    uint16_t x = btnX;
    uint16_t y = btnY;
    uint8_t iconType = 0;
    switch (btn) {
        case 0:
            iconType = 0;
            break;
        case 1:
            x += step;
            iconType = player.isRunning() ? 2 : 1;
            break;
        case 2:
            x += 2 * step;
            iconType = 3;
            break;
        case 3:
            x += 2 * step;
            y -= btnH + SD_BTN_GAP;
            iconType = 4;
            break;
        case 4:
            y += btnH + SD_BTN_GAP;
            iconType = 5;
            break;
        case 5:
            x += 2 * step;
            y += btnH + SD_BTN_GAP;
            iconType = 6;
            break;
        case 6:
            x += step;
            y += btnH + SD_BTN_GAP;
            iconType = 7;
            break;
        default:
            return;
    }

    if (btn == 6 && config.store.sdsnuffle) {
        pressed = !pressed;
    }

    const int cx = x + btnW / 2;
    const int cy = y + btnH / 2;
    const uint16_t activeFill = pressed ? normal : bg;
    const uint16_t activeIcon = pressed ? bg : normal;
    dsp.fillRoundRect(x, y, btnW, btnH, btnR, activeFill);
    dsp.drawRoundRect(x, y, btnW, btnH, btnR, normal);
    switch (iconType) {
        case 0:
            dsp.fillRect(cx - 12, cy - 11, 5, 22, activeIcon);
            dsp.fillTriangle(cx - 5, cy, cx + 11, cy - 11, cx + 11, cy + 11, activeIcon);
            break;
        case 1:
            dsp.fillTriangle(cx - 12, cy - 12, cx - 12, cy + 12, cx + 12, cy, activeIcon);
            break;
        case 2:
            dsp.fillRect(cx - 11, cy - 11, 8, 22, activeIcon);
            dsp.fillRect(cx +  3, cy - 11, 8, 22, activeIcon);
            break;
        case 3:
            dsp.fillTriangle(cx - 11, cy - 11, cx - 11, cy + 11, cx + 5, cy, activeIcon);
            dsp.fillRect(cx +  7, cy - 11, 5, 22, activeIcon);
            break;
        case 4:
            dsp.drawFastHLine(cx - 10, cy, 20, activeIcon);
            dsp.drawFastHLine(cx - 10, cy + 1, 20, activeIcon);
            dsp.drawFastHLine(cx - 10, cy + 2, 20, activeIcon);
            dsp.drawFastVLine(cx + 8, cy - 10, 11, activeIcon);
            dsp.drawFastVLine(cx + 9, cy - 10, 11, activeIcon);
            dsp.drawFastVLine(cx + 10, cy - 10, 11, activeIcon);
            dsp.fillTriangle(cx - 10, cy + 1, cx - 1, cy - 8, cx - 1, cy + 10, activeIcon);
            break;
        case 5:
            dsp.drawRect(cx - 14, cy - 7, 28, 18, activeIcon);
            dsp.fillRect(cx - 11, cy - 11, 12, 5, activeIcon);
            dsp.fillTriangle(cx - 9, cy + 1, cx, cy - 8, cx, cy + 10, activeIcon);
            break;
        case 6:
            dsp.drawRect(cx - 14, cy - 7, 28, 18, activeIcon);
            dsp.fillRect(cx - 11, cy - 11, 12, 5, activeIcon);
            dsp.fillTriangle(cx + 9, cy + 1, cx, cy - 8, cx, cy + 10, activeIcon);
            break;
        case 7:
            dsp.drawLine(cx - 15, cy - 8, cx - 6, cy - 8, activeIcon);
            dsp.drawLine(cx - 6, cy - 8, cx + 7, cy + 7, activeIcon);
            dsp.drawLine(cx - 15, cy + 8, cx - 6, cy + 8, activeIcon);
            dsp.drawLine(cx - 6, cy + 8, cx + 7, cy - 7, activeIcon);
            dsp.drawLine(cx + 7, cy - 7, cx + 14, cy - 7, activeIcon);
            dsp.drawLine(cx + 7, cy + 7, cx + 14, cy + 7, activeIcon);
            dsp.fillTriangle(cx + 14, cy - 7, cx + 8, cy - 12, cx + 8, cy - 2, activeIcon);
            dsp.fillTriangle(cx + 14, cy + 7, cx + 8, cy + 2, cx + 8, cy + 12, activeIcon);
            break;
    }
}

void Display::_drawPlaylist() {
    // dsp.drawPlaylist(currentPlItem);
    _plwidget->drawPlaylist(currentPlItem);
    uint8_t stations_list_return_time = STATIONS_LIST_RETURN_TIME;
    if (stations_list_return_time < 1) { stations_list_return_time = 1; }
    timekeeper.waitAndReturnPlayer(stations_list_return_time); // "stations_list_return_time"
                                                               //  Serial.printf(" Display::_drawPlaylist \n");
}

void Display::_drawNextStationNum(uint16_t num) {
    timekeeper.waitAndReturnPlayer(30);
    _meta->setText(config.stationByNum(num));
    _nums->setText(num, "%d");
}

void Display::putRequest(displayRequestType_e type, int payload) {
    if (displayQueue == NULL) { return; }
#if DEBUG_VOLUME_SCREEN
    if (type == NEWMODE && payload == VOL) {
        Serial.printf("[VOLDBG][%lu][Display::putRequest] queued NEWMODE VOL current=%s net=%d\n",
                      millis(), volDbgModeName(_mode), network.status);
    }
#endif
    requestParams_t request;
    request.type = type;
    request.payload = payload;
    xQueueSend(displayQueue, &request, DSQ_SEND_DELAY);
    #ifdef USE_NEXTION
    nextion.putRequest(request);
    #endif
}

void Display::_layoutChange(bool played) {
    if (config.store.vumeter && _vuwidget) {
        if (played) {
            if (_vuwidget) { _vuwidget->unlock(); }
            //_clock->moveTo(clockMove);
            if (clockMove.width < 0) {
                _clock->moveBack();
            } else {
                _clock->moveTo(clockMove);
            }
            if (_weather) { _weather->moveTo(weatherMoveVU); }
        } else {
            if (_vuwidget) {
                if (!_vuwidget->locked()) { _vuwidget->lock(); }
            }
            _clock->moveBack();
            if (_weather) { _weather->moveBack(); }
        }
    } else {
        if (played) {
            if (clockMove.width < 0) {
                _clock->moveBack();
            } else {
                _clock->moveTo(clockMove);
            }
            if (_weather) { _weather->moveTo(weatherMove); }
            //_clock->moveBack();
        } else {
            if (_weather) { _weather->moveBack(); }
            _clock->moveBack();
        }
    }
}

void Display::loop() {
    PROFILE_SCOPE("display.loop");
    if (_bootStep == 0) {
        _pager->begin();
        _bootScreen();
        return;
    }
    if (displayQueue == NULL || _locked) { return; }
    {
        PROFILE_SCOPE("display.pager");
        _pager->loop();
    }
    if (_clock && (_mode == PLAYER || _mode == SCREENSAVER)) {
        PROFILE_SCOPE("display.clocktick");
        _clock->tick();
    }
    #ifdef USE_NEXTION
    {
        PROFILE_SCOPE("display.nextion");
        nextion.loop();
    }
    #endif
    requestParams_t request;
    bool hasRequest = false;
    {
        PROFILE_SCOPE("display.queue");
        hasRequest = xQueueReceive(displayQueue, &request, DSP_QUEUE_TICKS);
    }
    if (hasRequest) {
        PROFILE_SCOPE("display.request");
        bool pm_result = true;
        {
            PROFILE_SCOPE("display.plugins");
            pm.on_display_queue(request, pm_result);
        }
        if (pm_result) {
            switch (request.type) {
                case NEWMODE: {
                    PROFILE_SCOPE("display.mode");
                    _swichMode((displayMode_e)request.payload);
                    break;
                }
                case CLOSEPLAYLIST:
                    if (!(config.getMode() == PM_SDCARD &&
                          player.isRunning() &&
                          request.payload == config.lastStation())) {
                        player.sendCommand({PR_PLAY, request.payload});
                    }
                    break;
                case CLOCK: {
                    PROFILE_SCOPE("display.time");
                    if (_mode == PLAYER || _mode == SCREENSAVER) { _time(request.payload == 1); }
                    if (_mode == SD_PLAYER) {
                        PROFILE_SCOPE("display.sdplayer");
                        _sdPlayerScreen();
                    }
                    /*#ifdef USE_NEXTION
                      if(_mode==TIMEZONE) nextion.localTime(network.timeinfo);
                      if(_mode==INFO)     nextion.rssi();
                    #endif*/
                    break;
                }
                case NEWTITLE: {
                    PROFILE_SCOPE("display.title");
                    _title();
                    break;
                }
                case NEWSTATION: {
                    PROFILE_SCOPE("display.station");
                    _station();
                    break;
                }
                case NEXTSTATION: {
                    PROFILE_SCOPE("display.nextst");
                    _drawNextStationNum(request.payload);
                    break;
                }
                case DRAWPLAYLIST: {
                    PROFILE_SCOPE("display.playlist");
                    _drawPlaylist();
                    break;
                }
                case DRAWVOL: {
                    PROFILE_SCOPE("display.volume");
                    _volume();
                    break;
                }
                case DBITRATE: {
                    PROFILE_SCOPE("display.bitrate");
                    if (_mode == PLAYER) {
                        char buf[20];
                        snprintf(buf, 20, bitrateFmt, config.station.bitrate);
                        if (_bitrate) { _bitrate->setText(config.station.bitrate == 0 ? "" : buf); }
                        if (_fullbitrate) {
                            _fullbitrate->setBitrate(config.station.bitrate);
                            _fullbitrate->setFormat(config.configFmt);
                        }
                        if (_chtxt) {
                            if (_mode == VOL) {
                                _chtxt->setText("v8.8");
                            } else {
                                _chtxt->setText(config.lastStation(), "CH:%d");
                            }
                        }
                    }
                } break;
                case CLEARALLBITRATE: {    // "nameday"
                    PROFILE_SCOPE("display.clearbr");
                    if (_mode == PLAYER) {
                        if (_fullbitrate) { _fullbitrate->clearAll(); }
                    }
                } break;
                case AUDIOINFO: {
                    PROFILE_SCOPE("display.audioinf");
                    if (_heapbar) {
                        _heapbar->lock(!config.store.audioinfo);
                        _heapbar->setValue(player.inBufferFilled());
                    }
                    break;
                }
                case SHOWVUMETER: {
                    PROFILE_SCOPE("display.vumeter");
                    if (_vuwidget) {
                        _vuwidget->lock(!config.store.vumeter);
                        _layoutChange(player.isRunning());
                    }
                    break;
                }
                case SHOWWEATHER: {
                    PROFILE_SCOPE("display.weather");
                    if (_weather) { _weather->lock(!config.store.showweather); }
                    if (!config.store.showweather) {
    #ifndef HIDE_IP
                        if (_volip) { _volip->setText(config.ipToStr(WiFi.localIP()), iptxtFmt); }
    #endif
                    } else {
                        if (_weather) { _weather->setText(LANG::const_getWeather); }
                    }
                    break;
                }
                case NEWWEATHER: {
                    PROFILE_SCOPE("display.weather");
                    if (_weather && timekeeper.weatherBuf) { _weather->setText(timekeeper.weatherBuf); }
                    break;
                }
                case BOOTSTRING: {
                    PROFILE_SCOPE("display.boot");
                    if (_bootstring) { _bootstring->setText(config.ssids[request.payload].ssid, LANG::bootstrFmt); }
                    /*#ifdef USE_NEXTION
                      char buf[50];
                      snprintf(buf, 50, bootstrFmt, config.ssids[request.payload].ssid);
                      nextion.bootString(buf);
                    #endif*/
                    break;
                }
                case WAITFORSD: {
                    PROFILE_SCOPE("display.waitsd");
                    if (_bootstring) { _bootstring->setText(LANG::const_waitForSD); }
                    break;
                }
                case SDFILEINDEX: {
                    PROFILE_SCOPE("display.sdindex");
                    if (_mode == SDCHANGE) { _nums->setText(request.payload, "%d"); }
                    break;
                }
                case DSPRSSI: {
                    PROFILE_SCOPE("display.rssi");
                    if (_rssi) { _setRSSI(request.payload); }
                    if (_heapbar && config.store.audioinfo) { _heapbar->setValue(player.isRunning() ? player.inBufferFilled() : 0); }
                    break;
                }
                case PSTART: {
                    PROFILE_SCOPE("display.layout");
                    if (_mode == SD_PLAYER) {
                        _drawSdControls(true);
                    } else {
                        _layoutChange(true);
                    }
                    break;
                }
                case PSTOP: {
                    PROFILE_SCOPE("display.layout");
                    if (_mode == SD_PLAYER) {
                        _drawSdControls(true);
                    } else {
                        _layoutChange(false);
                    }
                    break;
                }
                case DSP_START: {
                    PROFILE_SCOPE("display.start");
                    _start();
                    break;
                }
                case NEWIP: {
                    PROFILE_SCOPE("display.ip");
    #ifndef HIDE_IP
                    if (_volip) { _volip->setText(config.ipToStr(WiFi.localIP()), iptxtFmt); }
    #endif
                    break;
                }
				case APPLY_THEME: {
                    PROFILE_SCOPE("display.theme");
                    _applyTheme();
                    break;
                }
                default:
                    break;

                    // check if there are more messages waiting in the Q, in this case break the loop() and go
                    // for another round to evict next message, do not waste time to redraw the screen, etc...
                    if (uxQueueMessagesWaiting(displayQueue)) { return; }
            }
        }
    }

    {
        PROFILE_SCOPE("display.draw");
        dsp.loop();
    }
    /*
    #if I2S_DOUT==255
    player.computeVUlevel();
    #endif
  */
}

void Display::_setRSSI(int rssi) {
    if (!_rssi) { return; }
    #if RSSI_DIGIT
    _rssi->setText(rssi, rssiFmt);
    return;
    #endif
    char rssiG[3];
    int  rssi_steps[] = {RSSI_STEPS};
    if (rssi >= rssi_steps[0]) { strlcpy(rssiG, "\004\006", 3); }
    if (rssi >= rssi_steps[1] && rssi < rssi_steps[0]) { strlcpy(rssiG, "\004\005", 3); }
    if (rssi >= rssi_steps[2] && rssi < rssi_steps[1]) { strlcpy(rssiG, "\004\002", 3); }
    if (rssi >= rssi_steps[3] && rssi < rssi_steps[2]) { strlcpy(rssiG, "\003\002", 3); }
    if (rssi < rssi_steps[3] || rssi >= 0) { strlcpy(rssiG, "\001\002", 3); }
    _rssi->setText(rssiG);
}

void Display::_station() {
    _meta->setAlign(metaConf.widget.align);
    if (config.station.name[0] == '.') {
        _meta->setText(config.station.name + 1);
    } else {
        _meta->setText(config.station.name);
    }
    #ifdef STATION_LOGO_WIDGET
    if (config.getMode() != PM_SDCARD) {
        _stationLogo.setStation(config.station.name);
    }
    #endif

    /*#ifdef USE_NEXTION
    nextion.newNameset(config.station.name);
    nextion.bitrate(config.station.bitrate);
    nextion.bitratePic(ICON_NA);
  #endif*/
}

char* split(char* str, const char* delim) {
    char* dmp = strstr(str, delim);
    if (dmp == NULL) { return NULL; }
    *dmp = '\0';
    return dmp + strlen(delim);
}

void Display::_title() {
    // Ha üres a title, használja a playlistben tárolt nevet.
    if (strlen(config.station.title) == 0) { strlcpy(config.station.title, config.station.name, sizeof(config.station.title)); }
    if (_mode == SD_PLAYER) {
        if (_sdtitle)  { PROFILE_SCOPE("title.sdtitle"); _sdtitle->setText(config.station.title); }
        if (_sdartist) { PROFILE_SCOPE("title.sdartist"); _sdartist->setText(config.station.sdArtist); }
        if (_sdalbum)  { PROFILE_SCOPE("title.sdalbum"); _sdalbum->setText(config.station.sdAlbum); }
        #ifdef SD_COVER_ART
        if (config.getMode() == PM_SDCARD) {
            PROFILE_SCOPE("title.sdcover");
            _sdcover.setTrack(config.station.url);
        }
        #endif
    } else if (strlen(config.station.title) > 0) {
        PROFILE_SCOPE("title.radio");
        char tmpbuf[strlen(config.station.title) + 1];
        strlcpy(tmpbuf, config.station.title, sizeof(tmpbuf));
        char* stitle = split(tmpbuf, " - ");
        if (stitle && _title2) {
            { PROFILE_SCOPE("title.line1"); _title1->setText(tmpbuf); }
            { PROFILE_SCOPE("title.line2"); _title2->setText(stitle); }
        } else {
            { PROFILE_SCOPE("title.line1"); _title1->setText(config.station.title); }
            if (_title2) { PROFILE_SCOPE("title.line2"); _title2->setText(""); }
        }
    } else {
        { PROFILE_SCOPE("title.line1"); _title1->setText(""); }
        if (_title2) { PROFILE_SCOPE("title.line2"); _title2->setText(""); }
    }
    if (player_on_track_change) {
        PROFILE_SCOPE("title.playerhook");
        player_on_track_change();
    }
    {
        PROFILE_SCOPE("title.pluginhook");
        pm.on_track_change();
    }
}

void Display::_time(bool redraw) {

    #if LIGHT_SENSOR != 255
    if (config.store.dspon) {
        PROFILE_SCOPE("time.light");
        config.store.brightness = AUTOBACKLIGHT(analogRead(LIGHT_SENSOR));
        config.setBrightness();
    }
    #endif
    if (config.isScreensaver && network.timeinfo.tm_sec % 60 == 0) {
        PROFILE_SCOPE("time.screensav");
    #if TIME_SIZE < 19
        uint16_t ft = static_cast<uint16_t>(random(TFT_FRAMEWDT, (dsp.height() - TIME_SIZE * CHARHEIGHT - TFT_FRAMEWDT)));
    #else
        uint16_t ft = static_cast<uint16_t>(random(TFT_FRAMEWDT + TIME_SIZE, (dsp.height() - _clock->dateSize() - TFT_FRAMEWDT * 2)));
    #endif
        uint16_t lt = static_cast<uint16_t>(random(TFT_FRAMEWDT, (dsp.width() - _clock->clockWidth() - TFT_FRAMEWDT)));
        if (clockConf.align == WA_CENTER) { lt -= (dsp.width() - _clock->clockWidth()) / 2; }
        //_clock->moveTo({clockConf.left, ft, 0});
        _clock->moveTo({lt, ft, 0});
    }
    {
        PROFILE_SCOPE("time.clockdraw");
        _clock->draw(redraw);
    }
    /*#ifdef USE_NEXTION
      nextion.printClock(network.timeinfo);
    #endif*/
}

void Display::_volume() {
    if (_volbar) {
        int vol = (config.store.volume);
        if (vol > VOLUME_CONTROL_STEPS) { vol = VOLUME_CONTROL_STEPS; }
        if (vol < 0) { vol = 0; }
        _volbar->setValue(vol);
    }
    #ifndef HIDE_VOL
    if (_voltxt) {
        _voltxt->setText(config.store.volume, voltxtFmt);
    }
    #endif
    if (_mode == VOL) {
        timekeeper.waitAndReturnPlayer(2);
        _nums->setText(config.store.volume, numtxtFmt);
    }
    /*#ifdef USE_NEXTION
      nextion.setVol(config.store.volume, _mode == VOL);
    #endif*/
}

void Display::flip() {
    dsp.flip();
}

void Display::invert() {
    dsp.invert();
}

void Display::setContrast() {
    #if DSP_MODEL == DSP_NOKIA5110
    dsp.setContrast(config.store.contrast);
    #endif
}

bool Display::deepsleep() {
    #if defined(LCD_I2C) || defined(DSP_OLED) || BRIGHTNESS_PIN != 255
    dsp.sleep();
    return true;
    #endif
    return false;
}

void Display::wakeup() {
    #if defined(LCD_I2C) || defined(DSP_OLED) || BRIGHTNESS_PIN != 255
    dsp.wake();
    #endif
}

void Display::clear(bool black) {
    dsp.clearDsp(black);
}

void Display::setBrightnessPercent(uint8_t percent) {
    // Serial.printf("display.cpp--> setBrightnessPercent(%d)\n", percent);
    percent = constrain(percent, 0, 100);
    #if DSP_MODEL == DSP_SSD1322
    uint8_t master = map(percent, 0, 100, 0, 15);
    uint8_t contrast = map(percent, 0, 100, 0, 255);
    dsp.ssd1322_setMasterContrast(master);
    dsp.ssd1322_setContrast(contrast);
    #else
        #if (BRIGHTNESS_PIN != 255)
    analogWrite(BRIGHTNESS_PIN, map(percent, 0, 100, 0, 255));
        #endif
    #endif
}

//============================================================================================================================
#else // !DUMMYDISPLAY
//============================================================================================================================
void Display::init() {
    _createDspTask();
    #ifdef USE_NEXTION
    nextion.begin(true);
    #endif
}
void Display::_start() {
    #ifdef USE_NEXTION
    // nextion.putcmd("page player");
    nextion.start();
    #endif
    config.setTitle(LANG::const_PlReady);
}

void Display::putRequest(displayRequestType_e type, int payload) {
    if (type == DSP_START) { _start(); }
    #ifdef USE_NEXTION
    requestParams_t request;
    request.type = type;
    request.payload = payload;
    nextion.putRequest(request);
    #else
    if (type == NEWMODE) { mode((displayMode_e)payload); }
    #endif
}
//============================================================================================================================
#endif // DUMMYDISPLAY
