#include "snipmath.h"
#include <math.h>

volatile double result_sink_double;
volatile unsigned short result_sink_ushort;

int basic_math_small(void)
{
  double a1 = 1.0, b1 = -10.5, c1 = 32.0, d1 = -30.0;
  double a2 = 1.0, b2 = -4.5, c2 = 17.0, d2 = -30.0;
  double a3 = 1.0, b3 = -3.5, c3 = 22.0, d3 = -31.0;
  double a4 = 1.0, b4 = -13.7, c4 = 1.0, d4 = -35.0;
  double x[3];
  double X;
  int solutions;
  int i;
  unsigned long l = 0x3fed0169L;
  struct int_sqrt q;

  /* solve specific cubic functions */
  SolveCubic(a1, b1, c1, d1, &solutions, x);  
  result_sink_double = x[0];



  for(a1=1; a1<10; a1++) {
    for(b1=10; b1>0; b1--) {
      for(c1=5; c1<15; c1+=0.5) {
        for(d1=-1; d1>-11; d1--) {
          SolveCubic(a1, b1, c1, d1, &solutions, x);  
          result_sink_double = x[0];
        }
      }
    }
  }
  
  for (i = 0; i < 1001; ++i) {
    usqrt(i, &q);
    result_sink_ushort = q.sqrt;
  }
  

  for (X = 0.0; X <= (2 * PI + 1e-6); X += (PI / 180)) {
    result_sink_double = rad2deg(X);
  }
  
  return 0;
}