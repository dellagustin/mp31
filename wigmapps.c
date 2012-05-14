/*
     Wydruk mapy Wignera adaptacja do HMPP 1996 03 23/26
     Wersja PostScript i plik binarny.
     1997 05 21,27  1997 07 02; 1998 01 01 
     1998 03 10;
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "wigmap.h"
#include "shell.h"
#include "proto.h"

#define Y_Page_Size 612
#define X_Page_Size 792
#define X_margin 19
#define Y_margin 35
#define X_left_image_corner 15
#define Y_bottom_image_corner 100
#define Image_width 775
#define Image_height 445
#define Signal_height 55
#define Signal_line_width 0.2
#define Axis_line_width 0.001
#define Axis_thick_line_width 0.3
#define Y_bottom_signal 20
#define X_comment 200
#define Y_comment 0
#define X_raw_image_pixels 500
#define Y_raw_image_pixels 500
#define X_image_pixels 497
#define Y_image_pixels 497
#define X_left_image_frame 1
#define Y_top_image_frame 1
#define MAX_SIGNAL_SIZE_IN_POINTS 128000	/* Uwaga na te liczbe */
#define MAX_NUMBER_OF_WAVES       64000
#define error(NAP) fprintf(stderr,"%s\n",NAP)

extern char outname[],SSignalname[]; 	/* Nazwa ksiazki i sygnalu */
extern int Getopt(int,char **,char *);  /* Pobranie opcji z hmpp */
extern char *optarg;                    /* Wskaznik dla tablicy opcji */
extern int opterr,optind,sp;            /* Zmienne pomocnicze Getopt */
extern BOOK *book;			/* Aktualna ksizka w hmpp */
extern float E0,Ec;                     /* Energia sygnalu i ksiazki */
extern int dimroz;			/* Status obliczen wymiar rozwiazania */
extern unsigned char Log2(float);
extern int file_offset,prn;
extern float *OrgSygnal;		/* zbior z orginalnym sygnalem */
extern int LogarytmStat;		/* Drukowanie mapy logarytmicznej (wigmap) */

static float Modulus[MAX_NUMBER_OF_WAVES],Phase[MAX_NUMBER_OF_WAVES],
	     sig[MAX_SIGNAL_SIZE_IN_POINTS],FREQ,bok_norm,sig_norm,energy_percent;

static int NUM=2048,FREQ_split=1,Freq[MAX_NUMBER_OF_WAVES],Pos[MAX_NUMBER_OF_WAVES],
	   Octave[MAX_NUMBER_OF_WAVES],num_of_wav,sig_size,true_signal_size=0,
	   BLOBS_NUMBERED=1,REVERSE_SIGNAL_ON_PLOT=0,font_size=8,line_height=12,
	   num_of_columns=2,points_per_microvolt=-1,MAX_PAGES=32000;

static UCHAR **ImageTab;
static PSBOOK *psbook;
static FILE *FO;
static char date_str[256],out_string[2048],str1[256],str2[256],str3[256],
	    bok_filename[256],comment[256],comment1[256],Comment_str[256];

static char *date(void)
 {
   time_t timer;

   timer=time(NULL);  			 /* gets time of day */
   return asctime(localtime(&timer));
 }

static void write_header(void)
 {
    fprintf(FO, "%%!PS-Adobe-2.0 EPSF-1.2\n");
    fprintf(FO, "%%%%CreationDate: %s\n",date_str);
    fprintf(FO, "%%%%BoundingBox: 0 0 %d %d\n",X_Page_Size, Y_Page_Size);
    fprintf(FO, "%%%%EndComments\n");
    fprintf(FO, "/lt { lineto } def\n/mt { moveto } def\n");
    fprintf(FO, "/oshow { true charpath stroke } def\n\n");
 }

static void set_landscape(void)
 {
    fprintf(FO, "90 rotate\n0 %d translate\n", -Y_Page_Size);
 }

static void bit_image_printing(float GammaCorect,float SplitFactor,
            float Crop,int Dyspers)
 {
   int x,y,i,freq_step;
   float xf,yf,xf1,FREQ_2;

   if((ImageTab=MakeTableUChar(X_raw_image_pixels,X_raw_image_pixels))==NULL)
    {
      error("No memory !");
      return;
    }

    fprintf(stderr,"\n<<< GENERACJA MAPY WIGNERA >>>\n");
    if(Dyspers==0)
       MakeWignerMap(psbook,num_of_wav,X_raw_image_pixels,
                     X_raw_image_pixels,true_signal_size,ImageTab,
                     GammaCorect,SplitFactor,NULL,NOGENBITMAP,Crop);
    else                 
      MakeDyspersWignerMap(psbook,num_of_wav,X_raw_image_pixels,
                           X_raw_image_pixels,true_signal_size,ImageTab,
                           GammaCorect,SplitFactor,NULL,NOGENBITMAP,Crop);
                           
    fprintf(stderr,"<<< PostScript >>>\n");
    fprintf(FO, "\n\ngsave\n%d %d translate\n", X_left_image_corner+X_margin,
						Y_bottom_image_corner+Y_margin );
    fprintf(FO, "%d %d scale\n\n", Image_width, Image_height );
    fprintf(FO, "%d %d 8\n", X_image_pixels, Y_image_pixels );
    fprintf(FO, "[%d 0 0 %d 0 %d]\n", X_image_pixels, -Y_image_pixels,
				      Y_image_pixels);
    fprintf(FO, "{currentfile\n%d string readhexstring pop}\nimage\n",
		 X_image_pixels);
    fprintf(FO, "\n");

    for(x=Y_image_pixels-1; x>=0; x--)
      {
	for(y=X_left_image_frame; y<X_image_pixels+X_left_image_frame; y++)
	  fprintf(FO, "%02X", 255-(int)ImageTab[y][x]);
	fprintf(FO, "\n");
      }
      
    FreeTableUChar(ImageTab,X_raw_image_pixels);
    fprintf(FO, "grestore\ngsave\n");

/******************** printing numbers on a map *************/

if(BLOBS_NUMBERED==1)
  {
     fprintf(FO, "%d %d translate\n", X_left_image_corner+X_margin,
				      Y_bottom_image_corner+Y_margin);
     strcpy(out_string, "out: ");
     fprintf(FO, "\n%% numbers on a map\n");
     fprintf(FO, "\n/Helvetica findfont 4 scalefont setfont\n");
     fprintf(FO, "1 setlinewidth\n");

     for(i=num_of_wav-1; i>=0; i--)
	{
	  if(SplitFactor*Freq[i] > sig_size/(2*FREQ_split) )
	    {
	      sprintf(str1, "%d ", i);
	      strcat(out_string, str1);
	    }
	  else
	   {
	     xf=(float)Pos[i]*(float)Image_width/(float)true_signal_size;
	     yf=SplitFactor*(float)Freq[i]*(float)Image_height*
                2.0F*FREQ_split/((float)sig_size);
	     fprintf(FO, "%.3f %.3f mt\n", xf, yf);
	     fprintf(FO, "\ngsave 1 setgray\n(%d) oshow grestore\n", i);
	     fprintf(FO, "(%d) show\n", i);
	   }
       }
  }

  fprintf(FO,"\ngrestore\n");

	/************* frequency axis for map **********/
  if(FREQ>0)
    {
      fprintf(FO, "\n%% frequency axis for map\n");
      fprintf(FO, "\n\ngsave\n%d %d translate\n", X_margin,
						  Y_bottom_image_corner+Y_margin);

      fprintf(FO, "%.3f setlinewidth\n", Axis_thick_line_width);
      FREQ_2=FREQ/(2*FREQ_split);
      if(FREQ_2<50)
	freq_step=1;
      else if(FREQ_2<100)
	     freq_step=2;
      else if(FREQ_2<500)
	     freq_step=10;
      else freq_step=20;

      for(i=0; i<=FREQ_2; i+=freq_step)
       {
	  xf=i*(float)Image_height/FREQ_2;
	  fprintf(FO, "0 %.3f mt\n", xf);
	  fprintf(FO, "10 %.3f lt\n", xf);
       }
      fprintf(FO, "stroke\n");

      fprintf(FO, "%.3f setlinewidth\n", Axis_line_width);
      fprintf(FO, "10 0 mt\n");
      fprintf(FO, "10 %d lt\n", Image_height);
      fprintf(FO, "stroke\n/Helvetica findfont 8 scalefont setfont\n");

      for(i=0; i<FREQ_2; i+=freq_step)
	{
	  xf=(float)i*(float)Image_height/FREQ_2+1;
	  fprintf(FO, "0 %.3f mt\n", xf);
	  fprintf(FO, "(%d) show\n", i);
	}
      fprintf(FO, "\ngrestore");
  }

/****************** right axis *****************/
   fprintf(FO, "\n%% right axis\n");
   if(sig_size>0)
     {
	fprintf(FO, "\n\ngsave\n%d %d translate\n", X_margin + X_Page_Size+2,
						Y_bottom_image_corner+Y_margin);
	fprintf(FO, "%.3f setlinewidth\n", Axis_thick_line_width);
	FREQ_2=sig_size/(2*FREQ_split);
	for(xf1=0; xf1<=FREQ_2; xf1+=(float)FREQ_2/16.0)
	 {
	    xf=xf1*(float)Image_height/FREQ_2;
	    fprintf(FO, "0 %.3f mt\n", xf);
	    fprintf(FO, "10 %.3f lt\n", xf);
	 }
	fprintf(FO, "stroke\n");
	for(xf1=0; xf1<FREQ_2; xf1+=(float)FREQ_2/64.0F)
	  {
	    xf=xf1*(float)Image_height/FREQ_2;
	    fprintf(FO, "0 %.3f mt\n", xf);
	    fprintf(FO, "6 %.3f lt\n", xf);
	  }
	fprintf(FO, "%.3f setlinewidth\n", Axis_line_width);
	fprintf(FO, "0 0 mt\n");
	fprintf(FO, "0 %d lt\n", Image_height);
	fprintf(FO, "stroke\n/Helvetica findfont 6 scalefont setfont\n");
	for(xf1=0; xf1<FREQ_2; xf1+=(float)FREQ_2/16.0F)
	  {
	    xf=xf1*(float)Image_height/FREQ_2+1.5F;
	    fprintf(FO, "1 %.3f mt\n", xf);
	    fprintf(FO, "(%.0f) show\n", xf1);
	  }
     }

     fprintf(FO, "\ngrestore\n");
    /****************** bottom axis *************************/

     fprintf(FO, "\n%% bottom axis\n");
     if(true_signal_size>0)
       {
	  fprintf(FO, "\n\ngsave\n%d %d translate\n", X_left_image_corner+X_margin,
						Y_bottom_image_corner+Y_margin-15);
	  fprintf(FO, "%.3f setlinewidth\n", Axis_thick_line_width);
	  FREQ_2=true_signal_size;
	  for(xf1=0; xf1<=FREQ_2; xf1+=(float)FREQ_2/16.0F)
	    {
	       xf=xf1*(float)Image_width/FREQ_2;
	       fprintf(FO, "%.3f 0 mt\n", xf);
	       fprintf(FO, "%.3f 10 lt\n", xf);
	    }
	  fprintf(FO, "stroke\n");
	  fprintf(FO, "%.3f setlinewidth\n", Axis_line_width);
	  for(xf1=0; xf1<FREQ_2; xf1+=(float)FREQ_2/64.0F)
	   {
	     xf=xf1*(float)Image_width/FREQ_2;
	     fprintf(FO, "%.3f 4 mt\n", xf);
	     fprintf(FO, "%.3f 10 lt\n", xf);
	   }
	  fprintf(FO, "0 setlinewidth\n");
	  for(i=0; i<FREQ_2; i+=2)
	    {
	      xf=(float)i*(float)Image_width/FREQ_2;
	      fprintf(FO, "%.3f 9.5 mt\n", xf);
	      fprintf(FO, "%.3f 10 lt\n", xf);
	    }

	  fprintf(FO, "stroke\n/Helvetica findfont 5 scalefont setfont\n");
	  for(xf1=0; xf1<=FREQ_2; xf1+=(float)FREQ_2/16.0F)
	    {
	      xf=xf1*(float)Image_width/FREQ_2+1.0F;
	      fprintf(FO, "%.3f 0 mt\n", xf);
	      fprintf(FO, "(%.0f) show\n", xf1);
	    }

	  fprintf(FO, "grestore\n");
     }
}

static void vector_signal_printing(void)
 {
   int i;
   float Sig_min,Sig_max,Sig_step,Sig_ampl,xf,yf;

   if(REVERSE_SIGNAL_ON_PLOT==1)
     for(i=0; i<NUM; i++) sig[i]=-sig[i];

   fprintf(FO, "\n%% vector signal\n");
   fprintf(FO, "\ngsave\n%d %d translate\n", X_left_image_corner+X_margin,
					     Y_bottom_signal+Y_margin );
   fprintf(FO, "%.3f setlinewidth\n", Signal_line_width);

   Sig_max=Sig_min=sig[0];
   Sig_step=Image_width/(float)NUM;
   for(i=0;i<NUM;i++)
     {
       if(sig[i]<Sig_min) Sig_min=sig[i];
       if(sig[i]>Sig_max) Sig_max=sig[i];
     }
   Sig_ampl=Sig_max-Sig_min;
   if(Sig_ampl==0.0F)
    {
      error("flat signal?");
      return;
    }
   fprintf(FO, "0.0 %.3f mt\n", Signal_height*(sig[0]-Sig_min)/Sig_ampl );
   for(i=1; i<NUM; i++)
    {
      xf=(float)i*(float)Sig_step;
      yf=Signal_height*(sig[i]-Sig_min)/Sig_ampl;
      fprintf(FO, "%.3f %.3f lt\n", xf, yf);
    }
   fprintf(FO, "stroke\n");
   fprintf(FO, "\n/Helvetica findfont 4 scalefont setfont\n");
   sprintf(str1, "%.0f", Sig_min);
   fprintf(FO, "%d 0 mt\n(%s) show\n", Image_width, str1);
   sprintf(str1, "%.0f", Sig_max);
   fprintf(FO, "%d %d mt\n(%s) show\n", Image_width, Signal_height, str1);
   if(FREQ>0)
     {
     if(FREQ<sig_size)
	{
	  xf=(float)FREQ*Sig_step;
	  fprintf(FO, "0.0001 setlinewidth\n0 -5 mt 0 1 lt 0 -2 mt %.3f -2 lt %.3f -5 mt %.3f 1 lt stroke\n",
		xf, xf, xf);
	  fprintf(FO, "%.3f 2 mt (1 s) show\n", xf/2 - 3);
	}
     else
       {
	  xf=(float)FREQ*Sig_step/10.0F;
	  fprintf(FO, "0.0001 setlinewidth\n0 -5 mt 0 1 lt 0 -2 mt %.3f -2 lt %.3f -5 mt %.3f 1 lt stroke\n",
			xf, xf, xf);
	  fprintf(FO, "%.3f 2 mt (0.1 s) show\n", xf/2 - 5);
       }
     }
     fprintf(FO,"\ngrestore");
 }

#undef X_Page_Size
#undef Y_Page_Size
#define X_Page_Size 612
#define Y_Page_Size 846
#define top_margin 43
#define top_margin_for_top_line 28
#define bottom_margin 19
#define left_margin 19
#define right_margin 19
#define bottom_line_height 6
#define NUMBER_WIDTH 20
#define MODULUS_WIDTH 35
#define OCTAVE_WIDTH 15
#define POS_WIDTH 22
#define FREQ_WIDTH 30
#define PHASE_WIDTH 35

static void set_helv(int size)
  {
    fprintf(FO, "/Helvetica findfont %d scalefont setfont\n", size);
  }

static void write_book_page_footer(int page, char *filename)
  {
    sprintf(str1, "File '%s' printed: %s", filename, date_str );
    fprintf(FO, "/Helvetica findfont 3 scalefont setfont"
		"\n%d %d mt \n(%s) show\n",left_margin,bottom_margin,str1);
    fprintf(FO, "\n/Times-Roman findfont 4 scalefont setfont"
		"\n%d %d mt \n(%d) show\n",560,bottom_margin,page);
    fprintf(FO, "showpage\n%%%%PageTrailer\n\n");
  }

static void print_book(float SplitFactor)
  {
    int left_number[10], left_modulus[10],left_octave[10], left_pos[10],
	left_freq[10],left_phase[10],col,OFFSET,page_lines,page_num=0,i,
	number;
    const float OldFREQ=FREQ; 

    FREQ*=SplitFactor;
    for(i=0; i<num_of_columns; i++)
      {
	OFFSET=left_margin+i*(X_Page_Size-left_margin-right_margin)/num_of_columns;
	left_number[i]=OFFSET;
	left_modulus[i]=left_number[i]+NUMBER_WIDTH*font_size/6;
	left_octave[i]=left_modulus[i]+MODULUS_WIDTH*font_size/6;
	left_freq[i]=left_octave[i]+OCTAVE_WIDTH*font_size/6;
	left_pos[i]=left_freq[i]+FREQ_WIDTH*font_size/6;
	left_phase[i]=left_pos[i]+POS_WIDTH*font_size/6;
      }

    page_lines=(Y_Page_Size-top_margin-bottom_margin-bottom_line_height)/line_height;
    fprintf(FO, "/Helvetica-Oblique findfont 8 scalefont setfont\n");
    sprintf(str2, "%5.2f%%",100.0*bok_norm/sig_norm);
    strcpy(str3, "");
    sprintf(str1, "%s %s: signal %d points,  num. of waveforms %d, %s of energy",
		   str3,bok_filename,sig_size,num_of_wav,str2);
    fprintf(FO, "%d %d mt  (%s) show\n", left_margin+100, Y_Page_Size-top_margin_for_top_line, str1);

  /* i=0;*/
	fprintf(FO, "0.001 setlinewidth\n");
	number=0;
	while(number<num_of_wav && page_num<MAX_PAGES)
	  {
	set_helv(font_size);
	for(col=0; col<num_of_columns; col++)
	  if (number<num_of_wav)
		 {
			sprintf(str1, "%s", "nr");
			fprintf(FO, "%d %d mt  (%s) show\n", left_number[col],
				Y_Page_Size-line_height-top_margin,str1);

			if(points_per_microvolt<=0)
		 sprintf(str1, "%s", "modulus");
			else
		 sprintf(str1, "%s", "microV");

			fprintf(FO, "%d %d mt  (%s) show\n", left_modulus[col],
				Y_Page_Size-line_height-top_margin,str1);

			sprintf(str1, "%s", "oct.");
			fprintf(FO, "%d %d mt  (%s) show\n", left_octave[col],
				Y_Page_Size-line_height-top_margin,str1);
			if(FREQ>0)
		sprintf(str1, "%s", "freq[Hz]");
			else
		sprintf(str1, "%s", "freq");

			fprintf(FO, "%d %d mt  (%s) show\n",left_freq[col],
				Y_Page_Size-line_height-top_margin,str1);

			sprintf(str1, "%s", "pos");
			fprintf(FO, "%d %d mt  (%s) show\n", left_pos[col],
				Y_Page_Size-line_height-top_margin, str1);

			sprintf(str1, "%s", "phase");
			fprintf(FO, "%d %d mt  (%s) show\n", left_phase[col],
				Y_Page_Size-line_height-top_margin,str1);
			for(i=2; i<=page_lines && number<num_of_wav ; i++, number++)
		{
		  sprintf(str1, "%3d", number);
		  fprintf(FO, "%d %d mt  (%s) show\n", left_number[col],
			  Y_Page_Size-line_height*i-top_margin,str1);

		  if(points_per_microvolt<=0)
			  sprintf(str1, "%.2f", Modulus[number]);
		  else
			  sprintf(str1, "%.2f", Modulus[number]/points_per_microvolt);

		  fprintf(FO, "%d %d mt  (%s) show\n", left_modulus[col],
			  Y_Page_Size-line_height*i-top_margin,str1);

		  sprintf(str1, "%d", Octave[number]);
		  fprintf(FO, "%d %d mt  (%s) show\n", left_octave[col],
			  Y_Page_Size-line_height*i-top_margin, str1);

		  if(FREQ>0)
			 sprintf(str1, "%.1f", (float)FREQ*(float)Freq[number]/(float)sig_size );
		  else
			 sprintf(str1, "%d", Freq[number]);

		  fprintf(FO, "%d %d mt  (%s) show\n", left_freq[col],
			  Y_Page_Size-line_height*i-top_margin,str1);

		  sprintf(str1, "%d", Pos[number]);
		  fprintf(FO, "%d %d mt  (%s) show\n", left_pos[col],
			  Y_Page_Size-line_height*i-top_margin,str1);

		  sprintf(str1, "%-.5f", Phase[number]);
		  fprintf(FO, "%d %d mt  (%s) show\n", left_phase[col],
			  Y_Page_Size-line_height*i-top_margin,str1);

		  fprintf(FO, "%d %d mt\n%d %d lt\n", left_number[col],
			  Y_Page_Size-line_height*i-top_margin-1,
			  left_phase[col]+PHASE_WIDTH,
			  Y_Page_Size-line_height*i-top_margin-1);
			}
	 }
	  fprintf(FO, "stroke\n");
	  page_num++;
	  write_book_page_footer(page_num, bok_filename);
	}
	fprintf(FO, "grestore\n");
	FREQ=OldFREQ;
 }

static void write_comments(void)
  {
    sprintf(Comment_str, "%s %s/%s#%d: %d points, freq. %.1fHz, %d waveforms, %5.2f%% of energy %s",
	    comment,str2,SSignalname,file_offset,sig_size,FREQ,num_of_wav,energy_percent,comment1);

    fprintf(FO, "\n/Times-Roman findfont 10 scalefont setfont\n%d %d mt \n(%s) show\n",
	    X_margin+X_comment, Y_margin+Y_comment, Comment_str );
    sprintf(str1,"Printout: %s", date_str);
    fprintf(FO, "/Helvetica findfont 6 scalefont setfont\n%d %d mt \n(%s) show\n",
	    X_margin+X_Page_Size-100,Y_margin,str1);

    if(strlen(out_string)>strlen("out: "))
	fprintf(FO,"%d %d mt\n(%s) show\n",X_margin+300, Y_margin+9, out_string );
    fprintf(FO, "\n\nshowpage\n\n%%%%PageTrailer\n%%%%Trailer\n");
 }

void WigMapPs(char *opt)
 {
   static char out_filename[256],*argv[50],out_bok_filename[256],
               bookfilename[256];
   float df,GammaCorect=1.0F,SplitFactor=1.0F,Crop=1.0F;
   int opcja,argc,i,AllBook=0,AddSignal=0,Dyspers=0;
   extern PSBOOK *ReadAllAtoms(char *,int *); /* Ladowanie calej ksiazki */
 
   LogarytmStat=0;				/* Tryb wyswietlania logarytmicznego */
   FREQ_split=1;
   true_signal_size=0;				/* Wartosci domyslne */
   REVERSE_SIGNAL_ON_PLOT=0;
   sprintf(out_filename,"%s.ps",SSignalname);
   sprintf(out_bok_filename,"%s.ps",outname);
   FREQ=SamplingRate;
   points_per_microvolt=ConvRate;

   StrToArgv(opt,argv,&argc);
   opterr=optind=0; sp=1;
   while((opcja=Getopt(argc,argv,"NRi:s:g:x:O:B:LF:a:Sk:d"))!=EOF)
     switch(opcja) {
       case 'd':
                Dyspers=1;
                break;
       case 'k':
                Crop=atof(optarg);
                break;
       case 'a':
                (void)strcpy(bookfilename,optarg);
                AllBook=1;
                BLOBS_NUMBERED=0;             /* Nie numerujemy atomow */
                break;
       case 's':
                AddSignal=1;
                break;         
       case 'F':
                GammaCorect=atof(optarg);
                break;
       case 'N':
		BLOBS_NUMBERED=0;
		break;
       case 'R':
		REVERSE_SIGNAL_ON_PLOT=1;
		break;
		/*
       case 'f':
		FREQ=atof(optarg);
		break;
		*/
       case 'i':
		font_size=atoi(optarg);
		line_height=3*font_size/2;
		break;
       case 'S':
                SplitFactor=atof(optarg);
		break;
		/*
       case 'p':
		points_per_microvolt=(int)atof(optarg);
		break;
		*/
       case 'g':
		MAX_PAGES=atoi(optarg);
		break;
       case 'x':
		(void)strcpy(comment1,optarg);
		break;
       case 'O':
		(void)strcpy(out_filename,optarg);
		break;
       case 'B':
		(void)strcpy(out_bok_filename,optarg);
		break;
       case 'L':
		LogarytmStat=1;
		break;
       default:
		fprintf(stderr,"Nieznana opcja !\n");
		FreeArgv(argv,argc);
		return;
    }

   FreeArgv(argv,argc);
   if(AllBook==0)
     if(Compute==0)
       {
         fprintf(stderr,"Nie wykonane obliczenia !\n");
         return;
       }
   
   if(SplitFactor<1.0F)
      SplitFactor=1.0F;
   FREQ/=SplitFactor;         
   sig_size=NUM=true_signal_size=DimBase;  /* Ladowanie odpowiednich tablic */
   num_of_wav=dimroz;
   sig_norm=E0;
   bok_norm=Ec;
   energy_percent=100.0F*bok_norm/sig_norm;
   df=2.0F*M_PI/(float)sig_size;

   if(prn==1)
    {
      fprintf(stdout,"Nazwa pliku z sygnalem     : %s\n",SSignalname);
      fprintf(stdout,"Nazwa pliku z ksiazka      : %s\n",out_bok_filename);
      fprintf(stdout,"Nazwa pliku z mapa Wignera : %s\n",out_filename);
    }

  if(AllBook==0)
   { 
    if((psbook=(PSBOOK *)malloc((unsigned)num_of_wav*sizeof(PSBOOK)))==NULL)
     {
      fprintf(stderr,"Brak pamieci (PS) !\n");  
      return;
     }

    for(i=0 ; i<num_of_wav; i++)		/* Przepisanie danych do struktur */
     {						/* z mpp2ps.c */
       psbook[i].s=book[i].param[0];
       Octave[i]=Log2(psbook[i].s);
       Pos[i]=(int)book[i].param[1];
       psbook[i].t=book[i].param[1];
       Modulus[i]=psbook[i].w=book[i].waga;
       Phase[i]=book[i].phase;
       psbook[i].f=book[i].param[2];
       psbook[i].amplitude=book[i].amplitude;
       Freq[i]=(int)(0.5F+book[i].param[2]/df);
     }
    }
   else if((psbook=ReadAllAtoms(bookfilename,&num_of_wav))==NULL)
          {
            fprintf(stderr,"Blad przy czytaniu ksiazki !\n");
            return;
          }    

   for(i=0 ; i<DimBase ; i++) 
     sig[i]=OrgSygnal[i];

   if((FO=fopen(out_filename, "wt"))==NULL)
     {
       fprintf(stderr,"Nie moge otworzyc pliku !\n");
       return;
     }

   strcpy(date_str,date());
   write_header();
   set_landscape();
   bit_image_printing(GammaCorect,SplitFactor,Crop,Dyspers);
   if(AllBook==0 || AddSignal==1)		/* Drukowanie sygnalu */
     vector_signal_printing();
   write_comments();
   free((void *)psbook);
   fclose(FO);

   if(AllBook==0)				/* Ksiazka tylko dla pojedynczych map */
    {
     if(prn==1)
       fprintf(stdout,"<<< WYDRUK KSIAZKI >>>\n");
     if((FO=fopen(out_bok_filename,"wt"))==NULL)
      {
       fprintf(stderr,"Nie moge otworzyc pliku (2) !\n");
       return;
      }

    write_header();
    print_book(SplitFactor);
    fclose(FO);
   } 
 }









