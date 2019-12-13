all:
	(cd CVQuote; make all)
	(cd CVTrade; make all)
	(cd CVMonitor; make all)
	(cd CVReplyWS; make all)
	(cd CVOrderBook; make all)
	(cd CVShareMem; make all)

clean:
	(cd CVQuote; make clean)
	(cd CVTrade; make clean)
	(cd CVMonitor; make clean)
	(cd CVReplyWS; make clean)
	(cd CVOrderBook; make clean)
	(cd CVShareMem; make clean)
