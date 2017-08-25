/* file name : process.c */
/* author name : Zhifeng Jiang ; ID : 3150101149 */
#include "myshell.h"

/* following are specifications of functions about process manipulation */

/* parsing background command */
/* to check if the command needs to be run in the background */
/* record the result properly */
void is_background(struct command * cmd){
	
	// Part 1 : see if it is a backgound command
	if ( strcmp(cmd->args[cmd->arg_num - 1], "&") == 0 )/* by seeking '&' */
		cmd->backgnd = 1;								/* set */
	else
		cmd->backgnd = 0;								/* unset */

	// Part 2 : if it is, need to remove & from the arguments
	if (cmd->backgnd == 1) {
		int i;											
		char** temp;									
		temp = (char **)malloc((cmd->arg_num - 1)*sizeof(char *));
		for( i = 0; i < cmd->arg_num - 1; i++)
			temp[i] = cmd->args[i];	/* save the necessary arguments */
		free(cmd->args[i]);			/* free the space of '&' */

		cmd->arg_num --;			/* update the content of the  structure */
		cmd->args = temp;
	}
}


/* fg */
void fg_main(struct command * cmd){
	signal(SIGTSTP, stop_job);			/* need also response to ctrl + z */
	pid_t pid;						/* the pid of the job */
	int i;							/* for iteration */
	int current;					/* the job that is to be process */

	if(jobs->back_num){					/* if the list is not empty */

		if ( cmd->arg_num == 1 )
			current = jobs->back_num - 1;				/* default job */
		else{
			sscanf(cmd->args[1], "%d", &current);		/* designated job */
			if (current < 1 || current > jobs->back_num){
				display_error("No such a job!");
				return;
			}
			current -= 1;
		}

		pid = jobs->back_jobs[current].pid;				/* obtain the pid */
		printf("%s\n", jobs->back_jobs[current].name);	/* display */
		
		change_job(1, current);						/* modify the job list */
		kill(pid, SIGCONT);							/* continue to run */
		waitpid(pid, &status, WUNTRACED);			/* wait for the process */
		
	}
	else
		display_error("No jobs!");
}

/* bg */
void bg_main(struct command * cmd){

	pid_t pid;						/* the pid of the job */
	int i;							/* for iteration */
	int current;					/* the job that is to be process */
	
	if(jobs->back_num){			/* if the list is not empty */

		if ( cmd->arg_num == 1 )
			current = jobs->back_num - 1;				/* default job */
		else{
			sscanf(cmd->args[1], "%d", &current);		/* designated job */
			if (current < 1 || current > jobs->back_num){
				display_error("No such a job!");
				return;
			}
			current -= 1;
		}

		pid = jobs->back_jobs[current].pid;;
		if ( current == jobs->back_num - 1 )
			printf("[%d]+    %s &\n", 
				current+1, jobs->back_jobs[current].name);
		else if ( current == jobs->back_num - 2 )
			printf("[%d]-    %s &\n", 
				current+1, jobs->back_jobs[current].name);
		else
			printf("[%d]     %s &\n", 
				current+1, jobs->back_jobs[current].name);


		kill(pid, SIGCONT);						/* continue to run */
		strcpy(jobs->back_jobs[current].state, "running");	
												/* change the state */
	}
	else
		display_error("No jobs!");
}

/* jobs */
void jobs_main(struct command * cmd){
	
	int i;									/* for iteration */

	for ( i = 0; i < jobs->back_num; i++){
		/* display the job id, state and name */
		
		if ( i == jobs->back_num - 1 )
			printf("[%d]+    %s\t\t%s", 
				i+1, jobs->back_jobs[i].state, jobs->back_jobs[i].name);
		else if ( i == jobs->back_num - 2)
			printf("[%d]-    %s\t\t%s", 
				i+1, jobs->back_jobs[i].state, jobs->back_jobs[i].name);
		else
			printf("[%d]     %s\t\t%s", 
				i+1, jobs->back_jobs[i].state, jobs->back_jobs[i].name);

		/* if is running, need to display an extra '&' */
		if ( strcmp(jobs->back_jobs[i].state, "running" ) == 0 )
			printf(" &\n");
		else
			printf("\n");

	}

}

/* manipulation on job list when a job starts */
void add_job(struct command * cmd, pid_t pid){

	int i;										/* for iteration */
	int * num;									/* index in the list */	
	struct job * target;						/* the list a job adds to */
	
	// Part 1 : choose a job list
	if(cmd->backgnd){
		target = jobs->back_jobs;
		num = &(jobs->back_num);
	}
	else{
		target = jobs->fore_jobs;
		num = &(jobs->fore_num);
	}
	// Part 2 : add the job into the list
	target[*num].pid = pid;						/* initial the pid */
	memset(target[*num].name, 0, MAX_CHAR);		/* intial the name */
	for ( i = 0; i < cmd->arg_num; i++){
		strcat(target[*num].name, cmd->args[i]);
		if ( i != cmd->arg_num - 1 )
			strcat(target[*num].name, " ");
	}
	strcpy(target[*num].state, "running");		/* initial the state */
	*num = *num + 1;
}

/* for ctrl + z */
void stop_job(){
	
	int current;	
	
	/* Step 1 : stop the latest foreground job and change it into background */
	if(jobs->fore_num == 0)			/* if there is no fore ground job */
		return;
	else{
		current = jobs->fore_num - 1;		/* stop the most recent job */
		strcpy(jobs->fore_jobs[current].state, "stopped");
		kill(jobs->fore_jobs[current].pid, SIGSTOP);

		change_job(0, current);				/* modify the two job list */
	}

	/* Step 2 : isplay some prompts	*/
	current = jobs->back_num - 1;
	printf("\n[%d]+    %s\t\t%s\n", current+1, 
		jobs->back_jobs[current].state, jobs->back_jobs[current].name);

}

/* manipulation on job list when a job ends */
void delete_job(pid_t pid, int backgnd){
	
	int i;							/* for iteration */	
	int current;					/* the index to be deleted */
	int found = 0;					/* the flag */
	struct job * source;			/* the list a job deletes from */
		
	if (!backgnd){					/* search in the foreground job list */
		source = jobs->fore_jobs;
		for ( i=0; i<jobs->fore_num; i++ )	/* for each job, check the pid */
			if(source[i].pid == pid){
				current = i;
				found = 1;
				break;
			}
		if(found){							/* if finds, remove the job */
			for ( i = current; i < jobs->fore_num - 1; i++ ){	
				source[i].pid = source[i+1].pid;
				strcpy(source[i].name, source[i+1].name);
				strcpy(source[i].state, source[i+1].state);
			}
			jobs->fore_num --;
		}
	}
	else{							/* search in the background job list */
		source = jobs->back_jobs;
		for ( i=0; i<jobs->back_num; i++ )	/* for each job, check the pid */
			if(source[i].pid == pid){
				current = i;
				break;
			}

		/* display the finish state for the background command */
		if ( current == jobs->back_num - 1)
			printf("[%d]+    %s\t%s\n", current+1, 
				"finished", source[current].name);
		else if ( current == jobs->back_num - 2)
			printf("[%d]-    %s\t%s\n", current+1, 
				"finished", source[current].name);
		else
			printf("[%d]     %s\t%s\n", current+1, 
				"finished", source[current].name);
		
		for ( i = current; i < jobs->back_num - 1; i++ ){/* remove the job */
			source[i].pid = source[i+1].pid;
			strcpy(source[i].name, source[i+1].name);
			strcpy(source[i].state, source[i+1].state);
		}
		jobs->back_num --;
	}	

}

/* manipulation on job list when changes the list a job belongs to */
void change_job(int backgnd, int cur){
	
	int i;							/* for iteration */
	int *num_s, *num_t;				/* length of the two list */
	struct job * source;			/* list that job moves from */
	struct job * target;			/* list that job moves to */

	// Step 1 : identify the pointers
	if (backgnd){					/* from background to foreground */
		num_s = &(jobs->back_num);
		num_t = &(jobs->fore_num);
		source = jobs->back_jobs;
		target = jobs->fore_jobs;
	}
	else{							/* form foreground to background */
		num_s = &(jobs->fore_num);
		num_t = &(jobs->back_num);
		source = jobs->fore_jobs;
		target = jobs->back_jobs;
	}

	// Step 2 : add to another list
	target[*num_t].pid = source[cur].pid;
	strcpy(target[*num_t].name, source[cur].name);
	strcpy(target[*num_t].state, source[cur].state);
	*num_t = *num_t + 1;

	// Step 3 : eliminate the record in the orginal list
	for ( i = cur; i < *num_s - 1; i++ ){	
		source[i].pid = source[i+1].pid;
		strcpy(source[i].name, source[i+1].name);
		strcpy(source[i].state, source[i+1].state);
	}
	*num_s = *num_s - 1;
}
	
/* display the finished background jobs and modify the background job list */
void display_finish(){
	
	int i;									/* for iteration */
	pid_t p_ret, p_child;		/* return value of waitpid, pid of child */

	for ( i = 0; i < jobs->back_num; i++ ){/* check of all the bg jobs once */
		p_child = jobs->back_jobs[i].pid;
		p_ret = waitpid(p_child, NULL, WNOHANG);
		if (p_ret == p_child)
			delete_job(p_child, 1);
	}
}						
