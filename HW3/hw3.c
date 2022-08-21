#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>


extern int next_thread_id;
extern int max_squares;
extern int total_tours;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct processNextMoveArgs {
	int** board;
	int m;
	int n;
	int currMove;
	int currPos[2];
	int seen;
	int current_tid;
};

void searchPotentialMoves(int** board, int* currPos, int m, int n, int* numMoves, int** nextMoves) {
	// (r-2), (c-1)
	if ((((currPos[0] - 2) >= 0 && (currPos[0] - 2) < m)  && ((currPos[1] - 1) >= 0 && (currPos[1] - 1) < n)) && board[currPos[0] - 2][currPos[1] - 1] == 0){
		(*numMoves)++;
		nextMoves[0][0] = currPos[0] - 2;
		nextMoves[0][1] = currPos[1] - 1;
	}
	else {
		nextMoves[0][0] = -1;
		nextMoves[0][1] = -1;
	}
	// (r-1), (c-2)
	if (((currPos[0] - 1 >= 0 && currPos[0] - 1 < m)  && (currPos[1] - 2 >= 0 && currPos[1] - 2 < n)) && board[currPos[0] - 1][currPos[1] - 2] == 0){
		(*numMoves)++;
		nextMoves[1][0] = currPos[0] - 1;
		nextMoves[1][1] = currPos[1] - 2;
	}
	else {
		nextMoves[1][0] = -1;
		nextMoves[1][1] = -1;
	}
	// (r+1), (c-2)
	if ((((currPos[0] + 1) >= 0 && (currPos[0] + 1) < m)  && ((currPos[1] - 2) >= 0 && (currPos[1] - 2) < n)) && board[currPos[0] + 1][currPos[1] - 2] == 0){
		(*numMoves)++; 
		nextMoves[2][0] = currPos[0] + 1;
		nextMoves[2][1] = currPos[1] - 2;
	}
	else{
		nextMoves[2][0] = -1;
		nextMoves[2][1] = -1;
	}
	// (r+2), (c-1)
	if ((((currPos[0] + 2) >= 0 && (currPos[0] + 2) < m)  && ((currPos[1] - 1) >= 0 && (currPos[1] - 1) < n)) && board[currPos[0] + 2][currPos[1] - 1] == 0){
		(*numMoves)++; 
		nextMoves[3][0] = currPos[0] + 2;
		nextMoves[3][1] =  currPos[1] - 1;
	}
	else {
		nextMoves[3][0] = -1;
		nextMoves[3][1] = -1;
	}
	// (r+2), (c+1)
	if ((((currPos[0] + 2) >= 0 && (currPos[0] + 2) < m)  && ((currPos[1] + 1) >= 0 && (currPos[1] + 1) < n)) && board[currPos[0] + 2][currPos[1] + 1] == 0){
		(*numMoves)++; 
		nextMoves[4][0] = currPos[0] + 2;
		nextMoves[4][1] =  currPos[1] + 1;
	}
	else{
		nextMoves[4][0] = -1;
		nextMoves[4][1] = -1;
	}
	// (r+1), (c+2)
	if ((((currPos[0] + 1) >= 0 && (currPos[0] + 1) < m)  && ((currPos[1] + 2) >= 0 && (currPos[1]  + 2) < n)) && board[currPos[0] + 1][currPos[1] + 2] == 0){
		(*numMoves)++; 
		nextMoves[5][0] = currPos[0] + 1;
		nextMoves[5][1] = currPos[1] + 2;
	}
	else{
		nextMoves[5][0] = -1;
		nextMoves[5][1] = -1;
	}
	// (r-1), (c+2)
	if ((((currPos[0] - 1) >= 0 && (currPos[0] - 1) < m)  && ((currPos[1] + 2) >= 0 && (currPos[1] + 2) < n)) && board[currPos[0] - 1][currPos[1] + 2] == 0){
		(*numMoves)++; 
		nextMoves[6][0] = currPos[0] - 1;
		nextMoves[6][1] =  currPos[1] + 2;
	}
	else{
		nextMoves[6][0] = -1;
		nextMoves[6][1] = -1;
	}
	// (r-2), (c+1)
	if ((((currPos[0] - 2) >= 0 && (currPos[0] - 2) < m)  && ((currPos[1] + 1) >= 0 && (currPos[1] + 1) < n)) && board[currPos[0] - 2][currPos[1] + 1] == 0){
		(*numMoves)++; 
		nextMoves[7][0] = currPos[0] - 2;
		nextMoves[7][1] =  currPos[1] + 1;
	}
	else{
		nextMoves[7][0] = -1;
		nextMoves[7][1] = -1; 
	}
	return;
}
//---------------------------------------------------------------------------------------------------------------------
void* processNextMove(void* args){

	struct processNextMoveArgs* args1 = args;

	int** board;
	int m,n; 
	int currMove; 
	int currPos[2];
	int seen;
	int current_tid;

	board = args1->board;
	m = args1->m;
	n = args1->n;
	currMove = args1->currMove;
	currPos[0] = args1->currPos[0];
	currPos[1] = args1->currPos[1];
	seen = args1->seen;	
	if(currMove == 1){
		current_tid = 0;
	}
	else{
		current_tid = args1->current_tid;
	}

	if (currMove == 1){
		printf("MAIN: Solving Sonny's knight's tour problem for a %dx%d board\n", m, n);
		printf("MAIN: Sonny starts at row %d and column %d (move #%d)\n",currPos[0],currPos[1], currMove);
	}

	int numMoves = 0; 

	int** nextMoves = calloc(8, sizeof(int*));

	int i;
	for (i = 0; i < 8; i++){
		nextMoves[i] = calloc(2, sizeof(int));
	}

	searchPotentialMoves(board, currPos, m, n, &numMoves, nextMoves); 

	// In the case that there is only one possible next move 
	if (numMoves == 1){

		free(args1);
		// Find the next move, input it into the board and processNextMove 
		int nextPos[2]; 
		int i;
		for (i = 0; i < 8; i++){
			if (!(nextMoves[i][0] == -1 && nextMoves[i][1] == -1)){
				nextPos[0] = nextMoves[i][0];
				nextPos[1] = nextMoves[i][1];
				break;
			}
		}

		board[nextPos[0]][nextPos[1]] = currMove + 1;

		for (i = 0; i < 8; i++){
			free(nextMoves[i]);
		}
		free(nextMoves);

		struct processNextMoveArgs* args = calloc(1,sizeof(struct processNextMoveArgs));
		args->board = board;
		args->m = m;
		args->n = n;
		args->currMove = currMove + 1;
		args->currPos[0] = nextPos[0];
		args->currPos[1] = nextPos[1];
		args->seen = seen + 1;
		args->current_tid = current_tid;

		processNextMove(args);
	}

	// In the case that there are more than one possible move
	else if (numMoves > 1){

		free(args1);
		
		if(current_tid == 0){
			printf("MAIN: %d possible moves after move #%d; creating %d child threads...\n",numMoves, currMove, numMoves);
		}
		else{
			printf("T%d: %d possible moves after move #%d; creating %d child threads...\n",current_tid, numMoves,currMove, numMoves);
		}

		pthread_t tid[8];

		int i;
		for (i = 0; i < 8; i++){
			if (!(nextMoves[i][0] == -1 && nextMoves[i][1] == -1)){

				// Create a new board for this specific move 
				int** newBoard = calloc(m, sizeof(int*));
				int j,k;
				for (j = 0; j < m; j++){
					newBoard[j] = calloc(n, sizeof(int));
					for (k = 0; k < n; k++){
						newBoard[j][k] = board[j][k];
					}
				}

				newBoard[nextMoves[i][0]][nextMoves[i][1]] = currMove + 1;

				int currPos[2];
				currPos[0] = nextMoves[i][0];
				currPos[1] = nextMoves[i][1];

				struct processNextMoveArgs* args = calloc(1,sizeof(struct processNextMoveArgs));
				args->board = newBoard;
				args->m = m;
				args->n = n;
				args->currMove = currMove + 1;
				args->currPos[0] = currPos[0];
				args->currPos[1] = currPos[1];
				args->seen = seen + 1;
				args->current_tid = next_thread_id;

				pthread_create(&tid[i], NULL, &processNextMove, args);
				pthread_mutex_lock(&mutex);
				next_thread_id++;
				pthread_mutex_unlock(&mutex);
				#ifdef NO_PARALLEL 

					pthread_join(tid[i], NULL);
					if(current_tid == 0){
						printf("MAIN: T%d joined\n", args->current_tid);
					}
					else{
						printf("T%d: T%d joined\n", current_tid, args->current_tid);
					}

				#endif

			}
		}

		#ifndef NO_PARALLEL
			// Wait for all children to be processed
			int j = 1;
			for (i = 0; i < 8; i++){
				
				if (nextMoves[i][0] != -1 && nextMoves[i][1] != -1){
					pthread_join(tid[i], NULL);		
					if(current_tid == 0){
						printf("MAIN: T%d joined\n", current_tid + j );
					}
					else{
						printf("T%d: T%d joined\n", current_tid, current_tid + j);
					}
					j++;
				}
			}
	
		#endif

		for (i = 0; i < 8; i++){
			free(nextMoves[i]);
		}
		free(nextMoves);
	}

	// In the case that there are no more possible moves 
	else if (numMoves == 0){

		//free(args1);

		// In the case that we have visited all squares
		if (seen == m * n){
			printf("T%d: Sonny found a full knight's tour; incremented total_tours\n", current_tid);
			pthread_mutex_lock(&mutex);
			total_tours++;
			max_squares = currMove;
			pthread_mutex_unlock(&mutex);
			// Free the board 

			for (i = 0; i < m; i++){
				free(board[i]);
			}
			free(board);
		}
		// In the case that we have reached a dead end
		else if (seen < m * n){
			if(current_tid == 0){
				printf("MAIN: Dead end at move #%d", currMove);
			}
			else{
				printf("T%d: Dead end at move #%d", current_tid, currMove);
			}
			if (currMove > max_squares){
				pthread_mutex_lock(&mutex);
				max_squares = currMove;
				pthread_mutex_unlock(&mutex);
				printf("; updated max_squares");
			}
			printf("\n");
		}
		
		// Update max_squares if seen is greater
		
		
		for (i = 0; i < 8; i++){
			free(nextMoves[i]);
		}
		free(nextMoves);
	}
	return NULL;
}
//---------------------------------------------------------------------------------------------------------------------

int simulate( int argc, char * argv[] ){
    //check if the number of arguments is correct
    if (argc != 5){
		fprintf(stderr, "ERROR: Invalid argument(s)\n");
		fprintf(stderr, "USAGE: a.out <m> <n> <r> <c>\n");
		return EXIT_FAILURE;	
	}
    int m = atoi(argv[1]);
    int n = atoi(argv[2]);
    //check if the value of m and n is valid
	if (!(m > 2 && n > 2)){
		fprintf(stderr, "ERROR: Invalid argument(s)\n");
		fprintf(stderr, "USAGE: a.out <m> <n> <r> <c>\n");
		return EXIT_FAILURE;	
	}
	
	int row = atoi(argv[3]);
	int col = atoi(argv[4]);
	if(!(row >= 0 && row < m && col >= 0 && col < n)){
		fprintf(stderr, "ERROR: Invalid argument(s)\n");
		return EXIT_FAILURE;
	}
	if(row*col > m*n){
		fprintf(stderr, "ERROR: Invalid argument(s)\n");
		return EXIT_FAILURE;
	}
	//remember to free the board
	int** board = calloc(m, sizeof(int*));
	int i,j;
	for (i = 0; i < m; i++){
		board[i] = calloc(n, sizeof(int));
		for (j = 0; j < n; j++){
			board[i][j] = 0;
		}
	}
	board[row][col] = 1; 

	int currPos[2]; 
	currPos[0] = row;
	currPos[1] = col;

	struct processNextMoveArgs* args = calloc(1,sizeof(struct processNextMoveArgs));

	args->board = board;
	args->m = m;
	args->n = n;
	args->currMove = 1;
	args->currPos[0] = currPos[0];
	args->currPos[1] = currPos[1];
	args->seen = 1;
	args->current_tid = 0;

	processNextMove(args);
	if(total_tours == 0){
		if(max_squares == 1){
			printf("MAIN: Search complete; best solution(s) visited %d square out of %d\n", max_squares, m*n);
		}else{
			printf("MAIN: Search complete; best solution(s) visited %d squares out of %d\n", max_squares, m*n);
		}
	}
	else{
		printf("MAIN: Search complete; found %d possible paths to achieving a full knight's tour\n", total_tours);
	}
	pthread_mutex_destroy(&mutex);

	for (i = 0; i < m; i++){
		free(board[i]);
	}
	free(board);
	return EXIT_SUCCESS;
}