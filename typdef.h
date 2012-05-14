		/* DEFINICJE TYPOW 1996 01 22/05 01 1997 01 05 */

#ifndef TYPEDEF_H
#define TYPEDEF_H

typedef unsigned char  UCHAR;
typedef unsigned long  ULONG;

#define PARSIZE 3		/* Liczba parametrow atomow */

typedef struct {
  float Energia,param[PARSIZE],waga,phase,amplitude;
  int numer;
} BOOK;				/* Ksizka z rozkladem (reprezentacja wewnetrzna) */

typedef struct {
   float s,t,f,w,amplitude;
 } PSBOOK;		/* Wewnetrzna struktura ksiazki od read_book() */

#endif

