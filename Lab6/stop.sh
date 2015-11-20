# script to rmmod kernel module & delete psuedo char device
# borrowed from Kevin Farley with permission
sudo rmmod lab6
sudo rm /dev/interface
rm test
make clean
exit;
