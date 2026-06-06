#pragma once
#if defined(SD_COVER_ART) && SDC_CS != 255

typedef unsigned long my_ulong;
#include <JPEGDEC.h>
#include "../../core/options.h"
#include "../../core/player.h"
#include <SD.h>
#include <SPIFFS.h>
#include "../../core/sdmanager.h"
#include "../dspcore.h"
#ifdef SD_COVER_SOURCE_LASTFM
    #ifdef INTELSHORT
        #undef INTELSHORT
    #endif
    #ifdef INTELLONG
        #undef INTELLONG
    #endif
    #ifdef MOTOSHORT
        #undef MOTOSHORT
    #endif
    #ifdef MOTOLONG
        #undef MOTOLONG
    #endif
    #include <PNGdec.h>
    #include <HTTPClient.h>
    #include <WiFiClient.h>
    #include <WiFiClientSecure.h>
    #include "../../core/config.h"
#endif

extern DspCore dsp;

#ifndef LASTFM_COVER_TIMEOUT_MS
    #define LASTFM_COVER_TIMEOUT_MS 5000
#endif

#ifndef SD_COVER_W
  #define SD_COVER_W 100
#endif
#ifndef SD_COVER_H
  #define SD_COVER_H 100
#endif
#ifndef SD_COVER_X
  #define SD_COVER_X 10
#endif
#ifndef SD_COVER_Y
  #define SD_COVER_Y 110
#endif

static uint16_t *_sdCoverSrcBuf = nullptr;
static int16_t   _sdCoverSrcW   = 0;
static int16_t   _sdCoverSrcH   = 0;
#ifdef SD_COVER_SOURCE_LASTFM
static PNG       *_sdCoverPng     = nullptr;
static uint16_t  *_sdCoverPngLine = nullptr;
#endif

static int _sdCoverDrawCb(JPEGDRAW *pDraw) {
    if (!_sdCoverSrcBuf) return 0;
    for (int r = 0; r < pDraw->iHeight; r++) {
        int sy = pDraw->y + r;
        if (sy < 0 || sy >= _sdCoverSrcH) continue;
        int copyStart = max(0, (int)pDraw->x);
        int copyEnd   = min((int)_sdCoverSrcW, (int)pDraw->x + (int)pDraw->iWidth);
        if (copyStart >= copyEnd) continue;
        memcpy(&_sdCoverSrcBuf[sy * _sdCoverSrcW + copyStart],
               &pDraw->pPixels[r * pDraw->iWidth + (copyStart - pDraw->x)],
               (size_t)(copyEnd - copyStart) * sizeof(uint16_t));
    }
    return 1;
}

#ifdef SD_COVER_SOURCE_LASTFM
static int _sdCoverPngDrawCb(PNGDRAW *pDraw) {
    if (!_sdCoverPng || !_sdCoverPngLine || !_sdCoverSrcBuf) return 0;
    if (pDraw->y < 0 || pDraw->y >= _sdCoverSrcH) return 0;
    _sdCoverPng->getLineAsRGB565(pDraw, _sdCoverPngLine, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);
    int copyW = min((int)_sdCoverSrcW, (int)pDraw->iWidth);
    memcpy(&_sdCoverSrcBuf[pDraw->y * _sdCoverSrcW], _sdCoverPngLine, (size_t)copyW * sizeof(uint16_t));
    return 1;
}
#endif

class SdCoverWidget {
public:
    void init(uint16_t bgcolor) {
        _bgcolor = bgcolor;
        _lastPath[0] = '\0';
        _lastMetaSig[0] = '\0';
        _pixelBuf = nullptr;
    }

    void setBgColor(uint16_t color) { _bgcolor = color; }

    // CzyĹ›ci tylko Ĺ›cieĹĽkÄ™ â€“ bufor pikseli zostaje (redraw dalej dziaĹ‚a)
    void reset() { _lastPath[0] = '\0'; _lastMetaSig[0] = '\0'; }

    // WywoĹ‚uj tylko z loopTask (dostÄ™p do SD)
    void setTrack(const char *trackPath) {
        if (!trackPath) return;
        if (!player.isRunning()) {
            Serial.println("[SD_COVER] skip: player not running");
            return;
        }
#ifdef SD_COVER_SOURCE_LASTFM
        char metaSig[384];
        snprintf(metaSig, sizeof(metaSig), "%s|%s|%s",
                 config.station.title, config.station.sdArtist, config.station.sdAlbum);
        Serial.printf("[SD_COVER][CTX] path='%s' title='%s' artist='%s' album='%s'\n",
                      trackPath, config.station.title, config.station.sdArtist, config.station.sdAlbum);
        if (strcmp(trackPath, _lastPath) == 0 && strcmp(metaSig, _lastMetaSig) == 0) return;
        strlcpy(_lastPath, trackPath, sizeof(_lastPath));
        strlcpy(_lastMetaSig, metaSig, sizeof(_lastMetaSig));

        if (_drawEmbeddedMp3Cover(trackPath)) return;
        Serial.println("[SD_COVER] source=embedded fail");
        if (_drawLocalCover(trackPath)) return;
        Serial.println("[SD_COVER] source=local fail");

        char key[384];
        _buildLastFmKey(key, sizeof(key));
        if (config.station.sdArtist[0] == '\0' || config.station.sdAlbum[0] == '\0') {
            Serial.println("[SD_COVER][LastFM] skip: missing artist/album metadata");
            Serial.println("[SD_COVER] source=lastfm fail");
        } else if (key[0] != '\0') {
            if (_drawLastFm()) return;
            Serial.println("[SD_COVER] source=lastfm fail");
        } else {
            Serial.println("[SD_COVER] source=lastfm fail (missing metadata key)");
            Serial.printf("[SD_COVER][LastFM] artist='%s' album='%s' title='%s'\n",
                          config.station.sdArtist, config.station.sdAlbum, config.station.title);
        }

        if (_drawDefaultAlbum()) return;
        Serial.println("[SD_COVER] source=none");
        _freeBuf();
        redraw();
#else
        char metaSig[384];
        snprintf(metaSig, sizeof(metaSig), "%s|%s|%s",
                 config.station.title, config.station.sdArtist, config.station.sdAlbum);
        if (strcmp(trackPath, _lastPath) == 0 && strcmp(metaSig, _lastMetaSig) == 0) return;
        strlcpy(_lastPath, trackPath, sizeof(_lastPath));
        strlcpy(_lastMetaSig, metaSig, sizeof(_lastMetaSig));
        if (_drawLocalCover(trackPath)) return;
        Serial.println("[SD_COVER] source=local fail");
        if (_drawDefaultAlbum()) return;
        Serial.println("[SD_COVER] source=none");
        _freeBuf();
        redraw();
#endif
    }

    // Bezpieczne z DspTask â€“ tylko push pikseli, zero dostÄ™pu do SD
    void redraw() {
        if (_pixelBuf) {
            dsp.startWrite();
            dsp.setAddrWindow(SD_COVER_X, SD_COVER_Y, SD_COVER_W, SD_COVER_H);
            dsp.writePixels(_pixelBuf, SD_COVER_W * SD_COVER_H);
            dsp.endWrite();
        } else {
            dsp.fillRect(SD_COVER_X, SD_COVER_Y, SD_COVER_W, SD_COVER_H, _bgcolor);
        }
    }

    void clear() {
        if (_pixelBuf) { free(_pixelBuf); _pixelBuf = nullptr; }
        _lastPath[0] = '\0';
        _lastMetaSig[0] = '\0';
        _lastFmFailKey[0] = '\0';
        _lastFmFailUntilMs = 0;
        dsp.fillRect(SD_COVER_X, SD_COVER_Y, SD_COVER_W, SD_COVER_H, _bgcolor);
    }

private:
    uint16_t  _bgcolor   = 0;
    char      _lastPath[256] = {};
    char      _lastMetaSig[384] = {};
    char      _lastFmFailKey[384] = {};
    uint32_t  _lastFmFailUntilMs = 0;
    uint16_t *_pixelBuf  = nullptr;  // trwaĹ‚y bufor w PSRAM â€“ redraw bez SD

#ifdef SD_COVER_SOURCE_LASTFM
    static bool _isMp3Path(const char *path) {
        if (!path) return false;
        const char *dot = strrchr(path, '.');
        return dot && strcasecmp(dot, ".mp3") == 0;
    }

    static uint32_t _readSynchsafe28(const uint8_t *p) {
        return ((uint32_t)(p[0] & 0x7F) << 21) |
               ((uint32_t)(p[1] & 0x7F) << 14) |
               ((uint32_t)(p[2] & 0x7F) << 7)  |
               ((uint32_t)(p[3] & 0x7F));
    }

    static uint32_t _readFrameSize4(const uint8_t *p, bool synchsafe) {
        if (synchsafe) return _readSynchsafe28(p);
        return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
    }

    bool _drawEmbeddedMp3Cover(const char *trackPath) {
        if (!_isMp3Path(trackPath) || !sdman.ready) return false;
        File f = sdman.open(trackPath, "r");
        if (!f) return false;

        uint8_t hdr[10];
        if (f.read(hdr, sizeof(hdr)) != sizeof(hdr)) {
            f.close();
            return false;
        }
        if (memcmp(hdr, "ID3", 3) != 0) {
            f.close();
            return false;
        }

        const uint8_t id3Major = hdr[3];
        const bool id3v24 = (id3Major == 4);
        Serial.printf("[SD_COVER][ID3] version=2.%u\n", id3Major);
        uint32_t tagSize = _readSynchsafe28(hdr + 6);
        if (tagSize < 10) {
            f.close();
            return false;
        }

        uint32_t toRead = tagSize;
        if (toRead > 262144UL) toRead = 262144UL; // scan up to 256KB of ID3
        uint8_t *buf = (uint8_t*)ps_malloc(toRead);
        if (!buf) buf = (uint8_t*)malloc(toRead);
        if (!buf) {
            f.close();
            return false;
        }

        size_t got = f.read(buf, toRead);
        f.close();
        if (got < 10) {
            free(buf);
            return false;
        }

        bool decoded = false;
        uint32_t pos = 0;
        while (pos + 10 <= got) {
            const uint8_t *fr = &buf[pos];
            if (fr[0] == 0 && fr[1] == 0 && fr[2] == 0 && fr[3] == 0) break;
            uint32_t frameSize = _readFrameSize4(fr + 4, id3v24);
            if (frameSize == 0 || pos + 10 + frameSize > got) break;
            if (fr[0] == 'A' && fr[1] == 'P' && fr[2] == 'I' && fr[3] == 'C') {
                const uint8_t *payload = fr + 10;
                uint32_t left = frameSize;
                if (left > 4) {
                    uint32_t p = 0;
                    p++; // text encoding
                    while (p < left && payload[p] != 0) p++; // MIME
                    if (p + 2 < left) {
                        p++; // skip MIME terminator
                        p++; // picture type
                        while (p < left && payload[p] != 0) p++; // description (single-byte)
                        if (p + 1 < left) {
                            p++; // desc terminator
                            decoded = _decodeImageFromRamNoClear((uint8_t*)(payload + p), left - p);
                        }
                    }
                }
            } else if (fr[0] == 'P' && fr[1] == 'I' && fr[2] == 'C') {
                const uint8_t *payload = fr + 10;
                uint32_t left = frameSize;
                if (left > 7) {
                    uint32_t p = 0;
                    p++; // text encoding
                    p += 3; // image format
                    p++; // picture type
                    while (p < left && payload[p] != 0) p++; // description
                    if (p + 1 < left) {
                        p++;
                        decoded = _decodeImageFromRamNoClear((uint8_t*)(payload + p), left - p);
                    }
                }
            }
            if (decoded) {
                Serial.println("[SD_COVER][ID3] source=embedded");
                break;
            }
            pos += 10 + frameSize;
        }

        free(buf);
        if (!decoded) {
            Serial.println("[SD_COVER][ID3] source=embedded miss");
        }
        return decoded;
    }
#endif

    bool _decodeImageFromRamNoClear(uint8_t *imageBuf, size_t imageLen) {
        uint16_t *prev = _pixelBuf;
        _decodeImageFromRam(imageBuf, imageLen);
        return _pixelBuf && _pixelBuf != prev;
    }

    bool _decodeFileImageNoClear(const char *filePath) {
        if (!filePath || !sdman.ready) return false;
        File f = sdman.open(filePath, "r");
        if (!f) return false;
        size_t fsize = f.size();
        if (fsize < 4 || fsize > 500000) {
            f.close();
            return false;
        }

        uint8_t *buf = (uint8_t*)ps_malloc(fsize);
        if (!buf) buf = (uint8_t*)malloc(fsize);
        if (!buf) {
            f.close();
            return false;
        }
        bool ok = (f.read(buf, fsize) == fsize);
        f.close();
        if (!ok) {
            free(buf);
            return false;
        }

        bool decoded = _decodeImageFromRamNoClear(buf, fsize);
        free(buf);
        return decoded;
    }

    void _buildCoverPath(const char *trackPath, const char *name, char *out, size_t maxLen) {
        const char *slash = strrchr(trackPath, '/');
        size_t dirLen = slash ? (size_t)(slash - trackPath) : 0;
        size_t nameLen = strlen(name);
        if (dirLen >= maxLen - (nameLen + 2)) dirLen = maxLen - (nameLen + 3);
        memcpy(out, trackPath, dirLen);
        out[dirLen] = '\0';
        strlcat(out, "/", maxLen);
        strlcat(out, name, maxLen);
    }

    bool _drawLocalCover(const char *trackPath) {
        if (!trackPath || !sdman.ready) return false;
        const char *candidates[] = {
            "front.jpg", "front.png",
            "cover.jpg", "cover.png",
            "folder.jpg", "folder.png"
        };
        char coverPath[256];
        for (size_t i = 0; i < (sizeof(candidates) / sizeof(candidates[0])); i++) {
            _buildCoverPath(trackPath, candidates[i], coverPath, sizeof(coverPath));
            bool exists = sdman.exists(coverPath);
            Serial.printf("[SD_COVER][LOCAL] check path='%s' exists=%d\n", coverPath, exists ? 1 : 0);
            if (!exists) continue;
            if (_decodeFileImageNoClear(coverPath)) {
                Serial.printf("[SD_COVER] source=local file=%s path='%s'\n", candidates[i], coverPath);
                return true;
            }
            Serial.printf("[SD_COVER][LOCAL] decode fail path='%s'\n", coverPath);
        }
        return false;
    }

    bool _drawDefaultAlbum() {
        const char *candidates[] = {"/default_album.jpg", "/data/default_album.jpg"};
        for (size_t i = 0; i < (sizeof(candidates) / sizeof(candidates[0])); i++) {
            File f = SPIFFS.open(candidates[i], "r");
            if (!f) continue;
            size_t fsize = f.size();
            if (fsize < 4 || fsize > 500000) {
                f.close();
                continue;
            }
            uint8_t *jpegBuf = (uint8_t*)ps_malloc(fsize);
            if (!jpegBuf) jpegBuf = (uint8_t*)malloc(fsize);
            if (!jpegBuf) {
                f.close();
                continue;
            }
            bool ok = (f.read(jpegBuf, fsize) == fsize);
            f.close();
            if (!ok) {
                free(jpegBuf);
                continue;
            }
            _decodeJpegFromRam(jpegBuf, fsize);
            free(jpegBuf);
            if (_pixelBuf) {
                Serial.println("[SD_COVER] source=default");
                return true;
            }
        }
        return false;
    }

    bool _scaleSrcToCover(uint16_t *srcBuf, int srcW, int srcH) {
        if (!srcBuf || srcW <= 0 || srcH <= 0) return false;

        uint16_t *dst = (uint16_t*)ps_malloc((size_t)SD_COVER_W * SD_COVER_H * sizeof(uint16_t));
        if (!dst) dst = (uint16_t*)malloc((size_t)SD_COVER_W * SD_COVER_H * sizeof(uint16_t));
        if (!dst) return false;
        for (int i = 0; i < SD_COVER_W * SD_COVER_H; i++) dst[i] = _bgcolor;

        // Temporary: render to full cover area (no 200x200 cap).
        const int maxW = SD_COVER_W;
        const int maxH = SD_COVER_H;
        const int boxX = (SD_COVER_W - maxW) / 2;
        const int boxY = (SD_COVER_H - maxH) / 2;

        int drawW = maxW;
        int drawH = (int)((int64_t)srcH * maxW / srcW);
        if (drawH > maxH) {
            drawH = maxH;
            drawW = (int)((int64_t)srcW * maxH / srcH);
        }
        if (drawW < 1) drawW = 1;
        if (drawH < 1) drawH = 1;

        const int offX = boxX + (maxW - drawW) / 2;
        const int offY = boxY + (maxH - drawH) / 2;

        for (int y = 0; y < drawH; y++) {
            int sy = (int)((int64_t)y * srcH / drawH);
            if (sy >= srcH) sy = srcH - 1;
            for (int x = 0; x < drawW; x++) {
                int sx = (int)((int64_t)x * srcW / drawW);
                if (sx >= srcW) sx = srcW - 1;
                dst[(offY + y) * SD_COVER_W + (offX + x)] = srcBuf[sy * srcW + sx];
            }
        }

        _freeBuf();
        _pixelBuf = dst;
        return true;
    }

    void _decodeJpegFromRam(uint8_t *jpegBuf, size_t fsize) {
        if (!jpegBuf || fsize < 4 || fsize > 500000) { return; }

        JPEGDEC *jpeg = new JPEGDEC();
        bool decoded = false;
        if (jpeg && jpeg->openRAM(jpegBuf, (int)fsize, _sdCoverDrawCb)) {
            int srcW = jpeg->getWidth(), srcH = jpeg->getHeight();
            Serial.printf("[SD_COVER] src=jpeg %dx%d\n", srcW, srcH);
            if (srcW > 0 && srcH > 0 && srcW <= 800 && srcH <= 800) {
                _sdCoverSrcW = srcW;
                _sdCoverSrcH = srcH;
                _sdCoverSrcBuf = (uint16_t*)ps_malloc((size_t)srcW * srcH * sizeof(uint16_t));
                if (!_sdCoverSrcBuf) _sdCoverSrcBuf = (uint16_t*)malloc((size_t)srcW * srcH * sizeof(uint16_t));
            }
            jpeg->setPixelType(RGB565_LITTLE_ENDIAN);
            if (_sdCoverSrcBuf) {
                jpeg->decode(0, 0, 0);
                decoded = _scaleSrcToCover(_sdCoverSrcBuf, srcW, srcH);
            }
            jpeg->close();
        }
        delete jpeg;
        if (_sdCoverSrcBuf) { free(_sdCoverSrcBuf); _sdCoverSrcBuf = nullptr; }
        _sdCoverSrcW = 0;
        _sdCoverSrcH = 0;

        if (!decoded) { return; }
        redraw();
    }

#ifdef SD_COVER_SOURCE_LASTFM
    static bool _appendUrlEncoded(char *out, size_t maxLen, const char *src) {
        static const char hex[] = "0123456789ABCDEF";
        if (!out || !src || maxLen == 0) return false;
        size_t pos = strlen(out);
        while (*src) {
            unsigned char c = (unsigned char)*src++;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
                if (pos + 1 >= maxLen) return false;
                out[pos++] = (char)c;
            } else {
                if (pos + 3 >= maxLen) return false;
                out[pos++] = '%';
                out[pos++] = hex[c >> 4];
                out[pos++] = hex[c & 0x0F];
            }
        }
        out[pos] = '\0';
        return true;
    }

    void _buildLastFmKey(char *out, size_t maxLen) {
        if (!out || maxLen == 0) return;
        out[0] = '\0';

        if (config.station.sdArtist[0] == '\0' || config.station.sdAlbum[0] == '\0') return;
        snprintf(out, maxLen, "album:%s|%s", config.station.sdArtist, config.station.sdAlbum);
    }

    bool _isLastFmCooldownActive(const char *key) {
        if (!key || key[0] == '\0') return false;
        if (strcmp(key, _lastFmFailKey) != 0) return false;
        return (int32_t)(_lastFmFailUntilMs - millis()) > 0;
    }

    void _setLastFmCooldown(const char *key, uint32_t cooldownMs) {
        if (!key || key[0] == '\0') return;
        strlcpy(_lastFmFailKey, key, sizeof(_lastFmFailKey));
        _lastFmFailUntilMs = millis() + cooldownMs;
    }

    bool _lastFmApiUrl(char *url, size_t maxLen) {
        if (!url || maxLen == 0) return false;
        if (config.station.sdArtist[0] == '\0' || config.station.sdAlbum[0] == '\0') return false;
        strlcpy(url, "http://ws.audioscrobbler.com/2.0/?method=", maxLen);
        strlcat(url, "album.getinfo", maxLen);
        strlcat(url, "&api_key=", maxLen);
        strlcat(url, LASTFM_API_KEY, maxLen);
        strlcat(url, "&artist=", maxLen);
        if (!_appendUrlEncoded(url, maxLen, config.station.sdArtist)) return false;
        strlcat(url, "&album=", maxLen);
        if (!_appendUrlEncoded(url, maxLen, config.station.sdAlbum)) return false;
        strlcat(url, "&format=json", maxLen);
        return true;
    }

    bool _extractImageBySize(const char *json, const char *sizeName, char *out, size_t maxLen) {
        if (!json || !sizeName || !out || maxLen == 0) return false;
        out[0] = '\0';
        const char *p = json;
        while ((p = strstr(p, "\"size\"")) != nullptr) {
            const char *objStart = p;
            while (objStart > json && *objStart != '{') objStart--;
            const char *objEnd = strchr(p, '}');
            if (!objEnd) return false;

            char sizeNeedle[32];
            snprintf(sizeNeedle, sizeof(sizeNeedle), "\"%s\"", sizeName);
            const char *sizeHit = strstr(p, sizeNeedle);
            if (!sizeHit || sizeHit > objEnd) { p = objEnd + 1; continue; }

            const char *text = strstr(objStart, "\"#text\"");
            if (!text || text > objEnd) { p = objEnd + 1; continue; }
            const char *colon = strchr(text, ':');
            if (!colon || colon > objEnd) { p = objEnd + 1; continue; }
            const char *q1 = strchr(colon, '"');
            if (!q1 || q1 > objEnd) { p = objEnd + 1; continue; }

            size_t n = 0;
            for (const char *s = q1 + 1; *s && s < objEnd && n < maxLen - 1; s++) {
                if (*s == '"' && *(s - 1) != '\\') break;
                if (*s == '\\' && *(s + 1) == '/') continue;
                out[n++] = *s;
            }
            out[n] = '\0';
            return n > 0;
        }
        return false;
    }

    bool _extractBestImage(const char *json, char *out, size_t maxLen) {
        if (_extractImageBySize(json, "mega", out, maxLen)) return true;
        if (_extractImageBySize(json, "extralarge", out, maxLen)) return true;
        if (_extractImageBySize(json, "large", out, maxLen)) return true;
        if (_extractImageBySize(json, "medium", out, maxLen)) return true;
        return _extractImageBySize(json, "small", out, maxLen);
    }

    bool _httpGetToBuffer(const char *url, uint8_t **buf, size_t *len, size_t maxBytes) {
        if (!url || !buf || !len) return false;
        *buf = nullptr;
        *len = 0;

        HTTPClient http;
        WiFiClient plainClient;
        WiFiClientSecure secureClient;
        bool isHttps = strncmp(url, "https://", 8) == 0;
        if (isHttps) secureClient.setInsecure();
        bool ok = isHttps ? http.begin(secureClient, url) : http.begin(plainClient, url);
        if (!ok) return false;
        http.setTimeout(LASTFM_COVER_TIMEOUT_MS);

        int code = http.GET();
        if (code != HTTP_CODE_OK) {
            http.end();
            return false;
        }

        int contentLen = http.getSize();
        if (contentLen <= 0 || (size_t)contentLen > maxBytes) {
            http.end();
            return false;
        }

        uint8_t *tmp = (uint8_t*)ps_malloc(contentLen + 1);
        if (!tmp) tmp = (uint8_t*)malloc(contentLen + 1);
        if (!tmp) {
            http.end();
            return false;
        }

        WiFiClient *stream = http.getStreamPtr();
        size_t got = 0;
        uint32_t lastDataMs = millis();
        while (got < (size_t)contentLen && (millis() - lastDataMs) < LASTFM_COVER_TIMEOUT_MS) {
            int avail = stream->available();
            if (avail <= 0) {
                delay(1);
                continue;
            }
            size_t want = min((size_t)avail, (size_t)contentLen - got);
            int n = stream->read(tmp + got, want);
            if (n > 0) {
                got += (size_t)n;
                lastDataMs = millis();
            } else {
                delay(1);
            }
        }
        http.end();
        if (got != (size_t)contentLen) {
            free(tmp);
            return false;
        }

        tmp[got] = 0;
        *buf = tmp;
        *len = got;
        return true;
    }

    bool _isJpeg(const uint8_t *buf, size_t len) {
        return buf && len >= 2 && buf[0] == 0xFF && buf[1] == 0xD8;
    }

    bool _isPng(const uint8_t *buf, size_t len) {
        static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
        return buf && len >= sizeof(sig) && memcmp(buf, sig, sizeof(sig)) == 0;
    }

    bool _isProgressiveJpeg(const uint8_t *buf, size_t len) {
        if (!_isJpeg(buf, len)) return false;
        size_t p = 2;
        while (p + 3 < len) {
            if (buf[p] != 0xFF) { p++; continue; }
            while (p < len && buf[p] == 0xFF) p++;
            if (p >= len) break;
            uint8_t marker = buf[p++];
            if (marker == 0xC2) return true;
            if (marker == 0xC0) return false;
            if (marker == 0xD8 || marker == 0xD9 || (marker >= 0xD0 && marker <= 0xD7)) continue;
            if (p + 1 >= len) break;
            uint16_t segLen = ((uint16_t)buf[p] << 8) | buf[p + 1];
            if (segLen < 2) break;
            p += segLen;
        }
        return false;
    }

    bool _pngVariantUrl(const char *url, char *out, size_t maxLen) {
        if (!url || !out || maxLen == 0) return false;
        size_t len = strlen(url);
        if (len < 4 || len + 1 > maxLen) return false;
        const char *ext = url + len - 4;
        if (strcasecmp(ext, ".jpg") != 0) return false;
        strlcpy(out, url, maxLen);
        strlcpy(out + len - 4, ".png", maxLen - (len - 4));
        return true;
    }

    void _decodePngFromRam(uint8_t *pngBuf, size_t fsize) {
        if (!pngBuf || fsize < 8 || fsize > 500000) { return; }

        PNG *png = new PNG();
        bool decoded = false;
        if (png && png->openRAM(pngBuf, (int)fsize, _sdCoverPngDrawCb) == PNG_SUCCESS) {
            int srcW = png->getWidth(), srcH = png->getHeight();
            Serial.printf("[SD_COVER] src=png %dx%d\n", srcW, srcH);
            if (srcW > 0 && srcH > 0 && srcW <= 800 && srcH <= 800) {
                _sdCoverSrcW = srcW;
                _sdCoverSrcH = srcH;
                _sdCoverSrcBuf = (uint16_t*)ps_malloc((size_t)srcW * srcH * sizeof(uint16_t));
                if (!_sdCoverSrcBuf) _sdCoverSrcBuf = (uint16_t*)malloc((size_t)srcW * srcH * sizeof(uint16_t));
                _sdCoverPngLine = (uint16_t*)malloc((size_t)srcW * sizeof(uint16_t));
            }
            if (_sdCoverSrcBuf && _sdCoverPngLine) {
                _sdCoverPng = png;
                png->decode(NULL, 0);
                _sdCoverPng = nullptr;
                decoded = _scaleSrcToCover(_sdCoverSrcBuf, srcW, srcH);
            }
            png->close();
        }
        delete png;
        if (_sdCoverPngLine) { free(_sdCoverPngLine); _sdCoverPngLine = nullptr; }
        if (_sdCoverSrcBuf) { free(_sdCoverSrcBuf); _sdCoverSrcBuf = nullptr; }
        _sdCoverPng = nullptr;
        _sdCoverSrcW = 0;
        _sdCoverSrcH = 0;

        if (!decoded) { return; }
        redraw();
    }

    void _decodeImageFromRam(uint8_t *imageBuf, size_t imageLen) {
        if (_isJpeg(imageBuf, imageLen)) {
            // JPEGDEC used here cannot decode progressive JPEG (SOF2).
            // Reject centrally for every source (embedded/local/LastFM).
            if (_isProgressiveJpeg(imageBuf, imageLen)) {
                Serial.println("[SD_COVER] skip: progressive JPEG unsupported");
                return;
            }
            _decodeJpegFromRam(imageBuf, imageLen);
        } else if (_isPng(imageBuf, imageLen)) {
            _decodePngFromRam(imageBuf, imageLen);
        }
    }

    bool _drawLastFm() {
        if (strlen(LASTFM_API_KEY) < 8) {
            Serial.println("[SD_COVER][LastFM] Missing/invalid API key, fallback to local cover");
            return false;
        }

        char *apiUrl = (char*)malloc(512);
        if (!apiUrl) { return false; }
        if (!_lastFmApiUrl(apiUrl, 512)) { free(apiUrl); return false; }

        uint8_t *jsonBuf = nullptr;
        size_t jsonLen = 0;
        bool httpOk = _httpGetToBuffer(apiUrl, &jsonBuf, &jsonLen, 30000);
        free(apiUrl);
        if (!httpOk) {
            Serial.println("[SD_COVER][LastFM] Metadata request failed, fallback to local cover");
            return false;
        }

        char *imageUrl = (char*)malloc(384);
        if (!imageUrl) { free(jsonBuf); return false; }
        bool found = _extractBestImage((const char*)jsonBuf, imageUrl, 384);
        free(jsonBuf);
        if (!found) {
            Serial.println("[SD_COVER][LastFM] Cover not found, fallback to local/default cover");
            free(imageUrl);
            return false;
        }

        uint8_t *imageBuf = nullptr;
        size_t imageLen = 0;
        if (!_httpGetToBuffer(imageUrl, &imageBuf, &imageLen, 500000)) {
            Serial.println("[SD_COVER][LastFM] Image download failed, fallback to local cover");
            free(imageUrl);
            return false;
        }

        if (_isProgressiveJpeg(imageBuf, imageLen)) {
            free(imageBuf);
            imageBuf = nullptr;
            imageLen = 0;
            char *pngUrl = (char*)malloc(384);
            if (pngUrl && _pngVariantUrl(imageUrl, pngUrl, 384)) {
                bool ok = _httpGetToBuffer(pngUrl, &imageBuf, &imageLen, 500000);
                free(pngUrl);
                if (!ok) {
                    Serial.println("[SD_COVER][LastFM] Progressive JPEG and PNG fallback failed, fallback to local cover");
                    free(imageUrl);
                    return false;
                }
            } else {
                free(pngUrl);
                Serial.println("[SD_COVER][LastFM] Progressive JPEG but PNG variant URL unavailable, fallback to local cover");
                free(imageUrl);
                return false;
            }
        }

        free(imageUrl);
        _decodeImageFromRam(imageBuf, imageLen);
        free(imageBuf);
        if (_pixelBuf) {
            Serial.println("[SD_COVER] source=lastfm");
            return true;
        }
        return false;
    }
#endif

    void _freeBuf() {
        if (_pixelBuf) { free(_pixelBuf); _pixelBuf = nullptr; }
    }
};

#endif // SD_COVER_ART && SDC_CS != 255


