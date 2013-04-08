#define special flags
#
FLAGS_MYSQL := -DMYSQLPP_MYSQL_HEADERS_BURIED 

#
#define target, object, libraries
#
TARGET := ../video_player
OBJECTS := video-player.o decode-context.o file-context.o display.o  

CC := g++

LIBS := -L../lib -lavdevice -lavfilter -lpostproc -lavformat -lavcodec -ldl -lXfixes -lXext -lX11 -lasound -lSDL -lx264 -lvpx -lvorbisenc -lvorbis -ltheoraenc -ltheoradec -logg -lopencore-amrwb -lopencore-amrnb -lmp3lame -lfaac -lz -lrt -lswresample -lswscale -lavutil -lm -lmysqlpp -pthread 

CPPFLAGS =  -DLINUX=2 -D_REENTRANT -D_GNU_SOURCE -D_LARGEFILE64_SOURCE -pthread -Wall -fPIC -Werror -g -Iinclude

INC := vwriter.h app-fault.h ring-buffer.h ring-buffer-uint8.h null-stream.h env-writer.h

#
# Link commands
#
$(TARGET) : $(OBJECTS) 
	$(CC) -pthread -shared -fPIC $(OBJECTS) $(LIBS) -o $(TARGET)

#
# Compile commands
#
video-player.o : video-player.cc video-player.h file-context.h decode-context.h $(INC)
	$(CC) $(CPPFLAGS) -c -o video-player.o $(FLAGS_MYSQL) video-player.cc

decode-context.o : decode-context.cc decode-context.h decode-context.h $(INC)
	$(CC) $(CPPFLAGS) -c -o decode-context.o decode-context.cc

file-context.o : file-context.cc file-context.h decode-context.h $(INC)
	$(CC) $(CPPFLAGS) -c -o file-context.o file-context.cc

display.o : display.cc display.h $(INC)
	$(CC) $(CPPFLAGS) -c -o display.o display.cc

clean: 
	rm -rf $(TARGET) $(OBJECTS) 
