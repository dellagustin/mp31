      /* Zestaw generatorow liczb pseudolosowych 1997 01 10
	 Definicja NOMATHERR blokuje obsluge bledow matematycznych przy
	 generacji rozkladu p-stwa. */

#include <stdlib.h>
#include <time.h>
#include "rand2d.h"

extern float **MakeTable(int,int);		 /* Tworzenie tablic 2D */
extern void FreeTable(float **,int);
static int r250_index=0;             		 /* Zmienne pomocnicze generatora r250 */
static unsigned int r250_buffer[256];
static unsigned long seed=1UL;
static float **DystMatrix=NULL,*DystVector=NULL; /* Struktury pomocnicze */
static int GlobTimeSize=0,GlobFreqSize=0;        /* Rozmiar przestrzeni do losowania */
static unsigned short StartSeed=1;

/* Description: implements R250 random number generator, from S.
   Kirkpatrick and E.  Stoll, Journal of Computational Physics, 40, p. 517 (1981).
   Written by:    W. L. Maier. Adaptacja 1997 01 07 */

static unsigned myrand(void)
 {
   seed=seed*0x015a4e35UL+1UL;
   return (unsigned)(seed>>16) & 0x7fffU;
 }

unsigned short GetSeed(void)
 {
   return (unsigned short)StartSeed;
 }  

void r250_init(unsigned short newseed)
{
    unsigned int mask=0x8000U,msb=0xffffU;
    register int j,k;

    seed=(unsigned)(StartSeed=newseed);
    r250_index=0;
    for(j=0; j<250; j++)
	r250_buffer[j]=myrand();
    for(j=0; j<250; j++)
	if(myrand()>16384U)
	    r250_buffer[j]|=0x8000U;
    for(j=0; j<16; j++)
	{
	  k=11*j+3;
	  r250_buffer[k]&=mask;
	  r250_buffer[k]|=msb;
	  mask>>=1;
	  msb>>=1;
	}
}

static unsigned int r250(void)
{
    register int j;
    register unsigned int new_rand;

    if(r250_index>=147)
      j=r250_index-147;
    else
      j=r250_index+103;
    new_rand=r250_buffer[r250_index]^=r250_buffer[j];
    if(r250_index>=249)
      r250_index=0;
    else
      r250_index++;
    return new_rand;
}
  /* returns a random unsigned integer k
     uniformly distributed in the interval 0 <= k < n */

unsigned int r250n(unsigned n)
{
	 register int j;
	 register unsigned int new_rand,limit;

	 limit=(65535U/n)*n;
	 do
	  {
		 (void)r250();
		 if(r250_index>=147)
			j=r250_index-147;
		 else
			j=r250_index+103;

		 new_rand=r250_buffer[r250_index]^=r250_buffer[j];

	if(r250_index>=249)
	  r250_index=0;
	else
	  r250_index++;
	} while(new_rand>=limit);
	 return new_rand % n;
}

double dr250(void)         /* Return a number in [0.0 to 1.0) */
{
    register int    j;
    register unsigned int new_rand;

    if(r250_index>=147)
     j=r250_index-147;
    else
     j=r250_index+103;
    new_rand=r250_buffer[r250_index]^=r250_buffer[j];
    if(r250_index>=249)
	r250_index=0;
    else
	r250_index++;
    return new_rand/65536.0;
}

static int Select(float Dyst[],int n)      /* Losowanie z dystrybuanty Dyst */
  {
    const float x=DRND();
    register int left=0,right=n-1,mid;

    while(left<=right)			  /* Wyznaczenie elementu metoda */
     {                                    /* bisekcji */
       mid=(left+right) >> 1;
       if(Dyst[mid]==x)
	 break;
       else if(Dyst[mid]<x)
	      left=mid+1;
	    else
	      right=mid-1;
     }
   return left;
 }

int InicGen2D(float func(int,int),int TimeSize,int FreqSize,int Srand)
 {	/* Inicjacja generatora liczb z 2D rozkladu p-stwa func(x,y) */
   register int i,j;
   float sum,norma=0.0F;

   if((DystMatrix=MakeTable(TimeSize,FreqSize))==NULL)
     return -1;
   if((DystVector=(float *)malloc(TimeSize*sizeof(float)))==NULL)
    {
      FreeTable(DystMatrix,TimeSize); DystMatrix=NULL;
      return -1;
    }

   if(Srand==0)
     SRAND(0U);
   else SRAND((unsigned short)time(NULL));

   GlobTimeSize=TimeSize;
   GlobFreqSize=FreqSize;

   for(i=0 ; i<TimeSize ; i++)   /* Generacja rozkladu p-stwa + normalizacja */
     for(j=0 ; j<FreqSize ; j++)
      { 
        const float ftmp=func(i,j); /* Wartosci ujemne nie maja interpretacji */

        norma+=DystMatrix[i][j]=((ftmp<0.0F) ? 1.0F : ftmp); 
      }

   for(i=0,sum=0.0F ; i<FreqSize ; i++) /* Dystrybuanta brzegowa */
    sum+=DystMatrix[0][i];
   DystVector[0]=sum/norma;
   for(i=1 ; i<TimeSize ; i++)
    {
      for(j=0,sum=0.0F ; j<FreqSize ; j++)
       sum+=DystMatrix[i][j];
      DystVector[i]=DystVector[i-1]+sum/norma;
    }

   for(i=0 ; i<TimeSize ; i++)	       /* Dystrybuanty warunkowe */
     {
       for(j=0,sum=0.0F ; j<FreqSize ; j++)
	sum+=DystMatrix[i][j];
       DystMatrix[i][0]/=sum;
       for(j=1 ; j<FreqSize ; j++)
	 DystMatrix[i][j]=DystMatrix[i][j-1]+DystMatrix[i][j]/sum;
     }
   return 0;
 }

void CloseRand2D(void)			/* Zamkniecie generatora 2D */
 {
   if(DystVector!=NULL)
     {
       free((void *)DystVector);
       DystVector=NULL;
     }
   if(DystMatrix!=NULL)
     {
       FreeTable(DystMatrix,GlobTimeSize);
       DystMatrix=NULL;
     }
   GlobTimeSize=GlobFreqSize=0;
 }

int Rand2D(int *Time,int *Freq)		/* Generacja liczb z rozkladu 2D */
 {
   if(DystVector==NULL || DystMatrix==NULL)
     return -1;
   *Time=Select(DystVector,GlobTimeSize);
   *Freq=Select(DystMatrix[*Time],GlobFreqSize);
   return 0;
 }
 
static float *Dyst=NULL; /* Generator rozkaldu 1D */
static int Max1DSize=-1;
 
#include <stdio.h>

int InicRand1D(float Tab[],int n)
 {
   register int i; /* Inicjacja generatora licz losowych */
   float sum=0.0F;

   if(Dyst!=NULL)
     {
       free((void *)Dyst);
       Dyst=NULL;
     }
        
   if((Dyst=(float *)malloc(n*sizeof(float)))==NULL)
     return -1;
     
   Max1DSize=n;
   for(i=0 ; i<n ; i++)      /* Normawanie i regenerowanie */
     sum+=(Tab[i]<0.0F) ? (Dyst[i]=1.0F) : (Dyst[i]=Tab[i]);
     
   if(sum==0.0F)
     {
       free((void *)Dyst); Max1DSize=-1; Dyst=NULL;
       return -1;
     }
         
   Dyst[0]/=sum;            /* Wyznaczenie dystrybuanty */
   for(i=1 ; i<n ; i++)
     Dyst[i]=Dyst[i-1]+Dyst[i]/sum;  
     
   return 0;
 }     
 
void CloseRand1D(void)     /* Zwolnienie pamieci */
 {
   if(Dyst!=NULL)
     {
       free((void *)Dyst);
       Dyst=NULL;
     }
   Max1DSize=-1;
 }    
 
int Rand1D(int *value)     /* Generacja liczb losowych */
 {
   if(Max1DSize<=0)
     return -1;
   *value=Select(Dyst,Max1DSize);
   return 0;
 } 
 


