import time
from socket import *

clientSocket = socket(AF_INET, SOCK_DGRAM)
clientSocket.settimeout(1)

message = b'test'

addr = ("127.0.0.1", 6000)
clientSocket.sendto(message, addr)
