#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

char *BUILT_IN_COMMANDS[] = {"cd", "echo", "pwd", "exit"};
int REDIRECTING = 0;
int INTERACTIVE = 1;

char** parseInput(char* inputLine);
char* handleSystemCommand(char** argv);
char* handleNormalCommand(char** argv);
void readFile(char* inFile);
int argc;

int main(int argc, char* argv[])
{
	if(argc == 1)
	{
		handleCommandLineInput();
	}else if(argc == 2)
	{
		INTERACTIVE = 0;
		readFile(argv[1]);
	}else {
		printError();
		exit(1);
	}
	

	return 0;
}

void readFile(char* inFile)
{
	int fd = open(inFile, O_RDONLY);
	int pastError = 0;
	FILE* readFile = fdopen(fd, "r");

	if(!readFile)
	{
		printError();
		exit(1);
	}

	char inputBuffer[514];

	while(fgets(inputBuffer, 514, readFile) != NULL)
	{
		REDIRECTING = 0;
		if(strlen(inputBuffer) == 513 && inputBuffer[512] != '\n' && !pastError)
		{
			printOutput(inputBuffer);
			pastError = 1;
			printError();
		}else if(pastError){
			if(strlen(inputBuffer) < 513)
				pastError = 0;
		}
		else {
			inputBuffer[strlen(inputBuffer) - 1] = '\0';
			printOutput(inputBuffer);
			char** argv = parseInput(inputBuffer);
			if(argv == NULL)
			{
				printError();
			}else {
				if(argc == 0)
				{
					exitShell();
				}else {
					handleCommand(argv, 0);
				}
				
			}
		}
		
	}

	exit(0);
}

int printError()
{
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
	return 0;
}

int printOutput(char* output, char* inFile)
{
	int fileNo;
	if(REDIRECTING)
	{
		fileNo = open(inFile, O_TRUNC|O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
	}else {
		fileNo = STDOUT_FILENO;
	}
	write(fileNo, output, strlen(output));
	write(fileNo, "\n", 1);

	if(REDIRECTING)
	{
		close(fileNo);
	}
}

int printPrompt()
{
	char* promptString = "mysh> ";
	write(STDOUT_FILENO, promptString, strlen(promptString));
	fflush(stdout);
}

int exitShell()
{
	exit(0);
}

char* cd(char *directory)
{
	char* output = "";
	if(strlen(directory) == 0)
	{
		directory = getenv("HOME");
	}

	int error = chdir(directory);

	if(error == -1)
	{
		printError();
	}


	return output;
}

char* pwd(char **argv)
{
	char* output = getcwd(NULL, 0);
	
	if(output == NULL)
	{
		printError();
		free(output);
		return "";
	}else{
		printOutput(output, argv[argc-1]);
		return NULL;
	}
}

int compileAndRun(char **argv)
{
	char *programName;
	programName = malloc(strlen(argv[0]) - 1);
	strncpy(programName, argv[0], strlen(argv[0]) - 2);
	programName[strlen(argv[0]) - 1] = '\0';

	char *compileArgs[] = {"gcc", "-o", programName, argv[0], NULL};

	int oldRedirecting = REDIRECTING;
	int oldArgC = argc;
	REDIRECTING = 0;
	argc = 4;

	handleNormalCommand(compileArgs);

	REDIRECTING = oldRedirecting;
	argc = oldArgC;

	argv[0] = programName;
	handleNormalCommand(argv);
}

int handleCommandLineInput()
{
	char inputBuffer[514];
	printPrompt();
	int pastError = 0;

	while(fgets(inputBuffer, 514, stdin) != NULL)
	{
		REDIRECTING = 0;
		if(strlen(inputBuffer) == 513 && inputBuffer[512] != '\n')
		{
			pastError = 1;
			printError();
		}else if(pastError){
			if(strlen(inputBuffer) < 513)
				pastError = 0;
		}
		else {
			inputBuffer[strlen(inputBuffer) - 1] = '\0';
			char** argv = parseInput(inputBuffer);
			if(argv == NULL)
			{
				printError();
			}else {
				if(argc == 0)
				{
					exitShell();
				}else {
					handleCommand(argv, 0);
					printPrompt();
				}
				
			}
		}
	}
}

int handleCommand(char** argv, int verbose)
{
	if(verbose)
	{
		echo(argv, 1);
	}

	char* output;

	if(isSystemCommand(argv[0]))
	{
		output = handleSystemCommand(argv);
		if(output!= NULL)
			printOutput(output, argv[argc-1]);
	}else if(argv[0][strlen(argv[0]) -1] == 'c' && argv[0][strlen(argv[0]) -2] == '.')
	{
		compileAndRun(argv);
	} else {
		handleNormalCommand(argv);
	}

}

char** parseInput(char* inputLine)
{
	char** args = malloc(sizeof(void*));
	argc = 1;
	
	// Check for two >'s
	
	char* firstPointer = strstr(inputLine, ">");	// points to the first instance of '>'
	if(firstPointer != NULL)
	{
		char* secondPointer = strstr(firstPointer+1, ">");	// points to the second instance of '>'
		if(secondPointer != NULL)
		{
			return NULL;
		}
	}
	
	// parse

	args[0] = strtok(inputLine, " ");
	
	if(args[0] == NULL)
	{
		argc = 0;
	}

	while(argc != 0)
	{
		char * str = strtok(NULL, " ");

		if(str == NULL)
		{
			break;
		}
		else
		{
			args = realloc(args, (argc+1)*sizeof(void*));
			args[argc] = str;
			argc++;
		}
	}
	
	args = realloc(args, (argc+1)*sizeof(void*));
	args[argc] = NULL;
	
	// Test for >
	
	int numArrows = 0;
	int i;
	for(i = 0; i < argc; i++)
	{
		int indexOf = strcspn(args[i], ">");

		if(indexOf <= strlen(args[i])-1)	// contains >
		{
			numArrows++;
			
			if(numArrows >= 2)
			{
				return NULL;		// only one > per line
			}

			REDIRECTING = 1;

			int isLast = (indexOf == strlen(args[i])-1);
			int isFirst = (indexOf == 0);
			
			if(i == argc-1)
			{	
				if(isLast)
				{
					return NULL;	// cannot be ... aaa>
				}
				else if(isFirst)
				{
					args[argc-1] += sizeof(char);	// remove > from beginning of args[i]
				}
				else			// sandwiched >
				{
					args[argc-1] = strtok(args[i], ">");
					args[argc] = strtok(NULL, ">");
					args = realloc(args, (argc+1)*sizeof(void*));	// expand args
					args[argc+1] = NULL;
					argc++;
				}
			}
			else if(i == argc-2)
			{	
				if(strlen(args[argc-2]) == 1)	// ... > ...
				{
					args[argc-2] = args[argc-1];
					args[argc-1] = NULL;
					argc--;

					i--;	// must decrement i to check final argument
				}
				else if(isLast)		// ... aaa> ...
				{
					args[argc-2][strlen(args[argc-2])-1] = '\0';	// remove final character
				}
				else
				{
					return NULL;	// ... >aaa ... and ... aaa>aaa are illegal
				}
			}
			else
			{
				return NULL;	// no argument but the last or 2nd-to-last can contain >
			}
		}
	}

	return args;
}

char* handleSystemCommand(char** argv)
{
	char *command = argv[0];
	if(strcmp(*argv, "exit") == 0)
	{
		if(argc != 1)
			printError();

		exitShell();

	}else if(strcmp(*argv, "cd") == 0)
	{
		if(argc == 1)
		{
			cd("");
		}else{
			if(argc != 2)
				printError();

			cd(argv[1]);
		}
		

		return NULL;

	}else if(strcmp(*argv, "pwd") == 0)
	{
		if(argc != 1 && !REDIRECTING)
		{
			printError();
		}else if(REDIRECTING && argc != 2)
		{
			printError();
		}else{
			return pwd(argv);
		}			
		
	}else if(strcmp(*argv, "echo") == 0)
	{
		if(argc < 2)
			printError();
		
		echo(argv, 0);
		return NULL;
	}
}

int echo(char** argv, int includeCommand)
{
	int x = includeCommand ? 0 : 1;
	int y = 0;
	char output[513];
	int commandCount = argc;
	if(REDIRECTING){
		commandCount -=1;
	}
	for(x; x < commandCount; ++x)
	{
		int z;
		for(z = 0; z < strlen(argv[x]); ++z)
		{
			output[y++] = argv[x][z];
		}

		if(x != commandCount-1){
			output[y++] = ' ';
		}
		
	}

	output[y] = '\0';

	printOutput(output, argv[argc-1]);

	return 0;
}

char* handleNormalCommand(char** argv)
{
	int child_pid = fork();
	
	if(child_pid == 0)	// if child
	{
		int new_fd;
		if(REDIRECTING)
		{
			close(STDOUT_FILENO);
			int fd = open(argv[argc-1], O_TRUNC|O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
			if(fd==-1)
			{
				printError();
				exit(1);
			}
			argv[argc-1] = NULL;

		}

		execvp(argv[0], argv);
	}
	int returnCode;
	wait(&returnCode);
	if(errno == 10)
	{
		printError();
	}

}

int isSystemCommand(char* command)
{
	int x;
	for(x = 0; x < (sizeof(BUILT_IN_COMMANDS)/sizeof(void*)); ++x)
	{
		if(strcmp(command, BUILT_IN_COMMANDS[x]) == 0)
		{
			return 1;
		}
	}
	return 0;
}