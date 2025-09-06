#define MAXCARS 10
#define MAXSIDETEX 10
#define MAXROADTEX 10
#define MAXHORIZTEX 10

#include <string.h>
#include "gfxstor.h"

typedef struct gfxStore_t
{
  carGfx_t carGfx[MAXCARS];
  pcxImage_t sideTex[MAXSIDETEX];
  pcxImage_t roadTex[MAXROADTEX];
  pcxImage_t horizonTex[MAXHORIZTEX];
} gfxStore_t;

gfxStore_t gGfxStore;


void loadGfxInternal(gfxStore_t *stor)
{
  carGfx_t *cg;
  memset(&gGfxStore, 0, sizeof(gfxStore_t));


  // Road Textures
  loadPCXFile(stor->roadTex + 0, "data/roadtex3.pcx");
  loadPCXFile(stor->roadTex + 1, "data/iceroad.pcx");



  // Side Textures
  loadPCXFile(stor->sideTex + 0, "data/grastex1.pcx");



  // Car (road object) Images
  cg = stor->carGfx + 0;
  loadPCXFile(&cg->images[CARGFX_LEFT],  "data/lchev.pcx");
  loadPCXFile(&cg->images[CARGFX_RIGHT], "data/rchev.pcx");

  cg = stor->carGfx + 1;
  loadPCXFile(&cg->images[CARGFX_CENTER], "data/coupe1.pcx");

  cg = stor->carGfx + 2;
  loadPCXFile(&cg->images[CARGFX_CENTER], "data/tree.pcx");



  // Horizon Textures
  loadPCXFile(stor->horizonTex + 0, "data/horicity.pcx");
}

void loadGfx()
{
  loadGfxInternal(&gGfxStore);
}


pcxImage_t *getCarTex(int index, carGfxFacing_t facing)
{
  carGfx_t *cg = gGfxStore.carGfx + index;
  if (cg->images[facing].pixels == 0)
  {
    if (facing == CARGFX_LEFT || facing == CARGFX_RIGHT)
      return &cg->images[CARGFX_CENTER];
  }
  
  return &cg->images[facing];
}

pcxImage_t *getSideTex(int index)
{
  return &gGfxStore.sideTex[index];
}

pcxImage_t *getRoadTex(int index)
{
  return &gGfxStore.roadTex[index];
}

pcxImage_t *getHorizonTex(int index)
{
  return &gGfxStore.horizonTex[index];
}
