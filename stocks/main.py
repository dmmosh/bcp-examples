import os
import sys
import numpy as np
# Add the parent directory to the system path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_dir)
from header import *

        
def bcp_print(obj:c_void_p):
    bcp.bcp_print(obj)
    
def bcp_print_values(obj:c_void_p):
    bcp.bcp_print_values(obj)
    
def bcp_n(obj:c_void_p)->int:
    return bcp.bcp_n(obj)

def bcp_byte(obj:c_void_p)->int:
    return bcp.bcp_byte(obj)

def free_bcp(obj:c_void_p):
    bcp.free_bcp(obj)

def bcp_arr(obj:c_void_p)-> np.array:
    
    n = bcp_n(obj)
    byte = bcp_byte(obj)
    print(n,byte)
    
    type_arr = None
    match(byte):
        case 8:
            type_arr = c_uint8
        case 16:
            type_arr = c_uint16
        case 32:
            type_arr = c_uint32
        case 64:
            type_arr = c_uint64
        case _:
            raise TypeError('invalid byte size')
    
    arr_ptr = cast(bcp.bcp_arr(obj, byte//8), POINTER(type_arr))
    out =  np.ctypeslib.as_array(arr_ptr, shape=(n,)).copy()
    bcp.free_arr(arr_ptr)
    return out

def bcp_bit_sizes(obj:c_void_p)->np.array:
    arr_ptr = cast(bcp.bcp_bit_sizes(obj), POINTER(c_uint64))
    out =  np.ctypeslib.as_array(arr_ptr, shape=(bcp_byte(obj)+1,)).copy()
    bcp.free_arr(arr_ptr)
    return out


class bcp_obj:
    
    def __init__(self, arr_to_compress:np.array, arr_bit_hash:np.array, dtype: np.dtype):
        
        
        self.obj = bcp.bcp_compress(
                        np.ascontiguousarray(arr_to_compress).ctypes.data_as(c_void_p), 
                       np.ascontiguousarray(arr_bit_hash).ctypes.data_as(POINTER(c_uint64)), 
                       c_uint64(len(arr_to_compress)),
                       c_size_t(arr_to_compress.itemsize) )
    
    
        
    def __call__(self, *args, **kwds):
        pass
    
    def __del__(self):
        bcp.free_bcp(self.obj)
        
    def print(self):
        bcp.bcp_print(self.obj)
    
    def print_values(self):
        bcp.bcp_print_values(self.obj)
        
    def n(self)->int:
        return bcp.bcp_n(self.obj)
    
    def byte(self)->int:
        return bcp.bcp_byte(self.obj)
    
    def arr(self)-> np.array:
        return bcp_arr(self.obj)
    
    def bit_sizes(self)->np.array:
        return bcp_bit_sizes(self.obj)



byte = 4
max_value = pow(256,byte)-1
n = 30

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
print(bit_hash)
for v in values:
    bit_hash[int(log2(v))+1 if v>0 else 0] +=1

obj = bcp_obj(values,bit_hash,type_arr)
obj.print_values()
obj.print()
out = obj.arr()
print(out)
print(obj.bit_sizes())
