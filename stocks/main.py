import os
import sys
# Add the parent directory to the system path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_dir)
from header import *


bit_size = 64
max_value = pow(2,bit_size)-1
n = 10
def bit_nums(arr):
    out = [0]*(bit_size+1)
    for a in arr:
        out[int(log2(a))+1 if a>0 else 0]+=1
    return out

def cast_void(arr:list)-> c_void_p :
    return (c_void_p * len(arr))(*arr)

arr = [random.randint(0) for _ in range(n)]
arr.sort(reverse=True)
arr_in = (c_uint64 * n)(*arr)
bit_nums_arr = (c_uint64*(bit_size+1))(*(bit_nums(arr)))
obj = bcp.bcp_compress(pointer(arr_in), 
                       pointer(bit_nums_arr),
                       c_uint64(len(arr)),
                       c_uint64(len(bit_nums_arr)),
                       c_size_t(int(bit_size/8)) )

print(arr)
print()
bcp.bcp_print_values(obj)
bcp.free_bcp(obj)