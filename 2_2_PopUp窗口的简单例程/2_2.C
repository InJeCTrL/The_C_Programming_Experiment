#include<BIOS.H>
#include<DOS.H>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

char far *videobase = NULL;
typedef struct videoInfo
{
	char ASCII;
	char attribute;
}videoInfo;
typedef struct DFrameInfo
{
	int startx,endx,starty,endy;/*Position*/
	int curx,cury;/*Cursor Position*/
	char *Caption;/*Window Caption*/
	int border;/*Draw Border*/
	int active;/*Active Window*/
	videoInfo *savebuf;/*Saved Screen Info*/
}DFrameInfo;
typedef struct WndObjTbl
{
	DFrameInfo **WndObjList;
	long size;
	long length;
}WndObjTbl;
WndObjTbl *_WndObjTbl;
typedef long My_WndHandle;
int video_mode(void)
{
	union REGS in,out;
	in.h.ah = 0x0F;
	int86(0x10,&in,&out);
	return out.h.al;
}
int SetScreenIOMem(void)
{
	int videomode;
	videomode = video_mode();
	if (videomode != 2 && videomode != 3 && videomode != 7)
	{
		printf("Video Mode Not Match!");
		return 1;
	}
	if (videomode == 7)
		videobase = (char far *)0xb0000000;/*MDA*/
	else
		videobase = (char far *)0xb8000000;/*CGA EGA*/
	return 0;
}
int InitWndObjTbl(void)
{
	_WndObjTbl = (WndObjTbl*)malloc(sizeof(WndObjTbl));
	_WndObjTbl->WndObjList = (DFrameInfo**)malloc(5*sizeof(DFrameInfo*));/*Init object list size 5*/
	_WndObjTbl->size = 5;
	_WndObjTbl->length = 0;
	return 0;
}
int Mem_save_video(int startx,int endx,int starty,int endy,videoInfo **buf)
{
	register int i,j,k=0;
	char far *tPTR = NULL;
	*buf = (videoInfo*)malloc((endx-startx+1)*(endy*starty+1)*2*sizeof(videoInfo));
	for (i=starty;i<=endy;i++)
	{
		for (j=startx;j<=endx;j++)
		{
			tPTR = videobase+i*2+j*160;
			(*buf)[k].ASCII = *tPTR;
			(*buf)[k++].attribute = *(tPTR+1);
		}
	}
	return 0;
}
int Mem_restore_video(int startx,int endx,int starty,int endy,videoInfo **buf)
{
	register int i,j,k=0;
	char far *tPTR = NULL;
	for (i=starty;i<=endy;i++)
	{
		for (j=startx;j<=endx;j++)
		{
			tPTR = videobase+i*2+j*160;
			*tPTR = (*buf)[k].ASCII;
			*(tPTR+1) = (*buf)[k++].attribute;
		}
	}
	free(*buf);
	return 0;
}
int Mem_write_video(int x,int y,char *pStr,int Attrib)
{
	char far *tPTR = NULL;
	register int i;
	int len = strlen(pStr);
	tPTR = videobase + x*160 + y*2;
	for (i=0;i<len;i++,tPTR++)
	{
		*(tPTR++) = pStr[0];
		*tPTR = Attrib;
	}
	return 0;
}
int draw_frame(int startx,int starty,int endx,int endy,int border,char *Caption)
{
	register int i,j;
	char far *tPTR = NULL;
	for (i=startx;i<=endx;i++)
	{
		for (j=starty;j<=endy;j++)
		{
			tPTR = videobase + i*160 + j*2;
			*tPTR = ' ';
		}
	}
	if (border)
	{
		for (i=startx+1;i<endx;i++)
		{
			tPTR = videobase + i*160 + starty*2;
			*tPTR = '|';
		}
		for (i=starty;i<=endy;i++)
		{
			tPTR = videobase + endx*160 + i*2;
			*tPTR = '+';
		}
		for (i=endx-1;i>startx;i--)
		{
			tPTR = videobase + i*160 + endy*2;
			*tPTR = '|';
		}
		for (i=endy;i>=starty;i--)
		{
			tPTR = videobase + startx*160 + i*2;
			*tPTR = '+';
		}
	}
	Mem_write_video(startx,starty+1,Caption,0x70);
	return 0;
}
long InsertWndList(DFrameInfo *pWndInfo)
{
	if (_WndObjTbl->length <= _WndObjTbl->size-3)
	{
		_WndObjTbl->WndObjList = (DFrameInfo**)realloc(_WndObjTbl->WndObjList,(_WndObjTbl->size+5)*sizeof(DFrameInfo*));
		_WndObjTbl->size += 5;
	}
	_WndObjTbl->WndObjList[_WndObjTbl->length] = pWndInfo;
	return (_WndObjTbl->length++);
}
int BringToActive(My_WndHandle tHandle)
{
	register long i;
	for (i=0;i<_WndObjTbl->length;i++)
	{
		if (i == tHandle)
			_WndObjTbl->WndObjList[i]->active = 1;
		else
			_WndObjTbl->WndObjList[i]->active = 0;
	}
	return 0;
}
My_WndHandle NewWindow(DFrameInfo *pWndInfo)
{
	My_WndHandle tHandle;
	Mem_save_video(pWndInfo->startx,pWndInfo->endx,pWndInfo->starty,pWndInfo->endy,&(pWndInfo->savebuf));
	tHandle = InsertWndList(pWndInfo);
	draw_frame(pWndInfo->startx,pWndInfo->starty,pWndInfo->endx,pWndInfo->endy,pWndInfo->border,pWndInfo->Caption);
	BringToActive(tHandle);
	
	return tHandle;
}
int main(void)
{
	int chkerr;
	DFrameInfo Fa;
	chkerr = SetScreenIOMem();
	if (chkerr)
		return 1;
	InitWndObjTbl();

	Fa.startx = 3;
	Fa.endx = 9;
	Fa.starty = 3;
	Fa.endy = 60;
	Fa.Caption = "A";
	Fa.border = 1;

	NewWindow(&Fa);
	bioskey(0);
	return 0;
}