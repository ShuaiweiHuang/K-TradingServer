CC = g++ 

CFLAGS = -O2 -DDEBUG_OFF -DTIMETEST -std=c++11 -DWFMO -DMNTRMSG_OFF -lcurl

SRC = ./CVNet/CVServerSocket.cpp ./CVCommon/CVThread.cpp CVClient.cpp CVClients.cpp ./CVCommon/CVQueue.cpp CVQueueDAO.cpp \
	  CVQueueDAOs.cpp FprintfStderrLog.cpp ./CVCommon/CVSharedMemory.cpp CVHeartbeat.cpp CVPevents.cpp ./CVNet/CVSocket.cpp CVErrorMessage.cpp

SERVER_OBJS = $(SRC:.cpp=.o) CVMain.o

INC = -I ./CVNet -I ./CVCommon

LIB = -lpthread

OUTPUT_PATH = ../../../bin

all: CVTrade

CVMain.o: CVMain.cpp
	${CC} $(CFLAGS) -c `pkg-config --cflags --libs glib-2.0` CVMain.cpp

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f ${SERVER_OBJS} ${OUTPUT_PATH}/CVTrade

#########################################
# Socket Server Section
CVTrade: ${SERVER_OBJS}
	mkdir -p ${OUTPUT_PATH}
	${CC} $(CFLAGS) `pkg-config --cflags --libs glib-2.0` -o ${OUTPUT_PATH}/CVTrade ${SERVER_OBJS} ${INC} ${LIB}
