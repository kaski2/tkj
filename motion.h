#ifndef MOVEMENT_RECOGNITION_H
#define MOVEMENT_RECOGNITION_H

#include <math.h>

#define axyTHRESHOLD 0.4
#define azTHRESHOLD 0
#define gxyTHRESHOLD 20
#define gzTHRESHOLD 70
#define gxTHRESHOLD 20


int detectLift(double ax, double ay, double az, double gx) {
    if (az < azTHRESHOLD && (fabs(ax) < axyTHRESHOLD) && (fabs(ay) < axyTHRESHOLD) && gx > gxTHRESHOLD) {
        return 1;
    }
    return 0;
}

int detectSlide(double ax, double ay, double az, double gx, double gy, double gz) {
    if (az < azTHRESHOLD && fabs(gx) < gxyTHRESHOLD && fabs(gy) < gxyTHRESHOLD && fabs(gz) > gzTHRESHOLD) {
        return 1;
    }
    return 0;
}

int detectTurn(double ax, double ay, double az, double gx) {
    if ((az > 0.5) && fabs(ax) < axyTHRESHOLD && fabs(ay) < axyTHRESHOLD) {
        return 1;
    }
    return 0;
}

#endif
