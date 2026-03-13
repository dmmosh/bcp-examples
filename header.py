from ctypes import *
import numpy as np

'''
HEADER FILE
used for both client and server 

'''

library_name ='libbcp.so'

try:
    bcp = CDLL(library_name)
except OSError as e:
    print(f"Error loading library: {e}")
    exit()

class bcp_obj_bytes(Structure): # struct for bcp obj type
    _fields_ = [
        ("obj", c_void_p),
        ("size", c_uint64)
        
]

bcp.bcp_compress.restype = c_void_p
bcp.bcp_compress.argtypes = [c_void_p,
                             POINTER(c_uint64),
                             c_uint64,
                             c_size_t
                             ]
bcp.bcp_compress_bytes.restype = bcp_obj_bytes
bcp.bcp_compress_bytes.argtypes = [c_void_p,
                             POINTER(c_uint64),
                             c_uint64,
                             c_size_t
                             ]

bcp.bcp_n.argtypes = [c_void_p]
bcp.bcp_n.restype = c_size_t
bcp.bcp_byte.argtypes = [c_void_p]
bcp.bcp_byte.restype = c_size_t
bcp.bcp_bit_sizes.argtypes = [c_void_p]
bcp.bcp_bit_sizes.restype = POINTER(c_uint64)

bcp.bcp_print_values.argtypes = [c_void_p]
bcp.free_bcp.argtypes = [c_void_p]
bcp.free_arr.argtypes = [c_void_p]
bcp.bcp_print.argtypes = [c_void_p]
bcp.bcp_n.argtypes = [c_void_p]
bcp.bcp_bit_sizes.argtypes = [c_void_p]
bcp.bcp_arr.argtypes = [c_void_p, c_size_t]
bcp.bcp_arr.restype = c_void_p


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


def bcp_bit_sizes(obj:c_void_p)->np.array:
    arr_ptr = cast(bcp.bcp_bit_sizes(obj), POINTER(c_uint64))
    out =  np.ctypeslib.as_array(arr_ptr, shape=(bcp_byte(obj)+1,)).copy()
    bcp.free_arr(arr_ptr)
    return out



class arr_obj:
    def __init__(self, obj:c_void_p):
        n = bcp_n(obj)
        byte = bcp_byte(obj)
        #print(n,byte)

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

        self.arr_ptr = cast(bcp.bcp_arr(obj, byte//8), POINTER(type_arr))
        self.arr =  np.ctypeslib.as_array(self.arr_ptr, shape=(n,))
        
    def as_numpy(self):
        return self.arr

    def __call__(self):
        return self.arr
    
    def as_raw(self):
        return self.arr_ptr
        
    def __del__(self):
        bcp.free_arr(self.arr_ptr)
        self.arr_ptr = None
        
        
def bcp_arr(obj:c_void_p)-> arr_obj:
    return arr_obj(obj)


class bcp_obj:
    
    def __init__(self, arr_to_compress:np.array, arr_bit_hash:np.array, dtype: np.dtype):
        
        
        self.obj = bcp.bcp_compress_bytes(
                        np.ascontiguousarray(arr_to_compress).ctypes.data_as(c_void_p), 
                       np.ascontiguousarray(arr_bit_hash).ctypes.data_as(POINTER(c_uint64)), 
                       c_uint64(len(arr_to_compress)),
                       c_size_t(arr_to_compress.itemsize) )
    
    
        
    def __call__(self, *args, **kwds):
        pass
    
    def __del__(self):
        bcp.free_bcp(self.obj.obj)
        
    def print(self):
        bcp.bcp_print(self.obj.obj)
    
    def print_values(self):
        bcp.bcp_print_values(self.obj.obj)
        
    def n(self)->int:
        return bcp.bcp_n(self.obj.obj)
    
    def byte(self)->int:
        return bcp.bcp_byte(self.obj.obj)
    
    def arr(self)-> arr_obj:
        return bcp_arr(self.obj.obj)
    
    def bit_sizes(self)->np.array:
        return bcp_bit_sizes(self.obj.obj)
    
    def byte_size(self):
        return self.obj.size
        
