PROG = $(shell mkdir -p bin ; echo bin/sws)
#$(info $$PROG is [${PROG}])
IDIR = src
ODIR = $(shell mkdir -p build ; echo build)
#$(info $$ODIR is [${ODIR}])
SDIR = src
LIBS = -lmagic
CFLAGS=	-Wall -g
_OBJS = main.o net.o util.o magic_utils.o http.o cgi.o

OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c  -o $@ $< $(CFLAGS) 

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f $(ODIR)/*.o $(OUT)

all: mkdirs ${PROG} 

mkdirs:
	mkdir -p build
	mkdir -p bin
