all:
	(cd CVQuote; make all)
	(cd CVTrade; make all)
	(cd CVMonitor; make all)
	(cd CVShareMem; make all)

clean:
	(cd CVQuote; make clean)
	(cd CVTrade; make clean)
	(cd CVMonitor; make clean)
	(cd CVShareMem; make clean)
