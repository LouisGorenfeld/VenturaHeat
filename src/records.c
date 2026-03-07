#include <stdio.h>
#include <stdlib.h>
#include "records.h"

#define KEY "roadv3projectvhjam2025"
#define RECFILENAME "vheat.rec"

void shift(char *data, int len, int unshift) 
{
    char *p = KEY;
    for (int i=0; i < len; i++)
    {
        if (unshift)
            data[i] -= *p;
        else 
            data[i] += *p;
        p++;
        if (*p == '\0') {
            p = KEY;
        }
    }
}

void refreshRecords(gameState_t *state)
{
    for (int i=0; i <  MAX_STAGES; i++)
    {
        if (state->bestLaptimes[i].time && state->bestLaptimes[i].time > state->laptimes[i]) {
            state->bestLaptimes[i].time = state->laptimes[i];

            // TODO: Let the player (PLR :)) enter their own initials
            state->bestLaptimes[i].initials[0] = 'P';
            state->bestLaptimes[i].initials[1] = 'L';
            state->bestLaptimes[i].initials[2] = 'R';
        }
    }
}

void loadRecords(gameState_t *state)
{
    FILE *fp = fopen(RECFILENAME, "rb");
    if (fp) 
    {
        if (fgetc(fp) != 'V' || fgetc(fp) != '0') {
            fclose(fp);
            return;
        }

        fread(state->bestLaptimes, sizeof(lapRecord_t),  MAX_STAGES, fp);
        fread(&state->bestTotalTime, sizeof(lapRecord_t),  1, fp);

        fclose(fp);
    }
    else // Initialize records
    {
        for (int i=0; i < MAX_STAGES; i++) {
            state->bestLaptimes[i].time = 60*70;
            state->bestLaptimes[i].initials[0] = 'L';
            state->bestLaptimes[i].initials[1] = 'O';
            state->bestLaptimes[i].initials[2] = 'U';
        }

        state->bestTotalTime.time = 60*70* MAX_STAGES;
        state->bestTotalTime.initials[0] = 'L';
        state->bestTotalTime.initials[1] = 'O';
        state->bestTotalTime.initials[2] = 'U';
    }
}

void writeRecord(lapRecord_t *rec, FILE *fp)
{
    fwrite(&rec->time, sizeof(int), 1, fp);
    fwrite(rec->initials, 1, 3, fp);
}

void saveRecords(gameState_t *state)
{
    FILE *fp = fopen(RECFILENAME, "wb");
    if (fp) 
    {
        fputc('V', fp);
        fputc('0', fp);
        for (int i=0; i < MAX_STAGES; i++) {
            writeRecord(&state->bestLaptimes[i], fp);
        }
        writeRecord(&state->bestTotalTime, fp);
        fflush(fp);
        fclose(fp);    
    }
}
