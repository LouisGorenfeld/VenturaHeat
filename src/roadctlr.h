#ifndef ROADCTLR_H
#define ROADCTLR_H

// RoadController: Responsible for housing and positioning
// the road and adding objects, based upon a script.
// Controlled by Game.

// Road description language (null-terminated string)
// --------------------------------------------------
// l/L light/medium left
// r/R light/medium right
// h/H hairpin left/right
// u/U gentle/steep undulation (toggle uphill, toggle downhill)
// 2/3/4/i set number of lanes/island (moves the two roads inward/outward)
// F(RX)   for road R, toggle feature X (0=lcliff, 1=rcliff, 2=tunnel)
// T(SX)   set texture X for side S (0=left, 1=road, 2=right)
// t(SXY)   same, but briefly (for finish lines, crossroads, etc). Y specifies the
//         texture it goes to after displaying one iteration of the X texture
// G(SYX)  change generator for side S to style Y spitting out object X
//         Styles: (0=sparse, 1=dense, 2=horizontal rows, 3=lining road)
// O(RLX)  generate single object X in lane L on road R
// B(AB)   branch roads into commandset A for left road, commandset B for right
//         roads rejoin when the active road's list runs off the end
//
// ,       End command and wait for the next segment to continue parsing

#include "cars.h"
#include "road.h"
#include "gfxstor.h"

#define MAXCARS 128

// The range where objects are dropped off the map
// 8192: Two "screens" ahead (0-4096 being "in front")
#define ZFARCULL 8192 
// -4096: One "screen" behind
#define ZNEARCULL -4096

/*
These values look great without a DX or DY maximum
#define LIGHTCURVE 2000
#define MEDCURVE 3000
#define SHARPCURVE 4000
#define LIGHTHILL 500
#define MEDHILL 1500
#define SHARPHILL 2500
*/

/*
Prior to moving the z position where it cuves
#define LIGHTCURVE 3000
#define MEDCURVE 4000
#define SHARPCURVE 5000
#define LIGHTHILL 500
#define MEDHILL 1500
#define SHARPHILL 2500
*/

#define MAX_STAGES 6

typedef struct roadStage_t
{
  char *cmd;
  pcxImage_t *horizonTex;
} roadStage_t;

typedef struct roadCtlr_t
{
  char *parsePos;    // Pointer into the stage's cmd string

  roadDef_t roadDef;
  roadLive_t roadLive;
  roadStage_t stage[MAX_STAGES];
  int curStage; // TODO: Maybe this goes into gamestate

  car_t cars[MAXCARS];
  int carActive[MAXCARS];

  short zCounterScenery; // Z until next scenery generation
  short zTrafficDelta;   // Z until next traffic generation

} roadCtlr_t;

typedef enum texLoc_t
{
  TEXLOC_LEFT = 0,
  TEXLOC_ROAD,
  TEXLOC_RIGHT
} texLoc_t;

// Positive scrolls towards camera. Returns true if next stage has been loaded
int roadCtlScroll(roadCtlr_t *ctlr, short zAmt);
void roadCtlCarLogic(roadCtlr_t *ctlr);
void roadCtlDraw(roadCtlr_t *ctlr, unsigned char *dest);
void roadCtlInit(roadCtlr_t *ctlr);
// Direct set texture call
void roadCtlSetTexture(roadCtlr_t *ctlr, texLoc_t loc, pcxImage_t *img);
void roadCtlrAddRandomizedCar(roadCtlr_t *ctlr, int rank);

// Returns the car which has been allocated
// texNum is only used for adding the scenery object
car_t *roadCtlAddCar(roadCtlr_t *ctlr, fixed_t x, int z, fixed_t speed, carType_t type, int texNum);
void roadCtlRemoveCar(roadCtlr_t *ctlr, car_t *car);

#endif
