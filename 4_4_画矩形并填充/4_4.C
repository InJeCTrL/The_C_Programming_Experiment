#include<BIOS.H>
#include<DOS.H>
#include<stdio.h>
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
int MemLine(int startx,int starty,int endx,int endy,int color)
{/*Bresenham画线*/
	register int t,xerr=0,yerr=0;
	int incx,incy,distance;
	int detx = endx - startx;
	int dety = endy - starty;/*计算横纵坐标变化*/

	if (detx>0)/*若x增量大于0，步进1；增量0，步进0；增量小于0，步进-1*/
		incx = 1;
	else if (detx == 0)
		incx = 0;
	else
		incx = -1;
	if (dety>0)/*若y增量大于0，步进1；增量0，步进0；增量小于0，步进-1*/
		incy = 1;
	else if (dety == 0)
		incy = 0;
	else
		incy = -1;

	detx = abs(detx);
	dety = abs(dety);
	if (detx>dety)/*变化量大的轴选为步进主轴*/
		distance = detx;
	else
		distance = dety;

	for (t=0;t<=distance+1;t++)
	{
		Mempoint(startx,starty,color);
		xerr += detx;
		yerr += dety;
		if (xerr > distance)
		{
			xerr -= distance;
			startx += incx;
		}
		if (yerr > distance)
		{
			yerr -= distance;
			starty += incy;
		}
	}
	return 0;
}
int MemRect(int startx,int starty,int endx,int endy,int color)
{/*画矩形*/
	MemLine(startx,starty,startx,endy,color);/*上*/
	MemLine(startx,endy,endx,endy,color);/*右*/
	MemLine(endx,starty,endx,endy,color);/*下*/
	MemLine(startx,starty,endx,starty,color);/*左*/
	return 0;
}
int Mem_FillRect(int startx,int starty,int endx,int endy,int color)
{/*填充两坐标所围矩形*/
	for (;startx<=endx;startx++)
		MemLine(startx,starty,startx,endy,color);
	return 0;
}
int main(void)
{
	SetMode(4);/*设置显示模式为4*/
	SetPallette(0);/*使用0号调色板*/
	MemRect(2,2,36,36,2);/*(2,2)->(36,36)处打印红框*/
	Mem_FillRect(3,3,35,35,2);/*将上述红框填充红色xx*/
	bioskey(0);
	return 0;
}