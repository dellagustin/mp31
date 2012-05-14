	/* Tworzenie mapy Wignera dla zbioru atomow (1996 03 01/31 04 16/ 05/16)
           1997 05 21,27 1997 10 25; 1997 11 19/22 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "wigmap.h"
#include "proto.h"

#define ON      1
#define PI2M    12.566370614359F	/* 6.2831853F Old Value */
#define EPSYLON 1.0e-8F
#define NAL(x,y)     ((x)>=0 && (x)<SizeX && (y)>=0 && (y)<SizeY) 
#define LOGSTAT(i,j) ((LogarytmStat==1) ? log(1.0F+WignerTab[i][j]) : \
					  WignerTab[i][j]) /* Skala logarytmiczna */

typedef unsigned short USHORT;

extern int prn;                         /* Drukowanie informacji pomocniczych */
int LogarytmStat=0;			/* Licznenie mapy logarytmicznej */

static double SQR(double x)
 {
   return x*x;
 }

static float GAMMA(float x,float Gamma)
 {
   return ((Gamma!=1.0) ? pow(x,Gamma) : x);
 }

float **MakeTable(int n,int m) 	/* Alokacja 2D tablicy typu float */
 {
   const unsigned size=(unsigned)m*sizeof(float);
   register int i,j;
   float **ptr;

   if((ptr=(float **)malloc((unsigned)n*sizeof(float *)))==NULL)
     return NULL;
   for(i=0 ; i<n ; i++)
    {
      if((ptr[i]=(float *)malloc(size))==NULL)
       {
	 for(j=0 ; j<i ; j++)
	   free((void *)ptr[i]);
	 free((void *)ptr);
	 return NULL;
       }
     (void)memset((void *)ptr[i],0,size);
   }
   return ptr;
 }

void FreeTable(float **ptr,int n)   /* Zwolnienie pamieci po 2D tablicy float */
 {
   register int i;

   if(ptr==NULL)
     return;
   for(i=0 ; i<n ; i++)
     if(ptr[i]!=NULL)
       free((void *)ptr[i]);
   free((void *)ptr);
 }

static void MakePalette(UCHAR paleta[])
 {
   register int i;						/* Korekcja Gamma */

   for(i=0 ; i<256 ; i++)
     paleta[i]=(UCHAR)i;
 }

static unsigned char Code(float x,float y,float x_min,float x_max,
                          float y_min,float y_max)
 {
   unsigned char ret=0;		/* Kodowanie stanu konca odcinka */
   
   if(x<x_min) ret|=1U;
   if(x>x_max) ret|=2U;
   if(y<y_min) ret|=4U;
   if(y>y_max) ret|=8U;
   return ret;
 }
 
#define SWAP(X,Y) { ftmp= X ; X = Y; Y =ftmp; }
 
static int Clip(float xp,float yp,float xk,float yk,float x_min,
                float x_max,float y_min,float y_max)
 {
   unsigned char p,k;	/* Algorytm Cohena-Sutheranda */
   float ftmp;          /* okienkowania odcinka (obcinanie) */
   
   for(;;)
     {
       p=Code(xp,yp,x_min,x_max,y_min,y_max);
       k=Code(xk,yk,x_min,x_max,y_min,y_max);
       
       if(p==0 && k==0)
         return 0;
       
       if((p & k)!=0)  
         break;
         
       if(p==0)
         {
			  p=k;
           SWAP(xp,xk);
           SWAP(yp,yk);  
         }
         
       if((p & 1)!=0)
         {
           yp+=(x_min-xp)*(yk-yp)/(xk-xp);
           xp=x_min;
         }
       else if((p & 2)!=0)
         {
           yp+=(x_min-xp)*(yk-yp)/(xk-xp);
           xp=x_max;
         }  
       else if((p & 4)!=0)
         {
           xp+=(y_min-yp)*(xk-xp)/(yk-yp);
           yp=y_min;
         }  
       else if((p & 8)!=0)
         {
           xp+=(y_min-yp)*(xk-xp)/(yk-yp);
           yp=y_max;
         }         
     }
     
   return -1; 
 } 
 
#undef SWAP 
 
static int ClipRect(float xp,float yp,float d,float h,
                    float x_min,float x_max,float y_min,float y_max)
 {
   const float xk=xp+d,
               yk=yp+h;
                    
   if(xp>x_min && xp<x_max)	        /* Caly prostokat w polu */
     if(xk>x_min && xk<x_max) 
      if(yp>y_min && yp<y_max)
        if(yk>y_min && yk<y_max)
          return 1; 

   if(x_min>xp && x_min<xk)             /* Odwrotne zawieranie */
     if(x_max>xp && x_max<xk)
       if(y_min>yp && y_min<yk)
         if(y_max>yp && y_max<yk)
           return 1;    
                                	/* Sprawdzamy krawedzie */
   if(Clip(xp,yp,xk,yp,x_min,x_max,y_min,y_max)==0)
     return 1;
     
   if(Clip(xk,yp,xk,yk,x_min,x_max,y_min,y_max)==0)
     return 1;
     
   if(Clip(xk,yk,xp,yk,x_min,x_max,y_min,y_max)==0)
     return 1;
     
   if(Clip(xp,yk,xp,yp,x_min,x_max,y_min,y_max)==0)
     return 1;
   
   return 0;                           /* Poza polem zainteresowania */
 } 

void MakeWignerMap(PSBOOK book[],int IleAtomow,int SizeX,int SizeY,
		   int SizeBase,UCHAR *OutputTab[],float GammaCorect,
		   float SplitFactor,char *filename,int GenBitMap,float Crop)
 {						/* Generacja mapy Wignera */
  register int i,j,k,code,ZakresX,ZakresY,ii,jj,i0,j0,ok;
  float dx,dy,u,s,e,w,min,max,skala,Const,tmp,**WignerTab;
  UCHAR Paleta[257];
  static double OldExp1,OldExp2,ConstStep1,ConstStep2,Factor1,Factor2,
                ConstExp1,ConstExp2;
  FILE *file;              

  if((WignerTab=MakeTable(SizeX,SizeY))==NULL)
   {
     fprintf(stderr,"Nie ma tyle pamieci !\n");
     return;
   }

  if(SplitFactor<1.0F)
     SplitFactor=1.0F;
  if(GammaCorect<=1.0e-6F)
     GammaCorect=1.0e-6F;   
  if(Crop<0.0F || Crop>1.0F)
    Crop=1.0F;      
  MakePalette(Paleta);
  dx=(float)SizeBase/(float)SizeX;
  dy=(float)M_PI/(float)SizeY/SplitFactor;

  for(i=0 ; i<SizeX ; i++)
    for(j=0 ; j<SizeY ; j++)
     {
       WignerTab[i][j]=0.0F;
       OutputTab[i][j]=0;
     }

  for(k=0 ; k<IleAtomow ; k++)
   {
     if(prn==1)
      if((k%100)==0)
      {
        fprintf(stdout,"->%5d\r",k); 
	fflush(stdout);
      }

    s=book[k].s; u=book[k].t;
    e=book[k].f;
    w=book[k].w; /* *book[k].amplitude; */
    w*=w;
    Const=(float)sqrt(log(w/(2.0F*EPSYLON))/(PI2M*GammaCorect));

    ok=1;
    if(s==0)				/* Dirac */
     {
       ZakresY=SizeY;
       ZakresX=0;
     }
    else if(s==SizeBase)		/* Fourier */
	 {    
	   ZakresY=0;
	   ZakresX=SizeX;
	 }
    else
	 {			        /* Gabor */
	   ok=0;
	   ZakresY=(int)(PI2M*Const/(s*dy)+1.0F);
	   ZakresX=(int)(s*Const/dx+1.0);
	 }

    i0=(int)(0.5F+u/dx);
    if(i0>=SizeX) i0=SizeX-1;
    j0=(int)(0.5F+e/dy);

    if(NAL(i0,j0))
     {
       if(ZakresX>SizeX) 
         ZakresX=SizeX-1;
       if(ZakresY>SizeY) 
         ZakresY=SizeY-1;
     } 

    if(SplitFactor!=1.0F)
     if(!ClipRect(i0-ZakresX,j0-ZakresY,2.0F*ZakresX,2.0F*ZakresY,
                  0.0F,SizeX-1.0F,0.0F,SizeY-1.0F))
      continue;  /* Testowanie obecnosci obszaru w wybranym fragmencie */

    if(ok!=1)
     {
      ConstExp1=exp(-4.0*M_PI*SQR(dx/s)); 		/* Szybka implementacja */
      Factor1=ConstExp1; 				/* generacji blobow Gabora */
      ConstStep1=SQR(ConstExp1);
      ConstExp2=exp(-4.0*M_PI*SQR(dy*s/PI2M)); 
      OldExp1=1.0;
     }       
    
    for(i=0 ; i<=ZakresX; i++)
     {      
       if(ok!=1)
        {      
          OldExp2=1.0;				/* Szybka implementacja */
          Factor2=ConstExp2; 			/* blobow Gabora CD ... */	
          ConstStep2=SQR(ConstExp2);
        }  
       
       for(j=0 ; j<=ZakresY ; j++)
	{                 
	  if(ok==1)
	    tmp=w; 				/* W przypadku sinusa i delty */
	  else 					/* Dirac'a */
	   {
            tmp=w*(float)(OldExp1*OldExp2);
            OldExp2*=Factor2;
            Factor2*=ConstStep2;
           }
            
	  ii=i0+i;
	  jj=j0+j;
	  if(NAL(ii,jj))
	    {
	      WignerTab[ii][jj]+=tmp;
	      OutputTab[ii][jj]=1;
	    }

	  jj=j0-j;
	  if(NAL(ii,jj) && jj!=j0)
	    {
	      WignerTab[ii][jj]+=tmp;
	      OutputTab[ii][jj]=1;
	    }

	  ii=i0-i;
	  if(NAL(ii,jj) && ii!=i0)
	    {
	      WignerTab[ii][jj]+=tmp;
	      OutputTab[ii][jj]=1;
	    }

	  jj=j0+j;
	  if(NAL(ii,jj) && ii!=i0 && jj!=j0)
	    {
	      WignerTab[ii][jj]+=tmp;
	      OutputTab[ii][jj]=1;
	    }
	}
        
       if(ok!=1)
        { 
         OldExp1*=Factor1;
         Factor1*=ConstStep1;
        } 
     }
  }

  if(prn==1)
    fprintf(stdout,"\n<<< RESKALOWANIE >>>\n");

  min=max=GAMMA(LOGSTAT(0,0),GammaCorect);
  for(i=0 ; i<SizeX ; i++)
   for(j=0 ; j<SizeY ; j++) if(OutputTab[i][j]!=0)
    {
      WignerTab[i][j]=GAMMA(LOGSTAT(i,j),GammaCorect);
      if(WignerTab[i][j]<min) min=WignerTab[i][j];
      if(WignerTab[i][j]>max) max=WignerTab[i][j];
    }
  
  if(GenBitMap==GENBITMAP)			/* Generacja bitmapy float'ow */
   {
    if((file=fopen(filename,"wb"))!=NULL)
     {
       float tmp;
       
       if(prn==ON)
         fprintf(stdout,"<<< ZAPIS PLIKU Z BITMAPA >>>\n");
         
       for(i=0 ; i<SizeX ; i++)
         for(j=0 ; j<SizeY ; j++) 
           {
             if(OutputTab[i][j]!=0)
               tmp=WignerTab[i][j];
             else 
               tmp=0.0F;  
             fwrite((void *)&tmp,sizeof(float),1,file);
           } 
          
       fclose(file); 
       if(prn==ON)
         fprintf(stdout,"<<< KONIEC ZAPISU >>>\n");   
     }  
   }  
  else  
  {
    if(prn==ON)
      fprintf(stdout,"<<< KODOWANIE BITMAPY >>>\n");

    max=min+Crop*(max-min);        
    if(max!=min)
      skala=255.0F/(max-min);
    else skala=1.0F;
 
    for(i=0 ; i<SizeX ; i++)
     for(j=0 ; j<SizeY ; j++) 
       if(OutputTab[i][j]!=0)
       {
         if(WignerTab[i][j]>max)
           WignerTab[i][j]=max;
	 code=(int)(0.5F+skala*(WignerTab[i][j]-min));
	 if(code<0)   code=0;
	 if(code>254) code=254;
	 OutputTab[i][j]=Paleta[code];
       }
   }
   
  FreeTable(WignerTab,SizeX);
 }

UCHAR **MakeTableUChar(int n,int dim)  /* Funkcja eksportowana */
 {			/* Alokacja 2D tablicy typu UCHAR */
   const unsigned size=(unsigned)dim*sizeof(UCHAR);
   register int i,j;
   UCHAR **tab;

   if((tab=(UCHAR **)malloc((unsigned)n*sizeof(UCHAR *)))==NULL)
      return NULL;
   for(i=0 ; i<n ; i++)
    {
     if((tab[i]=(UCHAR *)malloc(size))==NULL)
       {
	 for(j=0 ; j<i ; j++) 
           free((void *)tab[j]);
	 free((void *)tab);
	 return NULL;
       }
     (void)memset((void *)tab[i],0,size);
    }
    return tab;
  }

void FreeTableUChar(UCHAR **tab,int n)
 {
   register int i ;	/* Zwolnienie pamieci po 2D tablicy UCHAR */

   if(tab==NULL)
     return;
   for(i=0 ; i<n ; i++) 
     if(tab[i]!=NULL) 
       free((void *)tab[i]);
   if(tab!=NULL)
     free((void *)tab);
 }

static USHORT **MakeTableUShort(int n,int dim)
 {			/* Alokacja 2D tablicy typu USHORT */
   const unsigned size=(unsigned)dim*sizeof(USHORT);
   register int i,j;
   USHORT **tab;

   if((tab=(USHORT **)malloc((unsigned)n*sizeof(USHORT *)))==NULL)
      return NULL;
   for(i=0 ; i<n ; i++)
    {
     if((tab[i]=(USHORT *)malloc(size))==NULL)
       {
	 for(j=0 ; j<i ; j++) 
           free((void *)tab[j]);
	 free((void *)tab);
	 return NULL;
       }
     (void)memset((void *)tab[i],0,size);
    }
    return tab;
  }

static void FreeTableUShort(USHORT **tab,int n)
 {
   register int i ;	/* Zwolnienie pamieci po 2D tablicy USHORT */

   if(tab==NULL)
     return;
     
   for(i=0 ; i<n ; i++) 
     if(tab[i]!=NULL) 
       free((void *)tab[i]);
   if(tab!=NULL)
     free((void *)tab);
 }

void MakeDyspersWignerMap(PSBOOK book[],int IleAtomow,int SizeX,int SizeY,int SizeBase,
		          UCHAR *OutputTab[],float GammaCorect,float SplitFactor,
                          char *filename,int GenBitMap,float Crop)
 {						/* Generacja mapy Wignera */
  register int i,j,k,code,ZakresX,ZakresY,ii,jj,i0,j0,ok;
  float dx,dy,u,s,e,w,min,max,skala,Const,tmp,**WignerTab,
        **SqrWignerMap=NULL;
  UCHAR Paleta[257];
  static double OldExp1,OldExp2,ConstStep1,ConstStep2,Factor1,Factor2,
                ConstExp1,ConstExp2;
  FILE *file;              
  USHORT **NTable=NULL;

  if((WignerTab=MakeTable(SizeX,SizeY))==NULL ||
     (SqrWignerMap=MakeTable(SizeX,SizeY))==NULL ||
     (NTable=MakeTableUShort(SizeX,SizeY))==NULL)
   {
     fprintf(stderr,"Nie ma tyle pamieci !\n");
     FreeTable(WignerTab,SizeX);
     FreeTable(SqrWignerMap,SizeX);
	  FreeTableUShort(NTable,SizeX);
     return;
   }

  if(SplitFactor<1.0F)
     SplitFactor=1.0F;
  if(GammaCorect<=1.0e-6F)
     GammaCorect=1.0e-6F;   
  if(Crop<0.0F || Crop>1.0F)
    Crop=1.0F;   
  MakePalette(Paleta);
  dx=(float)SizeBase/(float)SizeX;
  dy=(float)M_PI/(float)SizeY/SplitFactor;

  for(i=0 ; i<SizeX ; i++)
    for(j=0 ; j<SizeY ; j++)
     {
       SqrWignerMap[i][j]=WignerTab[i][j]=0.0F;
       OutputTab[i][j]=0;
       NTable[i][j]=0;
     }

  for(k=0 ; k<IleAtomow ; k++)
   {
     if(prn==1)
      if((k%100)==0)
      {
        fprintf(stdout,"->%5d\r",k); 
	fflush(stdout);
      }

    s=book[k].s; u=book[k].t;
    e=book[k].f;
    w=book[k].w; /* *book[k].amplitude; */
    w*=w;
    Const=1.5F*sqrt(log(w/(2.0F*EPSYLON))/(PI2M*GammaCorect));

    ok=1;
    if(s==0)				/* Dirac */
     {
       ZakresY=SizeY;
       ZakresX=0;
     }
    else if(s==SizeBase)		/* Fourier */
	 {    
	   ZakresY=0;
	   ZakresX=SizeX;
	 }
    else
	 {			        /* Gabor */
	   ok=0;
	   ZakresY=(int)(PI2M*Const/(s*dy)+1.0F);
	   ZakresX=(int)(s*Const/dx+1.0);
	 }

    i0=(int)(0.5F+u/dx);
    if(i0>=SizeX) i0=SizeX-1;
    j0=(int)(0.5F+e/dy);

    if(NAL(i0,j0))
     {
       if(ZakresX>SizeX) 
         ZakresX=SizeX-1;
       if(ZakresY>SizeY) 
         ZakresY=SizeY-1;
     } 

    if(SplitFactor!=1.0F)
     if(!ClipRect(i0-ZakresX,j0-ZakresY,2.0F*ZakresX,2.0F*ZakresY,
                  0.0F,SizeX-1.0F,0.0F,SizeY-1.0F))
      continue;  /* Testowanie obecnosci obszaru w wybranym fragmencie */

    if(ok!=1)
     {
      ConstExp1=exp(-4.0*M_PI*SQR(dx/s)); 		/* Szybka implementacja */
      Factor1=ConstExp1; 				/* generacji blobow Gabora */
      ConstStep1=SQR(ConstExp1);
      ConstExp2=exp(-4.0*M_PI*SQR(dy*s/PI2M)); 
      OldExp1=1.0;
     }       
    
    for(i=0 ; i<=ZakresX; i++)
     {      
       if(ok!=1)
        {      
          OldExp2=1.0;				/* Szybka implementacja */
          Factor2=ConstExp2; 			/* blobow Gabora CD ... */	
          ConstStep2=SQR(ConstExp2);
        }  
       
       for(j=0 ; j<=ZakresY ; j++)
	{                 
	  if(ok==1)
	    tmp=w; 				/* W przypadku sinusa i delty */
	  else 					/* Dirac'a */
	   {
            tmp=w*(float)(OldExp1*OldExp2);
            OldExp2*=Factor2;
            Factor2*=ConstStep2;
           }
            
	  ii=i0+i;
	  jj=j0+j;
	  if(NAL(ii,jj))
	    {
	      WignerTab[ii][jj]+=tmp;
              SqrWignerMap[ii][jj]+=SQR(tmp);
              NTable[ii][jj]++;
	      OutputTab[ii][jj]=1;
	    }

	  jj=j0-j;
	  if(NAL(ii,jj) && jj!=j0)
	    {
	      WignerTab[ii][jj]+=tmp;
              SqrWignerMap[ii][jj]+=SQR(tmp);
              NTable[ii][jj]++;
	      OutputTab[ii][jj]=1;
	    }

	  ii=i0-i;
	  if(NAL(ii,jj) && ii!=i0)
	    {
	      WignerTab[ii][jj]+=tmp;
              SqrWignerMap[ii][jj]+=SQR(tmp);
              NTable[ii][jj]++;
	      OutputTab[ii][jj]=1;
	    }

	  jj=j0+j;
	  if(NAL(ii,jj) && ii!=i0 && jj!=j0)
	    {
	      WignerTab[ii][jj]+=tmp;
              SqrWignerMap[ii][jj]+=SQR(tmp);
              NTable[ii][jj]++;
	      OutputTab[ii][jj]=1;
	    }
	}
        
       if(ok!=1)
        { 
         OldExp1*=Factor1;
         Factor1*=ConstStep1;
        } 
     }
  }

  if(prn==1)
    fprintf(stdout,"\n<<< RESKALOWANIE >>>\n");
    
  for(i=0 ; i<SizeX ; i++)		/* Wyznaczamy dyspersje */
    for(j=0 ; j<SizeY ; j++) 
      if(OutputTab[i][j]!=0)
       {
        const float srednia=WignerTab[i][j]/NTable[i][j];
        
        WignerTab[i][j]=SqrWignerMap[i][j]/NTable[i][j]-SQR(srednia);   
       }                               

  min=max=GAMMA(LOGSTAT(0,0),GammaCorect);
  for(i=0 ; i<SizeX ; i++)
   for(j=0 ; j<SizeY ; j++) if(OutputTab[i][j]!=0)
    {
      WignerTab[i][j]=GAMMA(LOGSTAT(i,j),GammaCorect);
      if(WignerTab[i][j]<min) min=WignerTab[i][j];
      if(WignerTab[i][j]>max) max=WignerTab[i][j];
    }
  
  if(GenBitMap==GENBITMAP)			/* Generacja bitmapy float'ow */
   {
    if((file=fopen(filename,"wb"))!=NULL)
     {
       float tmp;
       
       if(prn==ON)
         fprintf(stdout,"<<< ZAPIS PLIKU Z BITMAPA >>>\n");
         
       for(i=0 ; i<SizeX ; i++)
         for(j=0 ; j<SizeY ; j++) 
           {
             if(OutputTab[i][j]!=0)
               tmp=WignerTab[i][j];
             else 
               tmp=0.0F;  
             fwrite((void *)&tmp,sizeof(float),1,file);
           } 
          
       fclose(file); 
       if(prn==ON)
         fprintf(stdout,"<<< KONIEC ZAPISU >>>\n");   
     }  
   }  
  else  
  {
    if(prn==ON)
      fprintf(stdout,"<<< KODOWANIE BITMAPY >>>\n");
    
    max=min+Crop*(max-min);  
    if(max!=min)
      skala=255.0F/(max-min);
    else skala=1.0F;
 
    for(i=0 ; i<SizeX ; i++)
     for(j=0 ; j<SizeY ; j++) 
       if(OutputTab[i][j]!=0)
       {
         if(WignerTab[i][j]>max)
           WignerTab[i][j]=max;
	 code=(int)(0.5F+skala*(WignerTab[i][j]-min));
	 if(code<0)   code=0;
	 if(code>254) code=254;
	 OutputTab[i][j]=Paleta[code];
       }
   }
   
  FreeTable(WignerTab,SizeX);
  FreeTable(SqrWignerMap,SizeX);
  FreeTableUShort(NTable,SizeX);
 }
 
