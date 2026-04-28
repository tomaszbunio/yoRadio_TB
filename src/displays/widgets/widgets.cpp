#include "../../core/options.h"
#if DSP_MODEL != DSP_DUMMY
    #ifdef NAMEDAYS_FILE
        #include "../../core/namedays.h"
    #endif

    #include "../../core/config.h"
    #include "../../core/network.h" //  for Clock widget
    #include "../../core/player.h"  //  for VU widget
    #include "../dspcore.h"
    #include "../tools/l10n.h"
    #include "../tools/psframebuffer.h"
    #include "Arduino.h"
    #include "widgets.h"
	
	#include "../fonts/dsfont70.h"
// clang-format on

/************************
      FILL WIDGET
 ************************/
void FillWidget::init(FillConfig conf, uint16_t bgcolor) {
    Widget::init(conf.widget, bgcolor, bgcolor);
    _width = conf.width;
    _height = conf.height;
}

void FillWidget::_draw() {
    if (!_active) { return; }
    dsp.fillRect(_config.left, _config.top, _width, _height, _bgcolor);
}

void FillWidget::setHeight(uint16_t newHeight) {
    _height = newHeight;
}
/************************
      TEXT WIDGET
 ************************/
TextWidget::~TextWidget() {
    free(_text);
    free(_oldtext);
}

void TextWidget::_charSize(uint8_t textsize, uint8_t& width, uint16_t& height) {
    #ifndef DSP_LCD
    width = textsize * CHARWIDTH;
    height = textsize * CHARHEIGHT;
    #else
    width = 1;
    height = 1;
    #endif
}

void TextWidget::init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor) {
    Widget::init(wconf, fgcolor, bgcolor);
    _buffsize = buffsize;
    _text = (char*)malloc(sizeof(char) * _buffsize);
    memset(_text, 0, _buffsize);
    _oldtext = (char*)malloc(sizeof(char) * _buffsize);
    memset(_oldtext, 0, _buffsize);
    _charSize(_config.textsize, _charWidth, _textheight);
    _textwidth = _oldtextwidth = _oldleft = 0;
    _uppercase = uppercase;
}

void TextWidget::setText(const char* txt) {
    strlcpy(_text, utf8To(txt, _uppercase), _buffsize);
    _textwidth = strlen(_text) * _charWidth;
    if (strcmp(_oldtext, _text) == 0) { return; }

    if (_active) { dsp.fillRect(_oldleft == 0 ? _realLeft() : min(_oldleft, _realLeft()), _config.top, max(_oldtextwidth, _textwidth), _textheight, _bgcolor); }

    _oldtextwidth = _textwidth;
    _oldleft = _realLeft();
    if (_active) { _draw(); }
}

void TextWidget::setText(int val, const char* format) {
    char buf[_buffsize];
    snprintf(buf, _buffsize, format, val);
    setText(buf);
}

void TextWidget::setText(const char* txt, const char* format) {
    char buf[_buffsize];
    snprintf(buf, _buffsize, format, txt);
    setText(buf);
}

uint16_t TextWidget::_realLeft(bool w_fb) {
    uint16_t realwidth = (_width > 0 && w_fb) ? _width : dsp.width();
    switch (_config.align) {
        case WA_CENTER: return (realwidth - _textwidth) / 2; break;
        case WA_RIGHT: return (realwidth - _textwidth - (!w_fb ? _config.left : 0)); break;
        default: return !w_fb ? _config.left : 0; break;
    }
}

void TextWidget::_draw() {
    if (!_active) { return; }
    dsp.setTextColor(_fgcolor, _bgcolor);
    dsp.setCursor(_realLeft(), _config.top);
    dsp.setFont();
    dsp.setTextSize(_config.textsize);
    dsp.print(_text);
    strlcpy(_oldtext, _text, _buffsize);
}

/**************************************************************************************************************
                                                  SCROLL WIDGET
 **************************************************************************************************************/
ScrollWidget::ScrollWidget(const char* separator, ScrollConfig conf, uint16_t fgcolor, uint16_t bgcolor) {
    init(separator, conf, fgcolor, bgcolor);
}

ScrollWidget::~ScrollWidget() {
    free(_fb);
    free(_sep);
    free(_window);
}

void ScrollWidget::init(const char* separator, ScrollConfig conf, uint16_t fgcolor, uint16_t bgcolor) {
    TextWidget::init(conf.widget, conf.buffsize, conf.uppercase, fgcolor, bgcolor);
    _sep = (char*)malloc(sizeof(char) * 4);
    memset(_sep, 0, 4);
    snprintf(_sep, 4, " %.*s ", 1, separator);
    _x = conf.widget.left;
    _startscrolldelay = conf.startscrolldelay;
    _scrolldelta = conf.scrolldelta;
    _scrolltime = conf.scrolltime;
    _charSize(_config.textsize, _charWidth, _textheight);
    _sepwidth = strlen(_sep) * _charWidth;
    _width = conf.width;
    _backMove.width = _width;
    _window = (char*)malloc(sizeof(char) * (MAX_WIDTH / _charWidth + 1));
    memset(_window, 0, (MAX_WIDTH / _charWidth + 1)); // +1?
    _doscroll = false;
    #ifdef PSFBUFFER
    _fb = new psFrameBuffer(dsp.width(), dsp.height());
    uint16_t _rl = (_config.align == WA_CENTER) ? (dsp.width() - _width) / 2 : _config.left;
    _fb->begin(&dsp, _rl, _config.top, _width, _textheight, _bgcolor);
    #endif
}

void ScrollWidget::_setTextParams() {
    if (_config.textsize == 0) { return; }
    if (_fb->ready()) {
    #ifdef PSFBUFFER
        _fb->setTextSize(_config.textsize);
        _fb->setTextColor(_fgcolor, _bgcolor);
    #endif
    } else {
        dsp.setTextSize(_config.textsize);
        dsp.setTextColor(_fgcolor, _bgcolor);
    }
}

bool ScrollWidget::_checkIsScrollNeeded() {
    return _textwidth > _width;
}

void ScrollWidget::setText(const char* txt) {
    strlcpy(_text, utf8To(txt, _uppercase), _buffsize - 1);
    if (strcmp(_oldtext, _text) == 0) { return; }
    _textwidth = strlen(_text) * _charWidth;
    _x = _fb->ready() ? 0 : _config.left;
    _doscroll = _checkIsScrollNeeded();
    if (dsp.getScrollId() == this) { dsp.setScrollId(NULL); }
    _scrolldelay = millis();
    if (_active) {
        _setTextParams();
        if (_doscroll) {
            if (_fb->ready()) {
    #ifdef PSFBUFFER
                _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
                _fb->setCursor(0, 0);
                snprintf(_window, _width / _charWidth + 1, "%s", _text); // TODO
                _fb->print(_window);
                _fb->display();
    #endif
            } else {
                dsp.fillRect(_config.left, _config.top, _width, _textheight, _bgcolor);
                dsp.setCursor(_config.left, _config.top);
                snprintf(_window, _width / _charWidth + 1, "%s", _text); // TODO
                dsp.setClipping({_config.left, _config.top, _width, _textheight});
                dsp.print(_window);
                dsp.clearClipping();
            }
        } else {
            if (_fb->ready()) {
    #ifdef PSFBUFFER
                _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
                _fb->setCursor(_realLeft(true), 0);
                _fb->print(_text);
                _fb->display();
    #endif
            } else {
                dsp.fillRect(_config.left, _config.top, _width, _textheight, _bgcolor);
                dsp.setCursor(_realLeft(), _config.top);
                dsp.print(_text);
            }
        }
        strlcpy(_oldtext, _text, _buffsize);
    }
}

void ScrollWidget::setText(const char* txt, const char* format) {
    char buf[_buffsize];
    snprintf(buf, _buffsize, format, txt);
    setText(buf);
}

void ScrollWidget::loop() {
    if (_locked) { return; }
    if (!_doscroll || _config.textsize == 0 || (dsp.getScrollId() != NULL && dsp.getScrollId() != this)) { return; }
    uint16_t fbl = _fb->ready() ? 0 : _config.left;
    if (_checkDelay(_x == fbl ? _startscrolldelay : _scrolltime, _scrolldelay)) {
        _calcX();
        if (_active) { _draw(); }
    }
}

void ScrollWidget::_clear() {
    if (_fb->ready()) {
    #ifdef PSFBUFFER
        _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
        _fb->display();
    #endif
    } else {
        dsp.fillRect(_config.left, _config.top, _width, _textheight, _bgcolor);
    }
}

void ScrollWidget::_draw() {
    if (!_active || _locked) { return; }
    _setTextParams();
    if (_doscroll) {
        uint16_t    fbl = _fb->ready() ? 0 : _config.left;
        uint16_t    _newx = fbl - _x;
        const char* _cursor = _text + _newx / _charWidth;
        uint16_t    hiddenChars = _cursor - _text;
        uint8_t     addChars = _fb->ready() ? 2 : 1;
        if (hiddenChars < strlen(_text)) {
    // TODO
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-truncation="
            snprintf(_window, _width / _charWidth + addChars, "%s%s%s", _cursor, _sep, _text);
    #pragma GCC diagnostic pop
        } else {
            const char* _scursor = _sep + (_cursor - (_text + strlen(_text)));
            snprintf(_window, _width / _charWidth + addChars, "%s%s", _scursor, _text);
        }
        if (_fb->ready()) {
    #ifdef PSFBUFFER
            _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
            _fb->setCursor(_x + hiddenChars * _charWidth, 0);
            _fb->print(_window);
            _fb->display();
    #endif
        } else {
            dsp.setCursor(_x + hiddenChars * _charWidth, _config.top);
            dsp.setClipping({_config.left, _config.top, _width, _textheight});
            dsp.print(_window);
    #ifndef DSP_LCD
            dsp.print(" ");
    #endif
            dsp.clearClipping();
        }
    } else {
        if (_fb->ready()) {
    #ifdef PSFBUFFER
            _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
            _fb->setCursor(_realLeft(true), 0);
            _fb->print(_text);
            _fb->display();
    #endif
        } else {
            dsp.fillRect(_config.left, _config.top, _width, _textheight, _bgcolor);
            dsp.setCursor(_realLeft(), _config.top);
            dsp.setClipping({_realLeft(), _config.top, _width, _textheight});
            dsp.print(_text);
            dsp.clearClipping();
        }
    }
}

void ScrollWidget::_calcX() {
    if (!_doscroll || _config.textsize == 0) { return; }
    _x -= _scrolldelta;
    uint16_t fbl = _fb->ready() ? 0 : _config.left;
    if (-_x > _textwidth + _sepwidth - fbl) {
        _x = fbl;
        dsp.setScrollId(NULL);
    } else {
        dsp.setScrollId(this);
    }
}

bool ScrollWidget::_checkDelay(int m, uint32_t& tstamp) {
    if (millis() - tstamp > m) {
        tstamp = millis();
        return true;
    } else {
        return false;
    }
}

void ScrollWidget::_reset() {
    dsp.setScrollId(NULL);
    _x = _fb->ready() ? 0 : _config.left;
    _scrolldelay = millis();
    _doscroll = _checkIsScrollNeeded();
    #ifdef PSFBUFFER
    _fb->freeBuffer();
    uint16_t _rl = (_config.align == WA_CENTER) ? (dsp.width() - _width) / 2 : _config.left;
    _fb->begin(&dsp, _rl, _config.top, _width, _textheight, _bgcolor);
    #endif
}

/**************************************************************************************************************
                                              SLIDER WIDGET (hangerő csík)
 **************************************************************************************************************/
void SliderWidget::init(FillConfig conf, uint16_t fgcolor, uint16_t bgcolor, uint32_t maxval, uint16_t oucolor) {
    Widget::init(conf.widget, fgcolor, bgcolor);
    _width = conf.width;
    _height = conf.height;
    _outlined = conf.outlined;
    _oucolor = oucolor, _max = maxval;
    _oldvalwidth = _value = 0;
}

void SliderWidget::setValue(uint32_t val) {
    _value = val;
    if (_active && !_locked) { _drawslider(); }
}

void SliderWidget::_drawslider() {
    uint16_t valwidth = map(_value, 0, _max, 0, _width - _outlined * 2);
    if (_oldvalwidth == valwidth) { return; }
    dsp.fillRect(_config.left + _outlined + min(valwidth, _oldvalwidth), _config.top + _outlined, abs(_oldvalwidth - valwidth), _height - _outlined * 2, _oldvalwidth > valwidth ? _bgcolor : _fgcolor);
    _oldvalwidth = valwidth;
}

void SliderWidget::_draw() {
    if (_locked) { return; }
    _clear();
    if (!_active) { return; }
    if (_outlined) { dsp.drawRect(_config.left, _config.top, _width, _height, _oucolor); }
    uint16_t valwidth = map(_value, 0, _max, 0, _width - _outlined * 2);
    dsp.fillRect(_config.left + _outlined, _config.top + _outlined, valwidth, _height - _outlined * 2, _fgcolor);
}

void SliderWidget::_clear() {
    dsp.fillRect(_config.left, _config.top, _width, _height, _bgcolor);
}
void SliderWidget::_reset() {
    _oldvalwidth = 0;
}

/************************
      VU WIDGET
 ************************/
VuWidget::~VuWidget() {
    #if !defined(DSP_OLED)
    if (_canvas) { free(_canvas); }
    #endif
}

void VuWidget::init(WidgetConfig wconf, VUBandsConfig bands, uint16_t vumaxcolor, uint16_t vumidcolor, uint16_t vumincolor, uint16_t bgcolor) {
    Widget::init(wconf, bgcolor, bgcolor);
    _vumaxcolor = vumaxcolor;
    _vumidcolor = vumidcolor; // Módosítás új sor.
    _vumincolor = vumincolor;
    _bands = bands;
    #ifndef DSP_OLED
    _vumaxcolor = vumaxcolor;
    _vumidcolor = vumidcolor; // Módosítás új sor.
    _vumincolor = vumincolor;
        #ifndef BOOMBOX_STYLE
    // két VU egymás alatt
    _canvas = new Canvas(_bands.width, _bands.height * 2 + _bands.space);
        #else
    // két VU egymás mellett
    _canvas = new Canvas(_bands.width * 2 + _bands.space, _bands.height);
        #endif
    #endif
}

/**************************** VU widget DRAW ********************************/
void VuWidget::_draw() {
    if (!_active || _locked) { return; }
    if (!config.store.vumeter) { return; }
    uint16_t        bandColor;
    static uint16_t measLpx = 0;
    static uint16_t measRpx = 0;
    const uint8_t   vu_decay_step = _bands.fadespeed;
    const uint16_t  dimension = _bands.width; // vízszintes
    static uint32_t last_draw_time;
    uint8_t         refresh_time = 33; // A VU rajzolás frissítési ideje (millis)
    #ifdef VU_PEAK
    static uint16_t peakL = 0, peakR = 0;           // Csúcsértékek
    static uint32_t peakL_time = 0, peakR_time = 0; // Csúcs időbélyeg
    const uint8_t   peak_decay_step = 1;            // A csúcs bomlása pixelben
    const uint16_t  peak_hold_ms = 200;             // Csúcs tartási idő
    #endif
    uint32_t now = millis();
    if (last_draw_time + refresh_time > now) {
        return;
    } else {
        last_draw_time = now;
    }
    uint16_t vulevel = player.getVUlevel();
    uint8_t  vuLeft = (vulevel >> 8) & 0xFF;
    uint8_t  vuRight = vulevel & 0xFF;
    // A maximális VU érték begyűjtése. Fájlonként nullázódik, és 2 másodpercenként -10 -el csökken.
    uint16_t maxVU = max(vuLeft, vuRight);
    if (maxVU > config.vuRefLevel) { config.vuRefLevel = maxVU; }
    // Minimális érték a kezdeti számításokhoz.
    if (config.vuRefLevel < 50) { config.vuRefLevel = 50; }
    // VU értéket pixel pozícióra skalázás
    uint16_t vuLpx = map(vuLeft, 0, config.vuRefLevel, 0, _bands.width);
    uint16_t vuRpx = map(vuRight, 0, config.vuRefLevel, 0, _bands.width);
    bool     played = player.isRunning();
    if (played) {
        // BAL csatorna
        if (vuLpx > measLpx) {
            measLpx = vuLpx; // gyors felfutás L
        } else {
            measLpx = (measLpx > vu_decay_step) ? measLpx - vu_decay_step : 0; // lassított lebomlás L
        }
        // JOBB csatorna
        if (vuRpx > measRpx) {
            measRpx = vuRpx; // gyors felfutás R
        } else {
            measRpx = (measRpx > vu_decay_step) ? measRpx - vu_decay_step : 0; // lassított lebomlás R
        }
        // --- Csúcs logika ---
    #ifdef VU_PEAK
        // BAL csatorna
        if (measLpx > peakL) {
            peakL = measLpx; // L - csúcs meghatározás
            peakL_time = now;
        } else if (now - peakL_time > peak_hold_ms && peakL > 0) {
            peakL = (peakL > peak_decay_step) ? peakL - peak_decay_step : 0; // L - csúcs lebomlás
        }
        // JOBB csatorna
        if (measRpx > peakR) {
            peakR = measRpx; // R - csúcs meghatározás
            peakR_time = now;
        } else if (now - peakR_time > peak_hold_ms && peakR > 0) {
            peakR = (peakR > peak_decay_step) ? peakR - peak_decay_step : 0; // R - csúcs lebomlás
        }
    #endif // VU_PEAK
    /*************************************  A VU sávok rajzolása  ***************************************/
    #ifdef BOOMBOX_STYLE // ===================== BOOMBOX_STYLE =====================
        #ifndef DSP_OLED
        // Két VU egymás mellett – TELJES TÖRLÉS kell a ghosting ellen!
        _canvas->fillRect(0, 0, _bands.width * 2 + _bands.space, _bands.height, _bgcolor);
        // --- LED színek határai ---
        int green_end = (_bands.width * 65) / 100;
        int yellow_end = (_bands.width * 85) / 100;
        // --- VU méretezés ---
        uint8_t   ledWidth = (dimension / _bands.perheight) - _bands.vspace;
        uint16_t  step = dimension / _bands.perheight;
        uint16_t  litCountL = measLpx / step;
        uint16_t  litCountR = measRpx / step;
        const int MID_GAP = 6;            // teljes hézag a két kijelző között
        const int MID_HALF = MID_GAP / 2; // 3 pixel balra + 3 pixel jobbra
        // === BAL csatorna (balra indul a középtől) ===
        for (int led = 0; led < litCountL; led++) {
            int x = _bands.width - MID_HALF - (led * step) - ledWidth;
            if (x < 0) { break; }
            if (led * step < green_end) {
                bandColor = _vumincolor;
            } else if (led * step < yellow_end) {
                bandColor = _vumidcolor;
            } else {
                bandColor = _vumaxcolor;
            }
            _canvas->fillRect(x, 0, ledWidth, _bands.height, bandColor);
        }
        // === JOBB csatorna (jobbra indul a középtől) ===
        for (int led = 0; led < litCountR; led++) {
            int x = _bands.width + MID_HALF + (led * step);
            if (x > (int)_bands.width * 2) { break; }
            if (led * step < green_end) {
                bandColor = _vumincolor;
            } else if (led * step < yellow_end) {
                bandColor = _vumidcolor;
            } else {
                bandColor = _vumaxcolor;
            }
            _canvas->fillRect(x, 0, ledWidth, _bands.height, bandColor);
        }
            #ifdef VU_PEAK
        const uint16_t peak_color = 0xFFFF;
        const uint16_t peak_bright = 0xF7FF;
        const int      peak_width = 1;
        // --- BAL peak ---
        int pxL = _bands.width - MID_HALF - peakL;
        // clamp hogy sose fusson ki balra
        if (pxL < 0) { pxL = 0; }
        _canvas->fillRect((pxL - 1 < 0 ? 0 : pxL - 1), 0, peak_width + 2, _bands.height, peak_bright);
        _canvas->fillRect(pxL, 0, peak_width, _bands.height, peak_color);
        // --- JOBB peak ---
        int pxR = _bands.width + MID_HALF + peakR;
        // clamp védelem, de jobbra
        int maxX = (_bands.width * 2 + MID_GAP) - peak_width - 1;
        if (pxR > maxX) { pxR = maxX; }
        _canvas->fillRect(pxR - 1, 0, peak_width + 2, _bands.height, peak_bright);
        _canvas->fillRect(pxR, 0, peak_width, _bands.height, peak_color);
            #endif // VU_PEAK
                   // --- KIJELZŐRE KÜLDÉS ---
        dsp.startWrite();
        dsp.setAddrWindow(_config.left + 4, _config.top + 10, _bands.width * 2 + _bands.space, _bands.height);
        dsp.writePixels((uint16_t*)_canvas->getBuffer(), (_bands.width * 2 + _bands.space) * _bands.height);
        dsp.endWrite();
        #endif // DSP_OLED
    #else      // ===================== BOOMBOX_STYLE END ======================
        // Háttér törlése – két VU egymás alatt
        #ifndef DSP_OLED
        _canvas->fillRect(0, 0, _bands.width, _bands.height * 2 + _bands.space, _bgcolor);
        #else
        dsp.fillRect(_config.left, _config.top, _bands.width, _bands.height * 2 + _bands.space, BLACK);
        #endif
        // --- LED-sáv rajzolása színátmenettel ---
        int green_end = (_bands.width * 65) / 100;
        int yellow_end = (_bands.width * 85) / 100;
        // --- VU méretezés ---
        uint8_t  ledWidth = (dimension / _bands.perheight) - _bands.vspace;
        uint16_t step = dimension / _bands.perheight; // step pixelben
        uint16_t litCountL = measLpx / step;
        uint16_t litCountR = measRpx / step;
        #ifdef DSP_OLED
        if (played) {
            if (litCountL == 0) { litCountL = 1; }
            if (litCountR == 0) { litCountR = 1; }
        }
        #endif
        // Bal sáv
        for (int led = 0; led < litCountL; led++) {
            int x = led * step;
            if (x < green_end) {
                bandColor = _vumincolor;
            } else if (x < yellow_end) {
                bandColor = _vumidcolor;
            } else {
                bandColor = _vumaxcolor;
            }
        #ifndef DSP_OLED
            _canvas->fillRect(x, 0, ledWidth, _bands.height, bandColor);
        #else
            dsp.fillRect(x + _config.left, _config.top, ledWidth, _bands.height, bandColor);
        #endif
        }
        // Jobb sáv
        for (int led = 0; led < litCountR; led++) {
            int x = led * step;
            if (x < green_end) {
                bandColor = _vumincolor;
            } else if (x < yellow_end) {
                bandColor = _vumidcolor;
            } else {
                bandColor = _vumaxcolor;
            }
        #ifndef DSP_OLED
            _canvas->fillRect(x, _bands.height + _bands.space, ledWidth, _bands.height, bandColor);
        #else
            dsp.fillRect(_config.left + x, _config.top + _bands.height + _bands.space, ledWidth, _bands.height, bandColor);
        #endif
        }
        #ifdef VU_PEAK
            #ifndef DSP_OLED
        const uint16_t peak_color = 0xFFFF;
        const uint16_t peak_bright = 0xF7FF;
            #else
        const uint16_t peak_color = 0xF;
        const uint16_t peak_bright = BLACK;
            #endif
        const int peak_width = 1;
        // Peak bal
        if (peakL > _bands.width - 2) { peakL -= 2; }
        if (peakL > 1 && peakL <= (int)_bands.width) {
            #ifndef DSP_OLED
            _canvas->fillRect(peakL - 1, 0, peak_width + 2, _bands.height, peak_bright);
            _canvas->fillRect(peakL, 0, peak_width, _bands.height, peak_color);
            #else
            dsp.fillRect(peakL - 1 + _config.left, _config.top, peak_width + 1, _bands.height, peak_bright);
            dsp.fillRect(peakL + _config.left, _config.top, peak_width, _bands.height, peak_color);
            #endif
        }
        // Peak jobb
        if (peakR > _bands.width - 2) { peakR -= 2; }
        if (peakR > 1 && peakR <= (int)_bands.width) {
            #ifndef DSP_OLED
            _canvas->fillRect(peakR - 1, _bands.height + _bands.space, peak_width + 2, _bands.height, peak_bright);
            _canvas->fillRect(peakR, _bands.height + _bands.space, peak_width, _bands.height, peak_color);
            #else
            dsp.fillRect(peakR - 1 + _config.left, _config.top + _bands.height + _bands.space, peak_width + 1, _bands.height, peak_bright);
            dsp.fillRect(peakR + _config.left, _config.top + _bands.height + _bands.space, peak_width, _bands.height, peak_color);
            #endif
        }
        #endif // VU_PEAK
        #ifndef DSP_OLED
        // Kirajzolás
        int drawWidth = _bands.width;
        int drawHeight = _bands.height * 2 + _bands.space;
        dsp.startWrite();
        dsp.setAddrWindow(_config.left, _config.top, drawWidth, drawHeight);
        dsp.writePixels((uint16_t*)_canvas->getBuffer(), drawWidth * drawHeight);
        dsp.endWrite();
        #endif
    #endif // BOOMBOX_STYLE
        /********************************** --- L/R címkék rajzolása --- **********************************/
        if (played && !_labelsDrawn) {
    #ifdef BOOMBOX_STYLE
        #ifndef DSP_OLED
            // Serial.println("L/R rajzolás");
            int label_width = _bands.height + 15;
            int label_height = _bands.height + 4;
            // teljes szélesség (két címke + 6px hézag)
            int total_width = 2 * label_width + 6;
            // bal széle a középre igazított blokk
            int center_left = (dsp.width() - total_width) / 2;
            // bal és jobb címke pozíció
            int label_left_L = center_left;
            int label_left_R = center_left + label_width + 6;
            // Bal (L) téglalap
            dsp.fillRect(label_left_L, _config.top - 4, label_width, label_height, 0x7BEF);
            dsp.setTextSize(1);
            dsp.setFont();
            dsp.setTextColor(0xFFFF);
            int text_x_L = label_left_L + (label_width - 6) / 2;
            #if DSP_MODEL == DSP_ILI9341
            int text_y = ((_config.top - 2) + (label_height - 10) / 2) - 1;
            #else
            int text_y = (_config.top - 2) + (label_height - 10) / 2;
            #endif
            dsp.setCursor(text_x_L, text_y);
            dsp.print("L");
            // Jobb (R) téglalap
            dsp.fillRect(label_left_R, _config.top - 4, label_width, label_height, 0x7BEF);
            int text_x_R = label_left_R + (label_width - 6) / 2;
            dsp.setCursor(text_x_R, text_y);
            dsp.print("R");
            _labelsDrawn = true;
        #endif
    #else //  NEM BOOMBOX_STYLE
        #ifndef DSP_OLED
            int label_width = _bands.height + 15;
            int label_height = _bands.height + 4;
            int label_offset = label_width + 4;
            int label_left = _config.left - label_offset;
            if (label_left >= 0) {
                dsp.fillRect(label_left, _config.top - 4, label_width, label_height, 0x7BEF);
                dsp.fillRect(label_left, _config.top + _bands.height + _bands.space, label_width, label_height - 1, 0x7BEF);
                dsp.setTextColor(0xFFFF);
                dsp.setTextSize(1);
                dsp.setFont();
                int text_x = label_left + (label_width - 6) / 2;
                int text_y_L = (_config.top - 2) + (label_height - 10) / 2;
                int text_y_R = _config.top + _bands.height + _bands.space + (label_height - 8) / 2;
                dsp.setCursor(text_x, text_y_L);
                dsp.print("L");
                dsp.setCursor(text_x, text_y_R);
                dsp.print("R");
                _labelsDrawn = true;
            }
        #else
            dsp.setTextColor(0xF);
            dsp.setTextSize(0);
            int text_x = _config.left - 6;
            int text_y_L = _config.top + 3;
            int text_y_R = _config.top + _bands.height + _bands.space + 2 + 4;
            dsp.setFont(&TinyFont5);
            dsp.setCursor(text_x, text_y_L);
            dsp.print("L");
            dsp.setCursor(text_x, text_y_R);
            dsp.print("R");
            dsp.setFont();
            _labelsDrawn = true;
        #endif
    #endif // BOOMBOX_STYLE
        }
    }
}

void VuWidget::loop() {
    if (_active && !_locked) { _draw(); }
}

void VuWidget::_clear() {
    #ifndef DSP_OLED
    dsp.fillRect(0, _config.top - 4, 479, 24, _bgcolor);
    _labelsDrawn = false; // L és R meg keljen rajzolni. Módosítás.
    // Serial.println("widget.cpp -> VuWidget::_clear()");
    #else
    dsp.fillRect(_config.left, _config.top - 4, _bands.width, _bands.height * 2 + _bands.vspace + 5, _bgcolor);
    _labelsDrawn = false; // L és R meg keljen rajzolni. Módosítás.
    #endif
}

/* A _labelsDrawn változó true értéke esetén rajzolja meg a VU méter előtti L R feliratot.
A  BitrateWidget::_clear() -ben kap false értéket.*/
bool VuWidget::_labelsDrawn = false; // Módosítás

void VuWidget::setLabelsDrawn(bool value) { // Saját
    _labelsDrawn = value;
}

bool VuWidget::isLabelsDrawn() { // Saját
    return _labelsDrawn;
}

  /************************
      NUM & CLOCK
 ************************/
  #if !defined(DSP_LCD)
    #if TIME_SIZE < 19  //19->NOKIA
const GFXfont *Clock_GFXfontPtr = nullptr;
      #define CLOCKFONT5x7
    #else
const GFXfont *Clock_GFXfontPtr = yoClockFontMain((uint8_t)CLOCKFONT);
const GFXfont *Clock_GFXfontPtr_Sec = yoClockFontSec((uint8_t)CLOCKFONT);
static uint8_t _clockFontId = (uint8_t)CLOCKFONT;
void widgetsSetClockFont(uint8_t fontId){
  fontId = yoClockFontSanitize(fontId);
  if(_clockFontId == fontId && Clock_GFXfontPtr && Clock_GFXfontPtr_Sec) return;
  _clockFontId = fontId;
  Clock_GFXfontPtr = yoClockFontMain(fontId);
  Clock_GFXfontPtr_Sec = yoClockFontSec(fontId);
}
    #endif
  #endif  //!defined(DSP_LCD)
  #if !defined(CLOCKFONT5x7) && !defined(DSP_LCD)
inline GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
  return gfxFont->glyph + c;
}
uint8_t _charWidth(unsigned char c) {
  const GFXfont *f = Clock_GFXfontPtr;
  if(!f) return 0;
  uint8_t first = pgm_read_byte(&f->first);
  uint8_t last  = pgm_read_byte(&f->last);
  if (c < first || c > last) return 0;
  GFXglyph *glyph = pgm_read_glyph_ptr(f, c - first);
  return pgm_read_byte(&glyph->xAdvance);
}
uint16_t _textHeight() {
  const GFXfont *f = Clock_GFXfontPtr;
  if(!f) return 0;
  uint8_t first = pgm_read_byte(&f->first);
  uint8_t last  = pgm_read_byte(&f->last);
  uint8_t ch = '8';
  uint8_t idx = (ch < first || ch > last) ? 0 : (ch - first);
  GFXglyph *glyph = pgm_read_glyph_ptr(f, idx);
  return pgm_read_byte(&glyph->height);
}
  #else
uint8_t _charWidth(unsigned char c) {
    #ifndef DSP_LCD
  return CHARWIDTH * TIME_SIZE;
    #else
  return 1;
    #endif
}
uint16_t _textHeight() {
  return CHARHEIGHT * TIME_SIZE;
}
  #endif
uint16_t _textWidth(const char *txt) {
  uint16_t w = 0, l = strlen(txt);
  for (uint16_t c = 0; c < l; c++) { w += _charWidth(txt[c]); }
  return w;
}

uint8_t _charWidth(unsigned char c, const GFXfont *f) {
  if (!f) return 0;
  uint8_t first = pgm_read_byte(&f->first);
  uint8_t last  = pgm_read_byte(&f->last);
  if (c < first || c > last) return 0;
  GFXglyph *glyph = pgm_read_glyph_ptr(f, c - first);
  return pgm_read_byte(&glyph->xAdvance);
}

uint16_t _textWidth(const char *txt, const GFXfont *f) {
  uint16_t w = 0, l = strlen(txt);
  for (uint16_t c = 0; c < l; c++) { w += _charWidth((unsigned char)txt[c], f); }
  return w;
}
/************************************************************************************************************
                                                      NUM WIDGET (hangerő stb.)
 ************************************************************************************************************/
void NumWidget::init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor) {
    Widget::init(wconf, fgcolor, bgcolor);
    _buffsize = buffsize;
    _text = (char*)malloc(sizeof(char) * _buffsize);
    memset(_text, 0, _buffsize);
    _oldtext = (char*)malloc(sizeof(char) * _buffsize);
    memset(_oldtext, 0, _buffsize);
    _textwidth = _oldtextwidth = _oldleft = 0;
    _uppercase = uppercase;
    _textheight = TIME_SIZE /*wconf.textsize*/;
}
/*
void NumWidget::setText(const char *txt) {
  strlcpy(_text, txt, _buffsize);
  _getBounds();
  if (strcmp(_oldtext, _text) == 0) {
    return;
  }
  uint16_t realth = _textheight;
  #if defined(DSP_OLED) && DSP_MODEL != DSP_SSD1322
  if (Clock_GFXfontPtr == nullptr) {
    realth = _textheight * 8;  //CHARHEIGHT
  }
  #endif
  if (_active)
  #ifndef CLOCKFONT5x7
    dsp.fillRect(_oldleft == 0 ? _realLeft() : min(_oldleft, _realLeft()), _config.top - _textheight + 1, max(_oldtextwidth, _textwidth), realth, _bgcolor);
  #else
    dsp.fillRect(_oldleft == 0 ? _realLeft() : min(_oldleft, _realLeft()), _config.top, max(_oldtextwidth, _textwidth), realth, _bgcolor);
  #endif

  _oldtextwidth = _textwidth;
  _oldleft = _realLeft();
  if (_active) {
    _draw();
  }
}
*/

void NumWidget::setText(const char* txt) {
    strlcpy(_text, txt, _buffsize);
    _getBounds();
    if (strcmp(_oldtext, _text) == 0) { return; }
    uint16_t clearHeight;
    #ifndef DSP_LCD
    if (Clock_GFXfontPtr != nullptr) {
        clearHeight = _textHeight();
    } else {
        clearHeight = CHARHEIGHT * TIME_SIZE;
    }
    #else
    clearHeight = _textheight;
    #endif
    int16_t clearTop = _config.top - clearHeight + 1;
    #ifndef DSP_LCD
    if (Clock_GFXfontPtr != nullptr) {
        clearTop -= 2;    // felső túlnyúlás
        clearHeight += 2; // alsó biztonság
    }
    #endif
    if (_active) {
    #ifndef CLOCKFONT5x7
        dsp.fillRect(_oldleft == 0 ? _realLeft() : min(_oldleft, _realLeft()), clearTop, max(_oldtextwidth, _textwidth), clearHeight, _bgcolor);
    #else
        dsp.fillRect(_oldleft == 0 ? _realLeft() : min(_oldleft, _realLeft()), _config.top, max(_oldtextwidth, _textwidth), clearHeight, _bgcolor);
    #endif
    }
    _oldtextwidth = _textwidth;
    _oldleft = _realLeft();
    if (_active) { _draw(); }
}

void NumWidget::setText(int val, const char* format) {
    char buf[_buffsize];
    snprintf(buf, _buffsize, format, val);
    setText(buf);
}

void NumWidget::_getBounds() {
    _textwidth = _textWidth(_text);
}

void NumWidget::_draw() {
    #ifndef DSP_LCD
    if (!_active || TIME_SIZE < 2) { return; }
    dsp.setTextSize(Clock_GFXfontPtr == nullptr ? TIME_SIZE : 1);
    dsp.setFont(Clock_GFXfontPtr);
    dsp.setTextColor(_fgcolor, _bgcolor);
    #endif
    if (!_active) { return; }
    dsp.setCursor(_realLeft(), _config.top);
    dsp.print(_text);
    strlcpy(_oldtext, _text, _buffsize);
    dsp.setFont();
}

/**************************
      PROGRESS WIDGET
 **************************/
void ProgressWidget::_progress() {
    char buf[_width + 1];
    snprintf(buf, _width, "%*s%.*s%*s", _pg <= _barwidth ? 0 : _pg - _barwidth, "", _pg <= _barwidth ? _pg : 5, ".....", _width - _pg, "");
    _pg++;
    if (_pg >= _width + _barwidth) { _pg = 0; }
    setText(buf);
}

bool ProgressWidget::_checkDelay(int m, uint32_t& tstamp) {
    if (millis() - tstamp > m) {
        tstamp = millis();
        return true;
    } else {
        return false;
    }
}

void ProgressWidget::loop() {
    if (_checkDelay(_speed, _scrolldelay)) { _progress(); }
}

/*************************************************************************************************************************
                                                               CLOCK WIDGET
 *************************************************************************************************************************/
void ClockWidget::init(WidgetConfig wconf, uint16_t fgcolor, uint16_t bgcolor) {
    Widget::init(wconf, fgcolor, bgcolor);
#if defined(FLIP_CLOCK) && defined(FLIP_CLOCK_BASELINE)
    _config.top   = FLIP_CLOCK_BASELINE;
    _config.left  = FLIP_CLOCK_LEFT;
    _config.align = FLIP_CLOCK_ALIGN;
#endif
    _timeheight = _textHeight();
    _fullclock = TIME_SIZE > 35 || DSP_MODEL == DSP_ILI9225;
    if (_fullclock) {
        _superfont = TIME_SIZE / 17; // magick
    } else if (TIME_SIZE == 19 || TIME_SIZE == 2) {
        _superfont = 1;
    } else {
        _superfont = 0;
    }
    _space = (5 * _superfont) / 2; // magick
    #ifndef HIDE_DATE
    if (_fullclock) {
        _dateheight = _superfont < 4 ? 1 : 2;
        // _clockheight = _timeheight + _space + CHARHEIGHT * _dateheight; //Original
        _clockheight = _timeheight;
    } else {
        _clockheight = _timeheight;
    }
    #else
    _clockheight = _timeheight;
    #endif
    _getTimeBounds();
    #ifdef PSFBUFFER
    _fb = new psFrameBuffer(dsp.width(), dsp.height());
    #ifndef FLIP_CLOCK
    // Dla FLIP_CLOCK: _begin() woływane jest PONIŻEJ, po obliczeniu _flipPanelH.
    // Gdyby _begin() zostało wywołane tutaj, _flipPanelH = 0 → _fb miałby wysokość 1px
    // i przy _fb->display() kasowałby górny rząd karteczek (narożniki zaokrąglone).
    _begin();
    #endif
    #endif

    #ifdef FLIP_CLOCK
    // Wyznacz maksymalną szerokość cyfry spośród '0'..'9' – wszystkie panele
    // muszą mieć tę samą szerokość niezależnie od aktualnie wyświetlanej cyfry.
    _flipDigitW = 0;
    for (char c = '0'; c <= '9'; c++) {
        uint8_t w = _charWidth((unsigned char)c, Clock_GFXfontPtr);
        if (w > _flipDigitW) _flipDigitW = w;
    }
    // Wysokość karteczki = wysokość tekstu + margines góra i dół (VPAD)
    _flipPanelH  = _timeheight + 2 * FLIP_PANEL_VPAD;
    _clockheight = _flipPanelH;
    // Przelicz _clockleft / _clockwidth mając już znane _flipDigitW
    _getTimeBounds();
    strncpy(_prevTimebuffer, _timebuffer, sizeof(_prevTimebuffer));
    // Zainicjalizuj obiekty FlipDigit dla HH i MM
    _initFlipDigits();
    #ifdef PSFBUFFER
    // Teraz _flipPanelH jest poprawne → _fb uzyska właściwe wymiary.
    _begin();
    #endif
    #endif
}

void ClockWidget::_begin() {
    #ifdef PSFBUFFER
    #ifdef FLIP_CLOCK
    _secGap = 0;  // wymusz ponowne obliczenie gap'u na pozycji PLAYER
    _beginFlipSecBuf();
    #else
    _fb->begin(&dsp, _clockleft, _config.top - _timeheight, _clockwidth, _clockheight + 1, config.theme.background);
    #endif
    #endif
}

#ifdef FLIP_CLOCK
/**
 * @brief Inicjalizuje _fb jako mały bufor PSRAM dla wyświetlania sekund.
 *        Zamiast alokować ~_clockwidth × _flipPanelH (nieużywany dla FLIP_CLOCK),
 *        alokujemy tylko obszar sekund: secW × secH.
 *        Dzięki temu _drawFlipSeconds() robi jeden szybki burst SPI (jak scroll widget),
 *        zamiast pixel-po-pikselu przez dsp.print() (~7 ms/s → ~0.05 ms/s).
 */
void ClockWidget::_beginFlipSecBuf() {
    #ifndef FLIP_PANEL_VPAD
    #define FLIP_PANEL_VPAD 5
    #endif
    #ifndef FLIP_DIGIT_GAP
    #define FLIP_DIGIT_GAP 4
    #endif
    if (!(_fullclock || _superfont > 0)) return;  // brak sekund → nie alokuj
    if (!Clock_GFXfontPtr_Sec) return;
    // Rozmiar glyfu "8" z fontu sekund
    uint8_t first = pgm_read_byte(&Clock_GFXfontPtr_Sec->first);
    GFXglyph *g = pgm_read_glyph_ptr(Clock_GFXfontPtr_Sec, '8' - first);
    int16_t secH = pgm_read_byte(&g->height);
    uint16_t secW = _textWidth("88", Clock_GFXfontPtr_Sec);
    // Pozycja X sekund: stały gap od prawej krawędzi karteczki MM.
    // Gap obliczany raz (gdy _secGap==0, czyli wywołanie z _begin() na pozycji PLAYER):
    //   wyśrodkowanie sekund między MM a prawym brzegiem ekranu.
    // Na screensaverze (_reset() → _beginFlipSecBuf()) _secGap jest już ustawiony → stała odległość.
    int16_t mmRight = (int16_t)_clockleft + (int16_t)_timewidth;
    if (_secGap == 0) {
        int16_t zone = (int16_t)dsp.width() - mmRight;
        _secGap = (zone - (int16_t)secW) / 2;
        if (_secGap < (int16_t)FLIP_DIGIT_GAP) _secGap = FLIP_DIGIT_GAP;  // minimum
    }
    int16_t leftSec = mmRight + _secGap;
    // Pozycja Y sekund: wyśrodkowana w panelu flip
    int16_t panelTop = (int16_t)_config.top - (int16_t)_timeheight - (int16_t)FLIP_PANEL_VPAD;
    int16_t topSec   = panelTop + ((int16_t)_flipPanelH - secH) / 2 + secH; // baseline
    // Zawsze używamy begin() – ustawia _dspl, _bgcolor i alokuje bufor.
    // Przed ponownym wywołaniem (np. z _reset()) najpierw zwalniamy istniejący bufor.
    if (_fb->ready()) { _fb->freeBuffer(); }
    _fb->begin(&dsp, leftSec, topSec - secH, secW + 2, secH + 2, config.theme.background);
    _fb->setLabel("SEKUNDY");
}
#endif

bool ClockWidget::_getTime() {
    #if defined AM_PM_STYLE
    strftime(_timebuffer, sizeof(_timebuffer), "%I:%M", &network.timeinfo);
    if (_timebuffer[0] == '0') {
        _timebuffer[0] = ' '; // Ha az eslő számjegy 0 kicseréli szóközre (azonos karakterszélesség szükséges)
    }
    #else
    strftime(_timebuffer, sizeof(_timebuffer), "%H:%M", &network.timeinfo);
    #endif
    bool ret = network.timeinfo.tm_sec == 0 || _forceflag != network.timeinfo.tm_year;
    _forceflag = network.timeinfo.tm_year;
    return ret;
}

uint16_t ClockWidget::_left() {
    #ifdef FLIP_CLOCK
    // FLIP_CLOCK: FlipDigit rysuje bezpośrednio na ekranie – zawsze zwracaj
    // rzeczywistą pozycję ekranową (nie lokalną 0 framebuffera).
    return _clockleft;
    #else
    if (_fb->ready()) {
        return 0;         // rysowanie do framebuffera – współrzędne lokalne
    } else {
        return _clockleft;
    }
    #endif
}
uint16_t ClockWidget::_top() {
    #ifdef FLIP_CLOCK
    // FLIP_CLOCK: _config.top = baseline czcionki (dół tekstu) na ekranie.
    // FlipDigit sam oblicza górną krawędź bufora: y - _h - FLIP_CARD_MARGIN.
    return _config.top;
    #else
    if (_fb->ready()) {
        return _timeheight;   // framebuffer – baseline w lokalnych współrzędnych
    } else {
        return _config.top;
    }
    #endif
}

void ClockWidget::_getTimeBounds() {
#ifdef FLIP_CLOCK
    if (Clock_GFXfontPtr && _flipDigitW > 0) {
        // Panel layout: [HH pw][gap][colon colonW][gap][MM pw]
        // Override FLIP_DIGIT_PAD / FLIP_DIGIT_GAP in myoptions.h to tune spacing.
        #ifndef FLIP_DIGIT_PAD
          #define FLIP_DIGIT_PAD 4  // horizontal padding inside each digit panel (px)
        #endif
        #ifndef FLIP_DIGIT_GAP
          #define FLIP_DIGIT_GAP 4  // gap between panels and colon (px)
        #endif
        const uint16_t padX  = FLIP_DIGIT_PAD;
        const uint16_t gap   = FLIP_DIGIT_GAP;
        uint16_t pw     = _flipDigitW * 2 + padX * 2; // panel fits 2 digits
        uint16_t colonW = _charWidth(':', Clock_GFXfontPtr);
        if (colonW == 0) colonW = pw / 4;
        _timewidth = pw * 2 + gap * 2 + colonW;
        // _dotsleft = colon position (after HH panel + gap)
        _dotsleft = pw + gap;

        uint8_t  fs        = _superfont > 0 ? _superfont : TIME_SIZE;
        uint16_t rightside = CHARWIDTH * fs * 2; // seconds area
        if (_fullclock) {
            rightside += _space * 2 + 1; // 2×space + vertical divider
            _clockwidth = _timewidth + rightside;
        } else {
            _clockwidth = (_superfont == 0) ? _timewidth : _timewidth + rightside;
        }
        switch (_config.align) {
            case WA_LEFT:  _clockleft = _config.left; break;
            case WA_RIGHT: _clockleft = dsp.width() - _clockwidth - _config.left; break;
            default:       _clockleft = (dsp.width() / 2 - _clockwidth / 2) + _config.left; break;
        }
        return;
    }
#endif
    _timewidth = _textWidth(_timebuffer);
    uint8_t  fs = _superfont > 0 ? _superfont : TIME_SIZE;
    uint16_t rightside = CHARWIDTH * fs * 2; // seconds
    if (_fullclock) {
        rightside += _space * 2 + 1; // 2space+vline
        _clockwidth = _timewidth + rightside;
    } else {
        if (_superfont == 0) {
            _clockwidth = _timewidth;
        } else {
            _clockwidth = _timewidth + rightside;
        }
    }
    switch (_config.align) {
        case WA_LEFT: _clockleft = _config.left; break;
        case WA_RIGHT: _clockleft = dsp.width() - _clockwidth - _config.left; break;
        default: _clockleft = (dsp.width() / 2 - _clockwidth / 2) + _config.left; break;
    }
    char buf[4];
    strftime(buf, 4, "%H", &network.timeinfo);
    _dotsleft = _textWidth(buf);
}

    #ifndef DSP_LCD

// Default vertical padding for flip panels (px above glyph top and below baseline).
// Override in myoptions.h: #define FLIP_PANEL_VPAD 5
#if defined(FLIP_CLOCK) && !defined(FLIP_PANEL_VPAD)
  #define FLIP_PANEL_VPAD 5
#endif

Adafruit_GFX& ClockWidget::getRealDsp() {
    #ifdef FLIP_CLOCK
    // FLIP_CLOCK: karteczki HH/MM rysowane przez FlipDigit bezpośrednio na dsp.
    // Sekundy, data i dwukropek też trafiają na dsp (nie do _fb).
    return dsp;
    #else
        #ifdef PSFBUFFER
    if (_fb && _fb->ready()) { return *_fb; }
        #endif
    return dsp;
    #endif
}

        #if DSP_MODEL == DSP_SSD1322

void ClockWidget::_drawShortDateSSD1322() {
    if (config.isScreensaver) { return; }
    // ⬅️ DÁTUM ELŐÁLLÍTÁSA KÖZÖS HELYEN
    _formatDate(); // _tmp -t tölti fel!
    WidgetConfig dc;
    memcpy_P(&dc, &dateConf, sizeof(WidgetConfig));
    // ===== FIX: 5x7 FONT MÉRETEK =====
    constexpr uint8_t  TS = 1;
    constexpr uint16_t H = CHARHEIGHT * TS;
    dsp.fillRect(0, dc.top, dsp.width(), H, config.theme.background);
    dsp.setFont(); // 5x7
    dsp.setTextSize(TS);
    dsp.setTextColor(config.theme.date, config.theme.background);
    // ===== SZÉLESSÉG SZÁMÍTÁS (5x7!) =====
    uint16_t w = strlen(_tmp) * CHARWIDTH * TS;
    uint16_t x;
    switch (dc.align) {
        case WA_CENTER: x = (dsp.width() - w) / 2; break;
        case WA_RIGHT: x = dsp.width() - w - dc.left; break;
        default: x = dc.left; break;
    }
    // ===== RAJZOLÁS =====
    dsp.setCursor(x, dc.top);
    dsp.print(_tmp);
}

        #endif

void ClockWidget::_printClock(bool force) {
    auto& gfx = getRealDsp();
    gfx.setTextSize(Clock_GFXfontPtr == nullptr ? TIME_SIZE : 1);
    gfx.setFont(Clock_GFXfontPtr);
    // bool clockInTitle = !config.isScreensaver && _config.top < _timeheight;  //DSP_SSD1306x32
    if (force) {
        #ifndef FLIP_CLOCK
        // FLIP_CLOCK: nie czyść ekranu tutaj – FlipDigit sam zarządza swoimi buforami.
        // _clearClock() w moveTo() czyści stare miejsce; kolejny fillRect nadpisywałby
        // świeżo narysowane karteczki i powodował trzykrotne miganie przy zmianie pozycji.
        _clearClock();
        #endif
        _getTimeBounds();
        #ifdef FLIP_CLOCK
        {
          // _printClock(force=true) wywoływane jest co minutę (gdy tm_sec==0).
          // Przekaż nowe wartości HH i MM do odpowiednich FlipDigit:
          //   - jeśli cyfry się zmieniły → flipTo() uruchamia animację
          //   - jeśli niezmienione (np. reset widgetu) → setValue() odświeża bez animacji
          char hhNew[3] = { _timebuffer[0], _timebuffer[1], '\0' };
          char mmNew[3] = { _timebuffer[3], _timebuffer[4], '\0' };
          if (strcmp(hhNew, _flipHH.current()) != 0) _flipHH.flipTo(hhNew);
          else                                        _flipHH.setValue(hhNew);
          if (strcmp(mmNew, _flipMM.current()) != 0) _flipMM.flipTo(mmNew);
          else                                        _flipMM.setValue(mmNew);
          strncpy(_prevTimebuffer, _timebuffer, sizeof(_prevTimebuffer));
          _drawFlipSeconds();  // odśwież sekundy przez psFrameBuffer (burst SPI)
        }
        #else
        #ifdef CLOCKFONT_MONO
        gfx.setTextColor(config.theme.clockbg, config.theme.background);
        gfx.setCursor(_left(), _top());
        gfx.print("88:88");
        #endif
        // if (clockInTitle) {
        // gfx.setTextColor(config.theme.meta, config.theme.metabg);
        //} else {
        gfx.setTextColor(config.theme.clock, config.theme.background);
        // }
        gfx.setCursor(_left(), _top());
        gfx.print(_timebuffer); // Az óra, perc kiírása.
        #endif // FLIP_CLOCK

        #if DSP_MODEL == DSP_SSD1322
            #ifndef HIDE_DATE
        _drawShortDateSSD1322();
            #endif
            #ifdef AM_PM_STYLE
        constexpr uint8_t FONT_H = 7; // TinyFont5 teljes magasság
        constexpr uint8_t ASCENT = 6; // baseline fölé

        dsp.fillRect(178,
                     23 - ASCENT, // 👈 FONT BASELINE KORREKCIÓ
                     12, FONT_H, config.theme.background);

        char buf[3];
        strftime(buf, sizeof(buf), "%p", &network.timeinfo);
        dsp.setTextSize(0);
        dsp.setFont(&TinyFont5);
        dsp.setTextColor(config.theme.clock, config.theme.background);
                #ifdef CLOCKFONT_MONO
        dsp.setCursor(179, 23);
                #else
        dsp.setCursor(188, 23);
                #endif
        dsp.print(buf); // AM vagy PM kiírása
            #endif
        #endif

        if (_fullclock) { // A másodperceket is kiírja nem csak az óra percet.
            bool fullClockOnScreensaver = (!config.isScreensaver || (_fb->ready() && FULL_SCR_CLOCK));
            _linesleft = _left() + _timewidth + _space;
            if (fullClockOnScreensaver) {
                // pionowa linia sekund
                // gfx.drawFastVLine(_linesleft, _top() - _timeheight, _timeheight, config.theme.div); // A másodperc vertikális vonala.
        #ifdef AM_PM_STYLE
                // A másodperc horizontális vonala.
                gfx.drawFastHLine(_linesleft, _top() - (_timeheight / 2), CHARWIDTH * _superfont * 2 + _space, config.theme.div);
                gfx.setTextSize(0);
                gfx.setFont(Clock_GFXfontPtr_Sec);
                char buf[3];
                strftime(buf, sizeof(buf), "%p", &network.timeinfo);
                gfx.setCursor(_linesleft + 8, _top());
                gfx.print(buf); // AM vagy PM kiírása

        #else
            #if DSP_MODEL == DSP_ILI9341
                constexpr int lineOffset = 17; // 320x240
            #else
                constexpr int lineOffset = 25; // 480x320
            #endif
                // Pozioma linia sekund.
                // gfx.drawFastHLine(_linesleft, _top() - (_timeheight / 2) + lineOffset, CHARWIDTH * _superfont * 2 + _space, config.theme.div);
        #endif
                if (!config.isScreensaver) {
                    _formatDate();
        #ifndef HIDE_DATE
                    memcpy_P(&_dateConf, &dateConf, sizeof(WidgetConfig));
                    // Sor törlése teljes szélességben
                    int lineHeight = _dateheight * 8;                                                 // kb. 8 pixel per TextSize
                    dsp.fillRect(0, _dateConf.top, dsp.width(), lineHeight, config.theme.background); // szürke 0x8410
                    strlcpy(_datebuf, utf8To(_tmp, false), sizeof(_datebuf));
                    uint16_t _datewidth = strlen(_datebuf) * CHARWIDTH * _dateheight;
                    dsp.setFont();
                    dsp.setTextSize(_dateheight);
                    uint16_t _dateleft = dsp.width() - _datewidth - _dateConf.left;
                    dsp.setCursor(_dateleft, _dateConf.top); // Zmień własną zmienną ustawień "_dateConf"
                    dsp.setTextColor(config.theme.date, config.theme.background);
                    dsp.print(_datebuf);
        #endif // HIDE_DATE
                }
            }
        }
    }
    // FLIP_CLOCK: sekundy rysuje wyłącznie _drawFlipSeconds() przez psFrameBuffer (burst SPI).
    // Tutaj nie rysujemy sekund, żeby nie było podwójnego rysowania w innej pozycji.
    gfx.setTextSize(Clock_GFXfontPtr == nullptr ? TIME_SIZE : 1);
    gfx.setFont(Clock_GFXfontPtr);
    #ifndef FLIP_CLOCK
        #ifndef DSP_OLED
            #ifdef CLOCKFONT_MONO
    gfx.setTextColor(dots ? config.theme.clock : config.theme.clockbg, config.theme.background);
            #else
    gfx.setTextColor(dots ? config.theme.clock : config.theme.background, config.theme.background);
            #endif

        #else
    // if (clockInTitle) {
    // gfx.setTextColor(dots ? config.theme.meta : config.theme.metabg, config.theme.metabg);
    // } else {
    gfx.setTextColor(dots ? config.theme.clock : config.theme.background, config.theme.background);
            // }
        #endif
    dots = !dots;
    gfx.setCursor(_left() + _dotsleft, _top());
    gfx.print(":");
    #endif // FLIP_CLOCK
    gfx.setFont();
    #ifndef FLIP_CLOCK
    // FLIP_CLOCK rysuje karteczki bezpośrednio na dsp – _fb nie jest używany do wyświetlania.
    // Gdyby _fb->display() zostało wywołane, nadpisałoby karteczki kolorem tła.
    if (_fb->ready()) { _fb->display(); }
    #endif
        // Mai névnap letöltése - csak ha engedélyezve van.
        #ifdef NAMEDAYS_FILE
    if (config.store.nameday) {
        static uint32_t lastRotation = 0;
        if (millis() - lastRotation >= 4000) {
            getNamedayUpper(_namedayBuf, sizeof(_namedayBuf));
            if (!config.isScreensaver && strcmp(_oldNamedayBuf, _namedayBuf) != 0) {
                strlcpy(_oldNamedayBuf, _namedayBuf, sizeof(_oldNamedayBuf));
                _namedaywidth = strlen(_namedayBuf) * CHARWIDTH * namedayConf.textsize; // csak változáskor számoljuk újra
                _printNameday();
            }
            lastRotation = millis();
        }
    }
        #endif // NAMEDAYS_FILE
}


void ClockWidget::_formatDate() {

        #if defined(DSP_OLED) && (DSP_MODEL == DSP_SSD1322)
            // ===== SSD1322: rövid, de NYELVHELYES dátum =====
            #if L10N_LANGUAGE == HU
    snprintf(_tmp, sizeof(_tmp), "%04d.%02d.%02d", network.timeinfo.tm_year + 1900, network.timeinfo.tm_mon + 1, network.timeinfo.tm_mday);

            #elif L10N_LANGUAGE == EN
    snprintf(_tmp, sizeof(_tmp), "%02d/%02d/%04d", network.timeinfo.tm_mon + 1, network.timeinfo.tm_mday, network.timeinfo.tm_year + 1900);

            #elif L10N_LANGUAGE == DE || L10N_LANGUAGE == PL || L10N_LANGUAGE == SK || L10N_LANGUAGE == RU || L10N_LANGUAGE == UA
    snprintf(_tmp, sizeof(_tmp), "%02d.%02d.%04d", network.timeinfo.tm_mday, network.timeinfo.tm_mon + 1, network.timeinfo.tm_year + 1900);

            #elif L10N_LANGUAGE == NL
    snprintf(_tmp, sizeof(_tmp), "%02d-%02d-%04d", network.timeinfo.tm_mday, network.timeinfo.tm_mon + 1, network.timeinfo.tm_year + 1900);

            #elif L10N_LANGUAGE == ES || L10N_LANGUAGE == GR
    snprintf(_tmp, sizeof(_tmp), "%02d/%02d/%04d", network.timeinfo.tm_mday, network.timeinfo.tm_mon + 1, network.timeinfo.tm_year + 1900);

            #else
    // fallback
    snprintf(_tmp, sizeof(_tmp), "%04d-%02d-%02d", network.timeinfo.tm_year + 1900, network.timeinfo.tm_mon + 1, network.timeinfo.tm_mday);
            #endif

    return;
        #endif

        // ===== MINDEN MÁS KIJELZŐ: meglévő hosszú, szöveges forma =====

        #if L10N_LANGUAGE == RU
    sprintf(_tmp, "%2d %s %d", network.timeinfo.tm_mday, LANG::mnths[network.timeinfo.tm_mon], network.timeinfo.tm_year + 1900);

        #elif L10N_LANGUAGE == EN
    sprintf(_tmp, "%2d %s %d", network.timeinfo.tm_mday, LANG::mnths[network.timeinfo.tm_mon], network.timeinfo.tm_year + 1900);

        #elif L10N_LANGUAGE == NL
    sprintf(_tmp, "%s %2d %s %d", LANG::dowf[network.timeinfo.tm_wday], network.timeinfo.tm_mday, LANG::mnths[network.timeinfo.tm_mon], network.timeinfo.tm_year + 1900);

        #elif L10N_LANGUAGE == HU
    sprintf(_tmp, "%d. %s %2d. %s", network.timeinfo.tm_year + 1900, LANG::mnths[network.timeinfo.tm_mon], network.timeinfo.tm_mday, LANG::dowf[network.timeinfo.tm_wday]);

        #elif L10N_LANGUAGE == PL
    sprintf(_tmp, "%s - %02d %s %04d", LANG::dowf[network.timeinfo.tm_wday], network.timeinfo.tm_mday, LANG::mnths[network.timeinfo.tm_mon], network.timeinfo.tm_year + 1900);

        #elif L10N_LANGUAGE == DE
    sprintf(_tmp, "%s, %02d. %s %d", LANG::dowf[network.timeinfo.tm_wday], network.timeinfo.tm_mday, LANG::mnths[network.timeinfo.tm_mon], network.timeinfo.tm_year + 1900);

        #elif L10N_LANGUAGE == ES
    sprintf(_tmp, "%s, %02d. %s %d", LANG::dowf[network.timeinfo.tm_wday], network.timeinfo.tm_mday, LANG::mnths[network.timeinfo.tm_mon], network.timeinfo.tm_year + 1900);
        #endif
}

        /*********************  A névnapok kiírása. *****************************/
        #ifdef NAMEDAYS_FILE
void ClockWidget::getNamedayUpper(char* dest, size_t len) { // commongfx.h - ban van deklarálva.
    const char* nameday = getNameDay(network.timeinfo.tm_mon + 1, network.timeinfo.tm_mday);
    char        tmp[32];
    strlcpy(tmp, nameday, sizeof(tmp));
    for (int i = 0; tmp[i]; i++) { tmp[i] = toupper((unsigned char)tmp[i]); }
    strlcpy(dest, utf8To(tmp, true), len);
}

void ClockWidget::_printNameday() {
    uint16_t nameday_top;
    memcpy_P(&_namedayConf, &namedayConf, sizeof(WidgetConfig));
            #if DSP_MODEL == DSP_ILI9341
    nameday_top = _namedayConf.top + 14;
            #else
    nameday_top = _namedayConf.top + 22;
            #endif
    if (config.store.nameday) {
        // Rajzold le a nyelvfüggő "Névnap:" szót fehér színnel.
        dsp.setTextColor(config.theme.date, config.theme.background);
        dsp.setCursor(_namedayConf.left, _namedayConf.top); // egy sorral feljebb
            #if NAMEDAYS_FILE == GR                         // Görög nyevnél túl hosszú, ezért 1- es méret.
        dsp.setTextSize(1);
            #else
        dsp.setTextSize(_namedayConf.textsize);
            #endif
        if (!config.isScreensaver) {
            // Serial.printf("Widget.cpp->nameday_label: %s \n", nameday_label);
            // Serial.printf("Widget.cpp->utf8To(nameday_label, false): %s \n", utf8To(nameday_label, false));
            dsp.print(utf8To(nameday_label, false)); // <<< Itt már a headerből jön "nameday"
            // Csak a neveket rajzolja arany színnel
            dsp.setTextColor(config.theme.nameday, config.theme.background); // szürke 0x8410
            // Névnap nevének területének törlése a kijelzőről.
            int clearWidth = max(_oldnamedaywidth, _namedaywidth); // A régi és az új név közül a szélesebb szélessége.
            dsp.fillRect(_namedayConf.left, nameday_top, clearWidth, CHARHEIGHT * _namedayConf.textsize, config.theme.background);
            dsp.setCursor(_namedayConf.left, nameday_top);
            dsp.setTextSize(_namedayConf.textsize);
            dsp.print(_namedayBuf);
            strlcpy(_oldNamedayBuf, _namedayBuf, sizeof(_namedayBuf));
            _oldnamedaywidth = _namedaywidth;
        }
    }
}

void ClockWidget ::clearNameday() {
    int clearWidth = max(_oldnamedaywidth, _namedaywidth); // A régi és az új név közül a szélesebb szélessége.
    dsp.fillRect(namedayConf.left, namedayConf.top, clearWidth, (CHARHEIGHT * namedayConf.textsize * 2) + 5, config.theme.background);
}
        #endif // NAMEDAYS_FILE

void ClockWidget::_clearClock() {
        #ifdef FLIP_CLOCK
    // FLIP_CLOCK: karteczki i sekundy rysowane bezpośrednio na dsp (nie przez _fb).
    // _fb->clear() czyściłoby tylko bufor w pamięci — stara pozycja na ekranie
    // pozostałaby niezmieniona. Zamiast tego robimy fillRect na ekranie.
    // _clockwidth obejmuje zarówno karteczki HH:MM, jak i kolumnę sekund,
    // więc jeden prostokąt czyści wszystko.
    dsp.fillRect(_clockleft, _config.top - _timeheight - FLIP_PANEL_VPAD,
                 _clockwidth, _flipPanelH + 1, config.theme.background);
    return;
        #endif
        #ifdef PSFBUFFER
    if (_fb->ready()) {
        _fb->clear();
    } else
        #endif
        #ifndef CLOCKFONT5x7
        // dsp.fillRect(_left(), _top()-_timeheight, _clockwidth, _clockheight+1, 0x8410);
        dsp.fillRect(_left(), _top() - (_timeheight + 2), _clockwidth, _clockheight + 3, config.theme.background);
        // Serial.println("Törlés");
        #else
    dsp.fillRect(_left(), _top(), _clockwidth + 1, _clockheight + 1, config.theme.background);
        #endif
}

void ClockWidget::draw(bool force) {
    if (!_active) { return; }
    _printClock(_getTime() || force);
}

void ClockWidget::_draw() {
    if (!_active) { return; }
    _printClock(true);
}

void ClockWidget::_reset() {
    #ifdef FLIP_CLOCK
    // FLIP_CLOCK: przy zmianie układu (np. font, motyw) przelicz granice
    // i przebuduj FlipDigit z nowymi współrzędnymi i wymiarami.
    _getTimeBounds();
    _initFlipDigits();
    #ifdef PSFBUFFER
    _beginFlipSecBuf();  // reinit bufora sekund na nowej pozycji
    #endif
    #else
        #ifdef PSFBUFFER
    if (_fb->ready()) {
        _fb->freeBuffer();
        _getTimeBounds();
        _begin();
    }
        #endif
    #endif
}

void ClockWidget::_clear() {
    _clearClock();
}

/*************************************************************************************************************************
                                                     FLIP CLOCK IMPLEMENTATION
 *************************************************************************************************************************/
#ifdef FLIP_CLOCK


/**
 * @brief Inicjalizuje obiekty FlipDigit (_flipHH i _flipMM) na podstawie
 *        aktualnych wymiarów i pozycji zegara.
 *
 * Układ poziomy karteczek na ekranie:
 *   [xHH .. xHH+pw]  – panel HH
 *   [+gap]            – odstęp
 *   [kolonW]          – dwukropek (rysowany osobno przez _drawFlipColon)
 *   [+gap]            – odstęp
 *   [xMM .. xMM+pw]  – panel MM
 *
 * Szerokość bufora FlipDigit = pw + FLIP_SHADOW_OFF, żeby cień (przesunięty
 * w prawo) mieścił się w buforze i nie wychodził poza karteczkę.
 */
void ClockWidget::_initFlipDigits() {
    const uint16_t pw  = _flipDigitW * 2 + FLIP_DIGIT_PAD * 2;  // szerokość jednej karteczki
    const uint16_t gap = FLIP_DIGIT_GAP;                          // odstęp między karteczką a dwukropkiem
    uint16_t colonW = _charWidth(':', Clock_GFXfontPtr);
    if (colonW == 0) colonW = pw / 4;  // fallback gdy font nie ma dwukropka

    // _config.top = baseline GFX (dół glifów), tak samo jak setCursor y
    int16_t y   = _config.top;
    int16_t xHH = _clockleft;  // lewy bok karteczki HH = lewy bok całego zegara
    // lewy bok karteczki MM = HH + szerokość + gap + dwukropek + gap
    int16_t xMM = xHH + (int16_t)pw + (int16_t)gap + (int16_t)colonW + (int16_t)gap;
    // Szerokość bufora zawiera cień (FLIP_SHADOW_OFF px) po prawej stronie karteczki
    int16_t fdW = (int16_t)pw + FLIP_SHADOW_OFF;
    int16_t fdH = (int16_t)_timeheight;  // wysokość = wysokość tekstu (bez marginesów VPAD)

    _flipHH.init(&dsp, xHH, y, fdW, fdH);
    _flipMM.init(&dsp, xMM, y, fdW, fdH);
    _flipHH.setLabel("flip HH");
    _flipMM.setLabel("flip MM");

    // Wyświetl aktualny czas natychmiast (bez animacji) po inicjalizacji
    char hhStr[3] = { _timebuffer[0], _timebuffer[1], '\0' };
    char mmStr[3] = { _timebuffer[3], _timebuffer[4], '\0' };
    _flipHH.setValue(hhStr);
    _flipMM.setValue(mmStr);
}

/**
 * @brief Rysuje bieżące sekundy do psFrameBuffer i wysyła burst SPI.
 *        Zastępuje stare dsp.print() (pixel-by-pixel, ~7 ms/s) jednym transferem bulk (~0.05 ms/s).
 *        Dwukropek usunięty – zbędny dla flip clock.
 */
void ClockWidget::_drawFlipSeconds() {
    if (!_fb->ready()) return;
    _fb->clear();
    _fb->setFont(Clock_GFXfontPtr_Sec);
    _fb->setTextSize(0);
    _fb->setTextColor(config.theme.seconds, config.theme.background);
    // Baseline = dolna krawędź bufora (height - 2 = margines 1px od dołu)
    _fb->setCursor(1, _fb->height() - 2);
    sprintf(_tmp, "%02d", network.timeinfo.tm_sec);
    _fb->print(_tmp);
    _fb->display();
}

/**
 * @brief Pętla animacji flip clock.
 *        Wywoływana co ~10 ms z Display::loop().
 *        Co sekundę: aktualizacja sekund przez psFrameBuffer (bez pixel-by-pixel SPI).
 *        Klatki animacji FlipDigit delegowane do _flipHH.loop() i _flipMM.loop().
 */
void ClockWidget::tick() {
    if (!_active) return;

    int8_t curSec = (int8_t)network.timeinfo.tm_sec;
    if (curSec != _lastSec) {
        _lastSec = curSec;
        _drawFlipSeconds();
    }

    _flipHH.loop();
    _flipMM.loop();
}

#endif // FLIP_CLOCK

    #else  // #ifndef DSP_LCD

void ClockWidget::_printClock(bool force) {
    strftime(_timebuffer, sizeof(_timebuffer), "%H:%M", &network.timeinfo);
    if (force) {
        dsp.setCursor(dsp.width() - 5, 0);
        dsp.print(_timebuffer);
    }
    dsp.setCursor(dsp.width() - 5 + 2, 0);
    dsp.print((network.timeinfo.tm_sec % 2 == 0) ? ":" : " ");
}

void ClockWidget::_clearClock() {}

void ClockWidget::draw() {
    if (!_active) { return; }
    _printClock(true);
}
void ClockWidget::_draw() {
    if (!_active) { return; }
    _printClock(true);
}
void ClockWidget::_reset() {}
void ClockWidget::_clear() {}
    #endif // #ifndef DSP_LCD

/****************************************************************************************************
                                                     BITRATE WIDGET
 ****************************************************************************************************/
void BitrateWidget::init(BitrateConfig bconf, uint16_t fgcolor, uint16_t bgcolor) {
    Widget::init(bconf.widget, fgcolor, bgcolor);
    _dimension = bconf.dimension;
    _bitrate = 0;
    _format = BF_UNKNOWN;
    _charSize(bconf.widget.textsize, _charWidth, _textheight);
    memset(_buf, 0, 6);
    // Serial.printf("widgets.cpp->BitrateWidget _init() _dimension %d\n", _dimension) ;
}

void BitrateWidget::setBitrate(uint16_t bitrate) {
    //  Serial.printf("widgets.cpp->BitrateWidget setBitrate() bitrate: %d \n", bitrate) ;
    _bitrate = bitrate; // Módosítás
                        //  if(_bitrate>999) _bitrate = 999;
    if (_bitrate > 20000) { _bitrate = _bitrate / 1000; }
    _draw();
}

void BitrateWidget::setFormat(BitrateFormat format) {
    //  Serial.printf("widgets.cpp->BitrateWidget setFormat() format: %d \n", format) ;
    _format = format;
    _draw();
}

// TODO move to parent
void BitrateWidget::_charSize(uint8_t textsize, uint8_t& width, uint16_t& height) {
    #ifndef DSP_LCD
    width = textsize * CHARWIDTH;
    height = textsize * CHARHEIGHT;
    #else
    width = 1;
    height = 1;
    #endif
}

void BitrateWidget::_draw() { // Módosítás
    _clear();
    // Serial.printf("widgets.cpp->BitrateWidget _draw() _active: %d _format: %d _bitrate %d \n", _active, _format, _bitrate) ;
    if (!_active || _format == BF_UNKNOWN || _bitrate == 0) {
        // Serial.printf("widgets.cpp->BitrateWidget _draw() nem fut le. \n") ;
        return;
    }
    #ifndef DSP_OLED
    if (config.store.nameday) { //  Ha be van kapcsolva a nameday Módosítás "nameday"
        dsp.drawRect(_config.left, _config.top, _dimension * 2, (_dimension / 2) - 6, _fgcolor);
        dsp.fillRect(_config.left + _dimension, _config.top, _dimension, (_dimension / 2) - 6, _fgcolor);
        // Serial.printf("widgets.cpp->BitrateWidget _draw() config.store.nameday: %d \n", config.store.nameday) ;
    } else {
        dsp.drawRect(_config.left, _config.top, _dimension, _dimension, _fgcolor);                              // Eredeti.
        dsp.fillRect(_config.left, _config.top + _dimension / 2 + 1, _dimension, _dimension / 2 - 1, _fgcolor); // Eredeti
        // Serial.printf("widgets.cpp->BitrateWidget _draw() config.store.nameday: %d \n", config.store.nameday) ;
    }
    dsp.setFont();
    dsp.setTextSize(_config.textsize);
    dsp.setTextColor(_fgcolor, _bgcolor);
    if (_bitrate < 999) {
        snprintf(_buf, 6, "%d", _bitrate); // Módisítás "bitrate"
    } else {
        float _br = (float)_bitrate / 1000;
        snprintf(_buf, 6, "%.1f", _br);
    }
    if (config.store.nameday) { //  Ha be van kapcsolva a nameday
        dsp.setCursor(_config.left + _dimension / 2 - _charWidth * strlen(_buf) / 2, _config.top + _dimension / 4 - _textheight / 2 - 2);
    } else {
        dsp.setCursor(_config.left + _dimension / 2 - _charWidth * 3 / 2 + 1, _config.top + (_dimension / 2) - 3 - _textheight);
    }
    dsp.print(_buf);
    dsp.setTextColor(_bgcolor, _fgcolor);
    if (config.store.nameday) { //  Ha be van kapcsolva a nameday
        dsp.setCursor(_config.left + _dimension + _dimension / 2 - _charWidth * 3 / 2, _config.top + _dimension / 4 - _textheight / 2 - 2);
    } else {
        dsp.setCursor(_config.left + _dimension / 2 - _charWidth * 3 / 2, _config.top + _dimension / 2 + _dimension / 4 - _textheight / 2 + 2);
    }
    switch (_format) {
        case BF_MP3: dsp.print("MP3"); break;
        case BF_AAC: dsp.print("AAC"); break;
        case BF_FLAC: dsp.print("FLC"); break;
        case BF_OGG: dsp.print("OGG"); break;
        case BF_WAV: dsp.print("WAV"); break;
        case BF_VOR: dsp.print("VOR"); break; // Módisítás "bitrate"
        case BF_OPU: dsp.print("OPU"); break; // Módisítás "bitrate"
        default: break;
    }
    #else // OLED DISPLAY
    // Serial.printf(
    // "widget.cpp--> BITRATE-- left: %d, top: %d, dimension: %d _bitrate: %d, textsize: %d \n ", _config.left, _config.top, _dimension, _bitrate, _config.textsize
    // );
    // felső: üres keret (bitrate szám)
    dsp.drawRect(_config.left, _config.top, _dimension, _dimension, GRAY_5);
    // alsó: kitöltött (formátum)
    dsp.fillRect(_config.left, _config.top + _dimension / 2, _dimension, _dimension / 2, GRAY_5);
    // -------- bitrate szám --------
    dsp.setFont();
    dsp.setTextSize(_config.textsize);
    dsp.setTextColor(GRAY_2);

    if (_bitrate < 999) {
        snprintf(_buf, 6, "%d", _bitrate); // Módisítás "bitrate"
    } else {
        float _br = (float)_bitrate / 1000;
        snprintf(_buf, 6, "%.1f", _br);
    }
    uint8_t cw = CHARWIDTH * _config.textsize;
    uint8_t ch = CHARHEIGHT * _config.textsize;
    dsp.setCursor((_config.left + (_dimension - strlen(_buf) * cw) / 2) + 1, _config.top + (_dimension / 4) - ch / 2 + 1);
    dsp.print(_buf);
    // -------- formátum szöveg --------
    dsp.setTextColor(BLACK);
    const char* fmt = "";
    switch (_format) {
        case BF_MP3: fmt = "MP3"; break;
        case BF_AAC: fmt = "AAC"; break;
        case BF_FLAC: fmt = "FLC"; break;
        case BF_OGG: fmt = "OGG"; break;
        case BF_WAV: fmt = "WAV"; break;
        case BF_VOR: fmt = "VOR"; break;
        case BF_OPU: fmt = "OPU"; break;
        default: return;
    }
    dsp.setCursor((_config.left + (_dimension - strlen(fmt) * cw) / 2) + 1, _config.top + (3 * _dimension / 4) - ch / 2);
    dsp.print(fmt);
    #endif
}

void BitrateWidget::_clear() {
    if (config.store.nameday && DSP_MODEL != DSP_SSD1322) {                                //  Ha be van kapcsolva a nameday
        dsp.fillRect(_config.left, _config.top, _dimension * 2, _dimension / 2, _bgcolor); // lapos forma törlése
        // Serial.printf("widgets.cpp->BitrateWidget _clear() (lapos törlés) config.store.nameday: %d \n", config.store.nameday) ;
    } else {
        dsp.fillRect(_config.left, _config.top, _dimension, _dimension, _bgcolor); // négyzetes forma törlése
        // Serial.printf("widgets.cpp->BitrateWidget _clear() (négyzetes törlés) config.store.nameday: %d \n", config.store.nameday) ;
    }
    VuWidget::setLabelsDrawn(false); // Módosítás! (false) esetén újrarajzolja az L R címkét. "wumeter"
}

/* Törli mindkét bitratewidget területét és a "nameday" területet is. */
void BitrateWidget::clearAll() {
    dsp.fillRect(_config.left, _config.top, _dimension * 2, _dimension + 11, _bgcolor);
    // Serial.printf("widgets.cpp->BitrateWidget clearAll() \n") ;
}

/**********************************************************************************
                                          PLAYLIST WIDGET
    **********************************************************************************/
    #if defined(PLAYLIST_SCROLL_MOVING_CURSOR) //&& (DSP_WIDTH == 480) && (DSP_HEIGHT == 320)

        #define MAX_PL_PAGE_ITEMS 15
static String   _plCache[MAX_PL_PAGE_ITEMS];
static int16_t  _plLoadedPage = -1;
static int16_t  _plLastGlobalPos = -1;
static uint32_t _plLastDrawTime = 0;

void PlayListWidget::init(ScrollWidget* current) {
    Widget::init({0, 0, 0, WA_LEFT}, 0, 0);

    _plItemHeight = playlistConf.widget.textsize * (CHARHEIGHT - 1) + playlistConf.widget.textsize * 4;

    _plTtemsCount = (dsp.height() - 2) / _plItemHeight;
    if (_plTtemsCount < 1) _plTtemsCount = 1;
    if (_plTtemsCount > MAX_PL_PAGE_ITEMS) _plTtemsCount = MAX_PL_PAGE_ITEMS;

    uint16_t contentHeight = _plTtemsCount * _plItemHeight;
    _plYStart = (dsp.height() - contentHeight) / 2;

    _plLoadedPage = -1;
    _plLastGlobalPos = -1;
    _plCurrentPos = 0;
    _plLastDrawTime = 0;
}

void _loadPlaylistPage(int pageIndex, int itemsPerPage, int totalItems) {
    for (int i = 0; i < MAX_PL_PAGE_ITEMS; i++) _plCache[i] = "";

    if (config.playlistLength() == 0) return;

    File playlist = config.SDPLFS()->open(REAL_PLAYL, "r");
    File index = config.SDPLFS()->open(REAL_INDEX, "r");

    if (!playlist || !index) return;

    int startIdx = pageIndex * itemsPerPage;

    for (int i = 0; i < itemsPerPage; i++) {
        int currentGlobalIdx = startIdx + i;
        if (currentGlobalIdx >= config.playlistLength()) break;

        index.seek(currentGlobalIdx * 4, SeekSet);
        uint32_t posAddr;
        if (index.readBytes((char*)&posAddr, 4) != 4) break;

        playlist.seek(posAddr, SeekSet);
        String line = playlist.readStringUntil('\n');

        int tabIdx = line.indexOf('\t');
        if (tabIdx > 0) line = line.substring(0, tabIdx);
        line.trim();

        if (config.store.numplaylist && line.length() > 0) {
            _plCache[i] = String(currentGlobalIdx + 1) + ". " + line;
        } else {
            _plCache[i] = line;
        }
    }
    playlist.close();
    index.close();
}

void PlayListWidget::drawPlaylist(uint16_t currentItem) {
    // If > 2000ms has passed since the last call, we assume we are returning from another screen and redraw the entire screen
    bool isLongPause = (millis() - _plLastDrawTime > 2000);
    _plLastDrawTime = millis();
    int activeIdx = (currentItem > 0) ? (currentItem - 1) : 0;
    int itemsPerPage = _plTtemsCount;
    int newPage = activeIdx / itemsPerPage;
    int newLocalPos = activeIdx % itemsPerPage;
    _plCurrentPos = newLocalPos;
    bool pageChanged = (newPage != _plLoadedPage);
    // FULL DRAWING CONDITIONS:
    // 1. Page change
    // 2. Return to player (V-Tom 2 seconds / automatic stream selection confirmation)
    // 3. Empty cache (start)
    if (pageChanged || isLongPause || _plCache[0].length() == 0) {
        // If it's just returning to the same page, we don't need to read from memory (saves time)
        // We only read from memory if the page has actually changed or the cache is empty.
        if (pageChanged || _plCache[0].length() == 0) {
            _loadPlaylistPage(newPage, itemsPerPage, config.playlistLength());
            _plLoadedPage = newPage;
        }
        // We draw the entire background and list (this removes the garbage after the player)
        dsp.fillRect(0, _plYStart, dsp.width(), itemsPerPage * _plItemHeight, config.theme.background);
        for (int i = 0; i < itemsPerPage; i++) { _printPLitem(i, _plCache[i].c_str()); }
    } else {
        // SMART REDRAW - Fast track for smooth scrolling
        int oldLocalPos = (_plLastGlobalPos > 0 ? _plLastGlobalPos - 1 : 0) % itemsPerPage;
        if (oldLocalPos != newLocalPos && oldLocalPos >= 0 && oldLocalPos < itemsPerPage) { _printPLitem(oldLocalPos, _plCache[oldLocalPos].c_str()); }
        _printPLitem(newLocalPos, _plCache[newLocalPos].c_str());
    }
    _plLastGlobalPos = currentItem;
}

void PlayListWidget::_printPLitem(uint8_t pos, const char* item) {
#ifndef DSP_LCD
   
    if (pos >= _plTtemsCount) return;

    int16_t yPos = _plYStart + pos * _plItemHeight;
    bool isSelected = (pos == _plCurrentPos);

#ifdef PL_WIDGET_WITCH_BAR
	uint16_t bgColor = isSelected ? config.theme.plcurrentfill
                                  : config.theme.background;
    uint16_t fgColor = isSelected ? config.theme.plcurrent
                                  : config.theme.playlist[0];
    
#else
    uint16_t bgColor = config.theme.background;
    uint16_t fgColor = isSelected ? config.theme.plcurrent
                                  : config.theme.playlist[0];
#endif

    dsp.fillRect(0, yPos, dsp.width(), _plItemHeight - 1, bgColor);

    if (item && item[0] != '\0') {
        dsp.setTextColor(fgColor, bgColor);
        dsp.setTextSize(playlistConf.widget.textsize);
        dsp.setCursor(TFT_FRAMEWDT, yPos + 4);
        dsp.print(utf8To(item, true));
    }
#endif
}

void PlayListWidget::resetCache() {
    _plLoadedPage = -1;
}
    #else

void PlayListWidget::init(ScrollWidget* current) {
    Widget::init({0, 0, 0, WA_LEFT}, 0, 0);
    _current = current;
        #ifndef DSP_LCD
    _plItemHeight = playlistConf.widget.textsize * (CHARHEIGHT - 1) + playlistConf.widget.textsize * 4;
    _plTtemsCount = round((float)dsp.height() / _plItemHeight);
    if (_plTtemsCount % 2 == 0) { _plTtemsCount++; }
    _plCurrentPos = _plTtemsCount / 2;
    _plYStart = (dsp.height() / 2 - _plItemHeight / 2) - _plItemHeight * (_plTtemsCount - 1) / 2 + playlistConf.widget.textsize * 2;
        #else
    _plTtemsCount = PLMITEMS;
    _plCurrentPos = 1;
        #endif
}

uint8_t PlayListWidget::_fillPlMenu(int from, uint8_t count) {
    int     ls = from;
    uint8_t c = 0;
    bool    finded = false;
    if (config.playlistLength() == 0) { return 0; }
    File playlist = config.SDPLFS()->open(REAL_PLAYL, "r");
    File index = config.SDPLFS()->open(REAL_INDEX, "r");
    while (true) {
        if (ls < 1) {
            ls++;
            _printPLitem(c, "");
            c++;
            continue;
        }
        if (!finded) {
            index.seek((ls - 1) * 4, SeekSet);
            uint32_t pos;
            index.readBytes((char*)&pos, 4);
            finded = true;
            index.close();
            playlist.seek(pos, SeekSet);
        }
        bool pla = true;
        while (pla) {
            pla = playlist.available();
            String stationName = playlist.readStringUntil('\n');
            stationName = stationName.substring(0, stationName.indexOf('\t'));
            if (config.store.numplaylist && stationName.length() > 0) { stationName = String(from + c) + ". " + stationName; }
            _printPLitem(c, stationName.c_str());
            c++;
            if (c >= count) { break; }
        }
        break;
    }
    playlist.close();
    return c;
}
        #ifndef DSP_LCD
void PlayListWidget::drawPlaylist(uint16_t currentItem) {
    uint8_t lastPos = _fillPlMenu(currentItem - _plCurrentPos, _plTtemsCount);
    if (lastPos < _plTtemsCount) { dsp.fillRect(0, lastPos * _plItemHeight + _plYStart, dsp.width(), dsp.height() / 2, config.theme.background); }
}

void PlayListWidget::_printPLitem(uint8_t pos, const char* item) {
    dsp.setTextSize(playlistConf.widget.textsize);
    if (pos == _plCurrentPos) {
        _current->setText(item);
    } else {
        uint8_t plColor = (abs(pos - _plCurrentPos) - 1) > 4 ? 4 : abs(pos - _plCurrentPos) - 1;
        dsp.setTextColor(config.theme.playlist[plColor], config.theme.background);
        dsp.setCursor(TFT_FRAMEWDT, _plYStart + pos * _plItemHeight);
        dsp.fillRect(0, _plYStart + pos * _plItemHeight - 1, dsp.width(), _plItemHeight - 2, config.theme.background);
        dsp.print(utf8To(item, true));
    }
}
        #else
void PlayListWidget::_printPLitem(uint8_t pos, const char* item) {
	
    if (pos == _plCurrentPos) {
        _current->setText(item);
    } else {
        dsp.setCursor(1, pos);
        char tmp[dsp.width()] = {0};
        strlcpy(tmp, utf8To(item, true), dsp.width());
        dsp.print(tmp);
    }
}

void PlayListWidget::drawPlaylist(uint16_t currentItem) {
    dsp.clear();
    _fillPlMenu(currentItem - _plCurrentPos, _plTtemsCount);
    dsp.setCursor(0, 1);
    dsp.write(uint8_t(126));
}
        #endif // DSP_LCD

void PlayListWidget::resetCache() {}
    #endif

#endif // #if DSP_MODEL!=DSP_DUMMY