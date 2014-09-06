

CC = g++
ifeq ($(HOME), yes)
CXXFLAGS = -std=c++11  -I /usr/local/include/boost
LDFLAGS = -L /usr/local/lib/boost/static -lpthread
else
CXXFLAGS = -std=c++11  -I /usr/local/include/boost
LDFLAGS = -L /usr/local/lib/boost/static -lpthread
endif


all: target

# debug, release flags
# to make with debug options, run make utility so: make DEBUG=yes
ifeq ($(DEBUG), yes)
CXXFLAGS += -g
LDFLAGS += -lboost_regex-gcc47-mt-d-1_56
else
CXXFLAGS += -DNDEBUG
LDFLAGS += -lboost_regex-gcc47-mt-1_56
endif


target: serv.o daemonize.o
	$(CC) serv.o daemonize.o -o network_server $(LDFLAGS)
	
serv.o: serv.cpp serv.hpp
daemonize.o: ../mon_daemon/daemonize.cpp ../mon_daemon/daemonize.hpp
	$(CC) $(CXXFLAGS) -c ../mon_daemon/daemonize.cpp


clean:
	rm -f *.o









