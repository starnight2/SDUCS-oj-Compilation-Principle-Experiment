#! /bin/bash
g++ -c 1.cpp -o 1.o
g++ -c 2.cpp -o 2.o
g++ -c main.cpp -o main.o


g++ 1.o 2.o main.o -o Main
