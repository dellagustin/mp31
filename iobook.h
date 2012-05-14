	/* Operacje IO na ksizce 1996 03 04/31 05/01, 12 10/14 */

#ifndef IOBOOK_H
#define IOBOOK_H
				/* Wersja zmodernizowana */
typedef struct {
 short file_offset;
 short book_size;
 int signal_size;
 float points_per_micro_V;
 float FREQUENCY;
 float signal_energy;
 float book_energy;
 } HEADER;

typedef struct {			/* Nowy  ATOM  */
 short number_of_atom_in_book;
 unsigned char octave;
 unsigned char type;
 short frequency;
 short position;
 float modulus;
 float amplitude;
 float phase;
 } ATOM;

#ifndef NOPROTOTYP
#ifdef INTELSWP
float unix_float_to_dos(char *);
void dos_float_to_unix(float,char *);
#endif
int WriteHeader(HEADER *,FILE *);		/* Zapis naglowka ksiazki */
int ReadHeader(HEADER *,FILE *);		/* Odczyt --//--   ---//-- */
int WriteAtom(ATOM *,FILE *);			/* Zapis atomu */
int ReadAtom(ATOM *,FILE *);                    /* Odczyt atomu */
int SetActualBook(int,FILE *);                  /* Ustawienie pozycji w zbiorze ksiazek */
int LicznikKsiazek(char *);			/* Ustalenie liczby ksiazek w pliku */
#endif
#endif



