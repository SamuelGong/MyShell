/* file name : myshell.h */
/* author name : Zhifeng Jiang ; ID : 3150101149 */

#ifndef _MYSHELL_H
#define _MYSHELL_H
#include <stdio.h>							/* standartd I/O */
#include <stdlib.h> 						/* getenv etc included */
#include <unistd.h>							/* */
#include <pwd.h>							/* getpwuid etc included */
#include <string.h>							/* about string process */
#include <sys/types.h>						/* */
#include <sys/stat.h>						/* stat etc included */
#include <sys/wait.h>						/* waitpid etc included */
#include <fcntl.h>							/* */
#include <dirent.h>							/* for directory accessing */
#include <time.h>							/* for time fetching */
#include <errno.h>							/* for testing */
#define MAX_JOBS 10							/* the maximum amount of jobs */
#define MAX_SEGS 500					/* the maximum number of segments */
#define MAX_CHAR 5000					/* the maximum length of a string */

/* following are external variables from myshell.c */
/* you can obtain their descritions from myshell.c */
extern int status;
extern int main_argc;
extern pid_t fpid;
extern char** main_argv;
extern struct con_list * cons;
extern struct var_list * vars;
extern struct job_list * jobs;
extern char father_path[];

/* enumeration of operations */
enum op{
	OP_PWD,									/* pwd */
	OP_ECHO,								/* echo */
	OP_CD,									/* cd */
	OP_TIME,								/* time */
	OP_CLR,									/* clr */
	OP_DIR,									/* dir */
	OP_ENVIRON,								/* environ */
	OP_UMASK,								/* umask */
	OP_FG,									/* fg */
	OP_BG,									/* bg */
	OP_JOBS,								/* jobs */
	OP_HELP,								/* help */
	OP_SHIFT,								/* shift */
	OP_EXIT,								/* exit */
	OP_EXEC,								/* exec */
	OP_ASSIGN,								/* = */
	OP_SET,									/* set */
	OP_UNSET,								/* unset */
	OP_TEST,								/* test */
	OP_CONTROL,								/* if and while */
	OP_IGN,									/* fi , continue and done */
	OP_EXTERNAL								/* external commands */
};


/* declaration of command structure */
struct command{
	int backgnd;							/* if run in the background */
	int infd;								/* input file descriptor */
	int outfd;								/* output file descriptor */
	int arg_num;							/* amount of arguments */
	char** args;							/* array of arguments */
};


/* declaration of pipe structure */
struct pipe{
	int fd[2];								/* file descriptors of pipes */
};


/* declaration of job structure */
struct job{
	pid_t pid;								/* the pid of the job */
	char name[MAX_CHAR];					/* the name of the job */
	char state[MAX_CHAR];					/* the state of the job */
};


/* declaration of job list structure */
struct job_list{
	int fore_num;							/* amount of foreground jobs */
	int back_num;							/* amount of background jobs */
	struct job fore_jobs[MAX_JOBS];			/* list of foreground jobs */
	struct job back_jobs[MAX_JOBS];			/* list of background jobs */
};

/* declaration of local shell variable structure */
struct var{
	char name[MAX_CHAR];					/* name of the variable */
	char content[MAX_CHAR];					/* value of the variable */
};

/* declaration of variable list structure */
struct var_list{
	int var_num;							/* amount of variables */
	struct var local[MAX_SEGS];				/* list of variables */
};

/* declaration of control structure */
struct control{
	int cur;								/* index of current cmd. */
	int next[2];							/* table of indices of next cmd. */
};

/* declaration of control list structure */
struct con_list{
	int if_num;							/* amount if control structure */
	int while_num;
	int done_num;
	int continue_num;
	struct control if_list[MAX_SEGS];		/* control structure list */
	struct control while_list[MAX_SEGS];
	struct control done_list[MAX_SEGS];
	struct control continue_list[MAX_SEGS];
};


/* following are declarations of all the basic functions */
/* specifications are in myshell.c */
void internal_cmd(struct command * cmd, int type, int fork);
											/* execute internal command */
void init(char** args, struct command * cmds, struct pipe * pipes);
											/* initialization */
int cmd_type(struct command * cmd); 		/* parsing internal command */
int common_cmd(struct command * cmd);		/* normal cmd execute entrance */


/* following are declarations of common command process functions */
/* specifications are in utility.c */
void pwd_main(struct command * cmd);		/* pwd */
void echo_main(struct command * cmd);		/* echo */
void cd_main(struct command * cmd);			/* cd */
void time_main(struct command * cmd);		/* time */
void clr_main(struct command * cmd);		/* clr */
void dir_main(struct command * cmd);		/* dir */
void environ_main(struct command * cmd);	/* environ */
void umask_main(struct command * cmd);		/* umask */
void help_main(struct command * cmd);		/* help */
void shift_main(struct command * cmd);		/* shift */
void exit_main(struct command * cmd);		/* exit */
void exec_main(struct command * cmd);		/* exec */


/* following are specifications of functions about process manipulation */
/* specifications are in process.c */
void fg_main(struct command * cmd);			/* fg */
void bg_main(struct command * cmd);			/* bg */
void jobs_main(struct command * cmd);		/* jobs */
void stop_job();							/* for ctrl + z */
void is_background(struct command * cmd);	/* parsing background command */
void change_job(int backgnd, int current);
		/* manipulation on job list when changes the list a job belongs to */
void add_job(struct command * cmd, pid_t pid);
							/* manipulation on job list when adds a job */
void delete_job(pid_t pid, int backgnd);	
							/* manipulation on job list when deletes a job */
void display_finish();
/* display the finished background jobs and modify the background job list */


/* following are declarations of funtions about variables */
/* specifications are in variable.c */
void set_path(char* newPath);   					/* set search path */
void assign_main(struct command * cmd);				/* = */
void set_main(struct command * cmd);				/* set */
void unset_main(struct command * cmd);				/* unset */
void test_main(struct command * cmd, int free_after_use);		
													/* test */
void delete_var(char* name);						/* delete a var. */
void print_all_var(int outfd);				/* print all the local shell var. */
void add_var(char* name, char* content);			/* add a var. */
void modify_var(char* name, char* content);			/* modify a var. */
int cmp_var(char* source, char* target, int mode);	/* compare a var. */
char* find_var(char* name);							/* find a var. */


/* following are declarations of string process functions */
/* specifications are in string.c */
char* str_ref(char* string);					/* replace by the reference */
int str_spl(char* string, char** array);		/* split string */
void str_rep(char* org, char* src, char* trg);	/* replace by a substring */


/* following are declarations of functions about control structure */
/* specifications are in control.c */
void control_main(struct command * cmd);		/* if and while */
int next_command(char* cmd, int current, int exit);		
												/* fetching the next cmd. */
void has_control(struct command * cmds, int cmd_num);
									/* construct the control structure list */


/* following are declarations about I/O */
/* specifications are in io.c */
void display_prompt(); 						/* display prompt */
void display_error(char* str);				/* display error */
void dup_fd(struct command * cmd);			/* duplicate fds if necessary */
void close_fd(struct command * cmd);
									/* close all the fds except stardard I/O */
int read_from_terminal(char* cmd);			/* read input from terminal */
int read_from_file(char* cmd, char* file);	/* read input from a batch file */
int has_io_redirect(struct command * cmd);  /* parsing redirection command */
int has_pipe(char** args, int arg_num, struct command* cmd, struct pipe* pipes, int mode);
						 					/* parsing pipe command */


#endif
