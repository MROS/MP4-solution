CC=g++
FLAGS=-std=c++11

all: inf-bonbon-server

inf-bonbon-server: main.o client.o match_queue.o mq.o load_library_util.o
	$(CC) $(FLAGS) -o inf-bonbon-server main.o mq.o client.o match_queue.o load_library_util.o -lrt -ldl

main.o: main.cpp json.hpp client.hpp mq.hpp config.hpp match_queue.hpp
	$(CC) $(FLAGS) -c main.cpp

client.o: client.cpp client.hpp json.hpp load_library_util.hpp
	$(CC) $(FLAGS) -c client.cpp

match_queue.o: match_queue.cpp match_queue.hpp client.hpp mq.hpp config.hpp linked_list.hpp load_library_util.hpp
	$(CC) $(FLAGS) -c match_queue.cpp
	
mq.o: mq.hpp mq.cpp
	$(CC) $(FLAGS) -c mq.cpp

load_library_util.o: load_library_util.hpp load_library_util.cpp
	$(CC) $(FLAGS) -c load_library_util.cpp

clean:
	rm *.o inf-bonbon-server
