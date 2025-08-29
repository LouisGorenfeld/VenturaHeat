#ifndef LMATH_H
#define LMATH_H

typedef int fixed_t;
extern fixed_t gSinTab[256];


// Degree is 0 - 1.0 in fixed point
fixed_t getCurve(fixed_t degree);

#define FSIN(a) (gSinTab[((a)&255)])
#define FCOS(a) (gSinTab[((a)+64)&255])

#define FIXSHIFT 16
#define FIXUNITF 65536.0f
#define FIXSHIFTH 8
#define FIXMUL(a, b) ((a)>>FIXSHIFTH)*((b)>>FIXSHIFTH)
#define FIXTOF(a) ((float)(a)/FIXUNITF)
#define FTOFIX(a) ((fixed_t)(a*FIXUNITF))
#define ITOFIX(a) ((fixed_t)(a)<<FIXSHIFT)
#define FIXTOI(a) ((fixed_t)(a)>>FIXSHIFT)


#define LININ(d, x1, x2) ((x1) + (((d) * ((x2)-(x1))) >> FIXSHIFT))

#endif
