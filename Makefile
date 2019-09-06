all:
	(cd CVQuote; make all)
	(cd CVReply; make all)
	(cd CVTrade; make all)
	(cd CVShareMem; make all)

clean:
	(cd CVQuote; make clean)
	(cd CVReply; make clean)
	(cd CVTrade; make clean)
	(cd CVShareMem; make clean)
