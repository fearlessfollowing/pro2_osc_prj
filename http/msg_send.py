import socket
import sys 
import os

serverAddr = '/dev/socket/http_socket'

def clientSocket():
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        sock.connect(serverAddr)
    except socket.error:
        print >> sys.stderr, "exception"
        # print >> sys.stderr, e 
        sys.exit(1)

    message = 'this is the message'
    sock.sendall(message)

    amountRecv = 0 
    amountSnd = len(message)

    while amountRecv < amountSnd:
        data = sock.recv(100)
        amountRecv += len(data)
        print >> sys.stderr, 'received "%s"' %data
    sock.close()

if __name__ == "__main__":
    clientSocket()
