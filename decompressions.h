#ifndef DECOMPRESSIONS_H
#define DECOMPRESSIONS_H

#include "a2methods.h"
#include "a2blocked.h"
#include "pnm.h"
#include "uarray2.h"

Pnm_ppm compVid_to_rgb(A2Methods_UArray2 compVidMap, Pnm_ppm image,
                       A2Methods_T methods);
A2Methods_UArray2 dct_to_pixel(Pnm_ppm image);

void decompress(FILE *input, A2Methods_T methods);

#endif