import socket

host = "localhost"
port = 9000

client = socket.socket(socket.AF_INET,socket.SOCK_STREAM)

client.connect((host,port))

client.send("%x %x %x %x %x %x")

response = client.recv(5000)

print response
