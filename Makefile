caesar: caesar_main.cpp
	g++ -o caesar caesar_main.cpp encoder.cpp encoder.h decoder.cpp decoder.h -lncurses -pthread -std=c++11 -lstdc++
