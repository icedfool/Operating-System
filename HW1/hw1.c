#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h> 
#include<ctype.h>
#define MAX_BUFFER_SIZE 128
// remember: NO BRAKETES ALLOWED

char* filterPunctuation(char *str){
    int strLen = strlen(str);
    char *newStr = (char*)calloc(MAX_BUFFER_SIZE,sizeof(char));
    int newStrLen = 0;
    for(int i = 0; i < strLen; i++){
        if(!ispunct(*(str + i))){
            *(newStr + newStrLen) = *(str + i);
            newStrLen++;
        }
    }
    *(newStr + newStrLen) = '\0';
    return newStr;
}

// a function that takes in a string and
// returns the sum of ASCII nums as the hash 
// value of the string
int hashStringIndex(char *str,int cacheSize){
    int strLen = strlen(str);
    int hashResult = 0;
    for(int i = 0; i < strLen; i++){
        hashResult += *(str + i);
    }
    int index = hashResult % cacheSize;
    return index;
}

char** splitStringIntoArray(char* inString){
    int* indexArray =  (int*)calloc(MAX_BUFFER_SIZE,sizeof(int));//REMEMBER TO FREE THIS
    *(indexArray+0) = -1;
    int currentindex = 1;
    for (int i = 0; i < strlen(inString); i++){
        int temp = *(inString + i);
        if(temp >= 48 && temp <= 57){
            continue;
        }
        else if(temp >= 65 && temp <= 90){
            continue;
        }
        else if(temp >= 97 && temp <= 122){
            continue;
        }
        else if(temp < 0){
            *(indexArray+currentindex) = i;
            currentindex++;
            continue;
        }
        else{
            *(indexArray+currentindex) = i;
            currentindex++;
        }
    }

    *(indexArray+currentindex) = strlen(inString);

    int arraySize = currentindex + 1;

    char** tempCache =  (char**)calloc(MAX_BUFFER_SIZE,sizeof(char*));//REMEMBER TO FREE THIS
    int j = 0;
    for (int i = 0; i < arraySize-1; i++){
        int copySize = *(indexArray+i+1) - *(indexArray+i) - 1;
        if(copySize <= 0){
            continue;
        }
        *(tempCache+j) =  (char*)calloc(MAX_BUFFER_SIZE,sizeof(char));
        *(tempCache+j) = strncpy(*(tempCache+j),inString+*(indexArray+i)+1,copySize);
        j++;
    }
    //free indexArray and contents inside
    free(indexArray);
    return tempCache;
}

// a function that takes in a string Array 
// and returns the size of the array
int getCacheSize(char** inCache){
    int size = 0;
    for (int i = 0; i < MAX_BUFFER_SIZE; i++){
        if(*(inCache+i) == NULL){
            break;
        }
        size++;
    }
    return size;
}

// a function that takes in a string and print 
// all the contents in the array
void printCacheContents(char **cache,int cacheSize){
    for(int i = 0; i < cacheSize; i++){
        if(*(cache+i) == NULL){
            continue;
        }
        printf("Cache index %d ==> \"%s\"\n",i,*(cache+i));
    }
}

// a function that takes in a string and fliter all the Punctuation
// at front and back of the string and returns the string without these
// Punctuation
char * fliterFrontAndBackPunctation(char *str){
    int strLen = strlen(str);
    char *newStr = (char *)calloc(MAX_BUFFER_SIZE,sizeof(char));
    int breakIndex = 0;
    for (int i = 0; i < strLen; i++){
        if(!ispunct(*(str + i))){
            break;
        }
        breakIndex++;
    }
    newStr = strcpy(newStr,str + breakIndex);
    strLen = strlen(newStr);
    for (int i = strLen-1; i >= 0; --i){
        if(!ispunct(*(newStr + i))){
            break;
        }
        *(newStr + i) = '\0';
    }
    return newStr;
}

void freeCache(char **cache,int cacheSize){
    for(int i = 0; i < cacheSize; i++){
        if(*(cache+i) == NULL){
            continue;
        }
        free(*(cache+i));
    }
    free(cache);
}

int main(int argc, char** argv){
    //the first argument is the cache size
    int cacheSize = atoi(*(argv+1));
    //the second argument is the file name
    FILE* ptr = fopen(*(argv+2), "r");//remember to close the file pointer
    //initialize the cache array
    //check if the cachesize is valid and if the argument passed in is a number
    if(argc != 3){
        fprintf(stderr,"Error: Invalid argument numbers.\n");
        fclose(ptr);
        return EXIT_FAILURE;
    }
    if(cacheSize <= 0){
        fprintf(stderr,"Error: Cache size is invalid\n");
        fclose(ptr);
        return EXIT_FAILURE;
    }
    //warn the user if the file is not found
    if (ptr == NULL) {
        fprintf(stderr,"Error: no such file.\n");
        fclose(ptr);
        return EXIT_FAILURE;
    }
    char** cacheArray =  (char**)calloc(cacheSize, sizeof(char*));//remember to free this
    // we need to consider about three cases:
    // 1. the index position in the cache is empty, then we need to 
    // allocate a new string for it.
    // 2. the index position in the cache is not empty, but the string
    // size is different from the current string, then we need to reallocte
    // a new string for it.
    // 3. the index position in the cache is not empty, and the string
    // size is the same as the current string, then we need to do nothing.

    char* buf = (char*)calloc(MAX_BUFFER_SIZE, sizeof(char));//REMEMBER TO FREE THIS
    //buf holds the current string
    while (fscanf(ptr, "%s", buf) != EOF) {
        // get a string that has fliterd Punctuation at front and back
        char* newStr = fliterFrontAndBackPunctation(buf);//REMEMBER TO FREE THIS
        // split the string into an array
        char** splitStrArr = splitStringIntoArray(newStr);//REMEMBER TO FREE THIS AND THE CONTENTS INSIDE
        // store the size of the tempCacheArray
        int tempCacheSize = getCacheSize(splitStrArr);
        for (int i = 0; i < tempCacheSize; i++){
            char * tempStr = filterPunctuation(*(splitStrArr+i));
            int stringIndex = hashStringIndex(tempStr,cacheSize);
            if(strlen(tempStr) == 1){
                free(tempStr);
                continue;
            }
            //case 1
            if(*(cacheArray+stringIndex) == NULL){
                char* mode = "calloc";
                *(cacheArray+stringIndex) = (char *)calloc(MAX_BUFFER_SIZE,sizeof(char));
                *(cacheArray+stringIndex) = strcpy(*(cacheArray+stringIndex),tempStr);
                //set the last character of current string to '\0'
                *(*(cacheArray + stringIndex) + strlen(tempStr)) = '\0';
                printf("Word \"%s\" ==> %d (%s)\n", tempStr, stringIndex, mode);
            }
            else if(strlen(*(cacheArray+stringIndex)) != strlen(*(splitStrArr+i))){
                char* mode = "realloc";
                *(cacheArray+stringIndex) = (char *)realloc(*(cacheArray+stringIndex), MAX_BUFFER_SIZE);
                *(cacheArray+stringIndex) = strcpy( *(cacheArray+stringIndex),tempStr);
                *(*(cacheArray + stringIndex) + strlen(tempStr)) = '\0';
                printf("Word \"%s\" ==> %d (%s)\n", tempStr, stringIndex, mode);
            }
            else if (strlen(*(cacheArray+stringIndex)) == strlen(tempStr)){
                char * mode = "nop";
                *(cacheArray+stringIndex) = strcpy(*(cacheArray+stringIndex),tempStr);
                //set the last character of current string to '\0'
                *(*(cacheArray + stringIndex) + strlen(tempStr)) = '\0';
                printf("Word \"%s\" ==> %d (%s)\n", tempStr, stringIndex, mode);
            }
            free(tempStr);
        } 
        free(newStr);
        freeCache(splitStrArr,tempCacheSize);
    }
    //print the cache contents
    printCacheContents(cacheArray,cacheSize);
    //free the cache array
    freeCache(cacheArray,cacheSize);
    free(buf);
    //close the file
    fclose(ptr);
    return EXIT_SUCCESS;
}