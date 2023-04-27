#include "lib-header/string.h"

uint8_t strlen(char* str){
    int i = 0;
    while(str[i] != '\0'){
        i++;
    }
    return i;
}

uint8_t strcmp(char* str1, char* str2){
    int i = 0;
    while(str1[i] != 0 && str2[i] != 0){
        if(str1[i] > str2[i]){
            return 1;
        }else if(str1[i] < str2[i]){
            return -1;
        }
        i++;
    }
    if(str1[i] == 0 && str2[i] == 0){
        return 0;
    }else if(str1[i] == 0){
        return -1;
    }else{
        return 1;
    }
}

void strcpy(char* dest, char* src){
    int i = 0;
    while(src[i] != '\0'){
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void strncpy(char* dest, char* src, uint8_t len){
    int i = 0;
    while(src[i] != '\0' && i < len){
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void strcat(char* dest, char* src){
    int i = 0;
    while(dest[i] != '\0'){
        i++;
    }
    int j = 0;
    while(src[j] != '\0'){
        dest[i] = src[j];
        i++;
        j++;
    }
    dest[i] = '\0';
}

void strset(char* str, char c, uint8_t len){
    int i = 0;
    while(i < len){
        str[i] = c;
        i++;
    }
    str[i] = '\0';
}