objects = ffwd.o 
libraries = -lavformat -lavcodec -lavutil -lswscale -lz -lbz2 -lX11
CFLAGS = -g -O0 -Wall -Wextra

all : ffwd
analyze : 
	clang --analyze $(libraries) $(objects:%.o=%.c); rm *.plist
ffwd : $(objects)
	$(CC) $(CFLAGS) -o ffwd $(objects) $(libraries)
clean : 
	rm ffwd $(objects)