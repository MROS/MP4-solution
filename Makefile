CC=g++
FLAGS=-std=c++11

all: inf-bonbon-server

inf-bonbon-server: main.o
	$(CC) $(FLAGS) -o inf-bonbon-server main.o

main.o: main.cpp json.hpp
	$(CC) $(FLAGS) -c main.cpp
