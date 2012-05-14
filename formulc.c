/*
   A fast interpreter of mathematical functions
   Copyright (c) 1994 by Harald Helfgott
   Programmer's Address:
   Harald Helfgott
   MB 1807, Brandeis University
   P.O. Box 9110
   Waltham, MA 02254-9110       OR
   (during the summer)
   2606 Willett Apt. 427
   Laramie, Wyoming 82070
   seiere@uwyo.edu
   Adaptacja na potrzeby programu hmpp. 1997 01 10 (poprawka na cc 1997 01 13)
   Generacja rozkladu p-stwa polozenia atomow w slownikach calkowicie 
   zrandominizowanych.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "proto.h"

#define TABLESIZE    256
#define BUFSIZE      2048
#define MAXPAR 	     3

typedef char CHAR;

static double pi(void);
static double value(UCHAR *function);
static CHAR *i_error;
static UCHAR *i_trans(UCHAR *,CHAR *,CHAR *);
static CHAR  *my_strtok(CHAR *);
static UCHAR *comp_time(UCHAR *,UCHAR *,int);

static double param['z'-'a'+1];
struct formu_item {
 CHAR *name;
 void *f;
 int n_pars;
};
						
static double rect(double a,double b,double x) /* dodatkowe funkcji logiczne */
 {
   double dtmp;
   
   if(a>b)			/* Przynaleznosc do przedzialu */
    {
      dtmp=a; a=b; b=dtmp;
    }

   return ((x>=a && x<=b) ? 1.0 : 0.0);
 }

static double and(double a,double b)
 {
   return ((a && b) ? 1.0 : 0.0);
 }

static double or(double a,double b)
 {
   return ((a || b) ? 1.0 : 0.0);
 }

static double not(double a)
 {
   return ((!a) ? 1.0 : 0.0);
 }
					
static double eq(double a,double b)	/* Operatory logiczne jak w FORTRANIE */
 {
   return ((a==b) ? 1.0 : 0.0);
 }

static double lt(double a,double b)
 {
   return ((a<b) ? 1.0 : 0.0);
 }

static double le(double a,double b)
 {
   return ((a>=b) ? 1.0 : 0.0);
 }

static double ne(double a,double b)
 {
   return ((a!=b) ? 1.0 : 0.0);
 }

static double ge(double a,double b)
 {
   return ((a>=b) ? 1.0 : 0.0);
 }

static double gt(double a,double b)
 {
   return ((a>b) ? 1.0 : 0.0);
 }

static double sqr(double x)
 {
   return x*x;
 }

static struct formu_item ftable[TABLESIZE]=
       {{"exp",(void *)exp,1},
	{"ln", (void *)log,1},
	{"sin",(void *)sin,1},
	{"cos",(void *)cos,1},
	{"tan",(void *)tan,1},
	{"asin",(void *)asin,1},
	{"acos",(void *)acos,1},
	{"atan",(void *)atan,1},
	{"atan2",(void *)atan2,2},
	{"abs", (void *)fabs,1},
	{"sqrt", (void *)sqrt,1},
	{"pi", (void *)pi,0},
        {"rect",(void *)rect,3},
        {"and",(void *)and,2},
        {"or",(void *)or,2},
        {"not",(void *)not,1}, 
        {"eq",(void *)eq,2},
        {"lt",(void *)lt,2},
        {"le",(void *)le,2},
        {"ne",(void *)ne,2},
        {"ge",(void *)ge,2},
        {"gt",(void *)gt,2},
        {"sqr",(void *)sqr,1},
	{NULL,NULL,0}};

char FunctionCode[BUFSIZE];		/* Postac funkcji do kompilacji */
					/* Zmienna globalna */
static int where_table(CHAR *name)
{
 struct formu_item *table_p;

 for(table_p=ftable; table_p->f!=NULL &&
     strcmp(name,table_p->name); table_p++)
   ;
 if(table_p->f==NULL)
   return -1;
 else return table_p-ftable;
}

static double value(register UCHAR *function)
{
 double buffer[BUFSIZE];
 register double *bufp = buffer,*ptrtmp;
 double x,y,z;
 register double result;

 if(!function)
   return 0;
 for(;;) {
   switch(*function++) {
    case '\0':goto finish;
    case 'D':
	      ptrtmp=(double *)function;	/* Poprawka */
	      *bufp++=*ptrtmp++;
	      function=(UCHAR *)ptrtmp;
	      break;
    case 'V': *bufp++=param[(*function++)-'a'];
	      break;
    case 'M':
	     result=-(*--bufp);
	     *bufp++=result;
	     break;
    case '+':
	     y=*(--bufp);
	     result=y+*(--bufp);
	     *bufp++=result;
	  break;
    case '-':
	     y=*--bufp;
	     result=*(--bufp)-y;
	     *bufp++=result;
	     break;
    case '*':
	     y=*(--bufp);
	     result=*(--bufp)*y;
	     *bufp++=result;
	     break;
    case '/':
	     y=*--bufp;
	     result=*(--bufp)/y;
	     *bufp++=result;
	     break;
    case '^':
	     y=*--bufp;
	     result=pow(*(--bufp),y);
	     *bufp++=result;
	     break;
    case 'F':
	     switch(ftable[*function].n_pars) {
	       case 0:*bufp++=(*((double (*)(void))
			       ftable[*function++].f))();
		      break;
	       case 1:x=*--bufp;
		      *bufp++=(*((double (*)(double))
			       ftable[*function++].f))(x);
		      break;
	       case 2:y=*--bufp;
		   x=*--bufp;
		   *bufp++=(*((double (*)(double,double))
			    ftable[*function++].f))(x,y);
		      break;
	       case 3:z=*--bufp;
		      y=*--bufp;
		      x=*--bufp;
		      *bufp++=(*((double (*)(double, double, double))
			       ftable[*function++].f))(x,y,z);
		      break;
	       default:
		       fprintf(stderr,"Bug! too many parameters\n");
		       return 0;
	      }
	     break;
    default:
	 fputs("Bug! Unrecognizable operator",stderr);
	 return 0;
   }
 }
 finish: if((bufp-buffer)!=1)
	  {
	   fprintf(stderr,"\nBug! Too many things in the buffer");
	   fprintf(stderr,"Buffer: ");
	   while(bufp-- > buffer)
	    fprintf(stderr,"%g ",*bufp);
	   fprintf(stderr,"\n");
	  }
	 return buffer[0];
}

static int isoper(CHAR c)
{
  return ((c == '+') || (c == '-') || (c == '*') || (c == '/')
		     || (c == '^'));
}

static int is_code_oper(UCHAR c)
{
 return ((c == '+') || (c == '-') || (c == '*') || (c == '/')
		    || (c == '^') || (c == 'M'));
}

static int isin_real(CHAR c)    /* + and - are not included */
{
 return (isdigit(c) || c=='.' || c=='E');
}

static size_t max_size(CHAR *source)
{
 int numbers=0;
 int functions=0;
 int operators=0;
 int variables=0;
 const size_t var_size=2*sizeof(UCHAR);
 const size_t num_size=sizeof(UCHAR)+sizeof(double);
 const size_t op_size=sizeof(UCHAR);
 const size_t end_size=sizeof('\0');
 CHAR *scan;

 for(scan=source; *scan; scan++)
  if(isalpha(*scan) && (*scan != 'E'))
  {
   if(isalpha(*(scan+1)));
   else if(*(scan+1) == '(')
	  functions++;
	else
	  variables++;
  }

 if(isoper(*source)) operators++;
 if(*source != '\0')
  for(scan = source+1; *scan; scan++)
   if(isoper(*scan) && *(scan-1) != 'E') operators++;

 scan=source;
 while(*scan)
  if(isin_real(*scan) || ((*scan == '+' || *scan == '-') &&
			   scan>source && *(scan-1)=='E'))
   {
    numbers++;
    scan++;
    while(isin_real(*scan) || ((*scan == '+' || *scan == '-') &&
				scan>source && *(scan-1)=='E'))
     scan++;
   }
  else scan++;

 return(numbers*num_size+operators*op_size+functions*num_size
	+variables*var_size+end_size);
}

static UCHAR *translate(CHAR *source, CHAR *args, int *leng, int *error)
{
 UCHAR *result;
 CHAR *scan, *scarg;
 UCHAR *function;
 UCHAR *nfunc;
 size_t size_estim;

 i_error=NULL;
 for(scan=source; *scan!='\0'; scan++) {
  if(islower(*scan) && !isalpha(*(scan+1)) &&
      (scan==source || !isalpha(*(scan-1))) ) {
   for(scarg=args; *scarg != '\0' && *scarg != *scan; scarg++)
     ;
   if(*scarg=='\0')
    {
     i_error=scan;

     *leng=0;
     *error=i_error-source;
     return NULL;
    }
  }
 }

 size_estim=max_size(source);
 if(!(function=(UCHAR *)malloc(size_estim))) {
  *leng = 0;
  *error = -1;
  return NULL;
 }

 result=i_trans(function,source,source+strlen(source));
 if(!result) {
  free(function);
  *leng = 0;
  if(i_error)
   *error = i_error-source;
  else *error = -1;
  return NULL;
 }
 else {
  *result='\0';
  *error=-1;
  *leng=result-function;

  if(((*leng)+1) * sizeof(UCHAR) > size_estim)
   {
    fprintf(stderr,"Dangerous bug! The size estimate falls short by %lu bytes",
	   (unsigned long)(((*leng)+1) * sizeof(UCHAR) - size_estim));
    fprintf(stderr,"	Please, tell the author about this error immediately! "
		   "Don't forget to write down what mathematical function caused"
		   "the program to crash. This program's reliability depends on you!");
    exit(EXIT_FAILURE);
   }
  else if(((*leng)+1)*sizeof(UCHAR)<size_estim) {
   if((nfunc=(UCHAR *)malloc(((*leng)+1)*sizeof(UCHAR)))!=NULL) {
    (void)memcpy(nfunc,function,((*leng)+1)*sizeof(UCHAR) );
    free(function);
    function=nfunc;
   }
  return function;
  }
 }
 return 0;
}

static UCHAR *comp_time(UCHAR *function, UCHAR *fend, int npars)
{
  UCHAR *scan;
  int ok;
  UCHAR temp;
  double tempd;
  register double *ptrtmp;

  ok=1;
  scan=function;
  while(npars-- > 0)
  {
    if(*scan++ != 'D') ok=0;
    ptrtmp=(double *)scan;		/* Poprawka */
    ptrtmp++;
    scan=(UCHAR *)ptrtmp;
  }

  if(!ok ||
      !( ( scan == fend - (sizeof((UCHAR) 'F')+sizeof(UCHAR))
	   && *(fend-2) == 'F' ) ||
	 ( scan == fend - sizeof(UCHAR)
	   && is_code_oper(*(fend-1)) ) )
    )
   return fend;

  temp=*fend;
  *fend='\0';
  tempd=value(function);
  *fend=temp;
  *function++='D';
  ptrtmp=(double *)function;	/* Poprawka */
  *(ptrtmp)++=tempd;
  function=(UCHAR *)ptrtmp;

  return function;
}

static CHAR *my_strtok(CHAR *s)
{
 int pars;
 static CHAR *token=NULL;
 CHAR *next_token;

 if(s!=NULL) token=s;
 else if(token!=NULL) s=token;
 else return NULL;

 for(pars=0; *s != '\0' && (*s != ',' || pars!=0); s++) {
   if(*s == '(') ++pars;
   if(*s == ')') --pars;
 }
 if(*s=='\0') {
  next_token=NULL;
  s=token;

  token=next_token;
  return s;
 } else {
  *s = '\0';
  next_token=s+1;
  s=token;

  token=next_token;
  return s;
 }
}

#define TWO_OP {     		                       \
  if(((tempu=i_trans(function,begin,scan))!=0) &&      \
      ((temp3=i_trans(tempu,scan+1,end)))!=0 ) {       \
   *temp3++ = *scan;            		       \
   temp3=comp_time(function,temp3,2);  		       \
   return temp3;                   		       \
  } else return NULL;         			       \
 }

#define ERROR_MEM {				       \
		    i_error=NULL;     		       \
		    return NULL;       		       \
		  }

static UCHAR *i_trans(UCHAR *function, CHAR *begin, CHAR *end)
{
 int pars;
 CHAR *scan;
 UCHAR *tempu,*temp3=NULL;
 CHAR *temps=NULL;
 CHAR tempch;
 double tempd;
 CHAR *endf;
 int n_function;
 int space;
 int i;
 CHAR *paramstr[MAXPAR];
 CHAR *par_buf;
 register double *ptrtmp;

 if(begin>=end) {
  i_error = begin;
  return NULL;
 }

 for(pars=0, scan=begin; scan<end && pars>=0; scan++) {
  if(*scan == '(') pars++;
  else if(*scan == ')') pars--;
 }
 if(pars<0 || pars>0) {
  i_error = scan-1;
  return NULL;
 }

 for(pars=0, scan=end-1; scan>=begin; scan--) {
  if(*scan == '(') pars++;
  else if(*scan == ')') pars--;
  else if(!pars && (*scan == '+' || ((*scan == '-') && scan!=begin))
	     && (scan==begin || *(scan-1)!='E') )
   break;
 }

 if(scan >= begin) TWO_OP
 for(pars=0, scan=end-1; scan>=begin; scan--) {
  if(*scan == '(') pars++;
  else if(*scan == ')') pars--;
  else if(!pars && (*scan == '*' || *scan == '/' ))
   break;
 }

 if(scan >= begin) TWO_OP
 for(pars=0, scan=end-1; scan>=begin; scan--) {
  if(*scan == '(') pars++;
  else if(*scan == ')') pars--;
  else if(!pars && (*scan == '^'))
   break;
 }

 if(scan >= begin) TWO_OP
 if(*begin == '-') {
  if((tempu=i_trans(function,begin+1,end))!=0) {
   *tempu++ = 'M';
   tempu=comp_time(function,tempu,1);
   return tempu;
  } else return NULL;
 }

 while(isspace(*begin))
  begin++;
 while(isspace(*(end-1)))
  end--;

 if(*begin == '(' && *(end-1) == ')')
  return i_trans(function,begin+1,end-1);

 if(end == begin+1 && islower(*begin)) {
  *function++ = 'V';
  *function++ = *begin;
  return function;
 }

 tempch = *end;
 *end = '\0';
 tempd=strtod(begin,(CHAR**)&tempu);
 *end = tempch;
 if((CHAR*) tempu == end)
 {
  *function++ = 'D';
  ptrtmp=(double *)function;		/* Poprawka */
  *(ptrtmp)++ = tempd;
  function=(UCHAR *)ptrtmp;
  return function;
 }

 if(!isalpha(*begin) && *begin != '_')
 {
  i_error=begin;
  return NULL;
 }
 for(endf = begin+1; endf<end && (isalnum(*endf) || *endf=='_');
     endf++);
 tempch = *endf;
 *endf = '\0';
 if((n_function=where_table(begin)) == -1) {
  *endf = tempch;
  i_error=begin;
  return NULL;
 }
 *endf = tempch;
 if(*endf != '(' || *(end-1) != ')') {
  i_error=endf;
  return NULL;
 }
 if(ftable[n_function].n_pars==0) {
   space=1;
   for(scan=endf+1; scan<(end-1); scan++)
    if(!isspace(*scan)) space=0;
   if(space) {
    *function++ = 'F';
    *function++ = n_function;
    function = comp_time(function-2,function,0);
    return function;
   } else {
    i_error=endf+1;
    return NULL;
   }
 } else {
    tempch=*(end-1);
    *(end-1)='\0';
    par_buf=(CHAR *)malloc(strlen(endf+1)+1);
    if(!par_buf)
     ERROR_MEM;
    strcpy(par_buf, endf+1);
    *(end-1) = tempch;
    for(i=0; i<ftable[n_function].n_pars; i++) {
     if( ( temps=my_strtok((i==0) ? par_buf : NULL) ) == NULL )
      break;
     paramstr[i]=temps;
    }
    if(temps==NULL) {
     free(par_buf);
     i_error=end-2;
     return NULL;
    }
    if((temps=my_strtok(NULL))!=NULL) {
     free(par_buf);
     i_error=(temps-par_buf)+(endf+1);
     return NULL;
    }

    tempu=function;
    for(i=0; i<ftable[n_function].n_pars; i++)
     if(!(tempu=i_trans( tempu, paramstr[i],
				 paramstr[i]+strlen(paramstr[i]) ) ) )
     {
      i_error=(i_error-par_buf)+(endf+1);
      free(par_buf);
      return NULL;
     }
    free(par_buf);
    *tempu++ = 'F';
    *tempu++ = n_function;
    tempu = comp_time(function,tempu,ftable[n_function].n_pars);
    return tempu;
 }
}

static double pi(void)
 {
   return 3.14159265358979323846264;
 }

static UCHAR *ByteCode=NULL;		/* "Przekompilowany" kod funkcji */

int CompileFunction(void)       /* Kompilacja wyrazenia arytmetycznego */
 {
   int len,error;

   if(ByteCode!=NULL)
    {
      free((void *)ByteCode);
      ByteCode=NULL;
    }
   ByteCode=translate(FunctionCode,"ft",&len,&error);
   if(error!=-1)
     return -1;
   return 0;
 }

float fval(int x,int y)		 /* Generacja wartosci funkcji */
 {                               /* 2D rozkald p-stwa */
   const double Conv=((SamplingRate<=0.0) ? 1.0 :
		       SamplingRate/(double)DimBase);

   param['t'-'a']=(double)x; /* Przeliczenie na czestotliwosc w Hz */
   param['f'-'a']=Conv*(double)y;
   return (float)value(ByteCode);
 }


