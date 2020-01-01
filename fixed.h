#pragma once

#include <cstdint>
#include <cstdlib>

struct fixed
{
    struct raw_t
    {
    };

    static constexpr raw_t raw;
    static constexpr int32_t fracsize_ = 10;
    static constexpr int32_t fracexp_ = 1 << fracsize_;
    static constexpr double fracexpd_ = fracexp_;
    
    int32_t value_;

    fixed(int32_t other = 0)
        : value_(other * fracexp_)
    {
    }

    fixed(int32_t other, raw_t)
        : value_(other)
    {
    }

//    fixed(double other)
//        : value_(static_cast<int32_t>(other * fracexp_))
//    {
//    }

    //arrondi vers 0
    int64_t roundin() const
    {
        return value_ / fracexp_;
    }

    //arrondi en s'éloignant de 0
    int64_t roundout() const
    {
        if (value_ < 0)
            return (value_ - fracexp_ + 1) / fracexp_;
        else
            return (value_ + fracexp_ - 1) / fracexp_;
    }

    int64_t fractional() const
    {
        return value_ % fracexp_;
    }

    fixed &operator+=(fixed other)
    {
        value_ += other.value_;
        return *this;
    }

    fixed &operator-=(fixed other)
    {
        value_ -= other.value_;
        return *this;
    }

    fixed &operator*=(fixed other)
    {
        value_ = (static_cast<int64_t>(value_)
                  * static_cast<int64_t>(other.value_))
                / fracexp_;
        return *this;
    }

    fixed &operator/=(fixed other)
    {
        value_ = (static_cast<int64_t>(value_) * fracexp_)
                 / static_cast<int64_t>(other.value_);
        return *this;
    }

    fixed &operator%=(fixed other)
    {
        value_ %= other.value_;
        return *this;
    }

    fixed &operator+=(int64_t other)
    {
        value_ += other * fracexp_;
        return *this;
    }

    fixed &operator-=(int64_t other)
    {
        value_ -= other * fracexp_;
        return *this;
    }

    fixed &operator*=(int64_t other)
    {
        value_ *= other;
        return *this;
    }

    fixed &operator/=(int64_t other)
    {
        value_ /= other;
        return *this;
    }

    fixed &operator%=(int64_t other)
    {
        value_ %= other * static_cast<int64_t>(fracexp_);
        return *this;
    }

    double to_double() const
    {
        return static_cast<double>(value_)
             / static_cast<double>(fracexp_);
    }
};

fixed PI(3217, fixed::raw);

fixed operator+(fixed number, fixed other)
{
    number += other;
    return number;
}

fixed operator-(fixed number, fixed other)
{
    number -= other;
    return number;
}

fixed operator*(fixed number, fixed other)
{
    number *= other;
    return number;
}

fixed operator/(fixed number, fixed other)
{
    number /= other;
    return number;
}

fixed operator%(fixed number, fixed other)
{
    number %= other;
    return number;
}

fixed operator-(fixed number)
{
    number *= -1;
    return number;
}

bool operator<(fixed number, fixed other)
{
    return number.value_ < other.value_;
}

bool operator<=(fixed number, fixed other)
{
    return number.value_ <= other.value_;
}

bool operator>(fixed number, fixed other)
{
    return number.value_ > other.value_;
}

bool operator>=(fixed number, fixed other)
{
    return number.value_ >= other.value_;
}

bool operator==(fixed number, fixed other)
{
    return number.value_ == other.value_;
}

bool operator!=(fixed number, fixed other)
{
    return number.value_ != other.value_;
}

fixed operator+(fixed number, int64_t other)
{
    number += other;
    return number;
}

fixed operator-(fixed number, int64_t other)
{
    number -= other;
    return number;
}

fixed operator-(int64_t number, fixed other)
{
    other -= number;
    other = -other;
    return other;
}

fixed operator*(fixed number, int64_t other)
{
    number *= other;
    return number;
}

fixed operator/(fixed number, int64_t other)
{
    number /= other;
    return number;
}

fixed operator%(fixed number, int64_t other)
{
    number %= other;
    return number;
}

bool operator<(fixed number, int32_t other)
{
    return number.value_ < other * fixed::fracexp_;
}

bool operator<=(fixed number, int32_t other)
{
    return number.value_ <= other * fixed::fracexp_;
}

bool operator>(fixed number, int32_t other)
{
    return number.value_ > other * fixed::fracexp_;
}

bool operator>=(fixed number, int32_t other)
{
    return number.value_ >= other * fixed::fracexp_;
}

bool operator==(fixed number, int32_t other)
{
    return number.value_ == other * fixed::fracexp_;
}

bool operator!=(fixed number, int32_t other)
{
    return number.value_ != other * fixed::fracexp_;
}

static uint16_t cos_grad[101] = {1024, 1023, 1023, 1022, 1021, 1020, 1019, 1017, 1015, 1013, 1011, 1008, 1005, 1002, 999, 995, 991, 987, 983, 978, 973, 968, 963, 957, 952, 946, 939, 933, 926, 919, 912, 904, 897, 889, 881, 873, 864, 855, 846, 837, 828, 818, 809, 799, 789, 778, 768, 757, 746, 735, 724, 712, 700, 689, 677, 665, 652, 640, 627, 614, 601, 588, 575, 562, 548, 535, 521, 507, 493, 479, 464, 450, 435, 421, 406, 391, 376, 361, 346, 331, 316, 301, 285, 270, 254, 239, 223, 207, 191, 176, 160, 144, 128, 112, 96, 80, 64, 48, 32, 16, 0};

fixed abs(fixed other)
{
    if (other < 0)
        return -other;
    else
        return other;
}

fixed cos(fixed angle)
{
    angle = abs(angle);
    angle %= 400;
    
    if (angle > 200)
        angle = 400 - angle;

    bool negative = angle > 100;
    if (negative)
        angle = 200 - angle;
    
    auto k = angle.fractional();
    auto angle1 = angle.roundin();
    auto angle2 = angle.roundout();
    auto cos1 = fixed(cos_grad[angle1]) / 1000;
    auto cos2 = fixed(cos_grad[angle2]) / 1000;
    auto result = cos1 * k + cos2 * (1 - k);

    if (negative)
        result = -result;
    return result;
}

fixed sin(fixed number)
{
    return cos(100 - number);
}

fixed sqrt(fixed number)
{
    #warning "verify"
    double d = number.to_double();
    fixed result;
    result.value_ = std::ceil(std::sqrt(d) * fixed::fracexpd_);
    while (result * result > number)
        result.value_--;
    return result;
}

fixed hypot(Point2D vect)
{
    #warning "verify"
    double d1 = vect.real().to_double();
    double d2 = vect.real().to_double();
    fixed result(std::ceil(std::hypot(d1, d2) * fixed::fracexpd_), fixed::raw);
    fixed squared_distance = d1 * d1 + d2 * d2;

    while (result * result > squared_distance)
        result.value_--;
    return result;
}

fixed atan2(Point2D vect)
{
    #warning "verify"
    double x = vect.real().to_double();
    double y = vect.imag().to_double();
    fixed angle(std::ceil(std::atan2(x, y) * fixed::fracexpd_ * 200. / 3.14159), fixed::raw);

    fixed scal = (expj(angle) * std::conj(vect)).real();
    fixed new_scal = scal;

    do
    {
        angle.value_--;
        scal = new_scal;
        new_scal = (expj(angle - fixed(1, fixed::raw)) * std::conj(vect)).real();
    } while (new_scal > scal);  
    angle.value_++;

    return angle;
}

