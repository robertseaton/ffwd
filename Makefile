objects = ffwd.o ao/alsa.o vo/x11.o
libraries = -lasound -lavformat -lavcodec -lavutil -lswscale -lz -lbz2 -lX11 -pthread
CFLAGS = -g -O0 -Wall -Wextra

all : ffwd
analyze : 
	clang --analyze $(libraries) $(objects:%.o=%.c); rm *.plist
ffwd : $(objects)
	$(CC) $(CFLAGS) -o ffwd $(objects) $(libraries)
ffwd.o : vo/vo.h ao/ao.h
clean : 
	rm ffwd $(objects)