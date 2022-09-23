/*
 * compress40.c
 * 
 * by Miguel Larena, Joshua Thomas
 * March 13, 2022
 * HW4 (arith)
 * 
 * Invokes the appropriate compress() and decompress() functions for 40image.
 * 
 */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "compress40.h"
#include "pnm.h"
#include "a2methods.h"
#include "a2blocked.h"
#include "compressions.h"
#include "decompressions.h"

/* compress40()
 *
 * Compresses a ppm image and writes the compressed file to standard output
 *
 * PARAMETERS: FILE *input - ppm file
 *    RETURNS: Nothing
 * EXCEPTIONS: Raises a C.R.E. if methods cannot be allocated
 */
void compress40(FILE *input){
    A2Methods_T methods = uarray2_methods_blocked;
    assert(methods);
    Pnm_ppm image = Pnm_ppmread(input, methods);
    compress(image);
    return;
}

/* decompress40()
 *
 * Decompresses a compressed file and writes the ppm image to standard output
 *
 * PARAMETERS: FILE *input - compressed file
 *    RETURNS: Nothing
 * EXCEPTIONS: Raises a C.R.E. if methods cannot be allocated
 */
void decompress40(FILE *input){
    A2Methods_T methods = uarray2_methods_blocked;
    assert(methods);
    decompress(input, methods);
}