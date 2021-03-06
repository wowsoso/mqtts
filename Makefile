CC=gcc
DEBUG=-g
INCLUDE=-Iinclude/

mqtts: src/main.c mqtts.o net.o handle.o event.o loop.o topic.o hasht.o
	${CC} $(DEBUG) src/main.c mqtts.o net.o handle.o event.o loop.o topic.o hasht.o -o mqtts $(INCLUDE)
mqtts.o: src/mqtts.c include/mqtts.h
	${CC} -c $(DEBUG) src/mqtts.c $(INCLUDE) -o mqtts.o
net.o: src/net.c include/net.h mqtts.o
	${CC} -c $(DEBUG) src/net.c $(INCLUDE) -o net.o
topic.o: src/topic.c
	${CC} -c $(DEBUG) src/topic.c $(INCLUDE) -o topic.o
handle.o: src/handle.c include/handle.h mqtts.o topic.o
	${CC} -c $(DEBUG) src/handle.c $(INCLUDE) -o handle.o
event.o: src/event.c include/event.h mqtts.o
	${CC} -c $(DEBUG) src/event.c $(INCLUDE) -o event.o
loop.o: src/loop.c mqtts.o event.o handle.o
	${CC} -c $(DEBUG) src/loop.c $(INCLUDE) -o loop.o
hasht.o: src/hasht.c include/hasht.h
	${CC} -c $(DEBUG) src/hasht.c $(INCLUDE) -o hasht.o
clean :
	rm *.o
