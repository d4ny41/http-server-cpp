test: src/main.o
	g++ -std=c++17 -Wall -Wextra src/main.o -o test

src/main.o: src/main.cpp
	g++ -std=c++17 -Wall -Wextra -c src/main.cpp -o src/main.o

clean: 
	rm src/*.o test