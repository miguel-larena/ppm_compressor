/*
 * decompressions.c
 * 
 * by Miguel Larena, Joshua Thomas
 * March 13, 2022
 * HW4 (arith)
 * 
 * Helper function definitions for decompression.
 * 
 */
#include <stdlib.h>
#include <assert.h>
#include "decompressions.h"
#include "a2methods.h"
#include "uarray2b.h"
#include "pnm.h"
#include "compressions.h"
#include "math.h"
#include "bitpack.h"
#include "arith40.h"

typedef struct fileInfoCl {
    FILE *fp;
    int width;
    int height;
} *fileInfoCl;

typedef struct a2Cl {
    A2Methods_UArray2 array;
    A2Methods_T methods;
} *a2Cl;

void get_codeWords (int col, int row, A2Methods_UArray2 original, void *element,
                                                                  void *cl);
A2Methods_UArray2 unpack_codeWordMap(A2Methods_UArray2 codeWordMap,
                                     A2Methods_T methods);
void unpack_codeWord(int col, int row, A2Methods_UArray2 original,
                                        void *element, void *cl);
A2Methods_UArray2 compFile_to_uarray2(FILE *in, A2Methods_T methods);
A2Methods_UArray2 dct_to_compVid(A2Methods_UArray2 dctMap, A2Methods_T methods);
void dequantize(int col, int row, A2Methods_UArray2 original, void *element,
                                                              void *cl);
void convert_dct_to_compVid(int col, int row, A2Methods_UArray2 original,
                                              void *element, void *cl);

/* decompress()
 *
 * Drives the logic for decompression by invoking all the helper functions
 *
 * PARAMETERS:         FILE *input - compressed file
 *             A2Methods_T methods - UArray2 methods suite
 *    RETURNS: Nothing
 * EXCEPTIONS: Raises a C.R.E. if file does not exist or if methods 
 *             or images cannot be allocated
 */
void decompress(FILE *input, A2Methods_T methods)
{
    assert(input);
    assert(methods);
    
    A2Methods_UArray2 codeWordMap = compFile_to_uarray2(input, methods);
    A2Methods_UArray2 dctMap = unpack_codeWordMap(codeWordMap, methods);
    A2Methods_UArray2 compVidMap = dct_to_compVid(dctMap, methods);
    
    Pnm_ppm image = malloc(sizeof(struct Pnm_ppm));
    assert(image);
    image = compVid_to_rgb(compVidMap, image, methods);
    Pnm_ppmwrite(stdout, image);
    
    methods->free(&codeWordMap);
    methods->free(&dctMap);
    methods->free(&compVidMap);
    Pnm_ppmfree(&image);
}

/* compFile_to_uarray2()
 *
 * Reads and validates a compressed file for storage into a UArray2 containing
 * its codewords in proper format.
 *
 * PARAMETERS:         FILE *input - compressed file
 *             A2Methods_T methods - UArray2 methods suite
 *    RETURNS: A2Methods_UArray2 of codewords
 * EXCEPTIONS: Raises a C.R.E. if file does not exist, if methods cannot
 *             be allocated, or if the file header is improperly formatted
 */
A2Methods_UArray2 compFile_to_uarray2(FILE *in, A2Methods_T methods)
{
    assert(in);
    assert(methods);
    
    unsigned height, width;
    int read = fscanf(in, "COMP40 Compressed image format 2\n%u %u", &width,                                                         &height);
    assert(read == 2);
    int c = getc(in);
    assert(c == '\n');
    
    width /= 2;
    height /= 2;
    A2Methods_UArray2 codeWordArr = methods->new(width, height,
                                                        sizeof(uint64_t));
                                                        
    methods->map_block_major(codeWordArr, get_codeWords, in);    
    return codeWordArr;
}

/* get_codeWords()
 *
 * Gets 32 bit codewords from a compressed file and properly allocates
 * the codeword in the supplied UArray2
 *
 * PARAMETERS:                    int col - column
 *                                int row - row
 *             A2Methods_UArray2 original - UArray2 to store codewords in
 *                          void *element - current index in UArray2
 *                               void *cl - file pointer
 *              
 *    RETURNS: Nothing
 * EXCEPTIONS: Raises a C.R.E if getc() reaches EOF
 */
void get_codeWords (int col, int row, A2Methods_UArray2 original, void *element,
                                                                  void *cl)
{
    (void) col; (void) row; (void) original;
    FILE *fp = (FILE *) cl;
    uint64_t *codeWord = (uint64_t *) element;
    int c;
    
    for (int i = 24; i >= 0; i -= 8) {
        c = getc(fp);
        if (c == EOF) {
            return;
        }
        assert(c != EOF);
        *codeWord = Bitpack_newu(*codeWord, 8, i, c);
    }
}

/* unpack_codeWordMap()
 *
 * Unpacks codewords and obtains the necessary values for their dct
 * representations
 * 
 *
 * PARAMETERS: A2Methods_UArray2 codeWordMap - UArray2 of codewords
 *                       A2Methods_T methods - UArray2 methods suite
 *    RETURNS: A2Methods_UArray2 
 * EXCEPTIONS: Raises a C.R.E. if codeWordMap or methods does not exist,
 *             or if a2Cl struct cannot be allocated
 */
A2Methods_UArray2 unpack_codeWordMap(A2Methods_UArray2 codeWordMap,
                                     A2Methods_T methods)
{
    assert(codeWordMap);
    assert(methods);
    
    int width = methods->width(codeWordMap);
    int height = methods->height(codeWordMap);
    A2Methods_UArray2 dctMap = methods->new(width, height,
                                            sizeof(struct dctSpace));
    a2Cl cl = malloc(sizeof(struct a2Cl));
    assert(cl);
    cl->array = dctMap;
    cl->methods = methods;
                                             
    methods->map_block_major(codeWordMap, unpack_codeWord, cl);
    free(cl);
    return dctMap;
}

/* unpack_codeWord()
 *
 * Apply function for unpack_codeWordMap() which unpacks the codewords 
 * using the appropriate Bitpack functions and initializes the dctSpace values
 * to their relevant fields.
 *
 * PARAMETERS:                    int col - column
 *                                int row - row
 *             A2Methods_UArray2 original - UArray2 of codewords
 *                          void *element - current index in UArray2 original
 *                               void *cl - a2Cl struct which holds dctMap
 *                                          and its methods suite
 *    RETURNS: Nothing
 * EXCEPTIONS: None
 */
void unpack_codeWord(int col, int row, A2Methods_UArray2 original,
                                        void *element, void *cl)
{
    (void) original;
    
    uint64_t *codeWord = (uint64_t *) element;
    a2Cl arrCl = (a2Cl) cl;
    dctSpace *dct = (dctSpace *) (arrCl->methods->at(arrCl->array, col, row));
    
    dct->a = Bitpack_getu(*codeWord, 9, 23);
    dct->b = Bitpack_gets(*codeWord, 5, 18);
    dct->c = Bitpack_gets(*codeWord, 5, 13);
    dct->d = Bitpack_gets(*codeWord, 5, 8);
    dct->pb = Bitpack_getu(*codeWord, 4, 4);
    dct->pr = Bitpack_getu(*codeWord, 4, 0); 
}

/* dct_to_compVid()
 *
 * Converts dct values in dctMap and returns a UArray2 of its equivalent
 * component video representation. 
 *
 * PARAMETERS: A2Methods_UArray2 dctMap - UArray2 of dctSpaces
 *                   2Methods_T methods - UArray2 methods suite
 *    RETURNS: A2Methods_UArray2 dctMap - UArray2 of compVids
 * EXCEPTIONS: Raises a C.R.E. if dctMap or methods does not exist,
 *             or if a2Cl struct cannot be allocated
 */
A2Methods_UArray2 dct_to_compVid(A2Methods_UArray2 dctMap, A2Methods_T methods)
{
    assert(dctMap);
    assert(methods);
    
    methods->map_block_major(dctMap, dequantize, methods);
    int width = (methods->width(dctMap)) * 2;
    int height = (methods->height(dctMap)) * 2;
    A2Methods_UArray2 compVidMap = methods->new(width, height,
                                               sizeof(struct compVid));
    
    a2Cl cl = malloc(sizeof(struct a2Cl));
    assert(cl);
    cl->array = compVidMap;
    cl->methods = methods;
    
    methods->map_block_major(dctMap, convert_dct_to_compVid, cl);
    free(cl);
    return compVidMap;
}

/* dequantize()
 *
 * Apply function for dct_to_compVid() that performs the inverse of quantize
 * for each dctSpace value, bounding each value to the appropriate range.
 *
 * PARAMETERS:                    int col - column
 *                                int row - row
 *             A2Methods_UArray2 original - UArray2 of dctSpaces
 *                          void *element - current index in UArray2 original
 *                               void *cl - a2Cl struct which holds dctMap
 *                                          and its methods suite
 *    RETURNS: Nothing
 * EXCEPTIONS: None
 */
void dequantize(int col, int row, A2Methods_UArray2 original, void *element,
                                                              void *cl)
{
    (void) col; (void) row; (void) original; (void) cl;
    
    dctSpace *dct = (dctSpace *) element;
    
    dct->a = (dct -> a) / 63.0;
    dct->b = (dct -> b) / 100.0;
    dct->c = (dct -> c) / 100.0;
    dct->d = (dct -> d) / 100.0;
    dct->pb = Arith40_chroma_of_index(dct->pb);
    dct->pr = Arith40_chroma_of_index(dct->pr);
    if (dct->b > 0.3) {
        dct->b = 0.3;
    }
    if (dct->c > 0.3) {
        dct->c = 0.3;
    }
    if (dct->d > 0.3) {
        dct->d = 0.3;
    }
    if (dct->b < -0.3) {
        dct->b = -0.3;
    }
    if (dct->c < -0.3) {
        dct->c = -0.3;
    }
    if (dct->d < -0.3) {
        dct->d = -0.3;
    }
}

/* convert_dct_to_compVid()
 *
 * Apply function for dct_to_compVid that converts each dctSpace to its
 * component video representation. Its index pattern is modified to account
 * for the 2*2 blocks that make up each dct.
 *
 * PARAMETERS:                    int col - column
 *                                int row - row
 *             A2Methods_UArray2 original - UArray2 of dctSpaces
 *                          void *element - current index in UArray2 original
 *                               void *cl - a2Cl struct which holds compVidMap
 *                                          and its methods suite
 *    RETURNS: Nothing
 * EXCEPTIONS: None
 */
void convert_dct_to_compVid(int col, int row, A2Methods_UArray2 original,
                                              void *element, void *cl)
{
    (void) original;
    
    dctSpace *dct = (dctSpace *) element;
    a2Cl arrCl = (a2Cl) cl;
    
    int currY = 1;
    for(int i = row * 2; i <= (i * 2) + 1; i++) {
        for(int j = col * 2; j <= (col * 2) + 1; j++) {
            compVid *compVidRep = (compVid *) arrCl->methods->at(arrCl->array,
                                                                 j, i);
            compVidRep->pb = dct->pb;
            compVidRep->pr = dct->pr;
            if (currY == 1) {
                compVidRep->y = (dct->a - dct->b - dct->c + dct->d);
            }
            if (currY == 2) {
                compVidRep->y = (dct->a - dct->b + dct->c - dct->d);
            }
            if (currY == 3) {
                compVidRep->y = (dct->a + dct->b - dct->c - dct->d);
            }
            if (currY == 4) {
              compVidRep->y = (dct->a + dct->b + dct->c + dct->d);
            }
            currY++;
        }
    }
}

/* compVid_to_rgb()
 *
 * Converts each component video value in a compVidMap to its equivalent 
 * rgb representation and returns a Pnm_ppm image
 *
 * PARAMETERS: A2Methods_UArray2 compVidMap - UArray2 of compVids
 *                            Pnm_ppm image - to store rgb values
 *                      A2Methods_T methods - methods suite
 *    RETURNS: Pnm_ppm image
 * EXCEPTIONS: None
 */
Pnm_ppm compVid_to_rgb(A2Methods_UArray2 compVidMap, Pnm_ppm image,
                       A2Methods_T methods)
{
    int width = methods->width(compVidMap);
    int height = methods->height(compVidMap);
    image->width = width;
    image->height = height;
    image->methods = methods;
    image->pixels = methods->new(width, height, sizeof(struct Pnm_rgb));
    image->denominator = 255;

    for(int i = 0; i < width; i++) {
        for(int j = 0; j < height; j++) {
            compVid *currPixel = (compVid *) methods->at(compVidMap, i, j);
            struct Pnm_rgb *rgbVals = malloc(sizeof(struct Pnm_rgb));

            float temp_r = (1.0 * currPixel->y) + (0.0 * currPixel->pb) + (1.402 *
                    currPixel->pr);
            float temp_g = (1.0 * currPixel->y) - (0.344136 * currPixel->pb) -
                    (0.714136 * currPixel->pr);
            float temp_b = (1.0 * currPixel->y) + (1.772 * currPixel->pb) + (0.0 *
                    currPixel->pr);

            int red = (int)(temp_r * (float) image->denominator);
            int green = (int)(temp_g * (float) image->denominator);
            int blue = (int)(temp_b * (float) image->denominator);
            
            struct Pnm_rgb *oldRgb = rgbVals;
            rgbVals = (struct Pnm_rgb*) methods->at(image->pixels,
                                                    i, j);
            free(oldRgb);
            
            rgbVals->red = red;
            rgbVals->green = green;
            rgbVals->blue = blue;
          }
    }
    return image;
}