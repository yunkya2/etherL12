# Makefile for ether_ne.sys
#
# make だけで通常バイナリ作成
# make led でデバッグ用バイナリ（キーボードLED点灯版）作成

LED_TARGET = dbgLED.sys
TARGET = etherL12.sys
#TARGET = ether_ne.sys

OBJS	 = ne.o asmsub.o main.o ne2000.o
LED_OBJS = ne.o asmsub.o main.o ne2000led.o
LIBS	 = libc.a libgnu.a

CC = gcc
#CFLAGS = -g
#CFLAGS = -O -fomit-frame-pointer -fstrength-reduce
#CFLAGS = -DENABLE_EDINIT -DDEBUG_INUSE -DDEBUG_INT -DDEBUG_S
CFLAGS = -Wall -O -D__DOS_INLINE__ -D__IOCS_INLINE__

AS = has
#ASFLAGS = -sEXIT_LED_BIT=10
ASFLAGS =
LD = hlk
LDFLAGS = -l -x

all: $(TARGET)
led: $(LED_TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

$(LED_TARGET): $(LED_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

ne2000.o: ne2000.c ne2000.h memcpy.h debug.h
	$(COMPILE.c) $<

ne2000led.o: ne2000.c ne2000.h memcpy.h debug.h
	$(COMPILE.c) -o $@ $< -DDEBUG_LED

main.o: main.c main.h ne2000.h
	$(COMPILE.c) $<

asmsub.o: asmsub.s

# EOF
