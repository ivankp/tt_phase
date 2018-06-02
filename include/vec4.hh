#ifndef IVANP_VEC4_HH
#define IVANP_VEC4_HH

class vec4 {
  double v[4];

public:
  vec4(): v{0,0,0,0} { }
  vec4(double x, double y, double z, double t): v{x,y,z,t} { }

  inline double operator[](unsigned i) const noexcept { return v[i]; }
  inline double& operator[](unsigned i) noexcept { return v[i]; }

  inline vec4& operator+=(const vec4& u) noexcept {
    v[0] += u[0]; v[1] += u[1]; v[2] += u[2]; v[3] += u[3];
    return *this;
  }
  inline vec4& operator-=(const vec4& u) noexcept {
    v[0] -= u[0]; v[1] -= u[1]; v[2] -= u[2]; v[3] -= u[3];
    return *this;
  }
  inline vec4& operator*=(double a) noexcept {
    v[0] *= a; v[1] *= a; v[2] *= a; v[3] *= a;
    return *this;
  }
  inline vec4& operator/=(double a) noexcept {
    v[0] /= a; v[1] /= a; v[2] /= a; v[3] /= a;
    return *this;
  }
};

inline vec4 operator+(const vec4& v, const vec4& u) noexcept {
  return { v[0]+u[0], v[1]+u[1], v[2]+u[2], v[3]+u[3] };
}
inline vec4 operator-(const vec4& v, const vec4& u) noexcept {
  return { v[0]-u[0], v[1]-u[1], v[2]-u[2], v[3]-u[3] };
}
inline double operator*(const vec4& v, const vec4& u) noexcept {
  return v[3]*u[3] - v[0]*u[0] - v[1]*u[1] - v[2]*u[2];
}

inline vec4 operator*(const vec4& v, double a) noexcept {
  return { v[0]*a, v[1]*a, v[2]*a, v[3]*a };
}
inline vec4 operator*(double a, const vec4& v) noexcept { return v*a; }

inline vec4 operator/(const vec4& v, double a) noexcept {
  return { v[0]/a, v[1]/a, v[2]/a, v[3]/a };
}

#endif
