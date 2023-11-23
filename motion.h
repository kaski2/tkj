#ifndef MOVEMENT_RECOGNITION_H
#define MOVEMENT_RECOGNITION_H

#include <math.h>

#define axyThreshold 0.4
#define azThreshold 0
#define gxyThreshold 10
#define gzThreshold 70
#define gxThreshold 20


int detectLift(double ax, double ay, double az, double gx) {
    if (az < azThreshold && (fabs(ax) < axyThreshold) && (fabs(ay) < axyThreshold) && gx > gxThreshold) {
        return 1;
    }
    return 0;
}

int detectSlide(double ax, double ay, double az, double gx, double gy, double gz) {
    if (az < azThreshold && fabs(gx) < gxyThreshold && fabs(gy) < gxyThreshold && fabs(gz) > gzThreshold) {
        return 1;
    }
    return 0;
}

int detectTurn(double ax, double ay, double az, double gx) {
    if ((az > 0.5) && fabs(ax) < axyThreshold && fabs(ay) < axyThreshold) {
        return 1;
    }
    return 0;
}

#endif
