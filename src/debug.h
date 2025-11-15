/* ether_ne.sys 用デバッグルーチン */

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdarg.h>

/* DEBUG_PRINTF マクロは、引数を二重括弧 (( )) で囲って下さい */
/* 例: DEBUG_PRINTF (("value = %d\n", num)); */
#ifdef DEBUG_S
static void
debug_printf (const char* fmt, ...)
{
  char buf [2048];
  va_list ap;

  va_start (ap, fmt);
  vsnprintf (buf, sizeof buf, fmt, ap);
  va_end (ap);
}
#define DEBUG_PRINTF(args)	(void) debug_printf args
#else
#define DEBUG_PRINTF(args)
#endif

void DPRINTF(char *fmt, ...);

#define KBDLED_OFF	0x00
#define KBDLED_KANA	0x01
#define KBDLED_ROMA	0x02
#define KBDLED_CODE	0x04
#define KBDLED_CAPS	0x08
#define KBDLED_INS	0x10
#define KBDLED_HIRA	0x20
#define KBDLED_ZEN	0x40

#ifdef DEBUG_LED
#define DEBUG_KBDLED(n)	(void) (*(unsigned char*) 0xe8802f = 0x80 | (~(n) & 0x7f))
#else
#define DEBUG_KBDLED(n)
#endif


#endif /* !DEBUG_H */

/* EOF */
