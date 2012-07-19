TARGETS = npexample.so

all : $(TARGETS)

npexample.so : npexample.c
	$(CC) $(CFLAGS) -DXP_UNIX=1 -DMOZ_X11=1 \
		npexample.c -o npexample.so -shared -fPIC

clean :

distclean : clean
	-rm $(TARGETS)

.PHONY : all clean distclean
