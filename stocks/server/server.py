import os
import sys
import numpy as np
from math import log2
from gi.repository import GLib
import socket
import subprocess
import yfinance as yf
import pandas as pd

import time

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


def relative_volatility_index(df, period=14):

    change = df.diff()

    # rolling standard deviation
    std = df.rolling(period).std()

    up_vol = std.where(change > 0, 0)
    down_vol = std.where(change <= 0, 0)

    up_vol = up_vol.rolling(period).mean()
    down_vol = down_vol.rolling(period).mean()

    rvi = 100 * up_vol / (up_vol + down_vol)

    return rvi




# sends the socket
def send_socket(sock:socket, view:memoryview,chunk_size= 2048):
    size = len(view) 
    sock.sendall(size.to_bytes(4,byteorder='big')) 
    i = 0
    while(i<size):
        try:
            # sendall() is still the safest bet for ensuring delivery
            i+= client_sock.send(view[i : i + chunk_size])
            
        except socket.error as e:
            print(f"Send failed: {e}")
            break
    



snp500 = pd.read_excel('https://www.ssga.com/us/en/intermediary/etfs/library-content/products/fund-data/etfs/us/holdings-daily-us-en-spy.xlsx', 
                        engine='openpyxl', 
                        skiprows=4,
                        usecols=['Ticker']
                        ).dropna()['Ticker'].to_numpy()



# for i,num in enumerate(snp500):
#     print(f"'{num}'",', ', sep='', end='')
#     if(i%10 == 0):
#         print()
    
snp_i = {value: index for index, value in enumerate(snp500)}

data = yf.download(tickers=list(snp500),
                     period='1d',
                     interval='1m',
                     threads=40
                     ).tail(27) # only the last 15 periods iterested in 


'''
gets the relative volatility index (RVI) with 14 periods
'''
#close = pd.concat({t: data[t]["Close"] for t in data}, axis=1)
rank = np.searchsorted(data['Close'].dropna().columns.values,
                       snp500, 
                       sorter=np.argsort(data['Close'].dropna().columns.values)) 

print(data['Close'])
# RVI , gets the most recent, drops the NaN
rvi = relative_volatility_index(data['Close']).tail(1).iloc[-1] # turns into series
rvi = (rvi.dropna().sort_values(ascending=False).clip(0,99.99) * 100 )

rvi_rank = np.array([snp_i[i] for i in rvi.keys() if i in snp_i]).astype(np.uint16) # actual indeces that will be sent alongside the compressed data

rvi = np.ascontiguousarray(rvi,dtype=np.uint16)
# with pd.option_context('display.float_format', '{:.20f}'.format):
#     print(rvi) 

bit_sizes = np.ascontiguousarray([0]*17,dtype=np.uint64)
for r in rvi:
    bit_sizes[int(log2(r))+1 if r>0 else 0] +=1

    
rvi_obj = bcp_obj(rvi,
              bit_sizes,
              np.uint16
)    
        
# CREATE SERVER AND MAKE IT DISCOVERABLE 
set_bluetooth_discovery(True)

# 2. Create the RFCOMM socket
# Note: RFCOMM is the standard 'Classic' serial protocol

bmarks = {
    'uc': 0, #uncompressed
    'bp':0, # bit compression
    'bcp': 0 # bit count compression
}

server_sock = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)

try:
    # Bind to any available adapter on channel 1
    server_sock.bind(("00:00:00:00:00:00", 1))
    server_sock.listen(1)
    
    print("Waiting for a client to connect...")
    
    # establishes connection with the client
    client_sock, client_info = server_sock.accept()
    print(f"Connection established with {client_info}")
    
    
    # memory views
    rvi_view = memoryview(rvi)
    rvi_obj_view= memoryview((c_ubyte * (rvi_obj.obj.size)).from_address(rvi_obj.obj.obj)),
    
    # sends the string array of tickers (index is key)
    send_socket(client_sock,snp500.astype('S6').tobytes()) # sends the indeces (keys) of snp 500  (only once)
    
    
    
    
    client_sock.shutdown(socket.SHUT_WR )
    #send_socket(client_sock, rvi_rank) # sends the rvi rank (uncompressed)
    
    
    

except Exception as e:
    print(f"Server Error: {e}")

finally:
    # 4. Clean up and hide the device again
    if 'client_sock' in locals():
        client_sock.close()
    server_sock.close()
    set_bluetooth_discovery(False)
    print("Server closed and discovery disabled.")
