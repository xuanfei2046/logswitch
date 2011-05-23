#include <stdio.h>
#include <string.h>
#include <pcre.h>
#include "regex.h"

#define OVECCOUNT 30    /* should be a multiple of 3 */

int read_config_regex(char *pattern, char *subject){
	pcre *re;
	const char *error;
	int pattern_length;
	int subject_length;
	int erroffset, rc;
	int ovector[OVECCOUNT];

	subject_length = (int)strlen(subject);
	pattern_length = (int)strlen(pattern);
	
	if (!(subject_length && pattern_length)) return -99;
		

	re = pcre_compile(
			pattern,              /* the pattern */
			0,                    /* default options */
			&error,               /* for error message */
			&erroffset,           /* for error offset */
			NULL);                /* use default character tables */
	/* Compilation failed: print the error message and exit */

	if (re == NULL)
	{
		printf("Warnning: Check configure etc/css_command_deny [%s]\n",pattern);
		//printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
		return -98;
	}

	rc = pcre_exec(
			re,                   /* the compiled pattern */
			NULL,                 /* no extra data - we didn't study the pattern */
			subject,              /* the subject string */
			subject_length,       /* the length of the subject */
			0,                    /* start at offset 0 in the subject */
			0,                    /* default options */
			ovector,              /* output vector for substring information */
			OVECCOUNT);           /* number of elements in the output vector */

	/*
	   if (rc < 0)
	   {
	   switch(rc)
	   {
	   case PCRE_ERROR_NOMATCH: printf("No match\n"); break;
	   default: printf("Matching error %d\n", rc); break;
	   }
	   pcre_free(re);  
	   return 1;
	   }

	   printf("\nMatch succeeded at offset %d\n", ovector[0]);
	 */
	pcre_free(re);     
	return(rc);
}
