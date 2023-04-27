#include "stdtype.h"

/**
 * calculate the length of a string
 * @param str the string to calculate
*/
uint8_t strlen(char* str);
/**
 * compare two strings
 * @param str1 the first string
 * @param str2 the second string
 * @return 0 if the strings are equal, 1 if str1 is greater than str2, -1 if str1 is less than str2
*/
uint8_t strcmp(char* str1, char* str2);

/**
 * copy a string
 * @param dest the destination string
 * @param src the source string
*/
void strcpy(char* dest, char* src);

/**
 * copy a string with a maximum length
 * @param dest the destination string
 * @param src the source string
 * @param len the maximum length
*/
void strncpy(char* dest, char* src, uint8_t len);

/**
 * concatenate two strings
 * @param dest the destination string
 * @param src the source string
*/
void strcat(char* dest, char* src);

/**
 * set a string to a character
 * @param str the string
 * @param c the character
 * @param len the length of the string
*/
void strset(char* str, char c, uint8_t len);