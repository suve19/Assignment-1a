all: client

clientmain.o: clientmain.cpp
	$(CXX) -Wall -c clientmain.cpp -I.

main.o: main.cpp
	$(CXX) -Wall -c main.cpp -I.


client: clientmain.o 
	$(CXX) -L./ -Wall -o client clientmain.o 


clean:
	rm *.o client
