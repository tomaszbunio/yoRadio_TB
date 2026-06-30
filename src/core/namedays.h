#ifndef NAMEDAYS_H
#define NAMEDAYS_H

#include "options.h"

#ifdef NAMEDAYS_FILE

#include <Arduino.h>

extern const char* namedays[];

extern const char* nameday_label;

extern uint32_t namedayLastRotation;
extern uint8_t  namedayCurrentIndex;
extern char     currentNamedayBuffer[30];
extern int      lastNamedayDay;
extern int      lastNamedayMonth;

const char *getNameDay(int month, int day);

#endif // NAMEDAYS_FILE

#endif // NAMEDAYS_H
