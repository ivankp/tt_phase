#ifndef LEGENDRE_HH
#define LEGENDRE_HH

#include <complex>
#include "ivanp/math/math.hh"

// (Sum[c(k) LegendreP[k,x], {k, 0, 6, 2}])^2
double Legendre(double x, const double* c) {
  using namespace ivanp::math;
  const double x2 = x*x, x4 = x2*x2, x6 = x4*x2;

  const double p2 = 1.5*x2 - 0.5;
  const double p4 = 4.375*x4 - 3.75*x2 + 0.375;
  const double p6 = 14.4375*x6 - 19.6875*x4 + 6.5625*x2 - 0.3125;

  const double c0 = std::sqrt(
    0.5 - (0.2*sq(c[0]) + (1./9.)*sq(c[1]) + (1./13.)*sq(c[2])) );
  // 0.5 on [-1,1]
  // 1   on [ 0,1]

  const auto phase = std::polar<double>(1.,c[3]);

  // std::norm(x) is |x|^2
  return std::norm( c0 + c[0]*phase*p2 + c[1]*p4 + c[2]*p6 );
}

#endif
