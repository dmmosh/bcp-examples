import os
import sys
import numpy as np
from math import log2
from gi.repository import GLib
import socket
import subprocess

import uuid
# Add the parent directory to the system path
# Calculate the path two directories up from the current script's location
current_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(current_dir)
double_parent_dir = os.path.dirname(parent_dir)

# Add the double previous directory to the system path
sys.path.append(double_parent_dir)
from header import *
        
def set_bluetooth_discovery(state=True):
    """Toggles Bluetooth discoverability and pairability using bluetoothctl."""
    mode = "on" if state else "off"
    name = "BCP Stocks Example" if state else ""
    try:
        subprocess.run(["bluetoothctl", "power", "on"], check=True)
        subprocess.run(["bluetoothctl", "system-alias", name], check=True)
        subprocess.run(["bluetoothctl", "discoverable", mode], check=True)
        subprocess.run(["bluetoothctl", "pairable", mode], check=True)
        print(f"Bluetooth discoverability set to: {mode}")
    except subprocess.CalledProcessError as e:
        print(f"Failed to set Bluetooth mode: {e}")
        


byte = 1
max_value = pow(256,byte)-1
n = 10

type_arr = None
match(byte):
    case 1:
        type_arr = np.uint8
    case 2:
        type_arr = np.uint16
    case 4:
        type_arr = np.uint32
    case 8:
        type_arr = np.uint64
    case _:
        raise TypeError('invalid byte size')

values = np.sort(np.random.randint(0,max_value,n,dtype=type_arr))[::-1]
bit_hash = np.array([0]*(byte*8+1),dtype=np.uint64)
for v in values:
    bit_hash[int(log2(v))+1 if v>0 else 0] +=1

obj = bcp_obj(values,bit_hash,type_arr)
obj.print()
out = obj.arr()

obj.print_values()

# CREATE SERVER AND MAKE IT DISCOVERABLE 

        
set_bluetooth_discovery(True)

# 2. Create the RFCOMM socket
# Note: RFCOMM is the standard 'Classic' serial protocol
server_sock = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)

try:
    # Bind to any available adapter on channel 1
    server_sock.bind(("00:00:00:00:00:00", 1))
    server_sock.listen(1)
    
    print("Waiting for a client to connect...")
    
    client_sock, client_info = server_sock.accept()
    print(f"Connection established with {client_info}")

    # 3. Simple Echo Loop
    while True:
        data = client_sock.recv(1024)
        print(len(data))
        
        if not data:
            print(f"Connection closed by client {client_info}")
            break
        
        
        print(f"Received: {data.decode('ascii')}")
        client_sock.send(b"ACK: " + data)

except Exception as e:
    print(f"Server Error: {e}")

finally:
    # 4. Clean up and hide the device again
    if 'client_sock' in locals():
        client_sock.close()
    server_sock.close()
    set_bluetooth_discovery(False)
    print("Server closed and discovery disabled.")
