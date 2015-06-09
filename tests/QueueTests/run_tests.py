import serial, time, struct
s = serial.Serial("/dev/ttyACM0", baudrate=9600)
#-------------------------------------------------------------------------------
#easy to type data
print "-"*80
#type ID 0x41 -> LED_on
print "testing type ID 0x41 -> LED_on:"
pkt = "A123456789"
print "writing packet: \"%s\"" % pkt
s.write(pkt)
print "reading line:", s.readline().strip()
print "reading line:", s.readline().strip()
time.sleep(1.0) #pause so you can see blinky!
print "-"*80

#type ID 0x42 -> LED_off
print "testing type ID 0x42 -> LED_off:"
pkt = "B123456789"
print "writing packet: \"%s\"" % pkt
s.write(pkt)
print "reading line:", s.readline().strip()
print "reading line:", s.readline().strip()
print "-"*80

#type ID 0x43 -> handle_int
print "testing type ID 0x43 -> handle_int:"
pkt = "CAAAA56789"
print "writing packet: \"%s\"" % pkt
s.write(pkt)
print "reading line:", s.readline().strip()
print "reading line:", s.readline().strip()
print 'evaluating "struct.unpack("i","AAAA")":', struct.unpack("i","AAAA")
print "-"*80

#type ID 0x44 -> handle_float
print "testing type ID 0x44 -> handle_float:"
pkt = "DAAAA56789"
print "writing packet: \"%s\"" % pkt
s.write(pkt)
print "reading line:", s.readline().strip()
print "reading line:", s.readline().strip()
print 'evaluating "struct.unpack("f","AAAA")":', struct.unpack("f","AAAA")  #eh... close enough for Arduino's 1 decimal place rounding
print "-"*80

#type ID 0x45 -> handle_char_array
print "testing type ID 0x45 -> handle_char_array:"
pkt = "E123456789"
print "writing packet: \"%s\"" % pkt
s.write(pkt)
print "reading line:", s.readline().strip()
print "reading line:", s.readline().strip()
print "-"*80

#type ID 0x46 -> unrecognized (default handler)
print "testing type ID 0x46 -> unrecognized (default handler):"
pkt = "F123456789"
print "writing packet: \"%s\"" % pkt
s.write(pkt)
print "reading line:", s.readline().strip()
print "reading line:", s.readline().strip()
print "-"*80

#-------------------------------------------------------------------------------
#"unprintable" data
#type ID 0xFF01 -> handle_int_float
print "testing type ID 0xFF01 -> handle_int_float:"
pkt = "\xff\x01\xff\xff\xff\xfe\x00\x00\x00\xbf"
print "writing packet: %r" % pkt
s.write(pkt)
print "reading line:", s.readline().strip()
print "reading line:", s.readline().strip()
print "reading line:", s.readline().strip()
print r'evaluating: struct.unpack("i","\xff\xff\xff\xfe") ==', struct.unpack("i","\xff\xff\xff\xfe")
print r'evaluating: struct.unpack("i","\x00\x00\x00\xbf") ==', struct.unpack("f","\x00\x00\x00\xbf")
print "-"*80
