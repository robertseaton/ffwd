objects = ffwd.o vo/x11.o
libraries = -lavformat -lavcodec -lavutil -lswscale -lz -lbz2 -lX11
CFLAGS = -g -O0 -Wall -Wextra

all : ffwd
analyze : 
	clang --analyze $(libraries) $(objects:%.o=%.c); rm *.plist
ffwd : $(objects)
	$(CC) $(CFLAGS) -o ffwd $(objects) $(libraries)
ffwd.o : vo/vo.h
clean : 
	rm ffwd $(objects)