			   /* SHELL 1996 03 19/20/23/26/29/30 04/29 1996 10 10; 1997 08 28 */
#include <stdio.h>	   /* funkcja ls wymaga zgodnosci z POSIX (obsluga katalogow) */
#include <stdlib.h>	   /* ustawienie definicji MYNOPOSIX wylancza te funkcje */
#include <string.h>	   /* UNIXPATH   - styl UNIX'a nazw plikow */
#include <ctype.h>         /* 1998 01 01 - parametryzowalne skrypty */
#ifndef MYNOPOSIX          /* 1999 06 11 - wielostopniowa sciezka */
#include <dirent.h>        /* 1999 06 30 - alfabetyczny help */ 
#include <sys/stat.h>
#endif
#include <time.h>
#include "shell.h"
#include "proto.h"

#define MAXCMD     256			/* Dlugosc argumentu lub komedy */
#define MAXHISTORY 512			/* zakres histor"ii shella */
#define MAXARGSIZE 80			/* Maksymalny rozmiar argumetow */
#define MAXROWS    2048			/* Maksymalna liczba wierszy helpa
*/
#define MAXCOLS    2048			/* Liczba kolumn helpa (dla
bezpieczenstwa) */
#define STRING	   256			/* Wielkosc bufora na zanaki */

#ifndef UNIXPATH			/* Dla Borland C/C++ (DJGPP GNU C/C++) */
#define SEPARATOR '\\'		        /* Separator elmentow */
#define ANTYSEPAR '/'
#else
#define SEPARATOR '/'
#define ANTYSEPAR '\\'
#endif

typedef struct {
   int count;
   char *napis,*opt;
 } HISTORY;

extern int prn;
static HISTORY *HistTable[MAXHISTORY];
static int CountHist=0;
char BatchPath[2*STRING];
extern char firsthelp[];

static char *makePathName(char *path,int n) {
  int i,j,m,k=0,index=0;
  
  for(i=0 ; BatchPath[i]!='\0' ; i++)
    if(BatchPath[i]==':' || BatchPath[i]==';') {
      if(k==n) {
	for(j=index,m=0 ; j<i ; j++,m++)
	  path[m]=BatchPath[j];
	path[m]='\0';

	if(path[m-1]!=SEPARATOR) {
	  path[m]=SEPARATOR;
	  path[m+1]='\0';
	}

	if(prn==1) 
	  fprintf(stdout,"search starts here: %s\n",path);
	return path;
      } else { 
	index=i+1;
	k++;
      }
    }
  return NULL;
}

void SetBatchPath(char *path)  /* Ustawienie scizki dla polecen zewnetrznych */
 {
   char buffor[STRING];
   int len;
   
   sprintf(buffor,"%s",path);
   if((len=strlen(buffor))!=0) {
     strcpy(BatchPath,buffor);
     if(BatchPath[len-1]!=';' || BatchPath[len-1]!=':') {
       BatchPath[len]=';';
       BatchPath[len+1]='\0';
     }
   }
   
   if(prn==1)
     fprintf(stdout,"PATH=%s\n",BatchPath);
 }          

void addPath(char *path) {
  char buffor[STRING];
   int len;
   
   sprintf(buffor,"%s",path);
   if((len=strlen(buffor))!=0) {
     strcat(BatchPath,buffor);
     if(BatchPath[len-1]!=';' || BatchPath[len-1]!=':') {
       BatchPath[len]=';';
       BatchPath[len+1]='\0';
     }
   }
   
   if(prn==1)
     fprintf(stdout,"PATH=%s\n",BatchPath);
}

static void AppendHistory(int licznik,char *napis,char *opt)
 {
   int i;					/* Dodanie nowego elementu historii */

   if(CountHist<MAXHISTORY)
     {
       if((HistTable[CountHist]=(HISTORY *)malloc(sizeof(HISTORY)))==NULL)
	 {
	   fputs("\n\rOut of memory in AppendHistory !\n\r",stderr);
	   exit(EXIT_FAILURE);
	 }
       HistTable[CountHist]->count=licznik;
       if((HistTable[CountHist]->napis=(char *)malloc((unsigned)(strlen(napis)+1)
						*sizeof(char)))==NULL ||
	  (HistTable[CountHist]->opt=(char *)malloc((unsigned)(strlen(opt)+1)
						*sizeof(char)))==NULL)
	{
	  fputs("\n\rBrak pamieci w AppendHistory (2)!\n\r",stderr);
	  exit(EXIT_FAILURE);
	}

      strcpy(HistTable[CountHist]->napis,napis);
      strcpy(HistTable[CountHist]->opt,opt);
      CountHist++;
    }
  else
   {
     free((void *)HistTable[0]->napis);
     free((void *)HistTable[0]->opt);
     free((void *)HistTable[0]);

     for(i=0 ; i<MAXHISTORY-1; i++) HistTable[i]=HistTable[i+1];

     if((HistTable[MAXHISTORY-1]=(HISTORY *)malloc(sizeof(HISTORY)))==NULL)
	 {
	   fputs("\n\rOut of memory in AppendHistory (3)!\n\r",stderr);
	   exit(EXIT_FAILURE);
	 }
       HistTable[MAXHISTORY-1]->count=licznik;
       if((HistTable[MAXHISTORY-1]->napis=(char *)malloc((unsigned)(strlen(napis)+1)*
						  sizeof(char)))==NULL
	  || (HistTable[MAXHISTORY-1]->opt=(char *)malloc((unsigned)(strlen(opt)+1)*
						   sizeof(char)))==NULL)
	{
	  fputs("\n\rOut of memory in AppendHistory (4)!\n\r",stderr);
	  exit(EXIT_FAILURE);
	}
      strcpy(HistTable[MAXHISTORY-1]->napis,napis);
      strcpy(HistTable[MAXHISTORY-1]->opt,opt);
   }
}

static void DeleteHistory(void)			/* Zwolniene pamieci po historii */
 {
   int i;

   for(i=0 ; i<CountHist ; i++)
    {
      free((void *)(HistTable[i]->napis));
      free((void *)(HistTable[i]->opt));
      free((void *)HistTable[i]);
    }
 }

#ifndef MYNOPOSIX				/* Dla systemu nie uwzglednajacego */
						/* standart POSIX dla obslugi katalogow */
static void Split(char *filepath, char *pathname, char *fname)
{
   char ch,*ptr;				/* Podzial calkowitej nazwy pliku */

   for(ptr=filepath; *ptr; ptr++)		/* Konwerja na wlasciwy */
    if(*ptr==ANTYSEPAR) *ptr=SEPARATOR;         /* separotor */

   ptr=strrchr(filepath,SEPARATOR);
   if(!ptr)
   {
      ptr=filepath;				 /* obviously, no path   */
      *pathname='\0';
   }
   else
    {
      ++ptr;					 /* skip the delimiter   */
      ch=*ptr;
      *ptr='\0';
      strcpy(pathname, filepath);
      *ptr=ch;
   }
   strcpy(fname,ptr);
}

/*
   Author: J. Kercheval				(interpretacja wyrazen reg)
   Created: Thu, 03/14/1991  22:24:34
*/

#ifndef BOOLEAN
#define BOOLEAN int
#define TRUE 1
#define FALSE 0
#endif

#define MATCH_PATTERN  6    /* bad pattern */
#define MATCH_LITERAL  5    /* match failure on literal match */
#define MATCH_RANGE    4    /* match failure on [..] construct */
#define MATCH_ABORT    3    /* premature end of text string */
#define MATCH_END      2    /* premature end of pattern string */
#define MATCH_VALID    1    /* valid match */
#define PATTERN_VALID  0    /* valid pattern */
#define PATTERN_ESC   -1    /* literal escape at end of pattern */
#define PATTERN_RANGE -2    /* malformed range in [..] construct */
#define PATTERN_CLOSE -3    /* no end bracket in [..] construct */
#define PATTERN_EMPTY -4    /* [..] contstruct is empty */

static BOOLEAN is_pattern(char *p)
{
   while ( *p ) {
      switch ( *p++ ) {
      case '?':
      case '*':
      case '[':
	 return TRUE;
      }
   }
   return FALSE;
}

static int matche(register char *, register char *);

static int matche_after_star(register char *p, register char *t)
{
   register int match = 0;
   register int nextp;

   while ( *p == '?' || *p == '*' ) {
      if ( *p == '?' ) {
	 if ( !*t++ ) {
	    return MATCH_ABORT;
	 }
      }
      p++;
   }
   if ( !*p ) {
      return MATCH_VALID;
   }
   nextp = *p;
   do {
      if ( nextp == *t || nextp == '[' ) {
	 match=matche(p, t);
      }
      if ( !*t++ ) match = MATCH_ABORT;
   }
   while ( match != MATCH_VALID &&
      match != MATCH_ABORT &&
      match != MATCH_PATTERN);
   return match;
}

static BOOLEAN is_valid_pattern(char *p, int *error_type)
{
   *error_type=PATTERN_VALID;

   while( *p ) {
      switch( *p ) {
      case '[':
	 p++;
	 if ( *p == ']' ) {
	    *error_type = PATTERN_EMPTY;
	    return FALSE;
	 }
	 if ( !*p ) {
	    *error_type = PATTERN_CLOSE;
	    return FALSE;
	 }
	 while( *p != ']' ) {
	    if( *p == '\\' ) {
	       p++;
	       if ( !*p++ ) {
		  *error_type = PATTERN_ESC;
		  return FALSE;
	       }
	    }
	    else
	       p++;
	    if ( !*p ) {
	       *error_type = PATTERN_CLOSE;
	       return FALSE;
	    }
	    if( *p == '-' ) {
	       if ( !*++p || *p == ']' ) {
		  *error_type = PATTERN_RANGE;
		  return FALSE;
	       }
	       else {
		  if( *p == '\\' )
		     p++;
		  if ( !*p++ ) {
		     *error_type = PATTERN_ESC;
		     return FALSE;
		  }
	       }
	    }
	 }
	 break;
      case '*':
      case '?':
      default:
	 p++;
	 break;
      }
   }
   return TRUE;
}

static int matche(register char *p, register char *t)
{
   register char range_start, range_end;

   BOOLEAN invert;
   BOOLEAN member_match;
   BOOLEAN loop;

   for ( ; *p; p++, t++ ) {
      if (!*t) {
	 return ( *p == '*' && *++p == '\0' ) ? MATCH_VALID : MATCH_ABORT;
      }
      switch ( *p ) {
      case '?':
	 break;
      case '*':
	 return matche_after_star (p, t);
      case '[':
	 {
	    p++;
	    invert = FALSE;
	    if ( *p == '!' || *p == '^') {
	       invert = TRUE;
	       p++;
	    }
	    if ( *p == ']' ) {
	       return MATCH_PATTERN;
	    }

	    member_match = FALSE;
	    loop = TRUE;
	    while ( loop ) {
	       if (*p == ']') {
		  loop = FALSE;
		  continue;
	       }
	       if ( *p == '\\' ) {
		  range_start = range_end = *++p;
	       }
	       else {
		  range_start = range_end = *p;
	       }
	       if (!*p)
		  return MATCH_PATTERN;
	       if (*++p == '-') {
		  range_end = *++p;
		  if (range_end == '\0' || range_end == ']')
		     return MATCH_PATTERN;
		  if (range_end == '\\') {
		     range_end = *++p;
		     if (!range_end)
			return MATCH_PATTERN;
		  }
		  p++;
	       }
	       if ( range_start < range_end ) {
		  if (*t >= range_start && *t <= range_end) {
		     member_match = TRUE;
		     loop = FALSE;
		  }
	       }
	       else {
		  if (*t >= range_end && *t <= range_start) {
		     member_match = TRUE;
		     loop = FALSE;
		  }
	       }
	    }

	    if ((invert && member_match) ||
	       !(invert || member_match))
	       return MATCH_RANGE;
	    if (member_match) {
	       while (*p != ']') {
		  if (!*p)
		     return MATCH_PATTERN;
		  if (*p == '\\') {
		     p++;
		     if (!*p)
			return MATCH_PATTERN;
		  }
		  p++;
	       }
	    }
	    break;
	 }
      default:
	 if (*p != *t)
	    return MATCH_LITERAL;
      }
   }

   if ( *t )
      return MATCH_END;
   else
      return MATCH_VALID;
}

static BOOLEAN match( char *p, char *t )
{
   int error_type;

   error_type=matche(p,t);
   return (error_type==MATCH_VALID ) ? TRUE : FALSE;
}

static int xstrcmp(char *mask,char *text)
 {
   if(!is_pattern(mask))
     return ((strcmp(mask,text)==0) ?  1 : 0);

   return match(mask,text);
 }

static int SprMatch(char *mask)
 {
   int error;

   (void)is_valid_pattern(mask,&error);
   switch(error) {
      case PATTERN_VALID:
	 break;
      case PATTERN_RANGE:
	 printf("    No End of Range in [..] Construct\n");
	 return -1;
      case PATTERN_CLOSE:
	 printf("    [..] Construct is Open\n");
	 return -1;
      case PATTERN_EMPTY:
	 printf("    [..] Construct is Empty\n");
	 return -1;
      default:
	 printf("    Internal Error in is_valid_pattern()\n");
	 return -1;
     }
  return 0;
 }

static char *PrintLong(unsigned long number,char *nowy)
 {
   char string[STRING];  				/* Zapis numeru z kropkami co 1000 */
   int i,n,j=0;

   sprintf(string,"%lu",number);
   n=strlen(string);
   for(i=0 ; i<n ; i++)
    if(((n-i-1)%3)==0)
      {
	nowy[j++]=string[i];
	nowy[j++]='.';
      }
    else nowy[j++]=string[i];
    nowy[j-1]='\0';
    return nowy;
 }

static void ls(char *path)			/* Wyswietlenie katalogu */
{
  DIR *dir;
  struct dirent *ent;
  struct stat statbuf;
  static char dirname[MAXCMD],filename[MAXCMD],sizename[STRING],mask[MAXCMD];
  unsigned long size=0UL;
  int i=0;

  Split(path,dirname,mask);			/* Rozdzielenie pliku na */
  if(*dirname=='\0')                            /* katalog i nazwe (maska) */
    strcpy(dirname,".");			/* Domyslne katalog aktualny */

  if(*mask=='\0')
    strcpy(mask,"*");				/* wszystkie pliki z katologu */

  if(SprMatch(mask)==-1)
    return;

  if((dir=opendir(dirname))==NULL)
   {
     perror("Unable to open directory");
     return;
   }

  while((ent=readdir(dir))!=NULL)
    if(strcmp(ent->d_name,".")!=0 && strcmp(ent->d_name,"..")!=0)
     if(xstrcmp(mask,ent->d_name)==1)            /* Sprawdzamy maske pliku */
	{
	  filename[0]='\0';
	  sprintf(filename,"%s%c%s",dirname,SEPARATOR,ent->d_name);
	  if(stat(filename,&statbuf)!=0)
           {
	     fprintf(stderr,"Problems with function stat for %s !\n",filename);
	     return;
	   }

	  if(!(statbuf.st_mode & S_IFDIR))    /* gdy nie jest podkatologiem */
	   {
	     fprintf(stdout,"%-20s %15s B    %s",ent->d_name,
	             PrintLong(statbuf.st_size,sizename),
		     asctime(localtime((time_t const *)&statbuf.st_mtime)));
	     size+=(unsigned long)statbuf.st_size;
	     i++;
 	   }
         else if(statbuf.st_mode & S_IFDIR)
		{
		   fprintf(stdout,"%-20s %15s      %s",ent->d_name,"<DIR>",
			   asctime(localtime((time_t const *)&statbuf.st_mtime)));
		   i++;
		}

         if(((i+1)&31)==0) pause();
     }
  fprintf(stdout,"\t%3d file(s) (%sB)\n",i,PrintLong(size,sizename));
  (void)closedir(dir);
}

#else

static void ls(char *opt)
 {
   fprintf(stderr,"<<<< UNAVAILABLE OPTION (NOPOSIX) %s >>>\n",opt);
   return;		/* Ta funkcja nic nie robi */
 }

#endif

void PrintAllHelp(COMMAND commands[],char *help[],int n) {
  static char *TextPage[MAXROWS],buffor[MAXCOLS],napis[MAXCOLS];
  int k=0,i,j,l;
 
  for(i=0 ; i<n ; i++) {
    const int index=permut[i].index;
    
    sprintf(buffor,"\n%-10s - ",commands[index].command);
    strcat(buffor,help[index]);
    l=j=0;
    while(buffor[j]!='\0') {
      if(buffor[j]=='\n') {
	napis[l]='\0';
	if((TextPage[k]=(char *)malloc((unsigned)(strlen(napis)+1)*sizeof(char)))==NULL) {
	  fputs("Not enough memory to print Help\n\r",stderr);
	  exit(EXIT_FAILURE);
	}
	strcpy(TextPage[k],napis);
	k++; l=0;
      } else {
	napis[l]=buffor[j];
	l++; 
      }
      j++;
    }
  }
  
  for(i=0 ; i<k ; i++) {
    fprintf(stdout,"%s\n",TextPage[i]);
    if(((i+1)%23)==0) pause();
  }
  
  for(i=0 ; i<k ; i++)
    free((void *)TextPage[i]);
}

static void Replace(char *Env[],char *string,char *out)
{
  register int i,NumEnv;       /* Zastepowanie nazw symbolicznych $1 .. $n */
  static char buffor[STRING];  /* na odpowiednie wartosci parametrow */

  if(Env==NULL)
   {
     strcpy(out,string);
     return;
   }

  for(i=0,out[0]='\0' ; string[i]!='\0' ; i++)
    if(string[i]=='$')
      {
	sscanf(string+i+1,"%s",buffor);
	NumEnv=atoi(buffor);
	if(NumEnv>=0 && NumEnv<MAXCMD && Env[NumEnv]!=NULL)
          strcat(out,Env[NumEnv]);
	i+=strlen(buffor);
      }
    else 
      strncat(out,string+i,1);
}

int Batch(COMMAND commands[],char *filename,char *argv[])
 {
   FILE *plik;
   char *cmdline,*comand,*opt;
   int i,k=0,ok,n=0,count=0;

   if((cmdline=(char *)malloc(MAXCMD*sizeof(char)))==NULL)
     return -1;

   if((comand=(char *)malloc(MAXCMD*sizeof(char)))==NULL) {
     free((void *)cmdline);
     return -1;
   }

   if((opt=(char *)malloc(MAXCMD*sizeof(char)))==NULL) {
     free((void *)cmdline); free((void *)comand);
     return -1;
   }

   if((plik=fopen(filename,"rt"))==NULL)
      return -1;
      
   if(prn==1)
     fprintf(stdout,"\t\t\t<<< START OF SCRIPT %s >>>\n",filename);

   while(commands[n].command!=NULL) n++; /* Liczba dostepnych opcji */
   while(!feof(plik))
    {
      comand[0]='\0'; opt[0]='\0';
      fscanf(plik,"%[^\n]\n",cmdline);
      ok=i=0;
      while(cmdline[i] && isspace(cmdline[i])) i++;

      if(cmdline[i]=='#')				/* Komentarz */
	continue;

      sscanf(cmdline+i,"%s",comand);
      i+=strlen(comand);
      while(cmdline[i] && isspace(cmdline[i])) i++;
      sscanf(cmdline+i,"%[^\n]\n",opt);
     
      if(strcmp(comand,"end")==0) 			/* Zakonczenie skryptu */
       {
         if(prn==1)
           fprintf(stdout,"[%d] BATCH(%s)>end\n",count,filename);
	 break;
       }

      if(strcmp(comand,"batch")==0)
       if(strcmp(opt,filename)!=0)			/* Zapobiega rekurencji (prymitywnie) */
	{
	  char **newargv,*buffor;
	  int argc,i;

	  if((newargv=(char **)malloc(MAXCMD*sizeof(char *)))==NULL)
	    continue;

	  if((buffor=(char *)malloc(MAXCMD*sizeof(char)))==NULL) {
	    free((void *)newargv);
	    continue;
	  }
	  
	  if(prn==1)
            fprintf(stdout,"[%d] BATCH(%s)>batch %s\n",count,filename,opt);
	  
	  for(i=0 ; i<MAXCMD ; i++)
	    newargv[i]=NULL;

	  Replace(argv,opt,buffor);                     /* Rozwniecie parametrow */
	  StrToArgv(buffor,newargv,&argc);
	  if(Batch(commands,newargv[0],newargv)==-1)
            fprintf(stderr,"File %s is missing !\n",opt);
	  FreeArgv(newargv,argc); free((void *)buffor); free((void *)newargv);
	  count++;
	  continue;
	}
      
      if(strcmp(comand,"sys")==0)
        {
	  if(prn==1)
             fprintf(stdout,"[%d] BATCH(%s)>!%s\n",count,filename,opt);
          if(system(opt)==-1)
            fprintf(stderr,"Shell command not executed properly !\n");
          count++;
          continue;
        }

      for(i=0 ; i<n ; i++)				/* Nieznane komedy sa ignorowane */
       if(strcmp(commands[i].command,comand)==0)
	 {
	    char buffor[STRING];

	    ok=1;
	    Replace(argv,opt,buffor);                   /* Rozwiniecie parametrow */
	    if(prn==1)
               fprintf(stdout,"[%d] BATCH(%s)> %s %s\n",count,filename,comand,buffor);
	    (*commands[i].func)(buffor);
	    count++;
	    break;
	 }
         
      if(ok!=1) 
        {
          char name[STRING],*newargv[MAXCMD],buffor[STRING],buff[STRING];
          int i,argc;
          
	  for(i=0 ; i<MAXCMD ; i++)
	    newargv[i]=NULL;

	  Replace(argv,opt,buff);                       /* Rozwiniecie parametrow */
	  sprintf(buffor,"%s %s",comand,buff);
	  StrToArgv(buffor,newargv,&argc);
	  if(Batch(commands,newargv[0],newargv)==-1) {	/* W biezacym katalogu */
	    int index=0,ok=0;
	    for( ; ; index++) {
	      if(makePathName(name,index)==NULL)
		break;

	      strcat(name,newargv[0]);
	      if(Batch(commands,name,newargv)!=-1) {
		ok=1;
		break;
	      }
	      if(ok==0) k++;
	    }
	  }
	  FreeArgv(newargv,argc);
	}
    }
    
  fclose(plik);
  free((void *)cmdline); free((void *)comand); free((void *)opt);
  if(prn==1)
    fprintf(stdout,"\n%4d ignored command(s)\n\t\t\t<<< END OF SCRIPT %s >>>\n",
	    k,filename);
  return 0;  
}

void Shell(COMMAND commands[],char *help[])
{
  char cmdline[STRING],argv[MAXCMD],argp[MAXCMD];
  int i,opcja,n=0,j,ok,len,count=0,itmp;

  while(commands[n].command!=NULL) n++;
  for( ; ; )
      {					/* Interactive mode */
	fprintf(stdout,"[%d]MP> ",count); fflush(stdout);
	argv[0]=argp[0]=0; cmdline[0]=0;
	if(!fgets(cmdline,sizeof(cmdline)-1,stdin))
	 {
	   fprintf(stderr,"Read error ! [%s]\n",cmdline);
	   return;
	 }

	i=0;
	while(cmdline[i] && isspace(cmdline[i])) i++;
	(void)sscanf(cmdline+i,"%s",argv);

	len=strlen(argv);
	if(len==0) continue;
	i+=len;
	while(cmdline[i] && isspace(cmdline[i])) i++;
	(void)sscanf(cmdline+i,"%[^\n]\n",argp);

	AppendHistory(count,argv,argp); count++;

	if(strcmp(argv,"history")==0)
	  {
	   if(strlen(argp)>0)
	    {
	      itmp=atoi(argp);
	      ok=0;
	      for(i=0 ; i<CountHist ; i++)
	       if(HistTable[i]->count==itmp)
		 {
		    ok=1;
		    strcpy(argv,HistTable[i]->napis);
		    strcpy(argp,HistTable[i]->opt);
		    fprintf(stdout,"[%d] (HISTORY [%d]) MP> %s %s\n",
				    count-1,itmp,argv,argp);
		    break;
		 }

	       if(!ok)
		 {
		   fprintf(stderr,"No such number\n");
		   continue;
		 }
	      }
	   else
	    {
	      fprintf(stdout,"\t\t<<< HISTORY >>>\n");
	      for(i=0 ; i<CountHist ; i++)
	       {
		  fprintf(stdout,"[%d] %s %s\n",HistTable[i]->count,
					      HistTable[i]->napis,
					      HistTable[i]->opt);
		  if(((i+1)%20)==0) pause();
	       }
	      fprintf(stdout,"\n");
	      continue;
	    }
	  }

	if(strcmp(argv,"man")==0) {
	  fprintf(stdout,"\n\t\t<<< BUILT-IN COMMANDS >>\n"
		         "exit        \t\t- leave shell\n"
		         "ls          \t\t- current directory listing\n"
		         "history [nr]\t\t- display entered commands or execute [nr] if given\n"
		         "batch       \t\t- run commands from file\n"
		         "!comd       \t\t- run a shell command comd\n"
		         "man         \t\t- display all available help\n"
		         "sys comd    \t\t- run shell command comd in script\n");

	  fprintf(stdout,"<<< NEXT - ENTER q - QUIT >>> ");
	  if(getchar()!='q') {
	    if(help!=NULL)
	      PrintAllHelp(commands,help,n); 
	  } else (void)getchar();
	  continue;
	}

	if(strcmp(argv,"help")==0 || strcmp(argv,"?")==0 || strcmp(argv,"h")==0)
	    {
	      ok=0;
	      if(strlen(argp)>0)
	       {
		 if(strcmp(argp,"help")==0 || strcmp(argp,"?")==0)
		  {
		    ok=1;
		    fprintf(stderr,"help [arg] or ? [arg] -- info about the arg command\n");
		  } 
                 else if(strcmp(argp,"batch")==0)
                     {
                       ok=1;
		       fprintf(stderr,"batch file -- run script file\n");
		     }	
		 else if(strcmp(argp,"sys")==0)
			 {
			   ok=1;
			   fprintf(stderr,"sys comd -- run shell command comd (from a script only)\n");
			 }
		 else if(strcmp(argp,"ls")==0)
			 {
			   ok=1;
			   fprintf(stderr,"ls adir -- listing of directory adir, regexp allowed\n"); 
			 }
		 else if(strcmp(argp,"history")==0)
			{
			  ok=1;
			  fprintf(stderr,"history [nr] -- list entered commands\n"	
					 "\t\t or if [nr] given - run command No [nr]\n");
		        }
		 else for(j=0 ; j<n ; j++)
		        if(strcmp(commands[j].command,argp)==0)
		          {
		     	     ok=1;
		             fprintf(stdout,"%-10s - ",commands[j].command);
	   	             if(help!=NULL)
			        fprintf(stdout,"%s ",help[j]);
		             fprintf(stdout,"\n");
		             break;
		           }
		  if(!ok) 
		     fprintf(stdout,"Unknown command : %s\n",argp);
	       }
	       else
		 { fprintf(stdout,"%s\n",firsthelp); }
	       continue;
	    }

	if(strcmp(argv,"exit")==0) {
	  DeleteHistory();
	   if(prn==1)
             fprintf(stdout,"<<< LEAVING MP SHELL >>>\n");
	   Quit("");
	   break;
	}

	if(strcmp(argv,"ls")==0)
	 {
	   ls(argp);
	   continue;
	 }

	if(strcmp(argv,"batch")==0)
	  {
	    char *newargv[MAXCMD];
	    int i,argc;

	    for(i=0 ; i<MAXCMD ; i++)
	      newargv[i]=NULL;

	    StrToArgv(argp,newargv,&argc);
	    if(Batch(commands,newargv[0],newargv)==-1)
              fprintf(stderr,"Missing file %s !\n",argp);
	    FreeArgv(newargv,argc);
	    continue;
	  }

	if(strcmp(argv,"!")==0)
	  {
	    if(system(argp)==-1)
	      fprintf(stderr,"Error executing system command !\n");
	    continue;
	  }

	if(argv[0]=='!')
	  {
	    argv[0]=' ';
	    (void)strcat(argv," "); (void)strcat(argv,argp);
	    if(system(argv)==-1)
	      fprintf(stderr,"Error executing system command !\n");
	    continue;
	  }

	opcja=-1;				/* Opcje zewnetrzne */
	for(i=0 ; i<n ; i++)
	 if(strcmp(commands[i].command,argv)==0)
	    {
	      opcja=i;
	      (*commands[i].func)(argp);
	      break;
	    }

	if(opcja==-1)
         {
          char name[STRING],*newargv[MAXCMD],buffor[STRING];
          int argc,i;
          
	  for(i=0 ; i<MAXCMD ; i++)
	    newargv[i]=NULL;

	  sprintf(buffor,"%s %s",argv,argp);
	  StrToArgv(buffor,newargv,&argc);
                  
          if(Batch(commands,newargv[0],newargv)==-1) {	 /* Polecenia w biezacym katologu */
	    int index=0,ok=0;
	    for( ; ; index++) {
	      if(makePathName(name,index)==NULL)
		break;

	      strcat(name,newargv[0]);
	      if(Batch(commands,name,newargv)!=-1) {
		ok=1;
		break;
	      }
	    }
	    if(ok==0) 
	      fprintf(stderr,"MP: Unknown command %s or bad script path %s\n",
		      argv,BatchPath);
	  }

	  FreeArgv(newargv,argc);
	 }    
      }
}

void StrToArgv(char string[],char *argv[],int *argc)
 {
   char buf[MAXARGSIZE];
   int i=0,m;                        /* Konwersja napisu na liste argumetow */

   *argc=0;
   for(;;)
    {
       while(isspace(string[i]) && string[i]) i++;		/* Biale znaki */
       if(sscanf(string+i,"%s",buf)<1) break;                   /* koniec */
       m=strlen(buf);
       if((argv[*argc]=(char *)malloc((unsigned)(m+1)*sizeof(char)))==NULL)
	 {
	   fprintf(stderr,"Out of memory in StrToArgv !\n");
	   exit(EXIT_FAILURE);
	 }
       strcpy(argv[*argc],buf);
       (*argc)++;
       i+=m;
    }
 }

void FreeArgv(char *argv[],int argc)     /* Zwolnienie pamieci z argumetami */
 {
   int i;

   for(i=0 ; i<argc ; i++)
     free((void *)argv[i]);
 }

extern int javaMode;

#ifndef MULTITHREAD
#include <signal.h>
#endif

void Quit(char *opt) {
    if(javaMode) {
	fprintf(stdout,"<<< LEAVING JAVA >>>\n");
#ifndef __MSDOS__
#ifndef MULTITHREAD
	if(signal(SIGCLD,SIG_IGN)==SIG_ERR) {
	    fprintf(stderr,"Problems trapping signal SIGCLD !\n");
	}
#endif
#endif
    } else {
	if(prn==1)
	    fprintf(stdout,"<<< LEAVING %s >>>\n",opt);
    }
    exit(EXIT_SUCCESS);
}
