CONTIKI=../contiki

# -----------------------------------------------------------------------------

NAME = contiki
DEFINES += FONT_W=6 FONT_H=8


GAME_C_FILES = contiki-main.c ctk/ctk-arch.c ctk/conio.c lib/cfs-fatfs.c \
	$(CONTIKI)/apps/about-dsc.c $(CONTIKI)/apps/about.c \
	$(CONTIKI)/apps/calc-dsc.c $(CONTIKI)/apps/calc.c \
	$(CONTIKI)/apps/editor-dsc.c $(CONTIKI)/apps/editor.c \
	$(CONTIKI)/apps/processes-dsc.c $(CONTIKI)/apps/processes.c \
	$(CONTIKI)/apps/shell-dsc.c $(CONTIKI)/apps/shell-gui.c $(CONTIKI)/apps/shell.c \
	$(CONTIKI)/apps/program-handler.c $(CONTIKI)/ctk/ctk.c \
	$(CONTIKI)/ctk/ctk-conio.c \
	$(CONTIKI)/ek/arg.c $(CONTIKI)/ek/ek.c $(CONTIKI)/ek/ek-service.c \
	$(CONTIKI)/lib/cfs.c $(CONTIKI)/lib/ctk-filedialog.c \
	$(CONTIKI)/lib/ctk-textentry-cmdline.c $(CONTIKI)/lib/memb.c \
	$(CONTIKI)/lib/petsciiconv.c $(CONTIKI)/lib/timer.c \
	lib/textmode/textmode.c \
	lib/events/events.c \

GAME_C_OPTS = -I ctk -I lib -I conf -I $(CONTIKI)/apps -I $(CONTIKI)/ctk \
    -I $(CONTIKI)/ek -I $(CONTIKI)/lib -I $(CONTIKI)/uip \
#	-D__GP32__ -DCTK_CONIO -DWITH_UIP \
	-DWITH_WWW -D WITH_TELNET_WWW \
	-DWITH_TELNET -D WITH_TELNET_ICON \
	-DWITH_EMAIL -D WITH_EMAIL_ICON \
	-DWITH_CALC -D WITH_CALC_ICON

BITBOX ?= ../../../../bitbox-dist/sdk

include $(BITBOX)/kernel/bitbox.mk

# -----------------------------------------------------------------------------
