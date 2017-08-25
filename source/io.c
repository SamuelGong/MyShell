/* file name : io.c */
/* author name : Zhifeng Jiang ; ID : 3150101149 */
#include "myshell.h"

/* following are funtions about I/O */

/* display prompt */
void display_prompt(){
	
	char* home;									/* for home path fetching */
	char pwd[MAX_CHAR];						/* for current path fetching */
	char hostName[MAX_CHAR];					/* for host name fetching */

	/* Part 1 : obtain information */
	gethostname(hostName, sizeof(hostName) - 1);	/* get the host name */
	getcwd(pwd, sizeof(pwd) - 1);					/* get the current path */
	home = getenv("HOME");							/* get the home path */
	str_rep(pwd, home, "~");	/* replace the home path with '~' in pwd */
	
	/* Part 2 : display the prompt	 */
	/* change into yellow and bold */
	printf("\033[1;33m%s", getenv("USER"));		/* print the user name */
	printf("@");
	printf("%s", hostName);						/* print the host name */
	printf(":");
	printf("%s", pwd);							/* print the current path */
	printf("> \033[0m");			/* finally cancel all the font settings */
	fflush(stdout);					/* flush the output buffer */
}

/* read input from terminal */
/* if fail, return 255, otherwise return the length of the command */	
int read_from_terminal(char* cmd){
	
	int count = 0;								/* for count the character */
	char ch;									/* intermediate character */
	memset(cmd, 0, MAX_CHAR);				/* clear the previous record */
	
	ch = getchar();
	while (count < MAX_CHAR - 1 && ch != '\n'){
		cmd[count] = ch;					/* append the character read */
		count++;								/* increment the counter */
		ch = getchar();
	}
	
	if ( ch != '\n' ){
		display_error("Command too long!\n");	/* error display */
		return -1;								/* exit with error code */
	}
	else {
		cmd[count] = '\0';						/* the end of a string */
		return count;							/* return the length */
	}

}

/* read input from a batch file */
int read_from_file(char* cmd, char* file){

	int count = 0;								/* for count the character */
	char ch;									/* intermediate character */
	FILE * fp;									/* the batch file */


	memset(cmd, 0, MAX_CHAR);				/* clear the previous record */	
	fp = fopen(file, "r");					/* open the file */
	if ( fp == NULL )
		return -1;

	ch = fgetc(fp);
	while (count < MAX_CHAR - 1 && ch != EOF){
		cmd[count] = ch;						/* append the character read */
		count++;								/* increment the counter */
		ch = fgetc(fp);
	}
	
	if ( ch != EOF ){
		display_error("Command too long!\n");	/* error display */
		return -1;								/* exit with error code */
	}
	else {
		cmd[count] = '\0';						/* the end of a string */
		return count;							/* return the length */
	}
	
	fclose(fp);
}

/* parsing pipe command */
/* by the way, partition segments into single commands by '|' */
/* and construct all the commands and pipes */
/* finally return the number of commands */
int has_pipe(char** args, int arg_num, struct command* cmds, struct pipe* pipes, int mode){
	
	int i, j, k;								/* for iteration */
	int front = 0, rear = -1;					/* two ends of a command */
	int cmd_count = 0;							/* amount of commands */
	int pipe_count = 0;							/* amount of pipes */
	int err;									/* store the error codes */

	for( i = 0; i <= arg_num; i++ ){
		if ( i == arg_num || strcmp(args[i], "|")== 0 
			|| strcmp(args[i], "`") == 0 ){	/* which means a command ends */
											/* construct a new command */
			if (front <= rear) {
				if(cmds[cmd_count].args)	/* free the space of old args */
					for ( j = 0; j < cmds[cmd_count].arg_num; j++)
						free(cmds[cmd_count].args[j]);
			
											/* allocate space for new args */
				cmds[cmd_count].args = (char **)malloc(
					(rear - front + 1)*sizeof(char *));
				cmds[cmd_count].arg_num = rear - front + 1;

				for ( j = front, k = 0; j <= rear; j++, k++){
					cmds[cmd_count].args[k] = (char *)malloc(
						( strlen(args[j]) + 1 ) * sizeof(char) );
					strcpy(cmds[cmd_count].args[k], args[j]);
				}
											/* designate file descriptor */
				cmds[cmd_count].infd = 0;	/* default input */
				cmds[cmd_count].outfd = 1;	/* default output */

				if ( mode == 0 ){			/* if from the terminal */

					/* if not the fisrt, it must read from the last pipe */
					if ( cmd_count )
						cmds[cmd_count].infd = pipes[pipe_count - 1].fd[0];
			
					/* if not the last, it must write into a new pipe */
					if ( i != arg_num ){
						err = pipe(pipes[pipe_count].fd);
						if ( err < 0 ){
							display_error("Cannot create pipe!");
							return -1;		/* fail, tips and return */
						}
						cmds[cmd_count].outfd = pipes[pipe_count].fd[1];
						pipe_count ++;
					}
				}
				else{						/* if from the file */

					/* if not the fisrt command in a line */ 
					/* it must read from the last pipe */
					if ( !(cmd_count == 0 
							|| strcmp(args[front - 1], "`") == 0 ) )
						cmds[cmd_count].infd = pipes[pipe_count - 1].fd[0];

					/* if not the last command in a line */
					/* it must write to a new pipe */
					if ( i != arg_num && strcmp(args[i], "`") != 0 ){
						err = pipe(pipes[pipe_count].fd);
						if ( err < 0 ){
							display_error("Cannot create pipe!");
							return -1;		/* fail, tips and return */
						}
						cmds[cmd_count].outfd = pipes[pipe_count].fd[1];
						pipe_count ++;
					}
				}
				cmd_count++;
			}
			front = i + 1;					/* prepare for next command */
			rear = i;
		}
		else {
			rear++;							/* extend the current command */
		}
	}
	return cmd_count;

}

/* parsing redirection command */
/* to check if the command has specification of redirection */
/* if fail, return -1, other wise return a non-negative integer */
/* meanwhile change the fd properly */
int has_io_redirect(struct command * cmd){
	
	int i;											/* for iterations */
	int err = 0;									/* error code */
	int arg_num = cmd->arg_num;						/* amount of arguments */
	int mark = 0;					/* mark if has the io redirection */
	int first = 0;					/* the first location of io redirection */
	char* home;									/* for home path fetching */
	home = getenv("HOME");						/* get the home path */

	/* Part 1 : change the file descriptor */
	for ( i = 0; i < arg_num; i++ ){
		if ( strcmp(cmd->args[i], "<") == 0 ){	/* input redirection */

			if ( !first ){
				first = i;
				mark = 1;/* mark and store the startting index of io red. */
			}
			str_rep(cmd->args[i+1], "~", home);	/* replace the "~" */
			err = open( cmd->args[i+1], O_RDONLY );
												/* open the file */
			if ( err < 0 ){
				display_error("Fail to open file to read.");
				break;						/* if fails, tips and return */
			}
			cmd->infd = err;
		}
		else if ( strcmp(cmd->args[i], ">") == 0 ){		

			if ( !first ){
				first = i;
				mark = 1;/* mark and store the startting index of io red. */
			}

											/* covering ouput */
			str_rep(cmd->args[i+1], "~", home);	/* replace the "~" */
			err = open( cmd->args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
											/* open the file */
			if ( err < 0 ){
				display_error("Fail to open file to write.");
				break;						/* if fails, tips and return */
			}
			cmd->outfd = err;
		}
		else if ( strcmp(cmd->args[i], ">>") == 0 ){ 	

			if ( !first ){
				first = i;
				mark = 1;/* mark and store the startting index of io red. */
			}
												/* appending output */
			str_rep(cmd->args[i+1], "~", home);	/* replace the "~" */
			err = open( cmd->args[i+1], O_APPEND | O_WRONLY | O_CREAT, 0644);
												/* open the file */
			if ( err < 0 ){
				display_error("Fail to open file to append.");
				break;						/* if fails, tips and return */
			}
			cmd->outfd = err;
		}
	}
	
	/* Part 2 : remove the redirection descriptors from the arguments */
	char** temp;
	if (mark){
		temp = (char **)malloc(first*sizeof(char *));
		for( i = 0; i < first; i++)
			temp[i] = cmd->args[i];	/* save the necessary arguments */
		for ( i = first; i < cmd->arg_num; i++)
			free(cmd->args[i]);		/* free the space of useless arguments */

		cmd->arg_num = first;		/* update the content of the  structure */
		cmd->args = temp;
	}

	return err;
}

/* duplicate fds if necessary */
void dup_fd(struct command * cmd){
	
	int infd = cmd->infd, outfd = cmd->outfd;
							/* copy the value since dup2 will clean */

	if(infd != 0){						/* relate the input fd */
		close(0);
    	dup(infd);
    }

    if(outfd != 1){						/* relate the output fd */
		close(1);
    	dup(outfd);
    }

	int i;								/* close the redundant fd. */
    for ( i = 3; i < sysconf(_SC_OPEN_MAX); i++ )
        close(i);

}		

void close_fd(struct command * cmd){

	if(cmd->infd != 0)						/* if input not from standard */
		close(cmd->infd);
	if(cmd->outfd != 1)						/* if output not from standard */
		close(cmd->outfd);

}

/* following are specifications about exception processing */
/* display error */
void display_error(char* str){
	
	fprintf(stderr, "%s\n", str);	
						/* display the error information on the screen */

}

