#include<DOS.H>
int main(void)
{
	union REGS in,out;
	in.h.ah = 2;		/*SetCursorPos*/
	in.h.bh = 0;		/*Page 0*/
	in.h.dh = 3;		/*Line 3*/
	in.h.dl = 5;		/*Row  5*/
	int86(0x10,&in,&out);
	in.h.ah = 0x0A;		/*ShowChar*/
	in.h.al = 0x31;		/*Show 1*/
	in.h.bh = 0;            /*Page 0*/
	in.h.bl = 0x20;        
	in.h.cl = 1;		/*1 Time*/
	int86(0x10,&in,&out);

	in.h.ah = 0x07;		/*InputChar*/
	int86(0x21,&in,&out);	/*Pause*/
	return 0;
}