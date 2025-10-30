#include <assert.h>
#include "roadctlr.h"
#include "cars.h"
#include "game.h"

// Game idea: Roll to see where car is hit (trunk, tire, driver),
// and the car either veers off the road in some way or explodes
// depending on what is damaged. It doesn't alter the damage awarded.
// You could list what got hit on the HUD, like "driver killed". Scaled
// flying tire sprite

void carLogic(roadCtlr_t *ctlr, car_t *car)
{

  // Common to all cars


  // TODO: Set the correct frame for the direction
  // Or set the index of the texture and let the ctlr pick it up
  // Let's put the img using logic into the controller, and we just set
  // the texture index up here

  moveRoadObj(&ctlr->roadLive, &car->roadObj, SPEEDTOZ(car->speed));

  // TODO: We probably want a tiny peristent area before/after visible playfield
}

void initCar(car_t *car, fixed_t x, int z, fixed_t speed, carType_t type, int texNum)
{
  car->type = type;
  car->speed = speed;
  car->roadObj.x = x;
  car->roadObj.z = z;

  switch (type)
  {
    case CARTYPE_COUPE:
      car->texNum = 1;
      break;

    case CARTYPE_TREE:
      car->texNum = 2;
      break;

    case CARTYPE_LCHEV:
      car->texNum = 0;
      break;

    case CARTYPE_RCHEV:
      car->texNum = 0;
      break;

    case CARTYPE_DUMMY_SCEN:
      car->texNum = texNum;
      break;

    default:
      assert(0);
  }
}


fixed_t fromMphPerSec(int mph)
{
  return ITOFIX(mph) / 70; // 70hz VGA
}

// This is difficult to map because sweet spot/bad/etc are in RPMs
// while min/max speed is mph.
// We need to make sure that you can shift up while in sweet spot and
// not land outside of min/max speeds for the next gear
void initGears(gearDef_t *gears)
{
  gearDef_t *g = gears;

  g->speedMin = 0;
  g->speedMax = 80;
  g->accelGood = fromMphPerSec(25);
  g->accelSweet = fromMphPerSec(30);
  g->accelBad = fromMphPerSec(15);
  g++;
  
  g->speedMin = 30;
  g->speedMax = 165;
  g->accelGood = fromMphPerSec(19);
  g->accelSweet = fromMphPerSec(28);
  g->accelBad = fromMphPerSec(15);
  g++;

  g->speedMin = 80;
  g->speedMax = 220;
  g->accelGood = fromMphPerSec(18);
  g->accelSweet = fromMphPerSec(27);
  g->accelBad = fromMphPerSec(15);
  g++;

  g->speedMin = 130;
  g->speedMax = 340;
  g->accelGood = fromMphPerSec(17);
  g->accelSweet = fromMphPerSec(25);
  g->accelBad = fromMphPerSec(8);
  g++;
}

void initPlrCar(plrCar_t *car)
{
  car->rpmsSweetLo = 4000;
  car->rpmsSweetHi = 8000;
  car->rpmsBadLo = 9000;
  initGears(car->gears);
  car->numGears = 4;
}


