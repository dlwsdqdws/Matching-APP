all:
	g++ -std=c++11 -o clientA clientA.cpp
	g++ -std=c++11 -o clientB clientB.cpp
	g++ -std=c++11 -o serverC central.cpp
	g++ -std=c++11 -o serverT serverT.cpp
	g++ -std=c++11 -o serverS serverS.cpp
	g++ -std=c++11 -o serverP serverP.cpp