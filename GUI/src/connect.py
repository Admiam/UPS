# connect.py
import socket
import struct


def connect_to_server(server_ip, player_name, port):
    # server_port = 4242  # Define your server's port
    try:
        s = socket.socket()
        s.connect((server_ip, port))

        # Construct the login message
        magic = "RPS|".encode('utf-8')
        command = "login|".encode('utf-8')
        message = player_name.encode('utf-8')
        total_length = len(magic) + len(command) + len(message)

        # Pack and send the data
        buffer = struct.pack(f'!{len(magic)}s{len(command)}sI{len(message)}s',
                             magic, command, total_length, message)

        print(buffer)
        s.send(buffer)

        return s  # Return the socket to be used by WaitingScreen

    except Exception as e:
        print(f"Connection error: {e}")
        return None  # Return None if connection fails
