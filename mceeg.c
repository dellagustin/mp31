/* Rozklad pojedynczego kanalu EEG 1996 04 22 08/26 1996 09 21/29 1996 10 03
   Warunki brzegowe naturalne, rotacyjne, zerowe dla int i float */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "proto.h"

#define ON         1
#define OFF        0
#define STRING 	   256			/* Maksymalna dlugosc napisow */
#define MAXARGS    30			/* Maksymalna liczba argumentow */
#define MIN(X,Y) ((X)<(Y) ? (X) : (Y))
#define REINIC() if(ReinicRandomDic==ON) RestartDiction("-i+")

#define LOAD_SIGNAL(NR,GDZIE) if(prn==1) { prn=0; ok2=1; }		  \
			      Para[0]='\0'; 			          \
			      sprintf(Para,"%s-#%d "#GDZIE,Common,(NR));  \
			      ReadSignalBinary(Para); 			  \
			      if(ok2==1) { prn=1; ok2=0; }		  \
			      if(LoadStatus==0) { setChannel=1; return; }
		  /* W przypadku ladowania float'ow */
#define LOAD_FLOAT_SIGNAL(NR,GDZIE) if(prn==1) { prn=0; ok2=1; }		  \
			      Para[0]='\0'; 			          \
			      sprintf(Para,"%s-#%d "#GDZIE,Common,(NR));  \
			      ReadFloatSignal(Para); 			  \
			      if(ok2==1) { prn=1; ok2=0; }		  \
			      if(LoadStatus==0) { setChannel=1; return; }

extern long FileSize(FILE *);		/* Ustalenie rozmiaru pliku */
extern int Getopt(int,char **,char *);  /* Pobranie opcji z hmpp */
extern void StrToArgv(char *,char **,int *); /* Wydobywanie z napisu zestawu opcji */
extern void FreeArgv(char **,int);	/* Zwolnienie pamieci po argumetach */
extern int CzyJestTakiPlik(char *);	/* Sprawdzenie obecnosci pliku */
extern void ReadSignalBinary(char *);	/* Ladowanie pliku binarnego */
extern void ReadFloatSignal(char *);	/* Ladowanie pliku binarnego z float'ami */
extern void AscSaveAllAtoms(char *);    /* Zapis ksiazki w formacie tekstowym */
extern void TranslateAtoms(int);        /* Przesuniecie atomow (frott) */

extern char *optarg;                    /* Wskaznik dla tablicy opcji */
extern int opterr,optind,sp;            /* Zmienne pomocnicze Getopt */
extern int prn,LoadStatus;		/* Wskaznik drukowania informacji pomocniczych */
extern int file_offset;			/* Przemieszczenie w sygnale */
extern float *sygnal;			/* Tablica z sygnalem */
extern int setChannel;

static long FileSizeChar(char *name) {
  FILE *plik;				/* Rozmiar pliku po podaniu nazwy */
  long size;

  if((plik=fopen(name,"rb"))==NULL)
    return 0L;
  
  size=FileSize(plik);
  fclose(plik);
  return size;
}

void MakeCanalEEG(char *opt) /* Analiza EEG (niezalezne warunki brzegowe) */
 {
   static char Common[STRING],*argv[MAXARGS],filename[STRING],
	       Para[STRING],bookname[STRING],tmpstr[STRING];
   int argc,opcja,offset=0,ok=0,IleSegmentow,Nkan=1,i,
       ok2=0,maxseg=-1,k,ReinicRandomDic=OFF,ascii=OFF;
       
   Common[0]='\0'; (void)strcpy(bookname,"-a"); /* Domyslna nazwa ksiazki */
   StrToArgv(opt,argv,&argc);
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"O:#:c:h:s:e:f:I:M:Rt:"))!=EOF)
    switch(opcja) {	/* Podstawowe opcje sa powtarzane */
       case 'R':
                ReinicRandomDic=ON;
                break;
       case 'O':
		strcpy(filename,optarg);
		if(CzyJestTakiPlik(filename)==-1) {
		  FreeArgv(argv,argc);
		  return;
		}
		sprintf(tmpstr,"-O%s ",optarg);
		strcat(Common,tmpstr);
		ok=1;
		break;
       case 'c':
		sprintf(tmpstr,"-c%d ",(ChannelNumber=atoi(optarg)-1)+1);
		strcat(Common,tmpstr);
		break;
       case 'h':
		sprintf(tmpstr,"-h%d ",ChannelMaxNum=atoi(optarg));
		strcat(Common,tmpstr);
		Nkan=atoi(optarg);
		break;
       case 's':
		sprintf(tmpstr,"-s%s ",optarg);
		strcat(Common,tmpstr);
		break;
       case 'e':
		sprintf(tmpstr,"-e%d ",atoi(optarg));
		strcat(Common,tmpstr);
		break;
       case 'f':
		sprintf(tmpstr,"-f%d ",atoi(optarg));
		strcat(Common,tmpstr);
		break;
       case '#':
		offset=atoi(optarg);
		break;
       case 'I':			/* Zmiana nazwy ksiazki */
		sprintf(bookname,"-A %s",optarg);
		break;
       case 'M':
		maxseg=atoi(optarg);	/* Maksymalny numer segmentu */
		break;
       case 't':
                if(strcmp(optarg,"ascii")==0)
                  ascii=ON;
                else if(strcmp(optarg,"binary")==0)
                  ascii=OFF;
                else fprintf(stderr,"Opcja -t %s zignorowana !\n",optarg);  
                break;  
       default:
		fputs("Niezana opcja !\n\r",stderr);
		FreeArgv(argv,argc);
		return;
     }

  FreeArgv(argv,argc);
  if(ok==0)
    {
      fputs("Nie podano nazwy pliku !\n\r",stderr);
      return;
    }

  setChannel=0;
  IleSegmentow=(int)(FileSizeChar(filename)/
		    ((long)Nkan*(long)sizeof(short)*(long)DimBase));

  if(maxseg>0)
    IleSegmentow=((IleSegmentow<maxseg) ? IleSegmentow : maxseg);

  if(IleSegmentow<3)
   {
     fputs("Za malo segmentow do analizy (>3) !\n\r",stderr);
     return;
   }
				/* Utworzenie ksiazki z calego kanalu */
  if(prn==1)
    fprintf(stdout,"\t\t<<<     ANALIZA KANALU EEG     >>>\n"
		   "\t\t   (%d SEGMENTOW %d DO ANALIZY)\n",IleSegmentow,IleSegmentow-offset);
  if(prn==1)
    fprintf(stdout,"\n\t<<< ANALIZA 1 REKORDU >>>\n");

  Reset(NULL);			/* Domyslnie lewym warunkiem brzegowym beda 0 */
  LOAD_SIGNAL(offset,-C);	/* Lodowanie sygnalu */
  LOAD_SIGNAL(offset+1,-R);     /* Lodowanie prawego warunku brzegowego */
  REINIC();			/* Reinicjacja slownika */
  AnalizaMP(NULL);			/* Analiza sygnalu */
  file_offset=offset;		/* Ustawiamy offset pliku (ladowanie przekreca) */
 
  if(ascii==OFF)
    SaveAllNewAtoms(bookname);	/* Zapis ksiazki na dysku */
  else {
    TranslateAtoms(DimBase*offset);
    AscSaveAllAtoms(bookname);   
  }  

  for(i=offset+1,k=1 ; i<IleSegmentow-1 ; i++,k++)
   {
     if(prn==1)
       fprintf(stdout,"\t<<< ANALIZA %d REKORDU >>>\n",i+1);

     Reset(NULL);
     LOAD_SIGNAL(i-1,-L);
     LOAD_SIGNAL(i,-C);
     LOAD_SIGNAL(i+1,-R);
     REINIC();
     AnalizaMP(NULL);
     file_offset=i;		/* Ladowanie zmienia file_offset */
    
     if(ascii==OFF)
       SaveAllNewAtoms(bookname);	/* Zapis ksiazki na dysku */
     else {
       TranslateAtoms(DimBase*i);
       AscSaveAllAtoms(bookname);   
     }  
   }

  if(prn==1)
    fprintf(stdout,"\t<<< ANALIZA OSTATNIEGO REKORDU >>>\n");

  Reset(NULL);                  /* Domyslnie prawy warunek brzegowy jest 0 */
  LOAD_SIGNAL(IleSegmentow-2,-L);
  LOAD_SIGNAL(IleSegmentow-1,-C);
  REINIC();
  AnalizaMP(NULL);
  file_offset=i;
  
  if(ascii==OFF)
    SaveAllNewAtoms(bookname);	/* Zapis ksiazki na dysku */
  else {
    TranslateAtoms(DimBase*i);
    AscSaveAllAtoms(bookname);   
  }  

  setChannel=1;
  if(prn==1)
    fprintf(stdout,"<<< KONIEC ANALIZY KANALU EEG >>>\n");
 }

void RottCanalEEG(char *opt)	/* Obliczenia kanalu EEG z dynamicznymi warunkami brzegowymi */
 {
   static char Common[STRING],*argv[MAXARGS],filename[STRING],
	       Para[STRING],bookname[STRING],tmpstr[STRING];
   int argc,opcja,offset=0,ok=0,IleSegmentow,Nkan=1,i,ok2=0,maxseg=-1,
       k,j,itmp,itmp2,ReinicRandomDic=OFF,ascii=OFF;

   Common[0]='\0'; (void)strcpy(bookname,"-a"); /* Domyslna nazwa ksiazki */
   StrToArgv(opt,argv,&argc);
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"O:#:c:h:s:e:f:I:M:Rt:"))!=EOF)
    switch(opcja) {	/* Podstawowe opcje sa powtarzane */
       case 'R':
                ReinicRandomDic=ON;
                break;
       case 'O':
		(void)strcpy(filename,optarg);
		if(CzyJestTakiPlik(filename)==-1)
		 {
		   FreeArgv(argv,argc);
		   return;
		 }
		sprintf(tmpstr,"-O%s ",optarg);
		(void)strcat(Common,tmpstr);
		ok=1;
		break;
       case 'c':
		sprintf(tmpstr,"-c%d ",(ChannelNumber=atoi(optarg)-1)+1);
		(void)strcat(Common,tmpstr);
		break;
       case 'h':
		sprintf(tmpstr,"-h%d ",ChannelMaxNum=atoi(optarg));
		(void)strcat(Common,tmpstr);
		Nkan=atoi(optarg);
		break;
       case 's':
		sprintf(tmpstr,"-s%s ",optarg);
		(void)strcat(Common,tmpstr);
		break;
       case 'e':
		sprintf(tmpstr,"-e%d ",atoi(optarg));
		(void)strcat(Common,tmpstr);
		break;
       case 'f':
		sprintf(tmpstr,"-f%d ",atoi(optarg));
		(void)strcat(Common,tmpstr);
		break;
       case '#':
		offset=atoi(optarg);
		break;
       case 'I':			/* Zmiana nazwy ksiazki */
		sprintf(bookname,"-A %s",optarg);
		break;
       case 'M':
		maxseg=atoi(optarg);	/* Maksymalny numer segmentu */
		break;
       case 't':
                if(strcmp(optarg,"ascii")==0)
                  ascii=ON;
                else if(strcmp(optarg,"binary")==0)
                  ascii=OFF;
                else fprintf(stderr,"Opcja -t %s zignorowana !\n",optarg);  
                break;  
       default:
		fputs("Niezana opcja !\n\r",stderr);
		FreeArgv(argv,argc);
		return;
     }

  FreeArgv(argv,argc);
  if(ok==0)
    {
      fputs("Nie podano nazwy pliku !\n\r",stderr);
      return;
    }

  setChannel=0;
  IleSegmentow=(int)(FileSizeChar(filename)/
		    ((long)Nkan*(long)sizeof(short)*(long)DimBase));

  if(maxseg>0)
    IleSegmentow=MIN(IleSegmentow,maxseg);

  if(IleSegmentow<3)
   {
     fputs("Za malo segmentow do analizy (>3) !\n\r",stderr);
     return;
   }
				/* Utworzenie ksiazki z calego kanalu */
  if(prn==1)
    fprintf(stdout,"\t\t<<<     ANALIZA KANALU EEG (ROTT)    >>>\n"
		   "\t\t      (%d SEGMENTOW %d DO ANALIZY)\n",
		   IleSegmentow,IleSegmentow-offset);

  if(prn==1)
    fprintf(stdout,"\n\t<<< ANALIZA 1 REKORDU >>>\n");

				/* Zwyczajne obliczenia */
  Reset(NULL);			/* Domyslnie lewym warunkiem brzegowym beda 0 */
  LOAD_SIGNAL(offset,-C);	/* Lodowanie sygnalu */
  LOAD_SIGNAL(offset+1,-R);     /* Lodowanie prawego warunku brzegowego */
  REINIC();
  AnalizaMP(NULL);			/* Analiza sygnalu */
  file_offset=offset;		/* Ustawiamy offset pliku (ladowanie przekreca) */
 
  if(ascii==OFF)
    SaveAllNewAtoms(bookname);	/* Zapis ksiazki na dysku */
  else {
    TranslateAtoms(DimBase*offset);
    AscSaveAllAtoms(bookname);   
  }  

  for(i=offset+1,k=1 ; i<IleSegmentow-1 ; i++,k++)
   {
     if(prn==1)
       fprintf(stdout,"\t<<< ANALIZA %d REKORDU >>>\n",i+1);

     for(j=0 ; j<(int)DimBase; j++)	/* Przesuniecie ramki sygnalu po */
       {                                /* kanale (dynamiczny warunek brzegowy) */
	 itmp=(int)DimBase+j;
	 sygnal[j]=sygnal[itmp];
	 sygnal[itmp]=sygnal[(int)DimBase+itmp];
       }

     LOAD_SIGNAL(i,-C);
     REINIC();
     AnalizaMP(NULL);
     file_offset=i;		/* Ladowanie zmienia file_offset */
     
     if(ascii==OFF)
       SaveAllNewAtoms(bookname);	/* Zapis ksiazki na dysku */
     else {
       TranslateAtoms(DimBase*i);
       AscSaveAllAtoms(bookname);   
     }  
   }

  if(prn==1)
    fprintf(stdout,"\t<<< ANALIZA OSTATNIEGO REKORDU >>>\n");

  for(j=0 ; j<(int)DimBase; j++) /* Przesuniecie z ustawienie prawego */
   {				 /* warunku brzegowego */
     itmp=(int)DimBase+j; itmp2=itmp+(int)DimBase;
     sygnal[j]=sygnal[itmp];
     sygnal[itmp]=sygnal[itmp2];
     sygnal[itmp2]=0.0F;	/* Prawy warunek brzegowy jest rowny 0 */
   }

  REINIC();
  AnalizaMP(NULL);
  file_offset=i;
  
  if(ascii==OFF)
    SaveAllNewAtoms(bookname);	/* Zapis ksiazki na dysku */
  else {
    TranslateAtoms(DimBase*i);
    AscSaveAllAtoms(bookname);   
  }  

  setChannel=1;
  if(prn==1)
    fprintf(stdout,"<<< KONIEC ANALIZY KANALU EEG (ROTT) >>>\n");
 }

void FloatRottCanalEEG(char *opt) /* Obliczenia kanalu EEG z dynamicznymi warunkami brzegowymi */
 {
   static char Common[STRING],*argv[MAXARGS],filename[STRING],
	       Para[STRING],bookname[STRING],tmpstr[STRING];
   int argc,opcja,offset=0,ok=0,IleSegmentow,Nkan=1,i,ok2=0,maxseg=-1,
       k,j,itmp,itmp2,shift=0,ReinicRandomDic=OFF,ascii=OFF;

   Common[0]='\0'; (void)strcpy(bookname,"-a"); /* Domyslna nazwa ksiazki */
   StrToArgv(opt,argv,&argc);
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"O:#:c:h:I:M:s:Rt:"))!=EOF)
    switch(opcja) {	/* Podstawowe opcje sa powtarzane */
       case 'R':
               ReinicRandomDic=ON;
               break; 
       case 'O':
		(void)strcpy(filename,optarg);
		if(CzyJestTakiPlik(filename)==-1)
		 {
		   FreeArgv(argv,argc);
		   return;
		 }
		sprintf(tmpstr,"-O%s ",optarg);
		(void)strcat(Common,tmpstr);
		ok=1;
		break;
       case 'c':
		sprintf(tmpstr,"-c%d ",(ChannelNumber=atoi(optarg)-1)+1);
		(void)strcat(Common,tmpstr);
		break;
       case 's':
		sprintf(tmpstr,"-s%s ",optarg);
		(void)strcat(Common,tmpstr);
		shift=atoi(optarg);
		break;
       case 'h':
		sprintf(tmpstr,"-h%d ",ChannelMaxNum=atoi(optarg));
		(void)strcat(Common,tmpstr);
		Nkan=atoi(optarg);
		break;
       case '#':
		offset=atoi(optarg);
		break;
       case 'I':			/* Zmiana nazwy ksiazki */
		sprintf(bookname,"-A %s",optarg);
		break;
       case 'M':
		maxseg=atoi(optarg);	/* Maksymalny numer segmentu */
		break;
       case 't':
                if(strcmp(optarg,"ascii")==0)
                  ascii=ON;
                else if(strcmp(optarg,"binary")==0)
                  ascii=OFF;
                else fprintf(stderr,"Opcja -t %s zignorowana !\n",optarg);  
                break;  
       default:
		fputs("Niezana opcja !\n\r",stderr);
		FreeArgv(argv,argc);
		return;
     }

  FreeArgv(argv,argc);
  if(ok==0)
    {
      fputs("Nie podano nazwy pliku !\n\r",stderr);
      return;
    }
  setChannel=0;
		/* Okreslenie liczby segmentow (FLOAT) */
  IleSegmentow=(int)(FileSizeChar(filename)/
		    ((long)Nkan*(long)sizeof(float)*(long)(DimBase+shift)));

  if(maxseg>0)
    IleSegmentow=MIN(IleSegmentow,maxseg);

  if(IleSegmentow<3)
   {
     fputs("Za malo segmentow do analizy (>3) !\n\r",stderr);
     return;
   }
				/* Utworzenie ksiazki z calego kanalu */
  if(prn==1)
    fprintf(stdout,"\t\t<<<     ANALIZA KANALU EEG (FLOATROTT)    >>>\n"
		   "\t\t        (%d SEGMENTOW %d DO ANALIZY)\n",
		   IleSegmentow,IleSegmentow-offset);
  if(prn==1)
    fprintf(stdout,"\n\t<<< ANALIZA 1 REKORDU >>>\n");

  Reset(NULL);			/* Domyslnie lewym warunkiem brzegowym beda 0 */
  LOAD_FLOAT_SIGNAL(offset,-C);	/* Lodowanie sygnalu */
  LOAD_FLOAT_SIGNAL(offset+1,-R); /* Lodowanie prawego warunku brzegowego */
  REINIC();
  AnalizaMP(NULL);			/* Analiza sygnalu */
  file_offset=offset;		/* Ustawiamy offset pliku (ladowanie przekreca) */
  if(ascii==OFF)
     SaveAllNewAtoms(bookname);	/* Zapis ksiazki na dysku */
  else
   {
     TranslateAtoms(DimBase*offset);
     AscSaveAllAtoms(bookname);   
   }  

  for(i=offset+1,k=1 ; i<IleSegmentow-1 ; i++,k++)
   {
     if(prn==1)
       fprintf(stdout,"\t<<< ANALIZA %d REKORDU >>>\n",i+1);

     for(j=0 ; j<DimBase; j++)		/* Przesuniecie ramki sygnalu po */
       {                                /* kanale (dynamiczny warunek brzegowy) */
	 itmp=DimBase+j;
	 sygnal[j]=sygnal[itmp];
	 sygnal[itmp]=sygnal[DimBase+itmp];
       }

     LOAD_FLOAT_SIGNAL(i,-C);
     REINIC();
     AnalizaMP(NULL);
     file_offset=i;		/* Ladowanie zmienia file_offset */
     
     if(ascii==OFF)
       SaveAllNewAtoms(bookname);
     else {
       TranslateAtoms(DimBase*i);
       AscSaveAllAtoms(bookname);
     }     
   }

  if(prn==1)
    fprintf(stdout,"\t<<< ANALIZA OSTATNIEGO REKORDU >>>\n");

  for(j=0 ; j<DimBase; j++) /* Przesuniecie z ustawienie prawego */
   {				 /* warunku brzegowego */
     itmp=DimBase+j; itmp2=itmp+DimBase;
     sygnal[j]=sygnal[itmp];
     sygnal[itmp]=sygnal[itmp2];
     sygnal[itmp2]=0.0F;	/* Prawy warunek brzegowy jest rowny 0 */
   }

  REINIC();
  AnalizaMP(NULL);
  file_offset=IleSegmentow-1;
  if(ascii==OFF)
    SaveAllNewAtoms(bookname);
  else {
    TranslateAtoms(DimBase*(IleSegmentow-1));
    AscSaveAllAtoms(bookname); 
  }  
  
  setChannel=1;
  if(prn==1)
    fprintf(stdout,"<<< KONIEC ANALIZY KANALU EEG (FLOATROTT) >>>\n");
 }

void FloatCanalEEG(char *opt)	/* Analiza kanalu z symetrycznymi warunkami brzegowymi */
 {
   static char Common[STRING],*argv[MAXARGS],filename[STRING],
	       Para[STRING],bookname[STRING],tmpstr[STRING];
   int argc,opcja,offset=0,ok=0,IleSegmentow,Nkan=1,i,ok2=0,maxseg=-1,
       k,j,shift=0,ReinicRandomDic=OFF,ascii=OFF;

   Common[0]='\0'; strcpy(bookname,"-a"); /* Domyslna nazwa ksiazki */
   StrToArgv(opt,argv,&argc);
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"O:#:c:t:h:I:M:s:R"))!=EOF)
    switch(opcja) {
       case 'R':
               ReinicRandomDic=ON;
               break;  
       case 'O':
		(void)strcpy(filename,optarg);
		if(CzyJestTakiPlik(filename)==-1)
		 {
		   FreeArgv(argv,argc);
		   return;
		 }
		sprintf(tmpstr,"-O%s ",optarg);
		(void)strcat(Common,tmpstr);
		ok=1;
		break;
       case 'c':
		sprintf(tmpstr,"-c%d ",(ChannelNumber=atoi(optarg)-1)+1);
		(void)strcat(Common,tmpstr);
		break;
       case 's':
		sprintf(tmpstr,"-s%s ",optarg);
		(void)strcat(Common,tmpstr);
		shift=atoi(optarg);
		break;
       case 'h':
		sprintf(tmpstr,"-h%d ",ChannelMaxNum=atoi(optarg));
		(void)strcat(Common,tmpstr);
		Nkan=atoi(optarg);
		break;
       case '#':
		offset=atoi(optarg);
		break;
       case 'I':			/* Zmiana nazwy ksiazki */
		sprintf(bookname,"-A %s",optarg);
		break;
       case 'M':
		maxseg=atoi(optarg);	/* Maksymalny numer segmentu */
		break;
       case 't':
	        if(strcmp(optarg,"ascii")==0)
	          ascii=ON;
	        else if(strcmp(optarg,"binary")==0)
                  ascii=OFF;
                else fprintf(stderr,"Opcja -t %s zignorowana !\n",optarg);  
                break;  
       default:
		fputs("Niezana opcja !\n\r",stderr);
		FreeArgv(argv,argc);
		return;
     }

  FreeArgv(argv,argc);
  if(ok==0)
    {
      fputs("Nie podano nazwy pliku !\n\r",stderr);
      return;
    }
  setChannel=0;
		/* Okreslenie liczby segmentow (FLOAT) */
  IleSegmentow=(int)(FileSizeChar(filename)/
		    ((long)Nkan*(long)sizeof(float)*(long)(DimBase+shift)));
  if(maxseg>0)
    IleSegmentow=MIN(IleSegmentow,maxseg);

  if(prn==1)
    fprintf(stdout,"\t\t<<<     ANALIZA KANALU EEG (FLOATROTT WITH SYM)    >>>\n"
		   "\t\t           (%d SEGMENTOW %d DO ANALIZY)\n",
		   IleSegmentow,IleSegmentow-offset);

  for(i=offset,k=1 ; i<IleSegmentow ; i++,k++)
   {
     const int itmp=2*(int)DimBase;

     if(prn==1)
       fprintf(stdout,"\t<<< ANALIZA %d REKORDU >>>\n",i+1);

     Reset(NULL);                	/* Zera wszedzie */
     LOAD_FLOAT_SIGNAL(i,-C);	/* Ladujemy segment centralny */
     for(j=0 ; j<(int)DimBase ; j++) /* Asymetryczne warunki brzegowe */
      {
	sygnal[(int)DimBase-1-j]=-sygnal[j];
	sygnal[itmp+j]=-sygnal[itmp-1-j];
      }
      
     REINIC(); 
     AnalizaMP(NULL);
     file_offset=i;		/* Ladowanie zmienia file_offset */
     if(ascii==OFF)
       SaveAllNewAtoms(bookname);	/* Zapis ksiazki na dysku */
     else {
       TranslateAtoms(DimBase*i);
       AscSaveAllAtoms(bookname);   
     }  
   }

  setChannel=1;
  if(prn==1)
    fprintf(stdout,"<<< KONIEC ANALIZY KANALU EEG (FLOATROTT WITH SYM) >>>\n");
 }
 













