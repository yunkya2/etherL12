/**********************************************************************
* Network and Ethernet Project for TUNE-up X680x0   <Neptune-X>       *
*                                                                     *
*     Neptune-X board driver for  Human-68k(ESP-X)   version 0.03     *
*                                                                     *
*         Programed 1996-7 by Shi-MAD.                                *
*                   Special thanks to  Niggle, FIRST, yamapu ...      *
**********************************************************************/
/*
 * ne2000.h 
 */

#ifndef NE2000_H
#define NE2000_H


#define	E8390_TX_IRQ_MASK	0x0a
#define	E8390_RX_IRQ_MASK	0x05
#define	E8390_RXCONFIG		0x04
#define	E8390_RXOFF		0x20
#define	E8390_TXCONFIG		0x00
#define	E8390_TXOFF		0x02


#define	E8390_STOP		0x01
#define	E8390_START		0x02
#define	E8390_TRANS		0x04
#define	E8390_RREAD		0x08
#define	E8390_RWRITE		0x10
#define	E8390_NODMA		0x20
#define	E8390_PAGE0		0x00
#define	E8390_PAGE1		0x40
#define	E8390_PAGE2		0x80

#define	E8390_CMD		0x00


/*** page 0 ***/
#define	EN0_CLDALO		0x02	/* 01 */
#define	EN0_STARTPG		0x02	
#define	EN0_CLDAHI		0x04	/* 02 */
#define	EN0_STOPPG		0x04
#define	EN0_BOUNDARY		0x06	/* 03 */
#define	EN0_TSR			0x08	/* 04 */
#define	EN0_TPSR		0x08
#define	EN0_NCR			0x0a	/* 05 */
#define	EN0_TCNTLO		0x0a
#define	EN0_FIFO		0x0c	/* 06 */
#define	EN0_TCNTHI		0x0c
#define	EN0_ISR			0x0e	/* 07 */
#define	EN0_CRDALO		0x10	/* 08 */
#define	EN0_RSARLO		0x10
#define	EN0_CRDAHI		0x12	/* 09 */
#define	EN0_RSARHI		0x12
#define	EN0_RCNTLO		0x14	/* 0a */
#define	EN0_RCNTHI		0x16	/* 0b */
#define	EN0_RSR			0x18	/* 0c */
#define	EN0_RXCR		0x18
#define	EN0_TXCR		0x1a	/* 0d */
#define	EN0_COUNTER0		0x1a
#define	EN0_DCFG		0x1c	/* 0e */
#define	EN0_COUNTER1		0x1c
#define	EN0_IMR			0x1e	/* 0f */
#define	EN0_COUNTER2		0x1e


#define	ENISR_RX		0x01
#define	ENISR_TX		0x02
#define	ENISR_RX_ERR		0x04
#define	ENISR_TX_ERR		0x08
#define	ENISR_OVER		0x10
#define	ENISR_COUNTERS		0x20
#define	ENISR_RDC		0x40
#define	ENISR_RESET		0x80
#define	ENISR_ALL		0x3f

/*** page 1 ***/
#define	EN1_PHYS		0x02
#define	EN1_CURPAG		0x0e
#define	EN1_MULT		0x10


#define	ENRSR_RXOK		0x01
#define	ENRSR_CRC		0x02
#define	ENRSR_FAE		0x04
#define	ENRSR_FO		0x08
#define	ENRSR_MPA		0x10
#define	ENRSR_PHY		0x20
#define	ENRSR_DIS		0x40
#define	ENRSR_DEF		0x80

#define	ENTSR_PTX		0x01
#define	ENTSR_ND		0x02
#define	ENTSR_COL		0x04
#define	ENTSR_ABT		0x08
#define	ENTSR_CRS		0x10
#define	ENTSR_FU		0x20
#define	ENRSR_CDH		0x40
#define	ENRSR_OWC		0x80


#define NE1SM_START_PAGE	0x20	/* first page of TX (NE-1000) */
#define NE1SM_STOP_PAGE		0x40	/* Lasrt page +1 of RX ring */
#define NE2SM_START_PAGE	0x40	/* (NE-2000) */
#define NE2SM_STOP_PAGE		0x80


/*
 * Structure of a 10Mb/s Ethernet header.
 */
struct	ether_header {
  unsigned char ether_dhost [6];
  unsigned char ether_shost [6];
  unsigned short int ether_type;
};

struct ether_packet {
  unsigned int header;
  struct eaddr dst;
  struct eaddr src;
  unsigned short type;
  unsigned char data [1500];
  unsigned int framecheck;
};

struct ed_ring {
  unsigned char rsr;			/* receiver status */
  unsigned char next_packet;		/* pointer to next packet */
  unsigned char count_l;		/* bytes in packet (length + 4) */
  unsigned char count_h;
};


#if 0
struct statistics {
  unsigned int transmits;
  unsigned int transmit_requests;
  unsigned int multicast_transmits;
  unsigned int broadcast_transmits;
  unsigned int individual_transmits;
  unsigned int transmit_errors;
  unsigned int collisions;
  unsigned int transmit_toolong;

  unsigned int receives;
  unsigned int receive_interrupts;
  unsigned int multicast_receives;
  unsigned int broadcast_receives;
  unsigned int individual_receives;
  unsigned int receive_errors;
  unsigned int frame_errors;
  unsigned int crc_errors;
};
#endif


/*
 *  情報の詰め合わせ（各状態保存構造体）
 */
struct ed_softc {
  unsigned char xmit_busy;		/* 送信中フラグ */
  unsigned char txb_cnt;		/* 送信バッファ数 */
  unsigned char txb_inuse;		/* 現在使用中の送信バッファナンバー */

  unsigned char txb_new;		/* pointer to where new buffer will be added */
  unsigned char txb_next_tx;		/* pointer to next buffer ready to xmit */
  unsigned short int txb_len [8];	/* buffered xmit buffer lengths */

  unsigned char next_packet;		/* pointer to next unread RX packet */
  volatile unsigned char semaphore;	/*  */
};


/*
 * 送受信カウンタ
 */
struct trans_counter {
  unsigned int send_byte;
  unsigned int recv_byte;
};
extern struct trans_counter trans_counter;


extern int SearchNeptuneX (volatile unsigned char** io_addr, unsigned int* irq);
extern int InitNeptune (void);
extern int TrapNeptuneVector (unsigned int irq, void* my_handler);
extern int GetMacAddr (struct eaddr* buf);
extern int SetMacAddr (const struct eaddr* data);
extern int SetPacketReception (int i);
extern int SendPacket (int len, const unsigned char* data);


#endif /* !NE2000_H */

/* EOF */
