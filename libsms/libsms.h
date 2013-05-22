#ifndef LIBSMS_H
#define LIBSMS_H

#include "libsms_global.h"

#define SMS_PORT_1 0
#define SMS_PORT_2 1

#define	SMS_DEVICE_NONE 0
#define SMS_DEVICE_JOYPAD 1
#define SMS_DEVICE_LIGHTGUN 2
#define SMS_DEVICE_PADDLE 3     //fixed range knob, no spinner
#define SMS_DEVICE_SPORTS_PAD 4 //trackball (360 degree spinner)
#define SMS_DEVICE_TEREBI 5     //Terebi Oekaki lightpen
#define SMS_DEVICE_MISC 6       //used for input pause


#define SMS_INPUT_UP 0
#define SMS_INPUT_DOWN 1
#define SMS_INPUT_LEFT 2
#define SMS_INPUT_RIGHT 3
#define SMS_INPUT_A 4
#define SMS_INPUT_B 5
#define SMS_INPUT_START 6       //game gear only
#define SMS_INPUT_X 7           //Lightgun, Paddle, Sports Pad
#define SMS_INPUT_Y 8           //Lightgun, Sports Pad
#define SMS_INPUT_TRIGGER 9     //Lightgun
#define SMS_INPUT_PAUSE 10      //sms, sg1000  pause button is on unit

#define SMS_MODE_64_COLOR 0     //SMS using mode 4
#define SMS_MODE_16_COLOR 1     //SMS using mode (0-3), means mainly backward compatibility to SG-1000 or legacy mode games like F-16 and a lot of korean games
#define SMS_MODE_16_COLOR_TMS 2 //SG-1000
#define SMS_MODE_4096_COLOR_GG 3//Game Gear

#define SMS_REGION_USA 0        //ntsc mode
#define SMS_REGION_JAPAN 1      //ntsc mode
#define SMS_REGION_EUROPE 2     //pal mode
#define SMS_REGION_AUTO 3       //not much reliable, there is no pal/ntsc detection possible by reading rom header

#define SMS_MEMORY_WRAM 0       //8k on SMS and Game Gear   |   1k on SG-1000
#define SMS_MEMORY_VRAM 1       //16k
#define SMS_MEMORY_SRAM 2       //up to 32k sram

#define SMS_REVISION_1 0        //there are some japanese sms games, which doesn't run on a SMS 2 unit
#define SMS_REVISION_2 1        //should be best option

#define GG_MODE_NORMAL 0
#define GG_MODE_SMS 1           //is for running sms games on the game gear
#define GG_MODE_GEAR_TO_GEAR 2  //emulates 2 game gears at the same time for 2 player mode

#define SMS_SOUND_SN76489 0 //default for sms, gg and sg (44100 Hz)
#define SMS_SOUND_YM2413 1  //FM sound (49716 Hz) note: runs in parallel to SN76489
#define GG_SOUND_SN76489 2 //second game gear in gear-to-gear mode (44100 Hz)

typedef void (*smsAudioSampleCallback)(signed sampleLeft, signed sampleRight, unsigned soundChip);
typedef void (*smsVideoRefreshCallback)(const unsigned short* frame, unsigned width, unsigned height, unsigned modeId, bool secondGG);
typedef void (*smsInputPollCallback)(void);
typedef signed (*smsInputStateCallback)(unsigned port, unsigned deviceId, unsigned objectId);

#ifdef __cplusplus
extern "C" {
#endif
    
LIBSMSSHARED_EXPORT unsigned smsLibraryId(void); //is incremented with each new release

//callbacks have to be implemented
LIBSMSSHARED_EXPORT void smsSetAudioSample(smsAudioSampleCallback); //a new audio sample is generated, calls more times during a frame
LIBSMSSHARED_EXPORT void smsSetVideoRefresh(smsVideoRefreshCallback); //a frame is completly calculated
LIBSMSSHARED_EXPORT void smsSetInputPoll(smsInputPollCallback);     //fetch user input
LIBSMSSHARED_EXPORT void smsSetInputState(smsInputStateCallback);   //calls for the input state of a device object

LIBSMSSHARED_EXPORT void smsSetDevice(bool port, unsigned deviceId); //plugin controller, lightgun, paddle ...

LIBSMSSHARED_EXPORT void smsPower(void); //hard reset
LIBSMSSHARED_EXPORT void smsReset(void); //only usefull by emulating a SMS 1 or SG-1000 unit, otherwise same as power
LIBSMSSHARED_EXPORT void smsRun(void); //emulates one frame

//the SMS displays SG-1000 games with other colors than the the original SG-1000 it does.
LIBSMSSHARED_EXPORT void smsLoad(const unsigned char* rom, unsigned romSize); //load SMS or SG-1000
LIBSMSSHARED_EXPORT void smsLoadSG1000(const unsigned char* rom, unsigned romSize); //load SG-1000 games
LIBSMSSHARED_EXPORT void smsLoadGameGear(const unsigned char* rom, unsigned romSize, unsigned ggMode); //load game gear or SMS games
LIBSMSSHARED_EXPORT void smsLoadBios(const unsigned char* bios, unsigned biosSize); //load bios file for SMS or GG
//note: a bios load works only in combination with one of the previous load functions, so first load the game
//      if you want to load the built-in game of a bios, load the bios as game

LIBSMSSHARED_EXPORT void smsUnload(void);

LIBSMSSHARED_EXPORT void smsSetRegion(unsigned regionId);
LIBSMSSHARED_EXPORT unsigned smsGetRegion(void);    //makes sense, if region is auto selected
LIBSMSSHARED_EXPORT void smsSetRevision(unsigned revisionId);  //only affect SMS emulation
LIBSMSSHARED_EXPORT void smsDisable3D(bool state);  //makes 3D games playable without glasses

LIBSMSSHARED_EXPORT unsigned char* smsGetMemoryData(unsigned memoryId); //get memory, e.g. sram data
LIBSMSSHARED_EXPORT unsigned smsGetMemorySize(unsigned memoryId);  //gets size of a memory area, e.g. sram size of the loaded game

LIBSMSSHARED_EXPORT void smsResetCheat(void); //remove all cheats, call it when cheat list changes and rebuild
LIBSMSSHARED_EXPORT bool smsSetCheat(const char* code); //add a cheat code

LIBSMSSHARED_EXPORT unsigned smsSavestateSize(void); //get size needed for a new save state
LIBSMSSHARED_EXPORT unsigned long long smsSavestateTimeStamp(unsigned char* data); //get TimeStamp of a save state
LIBSMSSHARED_EXPORT bool smsSaveState(unsigned char* data, unsigned size); //save the state of the game
LIBSMSSHARED_EXPORT bool smsLoadState(unsigned char* data, unsigned size); //resume game from a save state

//games don't depend on the yamaha chip
LIBSMSSHARED_EXPORT void smsEnableYamahaSoundChip(bool state); //about 60 japanese sms games support this
LIBSMSSHARED_EXPORT void smsDisableBorder(bool state); //don't let the game color the border and reduce the border to a minimum but keep aspect ratio
    
#ifdef __cplusplus
}
#endif

#endif // LIBSMS_H
