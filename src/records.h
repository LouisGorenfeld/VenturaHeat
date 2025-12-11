#ifndef RECORD_H
#define RECORD_H

#include "game.h"

void loadRecords(gameState_t *state);
void refreshRecords(gameState_t *state);    // Call after each game
void saveRecords(gameState_t *state);

#endif
