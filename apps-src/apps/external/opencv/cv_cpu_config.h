#if defined(_WIN32)
// OpenCV CPU baseline features

#define CV_CPU_COMPILE_SSE 1
#define CV_CPU_BASELINE_COMPILE_SSE 1

#define CV_CPU_COMPILE_SSE2 1
#define CV_CPU_BASELINE_COMPILE_SSE2 1

#define CV_CPU_COMPILE_SSE3 1
#define CV_CPU_BASELINE_COMPILE_SSE3 1

#define CV_CPU_BASELINE_FEATURES 0 \
    , CV_CPU_SSE \
    , CV_CPU_SSE2 \
    , CV_CPU_SSE3 \


// OpenCV supported CPU dispatched features

#define CV_CPU_DISPATCH_COMPILE_SSE4_1 1
#define CV_CPU_DISPATCH_COMPILE_SSE4_2 1
#define CV_CPU_DISPATCH_COMPILE_FP16 1
#define CV_CPU_DISPATCH_COMPILE_AVX 1
#define CV_CPU_DISPATCH_COMPILE_AVX2 1
#define CV_CPU_DISPATCH_COMPILE_AVX512_SKX 1


#define CV_CPU_DISPATCH_FEATURES 0 \
    , CV_CPU_SSE4_1 \
    , CV_CPU_SSE4_2 \
    , CV_CPU_FP16 \
    , CV_CPU_AVX \
    , CV_CPU_AVX2 \
    , CV_CPU_AVX512_SKX \

// #define CV_CPU_COMPILE_AVX 1
// #define CV_CPU_COMPILE_AVX2 1
// #define CV_CPU_COMPILE_AVX_512F 1
// #define CV_CPU_COMPILE_AVX512_COMMON 1
// #define CV_CPU_COMPILE_AVX512_SKX 1

#else

// OpenCV CPU baseline features

#define CV_CPU_COMPILE_NEON 1
#define CV_CPU_BASELINE_COMPILE_NEON 1

#define CV_CPU_BASELINE_FEATURES 0 \
    , CV_CPU_NEON \


// OpenCV supported CPU dispatched features



#define CV_CPU_DISPATCH_FEATURES 0 \

#endif