import socket

def udp_client(host, port, message):
    # Create a UDP socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # Send data to the server
    client_socket.sendto(message.encode(), (host, port))
    client_socket.sendto(message.encode(), (host, port))

    # Receive response from the server
    data, server_address = client_socket.recvfrom(1024)
    print(f"Received response from server: {data.decode()}")
    data, server_address = client_socket.recvfrom(1024)
    print(f"Received response from server: {data.decode()}")

    # Close the socket
    client_socket.close()

if __name__ == "__main__":
    # Set the server address and port
    server_host = "localhost"
    server_port = 25690

    # Message to be sent
    message_to_send = "Hello, UDP Server!"

    # Call the UDP client function
    udp_client(server_host, server_port, message_to_send)