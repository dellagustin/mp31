/* Podstawowe funkcje I/O dla programu hmpp II 1997 04 02 1997 05 28 */
/* 1997 08 20/26 1997 10 04; Ujednolicenie zapisu ksiazek 1998 01 31 */
/* 1998 05 11 poprawki przy inicjacji slownika i wczytaniu plikow ASCII */
/* 1998 06 05 doliczanie amplitudy */
/* 1999 08 29 view */
/* 1999 10 10 float=owa czestosc */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#ifdef __MSDOS__
#include <dir.h>
#else
// modified by GD
// #include <unistd.h>
#endif
#include "new_io.h"
#include "rand2d.h"
#include "shell.h"
#include "iobook.h"
#include "typdef.h"
#include "proto.h"

#define ERR(STR, CHR) if(opterr) fprintf(stdout,"%s%c\n",STR,CHR);
#define FreeVector(TAB) free((void *)TAB)
#define ON     1
#define OFF    0
#define SQR(X) ((X)*(X))
#define MAXOPTIONS 30
#define STRINGLEN  256
#define STRING     STRINGLEN

int opterr=1,optind=1,optopt,sp=1; /* Zmienne pomocnicze dla GetOpt */
char *optarg;
int DimBase,prn=ON,dimroz,OldDimRoz,file_offset,FastMode,VeryFastMode,
    LoadStatus,Compute=OFF,DiadicStructure; 			
float epsylon,*OrgSygnal,*sygnal,SamplingRate,ConvRate,E0,Ec,
      *OctaveDyst;
BOOK *book;
char outname[STRING]="book.b",SSignalname[STRING]="signal.asc";
int ChannelMaxNum=1,ChannelNumber=1;
char infoHeader[STRING]="http://brain.fuw.edu.pl";
int prninfo=ON;

float *MakeVector(int dim)   /* Utworzenie 1D tablicy danych float */
{
	const unsigned size=(unsigned)dim*sizeof(float);
	float *tab;
	
	if((tab=(float *)malloc(size))==NULL) 
		return NULL;
	
	(void)memset((void *)tab,0,size);
	
	return tab;
}

int Getopt(int argc, char **argv,char *opts)  	/* AT&T public domain */
{
  register int c;				/* Interpretacja opcji */
  register char *cp;

  if(sp==1)
    { 
      if(optind>=argc || argv[optind][0]!='-' || argv[optind][1]=='\0')
	return(EOF);
      else if(strcmp(argv[optind],"--")==0)
	{
	  optind++;
	  return(EOF);
	}
    }
  
  optopt=c=argv[optind][sp];
  if(c==':' || (cp=strchr(opts,c))==0)
   {
     ERR(": illegal option -- ", c);
     if(argv[optind][++sp] == '\0')
       {
	 optind++;
	 sp=1;
       }
     return('?');
   }
  if(*++cp==':')
   {
    if(argv[optind][sp+1]!='\0')
      optarg=&argv[optind++][sp+1];
    else if(++optind>=argc)
	   {
	     ERR(": option requires an argument -- ", c);
	     sp=1;
	     return('?');
	   }
	  else
	    optarg=argv[optind++];
    sp=1;
   }
  else
   {
     if(argv[optind][++sp]=='\0')
       {
	 sp=1;
	 optind++;
       }
     optarg=NULL;
   }
  return(c);
}

#undef ERR

int ReadSygnal(char *name,int Mode)  /* Wczytanie sygnalu do analizy */
 {
   FILE *plik;
   char Napis[STRINGLEN];
   register int i;

   if((plik=fopen(name,"rt"))==NULL)
    {
      fputs("\n\rProblems opening file\n\r",stderr);
      return -1;
    }

   i=0;
   while(fscanf(plik,"%s",Napis)>0 && i<DimBase)
     {
       sygnal[Mode+i]=atof(Napis);
       i++;
     }
     
   if(Mode==DimBase)
     for(i=0 ; i<DimBase ; i++)
       OrgSygnal[i]=sygnal[Mode+i];

   Compute=OFF;
   strcpy(SSignalname,name);  
   fclose(plik);
   return 0;
 }

// Load signal from ascii file
void Load(char *opt)		/* Ladowanie sygnalu w trybie ASCII */
{
	char *argv[MAXOPTIONS],name[STRING]="";
	int argc,opcja,Mode=(int)DimBase,ok=0,SymMode=OFF;
	
	StrToArgv(opt,argv,&argc);
	opterr=optind=0; sp=1;
	while((opcja=Getopt(argc,argv,"O:LRCS"))!=EOF)
		switch(opcja) {
	  case 'O':
		  ok=1;
		  strcpy(name,optarg);
		  break;
	  case 'L':
		  Mode=0;			/* Lewy warunek brzegowy */
		  break;
	  case 'R':
		  Mode=2*DimBase;		/* Prawy ----//-----//----- */
		  break;
	  case 'C':
		  Mode=DimBase;		/* Na upartego */
		  break;
	  case 'S':
		  SymMode=ON;
		  Mode=DimBase;
		  break;      
	  default:
		  fprintf(stderr,"Unknown option !\n");
		  FreeArgv(argv,argc);
		  return;
	}
	
	FreeArgv(argv,argc);
	if(ok==0)
	{
		fprintf(stderr,"Filename not given !\n");
		return;
	}
	
	if(prn==ON)
	{
		if(SymMode==ON)
			fprintf(stdout,"<<< SIMMETRIC BORDER CONDITION >>>\n");
		else if(Mode==0)
			fprintf(stdout,"<<< LEFT BORDER CONDITION >>>\n");
		else if(Mode==(int)DimBase)
			fprintf(stdout,"<<< LOADING SIGNAL >>>\n");
		else fprintf(stdout,"<<< RIGHT BORDER CONDITION >>>\n");
	}
	
	if(ReadSygnal(name,Mode)==-1)
	{
		fputs("Error reading signal !\n\r",stderr);
		return;
	}
	
	if(SymMode==ON)
	{
		const int Offset=2*DimBase;
		register int i;
		
		for(i=0 ; i<DimBase ; i++)
			sygnal[i]=sygnal[Offset+i]=OrgSygnal[i];
	}    
	
	if(prn==ON)
		fprintf(stdout,"<<< SIGNAL LOADED (ASCII) >>>\n");
}

extern int FindChirp;

void SetMPP(char *opt)
{
	const int OldDimBase=DimBase,OldOverSampling=OverSampling;
	char *argv[MAXOPTIONS];
	int argc,opcja,NewDimBase=DimBase,NewOverSampling=OverSampling;
	extern void ReInitBaseSize(int,int);
	
	StrToArgv(opt,argv,&argc);			/* Konwersja na postac argv */
	opterr=optind=0; sp=1;
	while((opcja=Getopt(argc,argv,"C:M:E:A:B:O:P:fI:h:o:F:p:N:TS:D:Z:"))!=EOF)
	{
		switch(opcja)
		{
		case 'S':
			NewOverSampling=atoi(optarg);
		case 'T':
			MallatDiction=ON;
			DiadicStructure=ON;
			break;
		case 'h':
			if(strcmp(optarg,"+")==0)
				Heuristic=ON;
			else if(strcmp(optarg,"-")==0)
				Heuristic=OFF;
			else 
				fprintf(stderr,"Bad option !\n");  
			break;    
		case 'o':
			ROctave=atoi(optarg);
			break;
		case 'f':
			RFreqency=atoi(optarg);
			break;
		case 'p':
			RPosition=atoi(optarg);
			break;
		case 'N': 
			StartDictionSize=atoi(optarg);
			Heuristic=ON;
			break;                                    
		case 'I':
			if(strcmp(optarg,"+")==0)
				prninfo=ON;
			else if(strcmp(optarg,"-")==0)
				prninfo=OFF;
			else 
				fprintf(stderr,"Bad option !\n");  
			break;         
		case 'F':
			SamplingRate=atof(optarg);
			break;
		case 'C':
			ConvRate=atof(optarg);
			break;
		case 'M':
			OldDimRoz=atoi(optarg);	/* Liczba wektorow w rozwiazaniu */
			break;
		case 'E':
			epsylon=(float)atof(optarg);
			break;
		case 'A':
			OldDimRoz+=atoi(optarg);	/* Zwiekszanie liczby o OPT */
			break;
		case 'B':
			(void)strcpy(outname,optarg);
			if(prn==ON)
				fprintf(stdout,"New \"book\" filename   : %s\n",outname);
			break;
		case 'O':
			NewDimBase=atoi(optarg);
			break;
		case 'D':
			AdaptiveConst=(float)atof(optarg);
			break;
		case 'P':
			if(strcmp(optarg,"on")==0 || strcmp(optarg,"+")==0)
				prn=ON;
			else if(strcmp(optarg,"off")==0 || strcmp(optarg,"-")==0)
				prn=OFF;
			else fprintf(stderr,"Incorrect parameter [on | off | + | -]\n");
			break;
		case 'Z':
			if(strcmp(optarg,"on")==0 || strcmp(optarg,"+")==0)
				FindChirp=ON;
			else if(strcmp(optarg,"off")==0 || strcmp(optarg,"-")==0)
				FindChirp=OFF;
			else fprintf(stderr,"Incorrect parameter [on | off | + | -]\n");
			break;
		default:
			fprintf(stderr,"Unknown option !\n");
			break;
		}
	}
	
	if(Heuristic==ON)			/* Poprawnosc konfiguracji */
	{
		if(StartDictionSize>=DictionSize)
		{
			Heuristic=OFF;
			fprintf(stderr,"Too big trial dictionary size: SIZE<%d !\n",
				DictionSize);
		}          
		else if(FastMode==OFF)
		{  
			Heuristic=OFF;
			fprintf(stderr,"Heuristic acceleration works only in FASTMODE !\n");
		}  
		else if(DiadicStructure==OFF)
		{
			Heuristic=OFF;  
			fprintf(stderr,"Heuristic accel. works only for \"dyadic octaves\" dictionaries !\n");
		}  
	}
	
	if(Heuristic==OFF)
		StartDictionSize=DictionSize;    
	
	if(OldDimBase!=NewDimBase || OldOverSampling!=NewOverSampling)
	{
		if(prn==ON)
			fprintf(stdout,"<<< CHANGE OF BASE DIMENSION >>>\n\n");
		ReInitBaseSize(NewDimBase,NewOverSampling);
	}
	
	if(prn==ON)
	{
		fprintf(stdout,"CURRENT SETTINGS:\n"
			"Reconstruction accuracy       %g %%\n"
			"Max number of iterations      %d\n"
			"Signal (base) size            %d\n"
			"Sampling frequency            %f Hz\n"
			"Calibration coeff. [p/uV]     %f 1/uV\n"
			"Number of Gabor atoms         %d\n"
			"Total number of atoms         %d\n"
			"Oversampling                  %d\n" 
			"Adaprive dictionary parameter %g\n"
			,epsylon,OldDimRoz,DimBase,
			SamplingRate,ConvRate,DictionSize,
			DictionSize+(3*DimBase)/2,OverSampling,AdaptiveConst);
		
		fprintf(stdout,"Information display           ");
		if(prn==ON)
			fprintf(stdout,"ON\n");
		else if(prn==OFF) fprintf(stdout,"OFF\n");
		
		fprintf(stdout,"Decomposition progress info   ");
		if(prninfo==ON)
			fprintf(stdout,"ON\n");
		else if(prninfo==OFF) fprintf(stdout,"OFF\n");
		
		fprintf(stdout,"Dyadic scales (2^j)           ");
		if(DiadicStructure==ON)
			fprintf(stdout,"ON\n");
		else if(DiadicStructure==OFF) fprintf(stdout,"OFF\n");
		
		fprintf(stdout,"Dyadic dictionary             ");
		if(MallatDiction==ON)
			fprintf(stdout,"ON\n");
		else if(MallatDiction==OFF) fprintf(stdout,"OFF\n");
		
		fprintf(stdout,"FASTMODE                      ");
		if(FastMode==ON)
			fprintf(stdout,"ON\n");
		else if(FastMode==OFF) fprintf(stdout,"OFF\n");
		
		fprintf(stdout,"Heuristic acceleration        ");
		if(Heuristic==ON)
		{
			fprintf(stdout,"ON\n");
			fprintf(stdout," Size of trial dictionary       %d\n"
				" Octave \"radius\"                %d\n"
				" Frequency --\"--                %d\n"
				" Translation --\"--              %d\n",
				StartDictionSize,ROctave,RFreqency,
				RPosition);
		}  
		else if(Heuristic==OFF) fprintf(stdout,"OFF\n");
		fprintf(stdout,"\n");
	}
	
	FreeArgv(argv,argc);
}

void PrintBook(void)	/* Wydruk parametrow atomow na ekran */
 {
   int i;

   if(Compute==OFF)
     {
       fprintf(stderr,"No decomposition computed !\n");
       return;
     }
        
   fprintf(stdout,"\t\t<<< CONTENT OF %s >>>\n",outname);
   fprintf(stdout,"   NUMBER  WEIGHT SCALE  TRANSL.   MODULATION PHASE   FACT\n");
   for(i=0 ; i<dimroz ; i++)
    {
      fprintf(stdout,"%7d  %6.4f %6.2f    %6.2f   %6.4f   %6.4f  %6.2f %c\n",
		      book[i].numer,book[i].waga,book[i].param[0],
		      book[i].param[1],book[i].param[2],book[i].phase,
		      100.0F*book[i].Energia/E0,'%');
      if(((i+1)%20)==0) pause();
    }
  fprintf(stdout,"\n"); fflush(stdout);
}

void TypeSignal(char *string)		/* Wyswietlenie informacji o sygnale */
 {
   int i;

   if(string!=NULL)
     fprintf(stdout,"%s\n",string);
   
   fprintf(stdout,"\t    <<< ORIGINAL SIGNAL, RESIDUUM >>>\n\n");
   for(i=0 ; i<DimBase ; i++)
     {
	fprintf(stdout,"\t%5d \t%12.8f \t%12.8f\n",i,OrgSygnal[i],sygnal[DimBase+i]);
	if(((i+1)%22)==0) pause();
     }
 }

void Reset(char *string)	 /* Zerowanie tablicy z sygnalem */
{
	const int trueDim=3*DimBase;
	register int i;
	
	if(string!=NULL && prn==1)
		fprintf(stdout,"<<< RESETTING SIGNAL %s >>>\n",string);
	
	for(i=0 ; i<trueDim ; i++)
		sygnal[i]=0.0F;
	
	for(i=0 ; i<DimBase ; i++)
		OrgSygnal[i]=0.0F;
}

static float Gabor(float s,float t, float freq, float phase, float crate,int x)
{
	float tmp,Amp, rt;
	
	if(s==0.0F)
		return (((float)x==t) ? ((phase>0.0) ? -1.0F : 1.0F) : 0.0F);
	else if(s==(float)DimBase)
		Amp=1.0F;
	else 
	{
		tmp=(((float)x)-t)/s;
		Amp=exp(-M_PI*SQR(tmp));
	}

	rt = ((float)x)-t;
	
	return Amp*cos((rt)*(freq + (rt)*crate)+phase);
}
  
/* Zamiana fazy w konwencji Mallata na faze w konwencji HMPP */

static double HmppPhase(double freq,double position,double phase)
{
  const double pi2=2.0*M_PI;
  double RawPhase=(phase<0.0) ? pi2+phase : phase;

  RawPhase+=freq*position;
  return RawPhase-pi2*floor(RawPhase/pi2);
}

/* Zamiana fazy w konwencji HMPP na faze w konwencji Mallata */

static double MppPhase(double freq,double position,double phase)
{
  const double RawPhase=phase-freq*position,pi2=2.0*M_PI,
               NewPhase=RawPhase-pi2*floor(RawPhase/pi2);
  
  return (NewPhase>=M_PI) ? NewPhase-pi2 : NewPhase;
}

void Rekonstrukcja(void)
{
	float sum,*Baza;
	int i,j;
	
	if(prn==ON)
		fprintf(stdout,"<<< RECONSTRUCTING SIGNAL FROM A \"BOOK\" >>>\n");
	
	if((Baza=MakeVector(DimBase))==NULL)
	{
		fputs("Not enough memory for base vector (LoadBook)\n\r",stderr);
		return;
	}
	
	for(i=0 ; i<DimBase ; i++)		/* reszta staje sie zerowa */
		sygnal[DimBase+i]=OrgSygnal[i]=0.0F;	/* zakladamy rzeczywisty==rekonstruowany */
	
	for(i=0 ; i<dimroz ; i++)
	{
		sum=0.0F;
		
		for(j=0 ; j<DimBase ; j++)
		{
			Baza[j]=Gabor(book[i].param[0],book[i].param[1],
				book[i].param[2],book[i].phase,book[i].param[3], j);
			
			sum+=SQR(Baza[j]);
		}
	
		sum=book[i].waga/(float)sqrt(sum);
		
		for(j=0 ; j<DimBase ; j++)
			OrgSygnal[j]+=sum*Baza[j];		/* rekonstrukcja "orginalu" */
	}
	
	FreeVector(Baza);
}

int LoadBookStatus;

void LoadBook(char *opt)			/* Zaladowanie ksiazki do analizy */
 {						/* dotyczy tylko modulow zewnetrznych */
   char *argv[MAXOPTIONS],filename[STRING];	/* bez algorytmu MP */
   int argc,opcja,offset=0,ok=0,TmpDimRoz,i,RekStatus=0;
   float df;
   FILE *plik;
   HEADER head;
   ATOM   atom;

   LoadBookStatus=Compute=OFF;
   StrToArgv(opt,argv,&argc);			/* Konwersja na postac argv */
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"O:#:L"))!=EOF)
     switch(opcja) {
       case '#':
	    offset=atoi(optarg);
	    break;
       case 'O':
	    (void)strcpy(filename,optarg);
	    ok=1;
	    break;
       case 'L':
	    RekStatus=1;
	    break;
      default:
	   fprintf(stderr,"Unknown option !\n");
	   FreeArgv(argv,argc);
	   return;
    }

  FreeArgv(argv,argc);
  if(ok==0)
   {
     fprintf(stderr,"\"Book\" filename not given\n");
     return;
   }

  if(prn==ON)
    fprintf(stdout,"<<< LOADING \"BOOK\" >>>\n");

  if((plik=fopen(filename,"rb"))==NULL)
   {
     fprintf(stderr,"Missing file %s !\n",filename);
     return;
   }

  if(SetActualBook(offset,plik)==-1)
   {
     fprintf(stderr,"Error setting position %d\n",offset);
     fclose(plik);
     return;
   }

   if(ReadHeader(&head,plik)==-1)
    {
      fprintf(stderr,"Cannot read header\n");
      fclose(plik);
      return;
    }

   TmpDimRoz=head.book_size;
   if(head.signal_size!=DimBase)
      fprintf(stderr,"Warning !! dimensions of signal (%d) and base (%d) differ\n"
              ,head.signal_size,DimBase);

   if(book!=NULL) 
     free((void *)book);

   if((book=(BOOK *)malloc((unsigned)(TmpDimRoz+1)*sizeof(BOOK)))==NULL)
    {
      fputs("Cannot allocate memory for \"book\"\n\r",stderr);
      return;
    }
    
   df=0.5F*(float)DimBase/(float)M_PI;
   Ec=0.0F;
   for(i=0 ; i<TmpDimRoz ; i++)          /* Lodowanie atomow */
    {
      if(ReadAtom(&atom,plik)==-1)
       {
	 fprintf(stderr,"Error reading atom !\n");
	 fclose(plik);
	 return;
       }

      book[i].numer=i;			/* Z uwagi na format pliku z ksiazka (konwersja int na short) */
      book[i].waga=atom.modulus;	/* po zaladowaniu piszemy tylko numery porzadkowe */
      book[i].phase=atom.phase;
      book[i].amplitude=atom.amplitude;
      book[i].param[0]=(float)(1U << atom.octave);
      book[i].param[1]=(float)atom.position;
      book[i].param[2]=(float)atom.frequency/df; /* Zapis ksiazek w konwencji Mallata */
      book[i].phase=HmppPhase(book[i].param[1],book[i].param[2],
			      book[i].phase);
      Ec+=(book[i].Energia=SQR(book[i].waga));
   }

  fclose(plik);
  dimroz=TmpDimRoz;			/* Nowy rozmiar rozwiazania */
  E0=head.signal_energy;		/* Energia sygnalu */

  if(RekStatus==1)			/* Rekonstrukcja wektora "Orginalnego" */
    Rekonstrukcja();
  LoadBookStatus=Compute=ON;
  if(prn==ON)
    fprintf(stdout,"<<< %d ATOMS LOADED >>>\n",dimroz);
 }
 
void LoadNewBook(char *opt) {	       /* Ladowanie nowej ksiazki */
  char *argv[MAXOPTIONS],filename[STRING];
  int argc,opcja,offset=0,ok=0,TmpDimRoz,i,RekStatus=0;
  float df;
  FILE *plik;
  FILE_HEADER file_header;
  SEG_HEADER  head;
  NEW_ATOM    atom;

  initField(&file_header);
  LoadBookStatus=Compute=OFF;
  StrToArgv(opt,argv,&argc);			
  opterr=optind=0; sp=1;
  while((opcja=Getopt(argc,argv,"O:#:L"))!=EOF)
    switch(opcja) {
       case '#':
	    offset=atoi(optarg);
	    break;
       case 'O':
	    (void)strcpy(filename,optarg);
	    ok=1;
	    break;
       case 'L':
	    RekStatus=1;
	    break;
      default:
	   fprintf(stderr,"Unknown option !\n");
	   FreeArgv(argv,argc);
	   freeAllFields(&file_header);
	   return;
    }

  FreeArgv(argv,argc);
  if(ok==0) {
    freeAllFields(&file_header);
    fprintf(stderr,"\"Book\" filename not given\n");
    return;
  }

  if(prn==ON)
    fprintf(stdout,"<<< LOADING A \"BOOK\" >>>\n");

  if((plik=fopen(filename,"rb"))==NULL) {
    freeAllFields(&file_header);
    fprintf(stderr,"Missing file %s !\n",filename);
    return;
  }

  if(checkBookVersion(plik)==-1) {
    freeAllFields(&file_header);
    fclose(plik);
    LoadBook(opt);
    return;
  }

  if(ReadFileHeader(&file_header,plik)==-1) {
    freeAllFields(&file_header);
    fprintf(stderr,"Cannot read header !\n");
    fclose(plik);
    return;
  }

  if(setBookPosition(offset,plik)==-1) {
    freeAllFields(&file_header);
    fprintf(stderr,"Error setting position %d\n",offset);
    fclose(plik);
    return;
  }

  if(prn==ON) {
     decomposition_info *dec_ptr=getDecompPtr(&file_header);
     signal_info *sig_ptr;
     char *ptr;

     fprintf(stdout,"<<< BOOK HEADER >>>\n\n");
     fprintf(stdout,"Energy percent      : %5.3f\n"
	            "Max iteration       : %d\n"
	            "Dictionary size     : %d\n"
	            "Dictionary type     : %c\n",
	     dec_ptr->energy_percent,
	     dec_ptr->max_number_of_iterations,
	     dec_ptr->dictionary_size,
	     dec_ptr->dictionary_type);

     sig_ptr=getSignalPtr(&file_header);
     fprintf(stdout,"Sampling freq.      : %f\n"
	            "Points per microvolt: %f\n"
	            "Number of channels  : %d\n",
	     sig_ptr->sampling_freq,
	     sig_ptr->points_per_microvolt,
	     sig_ptr->number_of_chanels_in_file);

     if((ptr=getTextPtr(&file_header))!=NULL) 
       fprintf(stdout,"Text                : %s\n",ptr);

     if((ptr=getDatePtr(&file_header))!=NULL) 
       fprintf(stdout,"Date                : %s\n",ptr);
  }

  freeAllFields(&file_header);
  if(ReadSegmentHeader(&head,plik)==-1) {
    fprintf(stderr,"Cannot read segment header !\n");
    fclose(plik);
    return;
  }

  if(prn==ON) {
    fprintf(stdout,"Channel             : %d\n",head.channel);
  }

  TmpDimRoz=head.book_size;
  if(head.signal_size!=DimBase)
    fprintf(stderr,"Warning! Dimensions of signa (%d) and base (%d) differ\n"
	    ,head.signal_size,DimBase);
  
  if(book!=NULL) 
    free((void *)book);

  if((book=(BOOK *)malloc((unsigned)(TmpDimRoz+1)*sizeof(BOOK)))==NULL) {
    fputs("Cannot allocate memory for \"book\"\n\r",stderr);
    fclose(plik);
    return;
  }
    
  df=0.5F*(float)DimBase/(float)M_PI;
  Ec=0.0F;
  for(i=0 ; i<TmpDimRoz ; i++) {
    if(ReadNewAtom(&atom,plik)==-1) {
      fprintf(stderr,"Error reading atom [%d] !\n",i);
      fclose(plik);
      return;
    }

    book[i].numer=i;
    book[i].waga=atom.modulus;	
    book[i].phase=atom.phase;
    book[i].amplitude=atom.amplitude;
    book[i].param[0]=atom.scale;
    book[i].param[1]=atom.position;
    book[i].param[2]=atom.frequency/df; 
    Ec+=(book[i].Energia=SQR(book[i].waga));
  }

  fclose(plik);
  dimroz=TmpDimRoz;			
  E0=head.signal_energy;		

  if(RekStatus==1)			
    Rekonstrukcja();
  LoadBookStatus=Compute=ON;
  if(prn==ON)
    fprintf(stdout,"<<< %d ATOMS LOADED >>>\n",dimroz);
}

void WriteAllSignal(char *opt)   /* Zapis calosci sygnalu do pliku */
 {
   char filename[STRING]="signal.asc",*argv[MAXOPTIONS];
   int i,opcja,argc,All=OFF,Size,orgsig=OFF;
   FILE *stream;

   StrToArgv(opt,argv,&argc);
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"O:ax"))!=EOF)
	switch(opcja) {
          case 'x':
                orgsig=ON;
                break;
          case 'O':
               (void)strcpy(filename,optarg);
               break;
          case 'a':
               All=ON;
               break;
          default:
		 fprintf(stderr,"Bad option !\n");
		 FreeArgv(argv,argc);
		 return;
	 }

     FreeArgv(argv,argc);
     if((stream=fopen(filename,"wt"))==NULL)
       {
         fprintf(stderr,"Cannot open file %s !\n",filename);
         return;
       }
       
    if(All==ON)
     {
      Size=3*DimBase;
      for(i=0 ; i<Size ; i++)
        fprintf(stream,"%d %g\n",i,sygnal[i]);
     }
   else if(orgsig==OFF)
     {
       Size=2*DimBase;
       for(i=DimBase ; i<Size ; i++)
         fprintf(stream,"%d %g\n",i-DimBase,sygnal[i]);
     }
   else 
    {
       for(i=0 ; i<DimBase ; i++)
         fprintf(stream,"%d %g\n",i,OrgSygnal[i]);
    }  
   fclose(stream);
 }                             

static double NormGabor(register int n,double alpha,double freq,double phase)
 {
   register int i;	/* Wyznaczenie odwrotnosci normy funkcji Gabora */
   const double Sin=sin(freq),Cos=cos(freq),CosPhase=cos(phase),
		SinPhase=sin(phase),ConstExp=exp(-alpha),ConstStep=SQR(ConstExp);
   double OldSin=0.0,OldCos=1.0,NewCos,dtmp,dtmp2,
	  OldExp=1.0,Factor=ConstExp,sum=SQR(CosPhase);

   for(i=0 ; i<n ; i++)
    {
	NewCos=OldCos*Cos-OldSin*Sin;		/* "Tablica" SIN i COS */
	OldSin=OldCos*Sin+OldSin*Cos;
	OldCos=NewCos;
	OldExp*=Factor;				/* Exp(-alpha*i^2) */
	Factor*=ConstStep;
	dtmp=OldExp*CosPhase*NewCos;
	dtmp2=OldExp*SinPhase*OldSin;
        sum+=SQR(dtmp-dtmp2)+SQR(dtmp+dtmp2);   /* Sumy kwadratow + symetrie */
    }
   return 1.0/sqrt(sum);			/* Odwrotnosc normy */
 }

static float MakeAmplitude(ATOM *atoms)
 {
   const double CONSTEPS=2.965674828188878;
   const int s=1U << atoms->octave,halfsignal=DimBase/2;
   double freq,newphase;  /* Wyznaczenie amplitudy atomu Gabora + COS + Dirac */
   int width;

   if(atoms->octave==0)            			   /* Dirac */
     return 1.0F;
   if((DimBase==s) && ((atoms->frequency==0) || 
		       (atoms->frequency==halfsignal)))
     return 1.0F/DimBase;      	   /* stala */
   if((atoms->frequency==0) || (atoms->frequency==halfsignal))
     return (float)(sqrt(M_SQRT2/(double)s));  	   /* Gauss */
   if(DimBase==s)		              	   /* cosinus */
     return (float)(2.0*M_SQRT2/sqrt(DimBase));

   width=(int)((double)s*CONSTEPS);
   if(width>halfsignal)
     width=halfsignal; 			  /* Amplituda pozostalych Gaborow */

   freq=2.0*M_PI*((double)atoms->frequency/(double)DimBase);
   newphase=HmppPhase(freq,atoms->position,atoms->phase);
   return NormGabor(width,M_PI/SQR((double)s),freq,newphase);
 }

static float MakeNewAmplitude(NEW_ATOM *atoms) {
  const double CONSTEPS=2.965674828188878;
  const int s=atoms->scale,halfsignal=DimBase/2;
  double freq,newphase;
  int width;

  if(s==0)
    return 1.0F;
  if((DimBase==s) && 
     ((atoms->frequency==0) || 
      (atoms->frequency==halfsignal)))
    return 1.0F/DimBase;      	 
  if((atoms->frequency==0) || (atoms->frequency==halfsignal))
    return (float)(sqrt(M_SQRT2/(double)s));
  if(DimBase==s)		            
    return (float)(2.0*M_SQRT2/sqrt(DimBase));

  width=(int)((double)s*CONSTEPS);
  if(width>halfsignal)
    width=halfsignal; 
  
  freq=2.0*M_PI*((double)atoms->frequency/(double)DimBase);
  newphase=HmppPhase(freq,atoms->position,atoms->phase);
  return NormGabor(width,M_PI/SQR((double)s),freq,newphase);
}

void SaveAllAtoms(char *opt)	/* Zapis ksiazki */
 {
   char *argv[MAXOPTIONS],name[STRING]="book.b",tryb[5]="wb";
   HEADER head;
   ATOM atom;
   FILE *plik;
   float df;
   int i,opcja,argc;

   if(Compute==OFF)
     {
       fprintf(stderr,"No decomposition computed !\n");
       return;
     }
        
   if(prn==ON)
     fprintf(stdout,"<<< SAVING \"BOOK\" of fitted atom's parameters  >>>\n");
   (void)strcpy(name,outname);
   StrToArgv(opt,argv,&argc);			/* Konwersja na postac argv */
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"S:A:sa"))!=EOF)
     switch(opcja) {
       case 'S':
		(void)strcpy(name,optarg);
       case 's':
		(void)strcpy(tryb,"wb");	/* Utworzenie nowego pliku */
		break;
       case 'A':
	       (void)strcpy(name,optarg);
       case 'a':
	       (void)strcpy(tryb,"a+b");
	       break;
       default:
	       fprintf(stderr,"Unknown option !\n");
	       FreeArgv(argv,argc);
	       return;
	}

   FreeArgv(argv,argc);
   if((plik=fopen(name,tryb))==NULL)
    {
      fprintf(stderr,"Cannot open file %s (%s)\n",name,tryb);
      return;
    }

   head.file_offset=(short)file_offset;
   head.book_size=(short)dimroz;
   head.signal_size=DimBase;
   head.signal_energy=E0;
   head.book_energy=Ec;
   head.points_per_micro_V=ConvRate;	     /* Wskaznik konwersji */
   head.FREQUENCY=SamplingRate;		     /* Modyfikacja zapisu ksiazki */

   if(WriteHeader(&head,plik)==-1)
     return;

   df=0.5F*DimBase/M_PI;
   for(i=0 ; i<dimroz ; i++)
    {
      atom.number_of_atom_in_book=(short)i; 
      atom.modulus=book[i].waga;
      if(book[i].param[0]==0.0F)
	atom.octave=0;
      else
	atom.octave=Log2(book[i].param[0]);
      atom.position=(short)(book[i].param[1]);
      atom.frequency=(short)(0.5F+df*book[i].param[2]);
      atom.phase=MppPhase(book[i].param[1],book[i].param[2],
			  book[i].phase); /* Zapis ksiazek w konwencji Mallata */
      atom.type=0;			  /* Dla slownika Gabora */
      if(book[i].amplitude<0.0)
	book[i].amplitude=MakeAmplitude(&atom);
      atom.amplitude=book[i].waga/(book[i].amplitude*ConvRate);

      if(WriteAtom(&atom,plik)==-1)
       {
	 fclose(plik);
	 return;
       }
    }
   fclose(plik);
   if(prn==ON)
     fprintf(stdout,"<<< FINISHED SAVING \"BOOK\" >>>\n");
 }

long FileSize(FILE *stream) {
  long curpos,length;

  curpos=ftell(stream);
  fseek(stream,0L,SEEK_END);
  length=ftell(stream);
  fseek(stream,curpos,SEEK_SET);
  return length;
}

void setInfo(char *opt) {
  if(strlen(opt)>STRING) 
    return;
  strcpy(infoHeader,opt);
  return;
}

void showInfo(char *opt) {
  if(prn==ON)
    fprintf(stdout,"%s %s\n",opt,infoHeader);
} 

void SaveAllNewAtoms(char *opt)	{
  char *argv[MAXOPTIONS],name[STRING]="book.b",tryb[5]="wb";
  FILE_HEADER file_header;
  SEG_HEADER  head;
  NEW_ATOM    atom;
  FILE *plik;
  float df;
  int i,opcja,argc,append=0,ok=0;
  decomposition_info dec_info;
  signal_info        sig_info;

  if(Compute==OFF) {
    fprintf(stderr,"No decomposition computed !\n");
    return;
  }
        
  if(prn==ON)
    fprintf(stdout,"<<< SAVING \"BOOK\" (parameters of fitted atoms) >>>\n");
  strcpy(name,outname);
  StrToArgv(opt,argv,&argc);			/* Konwersja na postac argv */
  opterr=optind=0; sp=1;
  while((opcja=Getopt(argc,argv,"S:A:sa"))!=EOF)
    switch(opcja) {
       case 'S':
		strcpy(name,optarg);
       case 's':
		strcpy(tryb,"wb");	/* Utworzenie nowego pliku */
		break;
       case 'A':
	       strcpy(name,optarg);
       case 'a':
	       strcpy(tryb,"a+b");
	       append=1;
	       break;
       default:
	       fprintf(stderr,"Unknown option !\n");
	       FreeArgv(argv,argc);
	       return;
    }
  
   FreeArgv(argv,argc);
   if((plik=fopen(name,tryb))==NULL) {
     fprintf(stderr,"Cannot open file %s (%s)\n",name,tryb);
     return;
   }

   if(append) {
     if(FileSize(plik)!=0L) {
       if(checkBookVersion(plik)==-1) {
	 fprintf(stderr,"Incompatible format !\n");
	 fclose(plik);
	 return;
       } 
     } else {
       ok=1;
     }
   } else {
     ok=1;
   }
   
   if(ok==1) {
     initField(&file_header);
     if(strlen(infoHeader)!=0)
       addTextInfo(&file_header,infoHeader);
     dec_info.energy_percent=epsylon;
     dec_info.max_number_of_iterations=OldDimRoz;
     dec_info.dictionary_size=StartDictionSize;
     dec_info.dictionary_type=(MallatDiction) ? 'M' 
       : ((DiadicStructure) ? 'D' : 'S');     
     addDecompInfo(&file_header,&dec_info);

     sig_info.sampling_freq=SamplingRate;
     sig_info.points_per_microvolt=ConvRate;
     sig_info.number_of_chanels_in_file=ChannelMaxNum;
     addSignalInfo(&file_header,&sig_info);
     addDate(&file_header);

     if(WriteFileHeader(&file_header,plik)==-1) {
       fprintf(stderr,"Cannot write file header !\n");
       freeAllFields(&file_header);
       return;
     }

     freeAllFields(&file_header);
   }

   head.channel=ChannelNumber;
   head.file_offset=file_offset;
   head.book_size=dimroz;
   head.signal_size=DimBase;
   head.signal_energy=E0;
   head.book_energy=Ec;
 
   if(WriteSegmentHeader(&head,plik)==-1) {
     fprintf(stderr,"Cannot write segment header !\n");
     return;
   }

   df=0.5F*DimBase/M_PI;
   for(i=0 ; i<dimroz ; i++) {
     atom.modulus=book[i].waga;
     atom.scale=(int)(0.5+book[i].param[0]);
     atom.position=(int)(0.5+book[i].param[1]);
     atom.frequency=df*book[i].param[2];
     atom.phase=book[i].phase; 

     if(book[i].amplitude<0.0)
       book[i].amplitude=MakeNewAmplitude(&atom);
     atom.amplitude=book[i].waga/(book[i].amplitude*ConvRate);

     if(WriteNewAtom(&atom,plik)==-1) {
       fprintf(stderr,"Cannot save an atom !\n");
       fclose(plik);
       return;
     }
   }
   
   fclose(plik);
   if(prn==ON)
     fprintf(stdout,"<<< FINISHED SAVING ATOMS >>>\n");
}

int CzyJestTakiPlik(char *name)   	/* Sprawdzenie obecnosci pliku */
 {
   FILE *plik;

   if((plik=fopen(name,"rb"))==NULL)
     {
	fprintf(stderr,"Cannot open file %s\n",name);
	return -1;
     }
   fclose(plik);
   return 0;
 }

void RestartDiction(char *opt)
 {
   char *argv[MAXOPTIONS];
   int opcja,argc;
   extern int MaxScale;
   
   StrToArgv(opt,argv,&argc);			/* Konwersja na postac argv */
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"R:i:d:"))!=EOF)
    switch(opcja) {
       case 'i':
          if(strcmp(optarg,"+")==0)
            SRAND((unsigned short)time(NULL));
          else if(strcmp(optarg,"-")==0)
                  SRAND(0);
          break;
       case 'R':
          DictionSize=atoi(optarg);
          break;
       case 'd':
          if(strcmp(optarg,"+")==0)
           {
             DiadicStructure=ON;
             MaxScale=Log2(DimBase);
	     InicRandom1D();
           }  
          else if(strcmp(optarg,"-")==0)
	    {
	      DiadicStructure=OFF;
	      MaxScale=DimBase-1;
	      InicRandom1D();
	    }
          break;
       default:
          fprintf(stderr,"Unknown option !\n");
          FreeArgv(argv,argc);
	  return;
	}
   if(prn==ON)
     SetMPP("");
   FreeArgv(argv,argc);
 }
 
void AscSaveAllAtoms(char *opt)
{
	char *argv[MAXOPTIONS],tryb[5],name[STRING]="book.asc";
	int opcja,argc,i;
	FILE *plik;
	
	if(Compute==OFF)
	{
		fprintf(stderr,"No decomposition computed !\n");
		return;
	}
	
	if(prn==ON)
		fprintf(stdout,"<<< SAVING ATOMS IN TEXT MODE >>>\n");
	(void)strcpy(name,outname);
	StrToArgv(opt,argv,&argc);			/* Konwersja na postac argv */
	opterr=optind=0; sp=1;
	while((opcja=Getopt(argc,argv,"S:A:sa"))!=EOF)
		switch(opcja) {
       case 'S':
		   (void)strcpy(name,optarg);
       case 's':
		   (void)strcpy(tryb,"wt");	/* Utworzenie nowego pliku */
		   break;
       case 'A':
		   (void)strcpy(name,optarg);
       case 'a':
		   (void)strcpy(tryb,"a+t");
		   break;
       default:
		   fprintf(stderr,"Unknown option !\n");
		   FreeArgv(argv,argc);
		   return;
	}
	
	FreeArgv(argv,argc);
	if((plik=fopen(name,tryb))==NULL)
    {
		fprintf(stderr,"Cannot open file %s (%s)\n",name,tryb);
		return;
    }
    
	// GD: em fase de testes!
	/*for(i=0 ; i<dimroz ; i++)
		fprintf(plik,"%g %g %g %g %g\n",book[i].waga,book[i].param[0],
		book[i].param[1],book[i].param[2],book[i].phase);*/

	for(i=0 ; i<dimroz ; i++)
		fprintf(plik,"%g %g %g %g %g %g\n",book[i].waga,book[i].param[0],
		book[i].param[1],book[i].param[2],book[i].phase, book[i].param[3]);

	fclose(plik);
	if(prn==ON)
		fprintf(stdout,"<<< FINISHED SAVING ATOMS IN TEXT MODE >>>\n");
}
            
void AscLoadBook(char *opt)			/* Zaladowanie ksiazki do analizy */
 {						/* dotyczy tylko modulow zewnetrznych */
   char *argv[MAXOPTIONS],filename[STRING];	/* bez algorytmu MP */
   int argc,opcja,ok=0,TmpDimRoz,i,RekStatus=0;
   FILE *plik;

   Compute=OFF;
   StrToArgv(opt,argv,&argc);			/* Konwersja na postac argv */
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"O:L"))!=EOF)
     switch(opcja) {
       case 'O':
	    (void)strcpy(filename,optarg);
	    ok=1;
	    break;
       case 'L':
	    RekStatus=1;
	    break;
      default:
	   fprintf(stderr,"Unknown option !\n");
	   FreeArgv(argv,argc);
	   return;
    }

  FreeArgv(argv,argc);
  if(ok==0)
   {
     fprintf(stderr,"Filename of \"book\" not given\n");
     return;
   }

  if(prn==ON)
    fprintf(stdout,"<<< LOADING A BOOK IN TEXT MODE >>>\n");

  if((plik=fopen(filename,"rt"))==NULL)
   {
     fprintf(stderr,"Missing file %s !\n",filename);
     return;
   }
    
   TmpDimRoz=0;
   while(!feof(plik))
     {
       fscanf(plik,"%*f %*f %*f %*f %*f\n"); 
       TmpDimRoz++;
     }
  
  rewind(plik);   
  if(book!=NULL) free((void *)book);
   if((book=(BOOK *)malloc((unsigned)(TmpDimRoz+1)*sizeof(BOOK)))==NULL)
    {
      fputs("Cannot allocate memory for book\n\r",stderr);
      fclose(plik);
      return;
    }

   E0=0.0;
   for(i=0 ; i<TmpDimRoz ; i++)          /* Lodowanie atomow */
    {
      double tmp;
      
      book[i].numer=i;			
      fscanf(plik,"%f %f %f %f %f\n",&book[i].waga,	
             &book[i].param[0],&book[i].param[1],
             &book[i].param[2],&book[i].phase);
      
      book[i].amplitude=-1.0F;
      book[i].Energia=tmp=SQR(book[i].waga);
      E0+=tmp;
   }

  Ec=E0;                                /* W tym przypadku nie mamy informacji o Ec */
  fclose(plik);
  dimroz=TmpDimRoz;			/* Nowy rozmiar rozwiazania */
  if(RekStatus==1)			/* Rekonstrukcja wektora "Orginalnego" */
    Rekonstrukcja();
    	
  Compute=ON;
  if(prn==ON)
    fprintf(stdout,"<<< %d ATOMS LOADED IN TEXT MODE >>>\n",dimroz);
 }

#define FAST 1
#define SLOW 0
#define RES  2
#define MULT 3
#define ONE  4
#define TOL  1.0e-7F		        /* Dokladnosc  */

void SigReconst(char *opt)		/* Rekonstrukcja sygnalu */
{
	float *rek,sum,*Baza;
	FILE *plik;
	int i,j,opcja,argc,sposob,numerek=0;
	char *argv[MAXOPTIONS],filename[STRING];
	
	if(Compute==OFF)
	{
		fprintf(stderr,"No decomposition computed\n");
		return;
	}
	
	StrToArgv(opt,argv,&argc);			/* Konwersja na postac argv */
	opterr=optind=0; sp=1;
	sposob=SLOW;

	while((opcja=Getopt(argc,argv,"O:FSRMm:"))!=EOF)
	{
		switch(opcja)
		{
		case 'm':
			sposob=ONE;
			numerek=atoi(optarg);
			break;
		case 'O':
			(void)strcpy(filename,optarg);
			break;
		case 'F':
			sposob=FAST;			/* Szybka rekonstrukcja */
			break;
		case 'S':
			sposob=SLOW;			/* Z pelnymi obliczeniami */
			break;
		case 'R':
			sposob=RES;			/* Tylko reszta */
			break;
		case 'M':
			sposob=MULT;
			break;
		default:
			fputs("Unknown option !\r\n",stderr);
			FreeArgv(argv,argc);
			return;
		}
	}
	
	FreeArgv(argv,argc);
	
	if((plik=fopen(filename,"wt"))==NULL)
	{
		fputs("Cannot open file (Reconstruction) !\n\r",stderr);
		return;
	}
	
	if(sposob==SLOW || sposob==MULT)
	{
		if((rek=MakeVector(DimBase))==NULL)
		{
			fputs("Not enough memory for reconstruction vector\n\r",stderr);
			return;
		}
		
		if((Baza=MakeVector(DimBase))==NULL)
		{
			fputs("Not enough memory for base vector (Reconstruction)\n\r",stderr);
			return;
		}
		
		if(sposob==SLOW)
		{
			if(prn==ON)
				fprintf(stdout,"<<< SIGNAL RECONSTRUCTION FROM NON-ORTHOGONAL BASE >>>\n");
			for(i=0 ; i<DimBase ; i++)
				rek[i]=0.0F;
		}
		
		for(i=0 ; i<dimroz ; i++)
		{
			if(prn==ON)
			{
				fprintf(stdout,"->%5d\r",i);
				fflush(stdout);
			}
			
			sum=0.0F;
			for(j=0 ; j<DimBase ; j++)
			{
				Baza[j]=Gabor(book[i].param[0],book[i].param[1],
					book[i].param[2],book[i].phase,book[i].param[3],j);
				sum+=SQR(Baza[j]);
			}
			
			sum=book[i].waga/((sum<=TOL) ? 1.0F : sqrt(sum));
			for(j=0 ; j<DimBase ; j++)
			{
				Baza[j]*=sum;
				if(sposob==SLOW)
					rek[j]+=Baza[j];
			}
			
			if(sposob==MULT)		/* Generacja poszczegolnych atomow */
			{
				fprintf(plik,"#\n#  SCALE= %g TRANS= %g FREQ= %g PHASE= %g MODULUS= %g\n#\n",
					book[i].param[0],book[i].param[1],
					book[i].param[2],book[i].phase,book[i].waga);
				
				for(j=0 ; j<DimBase ; j++)
					fprintf(plik,"%d %g\n",j,Baza[j]);
			}
		}
		
		if(prn==ON)
			fprintf(stdout,"\n");
		
		if(sposob==SLOW)
		{
			for(i=0 ; i<DimBase ; i++)
			{
				fprintf(plik,"%d %g\n",i,rek[i]);
			}
		}
			
		FreeVector(rek); 
		FreeVector(Baza);
	}
	else if(sposob==FAST)
	{
		if(prn==ON)
			fprintf(stdout,"<<< FAST RECONSTRUCTION OF SIGNAL >>>\n");
		
		for(i=0 ; i<DimBase ; i++)
			fprintf(plik,"%d %g\n",i,OrgSygnal[i]-sygnal[DimBase+i]);
	}
	else if(sposob==RES)
	{
		if(prn==ON)
			fprintf(stdout,"<<< SAVING RESIDUAL SINAL >>>\n");
		
		for(i=0 ; i<DimBase ; i++)
			fprintf(plik,"%d %g\n",i,sygnal[DimBase+i]);
	}
	else if(sposob==ONE)
	{
		if(prn==ON)
			fprintf(stdout,"<<< CREATING ATOM NR %d >>>\n",numerek);
		
		if(numerek<0 || numerek>=dimroz)
		{
			fprintf(stderr,"Bad number range !\n");
			return;
		}
		
		if((Baza=MakeVector(DimBase))==NULL)
		{
			fputs("Not enough memory for base vector (Reconstruction)\n\r",stderr);
			return;
		}
		
		sum=0.0F;
		for(j=0 ; j<DimBase ; j++)
		{
			Baza[j]=Gabor(book[numerek].param[0],book[numerek].param[1],
				book[numerek].param[2],book[numerek].phase,book[i].param[3],j);
			sum+=SQR(Baza[j]);
		}
		
		sum=book[numerek].waga/(float)sqrt(sum);
		
		for(i=0 ; i<DimBase ; i++)
			fprintf(plik,"%d %g\n",i,Baza[i]*sum);
	}
	
	fclose(plik);
}
                                
void TranslateAtoms(int trans)       /* Translacja atomu (na potrzeby frott) */
 {
    const float Trans=(float)trans;
    register int i;
    
    if(book!=NULL)
      for(i=0 ; i<dimroz ; i++)
        book[i].param[1]+=Trans;
 }       

void TuneOctave(char *opt)          /* Ustawienie dystrybuanty dla skal */
 {
   char *argv[MAXOPTIONS];
   int argc,opcja,i,octave;
   float value;
   FILE *file;
  
   if(OctaveDyst==NULL)
     return;
   StrToArgv(opt,argv,&argc);	    /* Konwersja na postac argv */
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"o:f:"))!=EOF)
     switch(opcja) { 
       case 'f':
                if((file=fopen(optarg,"rt"))==NULL)
                  fprintf(stderr,"Missing file %s !\n",optarg);
                else
                 {
                  for(i=0 ; i<DimBase ; i++)
                   {
                     if(feof(file))
                       break;
                     fscanf(file,"%d %f\n",&octave,&value);
                     if(octave>=0 && octave<DimBase)
                       OctaveDyst[octave]=value;
                   }                 
                  fclose(file);
                 }  
               break;
        case 'o':
               if(sscanf(optarg,"%d,%f",&octave,&value)==2)
                 if(octave>=0 && octave<DimBase)
                   OctaveDyst[octave]=value;
               break;
                        
        default:
		fputs("Unknown option !\r\n",stderr);
		FreeArgv(argv,argc);
		return;
    }
   
   FreeArgv(argv,argc); 
   InicRandom1D();    
 }
 
void ResetDyst(void)       /* Resetowanie dytrybuanty */
{
	register int i;
	
	if(OctaveDyst!=NULL)
    {
		for(i = 0; i < DimBase; i++)
			OctaveDyst[i]=1.0F;

		InicRandom1D();   
    }  
} 
 
void ShowDyst(char *string)     /* Wyswietlenie informacje o rozkladzie skal */
 {
   int i;
   
   if(OctaveDyst!=NULL && prn==ON) {
     if(string!=NULL)
       fprintf(stdout,"%s\n",string);
     for(i=0 ; i<DimBase ; i++)
      {
        fprintf(stdout,"[%3d] %f\n",i,OctaveDyst[i]);
        if((i+1) % 23==0)
          pause();
      }
   }
 }         
      
void SetTuneScale(void)  /* Inicjacja tablic */
{
	if((OctaveDyst=(float *)malloc(DimBase*sizeof(float)))==NULL)
    {
		fprintf(stderr,"Problemy z alokacja pamieci pod dystrybuante !\n");
		exit(EXIT_FAILURE);
    }
	
	ResetDyst(); 
}  
 
void CloseTune(void)   /* Usuwanie tablic */
 {
   if(OctaveDyst!=NULL)
     {
       free((void *)OctaveDyst);
       OctaveDyst=NULL;
     }
 }

void Norm(char *string)      /* Normalizacja sygnalu */
 {
   register int i;                 
   double sum=0.0,var=0.0;

   for(i=0 ; i<DimBase ; i++)
     sum+=OrgSygnal[i];
   sum/=(double)DimBase;  

   for(i=0 ; i<DimBase ; i++)
     OrgSygnal[i]-=sum;  

   for(i=0 ; i<DimBase ; i++)
     var+=SQR(OrgSygnal[i]);

   if(fabs(var)>=1.0e-8) {
     var=sqrt(var/DimBase);
     for(i=0 ; i<DimBase ; i++)
       OrgSygnal[i]/=var;
   }

   if(prn==ON) {
     fprintf(stdout,"Average : %g\nVariance : %g\n",sum,var);     
     if(string!=NULL)
       fprintf(stdout,"%s\n",string);
   }
 }    
 
void Echo(char *string)   /* Komunikat na ekran (dla polecen skryptowych) */
 {
   fprintf(stdout,"%s\n",string);
   return;
 }

char *trim(char *string)
{
	register int i;
	
	for(i=strlen(string)-1 ; i>=0 ; i--)
	{
		if(!isspace(string[i]))
		{
			string[i+1]='\0';
			break;
		}

		return string;
	}
}

#define AUTO     0
#define JDK      1
#define NETSCAPE 2
#define JRE      3

static char newpath[1024];
static int  mp_home_no_set;

static int appendEnv(void) {  
  char *ptr;

  strcpy(newpath,"CLASSPATH=");
  if((ptr=getenv("MP_HOME"))!=NULL)
    strcat(newpath,ptr);
  else { 
    fprintf(stdout,"MP_HOME not set !\n");
    mp_home_no_set=1;
    return -1;
  }

#ifdef UNIXPATH
  strcat(newpath,"/");
#else
  strcat(newpath,"\\");
#endif

  strcat(newpath,"TimeFreq.jar");
  if((ptr=getenv("CLASSPATH"))!=NULL) {
#ifdef UNIXPATH
    strcat(newpath,":");
#else
    strcat(newpath,";");
#endif
    strcat(newpath,ptr);
  }

  if(prn==1) 
    fprintf(stdout,"set %s\n",newpath);

  if(putenv(newpath)==-1) 
    fprintf(stderr,"CLASSPATH not set !\n");
  return 0;
}

static int isfile(const char *filename) {
  FILE *file;
  if((file=fopen(filename,"rb"))==NULL)
    return -1;
  fclose(file);
  return 0;
}

static int copyJarFlag=0;

static int CopyJar(const char *path,const char *name) {
   static char filename[2*STRING];

   if(isfile(name)==0) {
       copyJarFlag=0;
       return 0;
   }

#ifndef __MSDOS__
   sprintf(filename,"cp %s",path);
   if(filename[strlen(filename)-1]!='/')
      strcat(filename,"/");
#else
   sprintf(filename,"copy %s",path);
   if(filename[strlen(filename)-1]!='\\')
      strcat(filename,"\\");
#endif
   strcat(filename,name);
   strcat(filename," .");
   if(system(filename)!=0)
      return -1;
   copyJarFlag=1;
   return 0;
}
  
static int appendHtml(void) {
  FILE *file;
  char *ptr;

  if((ptr=getenv("MP_HOME"))==NULL) {
    fprintf(stderr,"MP_HOME not set !\n");
    mp_home_no_set=1;
    return -1;
  }

  if(CopyJar(ptr,"TimeFreq.jar")==-1) {
     fprintf(stderr,"TimeFreq.jar not found in %s !\n",ptr);
     return 1;
  }

  if((file=fopen("applet.html","wt"))==NULL) {
    fprintf(stderr,"Can't open file: applet.html\n");
    return -1;
  }

  fprintf(file,"<HTML>\n"
	  "<BODY>\n"
	  "<APPLET CODE=\"TimeFrequencyApplet.class\" "
          "ARCHIVE=\"TimeFreq.jar\" width=720 height=450>\n"
	  "<PARAM NAME=book VALUE=jbook.b>\n"
	  "</APPLET>\n"
	  "</BODY>\n"
          "</HTML>\n");
  
  fclose(file);
  return 0;
}

static int RunJavaApplet(char *opt) {
  static char *argv[MAXOPTIONS],buffor[5*STRING],data_path[2*STRING];
  int argc,opcja,mode=AUTO,code,ok=0;

  if(getcwd(data_path,sizeof(data_path)-1)==NULL)
    strcpy(data_path,".");

#ifdef UNIXPATH
  strcat(data_path,"/");
#else
  strcat(data_path,"\\");
#endif

  StrToArgv(opt,argv,&argc);	    /* Konwersja na postac argv */
  opterr=optind=0; sp=1;
  while((opcja=Getopt(argc,argv,"v:"))!=EOF)
    switch(opcja) { 
    case 'v':
      if(strcmp(optarg,"netscape")==0)
	mode=NETSCAPE;
      else if(strcmp(optarg,"jdk")==0)
	mode=JDK;
      else if(strcmp(optarg,"jre")==0)
	mode=JRE;
      break;
    default:
      fprintf(stderr,"unknown option\n");
      FreeArgv(argv,argc); 
      return -1;
    }

   FreeArgv(argv,argc); 
   switch(mode) {
   case NETSCAPE:
     if(appendHtml()==-1)
       return -1;
#ifndef __MSDOS__
     sprintf(buffor,"netscape file:\\%sapplet.html &",data_path);
#else
     sprintf(buffor,"netscape file:\\%sapplet.html",data_path);
#endif
     break;
   case JDK:
     if(appendEnv()==-1)
       return -1;
#ifdef __MSDOS__
     sprintf(buffor,"java -cp %s TimeFrequencyApplet %sjbook.b",
             newpath+10,data_path);
#else
     sprintf(buffor,"java TimeFrequencyApplet %sjbook.b",data_path);
#endif
     break;
   case JRE:
     if(appendEnv()==-1)
       return -1;
#ifdef __MSDOS__
     sprintf(buffor,"jre -cp %s TimeFrequencyApplet %sjbook.b",
             newpath+10,data_path);
#else
     sprintf(buffor,"jre TimeFrequencyApplet %sjbook.b",data_path); 
#endif
     break;
   }

#ifndef __MSDOS__ 
   strcat(buffor," > /dev/null");
#else
   strcat(buffor," > nul");
#endif

   SaveAllNewAtoms("-S jbook.b");
   fprintf(stdout,"SYSTEM: %s\n",buffor);
   if((code=system(buffor))!=0) {
     fprintf(stderr,"System code: %d [%s]\n",code,buffor);
     ok=-1;
   }

   if(mode!=NETSCAPE) {
#ifndef __MSDOS__
     sprintf(buffor,"rm -f jbook.b %s",copyJarFlag ? "TimeFreq.jar" : "" );
#else
     sprintf(buffor,"del jbook.b %s",copyJarFlag ? "TimeFreq.jar" : "" );
#endif
      system(buffor);
   }
   return ok;
}

#ifdef __MSDOS__
#define SEPARATOR ';'
#else
#define SEPARATOR ':'
#endif

static int fileInPath(const char *filename) {
  char *path=getenv("PATH"),*buff;
  int i,j,len,code=0;

  if(path==NULL)
    return -1;
  
  len=strlen(path);
  if((buff=(char *)malloc((len+2+strlen(filename))*sizeof(char)))==NULL)
    return -1;

  for(i=0,j=0 ; i<len ; i++) {
    buff[j++]=path[i]; 
    if(path[i]==SEPARATOR) {
      buff[j-1]='\0';
      if(j>=2) {
	if(buff[j-2]!='/')
	  strcat(buff,"/");
      }
      j=0; 
      strcat(buff,filename);
      if(isfile(buff)==0) {
	code=1;
	break;
      }
    } 
  }

  if(code!=1) {
    buff[j]='\0';
    if(j>=2) {
      if(buff[j-2]!='/')
	strcat(buff,"/");
    }

    strcat(buff,filename);
    if(isfile(buff)==0)
      code=1;
  }
  
  free((void *)buff);
  return code;
}

static int checkFileExits(void) {
    if(isfile("applet.html")==0 || isfile("jbook.b")==0) {
	char c;
	fprintf(stdout,"applet.html or jbook.b exits, overwrite (y/n) ? "); 
	fflush(stdout);
	fscanf(stdin,"%c",&c);
	if(c=='n' || c=='N')
	    return 1;

	remove("applet.html");
	remove("jbook.b");
    }
    return 0;
}

void viewJavaApplet(char *opt) {
  int  automode=ON,ok=0;
  char s1[64],s2[64];

  if(Compute==OFF) {
    fprintf(stderr,"No decomposition computed !\n");
    return;
  }

  if(checkFileExits()) {
      fprintf(stdout,"\nuser break\n");
      return;
  } 

  mp_home_no_set=0;
  switch(sscanf(opt,"%s %s",s1,s2)) {
  case 1: 
    if(strcmp(s1,"-vauto")==0) 
      automode=ON;  
    else 
      automode=OFF;
    break;
  case 2: 
    if(strcmp(s1,"-v")==0 && strcmp(s2,"auto")==0) 
      automode=ON; 
    else 
      automode=OFF;
    break;
  }
  
  if(automode==ON) {
    char *mode[]={ "-v jdk", "-v netscape", "-v jre" };
    char *progs[]=
#ifndef __MSDOS__
                 { "java", "netscape", "jre" };
#else
                 { "java.exe", "netscape.exe", "jre.exe" };
#endif
    int i;

    for(i=0 ; i<3 ; i++) {
      fprintf(stdout,"AUTO: %s\n",mode[i]);
      if(!fileInPath(progs[i]))
	continue;
      if(RunJavaApplet(mode[i])==0) {
	ok=1;
	break;
      }
      if(mp_home_no_set==1)
	break;
    }
  } else { 
    if(RunJavaApplet(opt)==0) 
      ok=1;
  }

  if(ok==0) 
      fprintf(stderr,"can't find JAVA !\n");
}

void setChannelFunc(char *opt) {
  int chn;
  if(sscanf(opt,"%d",&chn)!=1)
    fprintf(stdout,"CHANNEL NUMBER: %d\n",ChannelNumber);
  else if(chn>=0) {
    ChannelNumber=chn;
    fprintf(stdout,"NEW CHANNEL NUMBER: %d\n",ChannelNumber);
  }
}




