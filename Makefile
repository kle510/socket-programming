all:
	g++ -o client client.cpp 
	g++ -o edge.out edge.cpp 
	g++ -o server_and.out server_and.cpp 
	g++ -o server_or.out server_or.cpp 
	
.PHONY: edge server_or server_and

server_and:
	./server_and.out
server_or:
	./server_or.out
edge:
	./edge.out


