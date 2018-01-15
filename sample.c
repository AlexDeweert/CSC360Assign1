#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

/*Added by me*/
#include <unistd.h>
#include <errno.h>

/*Prototypes*/
char* getWorkingDirectory();
void parseInput(char*);

int main()
{
	char* prompt;

	int bailout = 0;
	while (!bailout) {
		prompt = getWorkingDirectory();
		char* input = readline(prompt);
		/* Note that readline strips away the final \n */

		if (!strcmp(input, "bye")) {
			bailout = 1;
		}
		else {
			parseInput(input);
			//printf("%s: command not found\n", input);
		}
		
		free(input);
		free(prompt);
		
	}
	//free(ptr_buf);
	printf("Bye Bye\n");
}

/* parseIput( char* )
*  separating command line input into usable arguments
*  likely to be fed into execvp() system call
*/
void parseInput( char* input ) {

	/*vars*/
	size_t size_args = 1;
	int num_tokens = 0;

	/*Read the string and tokenize it into individual words delimited by spaces*/
	char* token;
	char** args = (char**)malloc( sizeof(char*)*size_args );

	const char* delimeter = " ";

	/*First token*/
	token = strtok( input, delimeter );
	/*Remaining tokens*/
	while( token != NULL ) {
		printf( "TOKEN IS: %s\n", token );

		//Put each token into the args array
		//Grow args if tokens will exceed the length of array
		if( (num_tokens+1) > size_args ) {
			printf("num_tokens exceeded the size of the argument array\n");
			size_args++;
			printf("size args: %zu\n", size_args);
			args = (char**)realloc( args, sizeof(char*)*size_args );
		}
		printf("attempting to malloc\n");
		args[num_tokens] = (char*)malloc( sizeof(char)*(strlen(token)+1) );
		strncpy( args[num_tokens], token, strlen(token)+1 );
		num_tokens++;
		token = strtok( NULL, delimeter );
	}
	size_args++;
	/*Try to null terminate the array of strings*/
	args = (char**)realloc( args, sizeof(NULL)*(size_args) );
	printf("Size of null is: %zu, size of char* is: %zu, num_tokens: %d, size_args: %zu\n", sizeof(NULL), sizeof(char*), num_tokens, size_args);
	args[size_args-1] = NULL;

	int i;
	for( i = 0; i < num_tokens; i++ ) {
		printf("args at %d is: %s with size %zu\n", i, args[i], strlen( args[i]));
	}

	//Determine if the very first word is a "valid" command - ie will be args[0]
	if( strcmp( args[0], "ls" ) == 0) {
		printf("Got command \"ls\"\n");
	}

	

	//args[1...n-1] will be the options

	//args[n] = NULL to indicate end of args

	for( i = 0; i < size_args; i++) {
		free( args[i] );
	}
	free( args );
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
