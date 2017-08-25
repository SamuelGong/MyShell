/* file name : utility.c */
/* author name : Zhifeng Jiang ; ID : 3150101149 */
#include "myshell.h"

/* following are specifications of common command process functions */

/* pwd */
void pwd_main(struct command * cmd){
	
	char pwd[MAX_CHAR];						/* for current path fetching */
	getcwd(pwd, sizeof(pwd) - 1);			/* get the current path */
	printf("%s\n", pwd);

}

/* echo */
void echo_main(struct command * cmd){

	int i;										/* for iteraton */

	for (i = 1; i < cmd->arg_num ; i += 1){	/* echo the rest of the cmd */
		cmd->args[i] = str_ref(cmd->args[i]);			
										/* replace the ref. if necessary */
		printf("%s", cmd->args[i]);
		if ( i != cmd->arg_num - 1 )
			printf(" ");				/* add space */
	}
	printf("\n");

}

/* cd */
void cd_main(struct command * cmd){
	
	int err;									/* error code */
	char* home;									/* for home path fetching */
	home = getenv("HOME");						/* get the home path */

	if( cmd->arg_num == 1 ){					/* default is home dir. */
		err = chdir(home);						/* change */
		if ( err < 0 )
			display_error("Fail to change  directory!");
    }
	else{									/* a dir. designated */
		str_rep(cmd->args[1], "~", home);	/* replace '~' with home path */
		err = access(cmd->args[1], F_OK);	/* check if exist */
	
		if ( err < 0 )
			display_error("No such directory!");
		else{
			err = chdir(cmd->args[1]);			/* change */
			if ( err < 0 )
				display_error("Fail to change directory!");
		}
	}
}

/* time */
void time_main(struct command * cmd){
	time_t temp;
	struct tm *p;
	time(&temp);
	p = localtime(&temp);					/* obtain the time structure */

	char* weekday[8] = {"Sunday", "Monday", "Tuesday", "Wednesday",
						"Thursday", "Friday", "Saturday"};
											/* for display weekday */

	printf("%d-%02d-%02d %s %d:%02d:%02d\n", 
		1900+p->tm_year, 1+p->tm_mon, p->tm_mday, weekday[p->tm_wday],
		p->tm_hour, p->tm_min, p->tm_sec);
											/* print the result */

}

/* clr */
void clr_main(struct command * cmd){

	fprintf(stdout, "\033c");					/* esc + c */
	
}

/* dir */
void dir_main(struct command * cmd){

	int err;									/* for error code */
	char* home;									/* for home path fetching */
	char dir[MAX_CHAR];							/* the wanted directory */
	memset(dir, 0, MAX_CHAR);					/* initialize */
	
	/* Step 1 : get the directory */
	if (cmd->arg_num > 1 ){				/* if there is a path desinated */ 

		home = getenv("HOME");				/* get the home path */
		str_rep(cmd->args[1], "~", home);	/* replace '~' with home path */
		
		err = access(cmd->args[1], F_OK);	/* check if exist */
		if ( err < 0 ){
			display_error("No such directory!");
			return;							/* if fails, warn and return */
		}
		strcpy(dir, cmd->args[1]);
	}
	else{						/* default is the current working directory */
		getcwd(dir, sizeof(dir) - 1);
	}

	DIR *dp = opendir(dir);
	struct dirent *d_st;					/* folder information structure */

	if (!dp){
		display_error("Fail to access the directory!");
		return;							/* if access fails, warn and exit */
	}

	/* Step 2 : list all the files or sub-directory */
	int counter = 0;									/* for format */
	while ( (d_st = readdir(dp)) != NULL ){		
		counter ++;
		printf("%15s", d_st->d_name);
		if ( counter % 5 == 0)
			printf("\n");
	}
	printf("\n");

}

/* environ */
void environ_main(struct command * cmd){

	int i;										/* for iteration */	
	extern char **environ;		/* external variables that store the result */
	
	
	/* list all the environment variables */
	for( i = 0; environ[i] != NULL; i++ )
		printf("%s\n", environ[i]);

}

/* umask */
void umask_main(struct command * cmd){
	
	int new_mask, old_mask;						/* temporate usage */

	if( cmd->arg_num > 1 ){			/* if there is a new mask desinated */ 
		sscanf(cmd->args[1], "%o", &new_mask);
		umask(new_mask);			/* setting */
	}
	else{
		old_mask = umask(18);		/* simply for obtain the old umask */
		umask(old_mask);
		printf("%04o\n", old_mask);
	}	
	
}

/* help */
void help_main(struct command * cmd){

	int fd;									/* the fd of the manual */
	char doc_path[MAX_CHAR];				/* the path of the manual */
	char print[MAX_CHAR];					/* the content of the manual */
	memset(print, 0, MAX_CHAR);				/* clear the buffer pool */

	strcpy(doc_path, father_path);
	strcat(doc_path, "doc/");				/* to obtain the absolute path */
	
	if ( cmd->arg_num == 1 )				/* if there are no argument */
		strcat(doc_path, "general");		/* show the general manual */
	else{
		strcat(doc_path, cmd->args[1]);		/* else show the specific one */
	}

	fd = open( doc_path, O_RDONLY );		/* open the manual */
	if ( fd < 0 ){
		display_error("Command doesn't exist!");
		return;								/* if fails, tips and return */
	}
	read(fd, print, MAX_CHAR);						/* read the content */
	printf("%s", print);							/* print the result */
	close(fd);										/* close the input fd */

}

/* exit */
void exit_main(struct command * cmd){
	
	int code = 0;									/* exit code */
	if ( cmd->arg_num == 2 )
		sscanf(cmd->args[1], "%d", &code);
	
	fflush(stdout);									/* clean the buffer */
	fflush(stdin);
	exit(code);
}

/* exec */
void exec_main(struct command * cmd){
	
	int i;										/* for iteration */
	int len;									/* length of argument */
	int err;									/* error code */

	/* Step 1 : construct the external command */
	/* allocating space */
	struct command * ext = (struct command *)malloc(sizeof(struct command));
	ext->arg_num = cmd->arg_num - 1;
	ext->args = (char **)malloc( ext->arg_num * sizeof(char *) );

	/* copying the arguments */
	for ( i = 1 ; i < cmd->arg_num; i++ ){
		len = strlen(cmd->args[i]) + 1;
		ext->args[i-1] = (char *)malloc( len * sizeof(char) );
		strcpy(ext->args[i-1], cmd->args[i]);
	}
	
	/* Step 2 : exec the command as external one */
	err = execvp(ext->args[0], ext->args);
	if( err < 0 )
		display_error("Cannot find the command!");

}
