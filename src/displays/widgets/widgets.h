//MĂłdosĂ­tva! v0.9.710
#ifndef widgets_h
#define widgets_h
#if DSP_MODEL != DSP_DUMMY
  #include "widgetsconfig.h"

#include <Arduino.h>
class Adafruit_GFX;
#if defined(DSP_ILI9488)
  #include "../displayILI9488.h"
  #include "../dspcore.h"  // To daje typedef Canvas

#else
  // JeĹ›li nie wiemy jaki wyĹ›wietlacz, uĹĽyj forward declaration
  // ale jako klasa, nie typedef
  class GFXcanvas16;  // Forward declaration oryginalnej klasy
  typedef GFXcanvas16 Canvas;  // Teraz to jest poprawne
#endif

/** Ustawia czcionkÄ™ zegara runtime (1..7) */
void widgetsSetClockFont(uint8_t fontId);

  #ifndef DSP_LCD
    #define CHARWIDTH  6
    #define CHARHEIGHT 8
  #else
    #define CHARWIDTH  1
    #define CHARHEIGHT 1
  #endif

#include "FlipDigit.h"
class psFrameBuffer;

class Widget {
public:
  Widget() {
    _active = false;
  }
  virtual ~Widget() {}
  virtual void loop() {}
  virtual void init(WidgetConfig conf, uint16_t fgcolor, uint16_t bgcolor) {
    _config = conf;
    _fgcolor = fgcolor;
    _bgcolor = bgcolor;
    _width = _backMove.width = 0;
    _backMove.x = _config.left;
    _backMove.y = _config.top;
    _moved = _locked = false;
  }
  void setAlign(WidgetAlign align) {
    _config.align = align;
  }
  void setActive(bool act, bool clr = false) {
    _active = act;
    if (_active && !_locked) {
      _draw();
    }
    if (clr && !_locked) {
      _clear();
    }
  }
  void lock(bool lck = true) {
    _locked = lck;
    if (_locked) {
      _reset();
    }
    if (_locked && _active) {
      _clear();
    }
  }
  void unlock() {
    _locked = false;
  }
  bool locked() {
    return _locked;
  }
  void moveTo(MoveConfig mv) {
    if (mv.width < 0) {
      return;
    }
    _moved = true;
    if (_active && !_locked) {
      _clear();
    }
    _config.left = mv.x;
    _config.top = mv.y;
    if (mv.width > 0) {
      _width = mv.width;
    }
    _reset();
    _draw();
  }
  void moveBack() {
    if (!_moved) {
      return;
    }
    if (_active && !_locked) {
      _clear();
    }
    _config.left = _backMove.x;
    _config.top = _backMove.y;
    _width = _backMove.width;
    _moved = false;
    _reset();
    _draw();
  }

void setColors(uint16_t fg, uint16_t bg, bool redraw = true) {
	_fgcolor = fg;
    _bgcolor = bg;
    if (redraw && _active && !_locked) {
      _clear();
      _draw();
    }
  }

protected:
  bool _active, _moved, _locked;
  uint16_t _fgcolor, _bgcolor, _width;
  WidgetConfig _config;
  MoveConfig _backMove;
  virtual void _draw() {}
  virtual void _clear() {}
  virtual void _reset() {}
};

class TextWidget : public Widget {
public:
  TextWidget() {}
  TextWidget(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor) {
    init(wconf, buffsize, uppercase, fgcolor, bgcolor);
  }
  ~TextWidget();
  using Widget::init;
  void init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor);
  void setText(const char *txt);
  void setText(int val, const char *format);
  void setText(const char *txt, const char *format);
  bool uppercase() {
    return _uppercase;
  }

protected:
  char *_text;
  char *_oldtext;
  bool _uppercase;
  uint16_t _buffsize, _textwidth, _oldtextwidth, _oldleft, _textheight;
  uint8_t _charWidth;

protected:
  void _draw();
  uint16_t _realLeft(bool w_fb = false);
  void _charSize(uint8_t textsize, uint8_t &width, uint16_t &height);
};

class FillWidget : public Widget {
public:
  FillWidget() {}
  FillWidget(FillConfig conf, uint16_t bgcolor) {
    init(conf, bgcolor);
  }
  using Widget::init;
  void init(FillConfig conf, uint16_t bgcolor);
  void setHeight(uint16_t newHeight);

protected:
  uint16_t _height;
  void _draw();
};

class ScrollWidget : public TextWidget {
public:
  ScrollWidget() {}
  ScrollWidget(const char *separator, ScrollConfig conf, uint16_t fgcolor, uint16_t bgcolor);
  ~ScrollWidget();
  using Widget::init;
  void init(const char *separator, ScrollConfig conf, uint16_t fgcolor, uint16_t bgcolor);
  void loop();
  void setText(const char *txt);
  void setText(const char *txt, const char *format);
  void setColors(uint16_t fg, uint16_t bg, bool redraw = true);
  void setFbLabel(const char *lbl) { if (_fb) _fb->setLabel(lbl); }

private:
  char *_sep;
  char *_window;
  int16_t _x;
  bool _doscroll;
  uint8_t _scrolldelta;
  uint16_t _scrolltime;
  uint32_t _scrolldelay;
  uint16_t _sepwidth, _startscrolldelay;
  uint8_t _charWidth;
  int16_t _fontBaselineY = 0;
  psFrameBuffer *_fb = nullptr;
  psFrameBuffer *_scrollCache = nullptr;
  bool _scrollCacheReady = false;

private:
  void _setTextParams();
  void _buildScrollCache();
  void _calcX();
  void _drawFrame();
  void _draw();
  bool _checkIsScrollNeeded();
  bool _checkDelay(int m, uint32_t &tstamp);
  void _clear();
  void _reset();
};

class SliderWidget : public Widget {
public:
  SliderWidget() {}
  SliderWidget(FillConfig conf, uint16_t fgcolor, uint16_t bgcolor, uint32_t maxval, uint16_t oucolor = 0) {
    init(conf, fgcolor, bgcolor, maxval, oucolor);
  }
  using Widget::init;
  void init(FillConfig conf, uint16_t fgcolor, uint16_t bgcolor, uint32_t maxval, uint16_t oucolor = 0);
  void setValue(uint32_t val);
  void setOutlineColor(uint16_t color, bool redraw = true) {
    _oucolor = color;
    if (redraw && _active && !_locked) {
      _clear();
      _draw();
    }
  }

protected:
  uint16_t _height, _oucolor, _oldvalwidth;
  uint32_t _max, _value;
  uint8_t _outlined;
  void _draw();
  void _drawslider();
  void _clear();
  void _reset();
};

class SdFftWidget : public Widget {
public:
  SdFftWidget() {}
  ~SdFftWidget();
  void init(WidgetConfig wconf, uint16_t width, uint16_t height, uint8_t bands, uint16_t fgcolor, uint16_t bgcolor, uint16_t intervalMs = 50);
  void loop();

protected:
  uint16_t _height = 0;
  uint16_t _intervalMs = 50;
  uint8_t _bands = 16;
  uint8_t _prev[16] = {0};
  bool _inited = false;
  uint32_t _lastMs = 0;
  Canvas *_canvas = nullptr;
  void _draw();
  void _clear();
  void _reset();
};
/************************************************************ VU WIDGET **********************************************/
class VuWidget : public Widget {
public:
  VuWidget() {}  // MĂłdosĂ­tĂˇs: vumidcolor plussz paramĂ©ter.
  VuWidget(WidgetConfig wconf, VUBandsConfig bands, uint16_t vumaxcolor, uint16_t vumidcolor, uint16_t vumincolor, uint16_t bgcolor) {
    init(wconf, bands, vumaxcolor, vumidcolor, vumincolor, bgcolor);
  }
  ~VuWidget();  // MĂłdosĂ­tĂˇs: vumidcolor plussz paramĂ©ter.
  using Widget::init;
  void init(WidgetConfig wconf, VUBandsConfig bands, uint16_t vumaxcolor, uint16_t vumidcolor, uint16_t vumincolor, uint16_t bgcolor);
  void loop();
  static void setLabelsDrawn(bool value);  // MĂłdosĂ­tĂˇs
  static bool isLabelsDrawn();             // MĂłdosĂ­tĂˇs
  
  void setVuColors(uint16_t vumaxcolor, uint16_t vumidcolor, uint16_t vumincolor, uint16_t bgcolor, bool redraw = true) {
    _vumaxcolor = vumaxcolor;
    _vumidcolor = vumidcolor;
    _vumincolor = vumincolor;
    _bgcolor = bgcolor;
    _segmentsInitialized = false;
    if (redraw && _active && !_locked) {
      _clear();
      _draw();
    }
  }
  void invalidate() {
    _segmentsInitialized = false;
    _labelsDrawn = false;
  }

protected:
  #if defined(DSP_OLED)
  uint16_t _maxDimension = 216;  // VU teljes hossza pixelben
  uint16_t _peakL = 0;
  uint16_t _peakR = 0;
  uint8_t _peakFallDelay = 6;  // peak kĂ©sleltetĂ©s
  uint8_t _peakFallRate = 1;   // peak esĂ©s sebessĂ©ge
  uint8_t _peakFallDelayCounter = 0;
  #endif

  #if !defined(DSP_LCD) && !defined(DSP_OLED)
  Canvas *_canvas = nullptr;
  #endif
  static bool _labelsDrawn;  // MĂłdosĂ­tĂˇs Ăşj vĂˇltozĂł.
  VUBandsConfig _bands;
  uint16_t _vumaxcolor, _vumidcolor, _vumincolor;  // MĂłdosĂ­tĂˇs: plussz _vumidcolor
  static constexpr uint8_t _maxSegments = 64;
  uint16_t _segmentColorsL[_maxSegments] = {};
  uint16_t _segmentColorsR[_maxSegments] = {};
  bool _segmentsInitialized = false;
  void _draw();
  void _clear();
};
/********************************************************** NUM WIDGET *******************************************/
class NumWidget : public TextWidget {
public:
  using Widget::init;
  void init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor);
  void setText(const char *txt);
  void setText(int val, const char *format);

protected:
  void _getBounds();
  void _draw();
};

class ProgressWidget : public TextWidget {
public:
  ProgressWidget() {}
  ProgressWidget(WidgetConfig conf, ProgressConfig pconf, uint16_t fgcolor, uint16_t bgcolor) {
    init(conf, pconf, fgcolor, bgcolor);
  }
  using Widget::init;
  void init(WidgetConfig conf, ProgressConfig pconf, uint16_t fgcolor, uint16_t bgcolor) {
    TextWidget::init(conf, pconf.width, false, fgcolor, bgcolor);
    _speed = pconf.speed;
    _width = pconf.width;
    _barwidth = pconf.barwidth;
    _pg = 0;
  }
  void loop();

private:
  uint8_t _pg;
  uint16_t _speed, _barwidth;
  uint32_t _scrolldelay;
  void _progress();
  bool _checkDelay(int m, uint32_t &tstamp);
};

class ClockWidget : public Widget {
public:
  using Widget::init;
  void init(WidgetConfig wconf, uint16_t fgcolor, uint16_t bgcolor);
  void draw(bool force = false);
  uint8_t textsize() {
    return _config.textsize;
  }
  void clear() {
    _clearClock();
  }
  #ifdef NAMEDAYS_FILE
  void clearNameday();
  #endif
  inline uint16_t dateSize() {
    return _space + _dateheight;
  }
  inline uint16_t clockWidth() {
    return _clockwidth;
  }
  bool isMMTap(uint16_t x, uint16_t y) const;
  bool isSecondsTap(uint16_t x, uint16_t y) const;
  void tick();

private:
  #ifndef DSP_LCD
  Adafruit_GFX &getRealDsp();
  #endif
  #if defined(DSP_OLED) && (DSP_MODEL == DSP_SSD1322)
  void _drawShortDateSSD1322();
  #endif

protected:
  char _timebuffer[20] = "00:00";
  char _lastTimebuffer[6] = "";
  char _tmp[38] = {}, _datebuf[38] = {};  // MĂłdosĂ­tva 38-ra v7.4
  uint8_t _superfont = 0;
  uint16_t _clockleft = 0, _clockwidth = 0, _timewidth = 0, _dotsleft = 0, _linesleft = 0;
  uint8_t _clockheight = 0, _timeheight = 0, _dateheight = 0, _space = 0;
  char _namedayBuf[30] = {}, _oldNamedayBuf[30] = {};         // MĂłdosĂ­tĂˇs "nameday"
  uint16_t _namedaywidth, _oldnamedayleft, _oldnamedaywidth;  //MĂłdosĂ­tĂˇs "nameday"
  bool _namedayLabelDrawn = false;
  uint16_t _forceflag = 0;
  bool dots = true;
  bool _fullclock;
  psFrameBuffer *_fb = nullptr;
  WidgetConfig _namedayConf;  //"nameday"
  WidgetConfig _dateConf;     // MĂłdosĂ­tĂˇs Ăşj sor.
  void _draw();
  void _clear();
  void _reset();
  void _getTimeBounds();
  void _printClock(bool force = false);
  void _clearClock();
  void _formatDate();

  #ifdef NAMEDAYS_FILE
  void _printNameday();                          // MĂłdosĂ­tĂˇs Ăşj sor. "nameday"
  void getNamedayUpper(char *dest, size_t len);  // MĂłdosĂ­tĂˇs "nameday"
  #endif
  bool _getTime();
  bool _flipEnabled() const;
  bool _secondsEnabled() const;
  uint16_t _left();
  uint16_t _top();
  void _begin();

  // â”€â”€ Stan flip clock â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
  char     _prevTimebuffer[6]  = "00:00"; // poprzedni czas HH:MM (do wykrywania zmian)
  uint16_t _flipDigitW         = 0;       // max szerokoĹ›Ä‡ cyfry '0'..'9' w aktywnym foncie
  uint16_t _flipPanelH         = 0;       // wysokoĹ›Ä‡ karteczki = _timeheight + 2*VPAD
  int8_t   _lastSec            = -1;      // ostatnia sekunda â€“ do wykrycia zmiany dla blinku
  int16_t  _secGap             = 0;       // staĹ‚y odstÄ™p [px] od prawej krawÄ™dzi MM do sekund (obliczany raz na pozycji PLAYER)

  FlipDigit _flipHH;  // karteczka godzin (HH)
  FlipDigit _flipMM;  // karteczka minut (MM)

  void _initFlipDigits();    // inicjalizacja FlipDigit po (re)kalkulacji pozycji
  void _drawFlipSeconds();   // rysowanie sekund przez psFrameBuffer (szybki burst SPI)
  void _beginFlipSecBuf();   // inicjalizacja _fb jako bufor sekund
};

class BitrateWidget : public Widget {
public:
  BitrateWidget() {}
  BitrateWidget(BitrateConfig bconf, uint16_t fgcolor, uint16_t bgcolor) {
    init(bconf, fgcolor, bgcolor);
  }
  ~BitrateWidget() {}
  using Widget::init;
  void init(BitrateConfig bconf, uint16_t fgcolor, uint16_t bgcolor);
  void setBitrate(uint16_t bitrate);
  void setFormat(BitrateFormat format);
  void clearAll();

protected:
  BitrateFormat _format;
  char _buf[6];
  uint8_t _charWidth;
  uint16_t _dimension, _bitrate, _textheight;
  void _draw();
  void _clear();
  void _charSize(uint8_t textsize, uint8_t &width, uint16_t &height);
};

class PlayListWidget : public Widget {
public:
  using Widget::init;
  void init(ScrollWidget *current);
  void drawPlaylist(uint16_t currentItem);
  void resetCache();
  inline uint16_t itemHeight() {
    return _plItemHeight;
  }
  inline uint16_t currentTop() {
    return _plYStart + _plCurrentPos * _plItemHeight;
  }

private:
  ScrollWidget *_current;
  uint16_t _plItemHeight, _plTtemsCount, _plCurrentPos;
  int _plYStart;
  uint8_t _fillPlMenu(int from, uint8_t count);
  void _printPLitem(uint8_t pos, const char *item);
};

#endif
#endif

