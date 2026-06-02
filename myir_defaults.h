#ifndef MYIR_DEFAULTS_H
#define MYIR_DEFAULTS_H

/* Optional built-in IR mapping (restored automatically when IR EEPROM is empty). */
/* Set to true to prefill IR commands after a fresh flash/reset of IR section. */
#define IR_DEFAULT_MAP_ENABLED true

/*
  Put raw IR values captured by yoRadio (IR Recorder / serial) below.
  Leave as 0 to skip a given action.
*/
#define IR_DEFAULT_POWER 	33731149ULL
#define IR_DEFAULT_MUTE 	33735229ULL
#define IR_DEFAULT_VOL_UP 	33691879ULL
#define IR_DEFAULT_VOL_DOWN 33744919ULL
#define IR_DEFAULT_UP 	33738799ULL
#define IR_DEFAULT_PREV 33687799ULL
#define IR_DEFAULT_PLAY 33730639ULL
#define IR_DEFAULT_NEXT 33720439ULL
#define IR_DEFAULT_DOWN 33714319ULL
#define IR_DEFAULT_HOME 33708199ULL
#define IR_DEFAULT_MENU 33727069ULL
#define IR_DEFAULT_BACK 33702589ULL
#define IR_DEFAULT_1     33718399ULL
#define IR_DEFAULT_2     33702079ULL
#define IR_DEFAULT_3     33734719ULL
#define IR_DEFAULT_4     33693919ULL
#define IR_DEFAULT_5     33726559ULL
#define IR_DEFAULT_6     33710239ULL
#define IR_DEFAULT_7     33742879ULL
#define IR_DEFAULT_8     33689839ULL
#define IR_DEFAULT_9     33722479ULL
#define IR_DEFAULT_AST   33743389ULL
#define IR_DEFAULT_0     33685759ULL
#define IR_DEFAULT_HASH  33697999ULL

#endif
