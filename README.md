#Restarter

g++ -Wall -fexceptions -g  -c restarter.cpp -o restarter.o
g++  -o restarter restarter.o  -static  -lpthread
