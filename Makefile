LIBS = -lc
LIB = libbogl.a
DEVLINK = libbogl.so
SONAME = libbogl.so.0
SHARED_LIB = libbogl.so.0.1

CFLAGS = -O2 -g -D_GNU_SOURCE
WARNCFLAGS += -Wall -D_GNU_SOURCE
ALLCFLAGS = $(CFLAGS) $(WARNCFLAGS) $(FBCFLAGS)

architecture := $(shell dpkg-architecture -qDEB_BUILD_ARCH_CPU)
os := $(shell uname)

LIBOBJECTS = $(LIBBOGLOBJECTS) $(LIBBOMLOBJECTS) $(LIBBOWLOBJECTS)	\
	$(LIBRSRCOBJECTS)
LIBBOGLOBJECTS = bogl.o bogl-font.o
LIBBOMLOBJECTS = arrow.o boml.o bogl-debug.o
LIBBOWLOBJECTS = bowl.o symbol.o
LIBRSRCOBJECTS = helvB10.o helvB12.o helvR10.o timBI18.o tux75.o

SOURCES_DEP = arrow.c bdftobogl.c bogl-cfb.c bogl-cfb.h bogl-cfb8.c	\
bogl-cfb8.h bogl-font.c bogl-font.h \
bogl-test.c bogl.h boglP.h boml.c	\
boml.h bowl.c bowl.h pngtobogl.c bogl-debug.c

ifeq ($(os),Linux)
	SOURCES_DEP += bogl-pcfb.c bogl-pcfb.h bogl-tcfb.c bogl-tcfb.h bowl-boxes.c bogl.c

ifeq (,)
	FBCFLAGS += -DBOGL_CFB_FB=1
	LIBBOGLOBJECTS += bogl-cfb.o bogl-pcfb.o bogl-tcfb.o
endif

ifneq (,$(filter i386 arm ia64 amd64,$(architecture)))
	FBCFLAGS += -DBOGL_VGA16_FB=1
	LIBBOGLOBJECTS += bogl-vga16.o
	SOURCES_DEP += bogl-vga16.c bogl-vga16.h
endif
endif

OBJECTS = $(LIBOBJECTS) bowl-boxes.o

GENERATED = helvB10.c helvB12.c helvR10.c timBI18.c tux75.c

#		 libutf8/libutf8_plug.so unifont-reduced.bgf
all:    depend
ifeq ($(os),Linux)
all: $(SHARED_LIB) $(LIB) bterm bdftobogl pngtobogl
endif
all: reduce-font

%.lo: %.c
	$(CC) $(ALLCFLAGS) -o $@ -fPIC -c $<

%.o: %.c
	$(CC) $(ALLCFLAGS) -o $@ -c $<

$(SHARED_LIB): $(OBJECTS:%.o=%.lo)
	$(CC) -shared -Wl,-soname,$(SONAME) -o $(SHARED_LIB) $(OBJECTS:%.o=%.lo)

# unifont-reduced.bgf: bdftobogl unifont-reduced.bdf
#	./bdftobogl -b unifont-reduced.bdf > unifont-reduced.bgf

# unifont-reduced.bdf: ../bf-utf/unifont.bdf ../all.utf libutf8/libutf8_plug.so reduce-font
#	LD_PRELOAD=./libutf8/libutf8_plug.so LC_CTYPE=C.UTF-8 ./reduce-font $< < ../all.utf > $@

# ../all.utf: ../po.utf ../rn.utf $(xml_files)
#	cat $^ > $@

# ../po.utf:
#	$(MAKE) -C ../dbootstrap/po all-po-utf
#	cat ../dbootstrap/po/utf/*.po > $@

# ../rn.utf:
#	$(MAKE) RN_TARGET_DIR=. -C ../../scripts/rootdisk/messages install-utf
#	cat ../../scripts/rootdisk/messages/release_notes.* > $@

reduce-font: reduce-font.c

$(LIB): $(OBJECTS)
	rm -f $(LIB)
	ar rcs $(LIB) $(OBJECTS)

bogl-test: $(LIB) bogl-test.c tux75.o
	$(CC) $(ALLCFLAGS) -o bogl-test bogl-test.c tux75.o $(LIB)

bowl-boxes: $(LIB)
	$(CC) -DSTANDALONE_TEST $(ALLCFLAGS) bowl-boxes.c $(LIBOBJECTS) -o bowl-boxes

bterm: $(LIB) bterm.o bogl-term.o bogl-bgf.o
	$(CC) $+ $(LIB) -o bterm

bdftobogl: $(LIBBOGLOBJECTS)
%.c: %.bdf bdftobogl
	./bdftobogl $< > $@

pngtobogl: pngtobogl.o
	$(CC) $(ALLCFLAGS) -o pngtobogl pngtobogl.o -lgd
%.c: %.png pngtobogl
	./pngtobogl $< > $@

depend: .depend
.depend: $(SOURCES_DEP)
	$(CPP) $(ALLCFLAGS) -M $(SOURCES_DEP) > .depend

clean: 
	rm -rf reduce-font bterm bdftobogl pngtobogl *.o $(GENERATED) *-test lang.h tmp.*.c bowl-boxes $(LIB) unifont-reduced.bgf unifont-reduced.bdf
	rm -f $(OBJECTS:%.o=%.lo) $(SHARED_LIB)
	rm -f .depend

distclean: clean
	rm -f $(LIB) .depend *~ .nfs*

force:

ifeq (.depend,$(wildcard .depend))
include .depend
endif

install: all
	install -d $(DESTDIR)/usr/bin
ifeq ($(os),Linux)
	install -d $(DESTDIR)/usr/lib $(DESTDIR)/usr/include/bogl
	install -m644 $(SHARED_LIB) $(DESTDIR)/usr/lib/$(SHARED_LIB)
	ln -s $(SHARED_LIB) $(DESTDIR)/usr/lib/$(DEVLINK)
	ln -s $(SHARED_LIB) $(DESTDIR)/usr/lib/$(SONAME)
	install -m644 $(LIB) $(DESTDIR)/usr/lib/$(LIB)
	install -m644 *.h $(DESTDIR)/usr/include/bogl
	install -m755 bdftobogl bterm pngtobogl $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/usr/share/terminfo
	tic -o$(DESTDIR)/usr/share/terminfo bterm.ti
endif
	install -m755 mergebdf reduce-font $(DESTDIR)/usr/bin
