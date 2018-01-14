#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

/*Added by me*/
#include <unistd.h>
#include <errno.h>

/*Prototypes*/
char* getWorkingDirectory();

int main()
{
	char* prompt;

	int bailout = 0;
	while (!bailout) {
		prompt = getWorkingDirectory();
		char* reply = readline(prompt);
		/* Note that readline strips away the final \n */

		if (!strcmp(reply, "bye")) {
			bailout = 1;
		}
		else if ( !strcmp(reply, "pwd")) {
			getWorkingDirectory();
		}
		else {
			printf("\nYou said: %s\n\n", reply);
		}
		
		free(reply);
		free(prompt);
		
	}
	//free(ptr_buf);
	printf("Bye Bye\n");
}

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
	//printf("\nptr_buf size: %zu\nbuf size: %zu\n", strlen(ptr_buf), sizeof(buf));
	//strncpy( cwd, ptr_buf, sizeof(buf) );
	free(ptr_buf);
	return finalPrompt;
	//free(finalPrompt);
}
