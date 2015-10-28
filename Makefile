OBJS = search_recorder.o message.pb.o
CC = clang++
DEBUG = -g
CFLAGS = -std=c++0x -W -Wall -c $(DEBUG)
LFLAGS = -lzmq `pkg-config --cflags --libs protobuf`

search_recorder: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o search_recorder
search_recorder.o: search_recorder.cpp
	$(CC) $(CFLAGS) search_recorder.cpp -o search_recorder.o
message.pb.o: message.pb.cpp message.pb.hh
	g++ -c message.pb.cpp -o message.pb.o
clean:
	rm -f *.o search_recorder