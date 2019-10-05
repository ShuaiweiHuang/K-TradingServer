CC =g++

CFLAGS =-O2 -std=c++11 -DDEBUG_OFF

SRC = CVHeartbeat.cpp CVServer.cpp CVWebConns.cpp CVDumpErrLog.cpp CVGlobal.cpp CVRequest.cpp CVWebConn.cpp CVServiceHandler.cpp ./CVCommon/CVClientSocket.cpp ./CVCommon/CVPevents.cpp ./CVCommon/CVServerSocket.cpp ./CVCommon/CVSharedMemory.cpp ./CVCommon/CVThread.cpp ./CVCommon/CVQueue.cpp CVQueueNode.cpp CVQueueNodes.cpp
 

SERVER_OBJS = $(SRC:.cpp=.o) CVMain.o

INC = -I ./INCLUDE

LIB = -lssl -lcrypto -pthread  -L /usr/local/lib libboost_system.so

OUTPUT_PATH = ../bin

all: CVQuote

CVMain.o: CVMain.cpp
	${CC} $(CFLAGS) -c `pkg-config --cflags --libs glib-2.0` CVMain.cpp

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@ 

clean:
	@rm -f ${SERVER_OBJS} ${OUTPUT_PATH}/CVQuote *.o

CVQuote: ${SERVER_OBJS}
	mkdir -p ${OUTPUT_PATH}
	${CC} $(CFLAGS) `pkg-config --cflags --libs glib-2.0` -o ${OUTPUT_PATH}/CVQuote ${SERVER_OBJS} ${INC} ${LIB}
