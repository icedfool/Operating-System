#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/select.h>
#include <stdbool.h>

#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024


// read the file line by line and store it in the format of string array,
// return the number of lines in the file(pass by reference)
char** read_file(char* filename, int max_buffer_size, int *count_words) {
    FILE * fp =  fopen(filename, "r");
    char * buffer = calloc(max_buffer_size, sizeof(char));    
    while(fgets(buffer, max_buffer_size, fp)){
        (*count_words)++;
    }
    fseek(fp, 0, SEEK_SET);
    char ** words = calloc(*count_words, sizeof(char*));
    for(int i = 0; i < *count_words; i++) {
        words[i] = calloc(max_buffer_size, sizeof(char));
    }
    int i = 0;
    while(fgets(buffer, max_buffer_size, fp)) {
        words[i] = strcpy(words[i], buffer);
        i++;
    }
    fclose(fp);
    free(buffer);
    return words;
}

// a fuction to show how many characters are on the correct place in the word
int correct_position_count(char* secrect_word, char* guess_word) {
    //make the secrect word to lower case
    int length = strlen(secrect_word);
    char* secrect_word_lower = calloc(length, sizeof(char));
    for(int i = 0; i < length; i++) {
        secrect_word_lower[i] = tolower(secrect_word[i]);
    }
    //make the guess word to lower case
    int length_guess = strlen(guess_word);
    char* guess_word_lower = calloc(length_guess, sizeof(char));
    for(int i = 0; i < length_guess; i++) {
        guess_word_lower[i] = tolower(guess_word[i]);
    }
    //count the number of characters on the correct place
    int count = 0;
    for(int i = 0; i < length; i++) {
        if(secrect_word_lower[i] == guess_word_lower[i]) {
            count++;
        }
    }
    free(secrect_word_lower);
    free(guess_word_lower);
    return count;
}

// a function how many character the user guessed is in the word but not in the correct place
int correct_character_count(char* secrect_word, char* guess_word) {
    //make the secrect word to lower case
    int length = strlen(secrect_word);
    char* secrect_word_lower = calloc(length, sizeof(char));
    for(int i = 0; i < length; i++) {
        secrect_word_lower[i] = tolower(secrect_word[i]);
    }
    //make the guess word to lower case
    int length_guess = strlen(guess_word);
    char* guess_word_lower = calloc(length_guess, sizeof(char));
    for(int i = 0; i < length_guess; i++) {
        guess_word_lower[i] = tolower(guess_word[i]);
    }
    //count the number of characters showed in the word but not in the correct place
    int count = 0;
    int* all_char_secret = calloc(26, sizeof(int));
    for(int i = 0; i < 26; i++) {
        all_char_secret[i] = 0;
    }
    int* all_char_guess = calloc(26, sizeof(int));
    for(int i = 0; i < 26; i++) {
        all_char_guess[i] = 0;
    }
    for(int i = 0; i < length; i++) {
        all_char_secret[secrect_word_lower[i] - 'a'] += 1;
    }
    for(int i = 0; i < length_guess; i++) {
        all_char_guess[guess_word_lower[i] - 'a'] += 1;
    }
    for(int i = 0; i < 26; i++) {
        if(all_char_guess[i] > all_char_secret[i]) {
            count += all_char_secret[i];
        }
        else if(all_char_guess[i] < all_char_secret[i]) {
            count += all_char_guess[i];
        }
        else if (all_char_guess[i] == all_char_secret[i]) {
            count += all_char_guess[i];
        }
    }
    free(all_char_secret);
    free(all_char_guess);
    free(secrect_word_lower);
    free(guess_word_lower);
    return count;
}

int main(int argc, char* argv[]){
    int randseed = atoi(argv[1]);
    
    unsigned int port = atoi(argv[2]);
    if(port < 1024 || port > 65535){
        printf("Invalid port number\n");
        return EXIT_FAILURE;
    }

    char* file_name = argv[3];
    int max_buffer_size = atoi(argv[4]);

    int word_count = 0;
    char** dictionary_array = read_file(file_name, max_buffer_size, &word_count);
    srand(randseed);
    int random_index = rand() % word_count;
    
    char* secret_word = calloc(max_buffer_size, sizeof(char));
    strcpy(secret_word, dictionary_array[random_index]);
    for(int i = 0; i < strlen(secret_word); i++) {
        secret_word[i] = tolower(secret_word[i]);
    }
    secret_word[strlen(secret_word) - 1] = '\0';
    
    printf("The secret word is: %s\n", secret_word);
    printf("The size of the secret word is: %ld\n", strlen(secret_word));
    //=============================from above we can get the secret word


    fd_set readfds;
    int client_sockets[ MAX_CLIENTS ]; /* client socket fd list */
    int client_socket_index = 0;  /* next free spot */

    /* Create the listener socket as TCP socket */
    /*   (use SOCK_DGRAM for UDP)               */
    int sock = socket( PF_INET, SOCK_STREAM, 0 );
        /* note that PF_INET is protocol family, Internet */

    if ( sock < 0 )
    {
        perror( "socket()" );
        exit( EXIT_FAILURE );
    }

    /* socket structures from /usr/include/sys/socket.h */
    struct sockaddr_in server;
    struct sockaddr_in client;

    server.sin_family = PF_INET;
    server.sin_addr.s_addr = INADDR_ANY;


    /* htons() is host-to-network-short for marshalling */
    /* Internet is "big endian"; Intel is "little endian" */
    server.sin_port = htons( port );
    int len = sizeof( server );

    if ( bind( sock, (struct sockaddr *)&server, len ) < 0 )
    {
        perror( "bind()" );
        exit( EXIT_FAILURE );
    }

    listen( sock, 5 );  /* 5 is number of waiting clients */

    int fromlen = sizeof( client );

    char buffer[ BUFFER_SIZE ];
    memset( buffer, 0, BUFFER_SIZE );
    int i, n;
    
    // initialize the client names array, notice that the first client
    // has the socket fd of 4.
    char **name_of_clients = calloc(MAX_CLIENTS, sizeof(char*));
    for (i = 0; i < MAX_CLIENTS; i++) {
        name_of_clients[i] = calloc(BUFFER_SIZE, sizeof(char));
    }
    
    while ( 1 ){

        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 500;  /* 2 seconds AND 500 microseconds */
   
        FD_ZERO( &readfds );
        FD_SET( sock, &readfds );   /* listener socket, fd 3 */

        /* initially, this for loop does nothing; but once we have */
        /*  client connections, we will add each client connection's fd */
        /*   to the readfds (the FD set) */
        for ( i = 0 ; i < client_socket_index ; i++ ){
            FD_SET( client_sockets[ i ], &readfds );
        }

        //the blocking call that waits for the client to connect
        select( FD_SETSIZE, &readfds, NULL, NULL, &timeout );
        //remember to comment this later
        //printf( "select() identified %d descriptor(s) with activity\n", ready );

        /* is there activity on the listener descriptor? */
        if ( FD_ISSET( sock, &readfds ) ){
            int newsock = accept( sock,
                                    (struct sockaddr *)&client,
                                    (socklen_t *)&fromlen );
                    /* this accept() call we know will not block */
            //printf( "Accepted client connection\n" );
            client_sockets[ client_socket_index++ ] = newsock;
            int n = send( newsock, "Welcome to Guess the Word, please enter your username.\n", 55, 0 );
            bool valid_name_found = false;
            while(valid_name_found == false) {
                n = recv( newsock, buffer, BUFFER_SIZE - 1, 0 );
                //when we found a valid username, we will add it to the name_of_clients array, and quit the loop
                if ( n < 0 ){
                    perror( "recv()" );
                    break;
                }
                //quite useless here, move to another part.
                else if ( n == 0 ){
                    int k;
                    close( newsock );

                    /* remove fd from client_sockets[] array: */
                    for ( k = 0 ; k < client_socket_index ; k++ ){
                        if ( newsock == client_sockets[ k ] ){
                        /* found it -- copy remaining elements over fd */
                            int m;
                            if(k == client_socket_index - 1){
                                name_of_clients[k][0] = '\0';
                                client_socket_index--;
                                break;
                            }
                            else{
                                for ( m = k ; m < client_socket_index - 1 ; m++ ){
                                    client_sockets[ m ] = client_sockets[ m + 1 ];
                                    strcpy(name_of_clients[ m ] ,name_of_clients[ m + 1 ]);
                                }
                                client_socket_index--;
                                break;
                            }
                        }
                    }
                    
                }
                else{
                    buffer[ n ] = '\0';
                    char *username = calloc(BUFFER_SIZE, sizeof(char));
                    for(int i = 0; i<BUFFER_SIZE; i++){
                        username[i] = tolower(buffer[i]);
                    }
                    //make the last character of the username '\0'
                    username[strlen(username) - 1] = '\0';

                    for(int i = 0; i < MAX_CLIENTS; i++){
                        if(strcmp(username ,name_of_clients[i]) == 0){
                            sprintf(buffer, "Username %s is already taken, please enter a different username\n", username);
                            buffer[strlen(buffer)] = '\0';
                            send( newsock, buffer, (int)strlen(buffer), 0 );
                            free(username);
                            valid_name_found = false;
                            break;
                        }else {
                            valid_name_found = true; 
                        }

                    }
                    while (valid_name_found == true){
                        strcpy(name_of_clients[client_socket_index - 1], username);
                        free(username);
                        //convert this to one string
                        sprintf(buffer, "Let's start playing, %s\n", name_of_clients[client_socket_index - 1]);
                        buffer[strlen(buffer)] = '\0';
                        send(newsock, buffer, (int)strlen(buffer), 0);
                        //and remember to notify him the game status
                        sprintf(buffer, "There are %d player(s) playing. The secret word is %d letter(s).\n", client_socket_index, (int)strlen(secret_word));
                        buffer[strlen(buffer)] = '\0';
                        send(newsock, buffer, (int)strlen(buffer), 0);
                        break;
                    }                    
                }
            }
        }


        /* is there activity on any of the established connections? */
        for ( i = 0 ; i < client_socket_index ; i++ ){
            int fd = client_sockets[ i ];

            if ( FD_ISSET( fd, &readfds ) ){
                
                /* can also use read() and write() */
                n = recv( fd, buffer, BUFFER_SIZE - 1, 0 );
                    /* we know this recv() call will not block */

                if ( n < 0 ){
                    perror( "recv()" );
                    break;
                }
                else if ( n == 0 ){
                    int k;
                    //printf( "Client on fd %d closed connection\n", fd );
                    close( fd );

                    /* remove fd from client_sockets[] array: */
                    for ( k = 0 ; k < client_socket_index ; k++ ){
                        if ( fd == client_sockets[ k ] ){
                        /* found it -- copy remaining elements over fd */
                            int m;
                            if(k == client_socket_index - 1){
                                name_of_clients[k][0] = '\0';
                            }
                            for ( m = k ; m < client_socket_index - 1 ; m++ ){
                                client_sockets[ m ] = client_sockets[ m + 1 ];
                                strcpy(name_of_clients[ m ] ,name_of_clients[ m + 1 ]);
                            }
                            client_socket_index--;
                            break;  /* all done */
                        }
                    }
                }
                else{
                    buffer[n] = '\0';
                    char *guess_word = calloc(BUFFER_SIZE, sizeof(char));
                    strcpy(guess_word, buffer);
                    guess_word[strlen(guess_word) - 1] = '\0';
                    if(strlen(guess_word) != strlen(secret_word)){
                        sprintf(buffer, "Invalid guess length. The secret word is %ld letter(s).\n",strlen(secret_word));
                        buffer[strlen(buffer)] = '\0';
                        send(fd, buffer, (int)strlen(buffer), 0);
                        free(guess_word);
                        break;
                    }
                    //if statement to check if the guess word is correct
                    if(strcmp(guess_word,secret_word) == 0){
                        sprintf(buffer, "%s has correctly guessed the word %s\n", name_of_clients[i], secret_word);
                        buffer[strlen(buffer)] = '\0';
                        for ( i = 0 ; i < client_socket_index ; i++ ){
                            int fd = client_sockets[ i ];
                            send(fd, buffer, (int)strlen(buffer), 0);
                        }

                        //close all the clients==================================================================
                        for (int i = 0; i < client_socket_index; i++){
                            int fd = client_sockets[ i ];
                            close(fd);
                            memset(buffer, 0, BUFFER_SIZE);
                            memset(name_of_clients[i], 0, BUFFER_SIZE);
                        }
                        client_socket_index = 0;
                        random_index = rand() % word_count;
                        strcpy(secret_word, dictionary_array[random_index]);
                        secret_word[strlen(secret_word) - 1] = '\0';
                        printf("The secret word is: %s\n", secret_word);
                        printf("The size of the secret word is: %ld\n", strlen(secret_word));
                        break;
                    }
                    int correct_character_guess = correct_character_count(guess_word, secret_word);
                    int correct_position_guess = correct_position_count(guess_word, secret_word);
                    sprintf(buffer,"%s guessed %s: %d letter(s) were correct and %d letter(s) were correctly placed.\n",
                            name_of_clients[i], guess_word, correct_character_guess, correct_position_guess);
                    buffer[strlen(buffer)] = '\0';
                    for ( i = 0 ; i < client_socket_index ; i++ ){
                        send( client_sockets[ i ], buffer, (int)strlen(buffer), 0 );
                    }
                    free(guess_word);
                }
            }
        }
    }
    return EXIT_SUCCESS;
}