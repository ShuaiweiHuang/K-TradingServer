all:
	(cd CVQuote; make all)
	(cd CVTrade; make all)
	(cd CVShareMem; make all)

clean:
	(cd CVQuote; make clean)
	(cd CVTrade; make clean)
	(cd CVShareMem; make clean)
