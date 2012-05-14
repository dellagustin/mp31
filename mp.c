/* Nowa implemementacja algorytmu MP dla slownikow zrandominizowanych
	1997 03 21/22  04 02/05  06 18; 1997 08 01 1997 08 12/15/22/26/27
	Wsparcie heurystyczne + nowa implementacja wyznaczanie iloczynow
	skalarnych (bywa niestabilna)
	1997 10 04 - poprawka do slownika Mallata
	1997 11 22 - zmiana metody przeszukiwania slownika
	1998 05 16 - poprawna interpretacja wyjatku w funkcji atan2 dla
	             kompilatora Borland C/C++ 4.52,+ poprawki do metody heurystycznej

        1998 06 05, 06 22 - wsparcie dla wielowatkowosci (experymentalnie)
	1998 07 16/18     - pelna implementacja wielowatkowosci
	2000 01 06/08/10  - adaptacyjne dobieranie slowka
                            (koniec rozwoju tej wersji oprogramowania)

	Definicja:
		WINDOWSGNU  - dla kompilatora Cygnus GNU C
		MULTITHREAD - kompilacja w trybie wielowatkowym
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "iobook.h"
#include "rand2d.h"
#include "formulc.h"
#include "proto.h"
#include "shell.h"

#define ROCTAVE       2		    /* Zakres przszukiwania oktaw */
#define RFREQ         40            /* ---//-----//------- czestotliwosci */
#define RPOSITION     40            /* ---//-----//------- polorzen*/
#define STARTDICTION  25000         /* Rozmiar slownika wstepnego */
#define MAXSCALE      4		    /* Korekcja niestabilnosci (na razie) */

#define DSQR(X)    ((double)(X)*(double)(X))
#define EPSYLON    1.0e-8           /* Dokladnosc wyznaczania wartosci iloczynow skalarnych */
#define SQR(X)     ((X)*(X))
#define DIRAK      0		    /* Typy atomow (identyfikacja) */
#define FOURIER    1
#define GABOR      3
#define ON         1
#define OFF        0
#define STRING     256
#define MAXOPTIONS 32

typedef float FLOAT;             /* Dla oszczednosci pamieci */
typedef unsigned short USHORT; 

#ifdef __GNUC__
__inline__
#endif
double TSQR(double x) { return x*x; }

typedef struct {
  char       typ;		/* Rodzaj atomu */
  int        scale;		/* skala atomu */
  int        trans;		/* translacja */
  int        iScale;            /* Wskaznik skali */
  int        iFreq;             /* Wskaznik czestotliwosci */
  double     frequency; 	/* czestotliwosc */
  double     modulus;           /* wspolczynnik rozwiniecia */
  double     phase;             /* faza atomu */
  double     amplitude;		/* amplituda sygnalu */
} BIGATOM;    

typedef struct {
  FLOAT CNorm;			/* Norma kosinusowa */
  FLOAT SNorm;			/* Norma sinusowa */
  FLOAT Fc;			/* Iloczyn kosinusowy z sygnalem */
  FLOAT Fs;			/* Iloczyn sinusowy z sygnalem */
  FLOAT Modulus;		/* Ostatina wartosc modulusa (dla przyspiszenia obliczen) */
  FLOAT Amplitude;              /* Ostatina wartosc amplitudy */
  FLOAT Phase;                  /* ---//-----//---- fazy */ 
  UCHAR mode;
} INFOATOM;
                                /* Szybka metoda genracji iloczynow skalarnych */
                                /* dla slownikow diadycznych */
typedef struct {		/* Tablica wartosci komponentow czastkowych */
   FLOAT ConstCos,
         ConstK;
 } HASHVAL;        
                                
static int **ConvTable,MaxIndex;	/* Struktury komponentow */
static HASHVAL **HashTable;
static double ConstFactor;
static FLOAT **HashCosTable;
static USHORT *Scale,*Translate,*Frequency; /* Parametry slownika */

int StartDictionSize=STARTDICTION,Heuristic=ON,
    ROctave=ROCTAVE,RFreqency=RFREQ,RPosition=RPOSITION;  /* Parametry heurystyki */
static double **OldAtomsTable;		/* Tablica maksymalnych atomow dla heurystyki */
                                
static int MakeFastMPDictionary(int);
static void CloseFastMPDictionary(void);
static int GetMaxScale(int);

#ifdef LINUX	/* Funkcje rozwijalne w mjejscu wywolania */
               	/* Written by John C. Bowman <bowman@ipp-garching.mpg.de> */
                        
__inline double InlineSqrt(double __x)
{
  register double __value;
        
  __asm__ __volatile__
	("fsqrt" : "=t" (__value): "0" (__x));

  return __value;
}

__inline double InlineExp(double __x)
{
  register double __value, __exponent;
        
  __asm__ __volatile__
		("fldl2e			# e^x = 2^(x * log2(e))\n\t"
		 "fmul	%%st(1)			# x * log2(e)\n\t"
		 "fstl	%%st(1)\n\t"	
                 "frndint			# int(x * log2(e))\n\t"
		 "fxch\n\t"
		 "fsub	%%st(1)			# fract(x * log2(e))\n\t"
		 "f2xm1				# 2^(fract(x * log2(e))) - 1\n\t"
		 : "=t" (__value), "=u" (__exponent) : "0" (__x));
 __value += 1.0;
 __asm__ __volatile__
		("fscale" : "=t" (__value): "0" (__value), "u" (__exponent));
  return __value;
}

__inline double InlineAtan2(double __y, double __x)
{
  register double __value;
        
  __asm__ __volatile__
		("fpatan\n\t"
		 "fldl %%st(0)"
		 : "=t" (__value): "0" (__x), "u" (__y));

  return __value;
}

__inline double InlineSin(double __x)
{
  register double __value;
        
  __asm__ __volatile__
		("fsin" : "=t" (__value): "0" (__x));

  return __value;
}

__inline double InlineCos(double __x)
{
 register double __value;
        
 __asm__ __volatile__
		("fcos" : "=t" (__value): "0" (__x));

 return __value;
}

__inline double InlineLog(double __x)
{
	register double __value;
        
	__asm__ __volatile__
		("fldln2\n\t"
		 "fxch\n\t"
		 "fyl2x"
		 : "=t" (__value) : "0" (__x));

 	return __value;
}

__inline double InlineFloor(double __x)
{
  register double __value;
  volatile short __cw, __cwtmp;

  __asm__ volatile ("fnstcw %0" : "=m" (__cw) : );
  __cwtmp = (__cw & 0xf3ff) | 0x0400; /* rounding down */
  __asm__ volatile ("fldcw %0" : : "m" (__cwtmp));
  __asm__ volatile ("frndint" : "=t" (__value) : "0" (__x));
  __asm__ volatile ("fldcw %0" : : "m" (__cw));
  return __value;
}

__inline void sincos(double __x, double *__sinx, double *__cosx)
  {
    register double __cosr,__sinr;
    
	 __asm__ __volatile__
     ("fsincos": "=t" (__cosr), "=u" (__sinr) : "0" (__x));
    *__sinx=__sinr; *__cosx=__cosr;
  }

#define exp(X)     InlineExp(X)
#define atan2(X,Y) InlineAtan2((X),(Y))
#define sin(X)     InlineSin(X)
#define cos(X)     InlineCos(X)
#define sqrt(X)    InlineSqrt(X)
#define log(X)     InlineLog(X)
#define floor(X)   InlineFloor(X)
#else
#define sincos(x,Sin,Cos) { *(Sin)=sin(x); *(Cos)=cos(x); }
#ifdef PGI
#define abs(x) __builtin_abs(x)
#define atan2(x,y) __builtin_atan2(x,y)
#define atan(x) __builtin_atan(x)
#define tan(x) __builtin_tan(x)
#define cos(x) __builtin_cos(x)
#define sin(x) __builtin_sin(x)
#define fabs(x) __builtin_fabs(x)
#define sqrt(x) __builtin_sqrt(x)
#define log(x) __builtin_log(x)
#define log10(x) __builtin_log10(x)
#define exp(x) __builtin_exp(x)
#define pow(x,y) __builtin_pow(x,y)
#endif
#endif
 
int DictionSize,RandomType=NOFUNCRND,MaxScale,
    MallatDiction=OFF,OverSampling=2;
float AdaptiveConst=0.9F;

static int NWD(register int a,register int b)	/* Najwiekszy wspolny dzielnik */
 {
    while(a && b) if(a>b) a%=b; else b%=a;
    return ((!a) ? b : a);
 }

int MakeMallatDictionary(int n,int OverSamp,int load)
 {
   const int Halfen=(n >> 1)-1;
   const int LastPos=n-1;
   register int i=0,p1,p3,j,k,skok,Delta,maxT,t;
   int itmp,s,Div;

   p1=GetMaxScale(n); Div=1 << OverSamp;
   for(j=0 ; j<p1 ; j++) if(j!=0) 
    {
      s=1 << j; skok=(s*Div)/(itmp=NWD(Div,s)); p3=s*(1 << (j+OverSamp));
		Delta=s/itmp; maxT=s*(n >> j);
      for(k=0 ; k<p3 ; k+=skok) if(k!=0)
        for(t=0 ; t<maxT ; t+=Delta)
         {
           if(load==ON)
             {
	       Scale[i]=(USHORT)j;
               Frequency[i]=(USHORT)(((double)k*Halfen)/p3);
               Translate[i]=(USHORT)(((double)LastPos*t)/maxT);
             }
           i++;
         }  
    }

   return i;
 }

void InicRandom1D(void)
 {
    if(DiadicStructure==ON)		     /* Dla slownika diadycznego */
     {
       MaxScale=GetMaxScale(DimBase);
       if(InicRand1D(OctaveDyst,MaxScale-1)==-1)
         {
           fprintf(stderr,"Problems with scales generator !\n");
           exit(EXIT_FAILURE);
         }
     }
    else if(InicRand1D(OctaveDyst,DimBase-2)==-1)
     {
       fprintf(stderr,"Problems with scales generator !\n");
		 exit(EXIT_FAILURE);
     }      
 }

int InicRandom(int Srand)
 {
    InicRandom1D();    
    switch(RandomType) {                     /* Generatory funkcji 2D p-stwa */
      case FUNCRND:
	     if(CompileFunction()==-1)
	      {
		fprintf(stderr,"Incorrect form of probability density (CompileFunction) !\n");
		exit(EXIT_FAILURE);
	      }
	     if(prn==ON)
	       fprintf(stdout,"<<< INITIALIZING GENERATOR >>>\n");
	     return InicGen2D(fval,DimBase,DimBase/2,Srand);
      case NOFUNCRND:
		default:
	   if(Srand==OFF)
	     SRAND(0U);        		      /* Dla powtarzlnosci generacji slownika */
	   else SRAND((unsigned short)time(NULL));  /* Inicjacja losowa */
	   break;
		}
   return 0;
 }

void RandTimeAndFreq(int *Time,int *Freq,int RandOpc)
 { 		     /* Generacja pozycji atomu w przestrzeni czas-czestosc */
   switch(RandOpc) { /* Dla slownikow w pelni zrandomizowanych */
     case FUNCRND:
	    (void)Rand2D(Time,Freq);
	    return;
     case NOFUNCRND:
     default:
	  *Time=RANDOM(DimBase);
	  *Freq=1+RANDOM((DimBase >> 1)-1);
	  break;
     }
 }

#ifdef __GNUC__
__inline
#else
static
#endif
void LoadBigAtom(BIGATOM *atom,const int DimBase,const int k)
 {
   const double pi2=2.0*M_PI;		/* Ladowanie atomu */

   if(DiadicStructure==OFF)
     atom->scale=(int)Scale[k];
   else 
    {
      atom->iScale=(int)Scale[k];
      atom->scale=1U << atom->iScale; 
    }  
    
   atom->trans=Translate[k];
   atom->iFreq=Frequency[k];
   atom->frequency=pi2*(double)(atom->iFreq)/(double)DimBase;
 }

static void CloseDictionary(void)
 {
   if(Scale!=NULL)
     free((void *)Scale);
	if(Translate!=NULL)
     free((void *)Translate);
   if(Frequency!=NULL)
     free((void *)Frequency);
 }        

#define SWAP(X,Y)  tmp=(X); (X)=(Y); (Y)=tmp 

static int MakeDictionary(void)
 {
   unsigned int Size;
   int i,Time,Freq;
   USHORT OldSeed,GetSeed(void);		/* Tworzenie stablicowanych */
                                        	/* parametrow atomow */
   if(prn==ON)
     fprintf(stdout,"<<< GENERATING THE DICTIONARY >>>\n");
   
   if(MallatDiction==ON)			/* Zmiana wielkosci globalnej */
     DictionSize=MakeMallatDictionary(DimBase,OverSampling,OFF);
     
   Size=DictionSize*sizeof(USHORT);
   if((Scale=(USHORT *)malloc(Size))==NULL ||
      (Translate=(USHORT *)malloc(Size))==NULL ||
      (Frequency=(USHORT *)malloc(Size))==NULL)
     {
       CloseDictionary();
       return -1;
     }

   if(MallatDiction==ON)
     {
       if(prn==ON)
         fprintf(stdout,"<<< DYADIC DICTIONARY %d ATOMS >>>\n",
		 DictionSize+(3*DimBase)/2);
                 
       (void)MakeMallatDictionary(DimBase,OverSampling,ON);
       if(Heuristic==ON)
        {
          if(prn==ON)
            fprintf(stdout,"<<< MIXING THE DICTIONARY >>>\n");
        			                /* Na potrzeby heurystyki mieszamy */
          for(i=0 ; i<DictionSize ; i++)        /* atomy */
            {
              register USHORT tmp;
              const int k=(int)((double)rand()*DictionSize/(double)RAND_MAX);
              
	      SWAP(Scale[k],Scale[i]);
              SWAP(Translate[k],Translate[i]);
              SWAP(Frequency[k],Frequency[i]); 
            }
        }
       return 0;
     }  
  
   OldSeed=GetSeed();				/* Slownik stochastyczny */
   for(i=0 ; i<DictionSize ; i++)
    {
      int itmp;
      
      RandTimeAndFreq(&Time,&Freq,RandomType);
      Rand1D(&itmp);
      Scale[i]=(USHORT)(1+itmp);    
      Translate[i]=(USHORT)Time;
      Frequency[i]=(USHORT)Freq;
    }

  SRAND(OldSeed);
  return 0;
 }       
 
#undef SWAP
 
void ShowDictionary(char *opt)  /* Drukowanie struktury slownika */
 {
	ULONG *TransHist,*ScaleHist=NULL,*FreqHist;
	char *argv[MAXOPTIONS],filename[STRING],raportname[STRING],
	napis[STRING]="dist";
	const float df=0.5F*(float)(DimBase-1)/M_PI, /* Konwersja czestotliwosci */
			 FreqConv=((SamplingRate<=0.0F) ? 1.0F : 0.5F*SamplingRate/M_PI);
	const unsigned Size=(DimBase+1U)*sizeof(ULONG);
	int i,argc,opcja,gendat=OFF,genrap=ON;
	float *tmptab,MaxTrans,MaxScale,MaxFreq;
	FILE *stream=NULL;
	BIGATOM atom;

	StrToArgv(opt,argv,&argc);			/* Konwersja na postac argv */
	opterr=optind=0; sp=1;
	while((opcja=Getopt(argc,argv,"O:d:r:"))!=EOF)
	  switch(opcja) {
		 case 'O':
		  (void)strcpy(napis,optarg);
		  break;
		 case 'd':
		  if(strcmp(optarg,"+")==0)
			gendat=ON;
		 else if(strcmp(optarg,"-")==0)
			 gendat=OFF;
		  else
		    {
		      fprintf(stderr,"Available options -d[-|+] !\n");
		      FreeArgv(argv,argc);
		      return;
		    }
		  break;
		 case 'r':
			if(strcmp(optarg,"+")==0)
			genrap=ON;
		 else if(strcmp(optarg,"-")==0)
			 genrap=OFF;
			else {
			    fprintf(stderr,"Available options -r[-|+] !\n");
			    FreeArgv(argv,argc);
			    return;
			  }
		  break;
		 default:
			fprintf(stderr,"Unknown option !\n");
			FreeArgv(argv,argc);
			return;
	 }

	FreeArgv(argv,argc);
	sprintf(filename,"%s.dat",napis);
	sprintf(raportname,"%s.rap",napis);

	if(prn==ON)
	  fprintf(stdout,"<<< GENERATING REPORT FROM THE DICTIONARY >>>\n");

	if(gendat==ON)
	 if((stream=fopen(filename,"wt"))==NULL)
	  {
		fprintf(stderr,"Cannot open file %s\n",filename);
		return;
	  }
		      /* Tablice rozkladu slownika (histogramy) */
	if((TransHist=(ULONG *)malloc(Size))==NULL ||
	   (ScaleHist=(ULONG *)malloc(Size))==NULL ||
	   (FreqHist=(ULONG *)malloc(Size))==NULL)
	  {
	    if(TransHist!=NULL)
	      free((void *)TransHist);
	    if(ScaleHist!=NULL)
	      free((void *)ScaleHist);
	    if(stream!=NULL)
	      fclose(stream);
	    fprintf(stderr,"Bad memory allocation !\n");
	    return;
	  }

	if((tmptab=MakeVector(DimBase+PARSIZE+1))==NULL)
	  {
	    fprintf(stderr,"Cannot allocate memory !\n");
	    if(stream!=NULL)
	      fclose(stream);
	    free((void *)TransHist); free((void *)ScaleHist); free((void *)FreqHist);
	    return;
	  }

	for(i=0 ; i<DimBase ; i++)
	  TransHist[i]=ScaleHist[i]=FreqHist[i]=0UL;
     
	if(MakeDictionary()==-1)
	  {
	    fprintf(stderr,"Cannot generate the dictionary !\n");
	    if(stream!=NULL)
	      fclose(stream);
	    free((void *)TransHist); free((void *)ScaleHist);
	    free((void *)FreqHist); free((void *)tmptab);
	    return;
	  }  
   
	for(i=0 ; i<DictionSize ; i++)
	  {
	    LoadBigAtom(&atom,DimBase,i); 
	    if(gendat==ON)
	      fprintf(stream,"%3d %3d %6.5f\n",(int)atom.scale,
		      (int)atom.trans,FreqConv*atom.frequency);
	    if(genrap==ON)
	      {
		ScaleHist[atom.scale]++;
		TransHist[atom.trans]++;
		FreqHist[(int)(0.5F+df*atom.frequency)]++;
	      }
	  }
    
	if(stream!=NULL)
	  fclose(stream);

	if(genrap==ON)
	  {
	    MaxTrans=MaxScale=MaxFreq=0.0F;
	    for(i=1 ; i<DimBase ; i++)
	      {
		if(MaxTrans<(float)TransHist[i])
		  MaxTrans=TransHist[i];
		if(MaxScale<(float)ScaleHist[i])
		  MaxScale=ScaleHist[i];
		if(MaxFreq<(float)FreqHist[i])
		  MaxFreq=FreqHist[i];
	      }

	    MaxTrans=((MaxTrans==0.0F) ? 1.0F : 1.0F/MaxTrans);
	    MaxScale=((MaxScale==0.0F) ? 1.0F : 1.0F/MaxScale);
	    MaxFreq=((MaxFreq==0.0F) ? 1.0F : 1.0F/MaxFreq);

	    if((stream=fopen(raportname,"wt"))!=NULL)
	      {
		for(i=1 ; i<DimBase ; i++)
		  fprintf(stream,"%3d %7.6f %7.6f %7.6f\n",i,(float)ScaleHist[i]*MaxScale,
			  (float)TransHist[i]*MaxTrans,(float)FreqHist[i]*MaxFreq);
		fclose(stream);
	      }
	    else fprintf(stderr,"Cannot open file %s !\n",raportname);
	  }

	free((void *)tmptab); free((void *)TransHist);
	free((void *)ScaleHist); free((void *)FreqHist);
	CloseDictionary();
 }

UCHAR Log2(float x)	/* Logarytm przy podstawie 2 x calkowite */
 {
   if(fabs(x)<1.0e-8)
     return (UCHAR)0;
   return (UCHAR)(0.5+log(fabs(x))/M_LN2);
 }   
  
void PrintBigAtom(BIGATOM *atom)      /* Wydruk parametrow atomu na ekran */
 {
   char *name="Zly !";
   
   switch(atom->typ) {
     case DIRAK:
          name="Dirac";
          break;
     case FOURIER:
          name="Fourier";
          break;
     case GABOR:
          name="Gabor";
          break;
    }
                     
   fprintf(stdout,"  %10.7g  %3d  %3d  %8.6f  %8.6f  %s\n",
           atom->modulus,(int)atom->scale,
           (int)atom->trans,atom->frequency,atom->phase,name);
   fflush(stdout);
 }
                	/* Procedury numeryczne */

static INFOATOM *InfoTable;            /* Inforamcje o iloczynach skalarnych */
static double *GaborTab,*TmpExpTab;    /* Tablice pomocnicze */
static double ConstScale;	       /* Zakres istotnosci atomu */

#ifdef __GNUC__
__inline
#else
static
#endif 
int AtomLen(BIGATOM *atom,int DimBase)
 {
   const int len=(int)(ConstScale*atom->scale);
   
   return (len>=DimBase) ? DimBase-1 : len;
 }    

static void MakeExpTable(register double *ExpTab,double alpha,int trans,
                         register int start,register int stop)
 {
  register int left,right;	  	   /* Szybka generacja tablicy funkcji */
  register double Factor,OldExp,ConstStep; /* wykladniczej */
   
  if(start<trans && trans<stop)            /* Maksimum w srodku przedzialu */
   {                            	  
    *(ExpTab+trans)=OldExp=1.0;
    Factor=exp(-alpha);
    ConstStep=SQR(Factor);		   /* Symetria */	
    
    for(left=trans-1,right=trans+1 ; 
        start<=left && right<=stop ; 
        left--,right++)
     {
       OldExp*=Factor;
       *(ExpTab+left)=OldExp;
       *(ExpTab+right)=OldExp;
       Factor*=ConstStep;
     }  
   
    if(left>start)			  /* Kompensacja odstepstw od symetrii */
      for( ; start<=left ; left--)
       {
	 *(ExpTab+left)=OldExp*=Factor;
	 Factor*=ConstStep;
       }
    else for( ; right<=stop; right++)
          {
            *(ExpTab+right)=OldExp*=Factor;
            Factor*=ConstStep;
          }
    return;      
   } 
   
  ConstStep=exp(-2.0*alpha);		/* Maksimum poza przedzialem */ 
  if(trans>=stop)
   {
     const int itmp=trans-stop;		/* Przedzial na lewo od maksimum */

     *(ExpTab+stop)=OldExp=exp(-alpha*DSQR(itmp));
	  Factor=exp(-alpha*(double)((itmp << 1)+1));
     
     for(left=stop-1; start<=left ; left--)
      {
        *(ExpTab+left)=OldExp*=Factor;
        Factor*=ConstStep;
      }
   } 
   else
    {                                  /* Przedzial na prawo od maksimum */	
      const int itmp=start-trans;
      
      *(ExpTab+start)=OldExp=exp(-alpha*DSQR(itmp));
      Factor=exp(-alpha*(double)((itmp << 1)+1));
   
      for(right=start+1; right<=stop ; right++)
	{
	  *(ExpTab+right)=OldExp*=Factor;
	  Factor*=ConstStep;
	}
    }   
 }              

static void DotGaborAtoms(double alpha,double freq,int trans,int start,
                          register int stop,register double *GaborTab,
                          register double *ExpTab,double *DotSin,
                          double *DotCos)	      
 {
   register double NewCos,dtmp,sum1,sum2;
   double Sin,Cos,OldSin,OldCos;
   register int i;    

   sincos(freq,&Sin,&Cos);
   MakeExpTable(ExpTab,alpha,trans,start,stop);
   sincos(freq*(double)(start-trans),&OldSin,&OldCos);
   dtmp=*(ExpTab+start)**(GaborTab+start);
   sum1=dtmp*OldSin;
   sum2=dtmp*OldCos;
   
   for(i=start+1 ; i<=stop ; i++)		/* Iloczyny sklarne */
    {                                   	/* atomow gabora i fouriera */
      NewCos=OldCos*Cos-OldSin*Sin;
      OldSin=OldCos*Sin+OldSin*Cos;
      OldCos=NewCos;
      dtmp=*(ExpTab+i)**(GaborTab+i);
      sum1+=dtmp*OldSin;
      sum2+=dtmp*OldCos;
    }
    
   *DotSin=sum1;
   *DotCos=sum2;  
 }

/* Wyznaczenie iloczynu sklarnego atomow gabora
	metoda oparta na sumach  Gabora (dla slownikow diadycznych)
   te wielkosci nalezy zainicjowac  
   S1sqr=DSQR(atom1.scale);
   temp=-atom1.frequency*atom1.trans+atom1.phase;
 */ 

static double FastGaborDot(BIGATOM *atom1,BIGATOM *atom2,INFOATOM *info,
                           double Factor,double S1sqr,double temp,
                           double *amplitude,double *phase)
 {
   const int index=ConvTable[atom1->iScale][atom2->iScale];	
   const double ConstK=Factor*HashTable[index][abs(atom1->trans-atom2->trans)].ConstK;
   double Sin1,Sin2,Cos1,Cos2,Sin,Cos,Modulus;
  
   if(ConstK>EPSYLON)
    {              
      const double S2sqr=DSQR(atom2->scale),
	tw=(S2sqr*atom1->trans+S1sqr*atom2->trans)/(S1sqr+S2sqr),
	C1=ConstK*HashTable[index][atom1->iFreq+atom2->iFreq].ConstCos,
	C2=ConstK*HashTable[index][abs(atom1->iFreq-atom2->iFreq)].ConstCos,
	dtmp1=atom1->frequency*tw+temp, 
	dtmp2=atom2->frequency*(tw-(double)atom2->trans),             
	phase1=dtmp1-dtmp2,phase2=dtmp1+dtmp2;
                   
      sincos(phase1,&Sin1,&Cos1);
      sincos(phase2,&Sin2,&Cos2);
      info->Fc-=C2*Cos1+C1*Cos2;		
      info->Fs-=C1*Sin2-C2*Sin1; 
    }
   else
    {
      *amplitude=info->Amplitude;
      *phase=info->Phase;
      return info->Modulus;
    }

   if(info->Fs==0.0 && info->Fc==0.0)/* Zabespieczenie przed wyjatkiem w BC 4.5 */
     *phase=0.0;
   else
     *phase=atan2(-info->Fs/info->SNorm,info->Fc/info->CNorm);

   info->Phase=*phase;
   sincos(*phase,&Sin,&Cos);
   Modulus=TSQR(info->Fc*Cos-info->Fs*Sin)/(*amplitude=
	       (info->CNorm*SQR(Cos)+info->SNorm*SQR(Sin)));

   info->Amplitude=*amplitude;
   return info->Modulus=Modulus;            
 } 
   /* Wyznaczenie iloczynow sklarnych atomu Fouriera z Gaborem 
      dla slownikow diadycznych */

static void FastFourierDot(BIGATOM *atom1,BIGATOM *atom2,
                           double *Fc,double *Fs,register double Factor,
			   register double temp)
 {
  const int index=atom2->iScale;
  const double C1=Factor*HashCosTable[index][atom1->iFreq+atom2->iFreq],
               C2=Factor*HashCosTable[index][abs(atom1->iFreq-atom2->iFreq)],
               phase=atom1->frequency*(double)atom2->trans+temp;
  double Sin,Cos;
               
  sincos(phase,&Sin,&Cos);             
  *Fc=C2*Cos+C1*Cos;        
  *Fs=C1*Sin-C2*Sin;
 }                   

double FindCrossAtomPhase(BIGATOM *atom1,BIGATOM *atom2,double *GaborTab,
                          double *TmpExpTab,int DimBase,INFOATOM *info,
                          double *phase,double *amplitude,double temp,
			  double Factor)
 {                         
  double dtmp,dtmp2,dtmp3,DotSin,DotCos,Sin,Cos,K1,trans,Modulus;
  int start,stop,pozycja,itmp,itmp2,MaxStop;
  
  switch(atom1->typ) {
    case GABOR:
     {
       const double S1sqr=DSQR(atom1->scale), /* Wyznaczenie fazy atomu gabora */	
                   S2sqr=DSQR(atom2->scale),  /* z aktualizacja iloczynow sklarnych */
                   u1=(double)(atom1->trans),
                   u2=(double)(atom2->trans); 
      
       dtmp2=u1-u2; 
       K1=exp(-M_PI*SQR(dtmp2)/(dtmp=S1sqr+S2sqr));
       if(K1>EPSYLON)		/* Wyznaczenie obszaru istotnosci atomu */
        {       
         if(DiadicStructure==ON && VeryFastMode==ON) /* Zwiekszenie wydajnosci algorytmu (mozliwe niestabilnosci) */  
	   if(atom1->iScale>MAXSCALE && atom2->iScale>MAXSCALE)
             return FastGaborDot(atom1,atom2,info,Factor,S1sqr,temp,
				 amplitude,phase);
         MaxStop=3*DimBase-1;
         dtmp3=log(K1/EPSYLON);
         trans=floor(0.5+(S1sqr*u2+S2sqr*u1)/dtmp);
         pozycja=(int)(DimBase+trans);
	 dtmp=1.5+sqrt(dtmp3/(M_PI*(S1sqr+S2sqr)/(S1sqr*S2sqr)));
         start=pozycja-(int)dtmp;
         if(start<0)
           start=0;
         stop=pozycja+(int)dtmp;
         if(stop>MaxStop)
           stop=MaxStop;
            
         DotGaborAtoms(M_PI/DSQR(atom2->scale),atom2->frequency,
                       DimBase+atom2->trans,start,stop,GaborTab,TmpExpTab,
                       &DotSin,&DotCos);

         info->Fs-=DotSin;		/* Aktualizacja iloczynow skalarnych */
	 info->Fc-=DotCos;
       }
      else
       {
         *amplitude=info->Amplitude;
         *phase=info->Phase;
         return info->Modulus;
       } 
     }  
     break;
   case FOURIER:
         if(DiadicStructure!=ON)
          {    
           itmp=AtomLen(atom2,DimBase);  /* Wyznaczamy iloczyn skalarny na */
           itmp2=DimBase+atom2->trans;   /* obszarze analizowanego atomu */
           DotGaborAtoms(M_PI/DSQR(atom2->scale),atom2->frequency,
			 itmp2,itmp2-itmp,itmp2+itmp,
                         GaborTab,TmpExpTab,&DotSin,&DotCos);
											/* Uwaga ! Zakresu nie trzeba sprawdzac */
          }				/* Zwiekszenie wydajnosci dla slownikow diadycznych */
         else FastFourierDot(atom1,atom2,&DotCos,&DotSin,
                             Factor,temp);
         
         info->Fs-=DotSin;		
         info->Fc-=DotCos;
     break;    
   case DIRAK:				 /* Dla delty Diraka to tylko */
         itmp=atom1->trans-atom2->trans; /* odpowiednie wartosci */
         dtmp=(double)itmp/(double)atom2->scale;
         dtmp=atom1->modulus*exp(-M_PI*SQR(dtmp));
         info->Fs-=dtmp*sin(dtmp2=atom2->frequency*(double)itmp);
         info->Fc-=dtmp*cos(dtmp2);
     break;
  }     

  if(info->Fs==0.0 && info->Fc==0.0)
    *phase=0.0;
  else
    *phase=atan2(-info->Fs/info->SNorm,info->Fc/info->CNorm);

  info->Phase=*phase;
  sincos(*phase,&Sin,&Cos);
  Modulus=TSQR(info->Fc*Cos-info->Fs*Sin)/(*amplitude=
	      (info->CNorm*SQR(Cos)+info->SNorm*SQR(Sin)));

  info->Amplitude=*amplitude;
  return info->Modulus=Modulus;     
 }                              

static void MakeGaborAtom(double *GaborFunc,register int n,BIGATOM *atom)	      
 {
   register int i;
   const double ConstExp=exp(-M_PI/SQR((double)(atom->scale))),
                ConstStep=SQR(ConstExp),
                Ampli=atom->modulus/atom->amplitude,
                AmpliCos=Ampli*cos(atom->phase),
                AmpliSin=-Ampli*sin(atom->phase);
   double *PtrGaborFunc=GaborFunc+atom->trans,
          OldSin=0.0,OldCos=1.0,NewCos,OldExp=1.0,Factor=ConstExp,
          dtmp1,dtmp2,Sin,Cos;             
               
   sincos(atom->frequency,&Sin,&Cos);            
   *PtrGaborFunc=AmpliCos;		/* Generacja atomu Gabora */
   for(i=1 ; i<n ; i++)
    {
      NewCos=OldCos*Cos-OldSin*Sin;
      OldSin=OldCos*Sin+OldSin*Cos;
      OldCos=NewCos;
      OldExp*=Factor;
      Factor*=ConstStep;
      dtmp1=AmpliCos*OldExp*OldCos;
      dtmp2=AmpliSin*OldExp*OldSin;
      *(PtrGaborFunc+i)=dtmp1+dtmp2;
      *(PtrGaborFunc-i)=dtmp1-dtmp2;
    }
 }

static void MakeFourierAtom(double *sygnal,
                            register int n,BIGATOM *atom)	      
 {
   register int i;
   double Sin,Cos,CosPhase,SinPhase;
   const double Constans=atom->modulus/atom->amplitude;
   register double OldSin=0.0,OldCos=1.0,NewCos,dtmp,dtmp2;

   sincos(atom->frequency,&Sin,&Cos);
   sincos(atom->phase,&SinPhase,&CosPhase);
   sygnal+=atom->trans;
   *sygnal=CosPhase*Constans;		/* Generacja atomu Fouriera */
   for(i=1 ; i<n ; i++)
     {
       NewCos=OldCos*Cos-OldSin*Sin;
       OldSin=OldCos*Sin+OldSin*Cos;
       OldCos=NewCos;
       dtmp=CosPhase*NewCos;
       dtmp2=SinPhase*OldSin;
       *(sygnal+i)=Constans*(dtmp-dtmp2);
       *(sygnal-i)=Constans*(dtmp+dtmp2);
     }
 }

static void MakeMaximalAtom(double *GabTab,BIGATOM *atom,int DimBase)
  {
    switch(atom->typ) {  /* Generacja atomu (interface) */
      case GABOR:
                  MakeGaborAtom(GabTab+DimBase,AtomLen(atom,DimBase),atom);
		  break;
      case FOURIER:            
		  MakeFourierAtom(GabTab+DimBase,DimBase,atom);
                  break;
    }      
 }
      
/* Wyznaczenie iloczynu skalarnego funkcji Gabora z sygnalem 
   z jednoczesnym wyznaczeniem optymalnej fazy */

#ifdef MULTITHREAD
__inline
#else
static
#endif
double FindGaborPhase(register double *f,int n,double alpha,
		      double freq,double *phase,double *amplitude,
		      INFOATOM *info)
 {
   register int i;
   double Sin,Cos;
   register double dtmp,dtmp2;
   const double ConstExp=exp(-alpha),ConstStep=SQR(ConstExp);
   double OldSin=0.0,OldCos=1.0,NewCos,OldExp=1.0,
          Factor=ConstExp,Ks=0.0,Kc=0.0,Fs=0.0,Fc=*f,Modulus;

   sincos(freq,&Sin,&Cos);
   for(i=1 ; i<n ; i++)
     {
       NewCos=OldCos*Cos-OldSin*Sin;
       OldSin=OldCos*Sin+OldSin*Cos;
       OldExp*=Factor;
       dtmp=OldExp*NewCos;
       dtmp2=OldExp*OldSin;
       Kc+=SQR(dtmp);
       Ks+=SQR(dtmp2);
       Fc+=dtmp*(*(f+i)+*(f-i));
       Fs+=dtmp2*(*(f+i)-*(f-i));
       Factor*=ConstStep;
       OldCos=NewCos;
     }    

    Kc=2.0*Kc+1.0;
    Ks*=2.0;
    if(fabs(Fs)<1.0e-10 && fabs(Fc)<1.0e-10)
      *phase=0.0;
    else
      *phase=atan2(-Fs/Ks,Fc/Kc);
    sincos(*phase,&OldSin,&OldCos);
    Modulus=TSQR(Fc*OldCos-Fs*OldSin)/(*amplitude=
	    (Kc*SQR(OldCos)+Ks*SQR(OldSin)));

    if(FastMode==ON)			/* Dla szybkiej metody */
      {                          	/* inicjacja struktur danych */
	info->Fs=Fs;
	info->Fc=Fc;
	info->CNorm=Kc;
	info->SNorm=Ks;
	info->Modulus=Modulus;
	info->Amplitude=*amplitude;
	info->Phase=*phase;
      }
    
    return Modulus;             
 }
  
/* Iloczyn cosinusa z sygnalem residualnym 
   z jednoczesnym wyznaczeniem fazy */  
  
static double FindFourierPhase(register double *f,double freq,register int n,
                               double *phase,double *amplitude)                        
 {   
   register int i;
   double Sin,Cos;
   double OldSin=0.0,OldCos=1.0,NewCos,
          Ks=0.0,Kc=0.0,Fs=0.0,Fc=*f;
          
   sincos(freq,&Sin,&Cos);
   for(i=1 ; i<n ; i++)
     {
       NewCos=OldCos*Cos-OldSin*Sin;
       OldSin=OldCos*Sin+OldSin*Cos;
       OldCos=NewCos;
       Kc+=SQR(NewCos);
       Ks+=SQR(OldSin);
       Fc+=NewCos*(*(f+i)+*(f-i));
       Fs+=OldSin*(*(f+i)-*(f-i));
     }    
    
   Kc=2.0*Kc+1.0;
   Ks*=2.0;
   
   if(fabs(Fs)<1.0e-10 && fabs(Fc)<1.0e-10)
     *phase=0.0;
   else
     *phase=atan2(-Fs/Ks,Fc/Kc);
   
   sincos(*phase,&OldSin,&OldCos);
   return TSQR(Fc*OldCos-Fs*OldSin)/(*amplitude=
	      (Kc*SQR(OldCos)+Ks*SQR(OldSin)));
 }

/* Poszukiwanie atomu w slowniku */

#define NOTIME  0		   /* Przy reinicjacji nie piszemy czasu obliczen */
#define PRNTIME 1

#ifndef WINDOWSGNU
static double FirstIterTime=0.0;   /* Pomiar czasu pierwszej iteracji */
#endif
 
static void UpDate(double *sygnal,int DimBase,INFOATOM *info,BIGATOM *atom)
{
  /* Aktualizacja iloczynow skalarnych dla metody heurystycznej */
  atom->modulus=FindGaborPhase(sygnal+atom->trans,AtomLen(atom,DimBase),
			       M_PI/SQR((double)atom->scale),
			       atom->frequency,&atom->phase,
			       &atom->amplitude,info);
}

/* 2000-01-05 */

int debug=0;
static void find(float a[],int n,int k) {
  register int l=0,p=n-1,i,j;	
  float x,w;
  
  while(l<p) {
    x=a[k]; i=l; j=p;
    do {
      while(a[i]>x) i++;
      while(x>a[j]) j--;
      if(i<=j) {
	w=a[i]; 
	a[i]=a[j]; 
	a[j]=w;
	i++; 
	j--;
      }
    } while(i<=j);
    if(j<k) l=i;
    if(k<i) p=j;
  }
}

static float findmin(float a[],int k,int n) {
  register int i;
  float min;

  find(a,n,k);
  for(i=0,min=a[0] ; i<k ; i++)
    if(min>a[i])
      min=a[i];
  return min;
}

static void MakeAdaptiveDictionary(void) {
  float Const,*mod;
  register int i,k;

  if(prn==1)
    fprintf(stdout,"<<< MAKEING ADAPTIVE DICTIONARY >>>\n");

  if(debug==1) {
    FILE *file;
    if(prn==1) 
      fprintf(stdout,"<<< SAVEING MODULUS >>>\n");
    if((file=fopen("debug.dat","wt"))!=NULL) {
      for(i=0 ; i<DictionSize ; i++)
	fprintf(file,"%g\n",InfoTable[i].Modulus);
      fclose(file);
    }
  }
  
  if(fabs(AdaptiveConst-1.0)<1.0e-8) {
    for(i=0 ; i<DictionSize ; i++) 
      InfoTable[i].mode=1;
    return;
  }

  if((mod=(float *)malloc(DictionSize*sizeof(float)))==NULL) {
    fprintf(stderr,"malloc error: %s %d\n",__FILE__,__LINE__);
    exit(EXIT_FAILURE);
  }
 
  for(i=0 ; i<DictionSize ; i++) 
    mod[i]=InfoTable[i].Modulus;
 
  Const=findmin(mod,(int)((1.0-AdaptiveConst)*DictionSize),DictionSize);
  free((void *)mod);

  for(i=0,k=0 ; i<DictionSize ; i++)
    if(InfoTable[i].Modulus>=Const) {
      InfoTable[i].mode=1;
      k++;
    } else {
      InfoTable[i].mode=0;
    }

  if(prn==1) {
    fprintf(stdout,"<<< MODULUS TRESHOLD: %g >>>\n",Const);
    fprintf(stdout,"<<< NEW DICTINARY SIZE: %d (%6.3fx) >>>\n",k,
	    (float)DictionSize/k);
  }
}

static int GetMaxScale(int DimBase)     /* Liczba skala dla diadycznych */
 {
   return (int)(0.5+log((double)DimBase)/M_LN2);
 }   

#ifndef MULTITHREAD

static void FindBigAtom(double *sygnal,int DimBase,BIGATOM *maxatom,
                        int CurIter,int prntime,int Reinit)
 {
   const int MaxFreq=DimBase/2;
   const double df=M_PI/(double)MaxFreq; 
   int MaxOctave=GetMaxScale(DimBase);
   register double max,tmp;
   static BIGATOM OldMaxAtom;
   BIGATOM atom;
   register int i,k,itmp;
   int iScaleMin,iScaleMax,iFreqMin,iFreqMax,iPosMin,iPosMax;   
   
   max=SQR(sygnal[0]);			/* Baza Diraka */
   maxatom->typ=DIRAK;
   maxatom->modulus=sygnal[0];
   maxatom->amplitude=1.0;
   maxatom->phase=0.0;
   maxatom->frequency=0.5*M_PI;
   maxatom->trans=0;
   maxatom->scale=0;
   
   for(i=1 ; i<DimBase ; i++)		
    if((tmp=SQR(sygnal[i]))>max)
      {
	max=tmp;
	maxatom->modulus=sygnal[i];
        maxatom->trans=i;
      }
        
   atom.typ=FOURIER;			/* Baza Fouriera */
   atom.trans=MaxFreq;
   atom.scale=DimBase;

   for(i=1 ; i<MaxFreq ; i++)		
    {
     atom.modulus=FindFourierPhase(sygnal+MaxFreq,
              atom.frequency=df*(double)i,DimBase,
              &atom.phase,&atom.amplitude); 
              
     atom.iFreq=i;           
     if(atom.modulus>max)
       {
	 max=atom.modulus;
	 (void)memcpy((void *)maxatom,(void *)&atom,sizeof(BIGATOM));
       }
    }              
         
   atom.typ=GABOR;      
   if(CurIter==0 || FastMode==OFF || Reinit==ON)
     {
#ifndef WINDOWSGNU
       clock_t t1=0L,t2;
       
       if(FastMode==ON && prn==ON)
	 t1=clock();
#endif

       for(i=0 ; i<StartDictionSize ; i++) /* Atomy Gabora */
	 {
	   LoadBigAtom(&atom,DimBase,i);
	   atom.modulus=FindGaborPhase(sygnal+atom.trans,AtomLen(&atom,DimBase),
				       M_PI/SQR((double)atom.scale),atom.frequency,
				       &atom.phase,&atom.amplitude,&InfoTable[i]);
	   
	   if(atom.modulus>max)
	     {
	       max=atom.modulus;
	       (void)memcpy((void *)maxatom,(void *)&atom,sizeof(BIGATOM));
	     }
	 }

       MakeAdaptiveDictionary();      
       if(maxatom->typ==GABOR && Heuristic==ON)    /* Ustalenie zakresu poszukiwan */
	 {                                  	/* zakres poczatkowy */
	   iScaleMin=maxatom->iScale-ROctave;
	   if(iScaleMin<0) 
	     iScaleMin=0; 
	   
	   iScaleMax=maxatom->iScale+ROctave;
	   if(iScaleMax>MaxOctave)
	     iScaleMax=MaxOctave;
	   
	   iPosMin=maxatom->trans-RPosition;
	   if(iPosMin<0)
	     iPosMin=0; 
	   
	   iPosMax=maxatom->trans+RPosition;
	   if(iPosMax>=DimBase)
	     iPosMax=DimBase-1;
      
	   iFreqMin=maxatom->iFreq-RFreqency;
	   if(iFreqMin<=0)
	     iFreqMin=1;

	   iFreqMax=maxatom->iFreq+RFreqency;
	   itmp=DimBase >> 1;
	   if(iFreqMax>=itmp)
	     iFreqMax=itmp;
	   
	   for(i=StartDictionSize ; i<DictionSize ; i++) {
	       LoadBigAtom(&atom,DimBase,i);		/* Poszukujemy atomow z otoczenia maksimum */
	       if(atom.iScale>=iScaleMin && atom.iScale<=iScaleMax)
		 if(atom.iFreq>=iFreqMin  && atom.iFreq<=iFreqMax)
		   if(atom.trans>=iPosMin  && atom.trans<=iPosMax)
		     {
		       atom.modulus=FindGaborPhase(sygnal+atom.trans,
						   AtomLen(&atom,DimBase),
						   M_PI/SQR((double)atom.scale),
						   atom.frequency,
						   &atom.phase,&atom.amplitude,
						   &InfoTable[i]);

		       if(atom.modulus>max)
			 {
			   max=atom.modulus;
			   memcpy((void *)maxatom,(void *)&atom,sizeof(BIGATOM));
			   
			   iScaleMin=maxatom->iScale-ROctave; /* Ustalenie nowego */
			   if(iScaleMin<0) 		    /* zakresu poszukiwan */
			     iScaleMin=0;
			   
			   iScaleMax=maxatom->iScale+ROctave;
			   if(iScaleMax>MaxOctave)
			     iScaleMax=MaxOctave;
			   
			   iPosMin=maxatom->trans-RPosition;
			   if(iPosMin<0)
			     iPosMin=0;
			   
			   iPosMax=maxatom->trans+RPosition;
			   if(iPosMax>=DimBase)
			     iPosMax=DimBase-1;
			   
			   iFreqMin=maxatom->iFreq-RFreqency;
			   if(iFreqMin<=0)
			     iFreqMin=1;
			   
			   iFreqMax=maxatom->iFreq+RFreqency;
			   itmp=DimBase >> 1;
			   if(iFreqMax>=itmp)
			     iFreqMax=itmp;
			 }
		     }  
	     }
	 }
      
#ifndef WINDOWSGNU
       if(FastMode==ON && prn==ON && prntime==PRNTIME)
	 {
         double Tcomp;
         char napis[STRING]="";
         
         t2=clock();
	 FirstIterTime=Tcomp=(double)(t2-t1)/(double)CLOCKS_PER_SEC;
	 
	 if(Tcomp!=0)
           sprintf(napis,"%5.0f atoms/sec ",
		   (double)(DictionSize+(3*DimBase)/2)/(double)Tcomp);
         fprintf(stdout,"<<< FIRST ITERATION : %g sec"
		 " (%.0f min %.2f sek) %s>>>\n",
		 Tcomp,floor(Tcomp/60.0),
		 Tcomp-60.0*floor(Tcomp/60.0),napis);
	 }
#endif
       if(prn==ON) {
	 fprintf(stdout,"ITER. ENERG[%%]    MODULUS  SCALE POS"
		 "    FREQ     PHASE    TYPE\n");
       }
     }
   else
     {
       const int Nmax=3*DimBase;
       double Factor,temp;

       for(i=0 ; i<Nmax ; i++)
	 GaborTab[i]=0.0;

       MakeMaximalAtom(GaborTab,&OldMaxAtom,DimBase);
       if(Heuristic==ON)
	 for(i=0 ; i<Nmax ; i++)
	   OldAtomsTable[CurIter][i]=GaborTab[i];
       
       temp=-OldMaxAtom.frequency*OldMaxAtom.trans+OldMaxAtom.phase;
       Factor=OldMaxAtom.modulus/OldMaxAtom.amplitude;
       
       for(i=0 ; i<StartDictionSize ; i++)	/* Atomy Gabora */
	 if(InfoTable[i].mode) {
	   LoadBigAtom(&atom,DimBase,i);
	   atom.modulus=FindCrossAtomPhase(&OldMaxAtom,&atom,GaborTab,
					   TmpExpTab,DimBase,&InfoTable[i],&atom.phase,
					   &atom.amplitude,temp,Factor);
	   
	   if(atom.modulus>max) {
	     max=atom.modulus;
	     (void)memcpy((void *)maxatom,(void *)&atom,sizeof(BIGATOM));
	   }
	 }
       
       if(maxatom->typ==GABOR && Heuristic==ON)   /* Ustalenie zakresu poszukiwan */
	 {
	   iScaleMin=maxatom->iScale-ROctave;
	   if(iScaleMin<0)
	     iScaleMin=0;

	   iScaleMax=maxatom->iScale+ROctave;
	   if(iScaleMax>MaxOctave)
	     iScaleMax=MaxOctave;

	   iPosMin=maxatom->trans-RPosition;
	   if(iPosMin<0)
	     iPosMin=0;
	   
	   iPosMax=maxatom->trans+RPosition;
	   if(iPosMax>=DimBase)
	     iPosMax=DimBase-1;
	   
	   iFreqMin=maxatom->iFreq-RFreqency;
	   if(iFreqMin<=0)
	     iFreqMin=1;
	   
	   iFreqMax=maxatom->iFreq+RFreqency;
	   itmp=DimBase >> 1;
	   if(iFreqMax>=itmp)
	     iFreqMax=itmp;

	   for(i=StartDictionSize,k=0 ; i<DictionSize ; i++,k++)
	     {
	       LoadBigAtom(&atom,DimBase,i);  /* Przegladane sa tylko atomy z otoczenia maksimum */
	       if(atom.iScale>=iScaleMin && atom.iScale<=iScaleMax)
		 if(atom.iFreq>=iFreqMin  && atom.iFreq<=iFreqMax)
		   if(atom.trans>=iPosMin  && atom.trans<=iPosMax)
		     {
		       UpDate(sygnal,DimBase,&InfoTable[i],&atom);
		       if(atom.modulus>max)
			 {
			   max=atom.modulus;
			   memcpy((void *)maxatom,(void *)&atom,sizeof(BIGATOM));
			   
			   iScaleMin=maxatom->iScale-ROctave;
			   if(iScaleMin<0)
			     iScaleMin=0;
			   
			   iScaleMax=maxatom->iScale+ROctave;
			   if(iScaleMax>MaxOctave)
			     iScaleMax=MaxOctave;  
			   
			   iPosMin=maxatom->trans-RPosition;
			   if(iPosMin<0)
			     iPosMin=0;
         
			   iPosMax=maxatom->trans+RPosition;
			   if(iPosMax>=DimBase)
			     iPosMax=DimBase-1;
			   
			   iFreqMin=maxatom->iFreq-RFreqency;
			   if(iFreqMin<=0)
			     iFreqMin=1;
         
			   iFreqMax=maxatom->iFreq+RFreqency;
			   itmp=DimBase >> 1;
			   if(iFreqMax>=itmp)
			     iFreqMax=itmp;
			 }  
		     }
	     }
	 }
     }
   
   if(maxatom->typ!=DIRAK)         /* Sprawdzamy kwadraty modulusa */
     {
       maxatom->modulus=sqrt(maxatom->modulus);
       maxatom->amplitude=sqrt(maxatom->amplitude);  
     }         
   
   memcpy((void *)&OldMaxAtom,(void *)maxatom,sizeof(BIGATOM));
   if(maxatom->typ==DIRAK)
    {
      if(maxatom->modulus<0.0)      /* Konwencja interpretacji fazy atomu Diraka */
         {
           maxatom->modulus*=-1.0;
           maxatom->phase=M_PI;
         }
    } 

   if(maxatom->phase<0.0)
     maxatom->phase+=2.0*M_PI; 
 }                     

#else    /* Wersja wielowatkowa */

#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

static BIGATOM *maxatom;
static double max,*MSygnal;
static int iScaleMin,iScaleMax,iFreqMin,iFreqMax,iPosMin,iPosMax; 
static int ShmPtr;

static void SetSharedMemory(void) {
  InfoTable=(INFOATOM *)shmat(ShmPtr,NULL,0);
}

void Loop(int Start,int Stop,void f(int,BIGATOM *))
{
  const int iStop=Stop/2; /* Implementacja mechanizmu wspolbierznego */
  int pid,i;        /* Rozwiniecie petli za pomoca funkcji fork() */
  double lmax;
  BIGATOM *atomMax1,atomMax2,atom;
  int MemId;
 
  if((MemId=shmget(IPC_PRIVATE,sizeof(BIGATOM),0666|IPC_CREAT))==-1)
    {
      fprintf(stderr,"Error allocating shared memory (Loop)\n");
      exit(EXIT_FAILURE);
    }

  lmax=max;
  if((pid=fork())==-1)
    {
      fprintf(stderr,"Cannot create child process\n");
      exit(EXIT_FAILURE);
    }
  else if(pid==0)       /* Proces potomny */
    {
      SetSharedMemory(); /* Ustawienie wskaznikow dla procesow potomnych */
      atomMax1=(BIGATOM *)shmat(MemId,NULL,0);
      memcpy((void *)atomMax1,(void *)maxatom,sizeof(BIGATOM));

      for(i=Start ; i<=iStop ; i++)
	{
	  f(i,&atom);
	  if(atom.modulus>lmax)
	    {
	      lmax=atom.modulus;
	      memcpy((void *)atomMax1,(void *)&atom,sizeof(BIGATOM));
	    }
	}

      /* Procesy ZOMBI zablokowane przez przejacie sygnalu SIGCLD 
	 w programie glownym (stan procesu nie jest istotny) */
      exit(EXIT_SUCCESS); /* Wyjscie z procesu potomnego  */
    }
  else                    /* Proces macirzysty */
    {
      memcpy((void *)&atomMax1,(void *)maxatom,sizeof(BIGATOM)); 
      for(i=iStop+1 ; i<Stop ; i++)
	{
	  f(i,&atom);
	  if(atom.modulus>lmax)
	    {
	      lmax=atom.modulus;
	      memcpy((void *)&atomMax2,(void *)&atom,sizeof(BIGATOM));
	    }
	}

      wait(NULL);           /* Czekamy ma zakonczenie procesu potomnego 
			     stan procesu potomnego jest ignorowany */
      atomMax1=(BIGATOM *)shmat(MemId,NULL,0);
                            /* Proces macierzysty (wybor optimum) */
      if(atomMax1->modulus>max || atomMax2.modulus>max)
	{
	  if(atomMax1->modulus>atomMax2.modulus)
	    memcpy((void *)maxatom,(void *)atomMax1,sizeof(BIGATOM)); 
	  else
	    memcpy((void *)maxatom,(void *)&atomMax2,sizeof(BIGATOM));
	  max=maxatom->modulus;
	}
 
      shmdt((void *)atomMax1); shmctl(MemId,IPC_RMID,0);
    }
}

void OneIteration(int i,BIGATOM *atom) {
  atom->typ=GABOR;
  LoadBigAtom(atom,DimBase,i);
  atom->modulus=FindGaborPhase(MSygnal+atom->trans,AtomLen(atom,DimBase),
			       M_PI/SQR((double)atom->scale),atom->frequency,
			       &atom->phase,&atom->amplitude,&InfoTable[i]);
}

static double Factor,temp;
static BIGATOM OldMaxAtom;  

void TwoIteration(int i,BIGATOM *atom) {
  atom->typ=GABOR;
  LoadBigAtom(atom,DimBase,i);
  atom->modulus=FindCrossAtomPhase(&OldMaxAtom,atom,GaborTab,
				   TmpExpTab,DimBase,&InfoTable[i],
				   &atom->phase,
				   &atom->amplitude,temp,Factor);
}

static int MaxFreq;
static double df;
  
static void FindBigAtom(double *sygnal,int DimBase,BIGATOM *MaxAtom,
                        int CurIter,int prntime,int Reinit)
 {
   int MaxOctave=GetMaxScale(DimBase);
   register double tmp;
   register int i,k,itmp;  
   BIGATOM atom;

   maxatom=MaxAtom;
   MSygnal=sygnal;
   MaxFreq=DimBase/2;
   df=M_PI/(double)MaxFreq;

   max=SQR(sygnal[0]);			/* Baza Diraka */
   maxatom->typ=DIRAK;
   maxatom->modulus=sygnal[0];
   maxatom->amplitude=1.0;
   maxatom->phase=0.0;
   maxatom->frequency=0.5*M_PI;
   maxatom->trans=0;
   maxatom->scale=0;
   
   for(i=1 ; i<DimBase ; i++)		
    if((tmp=SQR(sygnal[i]))>max)
      {
	max=tmp;
	maxatom->modulus=sygnal[i];
        maxatom->trans=i;
      }
   
   atom.typ=FOURIER;			/* Baza Fouriera */
   atom.trans=MaxFreq;
   atom.scale=DimBase;  
   atom.iFreq=1;

   for(i=1 ; i<MaxFreq ; i++)
     {
       atom.modulus=FindFourierPhase(sygnal+MaxFreq,
				     atom.frequency=df*(double)i,DimBase,
				     &atom.phase,&atom.amplitude); 
 
       atom.iFreq=i;  
       if(atom.modulus>max)
	 {
	   max=atom.modulus;
	   memcpy((void *)maxatom,(void *)&atom,sizeof(BIGATOM));
	 }
     }
         
   atom.typ=GABOR;      
   if(CurIter==0 || FastMode==OFF || Reinit==ON)
     { 
       clock_t t1=0L,t2;
       if(FastMode==ON && prn==ON)
	 t1=clock();

     	/* Atomy Gabora (slownik probkujacy) wersja wielowatkowa */
       Loop(0,StartDictionSize,OneIteration);
       if(maxatom->typ==GABOR && Heuristic==ON)    /* Ustalenie zakresu poszukiwan */
	 {                                  	/* zakres poczatkowy */
	   iScaleMin=maxatom->iScale-ROctave;
	   if(iScaleMin<0) 
	     iScaleMin=0; 
	   
	   iScaleMax=maxatom->iScale+ROctave;
	   if(iScaleMax>MaxOctave)
	     iScaleMax=MaxOctave;
	   
	   iPosMin=maxatom->trans-RPosition;
	   if(iPosMin<0)
	     iPosMin=0; 
	   
	   iPosMax=maxatom->trans+RPosition;
	   if(iPosMax>=DimBase)
	     iPosMax=DimBase-1;
      
	   iFreqMin=maxatom->iFreq-RFreqency;
	   if(iFreqMin<=0)
	     iFreqMin=1;

	   iFreqMax=maxatom->iFreq+RFreqency;
	   itmp=DimBase >> 1;
	   if(iFreqMax>=itmp)
	     iFreqMax=itmp;
	   
	   for(i=StartDictionSize ; i<DictionSize ; i++)
	     {
	       LoadBigAtom(&atom,DimBase,i);		/* Poszukujemy atomow z otoczenia maksimum */
	       if(atom.iScale>=iScaleMin && atom.iScale<=iScaleMax)
		 if(atom.iFreq>=iFreqMin  && atom.iFreq<=iFreqMax)
		   if(atom.trans>=iPosMin  && atom.trans<=iPosMax)
		     {
		       atom.modulus=FindGaborPhase(sygnal+atom.trans,
						   AtomLen(&atom,DimBase),
						   M_PI/SQR((double)atom.scale),
						   atom.frequency,
						   &atom.phase,&atom.amplitude,
						   &InfoTable[i]);

		       if(atom.modulus>max)
			 {
			   max=atom.modulus;
			   memcpy((void *)maxatom,(void *)&atom,sizeof(BIGATOM));
			   
			   iScaleMin=maxatom->iScale-ROctave; /* Ustalenie nowego */
			   if(iScaleMin<0) 		    /* zakresu poszukiwan */
			     iScaleMin=0;
			   
			   iScaleMax=maxatom->iScale+ROctave;
			   if(iScaleMax>MaxOctave)
			     iScaleMax=MaxOctave;
			   
			   iPosMin=maxatom->trans-RPosition;
			   if(iPosMin<0)
			     iPosMin=0;
			   
			   iPosMax=maxatom->trans+RPosition;
			   if(iPosMax>=DimBase)
			     iPosMax=DimBase-1;
			   
			   iFreqMin=maxatom->iFreq-RFreqency;
			   if(iFreqMin<=0)
			     iFreqMin=1;
			   
			   iFreqMax=maxatom->iFreq+RFreqency;
			   itmp=DimBase >> 1;
			   if(iFreqMax>=itmp)
			     iFreqMax=itmp;
			 }
		     }  
	     }
	 }
       
       if(FastMode==ON && prn==ON && prntime==PRNTIME)
	 {
	   double Tcomp;
	   char napis[STRING]="";
         
	   t2=clock();
	   FirstIterTime=Tcomp=(double)(t2-t1)/(double)CLOCKS_PER_SEC;
	 
	   if(Tcomp!=0)
	     sprintf(napis,"%5.0f atoms/sec ",
		     (double)(DictionSize+(3*DimBase)/2)/(double)Tcomp);
	   fprintf(stdout,"<<< FIRST ITERATION: %g sec"
		   " (%.0f min %.2f sek) %s>>>\n",
		   Tcomp,floor(Tcomp/60.0),Tcomp-60.0*floor(Tcomp/60.0),napis);
	 }
     }
   else
     {
       const int Nmax=3*DimBase;

       for(i=0 ; i<Nmax ; i++)
	 GaborTab[i]=0.0;

       MakeMaximalAtom(GaborTab,&OldMaxAtom,DimBase);
       if(Heuristic==ON)
	 for(i=0 ; i<Nmax ; i++)
	   OldAtomsTable[CurIter][i]=GaborTab[i];
       
       temp=-OldMaxAtom.frequency*OldMaxAtom.trans+OldMaxAtom.phase;
       Factor=OldMaxAtom.modulus/OldMaxAtom.amplitude;
       
       Loop(0,StartDictionSize,TwoIteration);
       if(maxatom->typ==GABOR && Heuristic==ON)   /* Ustalenie zakresu poszukiwan */
	 {
	   iScaleMin=maxatom->iScale-ROctave;
	   if(iScaleMin<0)
	     iScaleMin=0;

	   iScaleMax=maxatom->iScale+ROctave;
	   if(iScaleMax>MaxOctave)
	     iScaleMax=MaxOctave;

	   iPosMin=maxatom->trans-RPosition;
	   if(iPosMin<0)
	     iPosMin=0;
	   
	   iPosMax=maxatom->trans+RPosition;
	   if(iPosMax>=DimBase)
	     iPosMax=DimBase-1;
	   
	   iFreqMin=maxatom->iFreq-RFreqency;
	   if(iFreqMin<=0)
	     iFreqMin=1;
	   
	   iFreqMax=maxatom->iFreq+RFreqency;
	   itmp=DimBase >> 1;
	   if(iFreqMax>=itmp)
	     iFreqMax=itmp;

	   for(i=StartDictionSize,k=0 ; i<DictionSize ; i++,k++)
	     {
	       LoadBigAtom(&atom,DimBase,i);  /* Przegladane sa tylko atomy z otoczenia maksimum */
	       if(atom.iScale>=iScaleMin && atom.iScale<=iScaleMax)
		 if(atom.iFreq>=iFreqMin  && atom.iFreq<=iFreqMax)
		   if(atom.trans>=iPosMin  && atom.trans<=iPosMax)
		     {
		       UpDate(sygnal,DimBase,&InfoTable[i],&atom);
		       if(atom.modulus>max)
			 {
			   max=atom.modulus;
			   memcpy((void *)maxatom,(void *)&atom,sizeof(BIGATOM));
			   
			   iScaleMin=maxatom->iScale-ROctave;
			   if(iScaleMin<0)
			     iScaleMin=0;
			   
			   iScaleMax=maxatom->iScale+ROctave;
			   if(iScaleMax>MaxOctave)
			     iScaleMax=MaxOctave;  
			   
			   iPosMin=maxatom->trans-RPosition;
			   if(iPosMin<0)
			     iPosMin=0;
         
			   iPosMax=maxatom->trans+RPosition;
			   if(iPosMax>=DimBase)
			     iPosMax=DimBase-1;
			   
			   iFreqMin=maxatom->iFreq-RFreqency;
			   if(iFreqMin<=0)
			     iFreqMin=1;
         
			   iFreqMax=maxatom->iFreq+RFreqency;
			   itmp=DimBase >> 1;
			   if(iFreqMax>=itmp)
			     iFreqMax=itmp;
			 }  
		     }
	     }
	 }
     }
   
   if(maxatom->typ!=DIRAK)         /* Sprawdzamy kwadraty modulusa */
     {
       maxatom->modulus=sqrt(maxatom->modulus);
       maxatom->amplitude=sqrt(maxatom->amplitude);  
     }         
   
   memcpy((void *)&OldMaxAtom,(void *)maxatom,sizeof(BIGATOM));
   if(maxatom->typ==DIRAK)
    {
      if(maxatom->modulus<0.0)      /* Konwencja interpretacji fazy atomu Diraka */
         {
           maxatom->modulus*=-1.0;
           maxatom->phase=M_PI;
         }
    } 

   if(maxatom->phase<0.0)
     maxatom->phase+=2.0*M_PI; 
 }                     

#endif

static void SubBigAtom(double *sygnal,register int n,BIGATOM *atom)	      
 {
   register int i;    /* Odjecie od sygnalu atomu Gabora */
   const double Sin=sin(atom->frequency),Cos=cos(atom->frequency),
                CosPhase=cos(atom->phase),SinPhase=sin(atom->phase),
                ConstExp=exp(-M_PI/SQR((double)(atom->scale))),
                ConstStep=SQR(ConstExp),
                Constans=atom->modulus/atom->amplitude;
   register double OldSin=0.0,OldCos=1.0,NewCos,dtmp,dtmp2,dtmp3,
	           OldExp=1.0,Factor=ConstExp;

   *sygnal-=CosPhase*Constans;
   for(i=1 ; i<n ; i++)
     {
       NewCos=OldCos*Cos-OldSin*Sin;
       OldSin=OldCos*Sin+OldSin*Cos;
       OldCos=NewCos;
       OldExp*=Factor;
       Factor*=ConstStep;
       dtmp3=OldExp*Constans;
       dtmp=CosPhase*NewCos;
       dtmp2=SinPhase*OldSin;
       *(sygnal+i)-=dtmp3*(dtmp-dtmp2);
       *(sygnal-i)-=dtmp3*(dtmp+dtmp2);
     }
 }

static void SubFourierAtom(double *sygnal,register int n,BIGATOM *atom)	      
 {
   register int i;  /* Odjecie od sygnalu kosinusa */
   const double Sin=sin(atom->frequency),Cos=cos(atom->frequency),
                CosPhase=cos(atom->phase),SinPhase=sin(atom->phase),
                Constans=atom->modulus/atom->amplitude;
   register double OldSin=0.0,OldCos=1.0,NewCos,dtmp,dtmp2;

   *sygnal-=CosPhase*Constans;	
   for(i=1 ; i<n ; i++)
    {
      NewCos=OldCos*Cos-OldSin*Sin;
      OldSin=OldCos*Sin+OldSin*Cos;
      OldCos=NewCos;
      dtmp=CosPhase*NewCos;
      dtmp2=SinPhase*OldSin;
      *(sygnal+i)-=Constans*(dtmp-dtmp2);
      *(sygnal-i)-=Constans*(dtmp+dtmp2);
    }
 }
 
static double **MakeDoubleTable(int Rows,int Col)   /* Alokacja pamieci pod tablice */
 {
   const unsigned Size=Col*sizeof(double);
   double **ptr;
   int i,j;
   
   if((ptr=(double **)malloc(Rows*sizeof(double *)))==NULL)
     return NULL;
     
   for(i=0 ; i<Rows ; i++)
    if((ptr[i]=(double *)malloc(Size))==NULL)
      {
        for(j=0 ; j<i ; j++)
          free((void *)ptr[i]);
        free((void *)ptr);
        return NULL;
      }
    else 
      memset((void *)ptr[i],0,Size);    
   return ptr;
 }            
  
static void FreeDoubleTable(double **ptr,int Rows)
 {
   int i;
   
   for(i=0 ; i<Rows ; i++)
     free((void *)ptr[i]);
   free((void *)ptr);
 }        
        	/* Algorytm MP analizy sygnalow */

void BigHmpp(float *syg,int DimBase,int DimRoz,BIGATOM atomy[])
 {
   const int TrueDimBase=3*DimBase,CosPosition=DimBase/2;
   double *resid,sum,*BeginSig,E,OldEnerg,*TmpTable;
   int i,j,itmp;

   if((resid=(double *)malloc(TrueDimBase*sizeof(double)))==NULL) {
     fprintf(stderr,"Error allocating memory (BigHmpp 1) !\n");
     exit(EXIT_FAILURE);
   }

   if((GaborTab=(double *)malloc(TrueDimBase*sizeof(double)))==NULL) {
     fprintf(stderr,"Error allocating memory (BigHmpp 2) !\n");
     exit(EXIT_FAILURE);
   }

   if((TmpExpTab=(double *)malloc(TrueDimBase*sizeof(double)))==NULL) {
     fprintf(stderr,"Error allocating memory (BigHmpp 3) !\n");
     exit(EXIT_FAILURE);
   }

   if((TmpTable=(double *)malloc(TrueDimBase*sizeof(double)))==NULL) {
     fprintf(stderr,"Error allocating memory (BigHmpp 4) !\n");
     exit(EXIT_FAILURE);
   }
   
   if(Heuristic==ON)
    if(StartDictionSize>DictionSize)
     {
       StartDictionSize=DictionSize;
       Heuristic=OFF;
     }  
            
   if(Heuristic==ON)	    /* Dla wspomagania wyszukiwania atomow */
     {
       if((OldAtomsTable=MakeDoubleTable(DimRoz,TrueDimBase))==NULL) 
	 {
	   fprintf(stderr,"Error allocating memory (BigHmpp (2)) !\n");
	   exit(EXIT_FAILURE);
	 }   
     }  
   else StartDictionSize=DictionSize; 
 
   if(prn==ON) {
       fprintf(stdout,"<<< CALCULATING ... >>>\n");
       fflush(stdout);
   }
            
   BeginSig=resid+DimBase;
   ConstScale=sqrt(log(1.0/EPSYLON)/M_PI);
     
   for(i=0 ; i<TrueDimBase ; i++)
     resid[i]=(double)syg[i];
    
   for(j=DimBase,sum=0.0; j<2*DimBase ; j++)
     sum+=SQR(resid[j]);
   E0=sum;			        /* Energia sygnalu */
   
   OldEnerg=0.0;			/* Inicjacja calkowitej energii */
   for(j=0 ; j<3*DimBase ; j++)         /* potrzebne do okreslenia stabilnosci metody */
    OldEnerg+=SQR(resid[j]);     
     
   for(i=0 ; i<DimRoz ; i++)
    {
      FindBigAtom(BeginSig,DimBase,&atomy[i],i,PRNTIME,OFF);
      /* Zachowujemy wartsci na wypadek utraty stabilnosci */
      memcpy((void *)TmpTable,(void *)resid,TrueDimBase*sizeof(double));
      switch(atomy[i].typ) {
       case GABOR:
              SubBigAtom(BeginSig+atomy[i].trans,
                         AtomLen(&atomy[i],DimBase),&atomy[i]);
              break;
       case DIRAK:
              if(atomy[i].phase<0.5)
                *(BeginSig+atomy[i].trans)-=atomy[i].modulus;
              else
                *(BeginSig+atomy[i].trans)+=atomy[i].modulus; 
              break;
       case FOURIER:
              SubFourierAtom(BeginSig+CosPosition,DimBase,&atomy[i]);
              break;
      }               
                                      
      for(j=0,sum=0.0; j<3*DimBase ; j++)	/* Po calosci sygnalu lacznie */
         sum+=SQR(resid[j]);			/* z warunkami brzegowymi */
      
      if(sum>OldEnerg)
        {
          if(prn==ON)		/* W przypadku stwierdzenia utray stabilnosci */
             fprintf(stderr,"<<< REINITIALIZATION >>>\n");
                            
            /* Reinicjujemy tablice iloczynow skalarnych */
            /* Startujemy od niezmienionej wartosci */
            
          memcpy((void *)resid,(void *)TmpTable,TrueDimBase*sizeof(double));
          FindBigAtom(BeginSig,DimBase,&atomy[i],i,NOTIME,ON); /* Tak jak dla zerowej iteracji */
          switch(atomy[i].typ) {
            case GABOR:
              SubBigAtom(BeginSig+atomy[i].trans,
                         AtomLen(&atomy[i],DimBase),&atomy[i]);
              break;
            case DIRAK:
              if(atomy[i].phase<0.5)
                *(BeginSig+atomy[i].trans)-=atomy[i].modulus;
              else
                *(BeginSig+atomy[i].trans)+=atomy[i].modulus; 
              break;
            case FOURIER:
              SubFourierAtom(BeginSig+CosPosition,DimBase,&atomy[i]);
              break;
          }               
                                      
         for(j=0,sum=0.0; j<3*DimBase ; j++)	/* Po calosci sygnalu */
           sum+=SQR(resid[j]);
           
        /* Jezeli to nie pomoze to darujemy sobie dalsze obliczenia */
        if(sum>OldEnerg)
         {
           if(prn==ON)
             fprintf(stderr,"<<< Cannot compensate stability loss >>\n");
           break;   
         }  
      }
          
      OldEnerg=sum;
      for(j=DimBase,sum=0.0; j<2*DimBase ; j++)
        sum+=SQR(resid[j]);
      
      E=100.0*(1.0-sum/E0);
      if(E<0.0) E=0.0;
      if(prn==ON && prninfo==ON)
       {   
         fprintf(stdout,"%3d %8.4f%% ",i,E);
         PrintBigAtom(&atomy[i]);
       }
     
      if(E>=epsylon)
        {
          if(prn==ON)
            fprintf(stdout,"<<< Required accuracy reached >>>\n");
          i++;  
          break;
        }
    } 
    
   dimroz=i;	   /* Wymiar rozwiazania */
   itmp=3*DimBase;
   for(i=0 ; i<itmp ; i++)
     sygnal[i]=(float)resid[i];
      
   free((void *)resid); free((void *)GaborTab); free((void *)TmpExpTab);
   free((void *)TmpTable);
   if(Heuristic==ON)
      FreeDoubleTable(OldAtomsTable,DimRoz); 
 }  
  
void AnalizaMP(char *nic)
 {
   register int i;
   BIGATOM *atomy;
   double Tcomp;
#if defined(LINUX) && !defined(__MSDOS__) && !defined(MULTITHREAD)
   clock_t t1,t2;
#else
   time_t start,stop;      
#endif

   if(nic!=NULL && prn==1)
     fprintf(stdout,"%s\n",nic);

   if(MakeDictionary()==-1)   /* Mozliwa jest zmina rozmiaru slownika */
     {
       fprintf(stderr,"Error allocating the dictionary !\n");
       exit(EXIT_FAILURE);
     }  
   
   if(FastMode==ON)
    {
      if(DiadicStructure==ON && VeryFastMode==ON)
        if(MakeFastMPDictionary(DimBase)==-1)
          {
            fprintf(stderr,"Errors allocating the dictionary !\n");
            exit(EXIT_FAILURE);
          } 
#ifdef MULTITHREAD
      if((ShmPtr=shmget(IPC_PRIVATE,DictionSize*sizeof(INFOATOM),
			0666|IPC_CREAT))==-1)
	{
	  fprintf(stderr,"Error allocating shared memory\n");
	  perror("");
	  exit(EXIT_FAILURE);
	}
      InfoTable=(INFOATOM *)shmat(ShmPtr,NULL,0);
#else
      if((InfoTable=(INFOATOM *)malloc(DictionSize*sizeof(INFOATOM)))==NULL)
       {
         fprintf(stderr,"Error allocating memory (AnalizaMP) !\n");
         exit(EXIT_FAILURE);
       }
#endif
    }
            
   if((atomy=(BIGATOM *)malloc(OldDimRoz*sizeof(BIGATOM)))==NULL)
    {
      fprintf(stderr,"Problems allocating memory (AnalizaMP) !\n");
      exit(EXIT_FAILURE);
    }
      
#if defined(LINUX) && !defined(__MSDOS__) && !defined(MULTITHREAD)
   t1=clock();
#else       
   time(&start);
#endif   
   BigHmpp(sygnal,DimBase,OldDimRoz,atomy);
#if defined(LINUX) && !defined(__MSDOS__) && !defined(MULTITHREAD)
   t2=clock();
   Tcomp=(double)(t2-t1)/(double)CLOCKS_PER_SEC-FirstIterTime;
#else       
   time(&stop);
   Tcomp=difftime(stop,start);
#endif   
       
   if(prn==ON)
    {
     char napis[STRING]=" ";
     
     if(Tcomp>0.0)
       sprintf(napis,"%5.0f atoms/sec",
               dimroz*(double)(DictionSize+3*DimBase/2)/(double)Tcomp);
     fprintf(stdout,"\nCalculations time: %g sek (%.0f min %.0f sek) %s\n",
                    Tcomp,floor(Tcomp/60.0),Tcomp-60.0*floor(Tcomp/60.0),napis);
#if defined(LINUX) && !defined(__MSDOS__) && !defined(MULTITHREAD)
     Tcomp+=FirstIterTime;
	  if(Tcomp>0.0)
       sprintf(napis,"%5.0f atoms/sec",
               dimroz*(double)(DictionSize+3*DimBase/2)/(double)Tcomp);
     else napis[0]='\0';          
     fprintf(stdout,"Total calculations time : %g sek (%.0f min %.0f sek) %s\n",
                    Tcomp,floor(Tcomp/60.0),Tcomp-60.0*floor(Tcomp/60.0),napis);
#endif                                        
    }                                    
     
   if(book!=NULL) free((void *)book);
   if((book=(BOOK *)malloc(dimroz*sizeof(BOOK)))==NULL)
    {
      fprintf(stderr,"Problems allocating memory (AnalizaMP) !\n");
      exit(1);
    }  
    
   for(i=0,Ec=0.0 ; i<dimroz ; i++)
     Ec+=SQR(atomy[i].modulus); 

   for(i=0 ; i<dimroz ; i++)	/* Konwenca dla reszty oprogramowania */
    {
      book[i].numer=i;
      *((float *)(book[i].param))=(float)(atomy[i].scale);;
      *((float *)(book[i].param)+1)=(float)(atomy[i].trans);
      *((float *)(book[i].param)+2)=(float)(atomy[i].frequency);
      book[i].phase=(float)(atomy[i].phase);
      book[i].waga=(float)(atomy[i].modulus);
      book[i].amplitude=(float)(atomy[i].amplitude);
      book[i].Energia=(float)SQR(book[i].waga);
    }
    
   Compute=ON;   
   free((void *)atomy);
   
   if(FastMode==ON)
    {
      if(DiadicStructure==ON && VeryFastMode==ON)
        CloseFastMPDictionary();
#ifdef MULTITHREAD          /* Zwolnienie pamieci dzielonej */
      shmdt(InfoTable);
      shmctl(ShmPtr,IPC_RMID,0);
#else
      free((void *)InfoTable);
#endif
    }
      
   CloseDictionary();
  }  
   
static FLOAT **MakeFLOATTable(int Sx,int Sy)
 {
  const unsigned Size=Sy*sizeof(FLOAT);
  int i,j;
  FLOAT **ptr;				/* Alokacja 2D tablicy */
   
  if((ptr=(FLOAT **)malloc(Sx*sizeof(FLOAT *)))==NULL)
     return NULL;
  for(i=0 ; i<Sx ; i++)
    if((ptr[i]=(FLOAT *)malloc(Size))==NULL)
	  {
       for(j=0 ; j<i ; j++)
         free((void *)ptr[i]);
       free((void *)ptr);
       return NULL;
     }
  return ptr;
 }
  
static void FreeFLOATTable(FLOAT **ptr,int Sx)
 {
   int i;				/* Zwolnienie 2D tablicy */
   
   for(i=0 ; i<Sx ; i++)
    free((void *)ptr[i]);
   free((void *)ptr); 
 }  
                                	/* Podobne funkcje dla typow innych tablic */
static int **MakeIntTable(int Sx,int Sy)
 {
  const unsigned Size=Sy*sizeof(int);
  int i,j,**ptr;			
   
  if((ptr=(int **)malloc(Sx*sizeof(int *)))==NULL)
     return NULL;
  for(i=0 ; i<Sx ; i++)
    if((ptr[i]=(int *)malloc(Size))==NULL)
     {
       for(j=0 ; j<i ; j++)
         free((void *)ptr[i]);
       free((void *)ptr);
       return NULL;
     }
  return ptr;
 }
 
static HASHVAL **MakeHashTable(int Sx,int Sy)
 {
  const unsigned Size=Sy*sizeof(HASHVAL);
  HASHVAL **ptr;
  int i,j;
   
  if((ptr=(HASHVAL **)malloc(Sx*sizeof(HASHVAL *)))==NULL)
     return NULL;
  for(i=0 ; i<Sx ; i++)
    if((ptr[i]=(HASHVAL *)malloc(Size))==NULL)
     {
       for(j=0 ; j<i ; j++)
         free((void *)ptr[i]);
       free((void *)ptr);
       return NULL;
     }
  return ptr;
 } 

static void FreeIntTable(int **ptr,int Sx)
 {
   int i;
   
   for(i=0 ; i<Sx ; i++)
    free((void *)ptr[i]);
   free((void *)ptr); 
 }  

static void FreeHashTable(HASHVAL **ptr,int Sx)
 {
   int i;
   
   for(i=0 ; i<Sx ; i++)
    free((void *)ptr[i]);
   free((void *)ptr); 
 }  

static double MakeGaborSum(double alpha,double freq,register int n)
 {
   register int i;
   const double ConstExp=exp(-alpha),ConstStep=SQR(ConstExp);
   register double OldSin=0.0,OldCos=1.0,NewCos,OldExp=1.0,
                   Factor=ConstExp,sum=0.5;
   double Sin,Cos;

   if(freq<=1.0e-6)
    {
      for(i=1 ; i<=n ; i++)
	{
	  OldExp*=Factor;
	  Factor*=ConstStep;
	  sum+=OldExp;
	}  
      return sum;
    }
     
   sincos(freq,&Sin,&Cos);
   for(i=1 ; i<=n ; i++)		/* Generacja sum Gaborow */
    {
      NewCos=OldCos*Cos-OldSin*Sin;
      OldSin=OldCos*Sin+OldSin*Cos;
      OldCos=NewCos;
      OldExp*=Factor;
      Factor*=ConstStep;
      sum+=OldExp*OldCos;
    }  
   return sum; 
 } 
 
static void ExpTable(double alpha,register int n,double tab[])
 {
   const double ConstExp=exp(-alpha),ConstStep=SQR(ConstExp);
	register double OldExp=1.0,Factor=ConstExp;
   register int i;
  
   tab[0]=1.0;			/* Generacja tablicy exp(-alpha*x**2) */
   for(i=1 ; i<n ; i++)
    {
      OldExp*=Factor;
      Factor*=ConstStep;
      tab[i]=OldExp;
    } 
 }  
 
#define EPSY 1.0e-12				/* Dokladnosc wyznaczania sum Gabora */
 
static int MakeFastMPDictionary(int DimBase)	/* Inicjacja tablic sum Gabora */
 {
   const double df=2.0*M_PI/(double)DimBase;
   register int Index=0,i,j,k,len;
	double S1sqr,S2sqr,dtmp,*tmp,scale,alpha;
 
   if(prn==ON)
     fprintf(stdout,"<<< CREATING TABLE OF GABOR SUMS >>>\n");    
   ConstFactor=sqrt(log(1.0/EPSY)/M_PI);
   MaxScale=GetMaxScale(DimBase);
   if((ConvTable=MakeIntTable(MaxScale,MaxScale))==NULL)
     return -1;
     
   for(i=0 ; i<MaxScale ; i++)
    for(j=i ; j<MaxScale ; j++)
      {
        ConvTable[i][j]=ConvTable[j][i]=Index;   
        Index++;
      }  
       
   if((HashTable=MakeHashTable(MaxIndex=Index,DimBase+1))==NULL)
     {
       FreeIntTable(ConvTable,MaxScale); ConvTable=NULL;
       return -1;
     }     
    
   if((HashCosTable=MakeFLOATTable(MaxScale,DimBase+1))==NULL)
    {
      FreeHashTable(HashTable,Index); HashTable=NULL;
      FreeIntTable(ConvTable,MaxScale);  ConvTable=NULL;
      return -1;
    } 
    
   if((tmp=(double *)malloc((DimBase+1)*sizeof(double)))==NULL)
     {
       FreeHashTable(HashTable,Index); HashTable=NULL;
       FreeIntTable(ConvTable,MaxScale);  ConvTable=NULL;
       FreeFLOATTable(HashCosTable,MaxScale); HashCosTable=NULL;
       return -1;
     } 
   
   for(i=0 ; i<MaxScale ; i++)			/* Slowniki Gaborow */
    {
      S1sqr=DSQR(1U << i);
      for(j=i ; j<MaxScale ; j++)
       {
         S2sqr=DSQR(1U << j);
         dtmp=S1sqr+S2sqr;
         scale=(S1sqr*S2sqr)/dtmp;
         alpha=M_PI/scale;
         len=(int)(1.5+ConstFactor*sqrt(scale));
         ExpTable(M_PI/dtmp,DimBase,tmp);
         Index=ConvTable[i][j];
         
         for(k=0 ; k<=DimBase ; k++)
          {
            HashTable[Index][k].ConstK=tmp[k];    
            HashTable[Index][k].ConstCos=MakeGaborSum(alpha,df*(double)k,len);
          }
       }
    }  
    
   for(i=0 ; i<MaxScale ; i++)			/* Slownika Fourierow */
    {
      scale=(double)(1U << i);
      len=(int)(1.5+ConstFactor*scale);
      alpha=M_PI/SQR(scale);
      for(j=0 ; j<=DimBase ; j++)
        HashCosTable[i][j]=MakeGaborSum(alpha,df*(double)j,len); 
    }
    
   free((void *)tmp);       
   return 0;
 }        
 
static void CloseFastMPDictionary(void)
 {
   if(ConvTable!=NULL)				/* Zwolnienie zasobow */
    {
     FreeIntTable(ConvTable,MaxScale);
     ConvTable=NULL;
    }
    
   if(HashTable!=NULL)
    {
      FreeHashTable(HashTable,MaxIndex);  
      HashTable=NULL;
    }
      
   if(HashCosTable!=NULL)
    {
      FreeFLOATTable(HashCosTable,MaxScale);
      HashCosTable=NULL;
    }
 } 

