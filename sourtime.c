    /* Inforamcja o dacie ostatniej modyfikacji kodu 1997 02 01
       Wszelkie zmiany w kodzie nalezy tu zaznaczac */

#include <stdio.h>

char ModDate[]="2000-01-10";

void SourceVersion(void)   /* Informacja o dacie ostatniej modyfikacji */
{
#ifndef  MULTITHREAD
/*   fprintf(stdout,"\n\n<<<<<< MP v. III  1996-1999   D. Ircha & P.J. Durka,  Warsaw University >>>>>>\n"
	*/
	fprintf(stdout,"\n\n**** mp v. III ** 1996-2000 ** D. Ircha & P.J. Durka ** Warsaw University ****\n"
		
#else 
		fprintf(stdout,"\n\n\t\t\t<<< MP v.III (multithread) 1996-2000 D. Ircha & P.J. Durka, Warsaw University >>>\n"
#endif
		"Matching Pursuit signal analysis with stochastic and dyadic Gabor dictionaries\n"
		"   Last (annotated:-) modification: %s, ", ModDate);
	   fprintf(stdout,"compiled: %s %s\n",__DATE__,__TIME__);     
#ifdef __VERSION__
	   fprintf(stdout,"   %s\n",__VERSION__);
#endif
	   fprintf(stdout,
	   /*		   "                            1996-1999 D. Ircha & P.J. Durka, Warsaw University\n"*/
	   		   "                        info & current versions at http://brain.fuw.edu.pl/~mp\n\n");
	   
}




