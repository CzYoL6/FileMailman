import socket

def udp_client(host, port):
    # Create a UDP socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # Send data to the server
    message = bytes([0x01])

    client_socket.sendto(message, (host, port))

    # Receive response from the server
    data, server_address = client_socket.recvfrom(1024)
    header = int.from_bytes(data[0:1], byteorder="little")
    print(header)
    filenamesize = int.from_bytes(data[1:1 + 4], byteorder="little")
    print(filenamesize)
    filename = (data[5:5 + filenamesize]).decode("ascii")
    print(filename)
    filesize = int.from_bytes(data[5 + filenamesize:], byteorder="little")
    print(filesize)

    # Close the socket
    client_socket.close()

if __name__ == "__main__":
    # Set the server address and port
    server_host = "localhost"
    server_port = 25690


    # Call the UDP client function
    udp_client(server_host, server_port)