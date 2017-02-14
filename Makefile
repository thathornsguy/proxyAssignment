all: clean awget ss

awget: awget.cpp
	g++ -std=c++11 -Wall -o awget awget.cpp

ss: ss.cpp
	g++ -pthread -std=c++11 -Wall -o ss ss.cpp

clean: 
	rm -rf awget ss
	
clear:
	clear
tar:
	tar cf Group14_P2.tar awget.cpp ss.cpp Makefile README

#to use: make memc u=<url> 
memawget:
	valgrind --leak-check=full --show-leak-kinds=all ./awget www.google.com
	
#to use: make memc p=<someport>
memss:
	valgrind --leak-check=full --show-leak-kinds=all ./ss


