/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/

#include "job_control.h"   // remember to compile with module job_control.c
#include <string.h>

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------

int main(void)
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */

	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   
		
		ignore_terminal_signals(); //Ignora ^C y ^Z

		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		if(args[0]==NULL) continue;   // if empty command

		/* built-in commands*/

		if(!strcmp(args[0], "hola")){
			printf("Hola mundo cruel\n");
			continue;
		}

		if(!strcmp(args[0], "cd")){
			if(args[1] != NULL){
				chdir(args[1]);
			}
			continue;


		}

		/* the steps are:
			 (1) fork a child process using fork()
			 (2) the child process will invoke execvp()
			 (3) if background == 0, the parent will wait, otherwise continue 
			 (4) Shell shows a status message for processed command 
			 (5) loop returns to get_commnad() function
		*/
		
		//Comandos externos

		pid_fork = fork();

		if(pid_fork > 0){ //Proceso del padre
		
			new_process_group(pid_fork);

			if(background){

				printf("Proceso %d en bg\n", pid_fork);

			}else{

				set_terminal(pid_fork); // Cede el terminal al hijo en fg
				waitpid(pid_fork, &status, WUNTRACED);
				status_res = analyze_status(status, &info);

				if(status_res == EXITED){
					printf("Proceso en bg murió voluntariamente\n");
				}else if(status_res == SIGNALED){
					printf("Proceso en bg lo mataron con una señal\n");
				}else if(status_res == SUSPENDED){
					printf("Proceso en bg se suspendió\n");
				}

				set_terminal(getpid()); // El shell recupera el terminal

			}

		}else if(pid_fork == 0){ //Hijo -> Proceso que quiero lanzar
			
			new_process_group(getpid());	

			if(background){

			}else{
				set_terminal(getpid()); // Por si el hijo se planifica antes que el padre
			}

			restore_terminal_signals(); //Para tener ^C, ^Z, etc.
			execvp(args[0] , args);
			//Si exec falla continua por aquí
			perror("Exec falló\n");
			exit(EXIT_FAILURE);

		}else{
			//ERROR
			//fprintf(stderr, "El fork falló\n");
			perror("El fork falló\n");
			continue;
		}

	} // end while
}
