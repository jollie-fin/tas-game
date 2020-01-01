#pragma once

#include <cstdint>
#include "point.h"
#include <vector>
#include <map>

struct StateObject
{
    Point2D pos_;
    Point2D speed_;
    uint32_t state_ : 8;
    uint32_t state_no_ : 7;
    uint32_t src_ : 9;
    uint32_t type_ : 8;
    uint32_t action_;
    uint32_t mvt_[2];
}; //32o

struct KeyStrokes
{
    uint8_t left_ : 1;
    uint8_t right_ : 1;
    uint8_t up_ : 1;
    uint8_t down_ : 1;
    uint8_t jump_ : 1;
    uint8_t action1_ : 1;
    uint8_t action2_ : 1;
    uint8_t action3_ : 1;
};

struct Character
{
    StateObject state_;
};

struct State
{
    static constexpr int nb_slots_ = 256;
    static constexpr int nb_vars_ = 256;
    std::array<StateObject, nb_slots_> slots_;
    std::array<int32_t, nb_vars_> var_;
    int32_t xscreen_;
    int32_t yscreen_;
    int32_t timestamp_;
    uint32_t rnd_;

    int allocate(StateObject so)
    {
        for (int i = 0; i != nb_slots_; ++i)
            if (slots_[i].type_ == 255)
            {
                slots_[i] = so;
                return i;
            }
        return -1;
    }

    bool free(int slot)
    {
        if (slots_[slot].type_ == -1)
            return false;
        slots_[slot].type_ = 255;
        return true;
    }

    uint32_t rnd()
    {
        //Borland C https://en.wikipedia.org/wiki/Linear_congruential_generator
        rnd_ = rnd_ * 22695477 + 1;
        return rnd_;
    }
};

State compute(const State &, std::vector<KeyStrokes> k);

class Arc
{
public:
    Point2D p1_, p2_;
    Point2D center_;
    fixed speed_;

    bool curved_{false};
    fixed length_;
    fixed radius_;
    fixed angle1_;
    fixed angle2_;

    Arc()
    {
    }

    Arc(Point2D p1, fixed speed, fixed radius, fixed angle1, fixed angle2)
        : p1_(p1)
        , speed_(speed)
        , curved_(true)
        , radius_(radius)
        , angle1_(angle1)
        , angle2_(angle2)
    {
        if (angle2_ > angle1_)
        center_ = p1 - radius * expj(angle1);
        p2_ = center_ + radius * expj(angle2);
        length_ = radius_ * 2 * PI * abs(angle2_ - angle1_) / 400;
        length_ /= speed_;
    }
    
    Arc(Point2D p1, Point2D p2, fixed speed)
        : p1_(p1), p2_(p2), speed_(speed), curved_(false)
    {
        Point2D delta = p2 - p1;
        length_ = hypot(delta.real(), delta.imag()) / speed_;
    }

    Arc swap() const
    {
        Arc result = *this;
        std::swap(result.p1_, result.p2_);
        std::swap(result.angle1_, result.angle2_);
        return result;
    }

    Point2D at(fixed param) const
    {
        fixed k = param / length_;
        fixed l = 1 - k;
        if (curved_)
        {
            fixed angle = k * angle1_ + l * angle2_;
            return center_ + radius_ * expj(angle);
        }
        else
        {
            return p1_ * k + l * p2_;
        }
    }
};

class Path
{
public:
    std::map<fixed, Arc> path_;
    bool reverse_{false};
    bool stop_end_{false};
    fixed total_length_;

    Path(std::vector<Arc> arcs, bool reverse, bool stop_end)
        : reverse_(reverse), stop_end_(stop_end)
    {
        for (auto &arc : arcs)
            add_arc(std::move(arc));
    }

    void add_arc(Arc arc)
    {
        fixed position = total_length_;
        total_length_ += arc.length_;
        path_[position] = arc;        
    }

    Point2D last_node() const
    {
        auto it = path_.rbegin();
        if (it != path_.rend())
            return it->second.p2_;
        else
            return {};
    }

    void add_segment(Point2D dest, fixed speed)
    {
        add_segment(last_node(), dest, speed);
    }

    void add_segment(Point2D source, Point2D dest, fixed speed)
    {
        add_arc(Arc(source, dest, speed));
    }

    void add_curve(fixed speed, fixed radius, fixed angle1, fixed angle2)
    {
        add_curve(last_node(), speed, radius, angle1, angle2);
    }

    void add_curve(Point2D source, fixed speed, fixed radius, fixed angle1, fixed angle2)
    {
        add_arc(Arc(source, speed, radius, angle1, angle2));
    }

    Point2D at(fixed timestamp) const
    {
        timestamp = abs(timestamp);

        if (stop_end_)
            timestamp = std::min(total_length_, timestamp);

        if (reverse_)
        {
            timestamp %= total_length_ * 2;
            if (timestamp > total_length_)
                timestamp = total_length_ * 2 - timestamp;
        }
        else
        {
            timestamp %= total_length_;
        }
        
        const auto &[base, arc] = *path_.lower_bound(timestamp);
        return arc.at(timestamp - base);
    }
};
