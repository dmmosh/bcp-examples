from ctypes import *
import os
import random
from math import log2


library_name ='libbcp.so'

try:
    bcp = CDLL(library_name)
except OSError as e:
    print(f"Error loading library: {e}")
    exit()

bcp.bcp_compress.restype = c_void_p
bcp.bcp_compress.argtypes = [c_void_p,
                             c_void_p,
                             c_uint64,
                             c_uint64,
                             c_size_t
                             ]

bcp.bcp_print_values.argtypes = [c_void_p]
bcp.free_bcp.argtypes = [c_void_p]
bcp.bcp_print.argtypes = [c_void_p]
bcp.bcp_n.argtypes = [c_void_p]
bcp.bcp_bit_sizes.argtypes = [c_void_p]
bcp.bcp_arr.argtypes = [c_void_p, c_size_t]
bcp.bcp_arr.restype = c_void_p
