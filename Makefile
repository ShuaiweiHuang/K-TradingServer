all:
	(cd CVQuote; make all)
	(cd CVShareMem; make all)

clean:
	(cd CVQuote_Debug; make clean)
	(cd CVShareMem; make clean)
