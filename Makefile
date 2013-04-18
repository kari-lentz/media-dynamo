#define special flags
#
FLAGS_MYSQL := -DMYSQLPP_MYSQL_HEADERS_BURIED 

#
#define target, object, libraries
#
TARGET := ../bin/video-player
OBJECTS := video-player.o sdl-holder.o video-decode-context.o audio-decode-context.o render-video.o  

CC := g++

LIBS := libavdevice libavformat libavfilter libavcodec libswresample libswscale libavutil sdl		

LDLIBS := $(shell pkg-config --libs $(LIBS)) $(LDLIBS)

CPPFLAGS =  -DLINUX=2 -D_REENTRANT -D_GNU_SOURCE -D_LARGEFILE64_SOURCE -pthread -Wall -Werror -g -Iinclude

INC := app-fault.h ring-buffer.h ring-buffer-video.h ring-buffer-audio.h vwriter.h null-stream.h env-writer.h

#
# Link commands
#
$(TARGET) : $(OBJECTS) 
	$(CC) -pthread $(OBJECTS) $(LDLIBS) -o $(TARGET)

#
# Compile commands
#
video-player.o : video-player.cc video-player.h video-decode-context.h decode-context.h ffmpeg-headers.h $(INC)
	$(CC) $(CPPFLAGS) -c -o video-player.o $(FLAGS_MYSQL) video-player.cc

sdl-holder.o : sdl-holder.cc sdl-holder.h
	$(CC) $(CPPFLAGS) -c -o sdl-holder.o $(FLAGS_MYSQL) sdl-holder.cc

video-decode-context.o : video-decode-context.cc video-decode-context.h decode-context.h $(INC)
	$(CC) $(CPPFLAGS) -c -o video-decode-context.o video-decode-context.cc

audio-decode-context.o : audio-decode-context.cc audio-decode-context.h decode-context.h $(INC)
	$(CC) $(CPPFLAGS) -c -o audio-decode-context.o audio-decode-context.cc

render-video.o : render-video.cc render-video.h render.h $(INC)
	$(CC) $(CPPFLAGS) -c -o render-video.o render-video.cc

clean: 
	rm -rf $(TARGET) $(OBJECTS) 
