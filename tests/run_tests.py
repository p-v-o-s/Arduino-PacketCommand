import unittest, glob
import serial, yaml

SERIAL_PORT_PATTERN = "/dev/ttyACM*"
SERIAL_BAUDRATE = 9600

import random, string
def randstr(size, chars=string.ascii_uppercase + string.ascii_lowercase +string.digits):
    return ''.join(random.choice(chars) for _ in range(size))

PS_STATUS = {
    'NO_PACKET_RECEIVED': 1,
    'SUCCESS':0,
    'ERROR_EXCEDED_MAX_COMMANDS':-1,
    'ERROR_NO_COMMAND_NAME_MATCH':-2,
    'ERROR_INVALID_PACKET':-3,
    'ERROR_INVALID_TYPE_ID':-4,
    'ERROR_NO_TYPE_ID_MATCH':-5,
    'ERROR_NULL_HANDLER_FUNCTION_POINTER':-6,
    'ERROR_PACKET_INDEX_OUT_OF_BOUNDS':-7,
    'ERROR_INPUT_BUFFER_OVERRUN':-8,
    'ERROR_QUEUE_OVERFLOW':-9,
    'ERROR_QUEUE_UNDERFLOW':-10,
}

################################################################################
class SerialCommandDrivenTestSuite(unittest.TestCase):
    def setUp(self):
        port = sorted(glob.glob(SERIAL_PORT_PATTERN))[0]
        print "opening serial port: %s" % port
        self.ser = serial.Serial(port, baudrate=SERIAL_BAUDRATE)
        self.ser.flush()
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
################################################################################
class PackCommandTestSuite(SerialCommandDrivenTestSuite):
    def setUp(self):
        super(PackCommandTestSuite, self).setUp()  #call the setup of the parent
        self._send("PCMD.RESET")
        resp = self._parse_resp().next()
        self.assertEqual(resp['pcs'],0) #check for error codes
    def testAddCommand(self):
        self._send("PCMD.ADDCMD")
        resp = self._parse_resp().next()
        self.assertEqual(resp['pcs'],0) #check for error codes
        self.assertEqual(0,0)
################################################################################
#class PackQueueTestSuite(SerialCommandDrivenTestSuite):
#    def setUp(self):
#        super(PackQueueTestSuite, self).setUp()  #call the setup of the parent
#        self._send("PQ.RESET")
#        resp = self._parse_resp().next()
#        self.assertEqual(resp['pqs'],0) #check for error codes
#    def testEnqueueDequeue(self):
#        for i in range(10):
#            #generate some random printable packet data
#            pkt = randstr(random.randint(1,32))
#            self._send("PQ.ENQ %s" % pkt)
#            resp = self._parse_resp().next()
#            self.assertEqual(resp['pqs'],0) #check for error codes
#            self._send("PQ.DEQ")
#            resp = self._parse_resp().next()
#            self.assertEqual(resp['pqs'],0)                  #check for error codes
#            self.assertEqual(resp['pkt.length'],len(pkt))    #check for size integrity
#            self.assertEqual(str(resp['pkt.data']),str(pkt)) #check for data integrity
#    def testEnqueueOverflow(self):
#        qsize = 0
#        pkts= []
#        while True:
#            #generate some random printable packet data
#            pkt = randstr(random.randint(1,32))
#            self._send("PQ.ENQ %s" % pkt)
#            resp = self._parse_resp().next()
#            if resp['pqs'] == PS_STATUS['ERROR_QUEUE_OVERFLOW']:
#               break
#            else:
#                self.assertEqual(resp['pqs'],0) #check for error codes
#                qsize += 1
#                pkts.append(pkt)
#        #test size
#        self._send("PQ.SIZE?")
#        resp = self._parse_resp().next()
#        self.assertEqual(int(resp['size']),qsize)
#        for pkt in pkts:
#            self._send("PQ.DEQ")
#            resp = self._parse_resp().next()
#            self.assertEqual(resp['pqs'],0)                  #check for error codes
#            self.assertEqual(resp['pkt.length'],len(pkt))    #check for size integrity
#            self.assertEqual(str(resp['pkt.data']),str(pkt)) #check for data integrity
#        #test size
#        self._send("PQ.SIZE?")
#        resp = self._parse_resp().next()
#        self.assertEqual(int(resp['size']),0)
#        
#    def testEnqueueOverflowUnderflow(self):
#        qsize = 0
#        pkts= []
#        while True:
#            #generate some random printable packet data
#            pkt = randstr(random.randint(1,32))
#            self._send("PQ.ENQ %s" % pkt)
#            resp = self._parse_resp().next()
#            if resp['pqs'] == PS_STATUS['ERROR_QUEUE_OVERFLOW']:
#               break
#            else:
#                self.assertEqual(resp['pqs'],0) #check for error codes
#                qsize += 1
#                pkts.append(pkt)
#        #test size
#        self._send("PQ.SIZE?")
#        resp = self._parse_resp().next()
#        self.assertEqual(int(resp['size']),qsize)
#        #dequeue all packets
#        for pkt in pkts:
#            self._send("PQ.DEQ")
#            resp = self._parse_resp().next()
#            self.assertEqual(resp['pqs'],0)                  #check for error codes
#            self.assertEqual(resp['pkt.length'],len(pkt))    #check for size integrity
#            self.assertEqual(str(resp['pkt.data']),str(pkt)) #check for data integrity
#        #test size
#        self._send("PQ.SIZE?")
#        resp = self._parse_resp().next()
#        self.assertEqual(int(resp['size']),0)
#        #cause underflow
#        self._send("PQ.DEQ")
#        resp = self._parse_resp().next()
#        self.assertEqual(resp['pqs'],PS_STATUS['ERROR_QUEUE_UNDERFLOW'])#check for error codes
#        self.assertEqual(resp['pkt.length'],0)    #length is zero on error
#    def testEnqueueRequeueOrder(self):
#        pkts = []
#        #generate some random printable packet data and enqueue (put on back)
#        pkt = randstr(random.randint(1,32))
#        self._send("PQ.ENQ %s" % pkt)
#        resp = self._parse_resp().next()
#        self.assertEqual(resp['pqs'],0) #check for error codes
#        pkts = pkts + [pkt]
#        #generate some random printable packet data and requeue (put on front)
#        pkt = randstr(random.randint(1,32))
#        self._send("PQ.REQ %s" % pkt)
#        resp = self._parse_resp().next()
#        self.assertEqual(resp['pqs'],0) #check for error codes
#        pkts = [pkt] + pkts
#        #dequeue the front element
#        self._send("PQ.DEQ")
#        resp = self._parse_resp().next()
#        self.assertEqual(str(resp['pkt.data']),pkts[0]) #check for data integrity
#        del pkts[0]
#        #dequeue the front element
#        self._send("PQ.DEQ")
#        resp = self._parse_resp().next()
#        self.assertEqual(str(resp['pkt.data']),pkts[0]) #check for data integrity
#        del pkts[0]
#    def testRequeueEnqueueOrder(self):
#        pkts = []
#        #generate some random printable packet data and requeue (put on front)
#        pkt = randstr(random.randint(1,32))
#        self._send("PQ.REQ %s" % pkt)
#        resp = self._parse_resp().next()
#        self.assertEqual(resp['pqs'],0) #check for error codes
#        pkts = [pkt] + pkts
#        #generate some random printable packet data and enqueue (put on back)
#        pkt = randstr(random.randint(1,32))
#        self._send("PQ.ENQ %s" % pkt)
#        resp = self._parse_resp().next()
#        self.assertEqual(resp['pqs'],0) #check for error codes
#        pkts = pkts + [pkt]
#        #dequeue the front element
#        self._send("PQ.DEQ")
#        resp = self._parse_resp().next()
#        self.assertEqual(str(resp['pkt.data']),pkts[0]) #check for data integrity
#        del pkts[0]
#        #dequeue the front element
#        self._send("PQ.DEQ")
#        resp = self._parse_resp().next()
#        self.assertEqual(str(resp['pkt.data']),pkts[0]) #check for data integrity
#        del pkts[0]
#      
if __name__ == "__main__":
    unittest.main()
