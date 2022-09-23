/*
 * bitpack.c
 * 
 * by Miguel Larena, Joshua Thomas
 * March 13, 2022
 * HW4 (arith)
 * 
 * Defines helper functions for bitpacking and unpacking codewords.
 * 
 */
#include "except.h"
#include "assert.h"
#include "bitpack.h"

Except_T Bitpack_Overflow = { "Overflow packing bits" };

/*Bitpack_fitsu
 *Puropse: Checks if an unsinged int can fit within a given unsigned width
 *Parameters: an unsigned int n and an unsigned width
 *Returns: A boolean value 
 *Effects: An error is raised if the width is greater than 64
 */
bool Bitpack_fitsu(uint64_t n, unsigned width){
    assert(width <= 64);
    uint64_t temp = n << (64-width);
    temp = temp >> (64-width);
    return (temp == n) ? 1 : 0;
}

/*Bitpack_fitss
 *Puropse: Checks if an singed int can fit within a given unsigned width
 *Parameters: a signed int n and an unsigned width
 *Returns: A boolean value 
 *Effects: An error is raised if the width is greater than 64
 */
bool Bitpack_fitss(int64_t n, unsigned width){
    assert(width <= 64);
    int64_t temp = n << (64-width);
    temp = temp >> (64-width);
    return (temp == n) ? 1 : 0;       
}

/*Bitpack_getu
 *Puropse: Gets the value in a word at a given lsb
 *Parameters: an unsigned int representing the word,  an unsigned width, and a
              unsignned lsb int.
 *Returns: The value as a uint64_t
 *Effects: An error is raised if the width is greater than 64 or the width+lsb
           is greater than 64
 */
uint64_t Bitpack_getu(uint64_t word, unsigned width, unsigned lsb){
    assert(width <= 64);
    assert(width+lsb <= 64);
    uint64_t getValue = word << (64 - width - lsb);
    getValue = word >> (64 - width);
    return getValue;
}

/*Bitpack_gets
 *Puropse: Gets the value in a word at a given lsb 
 *Parameters: an unsigned int representing the word,  an unsigned width, and a
              unsignned lsb int.
 *Returns: A boolean value 
  *Effects: An error is raised if the width is greater than 64 or the width+lsb
           is greater than 64
 */
int64_t Bitpack_gets(uint64_t word, unsigned width, unsigned lsb){
    assert(width <= 64);
    assert(width+lsb <= 64);
    int64_t getValue = word << (64 - width - lsb);
    getValue = word >> (64 - width);
    return getValue;
}

/*Bitpack_newu
 *Purpose: Update a value specified in a word
 *Parameters: The words and value to add as an uint6_t and the width and lsb as 
             unsigned ints
 *Returns: a new word as an uint64_t
 */
uint64_t Bitpack_newu(uint64_t word, unsigned width, unsigned lsb, 
                      uint64_t value)
{
    if(!Bitpack_fitsu(value, width))
            RAISE(Bitpack_Overflow);
    assert(width <= 64);
    assert(width+lsb <= 64);

    /* Shift value to correct lsb */
    value = value << lsb;
    /*create mask to clear old value to replace */
    uint64_t mask = ~0;
    mask = mask >> lsb;
    mask = mask >> (64-width);
    mask = ~mask;
    /*The bits at lsb are now all zeros */ 
    uint64_t valueToUpdate = mask & word;
    /*The bits at lsb with a given width is updated with value*/
    valueToUpdate = valueToUpdate | value;
    return valueToUpdate;

}

/*Bitpack_news
 *Purpose: Update a value specified in a word
 *Parameters: The words as an uint6_t, the width and lsb as unsigned ints, and
             the value to add as a int64_t
 *Returns: a new word as an uint64_t
 */
uint64_t Bitpack_news(uint64_t word, unsigned width, unsigned lsb,
                      int64_t value)
{
    if(!Bitpack_fitss(value, width) )
            RAISE(Bitpack_Overflow);
    assert(width <= 64);
    assert(width+lsb <= 64);

    /* Shift value to correct lsb */
    value = value << lsb;
    /*create mask to clear old value to replace */
    int64_t mask = ~0;
    mask = mask >> lsb;
    mask = mask << (64-width);
    mask = mask >> (64-width-lsb);
    mask = ~mask;
    /*The bits at lsb are now all zeros */ 
    int64_t valueToUpdate = mask & word;
    /*The bits at lsb with a given width is updated with value*/
    valueToUpdate = valueToUpdate | value;
    return valueToUpdate;
}