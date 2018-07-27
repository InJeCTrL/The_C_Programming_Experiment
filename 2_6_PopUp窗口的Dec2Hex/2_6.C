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
int goto_xy(int x,int y)
{
	union REGS in;
	in.h.ah = 0x02;				/*SetCursorPos*/
	in.h.bh = 0;
	in.h.dh = x;
	in.h.dl = y;
	int86(0x10,&in,&in);
	return 0;
}
int goto_WNDxy(My_WndHandle my_Hwnd,int x,int y)
{/*x,y:relative to the window*/
	/*outside the window, return 0*/
	if (x<0 || x+_WndObjTbl->WndObjList[my_Hwnd]->startx >= _WndObjTbl->WndObjList[my_Hwnd]->endx-1)
		return 0;
	if (y<0 || y+_WndObjTbl->WndObjList[my_Hwnd]->starty >= _WndObjTbl->WndObjList[my_Hwnd]->endy-1)
		return 0;
	_WndObjTbl->WndObjList[my_Hwnd]->curx = x;
	_WndObjTbl->WndObjList[my_Hwnd]->cury = y;
	goto_xy(_WndObjTbl->WndObjList[my_Hwnd]->startx+x+1,_WndObjTbl->WndObjList[my_Hwnd]->starty+y+1);
	return 1;
}
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
char get_WNDch(My_WndHandle my_Hwnd)
{/*return ASCII*/
	char t[2] = {0};
	if (_WndObjTbl->WndObjList[my_Hwnd]->active <= 0)
		goto_WNDxy(my_Hwnd,_WndObjTbl->WndObjList[my_Hwnd]->curx,_WndObjTbl->WndObjList[my_Hwnd]->cury);
	t[0] = bioskey(0);
	if (t[0])
	{
		switch (t[0])
		{
		case '\r':/*catch ENTER*/
			break;
		case 8:/*catch backspace*/
			break;
		default:
			if (_WndObjTbl->WndObjList[my_Hwnd]->cury+_WndObjTbl->WndObjList[my_Hwnd]->starty < _WndObjTbl->WndObjList[my_Hwnd]->endy-1)
			{
				Mem_write_video(_WndObjTbl->WndObjList[my_Hwnd]->startx+_WndObjTbl->WndObjList[my_Hwnd]->curx+1,_WndObjTbl->WndObjList[my_Hwnd]->starty+_WndObjTbl->WndObjList[my_Hwnd]->cury+1,t,7);
				_WndObjTbl->WndObjList[my_Hwnd]->cury++;
			}
		}
		if (_WndObjTbl->WndObjList[my_Hwnd]->curx < 0)
			_WndObjTbl->WndObjList[my_Hwnd]->curx = 0;
		if (_WndObjTbl->WndObjList[my_Hwnd]->curx + _WndObjTbl->WndObjList[my_Hwnd]->startx > _WndObjTbl->WndObjList[my_Hwnd]->endx-2)
			_WndObjTbl->WndObjList[my_Hwnd]->curx--;
		goto_WNDxy(my_Hwnd,_WndObjTbl->WndObjList[my_Hwnd]->curx,_WndObjTbl->WndObjList[my_Hwnd]->cury);
	}
	return t[0];
}
char* get_WNDstr(My_WndHandle my_Hwnd)
{/*return string*/
	char buf[513] = {0};/*buffer size 512*/
	char t;
	register int i=0;
	while (1)
	{
		t = get_WNDch(my_Hwnd);
		switch (t)
		{
		case '\r':/*catch ENTER*/
			buf[i] = 0;
			return buf;
		case 8:/*catch backspace*/
			if (i >= 1)
				i--;
			_WndObjTbl->WndObjList[my_Hwnd]->cury--;
			if (_WndObjTbl->WndObjList[my_Hwnd]->cury<0)
				_WndObjTbl->WndObjList[my_Hwnd]->cury = 0;
			goto_WNDxy(my_Hwnd,_WndObjTbl->WndObjList[my_Hwnd]->curx,_WndObjTbl->WndObjList[my_Hwnd]->cury);
			Mem_write_video(_WndObjTbl->WndObjList[my_Hwnd]->startx+_WndObjTbl->WndObjList[my_Hwnd]->curx+1,_WndObjTbl->WndObjList[my_Hwnd]->starty+_WndObjTbl->WndObjList[my_Hwnd]->cury+1," ",7);
			break;
		default:
			buf[i] = t;
			i++;
			break;
		}
	}
	return buf;
}
int put_WNDch(My_WndHandle my_Hwnd,char ch)
{
	register int x,y;
	char t[2] = {0};
	t[0] = ch;
	if (_WndObjTbl->WndObjList[my_Hwnd]->active <= 0)
		return 0;
	x = _WndObjTbl->WndObjList[my_Hwnd]->curx+_WndObjTbl->WndObjList[my_Hwnd]->startx+1;
	y = _WndObjTbl->WndObjList[my_Hwnd]->cury+_WndObjTbl->WndObjList[my_Hwnd]->starty+1;
	if (y >= _WndObjTbl->WndObjList[my_Hwnd]->endy)
		return 1;
	if (x >= _WndObjTbl->WndObjList[my_Hwnd]->endx)
		return 1;
	if (ch == '\n')
	{
		x++;
		y = _WndObjTbl->WndObjList[my_Hwnd]->starty + 1;
		_WndObjTbl->WndObjList[my_Hwnd]->curx++;
		_WndObjTbl->WndObjList[my_Hwnd]->cury = 0;
	}
	else
	{
		_WndObjTbl->WndObjList[my_Hwnd]->cury++;
		Mem_write_video(x,y,t,7);
	}		
	goto_WNDxy(my_Hwnd,_WndObjTbl->WndObjList[my_Hwnd]->curx,_WndObjTbl->WndObjList[my_Hwnd]->cury);
	return 0;
}
int put_WNDstr(My_WndHandle my_Hwnd,char *str)
{/*Fail return 0*/
	if (_WndObjTbl->WndObjList[my_Hwnd]->active <= 0)
		return 0;
	while (*str)
	{
		put_WNDch(my_Hwnd,*str);
		str++;
	}
	return 1;
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
	_WndObjTbl->WndObjList[tHandle]->active = 1;
	Mem_save_video(_WndObjTbl->WndObjList[tHandle]->startx,_WndObjTbl->WndObjList[tHandle]->endx,_WndObjTbl->WndObjList[tHandle]->starty,_WndObjTbl->WndObjList[tHandle]->endy,&(_WndObjTbl->WndObjList[tHandle]->savebuf));
	draw_frame(_WndObjTbl->WndObjList[tHandle]->startx,_WndObjTbl->WndObjList[tHandle]->starty,_WndObjTbl->WndObjList[tHandle]->endx,_WndObjTbl->WndObjList[tHandle]->endy,_WndObjTbl->WndObjList[tHandle]->border,_WndObjTbl->WndObjList[tHandle]->Caption);
	goto_WNDxy(tHandle,_WndObjTbl->WndObjList[tHandle]->curx,_WndObjTbl->WndObjList[tHandle]->cury);

	return 0;
}
int DeActive(My_WndHandle tHandle)
{
	_WndObjTbl->WndObjList[tHandle]->active = 0;
	Mem_restore_video(_WndObjTbl->WndObjList[tHandle]->startx,_WndObjTbl->WndObjList[tHandle]->endx,_WndObjTbl->WndObjList[tHandle]->starty,_WndObjTbl->WndObjList[tHandle]->endy,&(_WndObjTbl->WndObjList[tHandle]->savebuf));
	goto_xy(0,0);

	return 0;
}
My_WndHandle NewWindow(DFrameInfo *pWndInfo)
{
	My_WndHandle tHandle;
	tHandle = InsertWndList(pWndInfo);
	BringToActive(tHandle);
	
	return tHandle;
}
int main(void)
{
	int chkerr;
	DFrameInfo Fa;
	My_WndHandle hwnd;
	char *pStr = NULL;
	int n;
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
	Fa.curx = 0;
	Fa.cury = 0;
	Fa.active = 1;

	hwnd = NewWindow(&Fa);
	put_WNDstr(hwnd,"dec:");
	pStr = get_WNDstr(hwnd);
	put_WNDch(hwnd,'\n');
	sscanf(pStr,"%d",&n);
	sprintf(pStr,"%s%x","hex:",n);
	put_WNDstr(hwnd,pStr);

	bioskey(0);
	DeActive(hwnd);
	return 0;
}