
#from bluetooth import *
from bluetooth import find_service
from bluetooth import BluetoothSocket
from bluetooth import RFCOMM
import sys
import time

#--------------------------------------------
class rfcomm_client:

    def __init__(self):
        self.server_dbaddr = "BC:83:85:F0:56:F4"
        self.server_uuid = "4E989E9F-F81A-4EE8-B9A7-BB74D5CD3C96" # service uuid generated by some web service.
        self.sleep_time = 3.0
        self.service_matches = None
        self.socket = None

    def close_socket(self):
        try:
            if None != self.socket:
                print("close socket.")
                self.socket.close()
                self.socket = None
        except:
            print("Exception occured : close_socket")

    def find_server(self):
        print("search bluetooth devices for the rfcomm Server service")
        try:
            self.service_matches = find_service( uuid = self.server_uuid, address = self.server_dbaddr )
            if 0 < len(self.service_matches):
                print("server service found.")
                return True
            else:
                print("couldn\'t find the server service.")
        except:
            print("Exception occured : find_server")

        return False

    def connect_to_server(self):
        if 0 == len(self.service_matches):
            return

        self.close_socket()

        try:
            first_match = self.service_matches[0]
            port = first_match["port"]
            name = first_match["name"]
            host = first_match["host"]
            print("connecting... name:%s host:%s port:%s" % (name, host, port))

            self.socket=BluetoothSocket(RFCOMM)
            self.socket.connect((host, port))
            print("connected.")
            return True
        except:
            self.close_socket()
            print("Exception occured : connect_to_server")

        return False

    def find_and_connect_to_server(self):
        while True:
            print("--------------------------------------------------")
            print("find server")
            if True == self.find_server():
                print("--------------------------------------------------")
                print("connect to server")
                if True == self.connect_to_server():
                    return True

            print("--------------------------------------------------")
            print("retrying...")
            time.sleep(self.sleep_time)

        return False

    def loop(self):
        try:
            print("--------------------------------------------------")
            print("message loop start.")
            while True:
                data = self.socket.recv(1024)
                if len(data) == 0:
                    print("connection close : loop")
                    self.close_socket()
                    break
                print("received [%s]" % data)
                self.socket.send("received.\n")
        except:
            self.close_socket()
            print("Exception occured : loop")

    def run(self):
        while True:
            if True == self.find_and_connect_to_server():
                self.loop()

        print("rfcomm_client exit.")
        return

#--------------------------------------------
if sys.version >= '3':
    client = rfcomm_client()
    client.run()
else:
    print("rfcomm-test.py : doesn't support python2.")
