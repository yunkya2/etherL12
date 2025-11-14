//	asmsub.s

	.xdef	DI, EI											// ne2000.c nagai.

	.text
	.even



DI:
	move.w	%sr,sendp_sr
	ori.w	#0x700,%sr
	rts
EI:
	move.w	sendp_sr,%sr
	rts

sendp_sr:
	.space	2


	.even
