//TODO BGProcess loop check
//linked list find && delete


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
void addToList( char*, pid_t);
typedef struct LinkedList *node;
node makeNode();
void printList();

/*Structs*/
struct LinkedList {
	pid_t pid;
	char command[1024];
	struct LinkedList* next;
};

/*Globals*/
node root;
int listCount = 0;
pid_t simPID = 1;

int main()
{
	char* prompt;
	char** args;
	size_t argc;
	int bg_flag = 0;
	int running;
	
	/*Example struct init*/
	// bg_process bgp1;
	// bgp1.pid = (pid_t)5555;
	// bgp1.command[0] = '\0';
	// snprintf( bgp1.command, 1024, "command string" );
	// bgp1.next = NULL;


	do {
		prompt = getWorkingDirectory(0);
		char* input = readline(prompt);
		bg_flag = 0;
		if( strcmp(input,"") ) {
			parseInput(input,&args,&argc,&bg_flag);
			running = executeCommand(args,&bg_flag);
			//else { running = executeBackgroundCommand(args); }

			int i;
			for( i = 0; i < argc; i++) {
				free( args[i] );
			}
			free(args);
			free(input);
			free(prompt);
		}
		else {
			free(input);
			free(prompt);
		}
	} while( running );

	printf("Session closed\n");
}

int executeCommand( char** args, int* bg_flag ) {
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
		return 1;
	}

	else if( !strcmp( args[0], "exit") ) {
		return 0;
	}

	else if( !strcmp( args[0], "bglist") ) {
		printList();
		//addToList( args[0], (pid_t)simPID++ );
		return 1;
	}

	/*Use a program in binaries*/
	else {
		pid_t child_pid;
		//pid_t w;
		int status;//of child process
		child_pid = fork();

		if ( child_pid == 0 ) { //Child executes this
			execvp( args[0], args );
			/*PROGRAM IS OVERWRITTEN AFTER THIS POINT*/
			/*execvp failed if the following message is printed*/
			printf("Unknown Command (args** invalid).\n");
			exit(EXIT_SUCCESS);
		}
		else if( child_pid == -1 ) { 
			printf("Failed to fork\n"); 
			exit(EXIT_FAILURE);
		}
		else { //Parent executes this
			
			//TODO Fix this - figure out background process loop check
			//Only add to bg processes if bg is used
			if( (*bg_flag) ) { addToList(args[0],child_pid); }

			do {
				//If we're running a background process...
				if( (*bg_flag) ) {
					pid_t w = waitpid(child_pid, &status, WNOHANG);
					//printf("WNOHANG: %d\n", (int)WNOHANG);
					if( w == 0 ) {
						printf("Waitpid returned 0 \n");
					}
					else if( w == -1 ) {
						printf("Error waitpid returned -1\n");
					}
					else {
						printf("Waitpid returned %d\n", (int)w);
					}

					//while( w != child_pid ) printf("waitpid returned: %d\n", (int)w);
				}
				//else we're running a normal process where we just wait
				else {
					pid_t w = waitpid(child_pid, &status, WUNTRACED);
					//printf("WUNTRACED: %d\n", (int)WUNTRACED);
					//printf("waitpid returned: %d\n", (int)w);
				}
				
			} while( (!WIFEXITED(status)) && !WIFSIGNALED(status));
		}
		return 1;
	}
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
		printf( "TOKEN IS: %s\n", token );

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

node makeNode() {
	node temp;
	temp = (node)malloc(sizeof(struct LinkedList));
	(*temp).next = NULL;
	return temp;
}

void addToList( char* command, pid_t pid ) {

	node temp;
	node cur;
	temp = makeNode();
	(*temp).pid = pid;
	snprintf( (*temp).command, 1024, "ls -lah" );
	(*temp).next = NULL;

	if( root == NULL ) {
		listCount++;
		root = temp;
		//printf( "was empty, bgList->%d\n", (int)(*root).pid);
	}
	else {
		listCount++;
		cur = root;
		while( (*cur).next != NULL ) {
			//printf( "bgList->%d\n", (int)(*cur).pid);
			cur = (*cur).next;
		}
		(*cur).next = temp;
	}
}

void printList() {
	if( listCount == 0 ) { 
		//printf( "listcount 0\n");
		printf( "listEmpty\n");
	}
	else if( listCount == 1 ) { 
		//printf( "listcount 1\n");
		printf( "bgList->%d\n", (int)(*root).pid); 
	}
	else {
		node cur;
		cur = root;
		printf( "bgList->%d\n", (int)(*cur).pid);
		while( (*cur).next != NULL ) {
				//printf( "listcount %d\n",listCount);
				
				cur = (*cur).next;
				printf( "bgList->%d\n", (int)(*cur).pid);
			}	
	}
}