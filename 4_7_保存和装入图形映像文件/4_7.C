#include<BIOS.H>
#include<DOS.H>
#include<stdio.h>
#include<string.h>
double delta;
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
int SetMode(int mode)
{/*设置显示方式，mode为模式号*/
	union REGS r;
	r.h.al = mode;
	r.h.ah = 0;
	int86(0x10,&r,&r);
	return 0;
}
int SetPallette(int pnum)
{/*设置调色板*/
	union REGS r;
	r.h.bh = 1;
	r.h.bl = pnum;
	r.h.ah = 11;
	int86(0x10,&r,&r);
	return 0;
}
int Mempoint(int x,int y,int color)
{/*在指定坐标画点*/
	union mask
	{
		char c[2];
		int i;
	}bit_mask;
	int i,index,bit_position;
	unsigned char t;
	char xor;
	char far *ptr = (char far *)0xB8000000;/*指向CGA/VGA显示区*/
	bit_mask.i = 0xFF3F;
	if (x<0||x>199||y<0||y>319)/*超出显示区*/
		return 1;
	xor = color & 128;/*检查*/
	color &= 127;
	bit_position = y%4;
	color <<= 2*(3-bit_position);
	bit_mask.i >>= 2*bit_position;
	index = x*40+(y/4);
	if (x%2)
		index += 8152;
	if (!xor)
	{
		t = *(ptr+index) & bit_mask.c[0];
		*(ptr+index) = t|color;
	}
	else
	{
		t = *(ptr+index)|(char)0;
		*(ptr+index) = t^color;
	}
	return 0;
}
int MemCircle(int x_center,int y_center,int r,int color)
{/*Bresenham法画圆*/
	register int dx = 0;
	register int dy = r;/*画圆起点从最右点开始*/
	delta = 3 - r - r;/*判别式初始值*/
	Mempoint(x_center,y_center + dy,color);
	Mempoint(x_center,y_center - dy,color);
	Mempoint(x_center + dy,y_center,color);
	Mempoint(x_center - dy,y_center,color);
	while(dx <= dy)
	{
		if (delta >= 0)/*下一个点在圆内*/
		{
			delta += 4 * (dx - dy) + 10;
			--dy;
		}
		else/*下一个点在圆外*/
			delta += 4 * dx + 6;
		++dx;
		Mempoint(x_center + dx,y_center + dy,color);
		Mempoint(x_center + dx,y_center - dy,color);
		Mempoint(x_center - dx,y_center + dy,color);
		Mempoint(x_center - dx,y_center - dy,color);
		Mempoint(x_center + dy,y_center + dx,color);
		Mempoint(x_center + dy,y_center - dx,color);
		Mempoint(x_center - dy,y_center + dx,color);
		Mempoint(x_center - dy,y_center - dx,color);
	}
	return 0;
}
int MemFillCircle(int x_center,int y_center,int r,int color)
{/*以color填充圆*/
	r--;
	while (r)
		MemCircle(x_center,y_center,r--,color);
	return 0;
}
int save_pic()
{/*保存文件*/
	char fname[80] = {0};/*存放文件名*/
	FILE* fp;
	register int i,j;
	char far *ptr = (char far*)0xB8000000;/*指向屏幕缓冲区*/
	unsigned char buf[14][80] = {{0}};/*存放保存的前14行屏幕信息*/
	char far *temp = ptr;/*临时游标*/
	for (i=0;i<14;i++)
	{
		for (j=0;j<80;j++)
		{
			buf[i][j] = *temp;/*偶单元*/
			buf[i][j+1] = *(temp+8152);/*奇单元*/
			*temp = 0;
			*(temp+8152) = 0;/*原地址屏幕清零*/
			temp++;
		}
	}
	goto_xy(0,0);
	printf("Input FileName:");
	gets(fname);/*提示并输入文件名*/
	if (!(fp = fopen(fname,"wb")))
	{
		printf("open file error!\n");
		return 1;
	}
	temp = ptr;/*临时游标复位*/
	for (i=0;i<14;i++)/*开始恢复前14行*/
	{
		for (j=0;j<80;j++)
		{
			*temp = buf[i][j];/*偶单元*/
			*(temp+8152) = buf[i][j];/*奇单元*/
			temp++;
		}
	}
	for (i=0;i<8152;i++)
	{/*开始保存图像 */
		putc(*ptr,fp);
		putc(*(ptr+8152),fp);
		ptr++;
	}
	fclose(fp);
	return 0;
}
int load_pic()
{
	char fname[80] = {0};/*保存输入的待打开的图片*/
	FILE *fp;
	register int i,j;
	char far *ptr = (char far *)0xB8000000;/*指向屏幕缓冲区*/
	unsigned char buf[14][80] = {{0}};
	char far *temp = ptr;
	for (i=0;i<14;i++)
	{/*开始前14行清屏*/
		for (j=0;j<80;j++)
		{
			buf[i][j] = *temp;/*偶单元*/
			buf[i][j+1] = *(temp+8152);/*奇单元*/
			*temp = 0;
			*(temp+8152) = 0;/*原地址屏幕清零*/
			temp++;
		}
	}
	goto_xy(0,0);
	printf("Input FileName:");
	gets(fname);
	if (!(fp = fopen(fname,"rb")))
	{
		printf("open file error!\n");
		temp = ptr;/*临时游标复位*/
		for (i=0;i<14;i++)/*开始恢复前14行*/
		{
			for (j=0;j<80;j++)
			{
				*temp = buf[i][j];/*偶单元*/
				*(temp+8152) = buf[i][j];/*奇单元*/
				temp++;
			}
		}
		return 1;
	}
	temp = ptr;/*游标复位*/
	for (i=0;i<8152;i++)
	{
		*ptr = getc(fp);
		*(ptr+8152) = getc(fp);
		ptr++;
	}
	fclose(fp);
	return 0;
}
int main(void)
{
	SetMode(4);/*设置显示模式为4*/
	SetPallette(0);/*使用0号调色板*/
	MemCircle(60,60,30,1);/*x20,y20处绘制半径为10,颜色为绿的圆*/
	MemFillCircle(60,60,30,1);/*填充上述圆为绿色*/
	bioskey(0);
	save_pic();
	bioskey(0);
	load_pic();
	bioskey(0);
	return 0;
}