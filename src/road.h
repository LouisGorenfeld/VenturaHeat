#ifndef ROAD_H
#define ROAD_H


#include "lmath.h"
#include "loadpcx.h"

// Road: A single road with the variables needed to draw and animate it,
// as well as the sides (which can be toggled on or off)
// Each road also has its own object list and is responsible for maintaining
// and drawing it.

// Controlled by RoadCtlr

// Range and size of the ZMap
// (please do not tamper or you will create math problems)
#define ZMAPBITS 12
//#define ZMAXDRAW 4096
#define ZMAPLEN 100

// The drawing range
// BUG: If FARPLANE isn't where i==0,
// sprite culling crashes (maybe sprites not getting their sy assigned?)
#define ZFARPLANE 4096
// NEARPLANE should approximately be the closest line's z value
#define ZNEARPLANE 40

typedef struct car_t car_t;

// Road object for the display list system
// An enemy/car/etc definition allocated elsewhere *has* a roadObj
typedef struct roadObj_t roadObj_t;
struct roadObj_t
{  
  car_t *car;   // The pre-allocated car object that this is linked to 

  pcxImage_t *img;
  fixed_t x;
  int z;    // 0-4096 units of the ZMap (also see ZFARCULL and ZNEARCULL)

  // For renderer and screen-space collisions
  // (renderer is responsible for filling these out)
  short sx;
  short sy;
  fixed_t scale;

  roadObj_t *next;
  roadObj_t *last;
};


// This is practically all just scriptable now. Get rid of horizontex
// and move it into the script.
typedef struct roadDef_t roadDef_t;
struct roadDef_t
{
  pcxImage_t *horizonTex;
};


// These are used to toggle on and off drawing the sides of the road

#define DRAWSIDE_LEFT 1
#define DRAWSIDE_ROAD 2
#define DRAWSIDE_RIGHT 4

#define MAXROADOBJ 64



// A lot of these are not per-road. They will be moved into roadctlr.
typedef struct roadLive_t roadLive_t;
struct roadLive_t
{
  roadDef_t *def;

  // Objects are ordered from the front of the road (head) to
  // the back of the road (tail)
  roadObj_t *objHead;
  roadObj_t *objTail;
  int objCount;      // For debugging purposes

  // Note that the x is 0 for both if road is unskewed and centered
  int xFront;        // x px offset of the front of the screen (position)
  int xBack;         // x px offset of the back of the screen (facing)
  fixed_t ddxSeg;       // x'' for curves
  fixed_t ddySeg;       // y'' for curves
  fixed_t ddxFront;
  fixed_t ddyFront;
  
  fixed_t z;         // what the z value at the front of the screen is
  fixed_t seg;       // which segment number we are on

  unsigned short texOff;    // offset into texture map
  fixed_t segPos;           // position of segment in the ZMap
  fixed_t halfWidth;        // half the width of the road
  unsigned char drawFlags;  // See DRAWSIDE defines

  pcxImage_t *sideTex[2];     // Pointer to left and right side textures
  pcxImage_t *roadTex;

  fixed_t xHorizon;
  fixed_t yOffsetHorizon;
};

void roadInit(roadDef_t *def, roadLive_t *live);

// yOffset is for shifting it up or down the screen
// yOmitStart and yOmitEnd are for preventing overdraw when something like a tunnel or overpass
// is drawn
void roadDraw(roadLive_t *live, pcxImage_t *dest, int yOffset, int yOmitStart, int yOmitEnd);
// Add the display list item for the object
void linkRoadObj(roadLive_t *live, roadObj_t *obj);
void unlinkRoadObj(roadLive_t *live, roadObj_t *obj);
// Move the road object, keeping the list sorted
// Do not move a road object's z without using this
// Positive z is into distance
void moveRoadObj(roadLive_t *live, roadObj_t *obj, int zAmt);

fixed_t getCurveAmount(roadLive_t *live);
fixed_t getHillAmount(roadLive_t *live);

#ifndef NDEBUG
int getObjIndex(roadLive_t *live, roadObj_t *obj);
#endif

#endif


