import unittest, glob
import serial, yaml

SERIAL_PORT_PATTERN = "/dev/ttyACM*"
SERIAL_BAUDRATE = 9600


class SerialCommandDrivenTest(unittest.TestCase):
    def setUp(self):
        port = sorted(glob.glob(SERIAL_PORT_PATTERN))[0]
        print "opening serial port: %s" % port
        self.ser = serial.Serial(port, baudrate=SERIAL_BAUDRATE)
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
          print "<---%s" % line
          if line == '...':
            break
        buff = "\n".join(buff)
        return yaml.load_all(buff)
    def testEnqueueDequeue(self):
        pkts = ['test1','test2','test3','abcedefghijklmnopqrstuvwxyz','A'*32]
        for pkt in pkts:
          self._send("PQ.ENQ %s" % pkt)
          resp = self._parse_resp().next()
          self.assertEqual(resp['pcs'],0) #check for error codes
          self._send("PQ.DEQ")
          resp = self._parse_resp().next()
          self.assertEqual(resp['pcs'],0)               #check for error codes
          self.assertEqual(resp['pkt.length'],len(pkt)) #check for size integrity
          self.assertEqual(resp['pkt.data'],pkt)        #check for data integrity

if __name__ == "__main__":
    unittest.main()
