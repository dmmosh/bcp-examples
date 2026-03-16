import os
import sys
import numpy as np
from math import log2
from gi.repository import GLib
import socket
import subprocess
import yfinance as yf
import pandas as pd

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

snp500 = pd.read_excel('https://www.ssga.com/us/en/intermediary/etfs/library-content/products/fund-data/etfs/us/holdings-daily-us-en-spy.xlsx', 
                        engine='openpyxl', 
                        skiprows=4,
                        usecols=['Ticker']
                        ).dropna()['Ticker'].tolist()

data = yf.download(tickers=snp500,
                     period='1d',
                     interval='1m',
                     threads=40
                     ).tail(27) # only the last 15 periods iterested in 


'''
gets the relative volatility index (RVI) with 14 periods
'''
#close = pd.concat({t: data[t]["Close"] for t in data}, axis=1)

# RVI , gets the most recent, drops the NaN
rvi = relative_volatility_index(data['Close']).tail(1).iloc[-1] # turns into series
rvi = rvi.dropna().sort_values(ascending=False)
with pd.option_context('display.float_format', '{:.20f}'.format):
    print(rvi)


#volatility = ((stocks['High'] - stocks['Close'])/stocks['Price']).T.dropna()
#print('NUM OF NANS:', volatility.isna().sum())
#print(volatility)

# with pd.option_context('display.max_rows', None, 'display.max_columns', None):
#     print(volatility)
#print(type(obj.obj.obj))
#print(obj.obj.size)

byte = 4
max_value = pow(256,byte)-1
n = 10000

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

#obj.print_values()
        
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
    
    
    # buffer = (c_ubyte * obj.obj.size).from_address(obj.obj.obj) # converts to a unsigned byte array view
    # view = memoryview(buffer) # creates a memoryview (instead of a bytes object)
    #print(len(view)) confirmation

    
    
    buffer = (c_ubyte * (obj.obj.size)).from_address(obj.obj.obj) # converts to a unsigned byte array view
    view = memoryview(buffer) # creates a memoryview (instead of a bytes object)
    
    chunk_size = 5096 # chunks, n of bytes
    #n_chunks = obj.obj.size // chunk_size + (obj.obj.size % chunk_size != 0)
    print(obj.obj.size)
    # first request will be the n of chunks
    client_sock.send(obj.obj.size.to_bytes(4,byteorder='big')) 
    #print(obj.obj.size)

    #client_sock.sendall(view)    
    
    i=0
    while(i<obj.obj.size):
        try:
            # sendall() is still the safest bet for ensuring delivery
            i+= client_sock.send(view[i : i + chunk_size])
            
        except socket.error as e:
            print(f"Send failed: {e}")
            break
   
    
    client_sock.shutdown(socket.SHUT_WR)
    
    
    # 3. Simple Echo Loop
    #while True:
        #  data = client_sock.recv(1024)
        # # print(len(data))
        
        # if not data:
        #     print(f"Connection closed by client {client_info}")
        #     break
        
        
        #print(f"Received: {data.decode('ascii')}")
        
        
        
        #client_sock.send(b"ACK: " + data)

except Exception as e:
    print(f"Server Error: {e}")

finally:
    # 4. Clean up and hide the device again
    if 'client_sock' in locals():
        client_sock.close()
    server_sock.close()
    set_bluetooth_discovery(False)
    print("Server closed and discovery disabled.")
