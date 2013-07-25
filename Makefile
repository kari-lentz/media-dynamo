#define special flags
#
FLAGS_MYSQL := -DMYSQLPP_MYSQL_HEADERS_BURIED 

#
#define target, object, libraries
#
TARGET := ../bin/video-player
OBJECTS := video-player.o sdl-holder.o dom-context.o asset.o cairo-f.o video-decode-context.o audio-decode-context.o mp3-decode-context.o messages.o mpthree-iter.o music.o decoder-inst.o

CC := g++

LIBS := libavdevice libavformat libavfilter libavcodec libswresample libswscale libavutil sdl cairo pango pangocairo alsa libmpg123 openssl
LDLIBS := -lmysqlpp $(LDLIBS) 		
LDLIBS := $(shell pkg-config --libs $(LIBS)) $(LDLIBS)

CPPFLAGS := $(shell pkg-config --cflags $(LIBS)) -c $(FLAGS_MYSQL)

INC := app-fault.h ring-buffer.h ring-buffer-video.h ring-buffer-audio.h my-audio-frame.h unix-result.h result.h synch.h vwriter.h null-stream.h env-writer.h logger.h messages.h decoder-inst.h decoder-result.h stream-max-writer.h

#
# Link commands
#
$(TARGET) : $(OBJECTS) 
	$(CC) -pthread $(OBJECTS) $(LDLIBS) -o $(TARGET)

#
# Compile commands
#
video-player.o : video-player.cc video-player.h video-decode-context.h audio-decode-context.h mp3-decode-context.h decode-context.h alsa-engine.h alsa-result.h ffmpeg-headers.h $(INC)
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

mp3-decode-context.o : mp3-decode-context.cc mp3-decode-context.h $(INC)
	$(CC) $(CPPFLAGS) -c -o mp3-decode-context.o mp3-decode-context.cc

messages.o : messages.h
	$(CC) $(CPPFLAGS) -c -o messages.o messages.cc

mpthree-iter.o : mpthree-iter.cc alsa-engine.h
	$(CC) $(CPPFLAGS) -c -o mpthree-iter.o $(CC_SRC) mpthree-iter.cc

music.o: music.cc music.h result.h ring-buffer-audio.h ring-buffer.h my-audio-frame.h logger.h smart-ptr.h alsa-engine.h
	$(CC) $(CPPFLAGS) -c -o music.o music.cc

decoder-inst.o : decoder-inst.h music.h result.h ring-buffer-audio.h ring-buffer.h my-audio-frame.h logger.h smart-ptr.h alsa-engine.h
	$(CC) $(CPPFLAGS) -c -o decoder-inst.o decoder-inst.cc

clean: 
	rm -rf $(TARGET) $(OBJECTS) 
