#printf "\nBuild Server.....\n\n"
#cd ../CVServer
#make clean; make
#printf "\nBuild Share Port.....\n\n"
#cd ../CVSetSharedMemory
#make
#printf "\nBuild gateway.....\n\n"
#cd ../CVTradeGW
#make clean; make
#printf "\nBuild End\n\n"
#cd ../../
cd source
make clean ; make
cd ../
