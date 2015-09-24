cc=clang++

echo: main.o Epoll.o Implement.o
	$(cc) -o echo main.o Epoll.o Implement.o -std=c++11

main.o: main.cpp Implement.h Epoll.h
	$(cc) -c main.cpp -std=c++11

Implement.o: Implement.cpp Implement.h
	$(cc) -c Implement.cpp -std=c++11

Epoll.o: Epoll.cpp Epoll.h
	$(cc) -c Epoll.cpp -std=c++11
