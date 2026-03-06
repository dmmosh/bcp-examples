from ctypes import *
import os
import random
from math import log2

'''
typedef void* bcp_obj; // the object
typedef uint8_t arr_placeholder;
//typedef uint64_t __attribute__((aligned(1))) unaligned_uint64_t;



typedef struct bcp_bits{
    uint64_t i; // byte index of the end of the bcp bits array
    uint8_t start_bits; // the starting bits of the bcp bits array
    uint64_t* arr;
}bcp_bits;

typedef struct bcp_obj_bytes{
    bcp_obj obj;
    uint64_t size;
}bcp_obj_bytes;


#define BYTE_BITS 8
#define P8(x) printf("%"PRIu8"\n",x);
#define P16(x) printf("%"PRIu16"\n",x);
#define P32(x) printf("%"PRIu32"\n",x);
#define P64(x) printf("%"PRIu64"\n",x);

// ACCESSING bcp OBJ

// helper functions 
void bcp_print(const bcp_obj bcp);
void bcp_print_values(const bcp_obj bcp);
void* bcp_arr(const bcp_obj bcp, size_t size_out);
size_t bcp_n(const bcp_obj bcp);
size_t bcp_byte(const bcp_obj bcp);
uint8_t bcp_data_start_bits(const bcp_obj bcp);
uint8_t bcp_byte_raw(const bcp_obj bcp);
uint64_t* bcp_bit_sizes(const bcp_obj bcp);
uint64_t bcp_bit_fields_bits_max( uint64_t n_arr,  size_t byte);
bcp_bits bcp_bit_sizes_obj(const bcp_obj bcp);


// COMPRESSING bcp OBJ

bool will_it_fit(const uint64_t byte_size, const uint64_t num_i, const uint64_t bit_num, const uint64_t start_bits);
//bcp_obj bcp_compress(const void* arr_to_compress, const uint64_t* arr_bit_hash, const uint64_t n_arr, const size_t size);
bcp_obj bcp_compress(const void* arr_to_compress, const uint64_t* arr_bit_hash, const uint64_t n_arr, const uint16_t n_bit_arr, const size_t size);
bcp_obj_bytes bcp_compress_bytes(const void* arr_to_compress, const uint64_t* arr_bit_hash, const uint64_t n_arr, const uint16_t n_bit_arr, const size_t size);
void insert_bits(arr_placeholder* arr, uint64_t* i, uint64_t value,uint64_t bit_num, uint8_t start_bits);
void check_and_insert_bits(arr_placeholder** arr, size_t* byte_size, uint64_t value,  uint64_t* i, uint64_t bit_num, uint64_t start_bits);


'''

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
