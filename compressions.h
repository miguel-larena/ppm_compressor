#ifndef COMPRESSIONS_H
#define COMPRESSIONS_H

#include "a2methods.h"
#include "pnm.h"
#include "uarray2.h"
#include "seq.h"

typedef struct compVid {
        float y, pb, pr;
} compVid;

typedef struct dctSpace {
        float a, b, c, d, pb, pr;
} dctSpace;

void compress(Pnm_ppm image);

#endif