		/* Generatory liczb losowych 1997 01 10; 1997 10 04 */

#ifndef RAND2D_H
#define RAND2D_H

#define SRAND(SEED) r250_init(SEED)
#define DRND()      dr250()
#define RANDOM(N)   r250n(N)

void r250_init(unsigned short);	   /* Inicjacja generatora liczb losowych */
unsigned int r250n(unsigned);      /* Calkowita liczba losowa */
double dr250(void);                /* Liczba losowa z przedzialu [0,1) */
int InicGen2D(float (*)(int,int),int,int,int);  /* Inicjacja generatora rozkladu 2D */
void CloseRand2D(void);		   /* Zamkniecie generator 2D */
int Rand2D(int *,int *);           /* Generacja pary liczb z rozkladu 2D */
int InicRand1D(float *,int);       /* Inicjacja generatora liczb losowych 1D */
void CloseRand1D(void);		   /* Zamkniecie genratora 1D */
int Rand1D(int *);		   /* Generacja liczby losowej */

#endif
