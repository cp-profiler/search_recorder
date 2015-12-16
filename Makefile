OBJS = search_recorder.o message.pb.o
CC = g++
DEBUG = -g
CFLAGS = -std=c++0x -W -Wall -c $(DEBUG)
LFLAGS = `pkg-config --cflags --libs protobuf` -lboost_system

search_recorder: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o search_recorder
search_recorder.o: search_recorder.cpp
	$(CC) $(CFLAGS) search_recorder.cpp -o search_recorder.o
message.pb.o: message.pb.cpp message.pb.hh
	$(CC) $(CFLAGS) message.pb.cpp -o message.pb.o
clean:
	rm -f *.o search_recorder
