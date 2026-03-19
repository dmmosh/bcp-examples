#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <bcp.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <unistd.h>
uint64_t n;
bool normalDist;

// Macro to prevent a value from being optimized away
#define DO_NOT_OPTIMIZE(var) __asm__ volatile("" : "+g" (var))

#define S_END (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1e9

typedef struct arr_test{
    uint8_t* internal_arr8;
    uint16_t* internal_arr16;
    uint32_t* internal_arr32;
    uint64_t* internal_arr64;

    uint64_t* internal_arr8_bit_hash; 
    uint64_t* internal_arr16_bit_hash;
    uint64_t* internal_arr32_bit_hash;
    uint64_t* internal_arr64_bit_hash;
} arr_test;

uint8_t* arr8 = NULL; // 1 byte
uint16_t* arr16 = NULL;// 2 bytes
uint32_t* arr32 = NULL;// 4 bytes
uint64_t* arr64 = NULL;// 8 bytes


// a hash table representing the number of elements that has i+1 bits and 0 bit
uint64_t* arr8_bit_hash = NULL; 
uint64_t* arr16_bit_hash = NULL;
uint64_t* arr32_bit_hash = NULL;
uint64_t* arr64_bit_hash = NULL;


static inline uint64_t splitmix64(uint64_t *s) {
    uint64_t z = (*s += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

/**
 * Returns a normally distributed number of bits based on 
 * a desired mean and standard deviation.
 */
int get_normal_bit_count(double mean, double std_dev) {
    static double V1, V2, S;
    static int phase = 0;
    double X;

    if (phase == 0) {
        do {
            double U1 = (double)rand() / RAND_MAX;
            double U2 = (double)rand() / RAND_MAX;
            V1 = 2 * U1 - 1;
            V2 = 2 * U2 - 1;
            S = V1 * V1 + V2 * V2;
        } while (S >= 1 || S == 0);

        X = V1 * sqrt(-2 * log(S) / S);
    } else {
        X = V2 * sqrt(-2 * log(S) / S);
    }

    phase = 1 - phase;
    int result = (int)(X * std_dev + mean);
    
    // Clamp to valid range for 64-bit int
    if (result < 0) return 0;
    if (result > 64) return 64;
    return result;
}

uint64_t generate_custom_normal_bits(int mean_bits) {
    int bits_to_set = get_normal_bit_count(mean_bits, 4.0); // 4.0 is std_dev
    uint64_t val = 0;
    for (int i = 0; i < bits_to_set; i++) {
        val |= (1ULL << (rand() % 64));
    }
    return val;
}


// Comparator functions for descending order of unsigned int
// ik this is monotonous, no way to change this function

int compare_desc8(const void *a, const void *b) {
    uint8_t ua = *(const uint8_t *)a;
    uint8_t ub = *(const uint8_t *)b;
    if (ua > ub) return -1;
    if (ua < ub) return 1;
    return 0;
}

int compare_desc16(const void *a, const void *b) {
    uint16_t ua = *(const uint16_t *)a;
    uint16_t ub = *(const uint16_t *)b;
    if (ua > ub) return -1;
    if (ua < ub) return 1;
    return 0;
}

int compare_desc32(const void *a, const void *b) {
    uint32_t ua = *(const uint32_t *)a;
    uint32_t ub = *(const uint32_t *)b;
    if (ua > ub) return -1;
    if (ua < ub) return 1;
    return 0;
}

int compare_desc64(const void *a, const void *b) {
    uint64_t ua = *(const uint64_t *)a;
    uint64_t ub = *(const uint64_t *)b;
    if (ua > ub) return -1;
    if (ua < ub) return 1;
    return 0;
}


void sort_arr_test(arr_test arr){
    qsort(arr.internal_arr8, n, sizeof(arr.internal_arr8[0]), compare_desc8);
    qsort(arr.internal_arr16, n, sizeof(arr.internal_arr16[0]), compare_desc16);
    qsort(arr.internal_arr32, n, sizeof(arr.internal_arr32[0]), compare_desc32);
    qsort(arr.internal_arr64, n, sizeof(arr.internal_arr64[0]), compare_desc64);
    //int (*__compar_fn_t)(const void *a, const void *b); 
};


unsigned long rand_u_long() {
    uint64_t r = 0;
    
    // Combine multiple rand() calls to cover 64 bits
    // The exact number of calls depends on the system's RAND_MAX, 
    // but 4 calls of at least 15 bits each are safe for 64 bits.
    r |= (uint64_t)rand() << 48;
    r |= (uint64_t)rand() << 32;
    r |= (uint64_t)rand() << 16;
    r |= (uint64_t)rand();
    
    return r;
}

unsigned long pow_u_long(unsigned long a, unsigned long b){
    unsigned long out;
    for(int i =0; i<b; i++){
        out*=a;
    }

    return out;
}

// A custom function to print the binary representation of a single byte
void print_byte_binary(unsigned char byte) {
    for (int i = BYTE_BITS-1; i >=0; i--) {
        // Use bitwise right shift and AND to check each bit
        printf("%d", (byte >> i) & 1);
    }
}

// Function to print a generic buffer (void array) as binary - DBEUG
void print_binary_buffer(void *buffer, size_t n) {
    // Cast the void pointer to a char pointer for byte-level access

    arr_placeholder* arr = buffer; //  unsigned char, 1 byte
    for(size_t i=0; i<n; i++){
        print_byte_binary(arr[n-i-1]);
        printf(" ");
    }
    
    printf("\n");
}

void assign_curr_tests(const arr_test arr_tests){
    arr8 =   arr_tests.internal_arr8; 
    arr16 = arr_tests.internal_arr16;
    arr32 = arr_tests.internal_arr32;
    arr64 = arr_tests.internal_arr64;
    arr8_bit_hash = arr_tests.internal_arr8_bit_hash;
    arr16_bit_hash = arr_tests.internal_arr16_bit_hash;
    arr32_bit_hash = arr_tests.internal_arr32_bit_hash;
    arr64_bit_hash = arr_tests.internal_arr64_bit_hash;
};

// deletes the test
void delete_test(arr_test* arr_tests){
    free(arr_tests->internal_arr8);
    free(arr_tests->internal_arr16);
    free(arr_tests->internal_arr32);
    free(arr_tests->internal_arr64);
    free(arr_tests->internal_arr8_bit_hash);
    free(arr_tests->internal_arr16_bit_hash);
    free(arr_tests->internal_arr32_bit_hash);
    free(arr_tests->internal_arr64_bit_hash);
    arr_tests->internal_arr8 = NULL;
    arr_tests->internal_arr16 = NULL;
    arr_tests->internal_arr32 = NULL;
    arr_tests->internal_arr64 = NULL;
    arr_tests->internal_arr8_bit_hash = NULL;
    arr_tests->internal_arr16_bit_hash = NULL;
    arr_tests->internal_arr32_bit_hash = NULL;
    arr_tests->internal_arr64_bit_hash = NULL;
};



//initializes the tests
void assign_tests(arr_test test){
    // printf("%"PRIu32"\n",(sizeof(uint8_t)*BYTE_BITS+1));
    // printf("%"PRIu32"\n",(sizeof(uint16_t)*BYTE_BITS+1));
    // printf("%"PRIu32"\n",(sizeof(uint32_t)*BYTE_BITS+1));
    // printf("%"PRIu32"\n",(sizeof(uint64_t)*BYTE_BITS+1));

   
    memset(arr8_bit_hash,0,  sizeof(uint64_t)*(sizeof(uint8_t)*BYTE_BITS+1));
    memset(arr16_bit_hash,0,  sizeof(uint64_t)*(sizeof(uint16_t)*BYTE_BITS+1));
    memset(arr32_bit_hash,0,  sizeof(uint64_t)*(sizeof(uint32_t)*BYTE_BITS+1));
    memset(arr64_bit_hash,0,  sizeof(uint64_t)*(sizeof(uint64_t)*BYTE_BITS+1));

    uint64_t seed = rand_u_long();
    // creates no correlation dataset of random ints
    for(int i =0; i<n; i++) {
        if(normalDist){
            arr8[i] = generate_custom_normal_bits(BYTE_BITS);
            arr16[i] = generate_custom_normal_bits((sizeof(uint16_t)*BYTE_BITS)/2);
            arr32[i] = generate_custom_normal_bits((sizeof(uint32_t)*BYTE_BITS)/4);
            arr64[i] = generate_custom_normal_bits((sizeof(uint64_t)*BYTE_BITS)/8);
        } else {
            arr8[i] = splitmix64(&seed)%UINT8_MAX;
            arr16[i] = splitmix64(&seed)%UINT16_MAX;
            arr32[i] = splitmix64(&seed)%UINT32_MAX;
            arr64[i] = splitmix64(&seed)%UINT64_MAX;
        }

        if(arr8[i] == 0){
            arr8_bit_hash[0]++; 
        } else {
            arr8_bit_hash[(int)ilog2_32(arr8[i])+1]++;
        }

        if(arr16[i] == 0){
            arr16_bit_hash[0]++;
        } else {
            arr16_bit_hash[(int)ilog2_32(arr16[i])+1]++;
        }

        if(arr32[i] == 0){
            arr32_bit_hash[0]++;
        } else {
            arr32_bit_hash[(int)ilog2_32(arr32[i])+1]++;
        }

        if(arr64[i] == 0){
            arr64_bit_hash[0]++;
        } else {
            arr64_bit_hash[(int)ilog2_64(arr64[i])+1]++;
        }
    };
    sort_arr_test(test);


};

double min(const double a, const double b){
    if(a<=b){ 
        return a;
    }
    return b;
}

double max(const double a, const double b){
    if(a>=b){ 
        return a;
    }
    return b;
}

/*
n : number of tables
i: number of repetitions to get an average 

*/

int main(int argc, char* argv[]){
    if (argc < 3){
            perror("needs an n argument and i\n");
            return 0;
    }
    char *end_argv;
    normalDist = (argc == 4 && !strcmp(argv[3], "1"));
    n = strtoull(argv[1], &end_argv, 10); // number of elements per array
    uint64_t i_iter  = strtoull(argv[2], &end_argv, 10); // number of repetitions of thee 4 arrays




    srand(time(NULL));

    arr_test no_corr = (arr_test){
            malloc(n*sizeof(uint8_t)), // n bytes
            malloc(n*sizeof(uint16_t)),// 2*n bytes
            malloc(n*sizeof(uint32_t)),// 4*n bytes
            malloc(n*sizeof(uint64_t)),// 8*n bytes
            calloc((sizeof(uint8_t)*BYTE_BITS+1),sizeof(uint64_t)), 
            calloc((sizeof(uint16_t)*BYTE_BITS+1),sizeof(uint64_t)),
            calloc((sizeof(uint32_t)*BYTE_BITS+1),sizeof(uint64_t)),
            calloc((sizeof(uint64_t)*BYTE_BITS+1),sizeof(uint64_t)) 

    };
    assign_curr_tests(no_corr);



    // compress objects
    bcp_obj_bytes obj8;
    bcp_obj_bytes obj16; 
    bcp_obj_bytes obj32;
    bcp_obj_bytes obj64;

    // decompress arrays 
     uint8_t* arr8_decomp = NULL;
     uint16_t* arr16_decomp = NULL;
     uint32_t* arr32_decomp = NULL;
     uint64_t* arr64_decomp = NULL;

    struct timespec start, end; 
    double compress_time[] = {0,0,0,0}; // 8 , 16, 32, and 64 bits
    double decompress_time[] = {0,0,0,0}; // 8 , 16, 32, and 64 bits
    double compress_maxes[] = {0,0,0,0};
    double compress_ratio[] = {0,0,0,0};
    double space_savings[] = {0,0,0,0};
    double decompress_maxes[] = {0,0,0,0};
    double decompress_mins[] = {DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX};
    double compress_mins[] = {DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX};
    
    assign_tests(no_corr); // reassigns all tests
    
    void* arr[] = {
        (void*)arr8,
        (void*)arr16,
        (void*)arr32,
        (void*)arr64,
    };

    uint64_t* arr_bit_hashes[] = {
        arr8_bit_hash,
        arr16_bit_hash,
        arr32_bit_hash,
        arr64_bit_hash
    };

    size_t sizes[] = {
        (size_t)sizeof(uint8_t),
        (size_t)sizeof(uint16_t),
        (size_t)sizeof(uint32_t),
        (size_t)sizeof(uint64_t)
    };
    
    // WARMUP 
    for(uint64_t i=0; i<4; i++){
        bcp_obj_bytes obj_buffer;
        void* arr_buffer;
        obj_buffer = bcp_compress_bytes(arr[i],arr_bit_hashes[i], n, (size_t)sizes[i]);

        arr_buffer = bcp_arr(obj_buffer.obj,(size_t)sizes[i]);
        DO_NOT_OPTIMIZE(obj_buffer);
        DO_NOT_OPTIMIZE(arr_buffer);
        free_arr(arr_buffer);
        free_bcp_bytes(obj_buffer);
        arr_buffer = NULL;
    }
    
    


    // MEASUREMENT
    for (uint64_t i = 0; i < i_iter; i++)
    {
        
        for(uint64_t j = 0; j<4; j++){
            // 8 bit benchmarking 
            bcp_obj_bytes obj_buffer;
            void* arr_buffer;
            clock_gettime(CLOCK_MONOTONIC, &start);
            obj_buffer = bcp_compress_bytes(arr[j],arr_bit_hashes[j], n, sizes[j]);  // compression
            clock_gettime(CLOCK_MONOTONIC, &end);
            compress_time[j] += S_END;
            compress_ratio[j] += sizes[j] * n / (double)obj_buffer.size;
            space_savings[j] += (1 - 1/compress_ratio[j] )*100;

            clock_gettime(CLOCK_MONOTONIC, &start);
            arr_buffer = bcp_arr(obj_buffer.obj,sizes[j]); // decompression
            clock_gettime(CLOCK_MONOTONIC, &end);
            decompress_time[j] += S_END;

            compress_maxes[j] = max(compress_maxes[j],S_END);
            compress_mins[j] = min(compress_mins[j], S_END);
            decompress_maxes[j] = max(decompress_maxes[j],S_END);
            decompress_mins[j] = min(decompress_mins[j], S_END);

            DO_NOT_OPTIMIZE(obj_buffer);
            DO_NOT_OPTIMIZE(arr_buffer);
            free_arr(arr_buffer);
            free_bcp_bytes(obj_buffer);
            arr_buffer = NULL;

        }

    }

    // gets thee compression and decompression times

    

    printf("%"PRIu64" repetitions",i_iter);
    if(i_iter>2){
        printf("\t(-2 elements, max and min outliers dropped)");
    };
    printf("\n");
    for(int i = 0; i<4; i++){
        if(i_iter>2){
            compress_time[i] -= compress_maxes[i];
            compress_time[i] -= compress_mins[i];
            decompress_time[i] -= decompress_maxes[i];
            decompress_time[i] -= decompress_mins[i];
            compress_time[i] /= (i_iter-2);
        } else {
            compress_time[i] /= (i_iter);
            decompress_time[i] /= (i_iter);

        }
        double bytes = pow(2,i);
        double mb = (bytes*n) / (1024.0 * 1024.0); // mb
        double avg_comp_mbps = (mb) / compress_time[i];
        printf("%.0fb | %.2fmb |  AVG Compression:   %.6f sec | Throughput: %.2f MB/s\n", bytes, mb,compress_time[i], avg_comp_mbps);

    }
    for(int i = 0; i<4; i++){
        double bytes = pow(2,i);
        double mb = (bytes*n) / (1024.0 * 1024.0);
        double avg_comp_mbps = (mb) / decompress_time[i];
        printf("%.0fb | %.2fmb |  AVG Decompression:   %.6f sec | Throughput: %.2f MB/s\n", bytes, mb,decompress_time[i], avg_comp_mbps);

    }
    

    delete_test(&no_corr);
    return 0;
}




