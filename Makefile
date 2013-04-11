#define special flags
#
FLAGS_MYSQL := -DMYSQLPP_MYSQL_HEADERS_BURIED 

#
#define target, object, libraries
#
TARGET := ../bin/video-player
OBJECTS := video-player.o decode-context.o video-file-context.o display.o  

CC := g++

LIBS := libavdevice libavformat libavfilter libavcodec libswresample libswscale libavutil sdl		

LDLIBS := $(shell pkg-config --libs $(LIBS)) $(LDLIBS)

CPPFLAGS =  -DLINUX=2 -D_REENTRANT -D_GNU_SOURCE -D_LARGEFILE64_SOURCE -pthread -Wall -Werror -g -Iinclude

INC := app-fault.h ring-buffer.h ring-buffer-video.h null-stream.h env-writer.h

#
# Link commands
#
$(TARGET) : $(OBJECTS) 
	$(CC) -pthread $(OBJECTS) $(LDLIBS) -o $(TARGET)

#
# Compile commands
#
video-player.o : video-player.cc video-player.h video-file-context.h decode-context.h ffmpeg-headers.h $(INC)
	$(CC) $(CPPFLAGS) -c -o video-player.o $(FLAGS_MYSQL) video-player.cc

decode-context.o : decode-context.cc decode-context.h decode-context.h $(INC)
	$(CC) $(CPPFLAGS) -c -o decode-context.o decode-context.cc

video-file-context.o : video-file-context.cc video-file-context.h decode-context.h $(INC)
	$(CC) $(CPPFLAGS) -c -o video-file-context.o video-file-context.cc

display.o : display.cc display.h $(INC)
	$(CC) $(CPPFLAGS) -c -o display.o display.cc

clean: 
	rm -rf $(TARGET) $(OBJECTS) 
