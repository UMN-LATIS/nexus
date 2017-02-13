#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "math.h"

namespace nx {

class Point {
public:
    float v[3];
    Point() {}
    Point(float x, float y, float z) { v[0] = x; v[1] = y; v[2] = z; }
    float &operator[](int n) { return v[n]; }
    float operator[](int n) const { return v[n]; }

    Point operator+(Point const &p) const { return Point(v[0] + p[0], v[1] + p[1], v[2] + p[2]); }
    Point operator-(Point const &p) const { return Point(v[0] - p[0], v[1] - p[1], v[2] - p[2]); }
    Point operator*(const float s) const { return Point(v[0] * s, v[1] * s, v[2] * s); }
    Point operator/(const float s) const { return Point(v[0] / s, v[1] / s, v[2] / s); }

    Point &operator+=(Point const &p) { v[0] += p[0]; v[1] += p[1]; v[2] += p[2]; return *this; }
    Point &operator-=(Point const &p) { v[0] -= p[0]; v[1] -= p[1]; v[2] -= p[2]; return *this; }
    Point &operator*=(const float s) { v[0] *= s; v[1] *= s; v[2] *= s; return *this; }
    Point &operator/=(const float s) { v[0] /= s; v[1] /= s; v[2] /= s; return *this; }

    float norm() { return sqrt(squaredNorm()); }
    float squaredNorm() { return v[0]*v[0] + v[1]*v[1] + v[2]*v[2]; }
};

class Plane {
public:
    float v[4];
};

class Sphere {
public:
    Point center;
    float radius;
};

class Cone {
    short n[4];
};

};
#endif // GEOMETRY_H
