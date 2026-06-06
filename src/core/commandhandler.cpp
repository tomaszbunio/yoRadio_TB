// "nameday"
#include "options.h"
#include "commandhandler.h"
#include "player.h"
#include "display.h"
#include "netserver.h"
#include "config.h"
#include "controls.h"
#include "telnet.h"
#include "../clock_tts/clock_tts.h"
#include "../displays/dspcore.h"
#include "../plugins/backlight/backlight.h"
#include "../NeoPixel/NeoPixel.h"

#if DSP_MODEL == DSP_DUMMY
    #define DUMMYDISPLAY
#endif

CommandHandler cmd;

bool CommandHandler::exec(const char* command, const char* value, uint8_t cid) {
    // Serial.printf("commandhandler.cpp--> command: %s, value: %s, cId: %d \n", command, value, cid);
    if (strEquals(command, "start")) {
        // Serial.printf("commandhandler.cpp--> START \n");
        player.sendCommand({PR_PLAY, config.lastStation()});
        return true;
    }
    if (strEquals(command, "stop")) {
        // Serial.printf("commandhandler.cpp--> STOP \n");
        player.sendCommand({PR_STOP, 0});
        return true;
    }
    if (strEquals(command, "toggle")) {
        //  Serial.printf("commandhandler.cpp-->  player.toggle() \"toggle\" \n");
        player.toggle();
        /*
       uint32_t pos = strtoul(value, nullptr, 10);
       if (pos == 1) {
         player.toggle();
       } else {
         player.toggleFromWeb(pos);
       }
         */
        return true;
    }
    if (strEquals(command, "prev")) {
        player.prev();
        return true;
    }
    if (strEquals(command, "next")) {
        player.next();
        return true;
    }
    if (strEquals(command, "volm")) {
        player.stepVol(false);
        return true;
    }
    if (strEquals(command, "volp")) {
        player.stepVol(true);
        return true;
    }
#ifdef USE_SD
    if (strEquals(command, "mode")) {
        config.changeMode(atoi(value));
        return true;
    }
#endif
    if (strEquals(command, "reset") && cid == 0) {
        config.reset();
        return true;
    }
    // if (strEquals(command, "ballance")) { config.setBalance(atoi(value)); return true; }
    if (strEquals(command, "playstation") || strEquals(command, "play")) {
        // Serial.printf("commandhandler.cpp--> command: %s\n", command);
        int id = atoi(value);
        if (id < 1) { id = 1; }
        uint16_t cs = config.playlistLength();
        if (id > cs) { id = cs; }
        player.sendCommand({PR_PLAY, id});
        return true;
    }
    if (strEquals(command, "vol")) {
        int v = atoi(value);
        config.store.volume = v < 0 ? 0 : (v > 100 ? 100 : v);
        player.setVol(v);
        return true;
    }
    if (strEquals(command, "dspon")) {
        config.setDspOn(atoi(value) != 0);
        return true;
    }
    if (strEquals(command, "dim")) {
        int d = atoi(value);
        config.store.brightness = (uint8_t)(d < 0 ? 0 : (d > 100 ? 100 : d));
        config.setBrightness(true);
        return true;
    }

    if (strEquals(command, "clearspiffs")) {
        config.spiffsCleanup();
        config.saveValue(&config.store.play_mode, static_cast<uint8_t>(PM_WEB));
        return true;
    }
    /*********************************************/
    /****************** WEBSOCKET ****************/
    /*********************************************/
	 // --- Google clock TTS (options.html) ---
  if (strEquals(command, "ttsgoogle")) {
    bool en = static_cast<bool>(atoi(value));
    config.saveValue(&config.store.ttsgoogle, en);
    clock_tts_enable(en);
    return true;
  }
  if (strEquals(command, "ttsclock")) {
    uint16_t mins = static_cast<uint16_t>(atoi(value));
    if (mins < 1) mins = 1;
    config.saveValue(&config.store.ttsclock, mins);
    clock_tts_set_interval((int)mins);
    return true;
  }
#ifdef NEOPIXEL_ON
  if (strEquals(command, "neopixel_enabled")) {
    uint8_t en = static_cast<uint8_t>(atoi(value) ? 1 : 0);
    config.saveValue(&config.store.neopixel_enabled, en);
    NeoPixel_setEnabled(en == 1);
    return true;
  }
  if (strEquals(command, "neopixel_brightness")) {
    int b = atoi(value);
    if (b < 0) b = 0;
    if (b > 255) b = 255;
    config.saveValue(&config.store.neopixel_brightness, static_cast<uint8_t>(b));
    NeoPixel_setBrightness(config.store.neopixel_brightness);
    return true;
  }
  if (strEquals(command, "neopixel_effect")) {
    uint8_t effect = static_cast<uint8_t>(atoi(value));
    if (effect > 4) effect = 4;
    config.saveValue(&config.store.neopixel_effect, effect);
    NeoPixel_setEffect(effect);
    return true;
  }
  if (strEquals(command, "neopixel_effect2")) {
    uint8_t effect = static_cast<uint8_t>(atoi(value));
    if (effect > 4) effect = 4;
    config.saveValue(&config.store.neopixel_effect2, effect);
    NeoPixel_applyConfig();
    return true;
  }
  if (strEquals(command, "neopixel_enc1_color")) {
    uint16_t col = Config::htmlColorTo565(value);
    config.saveValue(&config.store.neopixel_enc1_color, col, false);
    config.scheduleEEPROMCommit();
    NeoPixel_setEncoderColor(1, col);
    return true;
  }
  if (strEquals(command, "neopixel_enc2_color")) {
    uint16_t col = Config::htmlColorTo565(value);
    config.saveValue(&config.store.neopixel_enc2_color, col, false);
    config.scheduleEEPROMCommit();
    NeoPixel_setEncoderColor(2, col);
    return true;
  }
  if (strEquals(command, "neopixel_rotate1_enabled")) {
    config.saveValue(&config.store.neopixel_rotate1_enabled, static_cast<uint8_t>(atoi(value) ? 1 : 0));
    NeoPixel_applyConfig();
    return true;
  }
  if (strEquals(command, "neopixel_rotate2_enabled")) {
    config.saveValue(&config.store.neopixel_rotate2_enabled, static_cast<uint8_t>(atoi(value) ? 1 : 0));
    NeoPixel_applyConfig();
    return true;
  }
  if (strEquals(command, "neopixel_rotate1_effect")) {
    uint8_t effect = static_cast<uint8_t>(atoi(value));
    if (effect > 4) effect = 4;
    config.saveValue(&config.store.neopixel_rotate1_effect, effect);
    NeoPixel_applyConfig();
    return true;
  }
  if (strEquals(command, "neopixel_rotate2_effect")) {
    uint8_t effect = static_cast<uint8_t>(atoi(value));
    if (effect > 4) effect = 4;
    config.saveValue(&config.store.neopixel_rotate2_effect, effect);
    NeoPixel_applyConfig();
    return true;
  }
  if (strEquals(command, "neopixel_rotate1_color")) {
    uint16_t col = Config::htmlColorTo565(value);
    config.saveValue(&config.store.neopixel_rotate1_color, col, false);
    config.scheduleEEPROMCommit();
    NeoPixel_applyConfig();
    return true;
  }
  if (strEquals(command, "neopixel_rotate2_color")) {
    uint16_t col = Config::htmlColorTo565(value);
    config.saveValue(&config.store.neopixel_rotate2_color, col, false);
    config.scheduleEEPROMCommit();
    NeoPixel_applyConfig();
    return true;
  }
#endif

  // --- Clock font (options.html) ---
  if (strEquals(command, "clockfont")) {
    uint8_t id = static_cast<uint8_t>(atoi(value));
    // Save only (no heavy display operations in AsyncWebSocket task)
	Serial.printf("commandhandler clockfont: value='%s' id=%d\n", value, id);
    config.saveValue(&config.store.clockfont, id, false);
    config.scheduleEEPROMCommit();
    config.scheduleUiApply(Config::UI_APPLY_CLOCKFONT);
    return true;
  }
  if (strEquals(command, "clockmode")) {
    uint8_t mode = static_cast<uint8_t>(atoi(value) ? 1 : 0);
    config.saveValue(&config.store.clockmode, mode, false);
    config.scheduleEEPROMCommit();
    config.scheduleUiApply(Config::UI_APPLY_CLOCK);
    return true;
  }
  if (strEquals(command, "clockseconds")) {
    uint8_t seconds = static_cast<uint8_t>(atoi(value) ? 1 : 0);
    config.saveValue(&config.store.clockseconds, seconds, false);
    config.scheduleEEPROMCommit();
    config.scheduleUiApply(Config::UI_APPLY_CLOCK);
    return true;
  }

  // --- Custom display theme (options.html) ---
  if (strEquals(command, "thememode")) {
    config.saveValue(&config.store.thememode, static_cast<bool>(atoi(value)), false);
    config.scheduleEEPROMCommit();
    config.scheduleUiApply(Config::UI_APPLY_THEME);
    return true;
  }
  if (strEquals(command, "tbg")) {
    uint16_t col = Config::htmlColorTo565(value);
    config.saveValue(&config.store.tbg, col, false);
    config.scheduleEEPROMCommit();
    config.scheduleUiApply(Config::UI_APPLY_THEME);
    return true;
  }
  if (strEquals(command, "tpr")) {
    uint16_t col = Config::htmlColorTo565(value);
    config.saveValue(&config.store.tpr, col, false);
    config.scheduleEEPROMCommit();
    config.scheduleUiApply(Config::UI_APPLY_THEME);
    return true;
  }
  if (strEquals(command, "tac")) {
    uint16_t col = Config::htmlColorTo565(value);
	Serial.printf("tac: value=%s col=0x%04X\n", value, col);
    config.saveValue(&config.store.tac, col, false);
    // Keep classic/flip clock text pickers aligned (some UIs send tac, some tfliptext).
    config.saveValue(&config.store.tfliptext, col, false);
    config.scheduleEEPROMCommit();
    config.scheduleUiApply(Config::UI_APPLY_THEME);
    return true;
  }
  if (strEquals(command, "tt1")) {
    uint16_t col = Config::htmlColorTo565(value);
    config.saveValue(&config.store.tt1, col, false);
    config.scheduleEEPROMCommit();
    config.scheduleUiApply(Config::UI_APPLY_THEME);
    return true;
  }
  if (strEquals(command, "tt2")) {
    uint16_t col = Config::htmlColorTo565(value);
    config.saveValue(&config.store.tt2, col, false);
    config.scheduleEEPROMCommit();
    config.scheduleUiApply(Config::UI_APPLY_THEME);
    return true;
  }
  if (strEquals(command, "tw")) {
    uint16_t col = Config::htmlColorTo565(value);
    config.saveValue(&config.store.tw, col, false);
    config.scheduleEEPROMCommit();
    config.scheduleUiApply(Config::UI_APPLY_THEME);
    return true;
  }
  if (strEquals(command, "tvmax")) {
    uint16_t col = Config::htmlColorTo565(value);
    config.saveValue(&config.store.tvmax, col, false);
    config.scheduleEEPROMCommit();
    config.scheduleUiApply(Config::UI_APPLY_THEME);
    return true;
  }
  if (strEquals(command, "tvmid")) {
    uint16_t col = Config::htmlColorTo565(value);
    config.saveValue(&config.store.tvmid, col, false);
    config.scheduleEEPROMCommit();
    config.scheduleUiApply(Config::UI_APPLY_THEME);
    return true;
  }
  if (strEquals(command, "tvmin")) {
    uint16_t col = Config::htmlColorTo565(value);
    config.saveValue(&config.store.tvmin, col, false);
    config.scheduleEEPROMCommit();
    config.scheduleUiApply(Config::UI_APPLY_THEME);
    return true;
  }
// Extra theme colors (options.html)
if (strEquals(command, "tdig")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.tdig, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}
if (strEquals(command, "tdiv")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.tdiv, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}
if (strEquals(command, "tnameday")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.tnameday, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}
if (strEquals(command, "tdate")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.tdate, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}
if (strEquals(command, "theap")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.theap, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}
if (strEquals(command, "tbuffer")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.tbuffer, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}
if (strEquals(command, "tip")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.tip, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}
if (strEquals(command, "tvol")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.tvol, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}
if (strEquals(command, "tvolbar")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.tvolbar, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}
if (strEquals(command, "tch")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.tch, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}
if (strEquals(command, "trssi")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.trssi, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}
if (strEquals(command, "tbitrate")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.tbitrate, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}
if (strEquals(command, "tseconds")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.tseconds, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}
if (strEquals(command, "tfliptext")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.tfliptext, col, false);
  // Keep classic and flip clock text colors in sync unconditionally.
  // This avoids edge-cases when clockmode updates arrive out of order.
  config.saveValue(&config.store.tac, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}
if (strEquals(command, "tflipcard")) {
  uint16_t col = Config::htmlColorTo565(value);
  config.saveValue(&config.store.tflipcard, col, false);
  config.scheduleEEPROMCommit();
  config.scheduleUiApply(Config::UI_APPLY_THEME);
  return true;
}


	if (strEquals(command, "ttsgoogle")) {
    bool en = static_cast<bool>(atoi(value));
    config.saveValue(&config.store.ttsgoogle, en);
    clock_tts_enable(en);
    return true;
  }
  if (strEquals(command, "ttsclock")) {
    uint16_t mins = static_cast<uint16_t>(atoi(value));
    if (mins < 1) mins = 1;
    config.saveValue(&config.store.ttsclock, mins);
    clock_tts_set_interval((int)mins);
    return true;
  }
	
    if (strEquals(command, "getindex")) {
        netserver.requestOnChange(GETINDEX, cid);
        return true;
    }

    if (strEquals(command, "getsystem")) {
        netserver.requestOnChange(GETSYSTEM, cid);
        return true;
    }
    if (strEquals(command, "getscreen")) {
        netserver.requestOnChange(GETSCREEN, cid);
        return true;
    }
    if (strEquals(command, "gettimezone")) {
        netserver.requestOnChange(GETTIMEZONE, cid);
        return true;
    }
    if (strEquals(command, "getcontrols")) {
        netserver.requestOnChange(GETCONTROLS, cid);
        return true;
    }
    if (strEquals(command, "getweather")) {
        netserver.requestOnChange(GETWEATHER, cid);
        return true;
    }
    if (strEquals(command, "getactive")) {
        netserver.requestOnChange(GETACTIVE, cid);
        return true;
    }
    if (strEquals(command, "newmode")) {
        config.newConfigMode = atoi(value);
        netserver.requestOnChange(CHANGEMODE, cid);
        return true;
    }

    if (strEquals(command, "invertdisplay")) {
        config.saveValue(&config.store.invertdisplay, static_cast<bool>(atoi(value)));
        display.invert();
        return true;
    }
    if (strEquals(command, "numplaylist")) {
        config.saveValue(&config.store.numplaylist, static_cast<bool>(atoi(value)));
        display.resetPlaylistCache();
        display.putRequest(NEWMODE, CLEAR);
        display.putRequest(NEWMODE, PLAYER);
        return true;
    }
    if (strEquals(command, "fliptouch")) {
        config.saveValue(&config.store.fliptouch, static_cast<bool>(atoi(value)));
        flipTS();
        return true;
    }
    if (strEquals(command, "dbgtouch")) {
        config.saveValue(&config.store.dbgtouch, static_cast<bool>(atoi(value)));
        return true;
    }
    if (strEquals(command, "flipscreen")) {
        config.saveValue(&config.store.flipscreen, static_cast<bool>(atoi(value)));
        display.flip();
        display.putRequest(NEWMODE, CLEAR);
        display.putRequest(NEWMODE, config.getMode() == PM_SDCARD ? SD_PLAYER : PLAYER);
        return true;
    }
    if (strEquals(command, "brightness")) {
        if (!config.store.dspon) { netserver.requestOnChange(DSPON, 0); }
        config.store.brightness = static_cast<uint8_t>(atoi(value));
        config.setBrightness(true);
        if (BRIGHTNESS_PIN != 255 && config.store.fadeEnabled) { // WEB UI backlight plugin
            if (backlightPlugin.isDimmed() || backlightPlugin.isFading())
                backlightPlugin.wake();
            else
                backlightPlugin.activity();
        }
        return true;
    }
    if (strEquals(command, "screenon")) {
        config.setDspOn(static_cast<bool>(atoi(value)));
        return true;
    }
    if (strEquals(command, "contrast")) {
        config.saveValue(&config.store.contrast, static_cast<uint8_t>(atoi(value)));
        display.setContrast();
        return true;
    }
    if (strEquals(command, "screensaverenabled")) {
        config.enableScreensaver(static_cast<bool>(atoi(value)));
        return true;
    }
    if (strEquals(command, "screensavertimeout")) {
        config.setScreensaverTimeout(static_cast<uint16_t>(atoi(value)));
        return true;
    }
    if (strEquals(command, "screensaverblank")) {
        config.setScreensaverBlank(static_cast<bool>(atoi(value)));
        return true;
    }
    if (strEquals(command, "screensaverplayingenabled")) {
        config.setScreensaverPlayingEnabled(static_cast<bool>(atoi(value)));
        return true;
    }
    if (strEquals(command, "screensaverplayingtimeout")) {
        config.setScreensaverPlayingTimeout(static_cast<uint16_t>(atoi(value)));
        return true;
    }
    if (strEquals(command, "screensaverplayingblank")) {
        config.setScreensaverPlayingBlank(static_cast<bool>(atoi(value)));
        return true;
    }
    /***** AUTO FADE *****/
    if (cmd.strEquals(command, "fadeenabled")) {
        config.saveValue(&config.store.fadeEnabled, static_cast<uint8_t>(atoi(value)), true, false);
        return true;
    }
    if (cmd.strEquals(command, "fadestartdelay")) {
        config.saveValue(&config.store.fadeStartDelay, static_cast<uint16_t>(atoi(value)), true, false);
        return true;
    }
    if (cmd.strEquals(command, "fadetarget")) {
        uint8_t target = atoi(value);
        if (target > 100) target = 100;
        config.saveValue(&config.store.fadeTarget, target, true, false);
        return true;
    }
    if (cmd.strEquals(command, "fadestep")) {
        uint8_t step = atoi(value);
        if (step > 100) step = 100;
        config.saveValue(&config.store.fadeStep, step, true, false);
        return true;
    }
    /*********************/
    if (strEquals(command, "abuff")) {
        config.saveValue(&config.store.abuff, static_cast<uint16_t>(atoi(value)));
        return true;
    }
    if (strEquals(command, "telnet")) {
        config.saveValue(&config.store.telnet, static_cast<bool>(atoi(value)));
        telnet.toggle();
        return true;
    }
    if (strEquals(command, "watchdog")) {
        config.saveValue(&config.store.watchdog, static_cast<bool>(atoi(value)));
        return true;
    }
    if (strEquals(command, "nameday")) {
        display.putRequest(CLEARALLBITRATE);                                     // Törli mindkét bitratewidget és a nameday területet.
        config.saveValue(&config.store.nameday, static_cast<bool>(atoi(value))); // Elmenti a gomb beállítását
        display.putRequest(DBITRATE);
        return true;
    }
    if (strEquals(command, "tzh")) {
        config.saveValue(&config.store.tzHour, static_cast<int8_t>(atoi(value)));
        return true;
    }
    if (strEquals(command, "tzm")) {
        config.saveValue(&config.store.tzMin, static_cast<int8_t>(atoi(value)));
        return true;
    }
    if (strEquals(command, "sntp2")) {
        config.saveValue(config.store.sntp2, value, 35, false);
        return true;
    }
    if (strEquals(command, "sntp1")) {
        config.setSntpOne(value);
        return true;
    }
    if (strEquals(command, "timeint")) {
        config.saveValue(&config.store.timeSyncInterval, static_cast<uint16_t>(atoi(value)));
        return true;
    }
    if (strEquals(command, "timeintrtc")) {
        config.saveValue(&config.store.timeSyncIntervalRTC, static_cast<uint16_t>(atoi(value)));
        return true;
    }

    if (strEquals(command, "volsteps")) {
        config.saveValue(&config.store.volsteps, static_cast<uint8_t>(atoi(value)));
        return true;
    }
    if (strEquals(command, "encacc")) {
        setEncAcceleration(static_cast<uint16_t>(atoi(value)));
        return true;
    }
    if (strEquals(command, "irtlp")) {
        setIRTolerance(static_cast<uint8_t>(atoi(value)));
        return true;
    }
    if (strEquals(command, "oneclickswitching")) {
        config.saveValue(&config.store.skipPlaylistUpDown, static_cast<bool>(atoi(value)));
        return true;
    }
    if (strEquals(command, "showweather")) {
        config.setShowweather(static_cast<bool>(atoi(value)));
        return true;
    }
    if (strEquals(command, "lat")) {
        config.saveValue(config.store.weatherlat, value, 10, false);
        return true;
    }
    if (strEquals(command, "lon")) {
        config.saveValue(config.store.weatherlon, value, 10, false);
        return true;
    }
    if (strEquals(command, "key")) {
        config.setWeatherKey(value);
        return true;
    }
    if (strEquals(command, "wint")) {
        config.saveValue(&config.store.weatherSyncInterval, static_cast<uint16_t>(atoi(value)));
        return true;
    }
    if (strEquals(command, "volume")) {
        player.setVol(static_cast<uint8_t>(atoi(value)));
        return true;
    }
    if (strEquals(command, "sdpos")) {
        ;
        uint32_t sdpos = static_cast<uint32_t>(atoi(value));
        config.setSDpos(sdpos);
        return true;
    }
    if (strEquals(command, "snuffle")) { // véletlen lejátszás
        config.setSnuffle(strcmp(value, "true") == 0);
        return true;
    }
    if (strEquals(command, "balance")) {
        config.setBalance(static_cast<int8_t>(atoi(value)));
        return true;
    }
    if (strEquals(command, "reboot")) {
        ESP.restart();
        return true;
    }
    if (strEquals(command, "boot")) {
        ESP.restart();
        return true;
    }
    if (strEquals(command, "format")) {
        SPIFFS.format();
        ESP.restart();
        return true;
    }
    if (strEquals(command, "submitplaylist")) {
        player.sendCommand({PR_STOP, 0});
        return true;
    }

#if IR_PIN != 255
    if (strEquals(command, "irbtn")) {
        config.setIrBtn(atoi(value));
        return true;
    }
    if (strEquals(command, "chkid")) {
        int slot = atoi(value);
        if (slot >= 0 && slot < 3) config.irchck = static_cast<uint8_t>(slot);
        return true;
    }
    if (strEquals(command, "irclr")) {
        int slot = atoi(value);
        if (config.irindex >= 0 && config.irindex < IR_ACTION_COUNT && slot >= 0 && slot < 3) {
            config.ircodes.irVals[config.irindex][slot] = 0;
        }
        return true;
    }
#endif
    if (strEquals(command, "reset")) {
        config.resetSystem(value, cid);
        return true;
    }

    if (strEquals(command, "smartstart")) {
        uint8_t ss = atoi(value) == 1 ? 1 : 2;
        config.setSmartStart(ss);
        return true;
    }
    if (strEquals(command, "audioinfo")) {
        config.saveValue(&config.store.audioinfo, static_cast<bool>(atoi(value)));
        display.putRequest(AUDIOINFO);
        return true;
    }
    if (strEquals(command, "vumeter")) {
        config.saveValue(&config.store.vumeter, static_cast<bool>(atoi(value)));
        display.putRequest(SHOWVUMETER);
        return true;
    }
    if (strEquals(command, "softap")) {
        config.saveValue(&config.store.softapdelay, static_cast<uint8_t>(atoi(value)));
        return true;
    }
    if (strEquals(command, "mdnsname")) {
        config.saveValue(config.store.mdnsname, value, MDNS_LENGTH);
        return true;
    }
    if (strEquals(command, "rebootmdns")) {
        if (strlen(config.store.mdnsname) > 0) {
            snprintf(config.tmpBuf, sizeof(config.tmpBuf), "{\"redirect\": \"http://%s.local/settings.html\"}", config.store.mdnsname);
        } else {
            snprintf(config.tmpBuf, sizeof(config.tmpBuf), "{\"redirect\": \"http://%s/settings.html\"}", config.ipToStr(WiFi.localIP()));
        }
        websocket.text(cid, config.tmpBuf);
        delay(500);
        ESP.restart();
        return true;
    }

    return false;
}
