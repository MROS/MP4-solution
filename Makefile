CC=g++
FLAGS=-std=c++11

all: inf-bonbon-server

inf-bonbon-server: main.o client.o match_queue.o
	$(CC) $(FLAGS) -o inf-bonbon-server main.o client.o match_queue.o -lrt

main.o: main.cpp json.hpp client.hpp mq.hpp config.hpp match_queue.hpp
	$(CC) $(FLAGS) -c main.cpp

client.o: client.cpp client.hpp json.hpp
	$(CC) $(FLAGS) -c client.cpp

match_queue.o: match_queue.cpp match_queue.hpp client.hpp mq.hpp config.hpp
	$(CC) $(FLAGS) -c match_queue.cpp
	
linked_list.o: linked_list.cpp linked_list.hpp
	$(CC) $(FLAGS) -c linked_list.cpp

clean:
	rm *.o inf-bonbon-server
