CC = g++

CFLAGS = -O2 -std=c++11

OUTPUT_PATH = ../bin

all: CVSetTickNumber

SRC = CVSharedMemory.cpp CVSetTickNumber.cpp

OBJS = $(SRC:.cpp=.o)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

#Tick number
CVSetTickNumber: ${OBJS}
	mkdir -p ${OUTPUT_PATH}
	${CC} -O2 CVSharedMemory.o CVSetTickNumber.o -o ${OUTPUT_PATH}/CVSetTickNumber 

clean:
	rm -f ${OBJS} ${OUTPUT_PATH}/CVSetTickNumber
