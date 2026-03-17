import os
import sys
import numpy as np
from math import log2
import socket
import subprocess


'''
bluetooth stock prices examplee
make for short time spans, so daytrader
send sorted data 
'''

# Add the parent directory to the system path
# Calculate the path two directories up from the current script's location
current_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(current_dir)
double_parent_dir = os.path.dirname(parent_dir)

# Add the double previous directory to the system path
sys.path.append(double_parent_dir)
from header import *


def rec(sock:socket):
    response = bytearray()
    b = int.from_bytes(sock.recv(4),byteorder='big')
    #print(n)
    while(b>0):
        chunk = sock.recv(5096)
        if not chunk:
            break
        b-=len(chunk)
        response.extend(chunk)
    return response


def get_mac_by_name(target_name):
    print(f"Scanning for '{target_name}'...")
    try:
        # Run 'bluetoothctl devices' to see already known/paired devices
        # Or 'hcitool scan' for a fresh inquiry
        result = subprocess.check_output(["bluetoothctl", "devices"], text=True)
        
        for line in result.splitlines():
            if target_name in line:
                # Extract MAC address (e.g., AA:BB:CC:DD:EE:FF)
                match = re.search(r"([0-9A-F]{2}:){5}[0-9A-F]{2}", line, re.I)
                if match:
                    return match.group(0)
    except Exception as e:
        print(f"Scan failed: {e}")
    return None

def connect_to_device(name):
    mac_address = get_mac_by_name(name)
    
    if not mac_address:
        print(f"Could not find MAC address for {name}. Make sure it's paired or visible.")
        return

    print(f"Found {name} at {mac_address}. Connecting...")

    # Standard Python socket communication
    try:
        # AF_BLUETOOTH is Linux-specific in the standard library
        sock = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
        sock.connect((mac_address, 1)) # Port 1 is the RFCOMM default
        
        
        snp500 = np.char.decode(rec(sock), encoding='ascii').astype(str)
        
        
        # arr = bcp_arr((c_char*len(response)).from_buffer(response))
        # print(len(response))
        # with np.printoptions(threshold=np.inf):
        #     print(arr.as_numpy())

        sock.close()
    except Exception as e:
        print(f"Socket error: {e}")

if __name__ == "__main__":
    connect_to_device("BCP Stocks Example")