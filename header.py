from ctypes import *
import os
import random
from math import log2
import numpy as np


library_name ='libbcp.so'

try:
    bcp = CDLL(library_name)
except OSError as e:
    print(f"Error loading library: {e}")
    exit()

bcp.bcp_compress.restype = c_void_p
bcp.bcp_compress.argtypes = [c_void_p,
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
