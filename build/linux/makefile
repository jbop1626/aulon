PROG     = aulon

OUTDIR   = ../../bin/linux/
OBJDIR   = $(OUTDIR)obj/
SRCDIR   = ../../src/
UDEVRULE = 99-$(PROG).rules

CC       = gcc
CFLAGS   = -O3 -std=c99 -Wall -Wextra -Wpedantic
OBJ      = $(OBJDIR)main.o $(OBJDIR)menu.o $(OBJDIR)menu_func.o      \
           $(OBJDIR)fs.o $(OBJDIR)io.o $(OBJDIR)commands.o           \
           $(OBJDIR)player_comms.o $(OBJDIR)usb.o $(OBJDIR)usb_log.o
LDFLAGS  =
LDLIBS   = -lusb-1.0


$(PROG): $(OBJ)
	$(CC) -o $(OUTDIR)$@ $^ $(CFLAGS) $(LDFLAGS) $(LDLIBS)

$(OBJDIR)%.o: $(SRCDIR)%.c
	@mkdir -p $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJDIR)main.o:         $(SRCDIR)menu.h $(SRCDIR)io.h $(SRCDIR)usb_log.h $(SRCDIR)defs.h
$(OBJDIR)menu.o:         $(SRCDIR)menu.h $(SRCDIR)menu_func.h $(SRCDIR)io.h $(SRCDIR)defs.h
$(OBJDIR)menu_func.o:    $(SRCDIR)menu_func.h $(SRCDIR)fs.h $(SRCDIR)io.h $(SRCDIR)commands.h $(SRCDIR)player_comms.h $(SRCDIR)usb.h
$(OBJDIR)fs.o:           $(SRCDIR)fs.h $(SRCDIR)io.h $(SRCDIR)commands.h
$(OBJDIR)io.o:           $(SRCDIR)io.h
$(OBJDIR)commands.o:     $(SRCDIR)io.h $(SRCDIR)commands.h $(SRCDIR)player_comms.h
$(OBJDIR)player_comms.o: $(SRCDIR)io.h $(SRCDIR)player_comms.h $(SRCDIR)usb.h
$(OBJDIR)usb.o:          $(SRCDIR)usb_log.h $(SRCDIR)usb.h $(SRCDIR)defs.h
$(OBJDIR)usb_log.o:      $(SRCDIR)io.h $(SRCDIR)usb_log.h

.PHONY: clean
clean:
	rm -f $(OUTDIR)$(PROG) $(OBJDIR)*.o 

.PHONY: install
install:
	install $(OUTDIR)$(PROG) /usr/local/bin/
	install ./$(UDEVRULE) /etc/udev/rules.d/
	udevadm control --reload-rules

.PHONY: uninstall
uninstall:
	rm -f /usr/local/bin/$(PROG)
	rm -f /etc/udev/rules.d/$(UDEVRULE)

