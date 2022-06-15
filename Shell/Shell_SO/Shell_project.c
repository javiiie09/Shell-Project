/**
UNIX Shell Project
 
Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA
 
Alumno: Javier Sánchez Alarcón
Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.
 
To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
    (then type ^D to exit program
 **/

#include "job_control.h"   // remember to compile with module job_control.c
#include <string.h>
#include <stdlib.h>
#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

// -----------------------------------------------------------------------
job* lista; //Lista de lista

// Manejador

void my_sigchld(int sig) {
    int status, info, pid_wait; // DECLARO LAS VARIABLES
    enum status status_res;
	job* jb = NULL;
    block_SIGCHLD(); //bloqueamos y desbloqueamos en los accesos a la lista
   
	for(int i = 1; i <= list_size(lista); ++i){
		jb = get_item_bypos(lista, i);
		pid_wait = waitpid(jb->pgid, &status, WUNTRACED|WNOHANG);
		
		if (pid_wait == jb->pgid) { // si el pid del jb selecionado coincide con el que le ha mandado la señal 
            status_res = analyze_status(status, &info);
			printf("wait realizado a proceso en background: %s, pid: %i\n", jb->command, jb->pgid);
            if (status_res == EXITED || status_res == SIGNALED) {
                delete_job(lista, jb); // borramos el jb de la lista
                i--;
            }  
            else if (status_res == SUSPENDED) {// si el status_res es suspendido 
                printf("\nThe background command %s with pid %d was suspended\n", jb->command, jb->pgid);
                jb->state = STOPPED; // paramos el trabajo 
            }
        } 
	}
	unblock_SIGCHLD();
}


// Main

int main(void) {

    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2]; /* command line (of 256) has max of 128 arguments */
    // probably useful variables:
    int pid_fork, pid_wait; /* pid for created and waited process */
    int status; /* status returned by wait */
    enum status status_res; /* status processed by analyze_status() */
    int info; /* info processed by analyze_status() */

	job* nuevo;
    ignore_terminal_signals(); // ignoramos las señales del terminal 
    lista = new_list("unalista"); //creamos la lista de trabajos 
    signal(SIGCHLD, my_sigchld); //leemos las señales de los hijos 

    while (1) {/* Program terminates normally inside get_command() after ^D is typed */
        printf("COMMAND->");
        fflush(stdout);
        get_command(inputBuffer, MAX_LINE, args, &background); //leemos el comando


        if (args[0] == NULL)
        {
            continue;
        }       		

// Comando CD

        if(!strcmp(args[0], "cd"))
        {
            if(args[1] == NULL)
            {
                printf("No given directory");
                continue;
            }
            int value = chdir(args[1]);
            continue;
        }
// Comando JOBS

        if (!strcmp(args[0], "jobs")) { //generamos el comando interno jobs
            if (list_size(lista) == 0) {
                printf("La lista esta vacia \n");
            }
            else {
                print_job_list(lista);
            } 
            continue;
        } 

// Comando FG

        if (strcmp(args[0], "fg") == 0) {
            int posicion;
            enum status statusfg;
            if (args[1] == NULL) {
                posicion = 1;
            }
            else { //sino, la pos que le especifiquemos
                posicion = atoi(args[1]);
            }
            nuevo = get_item_bypos(lista, posicion);
            if (nuevo == NULL) {
                printf("FG ERROR: JOB NOT FOUND \n");
                continue;
            } 
            if (nuevo->state == STOPPED || nuevo->state == BACKGROUND) {
                printf("Puesto en foreground el job %d que estaba suspendido o en background, el job era: %s\n", posicion, nuevo->command);
                nuevo->state = FOREGROUND; //cambiamos el estado de nuevoiliar a foreground
                set_terminal(nuevo->pgid); //le damos el terminal
                killpg(nuevo->pgid, SIGCONT); //manda una señal al grupo de proceso para que continue
                waitpid(nuevo->pgid, &status, WUNTRACED); //esperamos por el hijo 
                set_terminal(getpid()); //el padre recupera el el terninal
                statusfg = analyze_status(status, &info);
                if (statusfg == SUSPENDED) {//si esta suspendido lo paramos 
                    nuevo->state = STOPPED;
                } 
                else {
                    delete_job(lista, nuevo); //borramos el trabajo 
                } 
            } 
            else {
                printf("El proceso no estaba en background o suspendido");
            }
            continue;
        }

// Comando BG

        if (strcmp(args[0], "bg") == 0) { //si es el comando interno backgraund (bg)
            int posicion;
            if (args[1] == NULL) { //Si args[1] no existe, cogemos la pos 1
                posicion = 1;
            }
            else {
                posicion = atoi(args[1]); //sino, la pos que le especifiquemos
            }
            nuevo = get_item_bypos(lista, posicion); //selecionamos el trabajo en la posicion selecionada
            if (nuevo == NULL) {
                printf("BG ERROR: trabajo no encontrado \n");
            } else if (nuevo->state == STOPPED) {// si esta parado lo pasamos a punto 					
                nuevo->state = BACKGROUND;
                printf("Puesto en background el job %d que estaba suspendido, el job era: %s\n", posicion, nuevo->command);
                killpg(nuevo->pgid, SIGCONT);
            } 
            continue;
        }
		
		

        pid_fork = fork();

		if (pid_fork > 0){
			/* Padre = SHELL */
			new_process_group(pid_fork); // HIJO -> grupo propio, 
			                             //         lider de su grupo
			if (background){
				printf("background\n");
				// Insertar el proceso en bg en la lista
				nuevo = new_job(pid_fork, args[0], BACKGROUND); //Nuevo nodo job
				block_SIGCHLD();  // Impide que entre SIGCHLD cuando cambio la lista
				                  // en main()              
				add_job(lista, nuevo);  
				unblock_SIGCHLD();
				              
				print_job_list(lista); //DEBUG
				
			} else { 
				printf("foreground\n");
				
				set_terminal(pid_fork);

				waitpid(pid_fork, &status , WUNTRACED);

				status_res = analyze_status(status, &info);
				
				set_terminal(getpid());

				if (status_res == SUSPENDED){
					nuevo = new_job(pid_fork, args[0], STOPPED); //Nuevo nodo job
					block_SIGCHLD(); 				
					add_job(lista, nuevo); 
					unblock_SIGCHLD();               
					print_job_list(lista); //DEBUG								
				}
			}
		} else if (pid_fork == 0) {
			/* Proceso hijo */
			new_process_group(getpid()); // HIJO -> grupo propio, 
			                               
			if (!background){
				// SOLO EN FG!!!
				set_terminal(getpid()); // El hijo coge el terminal
			}
			restore_terminal_signals();
			execvp(args[0], args);
			perror("Si llego aquí exec falló\n");
			exit(EXIT_FAILURE);
		} 


	} // end while
}