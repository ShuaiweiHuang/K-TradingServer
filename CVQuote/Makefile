CC = g++ 

CFLAGS = -O2 -std=c++11 -fpermissive -DWFMO -DDEBUG

SRC = CVHeartbeat.cpp CVServer.cpp CVWebClientManager.cpp CVDumpErrLog.cpp CVGlobal.cpp CVRequest.cpp CVWebClient.cpp CVServerManager.cpp ./CVCommon/CVClientSocket.cpp ./CVCommon/CVPevents.cpp ./CVCommon/CVServerSocket.cpp ./CVCommon/CVSharedMemory.cpp ./CVCommon/CVThread.cpp CVWebSocket.cpp
 

SERVER_OBJS = $(SRC:.cpp=.o) CVMain.o

INC = -I ./INCLUDE

LIB = -lssl -lcrypto -pthread -lboost_system -lboost_random -lboost_chrono

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
