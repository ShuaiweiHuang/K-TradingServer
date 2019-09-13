CC = g++

CFLAGS = -O2 -std=c++11

OUTPUT_PATH = ../bin

all: CVSetShareNumber

SRC = CVSharedMemory.cpp CVSetTickNumber.cpp

OBJS = $(SRC:.cpp=.o)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

#Tick number
CVSetShareNumber: ${OBJS}
	mkdir -p ${OUTPUT_PATH}
	${CC} -O2 CVSharedMemory.o CVSetTickNumber.o -o ${OUTPUT_PATH}/CVSetShareNumber 

clean:
	rm -f ${OBJS} ${OUTPUT_PATH}/CVSetShareNumber
