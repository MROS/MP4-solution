CC=g++
FLAGS=-std=c++11

all: inf-bonbon-server

inf-bonbon-server: main.o client.o
	$(CC) $(FLAGS) -o inf-bonbon-server main.o client.o

main.o: main.cpp json.hpp client.hpp
	$(CC) $(FLAGS) -c main.cpp

client.o: client.cpp client.hpp
	$(CC) $(FLAGS) -c client.cpp

clean:
	rm *.o inf-bonbon-server
