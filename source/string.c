/* file name : string.c */
/* author name : Zhifeng Jiang ; ID : 3150101149 */
#include "myshell.h"

/* following are specifications of string process functions */

/* replace a substring in a given string with a new substring */
/* direct modifying in the orginal string, so no value need returning */
void str_rep(char* org, char* src, char* trg){
	
	char* start = strstr(org, src);		/* find the start of the substring */
	int pos;							/* the position of that start */
	char temp[MAX_CHAR];				/* for intermediate usage */
			
	while(start){						/* when such a substring can be found */
		memset(temp, 0, sizeof(temp));
		pos = start - org;
		strncpy(temp, org, pos);					/* copy the front */
		strcat(temp, trg);							/* replace the source */
		strcat(temp, org + pos + strlen(src));		/* copy the tail */
		strcpy(org, temp);							/* paste the new */
		start = strstr(org, src);		/* next searching */
	}
}

/* split string */
/* split the string and place all the substring in an array, then return the */
/* amount of substrings */
int str_spl(char* string, char** array){

	int i, j, k;
	int count = 0;									/* amount of substrings */
	int len = strlen(string);						/* length of the whole */
	int front = 0, rear = -1;						/* two ends of substring */

	for ( i=0; i<=len; i++){
		if (i == len || string[i] == ' ' || string[i] == '\t' 
			|| string[i] == '\n' ){				
												/* if meet space or ending */
			if ( front <= rear ){				/* extract substring */

				if(array[count])				/* first free the existing */
					free(array[count]);			

				array[count] = (char*)malloc(	/* then allocate new space */
					( rear - front + 2) * sizeof(char));
				for( j=front, k=0; j<=rear; j++, k++)
					array[count][k] = string[j];	/* copy to string array */
				array[count][k] = '\0';
				count ++;

				if ( i < len && string[i] == '\n' && string [i-1] != '\n' 
					&& string[i-1] != ' ' && string[i-1] != '\t' ){
											/* if meet a newline ,add a '`' */
					array[count] = (char*)malloc(2*sizeof(char*));
					strcpy(array[count], "`");
					count ++;
				}
			}
			front = i+1;		/* mark the new ends of the next substring */
			rear=i;
		}
		else {
			rear++;				/* not meet space or end, extend */
		}
	}
	return count;
}

/* if the string has some variables begin with '$' */
/* they should be replaced by the actual contents them are referring to */
/* this function helps doing things like that */
char* str_ref(char* string){
	
	int i, j;						/* for iteration */
	int pos;						/* the position of '+' , if exists */
	char* start;
	char replace[MAX_CHAR];
	memset(replace, 0, MAX_CHAR);
	
	start = strstr(string, "+");
	pos = start - string;			/* fetch the position of '+' */
	if( start != NULL ){

		int addend, augend;
		char *temp;
		temp = (char *)malloc(MAX_JOBS * sizeof(char));
		
		memset(temp, 0, MAX_JOBS);				/* fetching the addend */
		for (i = 0, j = 0; i < pos; i++, j++ )
			temp[j] = string[i];
		temp[j] = '\0';
		temp = str_ref(temp);
		sscanf(temp, "%d", &addend);

		memset(temp, 0, MAX_JOBS);				/* fetching the augend */
		for (i = pos + 1, j = 0; i < strlen(string); i++, j++ )
			temp[j] = string[i];
		temp[j] = '\0';
		temp = str_ref(temp);
		sscanf(temp, "%d", &augend);
		
		addend += augend;						/* do the addition */
		
		memset(temp, 0, MAX_JOBS);
		sprintf(temp, "%d", addend);		/* store the answer and return */
		return temp;
	}
	else if( string[0] == '$' ){	/* if need to replace var. quoted by '$' */

		/* Step 1 : figuring out the referencing */
		if ( string[1] <= '9' && string[1] >= '0'){		/* location */
			int num = (int)(string[1] - '0');
			sprintf(replace, "%s", main_argv[num]);
		}
		else if ( string[1] == '@' || string[1] == '*' ){/* all arguments */
			int i;
			for ( i = 0; i < main_argc; i++){
				strcat(replace, main_argv[i]);
				if ( i != main_argc - 1)
					strcat(replace, " ");
			}
		}
		else if ( string[1] == '#')						/* amound of arg. */
			sprintf(replace, "%d", main_argc);
		else if ( string[1] == '$')						/* pid of myshell */
			sprintf(replace, "%d", fpid);
		else if ( string[1] == '!'){					/* pid of last bg */
			if (jobs->back_num)
				sprintf(replace, "%d", 
					jobs->back_jobs[jobs->back_num - 1].pid);
		}
			
		else if ( string[1] == '?')						/* return status */
			sprintf(replace, "%d", WEXITSTATUS(status));
		else{						/* other environment variables */
			int j;
			char var[MAX_CHAR];

			memset(var, 0, MAX_CHAR);
			for ( j = 1; j < strlen(string); j ++)
				var[j-1] = string[j];
			var[j-1] = '\0';

			if (find_var(var) != NULL)						/* local var. */
				sprintf(replace, "%s", find_var(var));
			else if (getenv(var) != NULL)					/* environ var */
				sprintf(replace, "%s", getenv(var));
			else
				return string;
		}
		
		/* Step 2 : replacing */
		char* temp = string;
		string = (char* )malloc((strlen(replace) + 1) * sizeof(char));
		strcpy(string, replace);				/* change the pointer */
		if(temp)
			free(temp);							/* free the old string */
		return string;
	}
	else 										/* if no need to replace */
		return string;

}
