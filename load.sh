# usb bootloader stuff, keep this just in case
#dfu-suffix -a ./main.bin -v 1eaf -p 0003 -d 0001
#dfu-util -d 1eaf:0003 -a 1 -D ./main.bin -R

make clean
make

stm32flash -w main.bin -b 115200 -v -g 0x0 /dev/ttyUSB0
