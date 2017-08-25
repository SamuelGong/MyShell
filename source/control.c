/* file name : control.c */
/* author name : Zhifeng Jiang ; ID : 3150101149 */
#include "myshell.h"

/* following are funtions about control structure */

/* for if or while test */
void control_main(struct command * cmd){
	
	int i;										/* for iteration */
	int len;									/* length of argument */

	/* Step 1 : construct the test command */
	/* allocating space */
	struct command * test = (struct command *)malloc(sizeof(struct command));
	test->arg_num = cmd->arg_num - 1;
	test->args = (char **)malloc( test->arg_num * sizeof(char *) );

	/* copying the arguments */
	for ( i = 1 ; i < cmd->arg_num; i++ ){
		len = strlen(cmd->args[i]) + 1;
		test->args[i-1] = (char *)malloc( len * sizeof(char) );
		strcpy(test->args[i-1], cmd->args[i]);
	}
	
	/* Step 2 : execute the test command */
	test_main(test, 1);
	
}

/* construct the control structure list */
/* to be exact, parse the commands again */
/* and figure out all the beginnings and ends of the if and while structures */
void has_control(struct command * cmds, int cmd_num){

	int i, j;								/* for iteration */
	int s1[MAX_SEGS], s2[MAX_SEGS];
											/* stack */
	int t1 = -1, t2 = -1;					/* stack top pointers */

	for ( i = 0; i < cmd_num; i++ ){		/* simulating the stack */
		
		/* parsing if then ... fi structure */
		if ( strcmp(cmds[i].args[0], "if") == 0 ){
			s1[++t1] = i;
		}
		else if ( strcmp(cmds[i].args[0], "fi") == 0 ){
			cons->if_list[cons->if_num].cur = s1[t1];
			cons->if_list[cons->if_num].next[0] = 2 + s1[t1--];
			cons->if_list[cons->if_num].next[1] = i + 1 ;
			cons->if_num ++;
		}
		
		/* parsing while do ... done structure */
		else if ( strcmp(cmds[i].args[0], "while") == 0 ){
			s2[++t2] = i;
		}
		else if ( strcmp(cmds[i].args[0], "continue") == 0 ){
			cons->continue_list[cons->continue_num].cur = i;
			cons->continue_list[cons->continue_num].next[0]
				= cons->continue_list[cons->continue_num].next[1] = s2[t2];
			cons->continue_num ++;
		}
		else if ( strcmp(cmds[i].args[0], "done") == 0 ){
			cons->while_list[cons->while_num].cur = s2[t2];
			cons->while_list[cons->while_num].next[0] = 2 + s2[t2--];
			cons->while_list[cons->while_num].next[1] = i + 1 ;
			cons->while_num ++;

			cons->done_list[cons->done_num].cur = i;
			cons->done_list[cons->done_num].next[0] = 
				cons->done_list[cons->done_num].next[1] = s2[t2 + 1];
										/* match the while */
			cons->done_num ++;
		}
	}
}

/* figuring out the next command when reaching */
/* the beginning or the end of a control structure */
int next_command(char* cmd, int current, int exit){

	int i;										/* for iteration */

	int num;									/* for simplify searching */
	struct control * source;

	if ( strcmp(cmd, "if") == 0 ){
		num = cons->if_num;
		source = cons->if_list;
	}
	else if ( strcmp(cmd, "while") == 0 ){
		num = cons->while_num;
		source = cons->while_list;
	}
	else if ( strcmp(cmd, "done") == 0 ){
		num = cons->done_num;
		source = cons->done_list;
	}
	else if ( strcmp(cmd, "continue") == 0 ){
		num = cons->continue_num;
		source = cons->continue_list;
	}
	else 								/* return directly if is other cmd. */
		return current;

	for ( i = 0; i < num; i++ )			/* search for control record */
		if ( source[i].cur == current )
			break;

	return source[i].next[exit] - 1;

}
