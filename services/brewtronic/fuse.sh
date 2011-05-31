#
 avrdude -p m644p -P usb -i 2 -c avrisp2 -U lfuse:w:0xe7:m -U  hfuse:w:0xdc:m 