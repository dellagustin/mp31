   /* Wyznaczenie FFT sygnalu z okienkowaniem 1997 01 17; 1997 10 04 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "shell.h"
#include "proto.h"
					   /* Interface dla programu HMPP */
#define DVECTOR()  (double *)malloc(Size)
#define SQR(X)     (dtmp=(X),dtmp*dtmp)
#define WINDOW1(j,a,b) (1.0-fabs((((j)-1)-(a))*(b))) /* Parzen */
#define WINDOW2(j,a,b) 1.0                           /* Square */
#define WINDOW3(j,a,b) (1.0-SQR((((j)-1)-(a))*(b)))  /* Welch  */
#define STRING 	   256  	/* Maksymalna dlugosc napisow */
#define MAXARGS    30		/* Maksymalna liczba argumentow */
#define PARZEN      1
#define SQARE       2
#define WELCH       3
#define ORGINAL     0
#define RESIDUAL    1
#define YES         1
#define NO          0
#define FREEALL()   if(xRe!=NULL) free((void *)xRe);  \
		    if(xIm!=NULL) free((void *)xIm);  \
		    if(yRe!=NULL) free((void *)yRe);  \
		    if(yIm!=NULL) free((void *)yIm)

/* extern float SamplingRate;		      Czestotliwosc probkowania */
static int fft(int,double *,double *,double *,double *); /* FFT dla dowolnego n */

static int ComputeFFT(int n,float signal[],float ffttab[],int wind,int phase)
 {
   const unsigned Size=n*sizeof(double);
   double *xRe,*xIm=NULL,*yRe=NULL,*yIm=NULL,dtmp,facm,facp,sumw=0.0,w;
   int i,j;		/* FFT sygnalu + okienkowanie */

   if((xRe=DVECTOR())==NULL ||
      (xIm=DVECTOR())==NULL ||
      (yRe=DVECTOR())==NULL ||
      (yIm=DVECTOR())==NULL)
   {
     FREEALL();
     return -1;
   }

  facm=0.5*(double)(n-1)-0.5;
  facp=1.0/(0.5*(double)(n-1)+0.5);
  for(i=0 ; i<n ; i++)
   {
     switch(wind) {
       case PARZEN:
	      w=WINDOW1(i,facm,facp);
	      break;
       case SQARE:
	      w=WINDOW2(i,facm,facp);
	      break;
       case WELCH:
       default:
	      w=WINDOW3(i,facm,facp);
	      break;
      }

     xRe[i]=w*(double)signal[i];
     xIm[i]=0.0;
     sumw+=SQR(w);
   }

  if(fft(n,xRe,xIm,yRe,yIm)==-1)
   {
     FREEALL();
     return -1;
   }

   if(phase==YES)
   {
     for(i=n/2,j=0 ; i>=0 ; i--,j++)
       if(yRe[i]!=0.0 && yIm[i]!=0.0)
	 ffttab[j]=atan2(yRe[i],yIm[i]);
       else
	 ffttab[i]=0.0;
   }
   else
    for(i=n/2,j=0 ; i>=0 ; i--,j++)
      ffttab[j]=sqrt(SQR(yRe[i])+SQR(yIm[i]))/sumw;
   FREEALL();
   return 0;
 }

void SpectrumFFT(char *opt)		     /* Wyznaczenie FFT sygnalu */
 {
   char *argv[MAXARGS],filename[STRING]="fftsig.asc";
   int opcja,window=WELCH,sig=ORGINAL,phase=NO,i,argc,
       itmp,a=0,b=(int)DimBase,n;
   float *Tabfft,df;
   FILE *stream;

   StrToArgv(opt,argv,&argc);
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"O:w:ropax:y:"))!=EOF)
    switch(opcja) {
     case 'O':
	  (void)strcpy(filename,optarg);
	  break;
     case 'x':
	  a=atoi(optarg);
	  break;
     case 'y':
	  b=atoi(optarg);
	  break;
     case 'w':
	  window=atoi(optarg);
	  break;
     case 'o':
	  sig=ORGINAL;
	  break;
     case 'r':
	  sig=RESIDUAL;
	  break;
     case 'p':
	  phase=YES;
	  break;
     case 'a':
	  phase=NO;
	  break;
    default:
	  fputs("Nieznana opcja !\n\r",stderr);
	  FreeArgv(argv,argc);
	  return;
     }

   FreeArgv(argv,argc);
   if((Tabfft=(float *)malloc(DimBase*sizeof(float)))==NULL)
     return;

   if(a<0) a=0;
   else if(a>=(int)DimBase) a=(int)DimBase;

   if(b<0) b=0;
   else if(b>=(int)DimBase) b=(int)DimBase;
   if(a>b) { itmp=a; a=b; b=itmp; }

   n=b-a;
   if(prn==YES)
     fprintf(stdout,"<<< FFT >>>\n");

   if(ComputeFFT(n,((sig==ORGINAL) ? OrgSygnal+a : (sygnal+DimBase+a)),
		 Tabfft,window,phase)==-1)
    {
      free((void *)Tabfft);
      return;
    }

    if(prn==YES)
      fprintf(stdout,"<<< ZAPIS DANYCH >>>\n");

    if((stream=fopen(filename,"wt"))==NULL)
     {
       fprintf(stderr,"Nie moge otworzyc pliku %s do pisania !\n",filename);
       free((void *)Tabfft);
       return;
     }

   if(SamplingRate!=0.0)
     df=SamplingRate/(float)n;
   else
     df=1.0F;

   itmp=n/2;
   for(i=0 ; i<=itmp ; i++)
     fprintf(stream,"%g %g\n",df*(float)i,Tabfft[itmp-i]);
   fclose(stream);
   free((void *)Tabfft);
   if(prn==YES)
     fprintf(stdout,"<<< KONIEC FFT >>>\n");
   return;
 }

/* Arbitrary Length FFT Adaptacja 1997 01 17

   Author:
      Jens Joergen Nielsen            For non-commercial use only.
      Bakkehusene 54                  A $100 fee must be paid if used
      2970 Hoersholm                  commercially. Please contact.
      DENMARK

      e-mail : jnielsen@internet.dk   All rights reserved. March 1996.
*/

#define  maxPrimeFactor        37
#define  maxPrimeFactorDiv2    ((maxPrimeFactor+1)/2)
#define  maxFactorCount        20

static double  c3_1 = -1.5000000000000E+00;  /*  c3_1 = cos(2*pi/3)-1;          */
static double  c3_2 =  8.6602540378444E-01;  /*  c3_2 = sin(2*pi/3);            */
static double  c5_1 = -1.2500000000000E+00;  /*  c5_1 = (cos(u5)+cos(2*u5))/2-1;*/
static double  c5_2 =  5.5901699437495E-01;  /*  c5_2 = (cos(u5)-cos(2*u5))/2;  */
static double  c5_3 = -9.5105651629515E-01;  /*  c5_3 = -sin(u5);               */
static double  c5_4 = -1.5388417685876E+00;  /*  c5_4 = -(sin(u5)+sin(2*u5));   */
static double  c5_5 =  3.6327126400268E-01;  /*  c5_5 = (sin(u5)-sin(2*u5));    */
static double  c8   =  7.0710678118655E-01;  /*  c8 = 1/sqrt(2);    */

static int      groupOffset,dataOffset,adr;
static int      groupNo,dataNo,blockNo,twNo;
static double   omega, tw_re,tw_im;
static double   twiddleRe[maxPrimeFactor], twiddleIm[maxPrimeFactor],
		trigRe[maxPrimeFactor], trigIm[maxPrimeFactor],
		zRe[maxPrimeFactor], zIm[maxPrimeFactor];
static double   vRe[maxPrimeFactorDiv2], vIm[maxPrimeFactorDiv2];
static double   wRe[maxPrimeFactorDiv2], wIm[maxPrimeFactorDiv2];

static void factorize(int n, int *nFact, int fact[])
{
    register int i,j,k;
    int nRadix;
    int radices[7];
    int factors[maxFactorCount];

    nRadix    =  6;
    radices[1]=  2;
    radices[2]=  3;
    radices[3]=  4;
    radices[4]=  5;
    radices[5]=  8;
    radices[6]= 10;

    if (n==1)
    {
	j=1;
        factors[1]=1;
    }
    else j=0;
    i=nRadix;
    while ((n>1) && (i>0))
    {
      if ((n % radices[i]) == 0)
      {
        n=n / radices[i];
        j=j+1;
        factors[j]=radices[i];
      }
      else  i=i-1;
    }
    if (factors[j] == 2)   /*substitute factors 2*8 with 4*4 */
    {   
      i = j-1;
      while ((i>0) && (factors[i] != 8)) i--;
      if (i>0)
      {
        factors[j] = 4;
	factors[i] = 4;
      }
    }
    if (n>1)
    {
	const int itmp=(int)(sqrt(n)+1.0);

	for (k=2; k<itmp; k++)
	    while ((n % k) == 0)
	    {
		n=n / k;
		j=j+1;
		factors[j]=k;
	    }
	if (n>1)
	{
	    j=j+1;
	    factors[j]=n;
	}
    }
    for (i=1; i<=j; i++)
    {
      fact[i] = factors[j-i+1];
    }
    *nFact=j;
}

static int transTableSetup(int sofar[], int actual[], int remain[],
			    int *nFact,
			    int *nPoints)
{
    int i;

    factorize(*nPoints,nFact,actual);
    if(actual[*nFact]>maxPrimeFactor)
      return -1;
    remain[0]=*nPoints;
    sofar[1]=1;
    remain[1]=*nPoints / actual[1];
    for (i=2; i<=*nFact; i++)
    {
	sofar[i]=sofar[i-1]*actual[i-1];
	remain[i]=remain[i-1] / actual[i];
    }
   return 0;
}

static void permute(int nPoint, int nFact,int fact[], int remain[],
	     double xRe[], double xIm[],double yRe[], double yIm[])

{
    register int i,j,k;
    int count[maxFactorCount]; 

    for (i=1; i<=nFact; i++) count[i]=0;
    k=0;
    for (i=0; i<=nPoint-2; i++)
    {
        yRe[i] = xRe[k];
        yIm[i] = xIm[k];
        j=1;
        k=k+remain[j];
	count[1] = count[1]+1;
        while (count[j] >= fact[j])
	{
            count[j]=0;
            k=k-remain[j-1]+remain[j+1];
            j=j+1;
	    count[j]=count[j]+1;
	}
    }
    yRe[nPoint-1]=xRe[nPoint-1];
    yIm[nPoint-1]=xIm[nPoint-1];
}

static void initTrig(int radix)
{
    int i;
    double w,xre,xim;

    w=2.0*M_PI/radix;
    trigRe[0]=1; trigIm[0]=0;
    xre=cos(w);
    xim=-sin(w);
    trigRe[1]=xre; trigIm[1]=xim;
    for (i=2; i<radix; i++)
    {
        trigRe[i]=xre*trigRe[i-1] - xim*trigIm[i-1];
        trigIm[i]=xim*trigRe[i-1] + xre*trigIm[i-1];
    }
}

static void fft_4(double aRe[], double aIm[])
{
    double  t1_re,t1_im, t2_re,t2_im;
    double  m2_re,m2_im, m3_re,m3_im;

    t1_re=aRe[0] + aRe[2]; t1_im=aIm[0] + aIm[2];
    t2_re=aRe[1] + aRe[3]; t2_im=aIm[1] + aIm[3];

    m2_re=aRe[0] - aRe[2]; m2_im=aIm[0] - aIm[2];
    m3_re=aIm[1] - aIm[3]; m3_im=aRe[3] - aRe[1];

    aRe[0]=t1_re + t2_re; aIm[0]=t1_im + t2_im;
    aRe[2]=t1_re - t2_re; aIm[2]=t1_im - t2_im;
    aRe[1]=m2_re + m3_re; aIm[1]=m2_im + m3_im;
    aRe[3]=m2_re - m3_re; aIm[3]=m2_im - m3_im;
}

static void fft_5(double aRe[], double aIm[])
{
    double  t1_re,t1_im, t2_re,t2_im, t3_re,t3_im;
    double  t4_re,t4_im, t5_re,t5_im;
    double  m2_re,m2_im, m3_re,m3_im, m4_re,m4_im;
    double  m1_re,m1_im, m5_re,m5_im;
    double  s1_re,s1_im, s2_re,s2_im, s3_re,s3_im;
    double  s4_re,s4_im, s5_re,s5_im;

    t1_re=aRe[1] + aRe[4]; t1_im=aIm[1] + aIm[4];
    t2_re=aRe[2] + aRe[3]; t2_im=aIm[2] + aIm[3];
    t3_re=aRe[1] - aRe[4]; t3_im=aIm[1] - aIm[4];
    t4_re=aRe[3] - aRe[2]; t4_im=aIm[3] - aIm[2];
    t5_re=t1_re + t2_re; t5_im=t1_im + t2_im;
    aRe[0]=aRe[0] + t5_re; aIm[0]=aIm[0] + t5_im;
    m1_re=c5_1*t5_re; m1_im=c5_1*t5_im;
    m2_re=c5_2*(t1_re - t2_re); m2_im=c5_2*(t1_im - t2_im);

    m3_re=-c5_3*(t3_im + t4_im); m3_im=c5_3*(t3_re + t4_re);
    m4_re=-c5_4*t4_im; m4_im=c5_4*t4_re;
    m5_re=-c5_5*t3_im; m5_im=c5_5*t3_re;

    s3_re=m3_re - m4_re; s3_im=m3_im - m4_im;
    s5_re=m3_re + m5_re; s5_im=m3_im + m5_im;
    s1_re=aRe[0] + m1_re; s1_im=aIm[0] + m1_im;
    s2_re=s1_re + m2_re; s2_im=s1_im + m2_im;
    s4_re=s1_re - m2_re; s4_im=s1_im - m2_im;

    aRe[1]=s2_re + s3_re; aIm[1]=s2_im + s3_im;
    aRe[2]=s4_re + s5_re; aIm[2]=s4_im + s5_im;
    aRe[3]=s4_re - s5_re; aIm[3]=s4_im - s5_im;
    aRe[4]=s2_re - s3_re; aIm[4]=s2_im - s3_im;
}

static void fft_8(void)
{
    double  aRe[4], aIm[4], bRe[4], bIm[4], gem;

    aRe[0] = zRe[0];    bRe[0] = zRe[1];
    aRe[1] = zRe[2];    bRe[1] = zRe[3];
    aRe[2] = zRe[4];    bRe[2] = zRe[5];
    aRe[3] = zRe[6];    bRe[3] = zRe[7];

    aIm[0] = zIm[0];    bIm[0] = zIm[1];
    aIm[1] = zIm[2];    bIm[1] = zIm[3];
    aIm[2] = zIm[4];    bIm[2] = zIm[5];
    aIm[3] = zIm[6];    bIm[3] = zIm[7];

    fft_4(aRe, aIm); fft_4(bRe, bIm);

    gem    = c8*(bRe[1] + bIm[1]);
    bIm[1] = c8*(bIm[1] - bRe[1]);
    bRe[1] = gem;
    gem    = bIm[2];
    bIm[2] =-bRe[2];
    bRe[2] = gem;
    gem    = c8*(bIm[3] - bRe[3]);
    bIm[3] =-c8*(bRe[3] + bIm[3]);
    bRe[3] = gem;
    
    zRe[0] = aRe[0] + bRe[0]; zRe[4] = aRe[0] - bRe[0];
    zRe[1] = aRe[1] + bRe[1]; zRe[5] = aRe[1] - bRe[1];
    zRe[2] = aRe[2] + bRe[2]; zRe[6] = aRe[2] - bRe[2];
    zRe[3] = aRe[3] + bRe[3]; zRe[7] = aRe[3] - bRe[3];

    zIm[0] = aIm[0] + bIm[0]; zIm[4] = aIm[0] - bIm[0];
    zIm[1] = aIm[1] + bIm[1]; zIm[5] = aIm[1] - bIm[1];
    zIm[2] = aIm[2] + bIm[2]; zIm[6] = aIm[2] - bIm[2];
    zIm[3] = aIm[3] + bIm[3]; zIm[7] = aIm[3] - bIm[3];
}

static void fft_10(void)
{
    double  aRe[5], aIm[5], bRe[5], bIm[5];

    aRe[0] = zRe[0];    bRe[0] = zRe[5];
    aRe[1] = zRe[2];    bRe[1] = zRe[7];
    aRe[2] = zRe[4];    bRe[2] = zRe[9];
    aRe[3] = zRe[6];    bRe[3] = zRe[1];
    aRe[4] = zRe[8];    bRe[4] = zRe[3];

    aIm[0] = zIm[0];    bIm[0] = zIm[5];
    aIm[1] = zIm[2];    bIm[1] = zIm[7];
    aIm[2] = zIm[4];    bIm[2] = zIm[9];
    aIm[3] = zIm[6];    bIm[3] = zIm[1];
    aIm[4] = zIm[8];    bIm[4] = zIm[3];

    fft_5(aRe, aIm); fft_5(bRe, bIm);

    zRe[0] = aRe[0] + bRe[0]; zRe[5] = aRe[0] - bRe[0];
    zRe[6] = aRe[1] + bRe[1]; zRe[1] = aRe[1] - bRe[1];
    zRe[2] = aRe[2] + bRe[2]; zRe[7] = aRe[2] - bRe[2];
    zRe[8] = aRe[3] + bRe[3]; zRe[3] = aRe[3] - bRe[3];
    zRe[4] = aRe[4] + bRe[4]; zRe[9] = aRe[4] - bRe[4];

    zIm[0] = aIm[0] + bIm[0]; zIm[5] = aIm[0] - bIm[0];
    zIm[6] = aIm[1] + bIm[1]; zIm[1] = aIm[1] - bIm[1];
    zIm[2] = aIm[2] + bIm[2]; zIm[7] = aIm[2] - bIm[2];
    zIm[8] = aIm[3] + bIm[3]; zIm[3] = aIm[3] - bIm[3];
    zIm[4] = aIm[4] + bIm[4]; zIm[9] = aIm[4] - bIm[4];
}

static void fft_odd(int radix)
{
    double  rere, reim, imre, imim;
    register int i,j,k,n,max;

    n = radix;
    max = (n + 1)/2;
    for (j=1; j < max; j++)
    {
      vRe[j] = zRe[j] + zRe[n-j];
      vIm[j] = zIm[j] - zIm[n-j];
      wRe[j] = zRe[j] - zRe[n-j];
      wIm[j] = zIm[j] + zIm[n-j];
    }

    for (j=1; j < max; j++)
    {
        zRe[j]=zRe[0]; 
        zIm[j]=zIm[0];
        zRe[n-j]=zRe[0]; 
	zIm[n-j]=zIm[0];
        k=j;
	for (i=1; i < max; i++)
	{
	    rere = trigRe[k] * vRe[i];
            imim = trigIm[k] * vIm[i];
	    reim = trigRe[k] * wIm[i];
            imre = trigIm[k] * wRe[i];
            
	    zRe[n-j] += rere + imim;
            zIm[n-j] += reim - imre;
            zRe[j]   += rere - imim;
            zIm[j]   += reim + imre;

            k = k + j;
            if (k >= n)  k = k - n;
        }
    }
    for (j=1; j < max; j++)
    {
	zRe[0]=zRe[0] + vRe[j];
	zIm[0]=zIm[0] + wIm[j];
    }
}

static void twiddleTransf(int sofarRadix, int radix, int remainRadix,
			  double yRe[], double yIm[])

{   /* twiddleTransf */
    double  cosw, sinw, gem;
    double  t1_re,t1_im, t2_re,t2_im, t3_re,t3_im;
    double  t4_re,t4_im, t5_re,t5_im;
    double  m2_re,m2_im, m3_re,m3_im, m4_re,m4_im;
    double  m1_re,m1_im, m5_re,m5_im;
    double  s1_re,s1_im, s2_re,s2_im, s3_re,s3_im;
    double  s4_re,s4_im, s5_re,s5_im;


    initTrig(radix);
    omega = 2.0*M_PI/(double)(sofarRadix*radix);
    cosw =  cos(omega);
    sinw = -sin(omega);
    tw_re = 1.0;
    tw_im = 0;
    dataOffset=0;
    groupOffset=dataOffset;
    adr=groupOffset;
    for (dataNo=0; dataNo<sofarRadix; dataNo++)
    {
        if (sofarRadix>1)
        {
            twiddleRe[0] = 1.0; 
            twiddleIm[0] = 0.0;
            twiddleRe[1] = tw_re;
            twiddleIm[1] = tw_im;
            for (twNo=2; twNo<radix; twNo++)
	    {
                twiddleRe[twNo]=tw_re*twiddleRe[twNo-1]
                               - tw_im*twiddleIm[twNo-1];
                twiddleIm[twNo]=tw_im*twiddleRe[twNo-1]
			       + tw_re*twiddleIm[twNo-1];
	    }
	    gem   = cosw*tw_re - sinw*tw_im;
	    tw_im = sinw*tw_re + cosw*tw_im;
	    tw_re = gem;
        }
	for (groupNo=0; groupNo<remainRadix; groupNo++)
        {
	    if ((sofarRadix>1) && (dataNo > 0))
            {
                zRe[0]=yRe[adr];
                zIm[0]=yIm[adr];
                blockNo=1;
                do {
                    adr = adr + sofarRadix;
                    zRe[blockNo]=  twiddleRe[blockNo] * yRe[adr]
				 - twiddleIm[blockNo] * yIm[adr];
                    zIm[blockNo]=  twiddleRe[blockNo] * yIm[adr]
                                 + twiddleIm[blockNo] * yRe[adr]; 
                    
		    blockNo++;
		} while (blockNo < radix);
	    }
	    else
		for (blockNo=0; blockNo<radix; blockNo++)
                {
		   zRe[blockNo]=yRe[adr];
                   zIm[blockNo]=yIm[adr];
                   adr=adr+sofarRadix;
                }
            switch(radix) {
              case  2  : gem=zRe[0] + zRe[1];
                         zRe[1]=zRe[0] -  zRe[1]; zRe[0]=gem;
                         gem=zIm[0] + zIm[1];
                         zIm[1]=zIm[0] - zIm[1]; zIm[0]=gem;
                         break;
	      case  3  : t1_re=zRe[1] + zRe[2]; t1_im=zIm[1] + zIm[2];
                         zRe[0]=zRe[0] + t1_re; zIm[0]=zIm[0] + t1_im;
                         m1_re=c3_1*t1_re; m1_im=c3_1*t1_im;
                         m2_re=c3_2*(zIm[1] - zIm[2]); 
			 m2_im=c3_2*(zRe[2] -  zRe[1]);
			 s1_re=zRe[0] + m1_re; s1_im=zIm[0] + m1_im;
			 zRe[1]=s1_re + m2_re; zIm[1]=s1_im + m2_im;
			 zRe[2]=s1_re - m2_re; zIm[2]=s1_im - m2_im;
			 break;
              case  4  : t1_re=zRe[0] + zRe[2]; t1_im=zIm[0] + zIm[2];
			 t2_re=zRe[1] + zRe[3]; t2_im=zIm[1] + zIm[3];

                         m2_re=zRe[0] - zRe[2]; m2_im=zIm[0] - zIm[2];
                         m3_re=zIm[1] - zIm[3]; m3_im=zRe[3] - zRe[1];

                         zRe[0]=t1_re + t2_re; zIm[0]=t1_im + t2_im;
                         zRe[2]=t1_re - t2_re; zIm[2]=t1_im - t2_im;
                         zRe[1]=m2_re + m3_re; zIm[1]=m2_im + m3_im;
                         zRe[3]=m2_re - m3_re; zIm[3]=m2_im - m3_im;
                         break;
	      case  5  : t1_re=zRe[1] + zRe[4]; t1_im=zIm[1] + zIm[4];
                         t2_re=zRe[2] + zRe[3]; t2_im=zIm[2] + zIm[3];
                         t3_re=zRe[1] - zRe[4]; t3_im=zIm[1] - zIm[4];
                         t4_re=zRe[3] - zRe[2]; t4_im=zIm[3] - zIm[2];
			 t5_re=t1_re + t2_re; t5_im=t1_im + t2_im;
			 zRe[0]=zRe[0] + t5_re; zIm[0]=zIm[0] + t5_im;
			 m1_re=c5_1*t5_re; m1_im=c5_1*t5_im;
			 m2_re=c5_2*(t1_re - t2_re);
			 m2_im=c5_2*(t1_im - t2_im);

			 m3_re=-c5_3*(t3_im + t4_im);
                         m3_im=c5_3*(t3_re + t4_re);
                         m4_re=-c5_4*t4_im; m4_im=c5_4*t4_re;
                         m5_re=-c5_5*t3_im; m5_im=c5_5*t3_re;

                         s3_re=m3_re - m4_re; s3_im=m3_im - m4_im;
                         s5_re=m3_re + m5_re; s5_im=m3_im + m5_im;
                         s1_re=zRe[0] + m1_re; s1_im=zIm[0] + m1_im;
                         s2_re=s1_re + m2_re; s2_im=s1_im + m2_im;
                         s4_re=s1_re - m2_re; s4_im=s1_im - m2_im;

                         zRe[1]=s2_re + s3_re; zIm[1]=s2_im + s3_im;
                         zRe[2]=s4_re + s5_re; zIm[2]=s4_im + s5_im;
                         zRe[3]=s4_re - s5_re; zIm[3]=s4_im - s5_im;
			 zRe[4]=s2_re - s3_re; zIm[4]=s2_im - s3_im;
			 break;
	      case  8  : fft_8(); break;
	      case 10  : fft_10(); break;
	      default  : fft_odd(radix); break;
            }
	    adr=groupOffset;
            for (blockNo=0; blockNo<radix; blockNo++)
            {
                yRe[adr]=zRe[blockNo]; yIm[adr]=zIm[blockNo];
                adr=adr+sofarRadix;
            }
            groupOffset=groupOffset+sofarRadix*radix;
            adr=groupOffset;
        }
        dataOffset=dataOffset+1;
	groupOffset=dataOffset;
        adr=groupOffset;
    }
}

static int fft(int n, double xRe[], double xIm[],double yRe[], double yIm[])
{
    int   sofarRadix[maxFactorCount],
	  actualRadix[maxFactorCount],
	  remainRadix[maxFactorCount];
    int   nFactor;
    int   count;

    if(transTableSetup(sofarRadix, actualRadix, remainRadix, &nFactor, &n)==-1)
     return -1;
    permute(n, nFactor, actualRadix, remainRadix, xRe, xIm, yRe, yIm);

    for (count=1; count<=nFactor; count++)
      twiddleTransf(sofarRadix[count], actualRadix[count], remainRadix[count],
		    yRe, yIm);
    return 0;
}
