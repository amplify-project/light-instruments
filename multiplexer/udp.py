import socket

server_socket = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
server_socket.bind(('::', 9000))

print("Listening...")

while True:
    message, address = server_socket.recvfrom(1024)
    print(address, message)
