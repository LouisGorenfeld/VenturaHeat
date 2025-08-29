#ifndef _GENERATE_
#define _GENERATE_

#include "lmath.h"
#include "roadctlr.h"

typedef enum genStyle_t
  GENSTYLE_LIGHT = 0,
  GENSTYLE_DENSE,
  GENSTYLE_ROWS,
  GENSTYLE_LINE
} genStyle_t;

typedef struct gen_t
{
  // Configuration parameters
  // Each x position is one lane worth, with
  // the negative values being on the left side of
  // the road
  int xMin;
  int xMax;

  //
} gen_t;

void genRoll(gen_t *gen, roadCtlr_t *ctlr);



#endif
