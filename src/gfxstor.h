#ifndef GFXSTOR_H
#define GFXSTOR_H

#include "loadpcx.h"

// The images for the various facings of the car. If no left or right is
// specified, fall back on the center image (enforced by getCarTex)
typedef enum carGfxFacing_t
{
  CARGFX_LEFT = 0,
  CARGFX_CENTER,
  CARGFX_RIGHT,
  CARGFX_MAX
} carGfxFacing_t;

typedef struct carGfx_t
{
  pcxImage_t images[CARGFX_MAX];
} carGfx_t;

void loadGfx();
pcxImage_t *getCarTex(int index, carGfxFacing_t facing);
pcxImage_t *getScenTex(int index);
pcxImage_t *getSideTex(int index);
pcxImage_t *getRoadTex(int index);
pcxImage_t *getHorizonTex(int index);

#endif

