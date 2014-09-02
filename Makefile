

CC = gcc
COMMON_FLAGS = -lstdc++ -std=c++11
LIBS = -lpthread
MACROSES = 
CFLAGS = 
HEADERS_PATH = /usr/local/include/boost-1_54
LIBRARIES_PATH = /usr/local/lib/boost1_54_0


all: target

# debug, release flags
# to make with debug options, run make utility so: make DEBUG=yes
ifeq ($(DEBUG), yes)
CFLAGS = -g
LIBS += -lboost_regex-gcc47-mt-sd-1_54
else
MACROSES += NDEBUG
LIBS += -lboost_regex-gcc47-mt-s-1_54
endif


target: serv.o daemonize.o
	$(CC) $(CFLAGS) $(COMMON_FLAGS) $(LIBS) serv.o daemonize.o -o network_server	\
			-I $(HEADERS_PATH) -L $(LIBRARIES_PATH) $(LIBS)
	
serv.o: serv.hpp serv.cpp 
	$(CC) $(CFLAGS) $(COMMON_FLAGS) -c serv.cpp
	@echo "file: $^"


daemonize.o: ../mon_daemon/daemonize.cpp ../mon_daemon/daemonize.hpp
	$(CC) $(CFLAGS) $(COMMON_FLAGS) -c ../mon_daemon/daemonize.cpp


clean:
	rm -f *.o









