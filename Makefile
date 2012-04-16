objects = ffwd.o ao/alsa.o ao/ao.o vo/vo.o vo/x11.o
libraries = -lasound -lavformat -lavcodec -lavutil -lswscale -lz -lbz2 -lX11 -pthread
CFLAGS = -g -O0 -Wall -Wextra

all : ffwd
analyze : 
	clang --analyze $(libraries) $(objects:%.o=%.c); rm *.plist
ffwd : $(objects)
	$(CC) $(CFLAGS) -o ffwd $(objects) $(libraries)
ao/ao.o : ao/ao.h ao/play.h
ffwd.o : vo/vo.h ao/ao.h
vo/vo.o : vo/vo.h vo/draw.h
clean : 
	rm ffwd $(objects)