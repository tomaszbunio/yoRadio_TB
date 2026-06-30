#include "backlight.h"
#include <Arduino.h>
#include "../../core/options.h"
#include "../../core/config.h"
#include "../../core/display.h"
#include "../../core/network.h"

#if (BRIGHTNESS_PIN != 255)

BacklightPlugin backlightPlugin;

BacklightPlugin::BacklightPlugin() {}

void backlightPluginInit() {
    pm.add(&backlightPlugin);
}

bool BacklightPlugin::isFading() const {
    // Serial.println("backlight.cpp-->isFading()");
    return state == FADING;
}

bool BacklightPlugin::isDimmed() const {
    //  Serial.println("backlight.cpp-->isDimmed()");
    return state == DIMMED;
}

bool BacklightPlugin::isFadeControl() {
    if (state == FADING || state == DIMMED) {
        backlightPlugin.wake();
        return true;
    }
    activity();
    if ((millis() - lastUiWakeMs) < 500) {
        return true;
    } else {
        return false;
    }
}

void BacklightPlugin::activity() {
    lastActivity = millis();
}

void BacklightPlugin::setBacklight(uint8_t backLight) {
    #if BRIGHTNESS_PIN != 255
    if (!config.store.dspon) { display.wakeup(); }
    Serial.printf("##FADE -> %3d %%\n", backLight);
    display.setBrightnessPercent(backLight);
    if (!config.store.dspon) { config.store.dspon = true; }
    #endif
}

void BacklightPlugin::wake() {
    if (network.softStandby) return;
    Serial.println("##BACKLIGHT -> wake");
    if (!brightnessCaptured) return;
    setBacklight(config.store.brightness);
    lastUiWakeMs = millis();
    lastActivity = millis();
    brightnessCaptured = false;
    state = WAIT;
}

void BacklightPlugin::tick() {
    if (network.status == SOFT_AP) return;
    if (!display.ready()) return;
    // Serial.printf("Backlight.cpp-->config.store.fadeEnabled: %d\n",config.store.fadeEnabled);
    // Serial.printf("Backlight.cpp-->config.store.fadeStartDelay: %d\n",config.store.fadeStartDelay);
    // Serial.printf("Backlight.cpp-->config.store.fadeTarget: %d\n",config.store.fadeTarget);
    // Serial.printf("Backlight.cpp-->config.store.fadeStep: %d\n",config.store.fadeStep);
    if (config.store.fadeEnabled == 0) {
        if (state == FADING || state == DIMMED) { wake(); }
        return;
    }
    displayMode_e m = display.mode();
    if (m != lastMode) {
        lastMode = m;
        wake();
    }
    if (display.mode() != PLAYER) { return; }
    if (!brightnessCaptured && config.store.brightness > 0) {
        savedBrightness = config.store.brightness;
        currentBrightness = savedBrightness;
        brightnessCaptured = true;
    }
    uint32_t now = millis();
    switch (state) {
        case WAIT:
            if (now - lastActivity > config.store.fadeStartDelay * 1000) {
                targetBrightness = config.store.fadeTarget;
                state = FADING;
                lastFadeStep = now;
            }
            break;
        case FADING:
            if (now - lastFadeStep < FADE_PERIOD) break;
            lastFadeStep = now;
            if (currentBrightness > targetBrightness) {
                if (currentBrightness <= config.store.fadeStep)
                    currentBrightness = targetBrightness;
                else
                    currentBrightness -= config.store.fadeStep;
                setBacklight(currentBrightness);
            } else {
                state = DIMMED;
            }
            break;
        case DIMMED: break;
    }
}

void BacklightPlugin::on_setup() {
    Serial.printf("BACKLIGHT -> .on_setup this=%p\n", this);
}

void BacklightPlugin::on_ticker() {
    tick();
}

void BacklightPlugin::on_start_play() {
    wake();
}

void BacklightPlugin::on_stop_play() {
    wake();
}

void BacklightPlugin::on_track_change() {
    // wake();
}

void BacklightPlugin::on_btn_click(controlEvt_e& btnid) {
    wake();
}

void BacklightPlugin::on_display_player() {
    wake();
}

#endif
