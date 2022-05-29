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

//La lista es una estructura global
job *lista;

void my_sigchld(int x){
	
	/*
	MANEJADOR DE SIGCHLD ->
	recorrer todos los jobs en bg y suspendidos a ver
	qué les ha pasado
	SI MUERTOS -> quitar de la lista
	SI CAMBIAN DE ESTADO -> cambiar el job correspondiente
	*/

	int i, status, info, pid_wait;
	enum status status_res; //status processed by anlyze_status()
	job* jb;

	for(int i = 1; i <= list_size(lista); ++i){
		jb = get_item_bypos(lista, i);
		pid_wait = waitpid(jb -> pgid, &status, WUNTRACED|WNOHANG|WCONTINUED);
		if(pid_wait == jb -> pgid){
			status_res = analyze_status(status, &info);
			/*
			qué puede ocurrir?
			- EXITED
			- SIGNALED
			- SUSPENDED
			- CONTINUED
			*/

			printf("[SIGCHLD] Wait realizado para trabajo en background: %s, pid = %i\n", jb -> command, pid_wait);

			/*
			Actuar según los posibles casos reportado por status
			Al menos hay que considerar EXITED, SIGNALED y SUSPENDED
			en este ejemplo sólo se consideran los dos primeros
			*/

			if((status_res == SIGNALED) || (status_res == EXITED)){
				printf("Comando %s ejecutado en background con PID %d ha terminado su ejecucion\n\n", jb->command, jb->pgid);
				delete_job(lista, jb);
				i--; // OJO! El siguiente ha ocupado la posicion de este en la lista
			}
			if(status_res == CONTINUED){
				printf("Comando %s ejecutado en background con PID %d ha continuado su ejecucion\n\n", jb->command, jb->pgid);
				jb->state = CONTINUED;
			}
			if(status_res == SUSPENDED){
				printf("Comando %s ejecutado en background con PID %d ha suspendido su ejecucion\n\n", jb->command, jb->pgid);
				jb->state = STOPPED;
			}

		}
	}
	return;
}



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

	job *nuevo; // Para almacenar un nuevo job

	//IGNORAR SEÑALES MOLESTAS
	ignore_terminal_signals();

	//Creo una lista vacía para jobs en bg y suspendidos
	lista = new_list("unalista");

	//Instalar manejador de sigchild
	signal(SIGCHLD, my_sigchld);

	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   
		
		//ignore_terminal_signals(); //Ignora ^C y ^Z

		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		if(args[0]==NULL) continue;   // if empty command

		/* built-in commands*/

		if(!strcmp(args[0], "hola")){
			printf("Hola mundo cruel\n");
			continue;
		}

		if(!strcmp(args[0], "salir")){
			exit(0);
			continue;
		}

		if(!strcmp(args[0], "cd")){
			if(args[1] != NULL){
				chdir(args[1]);
			}else{
				printf("El dinerctorio %s no existe.\n", args[1]);
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

				//Insertar el proceso en bg en la lista
				nuevo = new_job(pid_fork, inputBuffer, BACKGROUND); // Nuevo nodo job

				block_SIGCHLD(); // Impide que entre SIGCHLD cuando cambio la lista en main()

				add_job(lista, nuevo);
				unblock_SIGCHLD();

				print_job_list(lista); //DEBUG

			}else{

				printf("foreground\n");

				set_terminal(pid_fork); // Cede el terminal al hijo en fg

				/*
				EL SHELL ESPERA A QUE:
					- el hijo muera(exit / signal)
					- el hijo se suspenda(stop)
				ESPERAMOS DE FORMA BLOQUEANTE
				(no usar WNOHANG)
				*/
				
				waitpid(pid_fork, &status, WUNTRACED);

				//El padre va a imprimir que pasó;
				status_res = analyze_status(status, &info);

				set_terminal(getpid()); //El padre recupera el terminal despues del wait

				/*
				¿Por qué puedo retornar del wait del job en fg?
				EXITED ->
				SIGNALED ->
				STOPPED -> Insertar en la lista el proceso stopped
				Insertar el proceso en bg en la lista
				*/

				if(status_res == EXITED){
					printf("Proceso en bg murió voluntariamente\n");
				}else if(status_res == SIGNALED){
					printf("Proceso en bg lo mataron con una señal\n");
				}else if(status_res == SUSPENDED){
					nuevo = new_job(pid_fork, inputBuffer, STOPPED);
					block_SIGCHLD();
					add_job(lista, nuevo);
					unblock_SIGCHLD();
					print_job_list(lista);
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
