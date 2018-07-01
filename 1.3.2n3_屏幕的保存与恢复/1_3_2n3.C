#include<DOS.H>
#include<malloc.h>
int goto_xy(int,int);
int ShowBG(void)
{
	union REGS in,out;
	int ch = 48;				/*Char == '0'*/
	while (ch<58)
	{
		in.h.ah = 0x0A;			/*ShowChar*/
		in.h.al = ch;
		in.h.bh = 0;			/*Page 0*/
		in.h.bl = 0x20;
		in.h.cl = 3;			/*Show 3 Times*/
		int86(0x10,&in,&out);
		ch++;
		goto_xy(ch-48,0);		/*TransToNextLine*/
	}
	return 0;
}
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
union REGS** SaveBG()
{
	int i,j;                            	/*i:Line,j:Row*/
	union REGS t;
	union REGS **buffer = NULL;		/*BufferSize:43*80*/
	buffer = (union REGS**)malloc(43*sizeof(union REGS*));
	for (i=0;i<43;i++)
		buffer[i] = (union REGS*)malloc(80*sizeof(union REGS));
	for (i=0;i<43;i++)
	{
		for (j=0;j<80;j++)
		{
			goto_xy(i,j);           /*SetPos*/
			t.h.ah = 0x08;		/*SavePosInfo*/
			t.h.bh = 0;
			int86(0x10,&t,&buffer[i][j]);
		}
	}
	return buffer;
}
int Clear()
{
	union REGS in;
	int i=0;
	while (i<43)
	{
		goto_xy(i++,0);
		in.h.ah = 0x0A;
		in.h.al = 0x00;
		in.h.bh = 0x00;
		in.h.bl = 0x00;
		in.h.cl = 0x50;				/*Clear*/
		int86(0x10,&in,&in);
	}
	return 0;
}
int RecoverBG(union REGS **buffer)
{
	int i,j;
	union REGS tbuffer;
	for (i=0;i<43;i++)
	{
		for (j=0;j<80;j++)
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
int PAUSE()
{
	union REGS in;
	in.h.ah = 0x07;
	int86(0x21,&in,&in);
	return 0;
}
int main(void)
{
	union REGS **buf = NULL;
	ShowBG();
	PAUSE();
	buf = SaveBG();
	Clear();
	PAUSE();
	RecoverBG(buf);
	PAUSE();
	return 0;
}