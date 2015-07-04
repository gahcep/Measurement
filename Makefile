all: measure_demo

measure_demo: main.o
	g++ main.o -o measure_demo

main.o:	main.cpp measurement.hpp
	g++ -Wall -std=c++1y -c main.cpp


clean:
	rm -rf *.i *.s *.o
