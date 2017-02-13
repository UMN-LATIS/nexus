#ifndef FPU_PRECISION_H
#define FPU_PRECISION_H
 
#ifdef _GNUCC
#include <fpu_control.h>



class FpuPrecision {
 public:
  static void store() {
    _FPU_GETCW(old()); // store old cw
  }
  static void setFloat() {
    fpu_control_t fpu_cw = (old() & ~_FPU_EXTENDED & ~_FPU_DOUBLE & ~_FPU_SINGLE) | _FPU_SINGLE;
    _FPU_SETCW(fpu_cw);
  }

  static void setDouble() {
   fpu_control_t fpu_cw = (old() & ~_FPU_EXTENDED & ~_FPU_DOUBLE & ~_FPU_SINGLE) | _FPU_DOUBLE;
   _FPU_SETCW(fpu_cw);
  }

  static void restore() {
    _FPU_SETCW(old()); // restore old cw
  }

  static fpu_control_t &old() {
    static fpu_control_t f;
    return f;
  }
};

#else
class FpuPrecision {
 public:
  static void store() {
  
  }
  static void setFloat() {
   
  }

  static void setDouble() {
 
  }

  static void restore() {
   
  }

 
};
#endif

#endif // FPU_PRECISION_H
