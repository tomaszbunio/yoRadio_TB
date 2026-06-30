#ifndef CLOCK_TTS_H
#define CLOCK_TTS_H

void clock_tts_setup();
void clock_tts_loop();
void clock_tts_force(const char* text, const char* lang = nullptr);

void clock_tts_set_language(const char* lang);
void clock_tts_set_interval(int minutes);
void clock_tts_enable(bool enable);

#endif
