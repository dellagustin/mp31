/* Odczyt sygnalu binarnego 1996 03 22/31 04/22,1996 09 29 
   1999 06 29 - zmiana numeracji kanalow
   1999 09 22 - poprawki
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"
#include "typdef.h"

#define MAXARGS    30
#define STRING     256
#define GETARG()   atoi(optarg)
#define TOTALCANAL 1
#define JEDNA      1			/* Elektrody odniesienia */
#define DWIE	   2
#define WCALE      0
#define ON	   1
#define OFF	   0

int setChannel=1;                       /* ustawianie numerow kanalow */

extern int ChannelMaxNum,ChannelNumber; 
extern int Getopt(int,char **,char *);  /* Pobranie opcji z hmpp */
extern char *optarg;                    /* Wskaznik dla tablicy opcji */
extern int opterr,optind,sp;            /* Zmienne pomocnicze Getopt */
extern int DimBase;			/* Wymiar bazy */
extern float *sygnal,*OrgSygnal;        /* Tablica z sygnalem do analizy */
extern int LoadStatus;			/* Wskaznik zaladowania zbioru */
extern char outname[],SSignalname[];	/* Nazwa zbioru z ksiazka i sygnalem */
extern int file_offset,prn;		/* Offset w pliku + wskaznik drukowania */

void ReadSignalBinary(char *opt)
 {
   char *argv[MAXARGS],filename[STRING]="";
   float ftmp=0.0F;
   FILE *plik;
   short *samp;
   int opcja,argc,offset=0,channel=0,total_channels=TOTALCANAL,
       shift=0,ref_1_num=-1,ref_2_num=-1,i,tryb,Mode=(int)DimBase;
   const int record_size=(int)DimBase;

   if(prn==ON)
     fprintf(stdout,"<<< LADOWANIE ZBIORU BINARNEGO >>>\n");

   LoadStatus=OFF;			/* Zakladamy ze moze byc bald */
   StrToArgv(opt,argv,&argc);
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"O:#:c:h:s:e:f:RLC"))!=EOF)
    switch(opcja) {
      case 'R':
		Mode=2U*DimBase;
		break;
      case 'L': 			/* Naturalne warunki brzegowe */
		Mode=0;
		break;
      case 'C':
		Mode=DimBase;
		break;
      case 'O':
	       (void)strcpy(filename,optarg);
	       (void)strcpy(SSignalname,filename);
	       break;
      case '#':
	       offset=GETARG();
	       break;
      case 'c':
	       channel=GETARG()-1;
	       break;
      case 'h':
	       total_channels=GETARG();
	       break;
      case 's':
	       shift=GETARG();
	       break;
      case 'e':
	       ref_1_num=GETARG()-1;
	       break;
      case 'f':
	       ref_2_num=GETARG()-1;
	       break;
      default:
	      fputs("Nieznana opcja !\n\r",stderr);
	      FreeArgv(argv,argc);
	      return;
     }

   FreeArgv(argv,argc);
   if(setChannel) {
     ChannelMaxNum=total_channels;
     ChannelNumber=channel+1;
   }

   if(ref_1_num!=-1 && ref_2_num==-1)
      tryb=JEDNA;
   else if(ref_1_num!=-1 && ref_2_num!=-1)
      tryb=DWIE;
   else if(ref_1_num==-1 && ref_2_num==-1)
      tryb=WCALE;
   else
     {
       fputs("Zle ustawiane elektrody odniesienia !\n\r",stderr);
       return;
     }

   if((plik=fopen(filename,"rb"))==NULL)
     {
       fprintf(stderr,"Brak pliku %s\n",filename);
       return;
     }

   if((samp=(short *)malloc((1+total_channels)*sizeof(short)))==NULL)
     {
       fputs("Brak pamieci do alokacji tablicy z sygnalem ReadSignalBinary !\n\r",
	     stderr);
       fclose(plik);
       return;
     }

   if(prn==ON)
    {
      if(Mode==0)
	fprintf(stdout,"<<< LEWY WARUNEK BRZEGOWY >>>\n");
      else if(Mode==(int)DimBase)
	     fprintf(stdout,"<<< LADOWANIE SYGNALU >>>\n");
      else fprintf(stdout,"<<< PRAWY WARUNEK BREZGOWY >>>\n");

      fprintf(stdout,"\n\toffset= %d\n\tchannel= %d\n\ttotal_channels= %d\n\tshift=%d\n"
		     "\trecord_size=%d\n\tref1=%d\n\tref2=%d\n",offset,channel,
		      total_channels,shift,record_size,ref_1_num,ref_2_num);
    }

    fseek(plik,sizeof(short)*offset*total_channels*record_size+shift,SEEK_SET);
    for(i=0; i<record_size; i++)
     {
       if(total_channels!=(int)fread((void *)samp,sizeof(short),total_channels,plik))
	 {
	   printf("Problems reading input %d\n",i);
	   free((void *)samp); fclose(plik);
	   return;
	}
#ifndef INTELSWP				/* Dla AIX'a (zapis w foramcie DOS'u) */
       swab((char *)samp,(char *)samp,sizeof(short)*total_channels);
#endif
       switch(tryb) {   		/* Z elektrodami odniesienia */
	 case JEDNA:
	     ftmp=(float)(samp[channel]-samp[ref_1_num]);
	     break;
	 case DWIE:
	     ftmp=(float)(samp[channel]-(samp[ref_1_num]+
			       samp[ref_2_num])/2);
	     break;
	 case WCALE:
	     ftmp=(float)(samp[channel]);
	     break;
       }

     sygnal[Mode+i]=ftmp;
     if(Mode==(int)DimBase)
	OrgSygnal[i]=ftmp;
    }

   fclose(plik);
   free((void *)samp);
   LoadStatus=ON;
   file_offset=offset;
   sprintf(outname,"%s.bok",filename);
   if(prn==ON)
     fprintf(stdout,"<<< ZBIOR ZALADOWANY (BIN) >>>\n");
 }

#ifndef INTELSWP

static float DosFloatToUNIX(UCHAR *buffor)
 {
    short isav;    /* Konwersja Dosowego float na UNIX */
    union {
	  char fc[4];
	  short fsi[2];
	  float ff;
      } gf[2];

    (void)memcpy((void *)&gf[0].ff,(void *)buffor,sizeof(float));
    swab(gf[0].fc,gf[1].fc,4);
    isav=gf[1].fsi[0]; gf[1].fsi[0]=gf[1].fsi[1]; gf[1].fsi[1]=isav;
    return gf[1].ff;
 }

#endif

void ReadFloatSignal(char *opt)	/* Odczyt w formacie float (KELLY) */
 {
   const int itmp=2*(int)DimBase;
   char *argv[MAXARGS],filename[STRING]="";
   int opcja,Ncanal=TOTALCANAL,argc,offset=0,cannal=0,
       Mode=(int)DimBase,i,wSym=0;
   ULONG pozycja,record_size,Skok;
   int ref_1_num=-1,ref_2_num=-1,tryb;
   long shift=0L;			/* Wzgledne przesuniecie w sygnale */
   FILE *plik;
   float *samp,ftmp;
   int total_channels=TOTALCANAL;
#ifndef INTELSWP
   register int j;
#endif

   LoadStatus=OFF;			/* Zakladamy ze moze byc bald */
   StrToArgv(opt,argv,&argc);
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"O:#:c:h:RLCs:Se:f:"))!=EOF)
    switch(opcja) {
      case 'e':
	       ref_1_num=GETARG()-1;
	       break;
      case 'f':
	       ref_2_num=GETARG()-1;
	       break;
      case 'S':
		wSym=1;
		break;
      case 'O':
		(void)strcpy(filename,optarg);
		break;
      case '#':
		offset=GETARG();
		break;
      case 'c':
		cannal=atoi(optarg)-1;
		break;
      case 'h':
		total_channels=Ncanal=GETARG();
		break;
      case 'R':
		Mode=2U*DimBase;
		break;
      case 'L':
		Mode=0;
		break;
      case 'C':
		Mode=DimBase;
		break;
      case 's':
	       shift=atol(optarg);
	       break;
      default:
	      fputs("Nieznana opcja !\n\r",stderr);
	      FreeArgv(argv,argc);
	      return;
     }

   FreeArgv(argv,argc);
   if(setChannel) {
     ChannelMaxNum=total_channels;
     ChannelNumber=cannal+1;
   }

   if(ref_1_num!=-1 && ref_2_num==-1)
     tryb=JEDNA;
   else if(ref_1_num!=-1 && ref_2_num!=-1)
     tryb=DWIE;
   else if(ref_1_num==-1 && ref_2_num==-1)
     tryb=WCALE;
   else {
     fputs("Zle ustawiane elektrody odniesienia !\n\r",stderr);
     return;
   }

   if((samp=(float *)malloc((1+total_channels)*sizeof(float)))==NULL) {
     fputs("Brak pamieci do alokacji tablicy z sygnalem ReadFloatSignal !\n\r",stderr);
     return;
   }

   if(prn==ON)
     fprintf(stdout,"<<< LADOWANIE ZBIORU BINARNEGO (FLOAT) >>>\n");

   if((plik=fopen(filename,"rb"))==NULL)
     {
       fprintf(stderr,"Brak pliku %s\n",filename);
       free((void *)samp);
       return;
     }

   if(prn==ON)
    {
      if(Mode==0)
	fprintf(stdout,"<<< LEWY WARUNEK BRZEGOWY >>>\n");
      else if(Mode==(int)DimBase)
	     fprintf(stdout,"<<< LADOWANIE SYGNALU >>>\n");
      else fprintf(stdout,"<<< PRAWY WARUNEK BREZGOWY >>>\n");
    }

   record_size=(ULONG)DimBase*(ULONG)Ncanal*(ULONG)sizeof(float);
   pozycja=(ULONG)offset*record_size+shift;
   Skok=(ULONG)Ncanal*(ULONG)sizeof(float);

   for(i=0 ; i<(int)DimBase ; i++) {
      if(fseek(plik,pozycja,SEEK_SET)!=0) {
	fputs("Bledna pozycja !\n\r",stderr);
	fclose(plik); free((void *)samp);
	return;
      }

      fread((void *)samp,sizeof(float)*total_channels,1,plik);
#ifndef INTELSWP
      for(j=0 ; j<total_channels ; j++)
	samp[j]=DosFloatToUNIX((char *)&samp[j]);
#endif

      switch(tryb) {   		/* Z elektrodami odniesienia */
      case JEDNA: 
	ftmp=samp[cannal]-samp[ref_1_num]; 
	break;
      case DWIE:
	ftmp=samp[cannal]-0.5F*(samp[ref_1_num]+samp[ref_2_num]);
	break;
      default:
	ftmp=samp[cannal];
	break;
      }

     sygnal[Mode+i]=ftmp;
     if(Mode==(int)DimBase)
       OrgSygnal[i]=sygnal[Mode+i];
     pozycja+=Skok;
    }

   if(wSym==1 && Mode==(int)DimBase)
    for(i=0 ; i<(int)DimBase ; i++) /* Asymetryczne warunki brzegowe */
      {
	sygnal[(int)DimBase-i-1]=-sygnal[i];
	sygnal[itmp+i]=-sygnal[itmp-i-1];
      }

   fclose(plik);
   LoadStatus=ON;
   file_offset=offset;
   sprintf(outname,"%s.bok",filename);
   if(prn==ON)
     fprintf(stdout,"<<< ZBIOR ZALADOWANY (BIN) >>>\n");
   free((void *)samp);
 }
