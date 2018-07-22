#include<BIOS.h>
#include<DOS.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
int totalret = 0;
typedef struct videoInfo
{
	char ASCII;
	char attribute;
}videoInfo;
typedef struct listunit
{
	int x;
	int y;
	char **menu;
	int count;
	int border;
	struct listunit **child;
}listunit;
int get_resp(listunit *list,char far *videobase)
{
	char ch;				
	int det,tret,i = 0;
	int tx = list->x,ty = list->y;
	tx++;ty++;
	det = tx - i;
	Mem_write_video(tx,ty,list->menu[i],0x70,videobase);
	while (1)			
	{
		ch = bioskey(0);
		if (ch == 32)			
		{
			Mem_write_video(tx,ty,list->menu[i],7,videobase);	
			i++;
			i %= list->count;
			tx = det + i;

			Mem_write_video(tx,ty,list->menu[i],0x70,videobase);	
		}
		else if (ch == 13)
		{
			if (list->child[i])
			{
				Mem_write_video(tx,ty,list->menu[i],7,videobase);
				totalret *= 10;
				totalret += i+1;

				tret = Mem_PullDown_menu(list,i,list->child[i],videobase);
				if (tret != -1)
				{
					totalret *= 10;
					totalret += tret;
					return totalret;
				}
				else
					tret /= 10;
			}
			else
				return i+1;
		}			
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
	free(buf);
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
int Mem_PullDown_menu(listunit *parent,int j,listunit *list,char far *videobase)
{
	register int i,len=0;
	int ret,endx,endy;
	videoInfo *buf = NULL;
	if (parent)
	{
		list->x = parent->x + j + 1;
		list->y = parent->y + 1;
	}
	if (list->x>24 || list->x<0 || list->y>79 || list->y<0)
	{
		printf("Screen Buff Overflow!A\n");
		return -2;
	}
	for (i=0;i<list->count;i++)
	{
		if (strlen(list->menu[i]) > len)
			len = strlen(list->menu[i]);
	}
	endy = len + list->y + 2;
	endx = list->count + 2 + list->x;
	/*printf("%d,%d",endx,endy);*/
	if ((endx+1>24) || (endy+1>29))
	{
		printf("Screen Buff Overflow!B\n");
		return -2;
	}
	buf = Mem_save_video(list->x,endx,list->y,endy,videobase);	
	Mem_clear_video(list->x,endx,list->y,endy,videobase);
	display_menu(list->x,list->y,list->menu,list->count,list->border,videobase);
	ret = get_resp(list,videobase);
	Mem_restore_video(list->x,endx,list->y,endy,videobase,buf);

	return ret;
}

int main(void)
{
	int videomode;
	char far *targAddr = NULL;
	listunit *first,*second,*third;
	int selectRet;

	/*Build List Structure*/
	third = (listunit*)malloc(sizeof(listunit));
	third->count = 4;
	third->child = (listunit**)malloc(third->count*sizeof(listunit));
	third->child[3] = third->child[2] = third->child[1] = third->child[0] = NULL;
	third->menu = (char**)malloc(third->count*sizeof(char*));
	third->menu[0] = "G";
	third->menu[1] = "H";
	third->menu[2] = "I";
	third->menu[3] = "J";
	third->border = 1;
	second = (listunit*)malloc(sizeof(listunit));
	second->count = 3;
	second->child = (listunit**)malloc(second->count*sizeof(listunit));
	second->child[1] = second->child[2] = NULL;
	second->child[0] = third;
	second->menu = (char**)malloc(second->count*sizeof(char*));
	second->menu[0] = "D";
	second->menu[1] = "E";
	second->menu[2] = "F";
	second->border = 1;
	first = (listunit*)malloc(sizeof(listunit));
	first->count = 3;
	first->child = (listunit**)malloc(first->count*sizeof(listunit));
	first->child[2] = first->child[0] = NULL;
	first->child[1] = second;
	first->menu = (char**)malloc(first->count*sizeof(char*));
	first->menu[0] = "A";
	first->menu[1] = "B";
	first->menu[2] = "C";
	first->x = 3;
	first->y = 3;
	first->border = 1;

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
	
	
	selectRet = Mem_PullDown_menu(NULL,0,first,targAddr);/*Main Menu*/
	printf("Return Value:%d\n",selectRet);

	free(first);
	free(second);
	free(third);

	bioskey(0);

	return 0;
}