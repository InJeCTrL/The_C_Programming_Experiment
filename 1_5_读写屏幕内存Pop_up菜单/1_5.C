#include<BIOS.h>
#include<DOS.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
typedef struct videoInfo
{
	char ASCII;
	char attribute;
}videoInfo;
int get_resp(int x,int y,int count,char *menu[],char far *videobase)
{
	char ch;				
	int det,i = 0;
	x++;y++;
	det = x - i;
	Mem_write_video(x,y,menu[i],0x70,videobase);
	while (1)			
	{
		ch = bioskey(0);
		if (ch == '1')			
		{
			Mem_write_video(x,y,menu[i],7,videobase);	
			i++;
			i %= count;
			x = det + i;

			Mem_write_video(x,y,menu[i],0x70,videobase);	
		}
		else if (ch == 13)		
			return i;		
		else if (ch == 27)
			return -1;	
	}
}
int video_mode(void)
{
	union REGS in,out;
	in.h.ah = 0x0F;
	int86(0x10,&in,&out);
	return out.h.al;
}
videoInfo* Mem_save_video(int startx,int endx,int starty,int endy,char far *videobase)
{
	register int i,j,k=0;
	char far *tPTR = NULL;
	videoInfo *buf = NULL;
	buf = (videoInfo*)malloc((endx-startx+1)*(endy*starty+1)*2*sizeof(videoInfo));
	for (i=starty;i<endy;i++)
	{
		for (j=startx;j<endx;j++)
		{
			tPTR = videobase+i*2+j*160;
			buf[k].ASCII = *tPTR;
			buf[k++].attribute = *(tPTR+1);
		}
	}
	return buf;
}
int Mem_restore_video(int startx,int endx,int starty,int endy,char far *videobase,videoInfo *buf)
{
	register int i,j,k=0;
	char far *tPTR = NULL;
	for (i=starty;i<endy;i++)
	{
		for (j=startx;j<endx;j++)
		{
			tPTR = videobase+i*2+j*160;
			*tPTR = buf[k].ASCII;
			*(tPTR+1) = buf[k++].attribute;
		}
	}
	return 0;
}
int Mem_clear_video(int startx,int endx,int starty,int endy,char far *videobase)
{
	register int i,j;
	char far *tPTR = NULL;
	for (i=starty;i<endy;i++)
	{
		for (j=startx;j<endx;j++)
		{
			tPTR = videobase+i*2+j*160;
			*tPTR = ' ';
		}
	}
	return 0;
}
int Mem_write_video(int x,int y,char *pStr,int Attrib,char far *videobase)
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
int draw_border(int x,int y,char *menu[],int count,char far *videobase)
{
	register int i,maxlen=0;
	char far *tPTR = NULL;
	for (i=0;i<count;i++)	
	{
		if (maxlen < strlen(menu[i]))
			maxlen = strlen(menu[i]);
	}
	tPTR = videobase + x*160 + y*2;
	*tPTR = '+';
	for (++y,i=0;i<maxlen;i++,y++)
	{
		tPTR = videobase + x*160 + y*2;
		*tPTR = '-';
	}
	tPTR = videobase + x*160 + y*2;
	*tPTR = '+';
	for (++x,i=0;i<count;i++,x++)
	{
		tPTR = videobase + x*160 + y*2;
		*tPTR = '|';
	}
	tPTR = videobase + x*160 + y*2;
	*tPTR = '+';
	for (--y,i=0;i<maxlen;i++,y--)
	{
		tPTR = videobase + x*160 + y*2;
		*tPTR = '-';
	}
	tPTR = videobase + x*160 + y*2;
	*tPTR = '+';
	for (--x,i=0;i<count;i++,x--)
	{
		tPTR = videobase + x*160 + y*2;
		*tPTR = '|';
	}
	return 0;
}
int display_menu(int x,int y,char *menu[],int count,int border,char far *videobase)
{
	register int i;
	if (border)
		draw_border(x,y,menu,count,videobase);
	x++;
	y++;
	for (i=0;i<count;i++)
		Mem_write_video(x+i,y,menu[i],7,videobase);
	return 0;
}
int Mem_Popup_menu(int x,int y,char *menu[],int count,int border,char far *videobase)
{
	register int i,len=0;
	int ret,endx,endy;
	videoInfo *buf = NULL;
	if (x>24 || x<0 || y>79 || y<0)
	{
		printf("Screen Buff Overflow!\n");
		return -2;
	}
	for (i=0;i<count;i++)
	{
		if (strlen(menu[i]) > len)
			len = strlen(menu[i]);
	}
	endy = len + y + 2;
	endx = count + 2 + x;
	if ((endx+1>24) || (endy+1>29))
	{
		printf("Screen Buff Overflow!\n");
		return -2;
	}
	buf = Mem_save_video(x,endx,y,endy,videobase);	
	Mem_clear_video(x,endx,y,endy,videobase);
	display_menu(x,y,menu,count,border,videobase);
	ret = get_resp(x,y,count,menu,videobase);
	Mem_restore_video(x,endx,y,endy,videobase,buf);

	return ret;
}

int main(void)
{
	int videomode;
	char far *targAddr = NULL;
	char *menu[] = {"A","B","C"};
	int selectRet;

	videomode = video_mode();
	if (videomode != 2 && videomode != 3 && videomode != 7)
	{
		printf("Video Mode Not Match!");
		return 1;
	}
	if (videomode == 7)
		targAddr = (char far *)0xb0000000;/*MDA*/
	else
		targAddr = (char far *)0xb8000000;/*CGA EGA*/
	
	
	selectRet = Mem_Popup_menu(5,5,menu,3,1,targAddr);/*3 items£¬x5y5£¬Border*/
	printf("Return Value:%d\n",selectRet);

	bioskey(0);

	return 0;
}