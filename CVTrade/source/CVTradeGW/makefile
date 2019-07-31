CC = g++

CFLAGS = -O2 -DIPVH_331 -DTIMETEST -DDEBUG_OFF -std=c++11 -DMNTRMSG_OFF

SRC = ./SKNet/SKSocket.cpp ./SKCommon/SKThread.cpp SKTandemDAO.cpp SKHeartbeat.cpp SKTandemDAOs.cpp pevents.cpp ./SKCommon/SKQueue.cpp SKReadQueueDAO.cpp SKWriteQueueDAO.cpp \
	SKReadQueueDAOs.cpp SKWriteQueueDAOs.cpp ./SKTIG/SKTandems.cpp ./SKTIG/SKTandem.cpp ./SKCommon/SKSharedMemory.cpp FprintfStderrLog.cpp

CLIENT_OBJS = $(SRC:.cpp=.o) SKTradeGW.o

INC = -I ./SKNet -I ./SKCommon

LIB = -lpthread

OUTPUT_PATH = ../../execution_data/make_bin

all: SKTradeGW


SKTradeGW.o: SKTradeGW.cpp
	${CC} $(CFLAGS) -c `pkg-config --cflags --libs glib-2.0` SKTradeGW.cpp

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f ${CLIENT_OBJS} ${OUTPUT_PATH}/SKTradeGW 

#########################################
# Socket Client Section
SKTradeGW: ${CLIENT_OBJS}
	mkdir -p ${OUTPUT_PATH}
	${CC} -O2 `pkg-config --cflags --libs glib-2.0` -o ${OUTPUT_PATH}/SKTradeGW ${CLIENT_OBJS} ${INC} ${LIB}
