/**********************************************************************
* Network and Ethernet Project for TUNE-up X680x0   <Neptune-X>       *
*                                                                     *
*     Neptune-X board driver for  Human-68k(ESP-X)   version 0.03     *
*                                                                     *
*         Programed 1996-7 by Shi-MAD.                                *
*                   Special thanks to  Niggle, FIRST, yamapu ...      *
**********************************************************************/
/*
 * ne2000.c
 */


#include <stdio.h>
#include <sys/dos.h>

#include "main.h"
#include "ne2000.h"
#include "memcpy.h"
#include "debug.h"


#define	NE_DATAPORT	0x20
#define	NE_RESET	0x3e
#define NE_BASEADR	0xece000	/* NE-2000 MemoryMaped Base I/O Address */
#define	NEPTUNE_IVN	0xf9		/* Neptune-X 割り込みベクタ */

#define	MEM_START	0x4000		/* NE2000 ローカルメモリ先頭アドレス */
#define	MEM_END		0x8000		/* NE2000 ローカルメモリ終端アドレス */
#define	MEM_SIZE	0x4000		/* NE2000 ローカルメモリサイズ */
#define	MEM_RING	0x4c00		/* NE2000 受信リングバッファ先頭アドレス */

#define	TX_PAGE_START	64		/* NE2000 先頭 送信ページ */
#define	REC_PAGE_START	76		/* NE2000 先頭 受信ページ */
#define	REC_PAGE_STOP	128		/* NE2000 終端 受信ページ */
#define	TXB_CNT		2		/* NE2000 送信 バッファ数 */

#define	ED_PAGE_SIZE	256
#define	ED_PAGE_MASK	256
#define	ED_PAGE_SHIFT	8
#define	ED_TXBUF_SIZE	6		/* NE2000 送信バッファページ数 */

#define	ETHER_MIN_LEN	64
#define ETHER_MAX_LEN	1518
#define	ETHER_ADDR_LEN	6


/* asmsub.s 内のサブルーチン */
extern void DI ();
extern void EI ();


struct ed_softc ed_softc;
struct trans_counter trans_counter;
unsigned char rx_buff [2048];		/* 受信バッファ */


#define SELECT_PAGE_0(n)	(io_addr [E8390_CMD] = E8390_PAGE0 + n)
#define SELECT_PAGE_1(n)	(io_addr [E8390_CMD] = E8390_PAGE1 + n)


/*********************************************************************
*    ＤＰ８３９０  リセット                                          *
*********************************************************************/
static void
DP8390_Reset (volatile unsigned char* baseadr)
{
  int i;
  unsigned char in;

  in = *(baseadr + NE_RESET);
  baseadr [NE_RESET] = in;

  for (i=0; i < 30000; i++)
    {
      if ((baseadr [EN0_ISR] & ENISR_RESET) !=0 )
	break;
    }
}


/************************************************
 *   ＭＡＣ  アドレス取得                       *
 ************************************************/
int
GetMacAddr (struct eaddr* buf)
{
  unsigned char* p;
  int i;

  p = (unsigned char*) buf;
  if (!p)
    return -1;

  /*** MAC アドレスをDP8390レジスタから取得 ***/
  SELECT_PAGE_1 (E8390_STOP);

  for (i = 0; i < 6; i++)
    {
      p [i] = io_addr [EN1_PHYS + i * 2];
    }

  return 0;
}

/************************************************
 *   ＭＡＣ  アドレス設定                       *
 ************************************************/
int
SetMacAddr (const struct eaddr* data)
{
  const unsigned char* p;
  int i;

  p = (unsigned char*) data;
  if (!p)
    return -1;

  /*** MAC アドレスをDP8390に設定 ***/
  SELECT_PAGE_1 (E8390_STOP);

  for (i = 0; i < 6; i++)
    {
      io_addr [EN1_PHYS + i * 2] = p [i];
    }

  return 0;
}


/************************************************
 *	dstが読み込み可能かチェック		*
 ************************************************/
static int
read_check (unsigned short* dst)
{
  unsigned short q;

  if (_dos_memcpy (dst, &q, 2))
    return 0;
  else
    return 1;
}


/************************************************
 *  Neptune-X を さがします                     *
 ************************************************/
int
SearchNeptuneX (volatile unsigned char** io_addr, unsigned int* irq)
{
  volatile unsigned char* p;
  int i;
  unsigned char in;

  for (i = 0; i < 0x400; i += 0x40)
    {
      p = (unsigned char*) NE_BASEADR + i;

      if (read_check ((unsigned short*) p))
	{
	  /* すくなくとも読みこみ可能 */
	  p [E8390_CMD] = E8390_NODMA + E8390_PAGE1 + E8390_STOP;
	  p [0x0d * 2] = 0xff;
	  p [E8390_CMD] = E8390_NODMA + E8390_PAGE0;
	  in = p [EN0_COUNTER0];

	  if (p [EN0_COUNTER0] != 0)
	    continue;			/* はずれ */

	  /* NE-2000系 らしい */	/* すごーい、手抜き */
	  *io_addr = p;
	  *irq = NEPTUNE_IVN;
	  return 0;
	}
    }

  return -1;
}


/************************************************
 * Neptune-X ベクタ設定                         *
 ************************************************/
int
TrapNeptuneVector (unsigned int irq, void* my_handler)
{
  _dos_intvcs (irq, my_handler);
  return 0;
}


/************************************************
 *  Neptune-X ボード初期化 関数                 *
 ************************************************/
int
InitNeptune (void)
{
#ifdef MULTICAST
  unsigned char multicast_table [8] = {0, 0, 0, 0, 0, 0, 0, 0};
#endif
  unsigned char prom [32];
  int i,wl;

  DEBUG_KBDLED (KBDLED_OFF);

  DP8390_Reset (io_addr);

  SELECT_PAGE_0 (E8390_NODMA + E8390_STOP);

  io_addr [EN0_DCFG] = 0x48;		/* set byte-transfer mode */
		    /* 0x49; */		/* set word-transfer mode */

  io_addr [EN0_RCNTLO] = 0x00;
  io_addr [EN0_RCNTHI] = 0x00;
  io_addr [EN0_IMR] = 0x00;		/* すべてマスク */
  io_addr [EN0_ISR] = 0xff;		/* 割り込みステータスリセット */
  io_addr [EN0_RXCR] = E8390_RXOFF;
  io_addr [EN0_TXCR] = E8390_TXOFF;


/* ＥＥＰＲＯＭよりＭＡＣアドレスなどを読み込みます */
  io_addr [EN0_RCNTLO] = 32;
  io_addr [EN0_RCNTHI] = 0;		/* 転送サイズ３２バイト */
  io_addr [EN0_RSARLO] = 0x00;
  io_addr [EN0_RSARHI] = 0x00;		/* 内蔵メモリ番地は０に */
  io_addr [E8390_CMD] = E8390_RREAD + E8390_START;	/* 読み込み開始 */

  wl = 2;
  for (i = 0; i < 32; i += 2)
    {
      prom [i + 0] = io_addr [NE_DATAPORT];
      prom [i + 1] = io_addr [NE_DATAPORT];

      if (prom [i + 0] != prom [i + 1])
	wl = 1;		/* ８ビットカード(NE-1000) */
    }

  if (wl == 2)
    {			/* １６ビットカード(NE-2000) */
      io_addr [EN0_DCFG] = 0x49;	/* set word-transfer mode */

      for (i = 0; i < 16; i++)
	prom [i] = prom [i + i];
    }
  else
    {			/* ８ビットカード(NE-1000) */
#if 0
      Print("\nNE1000\n\n");
#endif
      return -1;	/* 今回はNE-1000に関しては無視 */
    }

#if 0
  {
    unsigned char buf[32];
    sprint_eaddr (buf, prom);
    Print ("\nボードのＭＡＣアドレスは、");
    Print (buf);
    Print ("です。\n");
  }
#endif


  /*** MAC アドレスをDP8390に設定 ***/
  SELECT_PAGE_1 (E8390_STOP);

  for (i = 0; i < 6; i++)
    {
      io_addr [EN1_PHYS + i * 2] = prom [i];
    }


  /* 送信フラグ初期化 */
  ed_softc.xmit_busy = 0;
  ed_softc.txb_inuse = 0;
  ed_softc.txb_new = 0;
  ed_softc.txb_next_tx = 0;

  ed_softc.next_packet = REC_PAGE_START + 1;

  SELECT_PAGE_0 (E8390_NODMA + E8390_STOP);

  io_addr [EN0_DCFG] = 0x49;	/* １６ビット転送、ループバック解除、FIFOスレッショルド８バイト */


  /* リモートカウンタリセット*/
  io_addr [EN0_RCNTLO] = 0x00;
  io_addr [EN0_RCNTHI] = 0x00;

  /* 受信パケットをバッファリングしない */
  io_addr [EN0_RXCR] = E8390_RXOFF;

  /* 送信しない */
  io_addr [EN0_TXCR] = E8390_TXOFF;


  io_addr [EN0_TPSR] = TX_PAGE_START;
  io_addr [EN0_STARTPG] = REC_PAGE_START;

  io_addr [EN0_STOPPG] = REC_PAGE_STOP;
  io_addr [EN0_BOUNDARY] = REC_PAGE_START;

  io_addr [EN0_ISR] = 0xff;		/* 割り込みステータスリセット */
  io_addr [EN0_IMR] = 0x00;		/* 割り込み禁止 */

  SELECT_PAGE_1 (E8390_NODMA + E8390_STOP);

  io_addr [EN1_CURPAG] = ed_softc.next_packet;

  SELECT_PAGE_0 (E8390_NODMA + E8390_STOP);

  /* パケット受信許可 ＆ ブロードキャストパケット許可 */
  io_addr [EN0_RXCR] = 0x04;

  /* 送信許可 */
  io_addr [EN0_TXCR] = 0x00;

  SELECT_PAGE_0 (E8390_NODMA + E8390_START);	/* 動作開始 */

  ed_softc.semaphore = 0;		/* 出力はアクティブじゃない */

  return 0;
}


/************************************************
 *  Neptune-X ドライバ初期化   関数             *
 ************************************************/
#ifdef ENABLE_EDINIT
static int
EdInit (struct ed_softc* sc)
{

  SELECT_PAGE_0 (E8390_NODMA + E8390_STOP);

  /* 送信フラグ初期化 */
  sc->xmit_busy = 0;
  sc->txb_inuse = 0;
  sc->txb_new = 0;
  sc->txb_next_tx = 0;

  sc->next_packet = REC_PAGE_START + 1;

  io_addr [EN0_DCFG] = 0x49;	/* １６ビット転送、ループバック解除、FIFOスレッショルド８バイト */

  /* リモートカウンタリセット*/
  io_addr [EN0_RCNTLO] = 0x00;
  io_addr [EN0_RCNTHI] = 0x00;

  /* 受信パケットをバッファリングしない */
  io_addr [EN0_RXCR] = E8390_RXOFF;

  /* 送信しない */
  io_addr [EN0_TXCR] = E8390_TXOFF;


  io_addr [EN0_TPSR) = TX_PAGE_START;
  io_addr [EN0_STARTPG) = REC_PAGE_START;

  io_addr [EN0_STOPPG) = REC_PAGE_STOP;
  io_addr [EN0_BOUNDARY) = REC_PAGE_START;

  io_addr [EN0_ISR) = 0xff;		/* 割り込みステータスリセット */
  io_addr [EN0_IMR] = ENISR_RX + ENISR_TX + ENISR_RX_ERR + ENISR_TX_ERR + ENISR_OVER;
					/* 割り込み許可 */

  SELECT_PAGE_1 (E8390_NODMA + E8390_STOP);

  io_addr [EN1_CURPAG] = sc->next_packet;

  SELECT_PAGE_0 (E8390_NODMA + E8390_STOP);

  /* パケット受信許可 ＆ ブロードキャストパケット許可 */
  io_addr [EN0_RXCR] = 0x04;

  /* 送信許可 */
  io_addr [EN0_TXCR] = 0x00;

  SELECT_PAGE_0 (E8390_NODMA + E8390_START);	/* 動作開始 */

  return 0;
}
#define EDINIT(sc)	EdInit (sc)
#else
#define EDINIT(sc)
#endif


/************************************************
 *  Neptune-X NE2000パケットバッファから転送    *
 ************************************************/
static void
EdPioReadMem (unsigned short src, unsigned char* dst, unsigned int len)
{

  DEBUG_KBDLED (KBDLED_ZEN);

  if (len < 1)
    return;

  SELECT_PAGE_0 (E8390_NODMA + E8390_START);

  /* Set up DMA byte count. */
  io_addr [EN0_RCNTLO] = len & 0xff;
  io_addr [EN0_RCNTHI] = len >> 8;

  /* Set up destination address in NIC mem. */
  io_addr [EN0_RSARLO] = src & 0xff;
  io_addr [EN0_RSARHI] = src >> 8;

  /* Set remote DMA read. */
  io_addr [E8390_CMD] = E8390_RREAD + E8390_PAGE0 + E8390_START;

  /* NE2000受信バッファから読み込み */
  insw ((volatile unsigned short*) &io_addr [NE_DATAPORT],
				(unsigned short*) dst, len);

  DEBUG_KBDLED (KBDLED_HIRA | KBDLED_INS);
}


/************************************************
 *  Neptune-X NE2000受信リングバッファから転送  *
 ************************************************/
static int
EdRingToMem (unsigned short src, unsigned char* dst, unsigned int len)
{
  if (src + len > MEM_END)
    {
      unsigned int first_len  = MEM_END - src;

      EdPioReadMem (src, dst, first_len);

      len -= first_len;
      dst += first_len;
      src = MEM_RING;
    }

  EdPioReadMem (src, dst, len);

  return 0;
}


/************************************************
 *  Neptune-X  受信パケット 種別配送 関数       *
 ************************************************/
static int
SendToPrt (int type, int len)
{
  int_handler func;
  int n = 0;
  
  while ((func = SearchList2 (type, n)))
    {
      n++;
      func (len, (void*) &rx_buff, ID_EN0);
    }

  return 0;
}


/************************************************
 *  Neptune-X パケットとっちゃう 関数           *
 ************************************************/
static void
EdGetPacket (struct ed_softc* sc,
		unsigned short buf,		/* NE2000 パケットバッファ先頭アドレス */
		unsigned int len)		/* 上のながさ */
{
  int type;

  /* NE2000バッファから受信バッファメモリへパケット転送 */
  if (EdRingToMem (buf, rx_buff, len) == 0)
    {
      type = rx_buff [12] * 256 + rx_buff [13];
      SendToPrt (type, len);	/* パケットタイプでデータ分別 */
      return;
    }
}


/************************************************
 *  Neptune-X 受信割り込み処理 関数             *
 ************************************************/
static inline void
EdRint (struct ed_softc* sc)
{
  unsigned char boundary;
  unsigned int len;
  struct ed_ring packet_hdr;
  unsigned short int packet_ptr;

  DEBUG_KBDLED (KBDLED_CODE);

  SELECT_PAGE_1 (E8390_NODMA + E8390_START);

  while (sc->next_packet != io_addr [EN1_CURPAG])
    {
      /* 受信バッファブロックのアドレス計算 */
      packet_ptr = MEM_RING
		 + (sc->next_packet - REC_PAGE_START) * ED_PAGE_SIZE;

      /* NE2000バッファからパケットヘッダ取得 */
      EdPioReadMem (packet_ptr, (unsigned char*) &packet_hdr, sizeof (packet_hdr));
      trans_counter.send_byte += sizeof (packet_hdr);

      if (packet_hdr.rsr & 0x1e)
	return;

      /* パケットレングス取得 */
      len = packet_hdr.count_h << 8
	  | packet_hdr.count_l;

      /* パケットが最大長を越えたら… */
      if (len > ETHER_MAX_LEN)
	{
	  len &= ED_PAGE_SIZE - 1;

	  /* リングバッファ対策 */
	  if (packet_hdr.next_packet >= sc->next_packet)
	    len += (packet_hdr.next_packet - sc->next_packet) * ED_PAGE_SIZE;
	  else
	    len += ((packet_hdr.next_packet - REC_PAGE_START)
		    + (REC_PAGE_STOP - sc->next_packet)) * ED_PAGE_SIZE;
	}

      if ((packet_hdr.next_packet >= REC_PAGE_START)
       && (packet_hdr.next_packet <  REC_PAGE_STOP))
	{
	  /* 受信パケットいただき */
	  unsigned int rlen = len - sizeof (packet_hdr);

	  EdGetPacket (sc, packet_ptr + sizeof (packet_hdr), rlen);
	  trans_counter.recv_byte += rlen;
	  }
      else
	{
	  DEBUG_KBDLED (KBDLED_HIRA | KBDLED_INS | KBDLED_CODE);
	  return;
	}

      /* つぎの受信ブロックへ */
      sc->next_packet = packet_hdr.next_packet;

      /*
       * Update NIC boundary pointer - being careful to keep it one
       * buffer behind (as recommended by NS databook).
       */
      boundary = sc->next_packet - 1;
      if (boundary < REC_PAGE_START)
	boundary = REC_PAGE_STOP - 1;

      SELECT_PAGE_0 (E8390_NODMA + E8390_START);

      io_addr [EN0_BOUNDARY] = boundary;

      /*
       * Set NIC to page 1 registers before looping to top (prepare
       * to get 'CURR' current pointer).
       */
      SELECT_PAGE_1 (E8390_NODMA + E8390_START);

    } /* while() */
}


/************************************************
 *  Neptune-X 送信バッファ内容の送信  関数      *
 ************************************************/
static inline void
EdXmit (struct ed_softc* sc)
{
  unsigned short int len;

  DEBUG_KBDLED (KBDLED_KANA);

  if (sc->txb_inuse < 1)
    return;

  if (sc->xmit_busy)
    return;
  sc->xmit_busy = 1;			/* 送信中 */

  len = sc->txb_len [sc->txb_next_tx];	/* パケット長 代入 */

  while (sc->semaphore)
    {
      /* wait */
    }
  sc->semaphore = 1;

  /* 送信処理中は割り込み禁止 NE2000レジスタ使用権獲得 */
  DI ();

  SELECT_PAGE_0 (E8390_NODMA + E8390_START);

  /* 送信バッファスタートページ設定 */
  io_addr [EN0_TPSR] = TX_PAGE_START + sc->txb_next_tx * ED_TXBUF_SIZE;

  /* 送信長 設定 */
  io_addr [EN0_TCNTLO] = len & 0xff;
  io_addr [EN0_TCNTHI] = len >> 8;

  SELECT_PAGE_0 (E8390_NODMA + E8390_TRANS + E8390_START);

  /* 割り込み許可 */
  EI ();

  sc->semaphore = 0;

  /* 未送信バッファブロックをつぎのブロックへ */
  if (++sc->txb_next_tx == TXB_CNT)
    sc->txb_next_tx = 0;
}


/************************************************
 *  Neptune-X 割り込み処理 関数 (ne.sから)      *
 ************************************************/
void
NeptuneIntProcess (void)
{
  struct ed_softc* sc = &ed_softc;
  unsigned char isr;

#ifdef 0
  DEBUG_KBDLED (KBDLED_CAPS);
#endif

  SELECT_PAGE_0 (E8390_NODMA + E8390_START);

  isr = io_addr [EN0_ISR];		/* 割り込み原因を取得 */

  if (!isr)
    return;	/* なにもないぞ？ */

  for (; ; )	/* 泣いている（ぉ */
    {
      /* 割り込み原因を リセットします */
      io_addr [EN0_ISR] = isr;

      /* 割り込み原因 は 送信完了 または 送信エラー */
      if (isr & (ENISR_TX | ENISR_TX_ERR))
	{
#ifdef DEBUG_INT
	  unsigned char t;
	  t = io_addr [EN0_NCR];	/* 衝突回数 */
	  t = io_addr [EN0_TSR];	/* 送信ステータス */

	  Print ((isr & ENISR_TX_ERR) ? "原因:過剰衝突orFIFOアンダーラン\r\n"	/* 送信エラー発生 */
				      : "原因:送信完了\r\n");
#endif

	  /* 送信完了 でも 送信エラーでも */
	  sc->xmit_busy = 0;		/* セマフォ解放 */
	  sc->txb_inuse--;
	  EdXmit (sc);			/* 送信 */
	}

      /* 割り込み原因 は 受信完了 または 受信エラー または 受信バッファあふれ警告*/
      if (isr & (ENISR_RX | ENISR_RX_ERR | ENISR_OVER))
	{
#ifdef 0
	  DEBUG_KBDLED (KBDLED_ZEN | KBDLED_HIRA | KBDLED_CAPS);
#endif
	  /* 受信バッファがあふれちゃうぅ */
	  if (isr & ENISR_OVER)
	    EDINIT (sc);
	  else
	    {
#if 0
	      if (isr & ENISR_RX_ERR)
		;
	      else if (isr & ENISR_RX)
		;
#endif
	      EdRint (sc);		/* 受信処理 */
	    }
	}

      SELECT_PAGE_0 (E8390_NODMA + E8390_START);

#if 0
      if (isr & ENISR_COUNTERS)
	{
	  t = io_addr [EN0_COUNTER0];
	  t = io_addr [EN0_COUNTER1];
	  t = io_addr [EN0_COUNTER2];
      }
#endif

      isr = io_addr [EN0_ISR];
      if (!isr)
	return;

    } /* for(;;) */
}


/************************************************
 *  Neptune-X NE2000パケットバッファへ転送      *
 ************************************************/
static void
EdPioWriteMem (const unsigned char* src, unsigned short int dst, unsigned int len)
{

  DEBUG_KBDLED (KBDLED_HIRA);

  if (len < 1)
    return;

  SELECT_PAGE_0 (E8390_NODMA + E8390_START);

  /* Reset remote DMA complete flag. */
  io_addr [EN0_ISR] = ENISR_RDC;

  /* Set up DMA byte count. */
  io_addr [EN0_RCNTLO] = len & 0xff;
  io_addr [EN0_RCNTHI] = len >> 8;

  /* Set up destination address in NIC mem. */
  io_addr [EN0_RSARLO] = dst & 0xff;
  io_addr [EN0_RSARHI] = dst >> 8;

  /* Set remote DMA write. */
  io_addr [E8390_CMD] = E8390_RWRITE + E8390_PAGE0 + E8390_START;

  /* NE2000送信バッファへ書き込み */
  outsw ((volatile unsigned short*) &io_addr [NE_DATAPORT],
				(const unsigned short*) src, len);


#ifdef WAIT_AFTER_WRITEMEM
  /* ウェイト */
  {
    int maxwait = 100;	/* about 120usらしい */

    while ((io_addr [EN0_ISR] & ENISR_RDC) != ENISR_RDC && --maxwait)
      /* loop */;
#if 0
    if (!maxwait)
      EDINIT (sc);	/* EdReset (sc); */
#endif
  }
#endif

}


/************************************************
 *  Neptune-X 送信処理 関数                     *
 ************************************************/
static int
EdStart (unsigned int len, const unsigned char* data)
{
  struct ed_softc* sc = &ed_softc;
  unsigned short int buffer;

#ifdef DEBUG_INUSE
  DEBUG_KBDLED (KBDLED_ROMA | (sc->txb_inuse ? KBDLED_KANA : 0));
#else
  DEBUG_KBDLED (KBDLED_ROMA);
#endif

  /* 送信バッファが二つとも埋まっている場合は、 */
  /* 送信を開始してバッファが空くまで待つ	*/
  while (sc->txb_inuse == TXB_CNT)
    EdXmit (sc);

  while (sc->semaphore)
    {
      /* wait */
    }
  sc->semaphore = 1;

  /* 送信ブロックの先頭バッファメモリアドレスを計算 */
  buffer = MEM_START + (sc->txb_new * ED_TXBUF_SIZE * ED_PAGE_SIZE);

  /* 送信処理中は割り込み禁止 NE2000レジスタ使用権獲得 */
  DI ();

  /* 本体送信バッファからＮＥ２０００バッファへ転送 */
  EdPioWriteMem (data, buffer, len);
  trans_counter.send_byte += len;

  EI ();

  sc->semaphore = 0;

  sc->txb_len [sc->txb_new]
	= (len > ETHER_MIN_LEN) ? len : ETHER_MIN_LEN;

  sc->txb_inuse++;			/* １ブロック使用中と加算します */

  /* 送信バッファ 空きブロックをつぎのブロックへ */
  if (++sc->txb_new == TXB_CNT)
    sc->txb_new = 0;

  /* 送信中でなければ、送信する */
  if (sc->xmit_busy == 0)
    EdXmit (sc);			/* 送信 */

  return 0;
}


/************************************************
 *  Neptune-X パケット送信 関数 (ne.sから)      *
 ************************************************/
int
SendPacket (int len, const unsigned char* data)
{

  DEBUG_KBDLED (KBDLED_INS);
  DEBUG_PRINTF (("SendPacket() len=%d, dataAdd=%p\r\n", len, data));

  if (len < 1)
    return 0;
  if (len > 1514)		/* 6 + 6 + 2 + 1500 */
    return -1;			/* エラー */

  if (EdStart (len, data) !=0)
    {
      DEBUG_PRINTF (("SendPacket()にてEdStart()失敗\r\n"));
      return -1;
    }

  DEBUG_PRINTF (("SendPacket() exit\r\n"));

  return 0;
}


/************************************************
 *  Neptune-X 割り込み許可、不許可設定 関数     *
 ************************************************/
int
SetPacketReception (int i)
{

  SELECT_PAGE_0 (E8390_NODMA + E8390_START);

  if (i)
    {
      /* 割り込み許可 */
#if 0
    io_addr [EN0_IMR] =		   ENISR_TX		   + ENISR_TX_ERR + ENISR_OVER;
#else
    io_addr [EN0_IMR] = ENISR_RX + ENISR_TX + ENISR_RX_ERR + ENISR_TX_ERR + ENISR_OVER;
#endif
    }
  else
    io_addr [EN0_IMR] = 0x00;		/* 割り込み禁止 */

  return 0;
}


/* EOF */
