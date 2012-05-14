/* Generacja mapy Wignera w formacie GIF'a 1996 10 12/13
   Adaptacja na potrzeby programu hmpp 1997 01 11/12,1997 01 30
   - mozliwosc generacji map usrednionych
   1997 05 21;1997 07 02 - mozliwosc generacji tablicy float'ow [0..1]
   1997 08 21; 1997 11 16/19/22; 1998 01 01
   1999 06 29            - zmiana formatu zapisu ksizek 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gif_lib.h"
#include "typdef.h"
#include "asciitab.h"
#include "shell.h"
#include "wigmap.h"
#include "iobook.h"
#include "new_io.h"
#include "proto.h"

#define WHITE         255
#define STRING        256
#define GRAY	      1
#define COLOR         2
#define INVERT_GRAY   3
#define ON            1
#define OFF           0

static char Comment[STRING]="";		/* Tablica z komnetarzem */
static UCHAR **VirtualScreen;
static int SizeScreenX=-1,SizeScreenY=-1;
static UCHAR **ImageTab;		/* Tablica z obrazem mapy Wignera */
static PSBOOK *psbook;                  /* Struktura danych na potrzeby wigmap */
static int OldX,OldY;			/* Zmienne pomocnicze przy grafice */
extern int prn;				/* Drukowanie informacji pomocniczych */
extern float *OrgSygnal;		/* zbior z orginalnym sygnalem */
extern int LogarytmStat;		/* Drukowanie mapy logarytmicznej (wigmap) */
extern int Getopt(int,char **,char *);  /* Pobranie opcji z hmpp */
extern char *optarg;                    /* Wskaznik dla tablicy opcji */
extern int opterr,optind,sp;            /* Zmienne pomocnicze Getopt */
extern BOOK *book;			/* Aktualna ksizka w hmpp */
extern int dimroz;			/* Status obliczen wymiar rozwiazania */
extern float *OrgSygnal;		/* zbior z orginalnym sygnalem */
extern int LogarytmStat;		/* Drukowanie mapy logarytmicznej (wigmap) */
					/* Podstawowe funkcje graficzne */

static void PutPixel(int x,int y,int color)
 {					/* Ustawienie pixla na ekranie wirtualnym */
   if(x<SizeScreenX && x>=0 && y<SizeScreenY && y>=0)
		VirtualScreen[y][x]=(UCHAR)color;
 }

static void PutString(int x,int y,int color,char *string)
 {
   int i,k,j,l=0;	/* Wypisanie napisu w pozycji horyzontalnej */

   while(string[l]!='\0')
    {
     for(j=0 ; j<8 ; j++)
      for(i=0,k=7 ; i<8 ; i++,k--)
       if(AsciiTable[(int)string[l]][j] & (1U << k))
	 PutPixel(x+i,y+j,color);
     l++;
     x+=8;
    }
 }

static void PutSmallString(int x,int y,int color,char *string)
 {
   int i,k,j,l=0;	/* Wypisanie napisu w pozycji horyzontalnej */

   while(string[l]!='\0')
    {
     for(j=0 ; j<8 ; j++)
      for(i=0,k=7 ; i<8 ; i++,k--)
       if(AsciiTable[(int)string[l]][j] & (1U << k))
	 PutPixel(x+(3*i)/4,y+(3*j)/4,color);
     l++;
     x+=(3*8)/4;
    }
 }

static void PutStringVertical(int x,int y,int color,char *string)
 {
   int i,k,j,l=0;   /* Wypisanie napisu w pozycji pionowej */

   while(string[l]!='\0')
    {
     for(j=0 ; j<8 ; j++)
      for(i=0,k=7 ; i<8 ; i++,k--)
       if(AsciiTable[(int)string[l]][j] & (1U << k))
	 PutPixel(x+j,y-i,color);
     l++;
     y-=8;
    }
 }

static void nSwap(int *a,int *b)	/* Zamiana zmiennych miejscami */
 {
   register int tmp;

   tmp=*a; *a=*b; *b=tmp;
 }

#define ABS(X) (((X)>=0) ? (X) : -(X))

static void Line(int x1,int y1,int x2,int y2,int ucPixel) /* ucPixel == color */
 {
    int *px,*py,y,a,b,a1,b1,a2,b2,Aincr,
	Bincr,bincr,d,dx,dy,da,db;

    if(x2==x1)  /* Algorytm Bresenhama */
    {
      if(y1>y2) nSwap(&y1,&y2);
      for(y=y1; y<=y2; y++)
	PutPixel(x1,y,ucPixel);
      return;
    }

    dy=y2-y1;
    dx=x2-x1;

    if(ABS(dy)<=ABS(dx))
    {
      b2=y2; b1=y1; a2=x2; a1=x1;
      px=&a; py=&b;
    }
    else
    {
      b2=x2; b1=x1; a2=y2; a1=y1;
      px=&b; py=&a;
    }

    if(a1>a2)
    {
      nSwap(&a1,&a2);
      nSwap(&b1,&b2);
    }

    if(b2>b1)
      bincr=1;
    else
      bincr=-1;

    da=a2-a1; db=ABS(b2-b1);
    d=2*db-da;
    Aincr=2*(db-da);
    Bincr=2*db;

    a=a1; b=b1;
    PutPixel(*px,*py,ucPixel);

    for(a=a1+1; a<=a2; a++)
    {
      if(d>=0)
      {
	b+=bincr;
	d+=Aincr;
      }
      else d+=Bincr;
      PutPixel(*px,*py,ucPixel );
    }
 }

#undef ABS

static void Rectangle(int xp,int yp,int xk,int yk,int color)
 {
   int i ;			/* Rysowanie prostokata */

   if(xp>xk) nSwap(&xp,&xk);
   if(yp>yk) nSwap(&yp,&yk);

   for(i=xp ; i<=xk ; i++)
     {
       PutPixel(i,yp,color);
       PutPixel(i,yk,color);
     }
  for(i=yp ; i<=yk ; i++)
   {
     PutPixel(xp,i,color);
     PutPixel(xk,i,color);
   }
 }

static int OpenVirtualScreen(int SizeX,int SizeY)
 {                         /* Inicjacja struktur ekranu wirtualnego */
   const unsigned Size=SizeX*sizeof(UCHAR);
   int i,j;

   if((VirtualScreen=(UCHAR **)malloc(SizeY*sizeof(UCHAR *)))==NULL)
     return -1;
   for(i=0 ; i<SizeY ; i++)
    if((VirtualScreen[i]=(UCHAR *)malloc(Size))==NULL)
      {
	for(j=0 ; j<i ; i++)
	  free((void *)VirtualScreen[j]);
	free((void *)VirtualScreen);
	return -1;
      }
    else (void)memset((void *)VirtualScreen[i],0,Size);

   SizeScreenX=SizeX;
   SizeScreenY=SizeY;
   return 0;
 }

static void CloseVirtualScreen(void)
 {
   int i;	/* Zamkiniecie ekranu wirtualnego */

   for(i=0 ; i<SizeScreenY ; i++)
     free((void *)VirtualScreen[i]);
   free((void *)VirtualScreen);
   SizeScreenY=SizeScreenX=-1;
 }

#define SQR(X) ((X)*(X))
#define WCONST 0.1777777e-3F
#define HCONST 255.0F

static void MakePalette(GifColorType paleta[],int Opcja,int Laplas)
 {
   int i,k;	/* Generacja palety kolorow Termiczna lub odcienie szarosci */

   if(Opcja==COLOR)
    {
      paleta[255].Red=paleta[255].Green=paleta[255].Blue=255;
      for(i=0,k=254 ; i<255 ; i++,k--)
       {
	 paleta[k].Red=(UCHAR)(HCONST*exp(-(float)SQR(i-64L)*WCONST));
	 paleta[k].Green=(UCHAR)(HCONST*exp(-(float)SQR(i-128L)*WCONST));
	 paleta[k].Blue=(UCHAR)(HCONST*exp(-(float)SQR(i-192L)*WCONST));
       }
      paleta[0].Red=paleta[0].Green=paleta[0].Blue=0;
    }
   else if(Opcja==GRAY) {
     for(i=0 ; i<=255 ; i++)
       paleta[i].Red=paleta[i].Green=paleta[i].Blue=(UCHAR)i;
   } else if(Opcja==INVERT_GRAY) {
      for(i=0 ; i<=255 ; i++)
	paleta[i].Red=paleta[i].Green=paleta[i].Blue=(UCHAR)(255-i);
   }

   if(Laplas>0)
     for(i=Laplas ; i<255 ; i+=Laplas)
       {
         paleta[i].Red=0;
         paleta[i].Green=255;
         paleta[i].Blue=0;
       }
 }

static void QuitGifError(GifFileType *GifFile)
 {
    PrintGifError();		/* W przypadku blednej obslugi GIf'a */
    if(GifFile!=NULL)		/* Wyswietlenie komunikatu + zwolnienie zasbow */
      EGifCloseFile(GifFile);
 }

static int ImageToGif(UCHAR **Image,int ImageWidth,int ImageHeight,
		char *GifFileName,int Paleta,int Laplas)
 {
    int	NumLevels=255,i;	/* Konwersja ekranu wirtualnego do postaci */
    GifColorType *ColorMap;     /* obrazu w formacie GIF */
    GifFileType *GifFile;	/* Oparte na bibliotece giflib */

    if(prn==ON)
      fprintf(stdout,"<<< TWORZENIE OBRAZU W FORMACIE GIF'A : %s >>>\n",GifFileName);

    if((GifFile=EGifOpenFileName(GifFileName,0))==NULL)
     {
       QuitGifError(GifFile);
       return -1;
     }

    if((ColorMap=(GifColorType *)malloc(NumLevels*sizeof(GifColorType)))==NULL)
     {
      fprintf(stderr,"Failed to allocate memory required, aborted.\n");
      QuitGifError(GifFile);
      return -1;
     }

    if(prn==ON)
      fprintf(stdout,"<<< GENERACJA PALETY BARW (%s) >>>\n",
	      ((Paleta==COLOR) ? "TERM" : 
	       ( (Paleta==INVERT_GRAY) ? "INVERT GARY" : "GRAY") ) );

    MakePalette(ColorMap,Paleta,Laplas);
    if(EGifPutScreenDesc(GifFile,ImageWidth,ImageHeight,8,0,
       8,ColorMap)==GIF_ERROR)
     {
       QuitGifError(GifFile);
       free((void *)ColorMap);
       return -1;
     }

    if(EGifPutImageDesc(GifFile,0,0,ImageWidth,ImageHeight,
       FALSE,8,NULL)==GIF_ERROR)
     {
       QuitGifError(GifFile);
       free((void *)ColorMap);
       return -1;
     }

    if(prn==ON)
      fprintf(stdout,"<<< KODOWANIE OBRAZU >>>\n");

    for(i=0 ; i<ImageHeight ; i++)
     if(EGifPutLine(GifFile,Image[i],ImageWidth)==GIF_ERROR)
      {
	QuitGifError(GifFile);
	free((void *)ColorMap);
	return -1;
     }

    if(EGifCloseFile(GifFile)==GIF_ERROR)
     {
       QuitGifError(GifFile);
       free((void *)ColorMap);
       return -1;
     }

    free((void *)ColorMap);
    return 0;
 }

#define CENTRY 164

static void LineTo(int x,int y)     /* Generacja linji do x,y */
 {
   Line(OldX,OldY,x,y,WHITE);
   OldX=x;
   OldY=y;
 }

static void SignalToVirtualScreen(int sizey,int RealSignalLen,int reverse)
 {
   float min,max,skala,skalax;   /* Rysowanie wykresu na ekranie wirtualnym */
   int i;
   const float s=(reverse==1) ? -1.0F : 1.0F;

   min=max=s*OrgSygnal[0];
   for(i=0 ; i<(int)DimBase ; i++)
    {
      if(s*OrgSygnal[i]>max) max=s*OrgSygnal[i];
      if(s*OrgSygnal[i]<min) min=s*OrgSygnal[i];
    }

   skala=100.0F/((min==max) ? 1.0F : max-min);
   skalax=(float)RealSignalLen/(float)DimBase;
   OldX=70;
   OldY=sizey-CENTRY+140-(int)(skala*(s*OrgSygnal[0]-min));
   for(i=1 ; i<(int)DimBase ; i++)
     LineTo(70+(int)(skalax*(float)i),
	    sizey-CENTRY+140-(int)(skala*(s*OrgSygnal[i]-min)));
 }

static void MakeTimeFreqScale(int sizex,int sizey,float SplitFactor,
                              int ImageY)
 {
   const int maxh=ImageY-CENTRY;		  /* Rysowanie skal na gifie */
   float maxTime=DimBase/SamplingRate,maxFreq,
         Fstep,freq,Tstep,step,dtf;
   int X,Y;
   char string[20];
  
   if(maxTime<2.0F)
     Tstep=0.1F;
   else if(maxTime<5.0F)
     Tstep=0.5F;  
   else if(maxTime<10.0F)				   /* Wybor skali czasu */
     Tstep=1.0F;
   else if(maxTime<100.0F)
     Tstep=5.0F;
   else if(maxTime<200.0F)
     Tstep=15.0F;  
   else
     Tstep=maxTime/10.0F;
   
   dtf=sizex/maxTime;
   for(step=0 ; step<=maxTime ; step+=Tstep)
     {
       X=70+(int)(step*dtf);
       Line(X,maxh,X,maxh+3,WHITE);       
       sprintf(string,"%3.1f",step);
       PutSmallString(X-2*strlen(string),maxh+10,WHITE,string);
     }  
        
   maxFreq=0.5F*SamplingRate/SplitFactor;
   if(maxFreq<2.0F)				  /* Wybor skali czestosci */
     Fstep=0.1F;
   else if(maxFreq<5.0F)
     Fstep=0.5F;
   else if(maxFreq<10.0F)
     Fstep=1.0F;
   else if(maxFreq<100.0F)
     Fstep=5.0F;
   else if(maxFreq<200.0F)
     Fstep=15.0F;  
   else 
     Fstep=maxFreq/10.0F;
     
   dtf=sizey/maxFreq;
   for(freq=0 ; freq<=maxFreq ; freq+=Fstep)         /*  Rysowanie */ 
     {
       Y=maxh-(int)(freq*dtf);
       Line(70,Y,70-3,Y,WHITE);
       sprintf(string,"%2.1f",freq);
       PutSmallString(70-18-2*strlen(string),Y-5,WHITE,string);
     }
 }

static void MakeImageFrame(char *Title,int sizex,int sizey,
			   int RealSignalLen,int RealFreqLen,
                           int opcja,float SplitFactor)
 {
   /* const int SigCentr=RealSignalLen/2;*/
   int i,j;   				        /* Rysowanie "osnowy" rysunku */
   opcja=OFF;
   PutString(sizex/2-4*strlen(Title),12,WHITE,Title);
   Rectangle(0,0,sizex-1,sizey-1,WHITE);	/* Ramka rysunku */
   Rectangle(2,2,sizex-3,sizey-3,WHITE);

   for(i=0 ; i<255 ; i++)			/* Skala barw */
     for(j=0 ; j<20 ; j++)
       PutPixel(10+j,sizey-CENTRY-i,i);
   Rectangle(10,sizey-CENTRY,30,sizey-254-CENTRY,WHITE);
						/* Ramka Mapy */
   Rectangle(70,sizey-CENTRY,70+RealSignalLen,sizey-RealFreqLen-CENTRY,WHITE);
   /* 
      PutStringVertical(35,sizey-CENTRY-10,WHITE,"Frequency [Hz]");
      PutString(70+SigCentr-5*8,sizey-CENTRY+22,WHITE," Time [s] "); 
  

   if(opcja==OFF)  			    
     {
       PutString(100,sizey-CENTRY+30,WHITE,"Signal");
       Rectangle(70,sizey-CENTRY+40,70+RealSignalLen,sizey-CENTRY+140,WHITE);
       PutString(70+SigCentr,sizey-20,WHITE,"Time");    
       PutStringVertical(50,sizey-40,WHITE,"Amplitude");
    }
    */
   MakeTimeFreqScale(RealSignalLen,RealFreqLen,SplitFactor,sizey); 
 }

#define ERROR(STR) { fprintf(stderr,STR "\n");	        \
		     if(book!=NULL) free((void *)book); \
		     if(plik!=NULL) fclose(plik);       \
		     return NULL; }

static PSBOOK *ReadAllOldAtoms(char *bok_filename,int *NumOfWav)
 {      /* Wczytanie wszystkich zestawow ksiazek (usredniona mapa Wignera) */
   extern unsigned long NumberOfAllWaveForms;
   int i,k,NumberOfBloks,j,num_of_wav;
   PSBOOK *book=NULL;
   FILE *plik=NULL;
   HEADER head;			/* Funkcja zwraca globalna liczbe atomow */
   ATOM atom;
   float df;

   if((NumberOfBloks=LicznikKsiazek(bok_filename))==-1)
     ERROR("Problemy z odczytem struktury ksiazki !");

   num_of_wav=(int)NumberOfAllWaveForms;
   if((book=(PSBOOK *)malloc((unsigned)num_of_wav*sizeof(PSBOOK)))==NULL)
     ERROR("Brak pamieci (ReadAllAtoms) !");

   if((plik=fopen(bok_filename,"rb"))==NULL)
     ERROR("Problemy przy otwarciu zbioru !\n");

    if(ReadHeader(&head,plik)==-1)
     ERROR("Blad czytania naglowka (ReadAllAtoms) !");

    fseek(plik,0L,SEEK_SET);
    df=2.0F*M_PI/(float)(head.signal_size);

    Ec=E0=0.0F;
    for(i=0,k=0 ; i<NumberOfBloks ; i++)
     {
       if(ReadHeader(&head,plik)==-1)
	 ERROR("Blad czytania naglowka (ReadAllAtoms) !");

       if(head.signal_size!=DimBase)                /* W przypadku niezgodnosci */
	 if(prn==ON)
	   fprintf(stderr,"UWAGA ! Niezgodnosc rozmiaru bazy (%d)"
		   " i analizowanego naglowka (%d)\n",
		   DimBase,head.signal_size);

       E0+=head.signal_energy;
       for(j=0 ; j<head.book_size ; j++)
	{
	  if(ReadAtom(&atom,plik)==-1)
	    ERROR("Blad czytania atomu (ReadAllAtoms) !");

	  book[k].s=(float)(1<<(atom.octave));
	  book[k].t=(float)atom.position;
	  book[k].w=atom.modulus;
	  book[k].f=df*(float)atom.frequency;
	  book[k].amplitude=atom.amplitude;
	  Ec+=SQR(atom.modulus);
	  k++;
	}
     }
   fclose(plik);
   *NumOfWav=num_of_wav;
   return book;
 }

static PSBOOK *ReadAllNewAtoms(char *bok_filename,int *NumOfWav) {     
   extern unsigned long NumberOfAllWaveForms;
   int i,k,NumberOfBloks,j,num_of_wav;
   PSBOOK *book=NULL;
   FILE *plik=NULL;
   SEG_HEADER  head;		
   NEW_ATOM    atom;
   float df;

   if((NumberOfBloks=countBook(bok_filename))==-1)
     ERROR("Problemy z odczytem struktury ksiazki !");

   num_of_wav=(int)NumberOfAllWaveForms;
   if((book=(PSBOOK *)malloc((unsigned)num_of_wav*sizeof(PSBOOK)))==NULL)
     ERROR("Brak pamieci (ReadAllNewAtoms) !");

   if((plik=fopen(bok_filename,"rb"))==NULL)
     ERROR("Problemy przy otwarciu zbioru !\n");

   if(skipHeader(plik)==-1)
     ERROR("Problemy przy pomijaniu naglowka \n");

   if(ReadSegmentHeader(&head,plik)==-1)
     ERROR("Blad czytania naglowka (ReadAllNewAtoms) !");

   df=2.0F*M_PI/(float)(head.signal_size);
   Ec=E0=0.0F;
   for(i=0,k=0 ; i<NumberOfBloks ; i++) {
     if(i!=0) {
       if(ReadSegmentHeader(&head,plik)==-1)
	 ERROR("Blad czytania naglowka (ReadAllNewAtoms) !");
     }

     if(head.signal_size!=DimBase)    
       if(prn==ON)
	 fprintf(stderr,"UWAGA ! Niezgodnosc rozmiaru bazy (%d)"
		 " i analizowanego naglowka (%d)\n",
		 DimBase,head.signal_size);
     
     E0+=head.signal_energy;
     for(j=0 ; j<head.book_size ; j++) {
       if(ReadNewAtom(&atom,plik)==-1)
	 ERROR("Blad czytania atomu (ReadAllNewAtoms) !");
       
       book[k].s=(float)atom.scale;
       book[k].t=(float)atom.position;
       book[k].w=atom.modulus;
       book[k].f=df*(float)atom.frequency;
       book[k].amplitude=atom.amplitude;
       Ec+=SQR(atom.modulus);
       k++;
     }
   }
   fclose(plik);
   *NumOfWav=num_of_wav;
   return book;
}

#undef ERROR

PSBOOK *ReadAllAtoms(char *bok_filename,int *NumOfWav) {
  FILE *file;
  int mode;

  if((file=fopen(bok_filename,"rb"))==NULL) 
    return NULL;

  mode=checkBookVersion(file);
  fclose(file);

  if(mode==-1) {
    if(prn==1) 
      fprintf(stdout,"<<< OLD BOOK FORMAT >>>\n");
    return ReadAllOldAtoms(bok_filename,NumOfWav);
  }

  if(prn==1) 
    fprintf(stdout,"<<< NEW BOOK FORMAT >>>\n");
  return ReadAllNewAtoms(bok_filename,NumOfWav);
}

#define POZX(X) (70+(X))		/* Centrowanie Mapy Wignera */
#define POZY(Y) (MaxYpoz-CENTRY-(Y))

void WigToGif(char *opt)		/* Generacja mapy Wignera w postaci GIF'a */
 {
   char *argv[STRING],filename[STRING]="wigner.gif",title[STRING]="",
        avrfilename[STRING],bmpfile[STRING]="wigner.bin";
   int i,j,argc,x=DimBase,y=DimBase/2,opcja,paleta=COLOR,MaxYpoz,
       MaxXpoz,avropt=OFF,TmpDimRoz,IncludeSignal=OFF,
       BitMap=NOGENBITMAP,MakeDyspers=OFF,Laplas=0,reverse=0;
   float GammaCorect=1.0F,SplitFactor=1.0F,Crop=1.0F;
   PSBOOK *tmpbook;

   LogarytmStat=0;
   StrToArgv(opt,argv,&argc);
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"O:X:Y:LF:cgxa:sS:bB:dk:l:Ri"))!=EOF)
     switch(opcja) {
     case 'R':
       reverse=1;
       break;
     case 'l':
       Laplas=atoi(optarg);
       break;
     case 'k':
       Crop=atof(optarg);
       break;
     case 'B':
       (void)strcpy(bmpfile,optarg);
     case 'b':
       BitMap=GENBITMAP;
       break;
     case 'S':
       SplitFactor=atof(optarg);
       break;
     case 's':
       IncludeSignal=ON;
       break;
     case 'a':
       (void)strcpy(avrfilename,optarg);
       avropt=ON;
       break;
     case 'x':
       (void)strcpy(title,Comment);
       break;
     case 'c':
       paleta=COLOR;
       break;
     case 'g':
       paleta=GRAY;
       break;
     case 'i':
       paleta=INVERT_GRAY;
       break;
     case 'O':
       (void)strcpy(filename,optarg);
       break;
     case 'F':
       GammaCorect=atof(optarg);
       break;
     case 'X':
       x=atoi(optarg);
       break;
     case 'Y':
       y=atoi(optarg);
       break;
     case 'L':
       LogarytmStat=1;
       break;
     case 'd':
       MakeDyspers=ON;
       break;
     default:
       fprintf(stderr,"Nieznana opcja !\n");
       FreeArgv(argv,argc);
       return;
     }

   FreeArgv(argv,argc);
   if(avropt==OFF)
     if(Compute==OFF)
       {
	 fprintf(stderr,"Nie wykonane obliczenia !\n");
	 return;
       }
       
  psbook=NULL;
  if(avropt==OFF)
   if((psbook=(PSBOOK *)malloc((unsigned)dimroz*sizeof(PSBOOK)))==NULL)
    {
      fprintf(stderr,"Brak pamieci (Wig2Gif) !\n");
      return;
    }

   MaxXpoz=x+100;
   MaxYpoz=450+((y>256) ? y-256 : 0);
  
   if(BitMap==NOGENBITMAP)
     if(OpenVirtualScreen(MaxXpoz,MaxYpoz)==-1)
       {
         fprintf(stderr,"Brak pamieci w OpenVirtualScreen (Wig2Gif) !\n");
         if(psbook!=NULL) 
          free((void *)psbook);
         return;
       }

   if(avropt==OFF)
    for(i=0 ; i<dimroz; i++)  /* Konwersja na format funkcji MakeWignerMap */
     {
       psbook[i].s=book[i].param[0];
       psbook[i].t=book[i].param[1];
       psbook[i].w=book[i].waga;
       psbook[i].f=book[i].param[2];
     }

   if((ImageTab=MakeTableUChar(x,y))==NULL)	/* Obraz mapy Wignera */
     {
       fprintf(stderr,"No memory (Wig2Gif) !\n");
       CloseVirtualScreen();
       if(psbook!=NULL) free((void *)psbook);
       return;
     }

   if(prn==1)
     fprintf(stdout,"\n<<< GENERATING TIME-FREQUENCY MAP >>>\n");

   if(avropt==OFF)
     MakeWignerMap(psbook,dimroz,x,y,DimBase,
                   ImageTab,GammaCorect,SplitFactor,bmpfile,
		   BitMap,Crop);
   else
     {
       if((tmpbook=ReadAllAtoms(avrfilename,&TmpDimRoz))==NULL)
	{
	  fprintf(stderr,"Error (Wig2Gif,ReadAllAtoms) !\n");
	  CloseVirtualScreen();
	  if(psbook!=NULL) free((void *)psbook);
	  return;
	}
       if(MakeDyspers==OFF) 
          MakeWignerMap(tmpbook,TmpDimRoz,x,y,DimBase,ImageTab,
                        GammaCorect,SplitFactor,bmpfile,BitMap,Crop);
       else
	 MakeDyspersWignerMap(tmpbook,TmpDimRoz,x,y,DimBase,ImageTab,
                              GammaCorect,SplitFactor,bmpfile,BitMap,Crop);                 
       free((void *)tmpbook);
     }

   if(BitMap==NOGENBITMAP)
    {
     for(i=0 ; i<x ; i++)		/* Rysowanie mapy Wignara na ekranie wirtualnym */
       for(j=0 ; j<y ; j++)
         if(ImageTab[i][j]==0)
           PutPixel(POZX(i),POZY(j),1);
         else
           PutPixel(POZX(i),POZY(j),ImageTab[i][j]);

     if(avropt==OFF || IncludeSignal==ON)
       SignalToVirtualScreen(MaxYpoz,x,reverse);	      /* Rysowanie sygnalu */
     MakeImageFrame(title,MaxXpoz,MaxYpoz,x,y,
                    ((avropt==OFF || IncludeSignal==ON) ? OFF : ON),
                    SplitFactor); /* Osnowa rysunku */
					      /* Generacja gif'a */
     (void)ImageToGif(VirtualScreen,MaxXpoz,MaxYpoz,filename,paleta,Laplas);
     CloseVirtualScreen();
   }
   
   if(psbook!=NULL) 
     free((void *)psbook);
   FreeTableUChar(ImageTab,x);   /* Zwolnienie pamieci jest bezpieczne */
  
   if(prn==1)
     fprintf(stdout,"<<< KONIEC >>>\n");
 }

void GetComment(char *opt) {		      /* Pobranie komentarza wypisywanego */					      /* na mapce Wignera w postaci GIF'a */
   strcpy(Comment,opt);
   return;
 }

void ShowComment(void) {		      /* Wydruk tego komentarza na ekranie */
   fprintf(stdout,"Title: %s\n",Comment);
   return;
 }
 
