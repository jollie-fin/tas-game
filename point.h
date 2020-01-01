#pragma once

#include <complex>
#include <cstdint>
#include <utility>
#include "fixed.h"

using Point2D = std::complex<fixed>;
using IPoint2D = std::complex<int32_t>;
using Rectangle = std::pair<Point2D, Point2D>;
using IRectangle = std::pair<IPoint2D, IPoint2D>;

const Point2D baseX(1,0);
const Point2D baseY(0,1);

Point2D expj(fixed angle)
{
    return baseX * cos(angle) + baseY * sin(angle);
}

bool inside(Rectangle r, Point2D p)
{
    return r.first.real() <= p.real()
           && r.first.imag() <= p.imag()
           && r.second.real() >= p.real()
           && r.second.imag() >= p.imag();
}

bool inside(IRectangle r, IPoint2D p)
{
    return r.first.real() <= p.real()
           && r.first.imag() <= p.imag()
           && r.second.real() >= p.real()
           && r.second.imag() >= p.imag();
}
