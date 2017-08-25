/* file name : variables.c */
/* author name : Zhifeng Jiang ; ID : 3150101149 */
#include "myshell.h"

/* following are specifications of string process functions */

/* set search path */
void set_path(char* newPath){

	setenv("PATH", newPath, 1);					/* overwrite the PATH */

}

/* assign a local shell variables or create one */
void assign_main(struct command * cmd){
	
	int i, j;							/* for traverse the expression */
	int pos;							/* the position of '=' */
	int len;							/* the length of string */
	char ch;		
	char *name;							/* for construct the expression */
	char *content;
	name = (char *)malloc(MAX_CHAR * sizeof(char));
	content = (char *)malloc(MAX_CHAR * sizeof(char));
	memset(name, 0, MAX_CHAR);			/* clean */
	memset(content, 0, MAX_CHAR);

	len = strlen(cmd->args[0]);				/* figuring out where '=' is */
	for ( i = 0; i < len; i++ )
		if (cmd->args[0][i] == '=' ){
			pos = i;
			break;
	}	

	for ( i = 0, j = 0; i < pos; i++, j++)	/* construct the variable name */
		name[j] = cmd->args[0][i];
	name[j] = '\0';

	for ( i = pos+1, j = 0; i < len; i++ , j++)
		content[j] = cmd->args[0][i];
	content[j] = '\0';
	content = str_ref(content);				/* necessary replacement */

	if (find_var(name) == NULL)				/* do the assignment */
		add_var(name, content);
	else
		modify_var(name, content);

}

/* for command set */
void set_main(struct command * cmd){

	int i, j;						/* for iteration */
	
	if (cmd->arg_num == 1)			/* if no argument, display all the var. */
		print_all_var(cmd->outfd);	
	else{						/* otherwise set the positional variables */
		for ( i = 1, j = 1; i < cmd->arg_num; i++, j++){
			if ( j > 9 )
				break;								/* only set $1 - $9 */
			char* temp;
			temp = main_argv[j];
			main_argv[j] = (char*)malloc((strlen(cmd->args[i]) + 1) 
				* sizeof(char));					/* allocate space */
			cmd->args[i] = str_ref(cmd->args[i]);	/* reference process */
			strcpy(main_argv[j], cmd->args[i]);		/* set */
		}
		if ( j > main_argc )
			main_argc = j;
	}
	
}

/* for command unset */
void unset_main(struct command * cmd){

	if ( cmd->arg_num == 2 ){
		char* var = cmd->args[1];
		if (find_var(var) != NULL)						/* local var. */
			delete_var(var);
		else if (getenv(var) != NULL)					/* environ var */
			unsetenv(var);
	}

}	
	
/* for command test */
void test_main(struct command * cmd, int free_after_use){

	int code = 1;								/* exit code */
	int inv = 0;							/* mark if inverse the result */
	char *source, *target;						/* strings to be tested */

	if ( strcmp(cmd->args[1], "!") == 0 )
		inv = 1;
	
	if ( cmd->arg_num - inv == 3 ) {		/* if there are two arguments */
		source = cmd->args[2 + inv];	
		if ( cmd->args[1 + inv][1] == 'z' )			/* -z */
			code = strlen(find_var(source)) == 0 ? 1 : 0;
		else if ( cmd->args[1 + inv][1] == 'n' )		/* -n */
			code = strlen(find_var(source)) != 0 ? 1 : 0;
		else{									/* file test */
			struct stat buf;					/* for information fetching */
			if ( stat(source, &buf) < 0){
				if ( cmd->args[1 + inv][1] == 'e' )
					code = 0;
				exit(code);
			}
			
			if ( cmd->args[1 + inv][1] == 'd' )
				code = S_ISDIR(buf.st_mode);
			else if ( cmd->args[1 + inv][1] == 'e' )
				code = 1;
			else if ( cmd->args[1 + inv][1] == 'f' )
				code = S_ISREG(buf.st_mode);
			else if ( cmd->args[1 + inv][1] == 'g' )
				code = S_ISGID & buf.st_mode;
			else if ( cmd->args[1 + inv][1] == 'r' )
				code = S_IRUSR & buf.st_mode;
			else if ( cmd->args[1 + inv][1] == 's' )
				code = buf.st_size > 0 ? 1 : 0;
			else if ( cmd->args[1 + inv][1] == 'u' )
				code = S_ISUID & buf.st_mode;
			else if ( cmd->args[1 + inv][1] == 'w' )
				code = S_IWUSR & buf.st_mode;
			else if ( cmd->args[1 + inv][1] == 'x' )
				code = S_IXUSR & buf.st_mode;
		}
	}
	else {									/*  if there are three arguments*/
		source = cmd->args[1 + inv];
		target = cmd->args[3 + inv];
		source = str_ref(source);			/* necessary replacement */
		target = str_ref(target);
		if ( strcmp(cmd->args[2 + inv], "=") == 0 )
			code = cmp_var(source, target, 0) == 0 ? 1 : 0;
		else if ( strcmp(cmd->args[2 + inv], "!=") == 0 )
			code = cmp_var(source, target, 0) != 0 ? 1 : 0;
		else if ( strcmp(cmd->args[2 + inv], "-eq") == 0 )
			code = cmp_var(source, target, 1) == 0 ? 1 : 0;
		else if ( strcmp(cmd->args[2 + inv], "-ne") == 0 )
			code = cmp_var(source, target, 1) != 0 ? 1 : 0;
		else if ( strcmp(cmd->args[2 + inv], "-gt") == 0 )
			code = cmp_var(source, target, 1) > 0 ? 1 : 0;
		else if ( strcmp(cmd->args[2 + inv], "-ge") == 0 )
			code = cmp_var(source, target, 1) >= 0 ? 1 : 0;
		else if ( strcmp(cmd->args[2 + inv], "-lt") == 0 )
			code = cmp_var(source, target, 1) < 0 ? 1 : 0;
		else if ( strcmp(cmd->args[2 + inv], "-le") == 0 )
			code = cmp_var(source, target, 1) <= 0 ? 1 : 0;
	}

	if (free_after_use) {	/* free the space after calling from if / while */
		int i;
		for ( i = 0; i < cmd->arg_num; i++)
			free(cmd->args[i]);
		free(cmd);
	}
	
	if (inv)
		exit(code);								/* exit with a proper code */
	else
		exit(!code);
}

/* for command shift */
void shift_main(struct command * cmd){
	
	int i;											/* for iteration */
	int shift;										/* shift time */
	if ( cmd->arg_num == 1 )						/* no argument */
		shift = 1;
	else{
		sscanf(cmd->args[1], "%d", &shift);
	}
	
	if ( shift >= main_argc )
		return;
	
	char *temp = main_argv[3];
	for ( i = 1; i < main_argc; i++ ){			/* shifting */
		if ( i < main_argc - shift )
			main_argv[i] = main_argv[i+shift];
	}

	main_argc -= shift;
	
}

/* find a var. */
char* find_var(char* name){
	
	int i;										/* for iteration */
	
	for ( i = 0; i < vars->var_num; i++ )
		if ( strcmp(vars->local[i].name, name) == 0)
			return vars->local[i].content;		/* found, return the content */

	return NULL;								/* otherwise return nullptr */

}

/* delete a var. */
void delete_var(char* name){

	int i;										/* for iteration */
	int pos;									/* the position to be deleted */
	
	for ( i = 0; i < vars->var_num; i++ )				/* find the position */
		if ( strcmp(vars->local[i].name, name) == 0){
			pos = i;
			break;
		}
			
	for ( i = pos; i < vars->var_num; i++ ){			/* delete */
		strcpy(vars->local[i].name, vars->local[i+1].name);
		strcpy(vars->local[i].content, vars->local[i+1].content);
	}

	vars->var_num --;							/* update the amount of var. */	
}	

/* add a var. */
void add_var(char* name, char* content){

	strcpy(vars->local[vars->var_num].name, name);
	strcpy(vars->local[vars->var_num].content, content);
	vars->var_num ++;							/* update the amount of var. */	
	
}	

/* modify a var. */
void modify_var(char* name, char* content){
	
	int i;												/* for iteration */

	for ( i = 0; i < vars->var_num; i++ )				/* find the position */
		if ( strcmp(vars->local[i].name, name) == 0)
			break;
	strcpy(vars->local[i].content, content);

}

/* compare a var. */
int cmp_var(char* source, char* target, int mode){
	
	if ( mode == 0 ){						/* comparation between strings */
		return strcmp(source, target);
	}
	else{									/* comparation between integers */
		int src, trg;
		sscanf(source, "%d", &src);
		sscanf(target, "%d", &trg);
		return (src - trg);
	}
}

/* print all the local shell var. */
void print_all_var(int outfd){
	
	int i;										/* for iteration */

	for ( i = 0; i < vars->var_num; i++){	/* write all the var. to outfd */
		printf("%s=", vars->local[i].name);
		printf("%s\n", vars->local[i].content);
	}
}
