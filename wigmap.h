/* Czytane ksiaki + generacja mapy wignera (1996 03 01) 1997 07 02 */
/* 1997 11 22 */        

#ifndef WIGMAP_H
#define WIGMAP_H

#include "typdef.h"

#define GENBITMAP    1
#define NOGENBITMAP  0

void MakeWignerMap(PSBOOK *,int,int,int,int,UCHAR **,
                   float,float,char *,int,float);  /* Kreacja mapy Wignera */
void MakeDyspersWignerMap(PSBOOK *,int,int,int,int,UCHAR **,
                          float,float,char *,int,float); /* Kreacja mapy Dyspersji Wignera */                   
UCHAR **MakeTableUChar(int,int);		   /* Tworzenie tablicy UCHAR */
void FreeTableUChar(UCHAR **,int);                 /* Zwalniane Tablicy */
void read_book(void);				   /* Czytanie ksiazki z pliku */

#endif
