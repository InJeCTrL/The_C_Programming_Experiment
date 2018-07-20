#include"DOS.H"
#include"BIOS.H"
#include<stdio.h>
#include<string.h>
#include<malloc.h>
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
union REGS** SaveBG(int x,int endx,int y,int endy)
{
	int i,j;                            	/*i:Line,j:Row*/
	union REGS t;
	union REGS **buffer = NULL;		/*BufferSize:(endx-x+1)*(endy-x+1)*/
	buffer = (union REGS**)malloc((endy-y+1)*sizeof(union REGS*));
	for (i=x;i<endx;i++)
		buffer[i] = (union REGS*)malloc((endx-x+1)*sizeof(union REGS));
	for (i=x;i<endx;i++)
	{
		for (j=y;j<endy;j++)
		{
			goto_xy(i,j);           /*SetPos*/
			t.h.ah = 0x08;		/*SavePosInfo*/
			t.h.bh = 0;
			int86(0x10,&t,&buffer[i][j]);
		}
	}
	return buffer;
}
int RecoverBG(union REGS **buffer,int x,int endx,int y,int endy)
{
	int i,j;
	union REGS tbuffer;
	for (i=x;i<endx;i++)
	{
		for (j=y;j<endy;j++)
		{
			goto_xy(i,j);
			tbuffer.h.al = buffer[i][j].h.al;
			tbuffer.h.bl = buffer[i][j].h.ah;
			tbuffer.h.ah = 0x09;		/*SetPosInfo*/
			tbuffer.h.bh = 0;
			tbuffer.h.cl = 1;
			int86(0x10,&tbuffer,&tbuffer);

		}
	}
	return 0;
}
int display_menu(char *menu[],int x,int y,int count)
{/*menu数组存放各个选项文字，xy为显示菜单的坐标，count为选项个数*/
	register int i;
	union REGS in;

	for (i=0;i<count;i++)
	{/*输出count个菜单项*/
		goto_xy(x,y);
		printf("%s",menu[i]);
		x++;				/*向下偏移一行*/
	}
	return 0;
}
int draw_border(int startx,int starty,int endx,int endy)
{
	register int i;
	for (i=startx+1;i<endx;i++)
	{/*打印左右边框，除去四角*/
		goto_xy(i,starty);
		printf("|");
		goto_xy(i,endy);
		printf("|");
	}
	for (i=starty+1;i<endy;i++)
	{/*打印上下边框，除去四角*/
		goto_xy(startx,i);
		printf("-");
		goto_xy(endx,i);
		printf("-");
	}
	goto_xy(startx,starty);
	printf(" ");/*左上角*/
	goto_xy(startx,endy);
	printf(" ");/*右上角*/
	goto_xy(endx,starty);
	printf(" ");/*左下角*/
	goto_xy(endx,endy);
	printf(" ");/*右下角*/
	return 0;
}
int write_video(int x,int y,char *pStr,int attrib)
{/*传入xy表示在xy处开始写屏幕内存，pStr指向欲写入的字符串，attrib标识写入的属性*/
	int i = 0;
	union REGS in;
	goto_xy(x,y);
	while(pStr[i])/*pStr未到达结尾*/
	{
		in.h.ah = 0x09;
		in.h.al = pStr[i];
		in.h.bh = 0;
		in.h.bl = attrib;
		in.h.cl = 1;			/*仅输出一次*/
		int86(0x10,&in,&in);
		i++;					/*字符串向后偏移*/
	}
	return 0;
}
int get_resp(int x,int y,int count,char *menu[])
{/*空格向下移动光标，Enter确定，ESC关闭菜单*/
	char ch;					/*记录击键ASCII*/
	int det,i = 0;
	x++;y++;
	det = x - i;
	write_video(x,y,menu[i],0x70);
	while (ch = bioskey(0))			/*捕获击键*/
	{
		if (ch == '1')			/*若击键为1*/
		{
			write_video(x,y,menu[i],7);	/*Norm_Display常规显示*/
			i++;
			i %= count;
			x = det + i;

			write_video(x,y,menu[i],0x70);	/*Rev_VID反色显示*/
		}
		else if (ch == 13)		/*若击键为回车*/
			return i;		/*返回行号*/
		else if (ch == 27)
			return -1;			/*若ESC被按下则返回-1*/
	}
}
int popup_menu(char *menu[],int count,int x,int y,int border)
{/*传入menu：需要显示的列表文字，count：菜单项个数，xy显示popup菜单位置，border：是否显示边框
 返回-2：创建失败，返回-1：用户退出菜单，否则返回菜单项下标*/
	register int i,len;
	int endx,endy,choice,ret;
	unsigned int *p;
	union REGS** pBuff = NULL;
	if (x>24 || x<0 || y>79 || y<0)/*检测起始位置是否超出屏幕限制*/
	{
		printf("Screen Buff Overflow!\n");
		return -2;
	}
	len = 0;
	for (i=0;i<count;i++)
	{/*/得到popup菜单边界*/
		if (strlen(menu[i]) > len)
			len = strlen(menu[i]);
	}
	endy = len + 2 + y;/*2为边框宽度*/
	endx = count + 1 + x;/*1为边框宽度*/
	if ((endx+1>24) || (endy+1>29))/*菜单超限*/
	{
		printf("Screen Buff Overflow!\n");
		return -2;
	}
	pBuff = SaveBG(x,endx,y,endy);
	if (border)
		draw_border(x,y,endx,endy);
	display_menu(menu,x+1,y+1,count);
	ret = get_resp(x,y,count,menu);
	RecoverBG(pBuff,x,endx,y,endy);
	return ret;
}
int main(void)
{
	char *menu[] = {"A","B","C"};
	int selectRet;
	selectRet = popup_menu(menu,3,5,5,1);/*3个菜单项，x5y5，添加边框*/
	printf("Return Value:%d\n",selectRet);

	bioskey(0);

	return 0;
}
