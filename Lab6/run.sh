# script to insmod kernel module & initialize char device
# Borrowed from Kevin Farley with permission
sudo insmod lab6.ko
MAJOR= dmesg | tail -n 1 | awk "{print \$5}"
if [ -z $MAJOR ]
then
	MAJOR= dmesg | tail -n 1 | awk "{print \$4}"
fi
#sudo rm /dev/interface
sudo mknod /dev/interface c 250 0
sudo chmod a+w /dev/interface
gcc -o test test.c
exit;
