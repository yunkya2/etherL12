/* ether_ne.sys 用メモリコピールーチン */

#ifndef MEMCPY_H
#define MEMCPY_H

#include <x68k/iocs.h>


/* データポートからワード単位読み込み */
//static __inline void
void
insw (volatile unsigned short* port, unsigned short* dst, unsigned int len)
{
  register unsigned int reg_d0 __asm ("d0");
  register unsigned int reg_d1 __asm ("d1") = len;
  register unsigned short* reg_a0 __asm ("a0") = dst;

  __asm __volatile (
	"	lsr	#1,%3\n"
	"	subq	#1,%3\n"
	"	bcs	9f\n"
	"	btst	#0,%4\n"
	"	beq	5f\n"
	"	cmpi.b	#2,(0xcbc)\n"
	"	bcc	5f\n"
	"1:\n"
	"	move	(%1),%0\n"
	"	move	%0,-(%%sp)\n"
	"	move.b	(%%sp)+,(%2)+\n"
	"	move.b	%0,(%2)+\n"
	"	dbra	%3,1b\n"
	"	bra	9f\n"
	"5:\n"
	"	moveq	#0xf,%0\n"
	"	and	%3,%0\n"
	"	neg	%0\n"
	"	lsr	#4,%3\n"
	"	add	%0,%0\n"
	"	jmp	(6f-2,%%pc,%0.w)\n"
	"	.quad\n"
	"50:\n"
	"	.rept	16\n"
	"	move	(%1),(%2)+\n"
	"	.endr\n"
	"6:	dbra	%3,50b\n"
	"9:\n"
	"	btst	#0,%5\n"
	"	beq	10f\n"
	"	move	(%1),-(%%sp)\n"
	"	move.b	(%%sp)+,(%2)+\n"
	"10:\n"
		: "=d" (reg_d0)
		: "a" (port), "a" (reg_a0), "d" (reg_d1), "d" (dst), "d" (len)
		);
//		: "d0", "d1", "a0");

}


/* データポートへワード単位書き込み */
//static __inline void
void
outsw (volatile unsigned short* port, const unsigned short* src, unsigned int len)
{
  register unsigned int reg_d0 __asm ("d0");
  register unsigned int reg_d1 __asm ("d1") = len;
  register const unsigned short* reg_a0 __asm ("a0") = src;

  __asm __volatile (
	"1:	lsr	#1,%3\n"
	"	subq	#1,%3\n"
	"	bcs	9f\n"
	"	btst	#0,%4\n"
	"	beq	5f\n"
	"	cmpi.b	#2,(0xcbc)\n"
	"	bcc	5f\n"
	"1:\n"
	"	move.b	(%2)+,-(%%sp)\n"
	"	move	(%%sp)+,%0\n"
	"	move.b	(%2)+,%0\n"
	"	move	%0,(%1)\n"
	"	dbra	%3,1b\n"
	"	bra	9f\n"
	"5:\n"
	"	moveq	#0xf,%0\n"
	"	and	%3,%0\n"
	"	neg	%0\n"
	"	lsr	#4,%3\n"
	"	add	%0,%0\n"
	"	jmp	(6f-2,%%pc,%0.w)\n"
	"	.quad\n"
	"50:\n"
	"	.rept	16\n"
	"	move	(%2)+,(%1)\n"
	"	.endr\n"
	"6:	dbra	%3,50b\n"
	"9:\n"
	"	btst	#0,%5\n"
	"	beq	10f\n"
	"	move.b	(%2)+,-(%%sp)\n"
	"	move	(%%sp)+,(%1)\n"
	"10:\n"
		: "=d" (reg_d0)
		: "a" (port), "a" (reg_a0), "d" (reg_d1), "d" (src), "d" (len)
		);
//		: "d0", "d1", "a0");

}


#if 0
/************************************************
 *						*
 ************************************************/
static void*
memcpy (void* dst, const void* src, size_t size)
{
  const unsigned char* q;
  unsigned char* p;
  int y;

  if (!(p = dst) || !(q = src))
    return dst;

  for (y = 0; y < size; y++)
    *p++ = *q++;

  return dst;
}
#endif


#endif /* !MEMCPY_H */

/* EOF */
