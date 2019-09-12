CC = g++

CFLAGS = -O2 -std=c++11 -lcurl -lcrypto -DDEBUG_OFF

SRC = ./CVNet/CVSocket.cpp ./CVCommon/CVThread.cpp CVReplyDAO.cpp CVTandemDAOs.cpp CVPevents.cpp ./CVCommon/CVQueue.cpp CVWriteQueueDAO.cpp \
	CVWriteQueueDAOs.cpp ./CVTIG/CVTandems.cpp ./CVTIG/CVTandem.cpp ./CVCommon/CVSharedMemory.cpp FprintfStderrLog.cpp

OBJS = $(SRC:.cpp=.o) CVMain.o

INC = -I ./CVNet -I ./CVCommon

LIB = -lpthread

OUTPUT_PATH = ../../bin

all: CVReply

CVMain.o: CVMain.cpp
	${CC} $(CFLAGS) -c `pkg-config --cflags --libs glib-2.0` CVMain.cpp

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f ${OBJS} ${OUTPUT_PATH}/CVReply 

CVReply: ${OBJS}
	mkdir -p ${OUTPUT_PATH}
	${CC} $(CFLAGS) `pkg-config  --libs glib-2.0` -o ${OUTPUT_PATH}/CVReply ${OBJS} ${INC} ${LIB}
