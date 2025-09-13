#ifndef _SFX_H_
#define _SFX_H_

typedef enum sfxNum_t {
    kSFX_Engine = 0,
    kSFX_Crash,
    kSFX_Skid,
    kNumSFX
} sfxNum_t;

typedef struct sfxState_t  {
    unsigned short triggerState;
    unsigned short lastTriggerState;
} sfxState_t;


void triggerSfx(sfxState_t *sfx, sfxNum_t num);

void setSfxFrequency(sfxState_t *sfx, sfxNum_t num, int freq);

void tickSfx(sfxState_t *sfx);


#endif
