#define special flags
#
FLAGS_MYSQL := -DMYSQLPP_MYSQL_HEADERS_BURIED 

#
#define target, object, libraries
#
TARGET := ../bin/video-player
OBJECTS := video-player.o sdl-holder.o dom-context.o asset.o cairo-f.o video-decode-context.o audio-decode-context.o audio-silence-context.o messages.o

CC := g++

LIBS := libavdevice libavformat libavfilter libavcodec libswresample libswscale libavutil sdl cairo pango pangocairo		
LDLIBS := $(shell pkg-config --libs $(LIBS)) $(LDLIBS)

CPPFLAGS := $(shell pkg-config --cflags $(LIBS)) 

INC := app-fault.h ring-buffer.h ring-buffer-video.h ring-buffer-audio.h my-audio-frame.h unix-result.h result.h synch.h vwriter.h null-stream.h env-writer.h logger.h messages.h

#
# Link commands
#
$(TARGET) : $(OBJECTS) 
	$(CC) -pthread $(OBJECTS) $(LDLIBS) -o $(TARGET)

#
# Compile commands
#
video-player.o : video-player.cc video-player.h video-decode-context.h audio-decode-context.h audio-silence-context.h decode-context.h alsa-engine.h alsa-result.h ffmpeg-headers.h $(INC)
	$(CC) $(CPPFLAGS) -c -o video-player.o $(FLAGS_MYSQL) video-player.cc

sdl-holder.o : sdl-holder.cc sdl-holder.h
	$(CC) $(CPPFLAGS) -c -o sdl-holder.o $(FLAGS_MYSQL) sdl-holder.cc

dom-context.o : dom-context.cc dom-context.h $(INC)
	$(CC) $(CPPFLAGS) -c -o dom-context.o dom-context.cc

asset.o : asset.cc asset.h
	$(CC) $(CPPFLAGS) -c -o asset.o asset.cc	

cairo-f.o : cairo-f.cc cairo-f.h dom-context.h $(INC)
	$(CC) $(CPPFLAGS) -c -o cairo-f.o cairo-f.cc

video-decode-context.o : video-decode-context.cc video-decode-context.h decode-context.h $(INC)
	$(CC) $(CPPFLAGS) -c -o video-decode-context.o video-decode-context.cc

audio-decode-context.o : audio-decode-context.cc audio-decode-context.h decode-context.h $(INC)
	$(CC) $(CPPFLAGS) -c -o audio-decode-context.o audio-decode-context.cc

audio-silence-context.o : audio-silence-context.cc audio-silence-context.h $(INC)
	$(CC) $(CPPFLAGS) -c -o audio-silence-context.o audio-silence-context.cc

messages.o : messages.h
	$(CC) $(CPPFLAGS) -c -o messages.o messages.cc

clean: 
	rm -rf $(TARGET) $(OBJECTS) 
