/* file name : myshell.c */
/* author name : Zhifeng Jiang ; ID : 3150101149 */
#include "myshell.h"

/* global variables */
pid_t fpid;							/* the pid of myshell */
int status;							/* the exit status of f.g. command */
int main_argc;						/* the copy of argc */
char **main_argv;					/* the copy of argv */
struct job_list* jobs;				/* the list of jobs */
struct con_list* cons;				/* the list of control structures */
struct var_list* vars;				/* the list of local shell variables */
char father_path[MAX_CHAR];			/* the absolute path of father dir. */

int main(int argc,char *argv[]){
	
	int i, j;						/* for iterations */
	int err;						/* error code */
	int type;						/* operation types */
	int mode;						/* the input mode */
	int arg_num;					/* the amount of all arguments */
	int in_len;						/* the length of the overall input */
	int cmd_num;					/* the number of all the commands */
	int job_num = 0;				/* the current number of jobs */
	int should_run = 1;				/* exit flag */
	char input[MAX_CHAR];			/* commands from user */
	char *args[MAX_SEGS];			/* all arguments */
	struct pipe pipes[MAX_SEGS];	/* the array of pipes */
	struct command cmds[MAX_SEGS]; 	/* the array of command */
 
	/* Part 1 : initialization */
	fpid = getpid();
	main_argc = argc;				/* in order can be used in other places */
	main_argv = argv;
	init(args, cmds, pipes);		/* initialization of all arrays */
	signal(SIGTSTP, stop_job);		/* enable ctrl + z */
	//signal(SIGINT, SIG_IGN);		/* unable ctrl + c */

	if ( argc == 1 )				/* read from which place */
		mode = 0;					/* from terminal */
	else
		mode = 1;					/* from batch file */

	/* Part 2 : execution loop */
	while (should_run) {

	/* Step 1 : ready for input and read the input and construct commands */
		if ( mode == 0 ){	/* if no argument for main, waiting for input */
			
			display_finish(); 		/* display the finished background jobs */
									/* and process all the finished jobs */
			display_prompt();		/* display prompt */
			
			in_len = read_from_terminal(input);	/* read input as commands */
			if ( in_len <= 0 )
					continue;				/* if reading fail, try again */
		}
		else {				/* get the commands from the batch file */

			mode = 1;
			in_len = read_from_file(input, argv[1]);		
												/* read input as commands */
			if ( in_len <= 0 ){
				mode = 0;
				continue;	/* if reading fail, try to read from terminal */
			}
		}
		
		arg_num = str_spl(input, args);	/* split input into segments */
		cmd_num = has_pipe(args, arg_num, cmds, pipes, mode);	
						/* according to '|', partition segments to commands */
		if ( cmd_num < 0)
			continue;					/* if construting fail, try again */
		
		has_control(cmds, cmd_num);			/* figuring the beginning */
											/* and end of control structure */

	/* Step 2 : execute the commands */
		for ( i = 0; i < cmd_num; i++){
			
			is_background(&cmds[i]);		/* check if run in background */
			err = has_io_redirect(&cmds[i]);/* check if redirection needed */
			if ( err < 0 )
				break;						

			if ( strcmp(cmds[i].args[0], "quit") == 0 ){	/* quit */
				should_run = 0;
				break;
			}
			else{
				err = common_cmd(&cmds[i]);					/* execution */
				if ( err < 0 )
					break;
			}				

			i = next_command(cmds[i].args[0], i,
						WEXITSTATUS(status));		/* determinate the next */

		}		/* end of the for loop */
	}			/* end of the while loop */
	return 0;
}

/* initialization */
void init(char** args, struct command * cmds, struct pipe * pipes){
	
	// Part 0 : get the absolute path of father directory of the program
	int cnt = readlink("/proc/self/exe", father_path, MAX_CHAR);
	if  ( cnt < 0 || cnt >= MAX_CHAR ){
    	display_error("Fail to obtain the absolute address!");
    	exit(-1);
    }								/* to obatin the dir the program is in */

	int i, j = 2;					/* to obtain the father dir */
	for (i = cnt; i >=0; --i)
	{
		if ( j == 0 )
			break;
		else
			father_path[i] = '\0';
    	if ( father_path[i-1] == '/' )
    		j --;
	}
	
	// Part 1 : settings
	set_path("/bin:/usr/bin:./");				/* change the search path */
	
	// Part 2 : clearing
	memset(args, 0, (MAX_SEGS) * sizeof(char*));		  /* clear arguments */
	memset(cmds, 0, (MAX_SEGS) * sizeof(struct command));/* clear commands */
	memset(pipes, 0, (MAX_SEGS) * sizeof(struct pipe));  /* clear pipes */

	// Part 3 : allocating space for jobs
	jobs = (struct job_list *)malloc(sizeof(struct job_list));
	jobs->fore_num = jobs->back_num = 0;

	// Part 4 : allocating space for var.
	vars = (struct var_list *)malloc(sizeof(struct var_list));
	vars->var_num = 0;

	// Part 5 : allocating space for con.
	cons = (struct con_list *)malloc(sizeof(struct con_list));
	cons->if_num = cons->while_num = cons->done_num = cons->continue_num = 0;

}

/* normal cmd execute entrance */
/* distribute then entrance of all the common commands */
/* return 0 if succeeds and -1 otherwise */
int common_cmd(struct command * cmd){

	int err;								/* error code */
	int type;								/* command type */
	pid_t pid;								/* process id */

	type = cmd_type(cmd);				/* get the comand type */
	pid = fork();							/* fork a child */


	if ( pid < 0 ){							/* if fail */
		display_error("Fail to fork!");
		return pid;
	}
	else if ( pid == 0 ){					/* child process */
		
		dup_fd(cmd);
		if ( type == OP_EXTERNAL ){			/* external command */

			err = execvp(cmd->args[0], cmd->args);
			close_fd(cmd);					/* close fd if necessary */
			if( err < 0 )
				display_error("Fail to execute execvp!");	
		}
		else 								/* internal command */
			internal_cmd(cmd, type, 1);	

		exit(0);
	}
	else{									/* parent process */
		add_job(cmd, pid);			/* modify the job list */
		if (!cmd->backgnd){			/* if the child is in fore ground */
			internal_cmd(cmd, type, 0);		
									/* cmd that needs to run in parent */
			waitpid(pid, &status, WUNTRACED);	/* wait for child */
			delete_job(pid, 0);
			close_fd(cmd);					/* close fd if necessary */
		}
		else{							/* if the child is in back ground */
			//signal(SIGCHLD,SIG_IGN);	/* prevent zombie process */
			printf("[%d] %d\n", jobs->back_num, pid);/* display the info */
		}
		
		return 0;
	}

}

/* parsing internal command */
/* return the enumeration of the type of the operation */
int cmd_type(struct command * cmd){

	char op[MAX_CHAR];
	strcpy(op, cmd->args[0]);			/* get the operation string */
	
	if ( strcmp(op, "pwd") == 0 )
		return OP_PWD;
	else if ( strcmp(op, "echo") == 0 )
		return OP_ECHO;
	else if ( strcmp(op, "cd") == 0 )
		return OP_CD;
	else if ( strcmp(op, "time") == 0 )
		return OP_TIME;
	else if ( strcmp(op, "clr") == 0 )
		return OP_CLR;
	else if ( strcmp(op, "dir") == 0 )
		return OP_DIR;
	else if ( strcmp(op, "environ") == 0 )
		return OP_ENVIRON;
	else if ( strcmp(op, "umask") == 0 )
		return OP_UMASK;
	else if ( strcmp(op, "fg") == 0 )
		return OP_FG;
	else if ( strcmp(op, "bg") == 0 )
		return OP_BG;
	else if ( strcmp(op, "jobs") == 0 )
		return OP_JOBS;
	else if ( strcmp(op, "help") == 0 )
		return OP_HELP;
	else if ( strcmp(op, "shift") == 0 )
		return OP_SHIFT;
	else if ( strcmp(op, "exit") == 0 )
		return OP_EXIT;
	else if ( strcmp(op, "exec") == 0 )
		return OP_EXEC;
	else if ( strcmp(op, "set") == 0 )
		return OP_SET;
	else if ( strcmp(op, "unset") == 0 )
		return OP_UNSET;
	else if ( strcmp(op, "test") == 0 )
		return OP_TEST;
	else if ( strstr(op, "=") > 0)
		return OP_ASSIGN;
	else if ( strcmp(op, "if") == 0 )
		return OP_CONTROL;
	else if ( strcmp(op, "while") == 0)
		return OP_CONTROL;
	else if ( strcmp(op, "fi") == 0 )
		return OP_IGN;
	else if ( strcmp(op, "done") == 0)
		return OP_IGN;
	else if ( strcmp(op, "continue") == 0)
		return OP_IGN;
	else{
		return OP_EXTERNAL;
	}
}					

/* execute internal command */
/* cd, umask, fg, bg, jobs and shift are forced to run in the parent process */
void internal_cmd(struct command * cmd, int type, int fork){
	
	if ( fork > 0 ){							/* in a child process */
		switch (type){							/* branch */
			case OP_PWD : 
				pwd_main(cmd); 		break;
			case OP_ECHO :
				echo_main(cmd);		break;
			case OP_TIME :
				time_main(cmd);		break;
			case OP_CLR :
				clr_main(cmd);		break;
			case OP_DIR :
				dir_main(cmd);		break;
			case OP_ENVIRON :
				environ_main(cmd);	break;
			case OP_HELP :
				help_main(cmd);		break;
			case OP_TEST :
				test_main(cmd, 0);	break;
			case OP_CONTROL : 
				control_main(cmd);	break;
			default : 
									break;
		}
	}
	else{									/* if in the parent process */
		switch (type) {
			case OP_FG :
				fg_main(cmd);		break;
			case OP_BG :
				bg_main(cmd);		break;
			case OP_JOBS :
				jobs_main(cmd);		break;
			case OP_CD :
				cd_main(cmd);		break;
			case OP_UMASK :
				umask_main(cmd);	break;
			case OP_SHIFT :
				shift_main(cmd);	break;
			case OP_EXIT : 
				exit_main(cmd);		break;
			case OP_EXEC : 
				exec_main(cmd);		break;
			case OP_SET :
				set_main(cmd);		break;
			case OP_UNSET : 
				unset_main(cmd);	break;
			case OP_ASSIGN : 
				assign_main(cmd);	break;
			default :
				break;
		}
	}
}										
