/*
	CSC 360 ASSIGNMENT 1
	Simple Shell Interpreter
	Prof. J. PAN
	UVIC Winter 2018

	Student: Alexander (Lee) DEWEERT
	ID: V00855767
	2018-Jan-29
*/
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

/*Added by me*/
#include <unistd.h>
#include <errno.h>

/*Prototypes*/
char* getWorkingDirectory(int);
void parseInput(char*,char***,size_t*,int*);
int executeCommand(char**,int*);
void addToList( char*, char*, pid_t);
typedef struct LinkedList *node;
void printList();
void checkBackgroundProcesses();

/*Structs*/
struct LinkedList {
	pid_t pid;
	char command[1024];
	char arg1[1024];
	char arg2[1024];
	char arg3[1024];
	struct LinkedList* next;
};

/*Globals*/
node root;
int listCount = 0;

/* Main()
*  Runs an infinite loop asking user for input
*  unless the program has a critical error or the user
*  types "exit" to leave.
*/
int main()
{
	char* prompt;
	char** args;
	size_t argc;
	int bg_flag = 0;
	int running = 1;

	printf("\nWelcome to Simple Shell Interpreter (SSI) v1.0\n");
	printf("Valid commands: [cd] [cd ..] [cd ~] [any binary in /bin/ with unlimited arguments]\n");
	printf("                [exit] [bglist - use \"bg program arg1 arg2 ... argN\"]\n\n");

	do { //run these commands in a loop while...
		checkBackgroundProcesses();
		prompt = getWorkingDirectory(0);
		char* input = readline(prompt);
		bg_flag = 0; //
		if( strcmp(input,"") ) { //If input not empty string
			parseInput(input,&args,&argc,&bg_flag);
			running = executeCommand(args,&bg_flag);

			//Free all resources
			int i;
			for( i = 0; i < argc; i++) {
				free( args[i] );
			}
			free(args);
			free(input);
			free(prompt);
		} 
		else { //input was empty string
			free(input);
			free(prompt);
		}
	} while( running ); //...while running is true (returned by executeCommand)

	printf("Session closed\n");
}

/* checkBackgroundProcesses()
*  here we check the bglist for any nonterminated
*  processes. This is done before every input prompt.
*/
void checkBackgroundProcesses() {
	pid_t w;
	//BACKGROUND PROCESSES (loop checking to remove PID's if terminated)
	if( listCount > 0 ) { //If we had added a "bg" flag to command list count will be > 0
		w = waitpid(0, NULL, WNOHANG); //Waitpid for termination of bg running child
		while( w > 0 ) {
			if( w > 0 ) {
				if ( (*root).pid == w ) { //pid is root
					if( strlen( (*root).arg1) != 0 )
						printf("%d: %s %s has terminated.\n", (int)(*root).pid, (*root).command, (*root).arg1); 
					else
						printf("%d: %s has terminated.\n", (int)(*root).pid, (*root).command); 
					root = (*root).next; 
					listCount--;
				}
				//pid is not root, could be interior or tail
				else {
					node cur;
					node temp;
					cur = root;
					//find pid in list
					while( (*cur).pid != w ) {
						temp = cur;
						cur = (*cur).next;
					}
					//If the next element is NULL we're at tail
					//so delete the tail
					if( (*cur).next == NULL ) {
						free((*temp).next);
						(*temp).next = NULL;
					}
					//else it's an interior element
					// before: [cur] -> [next] -> NULL
					// after:  [cur] -----------> NULL
					// altern: [cur] -----------> [nextnext]
					else {
						(*cur).next = (*(*cur).next).next;	
					}
					listCount--;	
				}
			}
			w = waitpid(0, NULL, WNOHANG);
		}
	}
}

/* executeCommand(char**, int*)
*  This function takes an array of strings and an int flag.
*  The user will enter a command on terminal input which will
*  be interpreted here, and executed if it's a valid input, otherwise
*  an error message will be displayed.
*/
int executeCommand( char** args, int* bg_flag ) {
	pid_t child_pid;
	int runningBinary = 0;
	int status;//of child process
	/*Want to run chdir() since cd is not in bin*/
	if( !strcmp( args[0], "cd" ) ) { //default cd with no args is go home
		char* cwd = getWorkingDirectory(1);
		if( NULL == args[1] || !strcmp(args[1], "~") ) {
			chdir( getenv("HOME") );
		}
		else {
			chdir(args[1]);
		}
		free(cwd);
	}

	else if( !strcmp( args[0], "exit") ) {
		return 0;
	}

	else if( !strcmp( args[0], "bglist") ) {
		printList();
	}

	else { //otherwise were executing a binary
		runningBinary = 1;
		child_pid = fork();
		/*CHILD execution*/
		if ( child_pid == 0 ) {
			execvp( args[0], args );
			/*PROGRAM IS OVERWRITTEN AFTER THIS POINT*/
			/*execvp failed if the following message is printed*/
			printf("Unknown Command (args** invalid).\n");
			exit(EXIT_SUCCESS);
		}
		/*Fork failed*/
		else if( child_pid == -1 ) { 
			printf("Failed to fork\n"); 
			exit(EXIT_FAILURE);
		}
	}
			
	//Add child process to bg list only if bg_flag true
	if( (*bg_flag) && runningBinary ) {
		if( NULL == args[1] ) addToList(args[0], NULL, child_pid);
		else addToList(args[0], args[1], child_pid);
	}

	//NON-BG PROCESSES (will block so we don't need to add to BG list)
	else if (runningBinary) {
		do {
			//else we're running a normal process where we just wait
			waitpid(child_pid, &status, WUNTRACED);
		} while( (!WIFEXITED(status)) && !WIFSIGNALED(status));	
	}
	return 1;
}

/* parseIput( char* )
*  separating command line input into usable arguments
*  likely to be fed into execvp() system call
*  Note on the triple pointers, did it for pass by reference
*  behavior. There was likely better way to do this and it might be
*  improved in future if there's time.
*/
void parseInput( char* input, char*** args, size_t* argc, int* bg_flag ) {

	/*init args to hold one string*/
	(*args) = (char**)malloc( sizeof(char*) );
	/*vars*/
	(*argc) = 1;
	int num_tokens = 0;

	/*Read the string and tokenize it into individual words delimited by spaces*/
	char* token;

	const char* delimeter = " ";

	/*First token*/
	token = strtok( input, delimeter );
	/*Remaining tokens*/
	while( token != NULL ) {
		//printf( "TOKEN IS: %s\n", token );

		//Test if very first token is bg so we can ignore it and set the bg_flag
		//bg_flag will cause executeCommand() to run a child process in the background
		if( !strcmp(token,"bg") && num_tokens == 0 ) { printf("Setting bg_flag to TRUE...\n");(*bg_flag)=1; }
		
		//Otherwise parse out the arguments list as usual
		else {
			//Put each token into the args array
			//Grow args if tokens will exceed the length of array
			if( (num_tokens+1) > (*argc) ) {
				//printf("num_tokens exceeded the size of the argument array\n");
				(*argc)++;
				//printf("size args: %zu\n", (*argc));
				(*args) = (char**)realloc( (*args), sizeof(char*)*(*argc) );
			}
			//printf("attempting to malloc\n");
			(*args)[num_tokens] = (char*)malloc( sizeof(char)*(strlen(token)+1) );
			strncpy( (*args)[num_tokens], token, strlen(token)+1 );
			num_tokens++;
		}
		token = strtok( NULL, delimeter );
	}
	(*argc)++;
	/*Try to null terminate the array of strings*/
	(*args) = (char**)realloc( (*args), sizeof(NULL)*((*argc)) );
	//printf("num_tokens: %d, (*argc): %zu\n", num_tokens, (*argc));
	(*args)[(*argc)-1] = NULL;
}


/* getWorkingDirectory()
*  This function uses the system call getcwd() and appends it
*  to a default "simple shell" prompt. The fn returns a string
*  which is typically printed out in a loop that waits for
*  user input commands.
*/
char* getWorkingDirectory( int returnDirectoryOnly ) {
	char* buf = NULL;
	/*Note that ptr_buf points to the actual string*/
	char* ptr_buf = NULL;
	int allocated = 0;
	size_t buffer_multiplier = 8;
	size_t fullsize = 0;
	const char* promptStart = "SSI: ";
	const char* promptEnd = " > ";
	char* finalPrompt = NULL;

	/*We want the prompt to display a current working directory of any size
	So the loops continues to double the buffer_multipler by 2 on each iteration
	until system command getcwd() is successful. We will know if getcwd fails because
	the string pointer which contains the cwd will remain null if unsucessful*/
	while( ptr_buf == NULL ) {
		buffer_multiplier *= 2;
		ptr_buf = getcwd( buf, buffer_multiplier );
	}
	/**/
	if( !returnDirectoryOnly ){
		/*Calculate size of prompt and initialize*/
		fullsize = strlen(promptStart)+strlen(ptr_buf)+strlen(promptEnd);
		finalPrompt = (char*)malloc( fullsize+1 );
		finalPrompt[0] = '\0';
		/*Buid the prompt string*/
		strncat( finalPrompt, promptStart, fullsize );
		strncat( finalPrompt, ptr_buf, fullsize );
		strncat( finalPrompt, promptEnd, fullsize );
	}
	else {
		fullsize = strlen(ptr_buf);
		finalPrompt = (char*)malloc( fullsize+1 );
		finalPrompt[0] = '\0';
		strncat( finalPrompt, ptr_buf, fullsize );
	}
	free(ptr_buf);
	
	/*Can return the prompt string with cwd or just the cwd ( useful for chdir() )*/
	return finalPrompt;	
}

// node makeNode() {
// 	node temp;
// 	temp = (node)malloc(sizeof(struct LinkedList));
// 	(*temp).next = NULL;
// 	return temp;
// }

void addToList( char* command, char* arg1, pid_t pid ) {

	node temp;
	node cur;

	temp = (node)malloc(sizeof(struct LinkedList));
	(*temp).next = NULL;
	(*temp).pid = pid;
	memcpy((*temp).command, command, strlen(command)+1);
	if( NULL != arg1 ) memcpy((*temp).arg1, arg1, strlen(command)+1);
	
	//(*temp).next = NULL; //might need this if errors

	if( root == NULL ) {
		listCount++;
		root = temp;
		//printf( "was empty, bgList+=%d\n", (int)(*root).pid);
	}
	else {
		listCount++;
		cur = root;
		while( (*cur).next != NULL ) {
			//printf( "bgList+=%d\n", (int)(*cur).pid);
			cur = (*cur).next;
		}
		(*cur).next = temp;
	}
}

void printList() {
	if( listCount == 1 ) { 
		if( strlen( (*root).arg1) != 0 )
			printf( "%d: %s %s\n", (int)(*root).pid, (*root).command, (*root).arg1 );
		else
			printf( "%d: %s\n", (int)(*root).pid, (*root).command ) ;
	}
	else if( listCount > 1 ) {
		node cur;
		cur = root;
		if( strlen( (*cur).arg1) != 0 )
			printf( "%d: %s %s\n", (int)(*cur).pid, (*cur).command, (*cur).arg1 );
		else
			printf( "%d: %s\n", (int)(*cur).pid, (*cur).command );
		while( (*cur).next != NULL ) {
				cur = (*cur).next;
				if( strlen( (*cur).arg1) != 0 )
					printf( "%d: %s %s\n", (int)(*cur).pid, (*cur).command, (*cur).arg1 );
				else
					printf( "%d: %s\n", (int)(*cur).pid, (*cur).command );
			}	
	}
	printf( "Total background jobs: %d\n", listCount );
}