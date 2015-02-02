PROG = $(shell mkdir -p bin ; echo bin/sish)
#$(info $$PROG is [${PROG}])
IDIR = src
ODIR = $(shell mkdir -p build ; echo build)
#$(info $$ODIR is [${ODIR}])
SDIR = src
CFLAGS=	-Wall -g
_OBJS = myshell.o util.o

OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c  -o $@ $< $(CFLAGS) 

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(ODIR)/*.o $(OUT)

all: mkdirs ${PROG} 

mkdirs:
	mkdir -p build
	mkdir -p bin

