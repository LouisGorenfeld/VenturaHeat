#define MAXCARS 10
#define MAXSIDETEX 10
#define MAXROADTEX 10
#define MAXHORIZTEX 10

#include <string.h>
#include "gfxstor.h"
#include "data.h"

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
  loadPCX(stor->roadTex + 0, data_roadtex3);
  loadPCX(stor->roadTex + 1, data_iceroad);
  loadPCX(stor->roadTex + 2, data_chektex);



  // Side Textures
  loadPCX(stor->sideTex + 0, data_grastex1);
  loadPCX(stor->sideTex + 1, data_snowtex1);
  loadPCX(stor->sideTex + 2, data_beactex1);
  loadPCX(stor->sideTex + 3, data_beactex2);
  loadPCX(stor->sideTex + 4, data_mudtex1);



  // Car (road object) Images
  cg = stor->carGfx + 0;
  loadPCX(&cg->images[CARGFX_LEFT],  data_lchev);
  loadPCX(&cg->images[CARGFX_RIGHT], data_rchev);

  cg = stor->carGfx + 1;
  loadPCX(&cg->images[CARGFX_CENTER], data_coupe1);

  cg = stor->carGfx + 2;
  loadPCX(&cg->images[CARGFX_CENTER], data_tree);

//  cg = stor->carGfx + 3;
//  loadPCX(&cg->images[CARGFX_LEFT],  "data_lcheckp.pcx");
//  loadPCX(&cg->images[CARGFX_RIGHT], "data_rcheckp.pcx");
//  loadPCX(&cg->images[CARGFX_RIGHT], data_checkp.pcx");

  cg = stor->carGfx + 4;
  loadPCX(&cg->images[CARGFX_CENTER], data_coupe2);


  // Horizon Textures
  loadPCX(stor->horizonTex + 0, data_horicity);
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
