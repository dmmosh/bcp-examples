#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <bcp.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <unistd.h>
#include <stdarg.h>
uint64_t n;
bool normalDist;

// Macro to prevent a value from being optimized away
#define DO_NOT_OPTIMIZE(var) __asm__ volatile("" : "+g" (var))

#define S_END (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1e9

#define COMPRESS 0  // macros apply to all structs
#define DECOMPRESS 1
#define RATIO 2
#define SPACE 3
#define COMPRESS_SIZE 4  
#define DATA_SIZE 5 // the number of data, 5 
#define PD(x) printf("%0.10lf\n",x)

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

uint64_t mean[4] = {0,0,0,0};
double std[4] = {0,0,0,0}; 

void throw_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Error: "); // Print to standard error stream
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(EXIT_FAILURE); // Terminate the program
}

static inline uint64_t splitmix64(uint64_t *s) {
    uint64_t z = (*s += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

uint64_t splitmix64_range(uint64_t *s, uint64_t min, uint64_t max) {
    if (min == 0 && max == UINT64_MAX) {
        return splitmix64(s);
    }

    uint64_t range_m1 = max - min;
    uint64_t range = range_m1 + 1;

    if ((range & range_m1) == 0) {
        return (splitmix64(s) & range_m1) + min;
    }

    uint64_t limit = (uint64_t)-(int64_t)range % range;
    uint64_t res;

    do {
        res = splitmix64(s);
    } while (res < limit);

    return (res % range) + min;
}

double generate_gaussian(double mean, double std) {
    // 1. Get two uniform random doubles in range (0, 1]
    // We avoid 0 to prevent log(0) which is undefined
    double u1 = (rand() + 1.0) / (RAND_MAX + 1.0);
    double u2 = (rand() + 1.0) / (RAND_MAX + 1.0);

    // 2. Apply Box-Muller Transform
    double z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);

    // 3. Scale and shift to the desired mean and standard deviation
    return z * std + mean;
}

uint64_t get_random_uint64_gauss(double mean, double std, uint64_t min, uint64_t max) {
    double result = generate_gaussian(mean, std);

    // 4. Safety: Clip to 0 to avoid negative wraparound in uint64
    if (result < 0) return 0;
    if ((uint64_t)result < min){
        return min;
    }
    if((uint64_t)result>max){
        return max;
    }
    
    // 5. Cast to uint64_t
    return (uint64_t)result;
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

// changes the curr pointer
uint64_t read_buffer(arr_placeholder** curr, size_t size){
    uint64_t buffer = 0; // the buffer to store the value in
    for (uint64_t a = 0; a < size; a++) // iterates through the buffer
    {
        buffer |= ((uint64_t)(*curr)[a]) <<(8*a); // stores in buffer
    }
    *curr += size;
    return buffer;
}

double std_random(const double min, const double max){
    return (double)(min + (rand() / (double)RAND_MAX) * (max - min));
}


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
    P64(seed);
    if(normalDist){
    
        // the means are randomly generated in a range of (previous bit group, max bit of current]
        // after all, if space is limited, you typically use the large enough data sizes only if the data requires it 
        uint64_t bits = get_random_uint64_gauss(4,1,1,8);
        mean[0] = splitmix64_range(&seed,1<<bits,(1<<(bits+1))-1);
        bits = get_random_uint64_gauss(8,2,1,16);
        mean[1] = splitmix64_range(&seed,1<<bits,(1<<(bits+1))-1);
        bits = get_random_uint64_gauss(16,4,1,32);
        mean[2] = splitmix64_range(&seed,1<<bits,(1<<(bits+1))-1);
        bits = get_random_uint64_gauss(32,8,1,64);
        mean[3] = splitmix64_range(&seed,1<<bits,((bits==64)? UINT64_MAX : (1<<(bits+1))-1));
        std[0] = pow(2,std_random(4,7));
        std[1] = pow(2,std_random(8,15));
        std[2] = pow(2,std_random(16,31));
        std[3] = 10000000;

        PD(std[0]);
        PD(std[1]);
        PD(std[2]);
        PD(std[3]);
        P64(mean[0]);
        P64(mean[1]);
        P64(mean[2]);
        P64(mean[3]);
    }

    for(int i =0; i<n; i++) { 
        if(normalDist){
            
            arr8[i] = get_random_uint64_gauss((double)mean[0], std[0],0,UINT8_MAX) & ~(uint8_t)0;
            arr16[i] = get_random_uint64_gauss((double)mean[1],std[1],0,UINT16_MAX) & ~(uint16_t)0;
            arr32[i] = get_random_uint64_gauss((double)mean[2],std[2],0,UINT32_MAX)& ~(uint32_t)0;
            arr64[i] = get_random_uint64_gauss((double)mean[3],std[3],0,UINT64_MAX);
        } else {
            arr8[i] = splitmix64(&seed) & (1ULL << 8)-1;
            arr16[i] =splitmix64(&seed) & (1ULL << 16)-1;
            arr32[i] =splitmix64(&seed) & (1ULL << 32)-1;
            arr64[i] =3216;
        }

        // adds values to the running mean 


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

    srand((unsigned int)time(NULL));



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


    struct timespec start, end; 





    double data[][4] = { // the data, all are sums. will be divided by n by the end
        {0,0,0,0}, // compression time
        {0,0,0,0}, // decompression time
        {0,0,0,0}, // ratio   
        {0,0,0,0}, // space savings
        {0,0,0,0} // compress size
    };
    double maxes[][4] ={ // the maxes
        {0,0,0,0}, // max compression ( based on this, other values are simply to be subbed alongside)
        {0,0,0,0}, // max decompression ( based on this, other values are simply to be subbed alongside)
        {0,0,0,0}, // max ratio   
        {0,0,0,0}, // max space savings
        {0,0,0,0} // max compression time
    };
    double mins[][4] ={ // the mins
        {DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX}, // mins compression ( based on this, other values are simply to be subbed alongside)
        {DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX}, // mins decompression ( based on this, other values are simply to be subbed alongside)
        {DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX}, // mins ratio   
        {DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX}, // mins space savings
        {DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX} // mins compression time
    };

    
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

        arr_placeholder* arr_buffer_reader = (arr_placeholder*)arr_buffer;
        arr_placeholder* arr_test_reader = (arr_placeholder*)arr[i];

        // check the decompressedd values match the original values  
        for(uint64_t j=0; j<n; j++){ // iterates over the values and checks them
            uint64_t original = read_buffer(&arr_test_reader,sizes[i]);
            uint64_t decompressed  = read_buffer(&arr_buffer_reader,sizes[i]);
            if(decompressed != original){
                free_arr(arr_buffer);
                free_bcp_bytes(obj_buffer);
                arr_buffer = NULL;
                throw_error("[ERROR]\t value decompression mismatch\t\toriginal: %"PRIu64"\tdecompressed: %"PRIu64"\n", original, decompressed);
            }
        }

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
            // benchmarking
            // S_end gets the current time
            double data_curr[DATA_SIZE] = {0,0,0,0,0}; // the data , will store it here
            bcp_obj_bytes obj_buffer; // the object buffer
            void* arr_buffer; // the array buffer

            // run compression
            clock_gettime(CLOCK_MONOTONIC, &start);
            obj_buffer = bcp_compress_bytes(arr[j],arr_bit_hashes[j], n, sizes[j]);  // compression
            clock_gettime(CLOCK_MONOTONIC, &end);

            // store metadata
            data_curr[COMPRESS] = S_END; // the time for compression
            data_curr[COMPRESS_SIZE] = (double)obj_buffer.size; // the size of compressed file
            data_curr[RATIO] = (double)sizes[j] * n / (double)obj_buffer.size; // the compression ratio
            data_curr[SPACE] = (double)1-((double)obj_buffer.size/(sizes[j] * n )); // the space savings

            // run decompression
            clock_gettime(CLOCK_MONOTONIC, &start);
            arr_buffer = bcp_arr(obj_buffer.obj,sizes[j]); // decompression
            clock_gettime(CLOCK_MONOTONIC, &end);
            
            data_curr[DECOMPRESS] = S_END; // store decompression
            // run number validation (decompressed vs original)
            arr_placeholder* arr_buffer_reader = (arr_placeholder*)arr_buffer;
            arr_placeholder* arr_test_reader = (arr_placeholder*)arr[j];

            // check the decompressedd values match the original values  
            for(uint64_t i=0; i<n; i++){ // iterates over the values and checks them
                uint64_t original = read_buffer(&arr_test_reader,sizes[j]);
                uint64_t decompressed  = read_buffer(&arr_buffer_reader,sizes[j]);
                if(decompressed != original){ // to do : triggers this somehow
                    bcp_print_values(obj_buffer.obj);
                    free_arr(arr_buffer);
                    free_bcp_bytes(obj_buffer);
                    arr_buffer = NULL;
                    throw_error("[ERROR]\t value decompression mismatch\t\toriginal: %"PRIu64"\tdecompressed: %"PRIu64"\n", original, decompressed);
                }
            } // validation passed
                


            // store the data 
            for(uint64_t a=0; a<DATA_SIZE; a++){
                data[a][j] += data_curr[a];
                mins[a][j] = min(mins[a][j], data_curr[a]);
                maxes[a][j] = max(mins[a][j], data_curr[a]);
            }
            
            // clean up
            DO_NOT_OPTIMIZE(obj_buffer); 
            DO_NOT_OPTIMIZE(arr_buffer);
            free_arr(arr_buffer);
            free_bcp_bytes(obj_buffer);
            arr_buffer = NULL;

        }

    }

    // gets thee compression and decompression times

    
    printf("%"PRIu64" repetitions, averages", i_iter);
    if(i_iter>2){
        printf("\t(%"PRIu64", max and min outliers dropped)",i_iter-2);
    };
    if(normalDist){
        printf("\nValues follow a normal distribution with reasonable stds.");
    } else {
        printf("\nValues are randomly generated (worst-case).");
    }
    printf("\nDecompressed values match the source.\n\values\n");

    for(int i = 0; i<4; i++){
        // drops the outliers and gets the averages 
        // convenient to have it here

        for(int a = 0; a<DATA_SIZE; a++){
            if(i_iter>2){
                data[a][i] -=maxes[a][i]; // drops the max outlier from the  average count
                data[a][i] -=mins[a][i]; // drops the min  outlier from the  average count
                data[a][i] /= (i_iter-2);
            } else {
                data[a][i] /= (i_iter);
            }
        }
        printf("%zub | Mean: %"PRIu64" | Avg. bit num: %"PRIu64"",
            sizes[i],
            mean[i],
            ilog2_64(mean[i])+1
        );
        if(normalDist){
            //printf(" | Standard deviation: %")
        }

        printf("\n");

    }

    printf("\tcompression\n");
    for(int i = 0; i<4; i++){

        double bytes = (double)sizes[i];
        double kb = (bytes*n) / 1024.0;
        double avg_comp_mbps = (kb/1024) / data[COMPRESS][i];
        printf("%.0fb | Size: %.2fkb | C.Ratio: %.2f | Space savings: %.2f\% | Time: %.6f sec | Throughput: %.2f MB/s\n",
             bytes,
             data[COMPRESS_SIZE][i] ,
             data[RATIO][i],
             data[SPACE][i] *100,
             data[COMPRESS][i],
             avg_comp_mbps
            );

    }
    printf("\t decompression\n");
    for(int i = 0; i<4; i++){
        double bytes = (double)sizes[i];
        double kb = (bytes*n) / 1024.0;
        double avg_comp_mbps = (kb/1024) / data[DECOMPRESS][i];
        printf("%.0fb | Size: %.2fkb | Time: %.6f sec | Throughput: %.2f MB/s\n",
             bytes,
             kb,
             data[DECOMPRESS][i], 
             avg_comp_mbps
            );

    }
    

    delete_test(&no_corr);
    return 0;
}




