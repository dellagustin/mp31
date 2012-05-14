		      /* Prosty SHELL 1996 03 20/26 */

#ifndef SHELL_H
#define SHELL_H

#define pause()			                	  \
 {                             		                  \
   fprintf(stdout,"<<< NEXT - ENTER (q - STOP) >>>");   \
   if(getchar()=='q')					  \
    {							  \
      (void)getchar();   	            	          \
      break;                                              \
    }							  \
 }

typedef void (FUNCTION)(char *);

typedef struct {
  int index;
  char *name;
} PERMUT;

typedef struct {
  char *command;			/* Nazwa komedy zewnetrznej */
  void (*func)(char *);                 /* Funkcje od argumetow */
 } COMMAND;

extern PERMUT permut[];                 /* Tablica permutacji */

extern void StrToArgv(char *,char **,int *);    /* Wydobywanie z napisu zestawu opcji */
extern void FreeArgv(char **,int);		/* Zwolnienie pamieci po argumetach */
extern int Batch(COMMAND *,char *,char **);	/* Wykonanie zestawu funkcji */
extern void Shell(COMMAND *,char **);		/* Interakcyjne shell */
						/* Zestaw komend + help */
#endif
