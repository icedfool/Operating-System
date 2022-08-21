#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h> 
#include <ctype.h>
#define MAX_BUFFER_SIZE 128
// remember: NO BRAKETES ALLOWED

char** splitStringIntoArray(char* inString){
    int* indexArray =  (int*)calloc(MAX_BUFFER_SIZE,sizeof(int));
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

/*  a function that takes in a string Array 
    and returns the size of the array 
*/
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

// a helper function that takes in a string Array and a int type
// and frees the memory allocated to the array
void freeCache(char **cache,int cacheSize){
    for(int i = 0; i < cacheSize; i++){
        if(*(cache+i) == NULL){
            continue;
        }
        free(*(cache+i));
    }
    free(cache);
}

int main(int argc, char **argv){
    int fileNum = argc - 1;
    int count = 0;
    int* pipefd = (int*)calloc(2,sizeof(int));
    //create one pipe via the pipe() system call
    int rc = pipe( pipefd );
    //pipe failure check
    if ( rc == -1 ){
        perror( "pipe() failed\n" );
        close(*pipefd);
        close((*pipefd+1));
        free(pipefd);
        return EXIT_FAILURE;
    }
    printf("PARENT: Created pipe successfully\n");
    fflush(stdout);
    char* execArgument = calloc(30,sizeof(char));
    sprintf(execArgument,"%d",*pipefd);
    // at this time pipefd[0] is the read end(3) and 
    // pipefd[1] is the write end(4) of the pipe
    printf("PARENT: Calling fork() to create child process to execute hw2-cache.out...\n");
    fflush(stdout);
    pid_t ppid = fork(); 
    if(ppid == -1){
        perror( "fork() failed" );
        close(*pipefd);
        close((*pipefd+1));
        free(pipefd);
        return EXIT_FAILURE;
    }
    //parent process
    else if(ppid > 0){
        int* pidArray = calloc(argc-1,sizeof(int));
        while(count < fileNum){
            count++;
            //current file name
            char *fileName = *(argv + count);
            //create a child process via the fork() system call
            pid_t p = fork();
            *(pidArray+count-1) = p;
            //fork failure check
            if ( p == -1 ){
                perror( "fork() failed" );
                return EXIT_FAILURE;
            }
            //parent process
            else if ( p > 0 ){
                printf("PARENT: Calling fork() to create child process for \"%s\" file...\n",fileName);
                fflush(stdout);
            }
            // child process
            else{
                FILE* ptr = fopen(fileName, "r");//REMEMBER TO CLOSE THE FILE POINTER
                if(ptr == NULL){
                    perror("ERROR: file does not exist.\n");
                    close(*(pipefd+0));
                    close(*(pipefd+1));
                    free(pipefd);
                    return 2;
                }
                //a counter to keep track of the number of valid words
                int countValidWord = 0;
                //close the read end;
                close(*pipefd);
                char* buf = (char*)calloc(MAX_BUFFER_SIZE, sizeof(char));//REMEMBER TO FREE THIS
                while (fscanf(ptr, "%s", buf) != EOF){
                    //fliter the front and back punctation
                    char *newBuf = fliterFrontAndBackPunctation(buf);//REMEMBER TO FREE THIS WHEN WHILE ENDS
                    //split the string into an array
                    char** splitStrArr = splitStringIntoArray(newBuf);//REMEMBER TO FREE THIS AND THE CONTENTS INSIDE WHEN WHILE ENDS
                    //get the size of the array
                    int cacheSize = getCacheSize(splitStrArr);
                    // for each valid word in the array, add a '.' at the end of the word
                    // and then write the word to the pipe
                    for (int i = 0; i < cacheSize; i++){
                        if(*(splitStrArr+i) == NULL){
                            continue;
                        }
                        // skip the string that is onlu 1 character long
                        else if(strlen(*(splitStrArr+i)) == 1){
                            continue;
                        }
                        //add a '.' at the end of the word
                        char *temp = (char*)calloc(MAX_BUFFER_SIZE, sizeof(char));//REMEMBER TO FREE THIS
                        temp = strcpy(temp,*(splitStrArr+i));
                        temp = strcat(temp,".");
                        //dont forget the '\0'
                        *(temp+strlen(temp)) = '\0';
                        //write the word to the pipe
                        write(*(pipefd+1),temp,strlen(temp));
                        countValidWord++;
                        //free the memory allocated to the temp variable
                        free(temp);
                    }
                    //free the memory allocated to the splitStrArr variable
                    freeCache(splitStrArr,cacheSize);
                    //free the memory allocated to the newBuf variable
                    free(newBuf);
                }
                if(countValidWord == 0){
                    printf("CHILD: Successfully wrote %d words on the pipe\n",countValidWord);
                    fclose(ptr);
                    free(buf);
                    close(*(pipefd+0));
                    close(*(pipefd+1));
                    free(pipefd);
                    return 3;
                }
                printf("CHILD: Successfully wrote %d words on the pipe\n",countValidWord);
                fflush(stdout);
                fclose(ptr);
                free(buf);
                close(*(pipefd+0));
                close(*(pipefd+1));
                free(pipefd);
                return EXIT_SUCCESS;
                // remember we might need to return 1 in some situation
            }
        }
        int* statusArray = calloc(argc-1,sizeof(int));
        for (int i = 0; i < argc-1; i++){
            int returnStatus;
            waitpid(*(pidArray+i), &returnStatus, 0);
            *(statusArray + i) = returnStatus;
        }
        for(int i = 0; i < argc-1; i++){
            int exitStatus = WEXITSTATUS(*(statusArray + i) );
            if(exitStatus == 0 || exitStatus == 2 || exitStatus == 3){
                printf("PARENT: Child process terminated with exit status %d\n",exitStatus);
                fflush(stdout);
            }
            else{
                printf("PARENT: Child process terminated abnormally\n");
                fflush(stdout);
            }
        }
        close(*pipefd);
        close(*(pipefd+1));
        free(pipefd);
        free(pidArray);
        free(statusArray);
        int returnStatusExecl;
        waitpid(ppid, &returnStatusExecl, 0);
        int exitStatusEecl = WEXITSTATUS(returnStatusExecl);
        if(exitStatusEecl == 0){
            printf("PARENT: Child running hw2-cache.out terminated with exit status %d\n",exitStatusEecl);
            fflush(stdout);
        }
        else{
            printf("PARENT: Child running hw2-cache.out terminated abnormally\n");
            fflush(stdout);
        }
    }
    //child process
    else{
        close(*(pipefd+1));
        execl("hw2-cache.out","hw2-cache.out",execArgument,NULL);
    }
    free(execArgument);
    return EXIT_SUCCESS;
}