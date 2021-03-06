#include <iostream>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "buffer.h"
#include "ioctl_cmds.h"

#ifndef HALIDE_ATTRIBUTE_ALIGN
  #ifdef _MSC_VER
    #define HALIDE_ATTRIBUTE_ALIGN(x) __declspec(align(x))
  #else
    #define HALIDE_ATTRIBUTE_ALIGN(x) __attribute__((aligned(x)))
  #endif
#endif
#ifndef BUFFER_T_DEFINED
#define BUFFER_T_DEFINED
#include <stdbool.h>
#include <stdint.h>
typedef struct buffer_t {
    uint64_t dev;
    uint8_t* host;
    int32_t extent[4];
    int32_t stride[4];
    int32_t min[4];
    int32_t elem_size;
    HALIDE_ATTRIBUTE_ALIGN(1) bool host_dirty;
    HALIDE_ATTRIBUTE_ALIGN(1) bool dev_dirty;
    HALIDE_ATTRIBUTE_ALIGN(1) uint8_t _padding[10 - sizeof(void *)];
} buffer_t;
#endif
struct halide_filter_metadata_t;
extern "C" {
void *halide_malloc(void *ctx, size_t);
void halide_free(void *ctx, void *ptr);
void *halide_print(void *ctx, const void *str);
void *halide_error(void *ctx, const void *str);
int halide_debug_to_file(void *ctx, const char *filename, void *data, int, int, int, int, int, int);
int halide_start_clock(void *ctx);
int64_t halide_current_time_ns(void *ctx);
void halide_profiler_pipeline_end(void *, void *);
}

#ifdef _WIN32
float roundf(float);
double round(double);
#else
inline float asinh_f32(float x) {return asinhf(x);}
inline float acosh_f32(float x) {return acoshf(x);}
inline float atanh_f32(float x) {return atanhf(x);}
inline double asinh_f64(double x) {return asinh(x);}
inline double acosh_f64(double x) {return acosh(x);}
inline double atanh_f64(double x) {return atanh(x);}
#endif
inline float sqrt_f32(float x) {return sqrtf(x);}
inline float sin_f32(float x) {return sinf(x);}
inline float asin_f32(float x) {return asinf(x);}
inline float cos_f32(float x) {return cosf(x);}
inline float acos_f32(float x) {return acosf(x);}
inline float tan_f32(float x) {return tanf(x);}
inline float atan_f32(float x) {return atanf(x);}
inline float sinh_f32(float x) {return sinhf(x);}
inline float cosh_f32(float x) {return coshf(x);}
inline float tanh_f32(float x) {return tanhf(x);}
inline float hypot_f32(float x, float y) {return hypotf(x, y);}
inline float exp_f32(float x) {return expf(x);}
inline float log_f32(float x) {return logf(x);}
inline float pow_f32(float x, float y) {return powf(x, y);}
inline float floor_f32(float x) {return floorf(x);}
inline float ceil_f32(float x) {return ceilf(x);}
inline float round_f32(float x) {return roundf(x);}

inline double sqrt_f64(double x) {return sqrt(x);}
inline double sin_f64(double x) {return sin(x);}
inline double asin_f64(double x) {return asin(x);}
inline double cos_f64(double x) {return cos(x);}
inline double acos_f64(double x) {return acos(x);}
inline double tan_f64(double x) {return tan(x);}
inline double atan_f64(double x) {return atan(x);}
inline double sinh_f64(double x) {return sinh(x);}
inline double cosh_f64(double x) {return cosh(x);}
inline double tanh_f64(double x) {return tanh(x);}
inline double hypot_f64(double x, double y) {return hypot(x, y);}
inline double exp_f64(double x) {return exp(x);}
inline double log_f64(double x) {return log(x);}
inline double pow_f64(double x, double y) {return pow(x, y);}
inline double floor_f64(double x) {return floor(x);}
inline double ceil_f64(double x) {return ceil(x);}
inline double round_f64(double x) {return round(x);}

inline float nan_f32() {return NAN;}
inline float neg_inf_f32() {return -INFINITY;}
inline float inf_f32() {return INFINITY;}
inline bool is_nan_f32(float x) {return x != x;}
inline bool is_nan_f64(double x) {return x != x;}
inline float float_from_bits(uint32_t bits) {
 union {
  uint32_t as_uint;
  float as_float;
 } u;
 u.as_uint = bits;
 return u.as_float;
}
inline int64_t make_int64(int32_t hi, int32_t lo) {
    return (((int64_t)hi) << 32) | (uint32_t)lo;
}
inline double make_float64(int32_t i0, int32_t i1) {
    union {
        int32_t as_int32[2];
        double as_double;
    } u;
    u.as_int32[0] = i0;
    u.as_int32[1] = i1;
    return u.as_double;
}

template<typename T> T max(T a, T b) {if (a > b) return a; return b;}
template<typename T> T min(T a, T b) {if (a < b) return a; return b;}
template<typename A, typename B> A reinterpret(B b) {A a; memcpy(&a, &b, sizeof(a)); return a;}

static bool halide_rewrite_buffer(buffer_t *b, int32_t elem_size,
                           int32_t min0, int32_t extent0, int32_t stride0,
                           int32_t min1, int32_t extent1, int32_t stride1,
                           int32_t min2, int32_t extent2, int32_t stride2,
                           int32_t min3, int32_t extent3, int32_t stride3) {
 b->min[0] = min0;
 b->min[1] = min1;
 b->min[2] = min2;
 b->min[3] = min3;
 b->extent[0] = extent0;
 b->extent[1] = extent1;
 b->extent[2] = extent2;
 b->extent[3] = extent3;
 b->stride[0] = stride0;
 b->stride[1] = stride1;
 b->stride[2] = stride2;
 b->stride[3] = stride3;
 return true;
}
#ifndef HALIDE_FUNCTION_ATTRS
#define HALIDE_FUNCTION_ATTRS
#endif
#ifdef __cplusplus
extern "C" {
#endif

int32_t halide_error_bad_elem_size(void *, const char *, const char *, int32_t, int32_t);
int32_t halide_error_access_out_of_bounds(void *, const char *, int32_t, int32_t, int32_t, int32_t, int32_t);
int32_t halide_error_constraint_violated(void *, const char *, int32_t, const char *, int32_t);
int32_t halide_error_buffer_allocation_too_large(void *, const char *, int64_t, int64_t);
int32_t halide_error_buffer_extents_too_large(void *, const char *, int64_t, int64_t);
int32_t halide_error_explicit_bounds_too_small(void *, const char *, const char *, int32_t, int32_t, int32_t, int32_t);

static int slice_buffer(Buffer* src, Buffer* des, unsigned int x, unsigned int y, unsigned int width, unsigned int height)
  {
    if (width == 0 || height == 0) {
      printf("slice_buffer failed: width and height of slide should be non-zero.\n");
      return -1;
    }
    if (x + width > src->width || y + height > src->height) {
      printf("slice_buffer failed: slice is out of range.\n");
      return -1;
    }
    *des = *src; // copy depth, stride, data, etc.
    des->width = width;
    des->height = height;
    des->phys_addr += src->depth * (y * src->stride + x);
    des->mmap_offset += src->depth * (y * src->stride + x);
    // Kernel address too?
    return 0;
}

static int __pipeline_hls(buffer_t *_p1_buffer, buffer_t *_output_2_buffer) HALIDE_FUNCTION_ATTRS {
  // Open the hardware device
  int hwacc = open("/dev/hwacc0", O_RDWR);
  if(hwacc == -1){
    printf("Failed to open hardware device!\n");
    return(0);
  }

  struct timeval t1, t2;
  double elapsedTime = 0.0;

 uint8_t *_p1 = (uint8_t *)(_p1_buffer->host);
 (void)_p1;
 const bool _p1_host_and_dev_are_null = (_p1_buffer->host == NULL) && (_p1_buffer->dev == 0);
 (void)_p1_host_and_dev_are_null;
 const int32_t _p1_min_0 = _p1_buffer->min[0];
 (void)_p1_min_0;
 const int32_t _p1_min_1 = _p1_buffer->min[1];
 (void)_p1_min_1;
 const int32_t _p1_min_2 = _p1_buffer->min[2];
 (void)_p1_min_2;
 const int32_t _p1_min_3 = _p1_buffer->min[3];
 (void)_p1_min_3;
 const int32_t _p1_extent_0 = _p1_buffer->extent[0];
 (void)_p1_extent_0;
 const int32_t _p1_extent_1 = _p1_buffer->extent[1];
 (void)_p1_extent_1;
 const int32_t _p1_extent_2 = _p1_buffer->extent[2];
 (void)_p1_extent_2;
 const int32_t _p1_extent_3 = _p1_buffer->extent[3];
 (void)_p1_extent_3;
 const int32_t _p1_stride_0 = _p1_buffer->stride[0];
 (void)_p1_stride_0;
 const int32_t _p1_stride_1 = _p1_buffer->stride[1];
 (void)_p1_stride_1;
 const int32_t _p1_stride_2 = _p1_buffer->stride[2];
 (void)_p1_stride_2;
 const int32_t _p1_stride_3 = _p1_buffer->stride[3];
 (void)_p1_stride_3;
 const int32_t _p1_elem_size = _p1_buffer->elem_size;
 (void)_p1_elem_size;
 uint8_t *_output_2 = (uint8_t *)(_output_2_buffer->host);
 (void)_output_2;
 const bool _output_2_host_and_dev_are_null = (_output_2_buffer->host == NULL) && (_output_2_buffer->dev == 0);
 (void)_output_2_host_and_dev_are_null;
 const int32_t _output_2_min_0 = _output_2_buffer->min[0];
 (void)_output_2_min_0;
 const int32_t _output_2_min_1 = _output_2_buffer->min[1];
 (void)_output_2_min_1;
 const int32_t _output_2_min_2 = _output_2_buffer->min[2];
 (void)_output_2_min_2;
 const int32_t _output_2_min_3 = _output_2_buffer->min[3];
 (void)_output_2_min_3;
 const int32_t _output_2_extent_0 = _output_2_buffer->extent[0];
 (void)_output_2_extent_0;
 const int32_t _output_2_extent_1 = _output_2_buffer->extent[1];
 (void)_output_2_extent_1;
 const int32_t _output_2_extent_2 = _output_2_buffer->extent[2];
 (void)_output_2_extent_2;
 const int32_t _output_2_extent_3 = _output_2_buffer->extent[3];
 (void)_output_2_extent_3;
 const int32_t _output_2_stride_0 = _output_2_buffer->stride[0];
 (void)_output_2_stride_0;
 const int32_t _output_2_stride_1 = _output_2_buffer->stride[1];
 (void)_output_2_stride_1;
 const int32_t _output_2_stride_2 = _output_2_buffer->stride[2];
 (void)_output_2_stride_2;
 const int32_t _output_2_stride_3 = _output_2_buffer->stride[3];
 (void)_output_2_stride_3;
 const int32_t _output_2_elem_size = _output_2_buffer->elem_size;
 (void)_output_2_elem_size;
 int32_t _2 = _p1_min_0 + _p1_extent_0;
 int32_t _3 = _2 + -1;
 int32_t _4 = min(_3, 1043);
 int32_t _5 = max(_4, _p1_min_0);
 int32_t _6 = max(_5, 1023);
 int32_t _7 = min(_3, -20);
 int32_t _8 = max(_7, _p1_min_0);
 int32_t _9 = min(_8, 0);
 int32_t _10 = _6 - _9;
 int32_t _11 = _p1_min_1 + _p1_extent_1;
 int32_t _12 = _11 + -1;
 int32_t _13 = min(_12, 1043);
 int32_t _14 = max(_13, _p1_min_1);
 int32_t _15 = max(_14, 1023);
 int32_t _16 = min(_12, -20);
 int32_t _17 = max(_16, _p1_min_1);
 int32_t _18 = min(_17, 0);
 int32_t _19 = _15 - _18;
 if (_output_2_host_and_dev_are_null)
 {
  bool _20 = halide_rewrite_buffer(_output_2_buffer, 1, 0, 1024, 1, 0, 1024, 1024, 0, 0, 0, 0, 0, 0);
  (void)_20;
 } // if _output_2_host_and_dev_are_null
 if (_p1_host_and_dev_are_null)
 {
  int32_t _21 = _10 + 1;
  int32_t _22 = _19 + 1;
  bool _23 = halide_rewrite_buffer(_p1_buffer, 1, _9, _21, 1, _18, _22, _21, 0, 0, 0, 0, 0, 0);
  (void)_23;
 } // if _p1_host_and_dev_are_null
 bool _24 = _output_2_host_and_dev_are_null || _p1_host_and_dev_are_null;
 bool _25 = !(_24);
 if (_25)
 {
  bool _26 = _output_2_elem_size == 1;
  if (!_26)   {
   int32_t _27 = halide_error_bad_elem_size(NULL, "Output buffer output$2", "uint8", _output_2_elem_size, 1);
   return _27;
  }
  bool _28 = _p1_elem_size == 1;
  if (!_28)   {
   int32_t _29 = halide_error_bad_elem_size(NULL, "Input buffer p1", "uint8", _p1_elem_size, 1);
   return _29;
  }
  bool _30 = _output_2_min_0 <= 0;
  int32_t _31 = 1024 - _output_2_extent_0;
  bool _32 = _31 <= _output_2_min_0;
  bool _33 = _30 && _32;
  if (!_33)   {
   int32_t _34 = _output_2_min_0 + _output_2_extent_0;
   int32_t _35 = _34 + -1;
   int32_t _36 = halide_error_access_out_of_bounds(NULL, "Output buffer output$2", 0, 0, 1023, _output_2_min_0, _35);
   return _36;
  }
  bool _37 = _output_2_min_1 <= 0;
  int32_t _38 = 1024 - _output_2_extent_1;
  bool _39 = _38 <= _output_2_min_1;
  bool _40 = _37 && _39;
  if (!_40)   {
   int32_t _41 = _output_2_min_1 + _output_2_extent_1;
   int32_t _42 = _41 + -1;
   int32_t _43 = halide_error_access_out_of_bounds(NULL, "Output buffer output$2", 1, 0, 1023, _output_2_min_1, _42);
   return _43;
  }
  bool _44 = _p1_min_0 <= _9;
  int32_t _45 = _9 + _10;
  int32_t _46 = _45 - _p1_extent_0;
  int32_t _47 = _46 + 1;
  bool _48 = _47 <= _p1_min_0;
  bool _49 = _44 && _48;
  if (!_49)   {
   int32_t _50 = _9 + _10;
   int32_t _51 = _p1_min_0 + _p1_extent_0;
   int32_t _52 = _51 + -1;
   int32_t _53 = halide_error_access_out_of_bounds(NULL, "Input buffer p1", 0, _9, _50, _p1_min_0, _52);
   return _53;
  }
  bool _54 = _p1_min_1 <= _18;
  int32_t _55 = _18 + _19;
  int32_t _56 = _55 - _p1_extent_1;
  int32_t _57 = _56 + 1;
  bool _58 = _57 <= _p1_min_1;
  bool _59 = _54 && _58;
  if (!_59)   {
   int32_t _60 = _18 + _19;
   int32_t _61 = _p1_min_1 + _p1_extent_1;
   int32_t _62 = _61 + -1;
   int32_t _63 = halide_error_access_out_of_bounds(NULL, "Input buffer p1", 1, _18, _60, _p1_min_1, _62);
   return _63;
  }
  bool _64 = _output_2_stride_0 == 1;
  if (!_64)   {
   int32_t _65 = halide_error_constraint_violated(NULL, "output$2.stride.0", _output_2_stride_0, "1", 1);
   return _65;
  }
  bool _66 = _p1_stride_0 == 1;
  if (!_66)   {
   int32_t _67 = halide_error_constraint_violated(NULL, "p1.stride.0", _p1_stride_0, "1", 1);
   return _67;
  }
  int64_t _68 = (int64_t)(_output_2_extent_1);
  int64_t _69 = (int64_t)(_output_2_extent_0);
  int64_t _70 = _68 * _69;
  int64_t _71 = (int64_t)(_p1_extent_1);
  int64_t _72 = (int64_t)(_p1_extent_0);
  int64_t _73 = _71 * _72;
  int64_t _74 = (int64_t)(2147483647);
  bool _75 = _69 <= _74;
  if (!_75)   {
   int64_t _76 = (int64_t)(_output_2_extent_0);
   int64_t _77 = (int64_t)(2147483647);
   int32_t _78 = halide_error_buffer_allocation_too_large(NULL, "output$2", _76, _77);
   return _78;
  }
  int64_t _79 = (int64_t)(_output_2_extent_1);
  int64_t _80 = (int64_t)(_output_2_stride_1);
  int64_t _81 = _79 * _80;
  int64_t _82 = (int64_t)(2147483647);
  bool _83 = _81 <= _82;
  if (!_83)   {
   int64_t _84 = (int64_t)(_output_2_extent_1);
   int64_t _85 = (int64_t)(_output_2_stride_1);
   int64_t _86 = _84 * _85;
   int64_t _87 = (int64_t)(2147483647);
   int32_t _88 = halide_error_buffer_allocation_too_large(NULL, "output$2", _86, _87);
   return _88;
  }
  int64_t _89 = (int64_t)(2147483647);
  bool _90 = _70 <= _89;
  if (!_90)   {
   int64_t _91 = (int64_t)(2147483647);
   int32_t _92 = halide_error_buffer_extents_too_large(NULL, "output$2", _70, _91);
   return _92;
  }
  int64_t _93 = (int64_t)(_p1_extent_0);
  int64_t _94 = (int64_t)(2147483647);
  bool _95 = _93 <= _94;
  if (!_95)   {
   int64_t _96 = (int64_t)(_p1_extent_0);
   int64_t _97 = (int64_t)(2147483647);
   int32_t _98 = halide_error_buffer_allocation_too_large(NULL, "p1", _96, _97);
   return _98;
  }
  int64_t _99 = (int64_t)(_p1_extent_1);
  int64_t _100 = (int64_t)(_p1_stride_1);
  int64_t _101 = _99 * _100;
  int64_t _102 = (int64_t)(2147483647);
  bool _103 = _101 <= _102;
  if (!_103)   {
   int64_t _104 = (int64_t)(_p1_extent_1);
   int64_t _105 = (int64_t)(_p1_stride_1);
   int64_t _106 = _104 * _105;
   int64_t _107 = (int64_t)(2147483647);
   int32_t _108 = halide_error_buffer_allocation_too_large(NULL, "p1", _106, _107);
   return _108;
  }
  int64_t _109 = (int64_t)(2147483647);
  bool _110 = _73 <= _109;
  if (!_110)   {
   int64_t _111 = (int64_t)(2147483647);
   int32_t _112 = halide_error_buffer_extents_too_large(NULL, "p1", _73, _111);
   return _112;
  }
  {
    //int64_t _113 = 1048576;
   //   uint8_t *_input2_shuffled = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_113);
   Buffer buf_input2_shuffled;
   buf_input2_shuffled.width = 128;
   buf_input2_shuffled.height =128;
   buf_input2_shuffled.depth = 64;
   buf_input2_shuffled.stride = 128;
   int ok_1 = ioctl(hwacc, GET_BUFFER, (long unsigned int)&buf_input2_shuffled);
   if(ok_1 < 0) {
     printf("Failed to allocate buffer 1!\n");
     return(0);
   }
   uint8_t *_input2_shuffled = (uint8_t*) mmap(NULL, buf_input2_shuffled.stride *
                                               buf_input2_shuffled.height *
                                               buf_input2_shuffled.depth,
                                               PROT_WRITE, MAP_SHARED, hwacc, buf_input2_shuffled.mmap_offset);
   if(_input2_shuffled == MAP_FAILED){
     printf("mmap 1 failed!\n");
     return(0);
   }

   // produce input2_shuffled
   for (int _input2_shuffled_s0_y_grid = 0; _input2_shuffled_s0_y_grid < 0 + 128; _input2_shuffled_s0_y_grid++)
   {
    for (int _input2_shuffled_s0_x_grid = 0; _input2_shuffled_s0_x_grid < 0 + 128; _input2_shuffled_s0_x_grid++)
    {
     for (int _input2_shuffled_s0_y_in = 0; _input2_shuffled_s0_y_in < 0 + 8; _input2_shuffled_s0_y_in++)
     {
      for (int _input2_shuffled_s0_x_in = 0; _input2_shuffled_s0_x_in < 0 + 8; _input2_shuffled_s0_x_in++)
      {
       int32_t _114 = _input2_shuffled_s0_y_in * 8;
       int32_t _115 = _input2_shuffled_s0_x_in + _114;
       int32_t _116 = _input2_shuffled_s0_x_grid * 64;
       int32_t _117 = _115 + _116;
       int32_t _118 = _input2_shuffled_s0_y_grid * 8192;
       int32_t _119 = _117 + _118;
       int32_t _120 = _input2_shuffled_s0_x_grid * 8;
       int32_t _121 = _120 + _input2_shuffled_s0_x_in;
       int32_t _122 = _input2_shuffled_s0_y_grid * 8;
       int32_t _123 = _122 + _input2_shuffled_s0_y_in;
       int32_t _124 = _123 * _p1_stride_1;
       int32_t _125 = _121 + _124;
       int32_t _126 = _p1_min_1 * _p1_stride_1;
       int32_t _127 = _p1_min_0 + _126;
       int32_t _128 = _125 - _127;
       uint8_t _129 = _p1[_128];
       _input2_shuffled[_119] = _129;
      } // for _input2_shuffled_s0_x_in
     } // for _input2_shuffled_s0_y_in
    } // for _input2_shuffled_s0_x_grid
   } // for _input2_shuffled_s0_y_grid
   // consume input2_shuffled
   {
     //int64_t _130 = 1048576;
    //uint8_t *_output_shuffled = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_130);
    Buffer buf_output_shuffled;
    buf_output_shuffled.width = 128;
    buf_output_shuffled.height =128;
    buf_output_shuffled.depth = 64;
    buf_output_shuffled.stride = 128;
    int ok_2 = ioctl(hwacc, GET_BUFFER, (long unsigned int)&buf_output_shuffled);
    if(ok_2 < 0) {
      printf("Failed to allocate buffer 2!\n");
      return(0);
    }
    uint8_t *_output_shuffled = (uint8_t*) mmap(NULL, buf_output_shuffled.stride *
                                               buf_output_shuffled.height *
                                               buf_output_shuffled.depth,
                                               PROT_WRITE, MAP_SHARED, hwacc, buf_output_shuffled.mmap_offset);
    if(_output_shuffled == MAP_FAILED){
      printf("mmap 2 failed!\n");
      return(0);
    }
    gettimeofday(&t1, NULL);
    // produce output_shuffled
    for (int _output_shuffled_s0_y_grid_yo = 0; _output_shuffled_s0_y_grid_yo < 0 + 4; _output_shuffled_s0_y_grid_yo++)
    {
     for (int _output_shuffled_s0_x_grid_xo = 0; _output_shuffled_s0_x_grid_xo < 0 + 4; _output_shuffled_s0_x_grid_xo++)
     {
      {
        //int64_t _131 = 87616;
       //uint8_t *_input_shuffled = (uint8_t *)halide_malloc(NULL, sizeof(uint8_t)*_131);
       Buffer buf_input_shuffled;
       buf_input_shuffled.width = 37;
       buf_input_shuffled.height = 37;
       buf_input_shuffled.depth = 64;
       buf_input_shuffled.stride = 37;
       int ok_0 = ioctl(hwacc, GET_BUFFER, (long unsigned int)&buf_input_shuffled);
       if(ok_0 < 0) {
         printf("Failed to allocate buffer 0!\n");
         return(0);
       }
       uint8_t *_input_shuffled = (uint8_t*) mmap(NULL, buf_input_shuffled.stride *
                                                  buf_input_shuffled.height *
                                                  buf_input_shuffled.depth,
                                                  PROT_WRITE, MAP_SHARED, hwacc, buf_input_shuffled.mmap_offset);
       if(_input_shuffled == MAP_FAILED){
         printf("mmap 0 failed!\n");
         return(0);
       }

       // produce input_shuffled
       int32_t _132 = _output_shuffled_s0_y_grid_yo * 32;
       int32_t _133 = _132 + -2;
       for (int _input_shuffled_s0_y_grid = _133; _input_shuffled_s0_y_grid < _133 + 37; _input_shuffled_s0_y_grid++)
       {
        int32_t _134 = _output_shuffled_s0_x_grid_xo * 32;
        int32_t _135 = _134 + -2;
        for (int _input_shuffled_s0_x_grid = _135; _input_shuffled_s0_x_grid < _135 + 37; _input_shuffled_s0_x_grid++)
        {
         for (int _input_shuffled_s0_y_in = 0; _input_shuffled_s0_y_in < 0 + 8; _input_shuffled_s0_y_in++)
         {
          for (int _input_shuffled_s0_x_in = 0; _input_shuffled_s0_x_in < 0 + 8; _input_shuffled_s0_x_in++)
          {
           int32_t _136 = _input_shuffled_s0_y_in * 8;
           int32_t _137 = _input_shuffled_s0_x_in + _136;
           int32_t _138 = _output_shuffled_s0_x_grid_xo * 32;
           int32_t _139 = _input_shuffled_s0_x_grid - _138;
           int32_t _140 = _139 * 64;
           int32_t _141 = _137 + _140;
           int32_t _142 = _output_shuffled_s0_y_grid_yo * 32;
           int32_t _143 = _input_shuffled_s0_y_grid - _142;
           int32_t _144 = _143 * 2368;
           int32_t _145 = _141 + _144;
           int32_t _146 = _145 + 4864;
           int32_t _147 = _input_shuffled_s0_x_grid * 8;
           int32_t _148 = _147 + _input_shuffled_s0_x_in;
           int32_t _149 = _148 + -4;
           int32_t _150 = _p1_min_0 + _p1_extent_0;
           int32_t _151 = _150 + -1;
           int32_t _152 = min(_149, _151);
           int32_t _153 = max(_152, _p1_min_0);
           int32_t _154 = _input_shuffled_s0_y_grid * 8;
           int32_t _155 = _154 + _input_shuffled_s0_y_in;
           int32_t _156 = _155 + -4;
           int32_t _157 = _p1_min_1 + _p1_extent_1;
           int32_t _158 = _157 + -1;
           int32_t _159 = min(_156, _158);
           int32_t _160 = max(_159, _p1_min_1);
           int32_t _161 = _160 * _p1_stride_1;
           int32_t _162 = _153 + _161;
           int32_t _163 = _p1_min_1 * _p1_stride_1;
           int32_t _164 = _p1_min_0 + _163;
           int32_t _165 = _162 - _164;
           uint8_t _166 = _p1[_165];
           _input_shuffled[_146] = _166;
          } // for _input_shuffled_s0_x_in
         } // for _input_shuffled_s0_y_in
        } // for _input_shuffled_s0_x_grid
       } // for _input_shuffled_s0_y_grid

       int ok_slice = 0;
       Buffer sliced_bufs[3];
       //sliced_bufs[0] = buf_input_shuffled;
       ok_slice += slice_buffer(&buf_input_shuffled, &sliced_bufs[0],
                                0, 0, 37, 37);
       ok_slice += slice_buffer(&buf_input2_shuffled, &sliced_bufs[1],
                                _output_shuffled_s0_x_grid_xo * 32,
                                _output_shuffled_s0_y_grid_yo * 32, 32, 32);
       ok_slice += slice_buffer(&buf_output_shuffled, &sliced_bufs[2],
                                _output_shuffled_s0_x_grid_xo * 32,
                                _output_shuffled_s0_y_grid_yo * 32, 32, 32);
       if (ok_slice < 0) {
         printf("slide_buffer failed.\n");
         return 0;
       }

       ioctl(hwacc, PROCESS_IMAGE, (long unsigned int)sliced_bufs);
       ioctl(hwacc, PEND_PROCESSED, NULL);


       //halide_free(NULL, _input_shuffled);
       munmap((void*)_input_shuffled, buf_input_shuffled.stride * buf_input_shuffled.height * buf_input_shuffled.depth);
       ioctl(hwacc, FREE_IMAGE, (long unsigned int)(&buf_input_shuffled));
      } // alloc _input_shuffled
     } // for _output_shuffled_s0_x_grid_xo
    } // for _output_shuffled_s0_y_grid_yo
    gettimeofday(&t2, NULL);
    elapsedTime += (t2.tv_sec - t1.tv_sec) * 1000.0;
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
    //halide_free(NULL, _input2_shuffled);
    munmap((void*)_input2_shuffled, buf_input2_shuffled.stride * buf_input2_shuffled.height * buf_input2_shuffled.depth);
    ioctl(hwacc, FREE_IMAGE, (long unsigned int)(&buf_input2_shuffled));
    // consume output_shuffled
    bool _506 = 0 <= _output_2_min_1;
    int32_t _507 = _output_2_min_1 + _output_2_extent_1;
    bool _508 = _507 <= 1024;
    bool _509 = _506 && _508;
    if (!_509)     {
     int32_t _510 = _output_2_min_1 + _output_2_extent_1;
     int32_t _511 = _510 + -1;
     int32_t _512 = halide_error_explicit_bounds_too_small(NULL, "y", "output$2", 0, 1023, _output_2_min_1, _511);
     return _512;
    }
    bool _513 = 0 <= _output_2_min_0;
    int32_t _514 = _output_2_min_0 + _output_2_extent_0;
    bool _515 = _514 <= 1024;
    bool _516 = _513 && _515;
    if (!_516)     {
     int32_t _517 = _output_2_min_0 + _output_2_extent_0;
     int32_t _518 = _517 + -1;
     int32_t _519 = halide_error_explicit_bounds_too_small(NULL, "x", "output$2", 0, 1023, _output_2_min_0, _518);
     return _519;
    }
    // produce output$2
    for (int _output_2_s0_y_yo = 0; _output_2_s0_y_yo < 0 + 128; _output_2_s0_y_yo++)
    {
     for (int _output_2_s0_x_xo = 0; _output_2_s0_x_xo < 0 + 128; _output_2_s0_x_xo++)
     {
      for (int _output_2_s0_y_y_in = 0; _output_2_s0_y_y_in < 0 + 8; _output_2_s0_y_y_in++)
      {
       for (int _output_2_s0_x_x_in = 0; _output_2_s0_x_x_in < 0 + 8; _output_2_s0_x_x_in++)
       {
        int32_t _520 = _output_2_s0_x_xo * 8;
        int32_t _521 = _520 + _output_2_s0_x_x_in;
        int32_t _522 = _output_2_s0_y_yo * 8;
        int32_t _523 = _522 + _output_2_s0_y_y_in;
        int32_t _524 = _523 * _output_2_stride_1;
        int32_t _525 = _521 + _524;
        int32_t _526 = _output_2_min_1 * _output_2_stride_1;
        int32_t _527 = _output_2_min_0 + _526;
        int32_t _528 = _525 - _527;
        int32_t _529 = _output_2_s0_y_y_in * 8;
        int32_t _530 = _output_2_s0_x_x_in + _529;
        int32_t _531 = _output_2_s0_x_xo * 64;
        int32_t _532 = _530 + _531;
        int32_t _533 = _output_2_s0_y_yo * 8192;
        int32_t _534 = _532 + _533;
        uint8_t _535 = _output_shuffled[_534];
        _output_2[_528] = _535;
       } // for _output_2_s0_x_x_in
      } // for _output_2_s0_y_y_in
     } // for _output_2_s0_x_xo
    } // for _output_2_s0_y_yo
    //halide_free(NULL, _output_shuffled);
    munmap((void*)_output_shuffled, buf_output_shuffled.stride * buf_output_shuffled.height * buf_output_shuffled.depth);
    ioctl(hwacc, FREE_IMAGE, (long unsigned int)(&buf_output_shuffled));
    // consume output$2
   } // alloc _output_shuffled
  } // alloc _input2_shuffled
 } // if _25
 close(hwacc);
 printf("accelerator run takes %f ms.\n", elapsedTime);
 return 0;
}


int pipeline_zynq(buffer_t *_p1_buffer, buffer_t *_output_2_buffer) HALIDE_FUNCTION_ATTRS {
 uint8_t *_p1 = (uint8_t *)(_p1_buffer->host);
 (void)_p1;
 const bool _p1_host_and_dev_are_null = (_p1_buffer->host == NULL) && (_p1_buffer->dev == 0);
 (void)_p1_host_and_dev_are_null;
 const int32_t _p1_min_0 = _p1_buffer->min[0];
 (void)_p1_min_0;
 const int32_t _p1_min_1 = _p1_buffer->min[1];
 (void)_p1_min_1;
 const int32_t _p1_min_2 = _p1_buffer->min[2];
 (void)_p1_min_2;
 const int32_t _p1_min_3 = _p1_buffer->min[3];
 (void)_p1_min_3;
 const int32_t _p1_extent_0 = _p1_buffer->extent[0];
 (void)_p1_extent_0;
 const int32_t _p1_extent_1 = _p1_buffer->extent[1];
 (void)_p1_extent_1;
 const int32_t _p1_extent_2 = _p1_buffer->extent[2];
 (void)_p1_extent_2;
 const int32_t _p1_extent_3 = _p1_buffer->extent[3];
 (void)_p1_extent_3;
 const int32_t _p1_stride_0 = _p1_buffer->stride[0];
 (void)_p1_stride_0;
 const int32_t _p1_stride_1 = _p1_buffer->stride[1];
 (void)_p1_stride_1;
 const int32_t _p1_stride_2 = _p1_buffer->stride[2];
 (void)_p1_stride_2;
 const int32_t _p1_stride_3 = _p1_buffer->stride[3];
 (void)_p1_stride_3;
 const int32_t _p1_elem_size = _p1_buffer->elem_size;
 (void)_p1_elem_size;
 uint8_t *_output_2 = (uint8_t *)(_output_2_buffer->host);
 (void)_output_2;
 const bool _output_2_host_and_dev_are_null = (_output_2_buffer->host == NULL) && (_output_2_buffer->dev == 0);
 (void)_output_2_host_and_dev_are_null;
 const int32_t _output_2_min_0 = _output_2_buffer->min[0];
 (void)_output_2_min_0;
 const int32_t _output_2_min_1 = _output_2_buffer->min[1];
 (void)_output_2_min_1;
 const int32_t _output_2_min_2 = _output_2_buffer->min[2];
 (void)_output_2_min_2;
 const int32_t _output_2_min_3 = _output_2_buffer->min[3];
 (void)_output_2_min_3;
 const int32_t _output_2_extent_0 = _output_2_buffer->extent[0];
 (void)_output_2_extent_0;
 const int32_t _output_2_extent_1 = _output_2_buffer->extent[1];
 (void)_output_2_extent_1;
 const int32_t _output_2_extent_2 = _output_2_buffer->extent[2];
 (void)_output_2_extent_2;
 const int32_t _output_2_extent_3 = _output_2_buffer->extent[3];
 (void)_output_2_extent_3;
 const int32_t _output_2_stride_0 = _output_2_buffer->stride[0];
 (void)_output_2_stride_0;
 const int32_t _output_2_stride_1 = _output_2_buffer->stride[1];
 (void)_output_2_stride_1;
 const int32_t _output_2_stride_2 = _output_2_buffer->stride[2];
 (void)_output_2_stride_2;
 const int32_t _output_2_stride_3 = _output_2_buffer->stride[3];
 (void)_output_2_stride_3;
 const int32_t _output_2_elem_size = _output_2_buffer->elem_size;
 (void)_output_2_elem_size;
 int32_t _536 = __pipeline_hls(_p1_buffer, _output_2_buffer);
 bool _537 = _536 == 0;
 if (!_537)  {
  return _536;
 }
 return 0;
}
#ifdef __cplusplus
}  // extern "C"
#endif
