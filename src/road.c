#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cars.h"
#include "road.h"
#include "draw.h"
#include "lmath.h"
#include "roadctlr.h" // NEARCULL/FARCULL

//#define DEBUGDRAW
//#define DXLIMIT

#define ROADH 100

// Half of screen width in fixed
#define HSW16 (160<<16)

#define DXMAX (ITOFIX(1)<<10)
#define DYMAX (ITOFIX(1)<<4)
#define DYMIN (-ITOFIX(1))

// Line doubling is faster than interpolation for the hill effect,
// but a lot filthier
#define LINEDOUBLE

// Z index of the different curve thresholds (see render function)
// Lower is farther
#define ZPOSCURVETHRESH1 45
#define ZPOSCURVETHRESH2 15
#define ZPOSCURVETHRESH3 8

unsigned short hZMap[ZMAPLEN];

FILE *fp = 0; 


void drawRadar(roadLive_t *live, pcxImage_t *dest);

void calcZMap()
{
  int i;

  for (i=0; i < ZMAPLEN; i++)
    hZMap[i] = (1<<ZMAPBITS) / (i+1);    
}


void roadInit(roadDef_t *def, roadLive_t *live)
{
//  fp = fopen("roadobj.log", "w");

  memset(def, 0, sizeof(def));
  memset(live, 0, sizeof(live));
  calcZMap();
  live->def = def;
  live->halfWidth = 700;//600;
  live->xFront = 0<<16;   // This is skew, not just x
  live->xBack  = 0<<16;
}

// TODO: Move into loadpcx
void blitline(pcxImage_t *dest, int destxL, int desty, int destxR,
     pcxImage_t *src, int srcy)
{
// A table for screen lines might be nice here
// We could even just reuse the image structure
  char *wstop;
  char *rline = src->lines[srcy];
  char *wp;
  int flipped = 0;
  fixed_t srcWidth = src->width << 16;
  
  fixed_t ri = 0;
  fixed_t dx;

  if (destxL > destxR)
  {
    int t = destxL;
    destxL = destxR;
    destxR = t;
    flipped = 1;
  }

  if (destxR < 0 || destxL >= dest->width)
    return;

  dx = srcWidth / (destxR - destxL);  // Could table this per sy without adjustment..?
  // dx is 1/scale, which is z, so if we'd designed the zmap just right
  // we could probably use its value shifted some amount ;D

  if (!flipped)
  {
    // Draw left to right
    if (destxL < 0)
    {
      ri += dx * -destxL;
      destxL = 0;
    }
  
    if (destxR >= dest->width)
      destxR = dest->width - 1;

    wp = dest->lines[desty] + destxL;
    wstop = dest->lines[desty] + destxR;
    for (; wp <= wstop; wp++, ri += dx)
      *wp = rline[ri >> 16];
  }
  else
  {
    // Draw right to left
    if (destxR >= dest->width)
    {
      ri += dx * (destxR - dest->width - 1);
      destxR = 319;
    }
  
    if (destxL < 0)
      destxL = 0;

    wstop = dest->lines[desty] + destxL;
    wp = dest->lines[desty] + destxR;
    for (; wp >= wstop; wp--, ri += dx)
      *wp = rline[ri >> 16];
  }
}

void dumpRoad(roadLive_t *live) 
{
  if (!fp)
    return;

  roadObj_t *obj =  live->objHead;
  while (obj = obj->next) 
  {
    fprintf(fp, "obj: pcx=%p x=%f z=%d\n", obj->img, FIXTOF(obj->x), obj->z);
  }
  fclose(fp);
}

void roadCrash(roadLive_t *live, char *str)
{
  printf("FATAL: roadObj integrity: %s", str);  
  if (fp)
    fprintf(fp, "FATAL: roadObj integrity: %s\n", str);
  dumpRoad(live);
  fflush(fp);
  fclose(fp);
  while(1);
  exit(12);
}

void roadWarn(roadLive_t *live, char *str)
{
  if (fp)
    fprintf(fp, "WARNING: roadObj integrity: %s\n", str);  
  dumpRoad(live);
}

int getObjIndex(roadLive_t *live, roadObj_t *obj)
{
  roadObj_t *o = live->objHead;
  int i=0;
  while (o)
  {
    if (o == obj)
      return i;
      
    i++;
    o = o->next;
  }

  return -1;
}

void checkObjIntegrity(roadLive_t *live)
{
#ifdef NDEBUG
  return;
#endif

  if (live->objHead)
  {
    int count = 0;
    roadObj_t *o = live->objHead;
    if (!live->objTail) roadCrash(live, "Head without tail");

    while (o)
    {
      // Check that o is unique
      roadObj_t *o2 = o->next;
      while (o2)
      {
        if (o == o2) roadCrash(live, "Object is not unique");
        o2 = o2->next;
      }
      
      if (o->next)
      {
        if (o->z > o->next->z) roadWarn(live, "Object ahead of next");
      }
      else
      {
        if (o != live->objTail) roadCrash(live, "End of list is not tail");
      }
      
      o = o->next;
      count++;
    }

    if (count != live->objCount) roadCrash(live, "Count mismatch (forward)");
  }

  if (live->objTail)
  {
    int count = 0;
    roadObj_t *o = live->objTail;
    if (!live->objHead) roadCrash(live, "Tail without head");

    while (o)
    {
      if (!o->last && live->objHead != o) roadCrash(live, "Start of list is not head");

      count++;
      o = o->last;
    }

    if (count != live->objCount) roadCrash(live, "Count mismatch (backward)");
  }
}

// Insert the road obj "obj" relative to the slot currently occupied by "p"
void insertRoadObj(roadLive_t *live, roadObj_t *obj, roadObj_t *p, int isBefore)
{
  assert(obj);
  assert(p);
  if (fp)
    fprintf(fp, "insertRoadObj: obj=%p relativeTo=%p isBefore=%d\n", obj, p, isBefore);

  if (isBefore)
  {
    obj->next = p;
    obj->last = p->last;

    if (live->objHead == p)
      live->objHead = obj;
  }
  else
  {
    obj->next = p->next;
    obj->last = p;

    if (live->objTail == p)
      live->objTail = obj;
  }

  if (obj->last)
    obj->last->next = obj;

  if (obj->next)
    obj->next->last = obj;

  live->objCount++;
  checkObjIntegrity(live);
}

void linkRoadObj(roadLive_t *live, roadObj_t *obj)
{
  if (fp)
    fprintf(fp, "linkRoadObj: obj=%p\n", obj);
  if (!live->objHead)
  {
    live->objHead = obj;
    live->objTail = obj;
    obj->next = 0;
    obj->last = 0;
    live->objCount++;
  }
  else
  {
    roadObj_t *p = live->objHead;
    while (p)
    {
      if (obj->z <= p->z)
      {
        insertRoadObj(live, obj, p, 1);
        break;
      }

      p = p->next;
    }

    // We didn't find an object that it belongs before, so put it
    // at the end of the list
    if (!p)
      insertRoadObj(live, obj, live->objTail, 0);
  }


  checkObjIntegrity(live);
}


// With two objects, unlinking what I expect is the closest one
// leaves you with a tail but no head
void unlinkRoadObj(roadLive_t *live, roadObj_t *obj)
{
  if (fp)
    fprintf(fp, "unlinkRoadObj: obj=%p\n", obj);

  // When there are two cars, first obj->next is 0 and
  if (live->objHead == obj)
    live->objHead = obj->next;   // This is where it sets it to 0. List seems linked OK. Head/tail confusion?
  if (live->objTail == obj)
    live->objTail = obj->last;
    
  if (obj->last)
    obj->last->next = obj->next;
  if (obj->next)
    obj->next->last = obj->last;

  live->objCount--;
   
  checkObjIntegrity(live);
}

// Relink the road object to where it has moved
void moveRoadObj(roadLive_t *live, roadObj_t *obj, int zAmt)
{
//  fprintf(fp, "moveRoadObj: obj=%p zAmt=%d\n", obj, zAmt);

  obj->z += zAmt;

  if (zAmt > 0)  // Into distance
  {
    if (obj->next && obj->z > obj->next->z)
    {
      unlinkRoadObj(live, obj);
      insertRoadObj(live, obj, obj->next, 0);
    }
  }
  else if (zAmt < 0)
  {
    if (obj->last && obj->z < obj->last->z)
    {
      unlinkRoadObj(live, obj);
      insertRoadObj(live, obj, obj->last, 1);
    }
  }

  checkObjIntegrity(live);
}

// Draw a row of the road on line sy with left and right of the road at
// rl and rr, with the specified texture z
void drawRow(pcxImage_t *dest, roadLive_t *live,
     pcxImage_t *roadTex, pcxImage_t **sideTex,
     int sy, int rl, int rr, int zOff)
{
      int texWidth = rr - rl;
      blitline(dest, rr + texWidth, sy, rr, sideTex[0], (zOff>>3) % (sideTex[0]->height));
      blitline(dest, rl, sy, rr, roadTex, (zOff>>3) % roadTex->height);
      blitline(dest, rl - texWidth, sy, rl, sideTex[1], (zOff>>3) % (sideTex[1]->height));

      // Fill in for anything not covered by texture (based on color of first
      // pixel of the texture)
      if (rr + texWidth < dest->width - 1)
//        hline(dest, rr + texWidth, sy, dest->width - 1, 96+12);
        hline(dest, rr + texWidth, sy, dest->width - 1, 1);
      if (rl - texWidth > 0)
        hline(dest, 0, sy, rl - texWidth, 76);
}

// Dampen curves in the foreground and exaggerate them in the distance
fixed_t ddShiftCurve(fixed_t dd, int zIdx)
{
  if (zIdx <= ZPOSCURVETHRESH3)
  {
    return dd << 3;
  }
  else if (zIdx <= ZPOSCURVETHRESH2)
  {
    return dd;
  }
  else if (zIdx <= ZPOSCURVETHRESH1)
  {
    return dd >> 1;
  }
  else
  {
    return 0;
  }
}

fixed_t ddShiftHill(fixed_t dd, int zIdx)
{
  if (zIdx <= ZPOSCURVETHRESH3)
  {
    return dd << 5;
  }
  else if (zIdx <= ZPOSCURVETHRESH2)
  {
    return dd << 3;
  }
  else if (zIdx <= ZPOSCURVETHRESH1)
  {
    return dd;
  }
  else
  {
    return 0;
  }
}

typedef struct renderState_t {
  pcxImage_t *roadTex;
  pcxImage_t *sideTex[2];
  roadObj_t *pObj;
  fixed_t hw;
  fixed_t xoff;
  fixed_t dx;
  fixed_t dy;
  fixed_t y;
} renderState_t;

void roadDraw(roadLive_t *live, pcxImage_t *dest, int yOffset)
{
  int i;

  // The +2 is a fudge factor because we clip to the near plane
  // and this changes the fov so that it narrows a little more
  const fixed_t narrow = (ITOFIX(live->halfWidth-1)) / ROADH;
  const fixed_t skew = ITOFIX(live->xBack - live->xFront) / ROADH;
  fixed_t hw = ITOFIX(live->halfWidth);
  fixed_t xoff = ITOFIX(live->xFront); // front to back
  fixed_t dx = 0, dy = 0;
  fixed_t y;

  pcxImage_t *roadTex = live->roadTex;
  pcxImage_t *sideTex[2];
  sideTex[0] = live->sideTex[0];
  sideTex[1] = live->sideTex[1];
  
  
  int syLast = 0;
  roadObj_t *pObj;

  // zmap is ordered far to near, and we are drawing near to far
  y = (dest->height - 1 - yOffset) << 16;
  dy = 1 << 16;
  pObj = live->objHead;

  for (i=ZMAPLEN-1; i >= 0; i--)
  {
    int rl = FIXTOI(xoff+HSW16-hw);
    int rr = FIXTOI(xoff+HSW16+hw);
    int z = hZMap[i];  // z values range from 0 to 4096 (12-bit)
    int zOff = z + live->texOff;
    int sy = FIXTOI(y);

    // Sprite projection:
    // ------------------
    // See if we've past the next sprite's Z. If so, assign its sy
    // and move to the next sprite. If it's a scenery switch, assign
    // the texture at the appropriate y
    while (pObj && z >= pObj->z)
    {
      if (pObj->z >= ZNEARPLANE)
      {
        if (!pObj->img)
        {
          // TODO: Potentially use the first few low values for special types?
        }
        else
        {
          pObj->sy = sy;
        
          // This could also be tabled
          // 140 is target: When halfwidth is 140, scale is 1:1
          // Approximated with shifts (used to be hw/140, now it's this thing)
          // Using z instead of tying scale to the width of the road at that line
          // is smoother, but costs a division.
          pObj->scale = (1<<FIXSHIFT<<7) / pObj->z;
          if (pObj->scale == 0)
            printf("SET SCALE TO 0 for z=%d\n", pObj->z);
          pObj->sx = FIXTOI(HSW16 + xoff + ((pObj->x>>8)*pObj->scale));
        }
      }

      pObj = pObj->next;
    }

    if (z >= 6000)  // This looks good; it's a little farther than the ZFARPLANE
      break;

    // Finding texture y coordinate:
    // Shift 12-bit z value down to a range that repeats a reasonable
    // number of times for our 32px-high texture (arbitrary).
    roadObj_t *scenObj = live->objHead;
    while (scenObj && scenObj->car->type != CARTYPE_DUMMY_SCEN)
      scenObj = scenObj->next;

    if (sy < syLast)
    {
      if (scenObj && scenObj->sy == sy) 
      {
        if (scenObj->x == 0) // road 
          roadTex = scenObj->img;
        else if (scenObj->x == -1) // left side
          sideTex[0] = scenObj->img;
        else if (scenObj->x == 1) // right side
          sideTex[1] = scenObj->img;

        // Queue up next scenery change incident
        while (scenObj && scenObj->car->type != CARTYPE_DUMMY_SCEN)
          scenObj = scenObj->next;
      }

      drawRow(dest, live, roadTex, &sideTex[0], sy, rl, rr, zOff);
    }
    
    // Bring syLast up to sy
    // Quick line doubling technique
    if (syLast - sy > 1)
    {
      for (; syLast > (sy+1); syLast--)
      {
#ifdef LINEDOUBLE
        // TODO: Implement DRAWSIDE
        // TODO: Use asm blit; faster than memcpy
        memcpy(dest->lines[syLast-1], dest->lines[syLast], dest->width);
#else
#endif
      }
    }

    // Else no doubling required (might be downslope)

    xoff += skew + dx;
    hw -= narrow;

    y -= dy;
    syLast = sy;

    // Animate the curves and undulation as we go up the screen
    // Do this at the end because we use dy above for line doubling
    // ZPOSCURVETHRESH keeps the curves mellow in the front of the screen
    // but bending more severely in the distance
    if (i <= live->segPos>>16)
    {
      dx += ddShiftCurve(live->ddxSeg, i);
      dy += ddShiftHill(live->ddySeg, i);
    }
    else
    {
      dx += ddShiftCurve(live->ddxFront, i);
      dy += ddShiftHill(live->ddyFront, i);
    }

#ifdef DXLIMIT
    if (dx > DXMAX)
      dx = DXMAX;
    else if (dx < -DXMAX)
      dx = -DXMAX;
#endif

    if (dy > DYMAX)
      dy = DYMAX;
    else if (dy < DYMIN)
      dy = DYMIN;


#ifdef DEBUGDRAW
    if (i == ZPOSCURVETHRESH1)
      hline(dest, 0, sy, 319, 9);
    if (i == ZPOSCURVETHRESH2)
      hline(dest, 0, sy, 319, 9);
    if (i == ZPOSCURVETHRESH3)
      hline(dest, 0, sy, 319, 9);
#endif

//    if (y < 0 || y >= 200 << 16) // TODO: we may still want to find sprite y if below screen
//      break;
  }

  // Draw horizon
  {
    pcxImage_t *tex = live->def->horizonTex;
    int yStart = syLast - tex->height + FIXTOI(live->yOffsetHorizon);
    int yClipEnd = tex->height - FIXTOI(live->yOffsetHorizon);
    
    if (FIXTOI(live->xHorizon) >= live->def->horizonTex->width)
      live->xHorizon -= ITOFIX(live->def->horizonTex->width);
    else if (live->xHorizon < 0)
      live->xHorizon += ITOFIX(live->def->horizonTex->width);
  
    assert(live->xHorizon >= 0);  

    blitPCXClip
    (
        tex, dest,
        FIXTOI(-live->xHorizon), yStart,
        0, 0,
        tex->width, yClipEnd
    );

    blitPCXClip
    (
        tex, dest,
        tex->width - FIXTOI(live->xHorizon), yStart,
        0, 0,
        tex->width, yClipEnd
    );

    if (yStart < syLast)
       drawSquare(dest, 0, 0, dest->width, yStart, tex->lines[0][0]);
    else
       drawSquare(dest, 0, 0, dest->width, syLast, tex->lines[0][0]);
  } // next i in ZMAPLEN


#ifdef DEBUGDRAW
  hline(dest, 0, 100 + (live->segPos>>16), 319, 9);
  hline(dest, 0, syLast, 319, 15);
#endif

  // Blit road sprites, starting from the back (where pObj should now be)
  // TODO: For stationary objects, if they are off the left side of the road, the
  // left graphic is used if specified. This is good for something like the
  // chevrons.
  pObj = live->objTail;
  while (pObj)
  {
    if (pObj->car->type != CARTYPE_DUMMY_SCEN && pObj->z >= ZNEARPLANE && pObj->z < ZFARPLANE)
    {
      fixed_t width = pObj->scale * pObj->img->width;
      fixed_t height = pObj->scale * pObj->img->height;

      // TODO: Clip against horizon when on downslope
      blitPCXScaledSprite(pObj->img, dest,
                          pObj->sx - FIXTOI(width>>1), pObj->sy - FIXTOI(height),
                          pObj->scale, pObj->scale,
                          dest->height /* TODO: clip */);
    }

    pObj = pObj->last;
  }

  drawRadar(live, dest); 
}

// TODO: Interpolate between speeds so they don't transition too jarringly
// The logic here is that if the road doesn't curve near the front of
// the screen, the curve will stop animating before the curve scroll/physics
// take effect
fixed_t getCurveAmount(roadLive_t *live)
{
  if (FIXTOI(live->segPos) < ZPOSCURVETHRESH1)
    return live->ddxFront;
  else
    return live->ddxSeg;

    return live->ddxFront;
}


fixed_t getHillAmount(roadLive_t *live)
{
  if (FIXTOI(live->segPos) < ZPOSCURVETHRESH1)
    return live->ddyFront;
  else
    return live->ddySeg;

  return live->ddyFront;
}

void drawRadar(roadLive_t *live, pcxImage_t *dest) 
{
  const int baseX = 320 - 50;
  const int baseY = 50;
  const int zRange = ZFARCULL - ZNEARCULL;
  roadObj_t *pObj = live->objHead;

//  blitline(dest, baseX, baseY, baseX + FIXTOI(pObj->x * 40), 15);
  while (pObj) {
    // road z is 0-4096 , x is fixed_t -1.0 to +1.0 for on-the-road
    if (pObj->x >= ITOFIX(-1) && pObj->x <= ITOFIX(1)) {
      const int x = FIXTOI(pObj->x * 40);
      const int y = (ZNEARCULL - pObj->z) >> 8;
      assert((baseY+y) >= 0);
      assert((baseY+y) < 200);
      *(dest->lines[baseY + y] + baseX + x) = 15;
      *(dest->lines[baseY + y] + baseX + x + 1) = 15;
      *(dest->lines[baseY + y + 1] + baseX + x + 1) = 15;
      *(dest->lines[baseY + y + 1] + baseX + x) = 15;
    }

      const int x = 0;
      const int y = ZNEARCULL >> 8;
      *(dest->lines[baseY + y] + baseX + x) = 10;
      *(dest->lines[baseY + y] + baseX + x + 1) = 10;
      *(dest->lines[baseY + y] + baseX + x + 2) = 10;
      *(dest->lines[baseY + y] + baseX + x - 1) = 10;
      *(dest->lines[baseY + y] + baseX + x - 2) = 10;

    pObj = pObj->next;
  }
}