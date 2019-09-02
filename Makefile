cc = g++

BIN_CREATE = $(shell if [ ! -d "./bin" ]; then \
		mkdir "./bin"; \
	fi;)

OBJECTS = main.o Master.o Client.o MQTT.o bt_utils.o
CFLAGS = -I/usr/local/include/
LDFLAGS = -L/usr/local/lib/
LIBS = -lleveldb -levent
CDFLAGS = -std=c++11
WL = -Wall -g

all: $(OBJECTS)

main.o: main.cpp
	$(cc) -c main.cpp -o main.o $(CFLAGS) $(CDFLAGS) $(WL)

Master.o: Master.cpp
	$(cc) -c Master.cpp -o Master.o $(CFLAGS) $(CDFLAGS) $(WL)

Client.o: Client.cpp
	$(cc) -c Client.cpp -o Client.o $(CFLAGS) $(CDFLAGS) $(WL)

MQTT.o: MQTT.cpp
	$(cc) -c MQTT.cpp -o MQTT.o $(CFLAGS) $(CDFLAGS) $(WL)

bt_utils.o: bt_utils.cpp
	$(cc) -c bt_utils.cpp -o bt_utils.o $(WL)

install:
	$(BIN_CREATE)
	$(cc) -o bin/server main.o Master.o Client.o MQTT.o bt_utils.o $(CFLAGS) $(LDFLAGS) $(LIBS) $(CDFLAGS) $(WL)


.PHONY:
clean:
	rm -rf *.o \
	rm -rf *.map \
	rm -rf bin/*