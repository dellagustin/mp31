/* Rozklad sygnalu metoda MP slowniki stochastyczne 1997 04 02 1997 05 27 */
/* 1997 07 02 1997 08 26/28; 1997 09 14; 1997 10 04 1997 11 22 1998 01 01 */
/* 1999 06 29 */
/* 1999 07 07 (help[]) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"
#include "iobook.h"
#include "new_io.h"
#include "proto.h"
#include "rand2d.h"

#define ON     1
#define OFF    0
#define STRING 256

extern void setChannelFunc(char *);
extern int debug;

static int countAllBook(char *opt) {
  FILE *file;
  int mode;

  if((file=fopen(trim(opt),"rb"))==NULL) {
    fprintf(stdout,"Cannot open file %s ...\n",opt);
    return -1;
  }

  mode=checkBookVersion(file);
  fclose(file);
  if(mode==-1) {
    if(prn==ON)
      fprintf(stdout,"<<< OLD BOOK FORMAT >>>\n");
    return LicznikKsiazek(opt);
  } else {
    if(prn==ON)
      /*PJD      fprintf(stdout,"<<< NEW BOOK FORMAT >>>\n"); */
      fprintf(stdout,"<<< BOOK FORMAT v. III >>>\n");
    return countBook(opt);
  }
  return -1;
}

char firsthelp[]="list          -  full listing of commands with short descriptions\n"
                 "help command  -  description of command & its parameters (if available:)\n\n"
                 "Typically one first sets some parameters ('set' - signal&dictionary size etc.),\n"
                 "then loads a signal ('loadasc', 'loadfloat', 'loadint') and runs 'mp'.\n"
                 "Parameters of functions fitted to the signal (a \"book\") can be displayed\n"
                 "('showbook'), saved in ASCII ('saveascbook') or binary ('save') format,\n"
                 "or used to construct a time-frequency representation of explained signal's\n"
                 "energy ('tf2ps', 'tf2gif').\n"
                 "Script files (conv. *.mps), consisting of commands given by 'list',\n"
                 "can be executed from command line if present in 'path' (default .).\n" 
                 "There are also system commands (!, ?, man, help, ls, exit, history),\n"
                 "which cannot be used in scripts.\n"
                 "Details of the procedure and applications are described in papers\n" 
                 "available at  http://brain.fuw.edu.pl/~mp\n";

char *help[]= {
/*set*/
		    "settings\n"
		    "\t\t-D val - adaptive dictionary parameter 0..1 (default 0.9)\n"
                    "\t\t-M val - max number of algorithm's iterations\n"
                    "\t\t-A val - increase max number of algorithm's iterations by val\n"
                    "\t\t-E val - stop iterations after explaining val % of energy\n"
                    "\t\t-P +|- - messages on|off (+|-)\n"
                    "\t\t-I +|- - decomposition progress info on|off (+|-)\n"
                    "\t\t-O val - signal size (val points)\n"
                    "\t\t-F val - signal sampling frequency [Hz]\n"
                    "\t\t-C val - signal calibration constant [usually points/microVolt]\n"
                    "\t\t-B val - book filename\n"
                    "\t\t-T     - use dyadic dictionary (default stochastic):\n"
                    "\t\t\t-S val - oversampling (for dyadic dictionary)\n"
                    "\t\t-h +|- - use heuristic acceleration\n"
                    "\t\t\t-N val - number of atoms for trial dictionary:\n"
                    "\t\t\t-o val - range of scales for heuristic\n"
                    "\t\t\t-f val - range of frequencies for heuristic\n"
                    "\t\t\t-p val - range of atom's positions for heuristic\n",
/*loadasc*/
                    "load signal from ASCII file\n"
                    "\t\t-O filename\n"
                    "\t\t-S - simmetric border conditions\n"
                    "\t\t-L - left border condition\n"
                    "\t\t-R - right border condition\n"
                    "\t\t-C - signal (default)\n",
/*mp*/
                    "run Matching Pursuit on loaded signal, w/set parameters\n",
/*showbook*/
                    "display parameters of fitted atoms (current book)\n",
/*loadint*/
                    "load signal from integer [multichannel multiplexed] file\n"
                    "\t\t-O filename\n"
                    "\t\t-# offset\n"
                    "\t\t-c chan (counting from one, not zero)\n"
                    "\t\t-h num_chan - number of channels in file\n"
                    "\t\t-s shift    - number of bytes to skip (e.g. file header)\n"
/*                    "\t\t\ti.e. read from (offset*epoch_size+shift)*num_chan+chan\n"*/
                    "\t\t-e val      - reference channel #1\n"
                    "\t\t-f val      - reference channel #2\n"
                    "\t\t-L          - left border condition\n"
                    "\t\t-R          - right border condition\n"
                    "\t\t-C          - signal (default)\n",
/*tf2ps*/
                    "time-frequency - Wigner(MP) - map, in PostScript\n"
                    "\t\t-N      - no blobs numbers on printout\n"
                    "\t\t-R      - reverse input signal on plot only\n"
	              "\t\t-i val  - (integer) font size in points (default 8)\n"
                    "\t\t-S val  - (default 1) show lower 1/val of frequencies\n"
                    "\t\t-p val  - number of points per microVolt\n"
                    "\t\t-g val  - max number of pages on book printout\n"
                    "\t\t-O file - output PostScript filename\n"
                    "\t\t-F val  - gamma correction coeff. (default 1)\n"
                    "\t\t-L      - log energy scale\n"
                    "\t\t-B file - read atoms (\"book\") from file\n"
                    "\t\t-a file - average map using all the atoms in file\n"
                    "\t\t-s      - add (currently loaded) signal to the averaged map\n"
                    "\t\t-d      - map of dispersion (averaged map)\n"
                    "\t\t-k val  - cut off high values - may improve dynamics, experim.\n" /* poprzez obciecie grzbietow */
                    "\t\t-x val  - string on printout (default as set by 'title')\n",

                    "type the signal and residuum values\n",

                    "quit=exit\n",

                    "save MP results (a \"book\") in \"old\" (pre-v.III) format\n"
                    "\t\t-A(a) filename  - add current book to file\n"
                    "\t\t-S(s) filename  - create new file with current book\n"
                    "\t\t\t for [a,s] file=book.b, for capitals provide filename\n",

                    "set current signal to zeros\n",
/*mp_chan_int_sim*/
                    "batch analysis: integers, simmetric border conditions\n"
                    "\t\t  Subsequent epochs of one channel,\n\t\t  multichannel data multiplexed\n"
                    "\t\t-O filename - signal file\n"
                    "\t\t-c channel  - which channel\n"
                    "\t\t-h num_chan - number of channels in file\n"
                    "\t\t-# offset   - start from\n"
		        "\t\t-M offset   - stop at\n"
                    "\t\t-s shift\n"
                    "\t\t-I filename - results (output) file (\"book\")\n"
/*	            "\t\t-t val      - val=[ascii|binary], \"book\" format (default binary)\n"*/
                    "\t\t-R          - reinitialize dictionary before each decomposition\n"
                    "\t\t-e r1       - reference channel (electrode) #1\n"
                    "\t\t-f r2       - reference channel (electrode) #2\n"
                    "\t\ti.e.: if set, x(channel) -> x(channel)-(x(r1)+x(r2))/2\n",
/*countbook*/
                    "count the sets of fitted atoms (\"books\") in a *.b file\n",
/*mp_chan_in_rott*/
                    "batch analysis: integers, \"natural\" border conditions\n"
                    "\t\t  Subsequent epochs of one channel, multichannel data\n"
                    "\t\t  multiplexed, border conditions for each decomposition use\n"
                    "\t\t  most of the information about previous and next epochs\n"
                    "\t\t-O filename - signal file\n"
                    "\t\t-c channel  - which channel\n"
                    "\t\t-h num_chan - number of channels in file\n"
                    "\t\t-# offset   - start from\n"
		    "\t\t-M offset   - stop at\n"
                    "\t\t-s shift\n"
                    "\t\t-I filename - results (output) file (\"book\")\n"
/*   	            "\t\t-t val      - val=[ascii|binary], \"book\" format (default binary)\n"*/
                    "\t\t-R          - reinitialize dictionary before each decomposition\n"
                    "\t\t-e r1       - reference channel (electrode) #1\n"
                    "\t\t-f r2       - reference channel (electrode) #2\n"
                    "\t\ti.e.: if set, x(channel) -> x(channel)-(x(r1)+x(r2))/2\n",
/*loadfloat*/
                    "load signal from float [multichannel multiplexed] file\n"
                    "\t\ti.e. floating point 4-byte numbers (Intel). Options:\n"
                    "\t\t-O filename\n"
                    "\t\t-# offset\n"
                    "\t\t-c chan     - which channel (counting from 1, not 0)\n"
                    "\t\t-h num_chan - number of channels in file\n"
                    "\t\t-s shift\n"
                    "\t\t\ti.e. read from (offset*epoch_size+shift)*num_chan+chan\n"
                    "\t\t-e val      - reference channel #1\n"
                    "\t\t-f val      - reference channel #2\n"
                    "\t\t-L          - left border condition\n"
                    "\t\t-R          - right border condition\n"
                    "\t\t-C          - signal (default)\n",
/*mp_chan_float_rott*/
                    "batch analysis: floats, \"natural\" border conditions\n"
                    "\t\t\tSubsequent epochs of one channel,\n\t\t\tmultichannel data multiplexed.\n"
                    "\t\t\tBorder conditions for each decomposition use most\n"
                    "\t\t\tof the information about previous and next epochs.\n"
                    "\t\t\tFloats = floating point numbers, 4-byte, Intel\n"
                    "\t\t-O filename - signal file\n"
                    "\t\t-c channel  - which channel\n"
                    "\t\t-h num_chan - number of channels in file\n"
                    "\t\t-# offset   - start from\n"
		        "\t\t-M offset   - stop at\n"
                    "\t\t-s shift\n"
                    "\t\t-I filename - results (output) file (\"book\")\n"
/*	            "\t\t-t val      - val=[ascii|binary], \"book\" format (default binary)\n"*/
                    "\t\t-R          - reinitialize dictionary before each decomposition\n"
                    "\t\t-e r1       - reference channel (electrode) #1\n"
                    "\t\t-f r2       - reference channel (electrode) #2\n"
                    "\t\ti.e.: if set, x(channel) -> x(channel)-(x(r1)+x(r2))/2\n",
/*mp_chan_float_sim*/
                    "batch analysis: floats, simmetric border conditions\n"
                    "\t\t\tsubsequent epochs of one channel,"
			  "\n\t\t\tmultichannel data multiplexed,\n"
                    "\t\t\tsimmetric border conditions,\n"
                    "\t\t\tfloats = floating point numbers, 4-byte, Intel\n"
                    "\t\t-O filename - signal file\n"
                    "\t\t-c channel  - which channel\n"
                    "\t\t-h num_chan - number of channels in file\n"
                    "\t\t-# offset   - start from\n"
		        "\t\t-M offset   - stop at\n"
                    "\t\t-s shift\n"
                    "\t\t-I filename - results (output) file (\"book\")\n"
/*	            "\t\t-t val      - val=[ascii|binary], \"book\" format (default binary)\n"*/
                    "\t\t-R          - reinitialize dictionary before each decomposition\n"
                    "\t\t-e r1       - reference channel (electrode) #1\n"
                    "\t\t-f r2       - reference channel (electrode) #2\n"
                    "\t\ti.e.: if set, x(channel) -> x(channel)-(x(r1)+x(r2))/2\n",
/*list*/
                    "print command names w/1-line descriptions\n",
/*tf2gif*/
                    "time-frequency - Wigner(MP) - map, in GIF or raw format\n"
                    "\t\t-R      - reverse input signal on plot only\n"
                    "\t\t-S val  - (default 1) show lower 1/val of frequencies\n"
                    "\t\t-O file - output GIF filename\n"
                    "\t\t-F val  - gamma correction coeff. (default 1)\n"
                    "\t\t-L      - log energy scale\n"
                    "\t\t-B file - save \"raw\" t-f map (floats) in 'file'\n"
                    "\t\t-b      - save \"raw\" t-f map (floats) in 'image.raw'(???)\n"
                    "\t\t-a file - average map using all the atoms in file\n"
                    "\t\t-s      - add (currently loaded) signal to the averaged map\n"
                    "\t\t-d      - map of dispersion (averaged map)\n"
                    "\t\t-k val  - cut off high values - may improve dynamics, experim.\n" /* poprzez obciecie grzbietow */
                    "\t\t-x val  - string on printout - default as set by 'title'\n"
                    "\t\t-l val  - laplasjan 0--3\n"
                    "\t\t-X val  - horizontal dimension (points)\n"
                    "\t\t-Y val  - vertical dimension (points)\n"
                    "\t\t-c      - color (default)\n"
                    "\t\t-g      - shades of gray\n"
		    "\t\t-i      - shades of gray (inverse)",
/*title*/
                    "set printout's title\n",
/*showtitle*/
                    "show currently set printout's title\n",
/*ver*/
                    "program version info\n",

                    "save signal/residuum to disk file\n"
                    "\t\t-O filename (default 'signal.asc')\n"
                    "\t\t-x         - save original signal (default residuum)\n"
                    "\t\t-a         - include border conditions\n",
/*reinit*/
                    "reinitialize the dictionary\n"
                    "\t\t-R val     - number of (Gabor) atoms\n"
                    "\t\t-i [+|-]   - initialize random numbers generator\n"
                    "\t\t-d [+|-]   - use dyadic dictionary structure (scale = 2^j)\n",

                    "load ASCII file with MP results (\"book\")\n"
                    "\t\t-O file\n"
                    "\t\t-L - create current signal from book's reconstruction\n",

                    "save ASCII file with MP results (\"book\")\n"
                    "\t\t-A(a) file - add to file\n"
                    "\t\t-S(s) file - create new file\n"
                    "\t\t\t for [a,s] file=book.asc, for capitals provide filename\n",

                    "save dictionary's structure\n"
                    "\t\t-O filename (default dict.dat)\n"
                    "\t\t-r +|-      - histogram\n"
                    "\t\t-d +|-      - atom's parameters\n",
/*reconst*/
                    "save signal's reconstruction\n"
                    "\t\t-O filename\n"
                    "\t\t-S all calculations (default)\n"
                    "\t\t-M save table with all fitted atoms in time domain\n"
                     "\t\t-R save residuum\n"
                     "\t\t-m val -  reconstruct atom fitted as number val only\n",
/*path*/
                    "set search path for scripts (*.mps)\n",
/*tunedic*/
                    "set probability distribution of scales\n"
                    "\t\t-f filename - distribution from file\n"
                    "\t\t-o val,val  - distribution for a single element\n",
/*resetdic*/
                    "reset probability distribution of scales to defaults\n",
/*norm*/
  	            "signal normalization: sig -> (sig-aver(sig))/var(sig)\n",
/*fft*/
                    "FFT of signal or residuum\n"
                    "\t\t-O filename (default fftsig.asc))\n"
                    "\t\t-w val     - window: 1-Parzen (default), 2-square, 3-Welch\n"
                    "\t\t-o         - FFT of the signal (default)\n"
                    "\t\t-r         - FFT of the residuum\n"
                    "\t\t-p         - phase spectrum\n"
                    "\t\t-a         - amplitude spectrum (default)\n"
                    "\t\t-x val     - window start (default 0)\n"
                    "\t\t-y val     - window end (default signal's size as set)\n",
/*showdist*/
                    "print density of atoms at scales\n",
/*echo*/
                    "echo a string\n",
/*addpath*/
                    "add new path for scripts (*.mps) location\n",
/*loadbook*/
                    "load binary (v.III) file with MP results (\"book\")\n"
                    "\t\t-O filename\n"
                    "\t\t-# offset (of book in file)\n"
                    "\t\t-L - create current signal from book's reconstruction\n",
/*save*/
                    "save MP results (a \"book\") in binary (v.III) format\n"
                    "\t\t-A(a) filename  - add current book to file\n"
                    "\t\t-S(s) filename  - create new file with current book\n"
                    "\t\t\t for [a,s] file=book.b, for capitals provide filename\n",
/*setinfo*/
                    "set INFO field for book's file header (v.III binary)\n",
/*showinfo*/
                    "show current INFO field for book's file header\n",
/*viewJavaApplet*/
		    "display interactive t-f representation (java)\n"
		    "\tIf java is installed in your system, an application will be run\n"
		    "\tallowing e.g. for saving of the interactively reconstructed signal.\n"
		    "\tOtherwise, netscape is called with browser's security restrictions.\n"
		    "\tTo use this option please install java binaries (jdk, jre)\n"
		    "\tor netscape. Options:\n"
		    "\t\t-v [jdk | jre | netscape | auto] (default = auto)\n",
		    "set channel (info)\n"

};

COMMAND commands[]={                    /* Zbior komend shell'a */
                         {"set",SetMPP},        /* zewnetrznych (ls,help,history wbudowane)*/
                         {"loadasc",Load},
                         {"mp",AnalizaMP},
                         {"showbook",(FUNCTION *)PrintBook},
                         {"loadint",ReadSignalBinary},
                         {"tf2ps",WigMapPs},
                         {"type",TypeSignal},
                         {"quit",Quit},
                         {"saveoldbook",SaveAllAtoms},
                         {"reset_sig",Reset},
                         {"mp_chan_int_sim",MakeCanalEEG},
                         {"countbook",(FUNCTION *)countAllBook},
                         {"mp_chan_int_rott",RottCanalEEG},
                         {"loadfloat",ReadFloatSignal},
                         {"mp_chan_float_rott",FloatRottCanalEEG},
                         {"mp_chan_float_sim",FloatCanalEEG},
                         {"list",(FUNCTION *)CommandList},
                         {"tf2gif",WigToGif},
                         {"title",GetComment},
                         {"showtitle",(FUNCTION *)ShowComment},
                         {"ver",(FUNCTION *)SourceVersion},
                         {"write",WriteAllSignal},
                         {"reinit",RestartDiction},
                         {"loadascbook",AscLoadBook},
                         {"saveascbook",AscSaveAllAtoms},
                         {"showdic",ShowDictionary},
                         {"reconst",SigReconst},
                         {"path",SetBatchPath},
                         {"tunedic",TuneOctave},
                         {"resetdic",(FUNCTION *)ResetDyst},
                         {"norm",Norm},
                         {"fft",SpectrumFFT},
                         {"showdist",(FUNCTION *)ShowDyst},
                         {"echo",Echo},
                         {"addpath",addPath},
                         {"loadbook",LoadNewBook},
                         {"save",SaveAllNewAtoms},
                         {"setinfo",setInfo},
                         {"showinfo",showInfo},
			 {"view",viewJavaApplet},
			 {"mp_channel",setChannelFunc},
                         { NULL,NULL }
                        };

void PrintHelp(void)    /* Opcje uruchomieniowe programu */
 {
    SourceVersion(NULL);                        /* Ostatnia modyfikacja kodu */
    fprintf(stdout,"            command line options:\n");
    fprintf(stdout,"-P [+|-]  - messages on/off (default on)\n"
                   "-M val    - max number of algorithm's iterations\n"
                   "-E val    - stop iterations after explaining val %% of signal's energy\n"
                   "-B file   - run commands from script file\n"
                   "-O val    - create dictionary for signal size val points\n"
                   "-F val    - sampling frequency [Hz]\n"
                   "-C val    - calibration constant, usually [points/microV]\n"
                   "-i [+|-]  - losowa inicjacja slownika randomizowanego (???)\n"
                   "-R val    - use stochastic Gabor dictionary of val atoms\n"
                   "-x val    - anal. function of positions probability distribution in t-f space\n"
                   "-d        - use dyadic structures of scales (2^j) in stochastic dictionary\n"
                   "-f        - fast update of scalar products(???)\n"
                   "-v [+|-]  - very fast update of scalar products (???)\n"
                   "-s        - slower mode using less RAM\n"
                   "-Nn,o,f,p - initialization of Monte-Carlo heuristic\n"
                   "-T        - use dyadic dictionary\n"
                   "-S val    - oversampling (by val) of the dyadic dictionary\n"
	           "-D val    - adapive dictionary parameter 0..1 (default 0.9)\n"
                   "-H or -h  - hereby displayed screen :-)\n");
    fflush(stdout);
 }

PERMUT permut[256];

static int fcmp(const void *a,const void *b) {
  return strcmp(((PERMUT *)a)->name,((PERMUT *)b)->name);
}

static void sortAlphabetic(void) {
  register int k;

  for(k=0 ; commands[k].command!=NULL ; k++) {
    permut[k].index=k;
    permut[k].name=commands[k].command;
  }

  qsort((void *)permut,k,sizeof(PERMUT),fcmp);
}

void CommandList(void) {
  int i,j,k=1;

  for(i=0 ; commands[i].command!=NULL ; i++,k++) {
    const int index=permut[i].index;

    fprintf(stdout,"[%2d] %-18s ",k,commands[index].command);
    for(j=0 ; help[index][j]!='\n' ; j++)
      fprintf(stdout,"%c",help[index][j]);
    fprintf(stdout,"\n");
    if( (((i+1)%20)==0) && commands[i+1].command!=NULL)
/*zmiana PJD 99.07.14*/
      pause();
  }
}

static int AllocStatus=OFF;
extern UCHAR FunctionCode[];
extern int InicRandom(int),RandomType,DictionSize;
extern int MakeMallatDictionary(int,int,int);

void ReInitBaseSize(int NewDimBase,int NewOverSampling)
  {
    const int OldPrn=prn;                    /* Ustalenie nowego rozmiaru bazy */
    register int i,AllDim;

    prn=OFF;
    if(AllocStatus==ON)
      {
        CloseRand2D();                       /* Zwolnienie poprzednich zasobow */
        free((void *)sygnal);
        free((void *)OrgSygnal);
        CloseTune();
      }

    DimBase=NewDimBase;
    OverSampling=NewOverSampling;
    SetTuneScale();                         /* Alokcja nowych zasobow */
    if(MallatDiction==ON)
      {
        DictionSize=MakeMallatDictionary(DimBase,OverSampling,OFF);
        if(Heuristic==ON)                       /* Poprawnosc konfiguracji */
          {
            if(StartDictionSize>=DictionSize)
              Heuristic=OFF;
            else if(FastMode==OFF)
              Heuristic=OFF;
            else if(DiadicStructure==OFF)
              Heuristic=OFF;
          }
      }

    if(Heuristic==OFF)
      StartDictionSize=DictionSize;

    if(InicRandom(OFF)==-1)
      {
        fprintf(stderr,"Problems opening randomized dictionary !\n");
        exit(EXIT_FAILURE);
      }

    if((sygnal=MakeVector(AllDim=3*DimBase))==NULL ||
       (OrgSygnal=MakeVector(DimBase))==NULL)
      {
        fprintf(stderr,"Memory allocation error (main) !\n");
        exit(EXIT_FAILURE);
      }

    for(i=0 ; i<AllDim ; i++)
      sygnal[i]=0.0F;
    for(i=0 ; i<DimBase ; i++)
      OrgSygnal[i]=0.0F;
    prn=OldPrn;
  }

#ifdef MULTITHREAD
#include <signal.h>
#endif

int javaMode=OFF;

int main(int argc,char *argv[])
 {
   char filename[STRING],*path;
   int opcja,TimeSrand=OFF,i,makebatch=OFF,OldPrn;

#ifdef MULTITHREAD
   /* UWAGA !
      Blokowanie sygnalu SIGCLD zapobiega powstawaniu procesow ZOMBI
      w przypadku wczesniejszego zakonczenia procesu potomnego
      konstrukcja nieprzenosna dziala pod Linux'em
   */

   if(signal(SIGCLD,SIG_IGN)==SIG_ERR)
     {
       fprintf(stderr,"Problems trapping signal SIGCLD !\n");
       return 1;
     }
#endif

   sortAlphabetic();            /* Sortowanie polecen w kolejnosci alfabetycznej */
   OldDimRoz=32;                /* Liczba atomow do rekonstrukcji */
   DimBase=512;                 /* Wymiar sygnalu */
   epsylon=95.0F;               /* Dokladniosc rekonstrukcji */
   prn=ON;                      /* Drukowanie informacji */
   SamplingRate=1.0;            /* Czestotliwosc probkowania */
   ConvRate=1.0;                /* Wspolczynnik konwersji */
   RandomType=NOFUNCRND;        /* Sposob generacji atomow */
   DiadicStructure=OFF;         /* Diadyczna struktura slownika */
   FastMode=ON;                 /* Szybka generacja atomow (wymaga duzo pamieci) */
   VeryFastMode=ON;             /* Bardzo szybka generacja iloczynow skalarnych */
   DictionSize=70000;           /* Domyslny rozmiar slownika */
   Heuristic=OFF;               /* Wspomaganie heurystyczne */
   StartDictionSize=25000;      /* Rozmiar slownika do probkowania */
   MallatDiction=OFF;           /* Slownik Mallata */
   OverSampling=2;              /* Przeprobkowanie slownika */

   while((opcja=Getopt(argc,argv,"R:P:E:M:HhB:O:F:C:i:x:dsfN:TS:v:JD:X"))!=EOF)
      switch(opcja) {
        case 'X':
	      debug=1;
	      break;
        case 'S':
              OverSampling=atoi(optarg);
        case 'T':
              MallatDiction=ON;
              DiadicStructure=ON;
              break;
        case 'N':
              Heuristic=ON;     /* Pozostale parametry domyslne w mp.c */
              sscanf(optarg,"%d,%d,%d,%d",&StartDictionSize,&ROctave,
                                          &RFreqency,&RPosition);
              break;
        case 'v':
              if(strcmp(optarg,"+")==0)
                FastMode=VeryFastMode=ON;
              else if(strcmp(optarg,"-")==0)
                VeryFastMode=OFF;
              else
                {
                  fprintf(stderr,"Available options -v[-|+] !\n");
                  return 1;
                }
              break;
        case 'P':
              if(strcmp(optarg,"+")==0)
                prn=ON;
              else if(strcmp(optarg,"-")==0)
                prn=OFF;
              else
                {
                  fprintf(stderr,"Available options -P[-|+] !\n");
                  return 1;
                }
              break;
        case 'f':
              FastMode=ON;
              break;
        case 's':
              FastMode=OFF;
              break;
        case 'd':
	      DiadicStructure=ON;
              break;
        case 'R':
              DictionSize=atoi(optarg);
              break;
        case 'x':
              (void)strcpy((char *)FunctionCode,optarg);
              RandomType=FUNCRND;
              MallatDiction=OFF;
              break;
        case 'i':
                if(strcmp(optarg,"+")==0)
                   TimeSrand=ON;
                 else if(strcmp(optarg,"-")==0)
                         TimeSrand=OFF;
                      else
                        {
                          fprintf(stderr,"Available options -i[-|+] !\n");
                          return 1;
                        }
                 break;
        case 'O':
                DimBase=atoi(optarg);
                break;
        case 'F':
                 SamplingRate=atof(optarg);
                 break;
        case 'C':
                 ConvRate=atof(optarg);
                 break;
        case 'B':
                 (void)strcpy(filename,optarg);
                 makebatch=ON;
                 break;
        case 'M':
                OldDimRoz=atoi(optarg);
                break;
        case 'E':
                epsylon=atof(optarg);
                break;
        case 'J':  /* Wylaczenie buforowania */
	         javaMode=ON;
                 setvbuf(stdout,(char *)NULL,_IOLBF,0);
                 setvbuf(stderr,(char *)NULL,_IOLBF,0);
                 break;
        case 'D':
	         AdaptiveConst=(float)atof(optarg);
	         break;
        case 'H':
        case 'h':
        default:
                PrintHelp();
                return 0;
      }

  SetTuneScale();
  OldPrn=prn;                           /* Blokowanie drukowanie inforamcji */
  prn=OFF;
  if(MallatDiction==ON)
    DictionSize=MakeMallatDictionary(DimBase,OverSampling,OFF);

  if((path=getenv("MP_PATH"))==NULL)       /* Sciezka dla batch'y  */
   {
     char path2[10]=".";
     if(prn==ON)
/*PJD        fprintf(stderr,"\nWARNING: environment variable MP_PATH not set !\n");*/
        fprintf(stderr,"\nPath set to \".\" (HMPP env. var. not set)\n");
     SetBatchPath(path2);
   }
  else SetBatchPath(path);

  if(Heuristic==ON)                     /* Poprawnosc konfiguracji */
   {
     if(StartDictionSize>=DictionSize)
       Heuristic=OFF;
     else if(FastMode==OFF)
       Heuristic=OFF;
     else if(DiadicStructure==OFF)
       Heuristic=OFF;
   }

  if(Heuristic==OFF)
    StartDictionSize=DictionSize;

  if(prn==ON && RandomType==FUNCRND)
    fprintf(stdout,"VAL= %s\n",FunctionCode);

  if(InicRandom(TimeSrand)==-1)
   {
    fprintf(stderr,"Problem initializing stochastic dictionary:(\n");
     return 1;
   }

  if((sygnal=MakeVector(3*DimBase))==NULL ||
      (OrgSygnal=MakeVector(DimBase))==NULL)
    {
      fprintf(stderr,"Memory allocation error (main:(\n");
      return 1;
    }

   for(i=0 ; i<3*DimBase ; i++)
     sygnal[i]=0.0F;
   for(i=0 ; i<DimBase ; i++)
     OrgSygnal[i]=0.0F;

   AllocStatus=ON;
   if(Batch(commands,"mp.cfg",NULL)==-1)      /* Plik konfiguracyjny */
     {
       char name[STRING];
       strcmp(name,BatchPath); strcat(name,"mp.cfg");
       if(Batch(commands,name,NULL)==-1)
         if(prn==ON)
           fprintf(stderr,"\nNo config file (mp.cfg)\n");
     }

   prn=OldPrn;                          /* mozna ustawic prn */
   if(makebatch==ON)
     if(Batch(commands,filename,NULL)==-1)      /* Wykonanie skryptu */
      {
        fprintf(stderr,"cannot find script file %s\n",filename);
        return 1;
      }

   if(prn==ON)
    {
     SourceVersion(NULL);
     SetMPP("");
    }

   Shell(commands,help);        /* Rozpoczecie trybu iteracyjnego */
   CloseRand2D();
   free((void *)sygnal);
   free((void *)OrgSygnal);
   CloseTune();
   return 0;
 }
