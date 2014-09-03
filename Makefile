

CC = g++
CXXFLAGS = -std=c++11  -I /usr/local/include/boost-1_54
LDFLAGS = -L /usr/local/lib/boost1_54_0 -lpthread


all: target

# debug, release flags
# to make with debug options, run make utility so: make DEBUG=yes
ifeq ($(DEBUG), yes)
CXXFLAGS += -g
LDFLAGS += -lboost_regex-gcc47-mt-sd-1_54
else
CXXFLAGS += -DNDEBUG
LDFLAGS += -lboost_regex-gcc47-mt-s-1_54
endif


target: serv.o daemonize.o
	$(CC) $(LDFLAGS) serv.o daemonize.o -o network_server
	
serv.o: serv.cpp serv.hpp
daemonize.o: ../mon_daemon/daemonize.cpp ../mon_daemon/daemonize.hpp
	$(CC) $(CXXFLAGS) -c ../mon_daemon/daemonize.cpp


clean:
	rm -f *.o









