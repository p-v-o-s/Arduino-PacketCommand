import unittest, glob
import serial, yaml

SERIAL_PORT_PATTERN = "/dev/ttyACM*"
SERIAL_BAUDRATE = 9600

import random, string
def randstr(size, chars=string.ascii_uppercase + string.ascii_lowercase +string.digits):
    return ''.join(random.choice(chars) for _ in range(size))

PQ_STATUS = {
    'ERROR_QUEUE_OVERFLOW': -1,
    'ERROR_QUEUE_UNDERFLOW': -2
}

class SerialCommandDrivenTest(unittest.TestCase):
    def setUp(self):
        port = sorted(glob.glob(SERIAL_PORT_PATTERN))[0]
        print "opening serial port: %s" % port
        self.ser = serial.Serial(port, baudrate=SERIAL_BAUDRATE)
        self.ser.flush()
        self._send("PQ.RESET")
        resp = self._parse_resp().next()
        self.assertEqual(resp['pqs'],0) #check for error codes
    def tearDown(self):
        print "closing serial port"
        self.ser.close()
    def _send(self, cmd):
        self.ser.write("%s\n" % cmd.strip())
    def _parse_resp(self):
        buff = []
        while True:
          line = self.ser.readline().strip()
          buff.append(line)
          print "SER:\t%s" % line
          if line == '...':
            break
        buff = "\n".join(buff)
        return yaml.load_all(buff)
    def testEnqueueDequeue(self):
        for i in range(10):
            #generate some random printable packet data
            pkt = randstr(random.randint(1,32))
            self._send("PQ.ENQ %s" % pkt)
            resp = self._parse_resp().next()
            self.assertEqual(resp['pqs'],0) #check for error codes
            self._send("PQ.DEQ")
            resp = self._parse_resp().next()
            self.assertEqual(resp['pqs'],0)                  #check for error codes
            self.assertEqual(resp['pkt.length'],len(pkt))    #check for size integrity
            self.assertEqual(str(resp['pkt.data']),str(pkt)) #check for data integrity
    def testEnqueueOverflow(self):
        qsize = 0
        pkts= []
        while True:
            #generate some random printable packet data
            pkt = randstr(random.randint(1,32))
            self._send("PQ.ENQ %s" % pkt)
            resp = self._parse_resp().next()
            if resp['pqs'] == PQ_STATUS['ERROR_QUEUE_OVERFLOW']:
               break
            else:
                self.assertEqual(resp['pqs'],0) #check for error codes
                qsize += 1
                pkts.append(pkt)
        for pkt in pkts:
            self._send("PQ.DEQ")
            resp = self._parse_resp().next()
            self.assertEqual(resp['pqs'],0)                  #check for error codes
            self.assertEqual(resp['pkt.length'],len(pkt))    #check for size integrity
            self.assertEqual(str(resp['pkt.data']),str(pkt)) #check for data integrity
    def testEnqueueOverflowUnderflow(self):
        qsize = 0
        pkts= []
        while True:
            #generate some random printable packet data
            pkt = randstr(random.randint(1,32))
            self._send("PQ.ENQ %s" % pkt)
            resp = self._parse_resp().next()
            if resp['pqs'] == PQ_STATUS['ERROR_QUEUE_OVERFLOW']:
               break
            else:
                self.assertEqual(resp['pqs'],0) #check for error codes
                qsize += 1
                pkts.append(pkt)
        for pkt in pkts:
            self._send("PQ.DEQ")
            resp = self._parse_resp().next()
            self.assertEqual(resp['pqs'],0)                  #check for error codes
            self.assertEqual(resp['pkt.length'],len(pkt))    #check for size integrity
            self.assertEqual(str(resp['pkt.data']),str(pkt)) #check for data integrity
        #cause underflow
        self._send("PQ.DEQ")
        resp = self._parse_resp().next()
        self.assertEqual(resp['pqs'],PQ_STATUS['ERROR_QUEUE_UNDERFLOW'])#check for error codes
        self.assertEqual(resp['pkt.length'],0)    #length is zero on error
      
if __name__ == "__main__":
    unittest.main()
