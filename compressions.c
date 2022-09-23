/*
 * compressions.c
 * 
 * by Miguel Larena, Joshua Thomas
 * March 13, 2022
 * HW4 (arith)
 * 
 * Helper function definitions for compression.
 * 
 */
#include <stdlib.h>
#include "compressions.h"
#include "decompressions.h"
#include "a2methods.h"
#include "a2blocked.h"
#include "pnm.h"
#include "assert.h"
#include "uarray2b.h"
#include "uarray2.h"
#include "uarray.h"
#include "math.h"
#include "bitpack.h"
#include "seq.h"
#include "arith40.h"


/*quantize
 *Purpose: quantize float component videos
 *Parameters: the value to quantize as a float
 *Returns: a signed integer as an int6_t 
 */
int64_t quantize(float quant){
    if(quant > 0.3 )
            quant = 0.3;
    if(quant < -0.3)
            quant = -0.3;
    quant *= 50;
    return (int64_t)round(quant);
}

/*convert_to_word
 *Purpose: packs a singl video component into a 32bit word
 *Paramters: an UArray2_T holding all video components and index (i,j) as ints
 *Returns: a 32bit word as an uint32_t 
 */
uint32_t convert_to_word(UArray2_T dctMap, int i, int j)
{
    dctSpace *compressedPixel = (dctSpace *) UArray2_at(dctMap, i,j);
    float a = compressedPixel->a;
    float b = compressedPixel->b;
    float c = compressedPixel->c;
    float d = compressedPixel->d;
    float pb = compressedPixel->pb;
    float pr = compressedPixel->pr;

    unsigned chromaPb = Arith40_index_of_chroma(pb);
    unsigned chromaPr = Arith40_index_of_chroma(pr);
    uint64_t quantA = round(a * 511);

    /*quantize coefficients and pack into a 32bit word */
    uint64_t word = 0;
    word = Bitpack_newu(word, 9, 23, quantA);
    word = Bitpack_news(word, 5, 18, quantize(b));
    word = Bitpack_news(word, 5, 13 , quantize(c));
    word = Bitpack_news(word, 5, 8, quantize(d));
    word = Bitpack_newu(word, 4, 4, chromaPb);
    word = Bitpack_newu(word, 4, 0, chromaPr);
    return (uint32_t) word;
}

/*pack_word
 *Purpose: pack all video component structs into 32bit words
 *Parametes: A UArray2_T holding all video components
 *Returns: A sequence holding all 32bit words
 */
Seq_T pack_word(UArray2_T dctMap){
    Seq_T compressedImage = Seq_new(UArray2_height(dctMap) *
                                    UArray2_width(dctMap));
    for(int i = 0; i < UArray2_width(dctMap); i++){
        for(int j = 0; j < UArray2_height(dctMap); j++){
                uint32_t packedWord = convert_to_word(dctMap, i, j);
                Seq_addhi(compressedImage, &packedWord);
        }
    }
    return compressedImage;
}

/*pixel_to_dct
 *Purpose: Convert Y values into cosine coeffeciennts 
 *Parameters: A UArray2_T holding all video components, and the width and height
             as ints
 *Returns: A UArray2_T holding all cosine coeffecients and average pb pr values
          as floats
*/
UArray2_T pixel_to_dct(UArray2_T pixelSpace, int width, int height){
    UArray2_T dctMap = UArray2_new(width, height, 
                                      sizeof(struct dctSpace));

    for(int i = 0; i < width; i+=2){
        for(int j = 0; j < height; j+=2){
            compVid *pixel = UArray2_at(pixelSpace, i, j);
            compVid *pixel2 = UArray2_at(pixelSpace, i, j+1);
            compVid *pixel3 = UArray2_at(pixelSpace, i+1, j);
            compVid *pixel4 = UArray2_at(pixelSpace, i+1, j+1);

            dctSpace *dctPixel = (dctSpace *) UArray2_at(dctMap, i, j);
            dctPixel->pb = pixel->pb + pixel2->pb + pixel3->pb + pixel4->pb / 4;
            dctPixel->pr = pixel->pr + pixel2->pr + pixel3->pr + pixel4->pr / 4;
            dctPixel->a = (pixel4->y+pixel2->y+pixel3->y+pixel->y)/4;
            dctPixel->b = (pixel4->y+pixel3->y-pixel2->y-pixel->y)/4;
            dctPixel->c = (pixel4->y-pixel3->y+pixel2->y-pixel->y)/4;
            dctPixel->d = (pixel4->y-pixel3->y-pixel2->y+pixel->y)/4;
        }
    }
    /* Free old UArray2_T */
    UArray2_free(&pixelSpace);
    return dctMap;
}


/*rgb_to_comp
 *Purpose: Converts the rgb pixels of a image into component video
 *Parameters: The image as a Pnm_ppm, the width and height as ints, and a
              A2Methods_T representing the methods
 *Returns: A UArray2_T containg structs of converted rgb pixels
*/
UArray2_T rgb_to_comp(Pnm_ppm image, int width, int height, A2Methods_T methods){
    UArray2_T dctMap = UArray2_new(width, height, sizeof(struct compVid));

    for(int i = 0; i < width; i++){
        for(int j = 0; j < height; j++){
            Pnm_rgb currPixel = methods->at(image->pixels, i, j);

            /*convert to floats */
            float r = (float)((double)currPixel->red / 
                    (double)image->denominator);
            float g = (float)((double)currPixel->green / 
                    (double)image->denominator);
            float b = (float)((double)currPixel->blue / 
                    (double)image->denominator);

            /* convert to component Vidio */
            compVid *temp;
            temp = UArray2_at(dctMap, i, j);
            temp->y =  (0.299 * r) + (0.587 * g) + (0.114 * b);
            temp->pb = (-0.168736 * r) - (0.331264 * g) + (0.5 * b);
            temp->pr = (0.5 * r) - (0.418688 * g) - (0.081312 * b);

        }
    }
    return dctMap;
}


void copy_image(int col, int row, A2Methods_UArray2 originalImage,
                                  void *element, void *cl) 
{
    (void) originalImage;

    Pnm_ppm image       = *((Pnm_ppm *) cl);
    int copyWidth       = (int) ((image->width) - 1);
    int copyHeight      = (int) ((image->height) - 1);
    if (col > copyWidth || row > copyHeight) {
        return;
    }

    Pnm_rgb rgbOriginal = (Pnm_rgb) element;
    Pnm_rgb rgbCopy     = (Pnm_rgb) image->methods->at(image->pixels, col, row);

    rgbCopy->red        = rgbOriginal->red;
    rgbCopy->green      = rgbOriginal->green;
    rgbCopy->blue       = rgbOriginal->blue;
}

void trim_image(Pnm_ppm image, int width, int height, int size)
{
    if (width % 2 != 0) {
        width--;
    }
    if (height % 2 != 0) {
        height--;
    }
    
    A2Methods_UArray2 originalImage = image->pixels;
    
    image->width = width;
    image->height = height;
    image->pixels = image->methods->new(width, height, size);
    
    image->methods->map_block_major(originalImage, copy_image, &image);
    
    image->methods->free(&originalImage);
}

Pnm_ppm check_dims(Pnm_ppm image)
{
    assert(image);

    int width = image->width;
    int height = image->height;
    int size = image->methods->size(image->pixels);

    if (width % 2 != 0 || height % 2 != 0) {
        trim_image(image, width, height, size);
    }

    return image;
}

/*print_image
 *Purpose: Prints the compressed image in big-edian order to stdout
 *Parameters: The sequence of words
 *Returns: nothinng
 */

void print_image(Seq_T compressedImage, int width, int height){
    assert(compressedImage != NULL);

    fprintf(stdout, "COMP40 Compressed image format 2\n%d %d\n", width, 
           height);
    for(int i = 0; i < Seq_length(compressedImage); i++){
        uint32_t currentWord = *(uint32_t*) Seq_get(compressedImage, i);

        putchar(Bitpack_getu(currentWord, 8, 24));
        putchar(Bitpack_getu(currentWord, 8, 16));
        putchar(Bitpack_getu(currentWord, 8, 8));
        putchar(Bitpack_getu(currentWord, 8, 0));
    }
}

/*compress
 *Purpose: Prints to stdout a compressed ppm image
 *Parameters: The image to compress as a Pnm_ppm
 *Returns: 
 *Effects: compresses an image
 */
void compress(Pnm_ppm image){
    A2Methods_T methods = uarray2_methods_blocked;
    image = check_dims(image);

    int width = image->width;
    int height = image->height;
    /*convert rgb values to component video */
    UArray2_T dctMap = rgb_to_comp(image, width, height, methods);
    /*calculate cosine coefficients from component video values */
    dctMap = pixel_to_dct(dctMap, width, height);
    compVid pixel = *(compVid*)UArray2_at(dctMap, 0, 0);
    (void) pixel;
    /*pack pixels into 32bit words and store the words in a squence */
    Seq_T compressedImage = pack_word(dctMap);
     
    /*prints compressed image in big-edian order to stdout*/
    print_image(compressedImage, width, height);

    UArray2_free(&dctMap);
    Seq_free(&compressedImage);
    Pnm_ppmfree(&image);
}
