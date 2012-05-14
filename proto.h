/* Prototypy funkcji 1997 04 02; 1997 05 28; 1997 10 04 1998 01 01 */

#ifndef PROTO_H
#define PROTO_H

#include "typdef.h"

extern void ReadSignalBinary(char *);	/* Odczyt sygnalu binarnrgo */
extern void WigMapPs(char *);           /* Drukowanie mapy wignera */
extern void Quit(char *);	        /* Opuszczenie programu exit(0) */
extern void MakeCanalEEG(char *);	/* Analiza calego (pojedynczego) kanalu EEG */
extern void ReadFloatSignal(char *);	/* Ladowanie sygnalu binarnego w postaci float */
extern void RottCanalEEG(char *);	/* ----//-----//----//--- z dynamicznymi warunkami brzegowymi */
extern void FloatRottCanalEEG(char *);  /* ----//-----//----//-------//--//-- dla float'ow */
extern void FloatCanalEEG(char *);	/* Kanal EEG typu float z symetrycznymi warunkami brzegowymi */
extern void WigToGif(char *);		/* Generacja mapy Wignera do pliku postaci gif'a */
extern void GetComment(char *);		/* Pobranie komntarza dla gif'a */
extern void ShowComment(void);		/* Wyswietlenie komentarza */
extern void CommandList(void);		/* Wyswietlenie wszystkich komend zewnetrznych */
extern float *MakeVector(int);
extern int Getopt(int,char **,char *);
extern int ReadSygnal(char *,int);
extern void PrintHelp(void);
extern void Load(char *);
extern void SetMPP(char *);
extern void PrintBook(void);
extern void TypeSignal(char *);
extern void Reset(char *);
extern void LoadBook(char *);
extern void LoadNewBook(char *);
extern void WriteAllSignal(char *);
extern void SourceVersion(char *);
extern void AnalizaMP(char *);
extern void SaveAllAtoms(char *);
extern void SaveAllNewAtoms(char *);
extern UCHAR Log2(float);
extern void RestartDiction(char *);
extern void AscSaveAllAtoms(char *);
extern void AscLoadBook(char *);
extern void ShowDictionary(char *);
extern void SetName(char *); 
extern void FileWignerMap(char *);
extern void SigReconst(char *);
extern void SetBatchPath(char *);
extern void TuneOctave(char *);
extern void ResetDyst(void);
extern void SetTuneScale(void);
extern void CloseTune(void);
extern void Norm(char *);
extern void SpectrumFFT(char *);
extern void ShowDyst(char *);
extern void InicRandom1D(void);
extern void Echo(char *);
extern char *trim(char *);
extern void addPath(char *);
extern void setInfo(char *);
extern void showInfo(char *);
extern void viewJavaApplet(char *);

extern int opterr,optind,optopt,sp,DimBase,Compute,DiadicStructure,
           prn,dimroz,OldDimRoz,file_offset,DictionSize,FastMode,prninfo,
           StartDictionSize,Heuristic,ROctave,RFreqency,RPosition,
           MallatDiction,OverSampling,VeryFastMode;
extern int ChannelMaxNum,ChannelNumber;     
extern char *optarg,outname[],BatchPath[];
extern float epsylon,*OrgSygnal,*sygnal,SamplingRate,ConvRate,E0,Ec,
             *OctaveDyst,AdaptiveConst;
extern BOOK *book;

#define FUNCRND   1	/* Slownik z zadana funkcja p-stwa */
#define NOFUNCRND 2	/* Slownik z rozkladem rownomiernym */

                  /* None ANSI */

#ifndef M_E
#define M_E		2.7182818284590452354	/* e */
#endif

#ifndef M_LOG2E
#define M_LOG2E	        1.4426950408889634074	/* log_2 e */
#endif

#ifndef M_LOG10E
#define M_LOG10E	0.43429448190325182765	/* log_10 e */
#endif

#ifndef M_LN2
#define M_LN2		0.69314718055994530942	/* log_e 2 */
#endif

#ifndef M_LN10
#define M_LN10		2.30258509299404568402	/* log_e 10 */
#endif

#ifndef M_PI
#define M_PI		3.14159265358979323846	/* pi */
#endif

#ifndef M_PI_2
#define M_PI_2		1.57079632679489661923	/* pi/2 */
#endif

#ifndef M_PI_4
#define M_PI_4		0.78539816339744830962	/* pi/4 */
#endif

#ifndef M_1_PI
#define M_1_PI		0.31830988618379067154	/* 1/pi */
#endif

#ifndef M_2_PI	
#define M_2_PI		0.63661977236758134308	/* 2/pi */
#endif

#ifndef M_2_SQRTPI
#define M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
#endif

#ifndef M_SQRT2
#define M_SQRT2	        1.41421356237309504880	/* sqrt(2) */
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */
#endif

#endif









