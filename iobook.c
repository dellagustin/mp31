/* Operacje wejscia-wyjscia na ksizkach 1996 03 04/31; 05 01 08 26
   Wszystkie operacje wykonywane sa w standarcie IBM RS6000
   Wersja zmodyfikowana (zmiana struktury zapisu) 1996 12 10/14
   Poprawka 1997 04 29, 1998 01 06 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iobook.h"

unsigned long NumberOfAllWaveForms;
extern int prn;

extern char *trim(char *);

#ifdef INTELSWP

#ifndef __BORLANDC__
#ifndef __USE_XOPEN
#define __USE_XOPEN
#include <unistd.h>
#undef __USE_XOPEN 
#else
#include <unistd.h>
#endif
#endif

float unix_float_to_dos(char *f)
 {
  short isav;
  union	{
	 char fc[4];
	 short fsi[2];
	 float ff;
  } gf[2];

  (void)memcpy((void *)gf[0].fc,(void *)f,4);
  swab((void *)gf[0].fc,(void *)gf[1].fc,4);
   isav=gf[1].fsi[0];
   gf[1].fsi[0]=gf[1].fsi[1]; gf[1].fsi[1]=isav;
   return gf[1].ff;
 }

int unix_int_to_dos(char *f)
 {
  short isav;
  union	{
    char fc[4];
    short fsi[2];
    int ff;
  } gf[2];

   (void)memcpy((void *)gf[0].fc,(void *)f,4);
   swab((void *)gf[0].fc,(void *)gf[1].fc,4);
   isav=gf[1].fsi[0];
   gf[1].fsi[0]=gf[1].fsi[1]; gf[1].fsi[1]=isav;
   return gf[1].ff;
 }

void dos_float_to_unix(float f,char *ret)
 {
  short isav;
  union {
    char fc[4];
    short fsi[2];
    float ff;
  } gf[2];

  gf[0].ff=f;
  swab((void *)gf[0].fc,(void *)gf[1].fc,4);
  isav=gf[1].fsi[0];
  gf[1].fsi[0]=gf[1].fsi[1]; gf[1].fsi[1]=isav;
  (void *)memcpy((void *)ret,(void *)&gf[1].ff,4);
 }
 
void dos_int_to_unix(int f,char *ret)
 {
  short isav;
  union {
    char fc[4];
    short fsi[2];
    int ff;
  } gf[2];

  gf[0].ff=f;
  swab((void *)gf[0].fc,(void *)gf[1].fc,4);
  isav=gf[1].fsi[0];
  gf[1].fsi[0]=gf[1].fsi[1]; gf[1].fsi[1]=isav;
  (void *)memcpy((void *)ret,(void *)&gf[1].ff,4);
 }

#endif

int WriteHeader(HEADER *head,FILE *plik)	       /* Zapis naglownka */
 {
#ifdef INTELSWP
   swab((char *)&head->file_offset,(char *)&head->file_offset,2);
   dos_int_to_unix(head->signal_size,(char *)&head->signal_size);
   swab((char *)&head->book_size,(char *)&head->book_size,2);
   dos_float_to_unix(head->signal_energy,(char *)&head->signal_energy);
   dos_float_to_unix(head->book_energy,(char *)&head->book_energy);
   dos_float_to_unix(head->points_per_micro_V,(char *)&head->points_per_micro_V);
   dos_float_to_unix(head->FREQUENCY,(char *)&head->FREQUENCY);
#endif
   if(fwrite((void *)head,sizeof(HEADER),1,plik)==0U) 
     return -1;
   return 0;
 }

int WriteAtom(ATOM *atom,FILE *plik)                  /* Zapis atomu */
 {
#ifdef INTELSWP
   swab((char *)&atom->number_of_atom_in_book,(char *)&atom->number_of_atom_in_book,2);
   swab((char *)&atom->frequency,(char *)&atom->frequency,2);
   swab((char *)&atom->position,(char *)&atom->position,2);
   dos_float_to_unix(atom->modulus,(char *)&atom->modulus);
   dos_float_to_unix(atom->amplitude,(char *)&atom->amplitude);
   dos_float_to_unix(atom->phase,(char *)&atom->phase);
#endif
   if(fwrite((void *)atom,sizeof(ATOM),1,plik)==0U) 
     return -1;
   return 0;
 }

int ReadHeader(HEADER *head,FILE *plik)		       /* Odczyt naglownka */
 {
   if(fread((void *)head,sizeof(HEADER),1,plik)==0U)
     return -1;
#ifdef INTELSWP
   swab((char *)&head->file_offset,(char *)&head->file_offset,2);
   head->signal_size=unix_int_to_dos((char *)&head->signal_size);
   swab((char *)&head->book_size,(char *)&head->book_size,2);
   head->signal_energy=unix_float_to_dos((char *)&head->signal_energy);
   head->book_energy  =unix_float_to_dos((char *)&head->book_energy);
   head->points_per_micro_V=unix_float_to_dos((char *)&head->points_per_micro_V);
   head->FREQUENCY=unix_float_to_dos((char *)&head->FREQUENCY);
#endif
   return 0;
 }

int ReadAtom(ATOM *atom,FILE *plik)		       /* Odczyt atomu */
 {
   if(fread((void *)atom,sizeof(ATOM),1,plik)==0U)
     return -1;
#ifdef INTELSWP
   swab((char *)&atom->number_of_atom_in_book,(char *)&atom->number_of_atom_in_book,2);
   swab((char *)&atom->frequency,(char *)&atom->frequency,2);
   swab((char *)&atom->position,(char *)&atom->position,2);
   atom->modulus=unix_float_to_dos((char *)&atom->modulus);
   atom->amplitude=unix_float_to_dos((char *)&atom->amplitude);
   atom->phase  =unix_float_to_dos((char *)&atom->phase);
#endif
   return 0;
 }

int SetActualBook(int ktory,FILE *plik)
 {
   HEADER head;				/* Ustawienie nowej pozycji w ksiazce */
   long poz=0L;
   int i=0;

   rewind(plik);
   while(i<ktory)
    {
      if(ReadHeader(&head,plik)==-1)
	return -1;
      poz+=(long)head.book_size*(long)sizeof(ATOM)+(long)sizeof(HEADER);
      if(fseek(plik,poz,SEEK_SET)!=0)
	return -1;
      i++;
    }
  return 0;
 }

int LicznikKsiazek(char *opt)		/* Zliczenie ksiazek zawartych */
 {                                      /* w pliku */
   HEADER head;
   FILE *plik;
   int i=0;
   long poz=0L;

   NumberOfAllWaveForms=0UL;
   if((plik=fopen(trim(opt),"rb"))==0)
    {
      fprintf(stderr,"Brak pliku %s !\n",opt);
      return -1;
    }

   for( ; ; )
    {
      if(ReadHeader(&head,plik)==-1)
	break;
      poz+=(long)head.book_size*(long)sizeof(ATOM)+(long)sizeof(HEADER);
      NumberOfAllWaveForms+=(unsigned long)head.book_size;
      if(fseek(plik,poz,SEEK_SET)!=0)
	break;
      i++;
    }

   if(prn==1)
     fprintf(stdout,"LICZBA ZESTAWOW ATOMOW W PLIKU    : %d\n"
   		    "LICZBA WSZYSTKICH ATOMOW W PLIKU  : %lu\n",
		    i,NumberOfAllWaveForms);
   fclose(plik);
   return i;
 }
 
