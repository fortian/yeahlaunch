TARGET = yeahlaunch
CC = gcc
#CC = cc
INSTALL = install

PREFIX = /usr/local

LIBS = -lX11 
INCLUDES = -I/usr/X11R6/include 
# On modern distributions, this seems to be unneeded.  Uncomment and fix as
# needed if the build breaks.
# LIB_DIRS = -L/usr/X11R6/lib
FLAGS = -Os -Wall -Werror -ggdb

OBJECTS := yeahlaunch.o
SOURCES := yeahlaunch.c

$(TARGET): $(OBJECTS) 
	$(CC) $(DEFINES) $(INCLUDES) $(LIB_DIRS) $(LIBS) -o $@ $<
	strip $@

$(OBJECTS): $(SOURCES) 
	$(CC) $(FLAGS)  $(DEFINES) $(INCLUDES) $(LIB_DIRS) -c -o $@ $<

clean:
	rm -rf $(TARGET) $(OBJECTS)

install: $(TARGET) $(MAN)
	$(INSTALL) -o root -g root -m 0755 $(TARGET) $(PREFIX)/bin
	

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)
