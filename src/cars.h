#ifndef _CARS_H_
#define _CARS_H_

#include "lmath.h"
#include "road.h"

typedef struct roadCtlr_t _roadCtlr_t;
typedef struct car_t _car_t;

typedef enum carloc_t
{
  CARLOC_FRONT = 0,
  CARLOC_BACK,
  CARLOC_LEFT,
  CARLOC_RIGHT,
  CARLOC_ROOF,
  CARLOC_UNDER,
  CARLOC_ENGINE,
  CARLOC_TIRELF,  // left-front
  CARLOC_TIRERF,  // right-front
  CARLOC_TIRELR,  // left-rear
  CARLOC_TIRERR,  // right-rear
  NUMCARLOC
} carloc_t;


typedef enum carType_t
{
  CARTYPE_COUPE = 0,
  CARTYPE_TREE,
  // TODO: These could be the same, changing depending on facing or position
  CARTYPE_RCHEV,
  CARTYPE_LCHEV,
  // Texture change dummy object. The x is "side" (see roadctlr)
  CARTYPE_DUMMY_SCEN,
} carType_t;

typedef struct car_t
{
  carType_t type;
  fixed_t speed;  // MPH, same units as the player's speed
  fixed_t damage[NUMCARLOC];
  int texNum;      // TODO: Robust system with multiple sprites per car

  roadObj_t roadObj;
} car_t;

#define MAXGEARS 6

// Acceleration curve:
// |-- good -----------------|-- sweet --|-- good --|-- bad ----------|
//                           |           |          |
//                           |           |          rpmsBadLo     RPMs maxed
//                           |           |
//                           |           rpmsSweetHi
//                           rpmsSweetLo
//
//
// - Look up what zone are in for your RPMs to find acceleration rate
// - RPM increase tracks the acceleration
// - When shifting, interpolate between speed min/max to find target rpms:
//   Tach is MINRPMS to MAXRPMS, so at the top end of the gear RPMs will be
//   MAXRPMS while at the low end of the gear they will be MINRPMS
// - When shift is held, rpms shoot up at a quick rate. When it is released,
//   the RPMs will be dragged down to the target RPM (simulating a clutch)
// - The farther away from speedMax the speeds are when you downshift, the
//   more it will engine brake. If you're too slow for the gear, you get
//   accelBad performance
// - The highest gear's max speed sets the top speed of the vehicle

#define MINRPMS 1200
#define MAXRPMS 10000
#define OVERRPMS 12000

typedef struct gearDef_t
{
  int speedMin;
  int speedMax;

  // Base acceleration. Can be multiplied for worse/better engines
  fixed_t accelGood;
  fixed_t accelSweet;
  fixed_t accelBad;
} gearDef_t;

typedef struct plrCar_t
{
  int gear;        // Current gear
  int rpms;        // Current RPMs

  // Gearbox/acceleration modeling
  int numGears;
  gearDef_t gears[MAXGEARS];
  int rpmsSweetLo;
  int rpmsSweetHi;
  int rpmsBadLo;
  int skidding; // -num for left skid, +num for right skid. num is frames.

  car_t car;
} plrCar_t;


void initCar(car_t *car, fixed_t x, int z, fixed_t speed, carType_t type);
void carLogic(_roadCtlr_t *ctlr, car_t *car);
void copyCar(car_t *c1, car_t *c2);

void initPlrCar(plrCar_t *car);

#endif
