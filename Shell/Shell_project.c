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

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

job *tar;

void manejador(int sig){
	block_SIGCHLD();
	job *j;
	int status, info, pid_wait = 0;
	enum status status_res;

	for(int var = 1; var <= list_size(tar); ++var){

	}

}


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

	job *item;
	ignore_terminal_signals();
	//signal(SIGCHLD, manejador);
	int pp = 0;

	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   		
		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		if(args[0]==NULL) continue;   // if empty command

		// ----TAREA 2---- Implementar CD

		
		
		// ----TAREA 1---- Creación de procesos en primer y segundo plano

		if(!pp){ // Si no está en primer plano, crea un proceso hijo
			pid_fork = fork(); // Genera proceso hijo
		}

		if(pid_fork < 0){ // Proceso del padre
			if(background == 0){
				pid_wait = waitpid(pid_fork, &status, WUNTRACED); // Si background es 0, el proceso padre espera
				set_terminal(getpid());
				status_res = analyze_status(status, &info);
			
				if(status_res == EXITED){
					if(info != 255){
						printf("Comando %s ejecutado en primer plano con pid %d. Estado %s. Info: %d\n\n", args[0], pid_fork, status_strings[status_res], info);
					}
				}else if (status_res == SUSPENDED){
					block_SIGCHLD();
					item = new_job(pid_fork, args[0], STOPPED);
					//add_job(, item);
					printf("Comando %s ejecutado en primer plano con pid %d. Estado %s. Info: %d\n\n", args[0], pid_fork, status_strings[status_res], info);
					unblock_SIGCHLD();
				}

			}else{
				block_SIGCHLD();
				item = new_job(pid_fork, args[0], STOPPED);
				//add_job(, item);
				printf("Comando %s ejecutado en segundo plano con pid %d.\n\n", args[0], pid_fork);
				unblock_SIGCHLD();
			}
		//pp = 0;
		}else{ // Proceso del hijo
			new_process_group(getpid());
			if(background == 0){
				set_terminal(getpid());
			}
			restore_terminal_signals();

			execvp(args[0], args);
			printf("Error. Comando %s no encontrado. \n\n", args[0]);
			exit(-1); 
		}

		/* the steps are:
			 (1) fork a child process using fork()
			 (2) the child process will invoke execvp()
			 (3) if background == 0, the parent will wait, otherwise continue 
			 (4) Shell shows a status message for processed command 
			 (5) loop returns to get_commnad() function
		*/

	} // end while
}
