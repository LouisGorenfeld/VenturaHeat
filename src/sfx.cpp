#include "player20.h"
#include "sfx.h"

extern RADPlayer gRadPlayer;
extern FILE *gLogFP;


int isTriggerRequested(sfxState_t *sfx, sfxNum_t num) {
    return ((sfx->triggerState & (1<<num)) && ! (sfx->lastTriggerState & (1<<num))); 
}

void ackTrigger(sfxState_t *sfx, sfxNum_t num) {
    sfx->triggerState &= ~(1<<num);
    fprintf(gLogFP, "ackTrigger: %d 0x%x\n", num, sfx->triggerState);
}

void triggerSfx(sfxState_t *sfx, sfxNum_t num) {
    sfx->triggerState |= (1<<num);
    fprintf(gLogFP, "triggerSfx: %d 0x%x\n", num, sfx->triggerState);
}

void setSfxFrequency(sfxState_t *sfx, sfxNum_t num, int freq) {
  gRadPlayer.SetFrequencyDirect(8, freq);
}

void tickSfx(sfxState_t *sfx) {
    // Channel 9
    if (isTriggerRequested(sfx, kSFX_Skid)) {
        ackTrigger(sfx, kSFX_Skid);
        gRadPlayer.PlayNote(8, 2, 6, 2);
    }
    else if (isTriggerRequested(sfx, kSFX_Engine)) {
        // Do not ack; engine stays on
        gRadPlayer.PlayNote(8, 2, 0, 2);
    }

    // Channel 8
    if (isTriggerRequested(sfx, kSFX_Crash)) {
        ackTrigger(sfx, kSFX_Crash);
        gRadPlayer.PlayNote(7, 1, 3, 3);
    }

    sfx->lastTriggerState = sfx->triggerState;
}
