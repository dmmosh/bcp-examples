import os
import sys
import numpy as np
from math import log2
# Add the parent directory to the system path
# Calculate the path two directories up from the current script's location
current_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(current_dir)


# Add the double previous directory to the system path
sys.path.append(parent_dir)
from header import *


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

obj.print_values()