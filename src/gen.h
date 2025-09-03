#ifndef _GENERATE_
#define _GENERATE_

#include "lmath.h"
#include "roadctlr.h"

typedef enum genStyle_t {
  GENSTYLE_LIGHT = 0,
  GENSTYLE_DENSE,
  GENSTYLE_ROWS,
  GENSTYLE_LINE
} genStyle_t;

typedef enum objType_t {
  
} objType_t

typedef struct carGen_t
{
  // Configuration parameters
  // Each x position is one lane worth, with
  // the negative values being on the left side of
  // the road
  fixed_t xMin;
  fixed_t xMax;
  
  int objType;
} gen_t;

void genRoll(gen_t *gen, roadCtlr_t *ctlr);



#endif
