#include "player20.h"
#include "sfx.h"

extern RADPlayer gRadPlayer;

int triggerRequested(sfxState_t *sfx, sfxNum_t num) {
    return (sfx->triggerState & (1<<num) && ! sfx->lastTriggerState & (1<<num)); 
}

int ackTrigger(sfxState_t *sfx, sfxNum_t num) {
    return sfx->triggerState & ~(1<<num);
}

void triggerSfx(sfxState_t *sfx, sfxNum_t num) {
    sfx->triggerState |= (1<<num);
}

void setSfxFrequency(sfxState_t *sfx, sfxNum_t num, int freq) {
  gRadPlayer.SetFrequencyDirect(8, freq);
}

void tickSfx(sfxState_t *sfx) {
    // Channel 9
    if (triggerRequested(sfx, kSFX_Skid)) {
        ackTrigger(sfx, kSFX_Skid);
    }
    else if (triggerRequested(sfx, kSFX_Engine)) {
        // Do not ack; engine stays on
        gRadPlayer.PlayNote(8, 2, 0, 2);
    }

    // Channel 8
    if (triggerRequested(sfx, kSFX_Crash)) {
        gRadPlayer.PlayNote(7, 0, 4, 4);
        ackTrigger(sfx, kSFX_Crash);
    }

    sfx->lastTriggerState = sfx->triggerState;
}
