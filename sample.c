#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

/*Added by me*/
#include <unistd.h>
#include <errno.h>

/*Prototypes*/
char* getWorkingDirectory();
void parseInput(char*,char***,size_t*);
void executeCommand(char**);

int main()
{
	char* prompt;
	char** args;
	size_t argc;

	int bailout = 0;
	while (!bailout) {
		prompt = getWorkingDirectory();
		char* input = readline(prompt);
		/* Note that readline strips away the final \n */

		if (!strcmp(input, "logout")) {
			bailout = 1;
		}
		else {
			parseInput(input,&args,&argc);
			executeCommand( args );

			/*REFACTOR THIS LATER but useful for the moment*/
			int i;
			// for( i = 0; i < argc; i++ ) {
			// 	printf("[LOG] args at %d is: %s with size %zu\n", i, args[i], strlen( args[i]));
			// }
			/*Free the args (might do in fn later)*/
			for( i = 0; i < argc; i++) {
				free( args[i] );
			}
			free( args );
			//printf("%s: command not found\n", input);
		}
		
		free(input);
		free(prompt);
		
	}
	printf("Session closed\n");
}

void executeCommand( char** args ) {

	pid_t child_pid;
	int status;//of child process
		child_pid = fork();
	if( child_pid == -1 ) { 
		printf("failed to fork\n"); 
		exit(EXIT_FAILURE);
	}
	if ( child_pid == 0 ) { //Child executes this
		printf("Forked, attempting to call execvp()...\n");
		execvp( args[0], args );
		/*execvp failed if this point is reached*/
			printf("Unknown command or args** is an invalid format...\n");
			exit(EXIT_SUCCESS);
	}
	
	else { //Parent executs this
		printf("PARENT process is running...\n");

		/*Example from man pages for "wait"(if this works)*/
		do {
			pid_t w = wait(&status);
			if (w == -1) {
               perror("waitpid");
               exit(EXIT_FAILURE);
           }
           if (WIFEXITED(status)) {
               printf("exited, status=%d\n", WEXITSTATUS(status));
               //exit(EXIT_SUCCESS);
           } 
	} while ( !WIFEXITED(status) );//&& !WIFSIGNALED(status) );
	
	} 
}

/* parseIput( char* )
*  separating command line input into usable arguments
*  likely to be fed into execvp() system call
*  Note on the triple pointers, did it for pass by reference
*  behavior. There was likely better way to do this and it might be
*  improved in future if there's time.
*/
void parseInput( char* input, char*** args, size_t* argc ) {

	/*init args to hold one string*/
	(*args) = (char**)malloc( sizeof(char*) );
	/*vars*/
	(*argc) = 1;
	int num_tokens = 0;

	/*Read the string and tokenize it into individual words delimited by spaces*/
	char* token;
	//char** args = (char**)malloc( sizeof(char*)*(*argc) );

	const char* delimeter = " ";

	/*First token*/
	token = strtok( input, delimeter );
	/*Remaining tokens*/
	while( token != NULL ) {
		//printf( "TOKEN IS: %s\n", token );

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
		token = strtok( NULL, delimeter );
	}
	(*argc)++;
	/*Try to null terminate the array of strings*/
	(*args) = (char**)realloc( (*args), sizeof(NULL)*((*argc)) );
	printf("num_tokens: %d, (*argc): %zu\n", num_tokens, (*argc));
	(*args)[(*argc)-1] = NULL;
}


/* getWorkingDirectory()
*  This function uses the system call getcwd() and appends it
*  to a default "simple shell" prompt. The fn returns a string
*  which is typically printed out in a loop that waits for
*  user input commands.
*/
char* getWorkingDirectory() {
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
	while( !allocated ) {
		/*If the CWD string is longer than the first buffer size, reallocate*/
		if(  ptr_buf == NULL ) {
			buffer_multiplier *= 2;
			ptr_buf = getcwd( buf, buffer_multiplier );
		}
		else {
			/*The string finalPrompt has been verified to be the correct size
			and it is indeed null terminated*/
			fullsize = strlen(promptStart)+strlen(ptr_buf)+strlen(promptEnd);
			finalPrompt = (char*)malloc( fullsize+1 );
			size_t i;
			/*init finalPrompt array to null to avoid valgrind warnings*/
			finalPrompt[0] = '\0';
			/*Buid the prompt string*/
			strncat( finalPrompt, promptStart, fullsize );
			strncat( finalPrompt, ptr_buf, fullsize );
			strncat( finalPrompt, promptEnd, fullsize );
			/*The buffer is large enough to hold the cwd string; exit loop*/
			allocated = 1; 
		}
	}
	free(ptr_buf);
	return finalPrompt;
}
