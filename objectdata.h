#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <memory>
#include <set>

#include "point.h"
#include "data.h"

namespace ObjData
{
    enum ID_SPOT {
        SPAWN,
        HOLD,
        FEET,
        GRAB_HOLD,
        HOLDS,
        FALL_LEFT,
        FALL_RIGHT,
        WALL_LEFT,
        WALL_RIGHT,
        NUMBER_SPOTS
    };

    enum ID_MASK {
        WALL = NUMBER_SPOTS,
        GROUND,
        TARGET,
        ATTACK,
        LADDER,
        SHIELD,
        PORTAL,
        DESTROY,
        UNSPAWN,
        NUMBER_MASKS
    };

    static constexpr int DEPTH_BITS = 4;
    static constexpr int NUMBER_ANIMATIONS = 256;

    struct CollisionEvt
    {
        ID_SPOT id_spot_;
        int obj_spot_;
        ID_MASK id_mask_;
        int obj_mask_;
    };

    using CollisionEvts = std::vector<CollisionEvt>;

    struct Pixel
    {
        uint8_t r_,g_,b_,a_;
        uint32_t masks_ : 24;
        uint8_t depth_;
    };

    struct Image
    {
        std::vector<Pixel> content_;
        int w_, h_, stride_; 
    };

    class Sprite
    {
        public:
            Image *image_;
            int id_image_;
            IRectangle coor_;

        public:
            Sprite(Image *image, int id_image, IRectangle coor)
                : image_(image), id_image_(id_image), coor_(coor)
            {
            }

            bool contains(Point2D coor, int mask) const
            {
                IPoint2D position_in_sprite(coor.real().roundin(),
                                            coor.imag().roundin());
                position_in_sprite += coor_.first;
                if (inside(coor_, position_in_sprite))
                {
                    auto x = position_in_sprite.real();
                    auto y = position_in_sprite.imag();
                    return image_->content_[x + y * image_->stride_].masks_ & (1 << mask);
                }
                return false;
            }

    };

    class FrameData
    {
        public:
        Sprite sprite_;
        std::array<Point2D, NUMBER_SPOTS> spots_;
    };

    class AnimationData
    {
        public:
        std::vector<FrameData> frames_;
        bool loop_;

        const FrameData &get(uint32_t timestamp) const
        {
            if (loop_ || timestamp < frames_.size())
                return frames_[timestamp % frames_.size()];
            else
                return frames_.back();
        }
    };

    class GraphicData
    {
        public:
        std::array<AnimationData, NUMBER_ANIMATIONS> animations_;
    };

    class SpriteInstance
    {
        public:
            FrameData *frame_;
            Point2D coor_;
            int order_;
            int id_;
            Point2D parallax_coeff_;
            bool has_parallax_;
            int object_;

            Point2D pos(Point2D camera) const
            {
                Point2D delta = coor_ - camera;
                fixed x = delta.real() * parallax_coeff_.real();
                fixed y = delta.imag() * parallax_coeff_.imag();
                return coor_ + (x, y);
            }

            bool contains(ID_MASK mask,
                          const SpriteInstance &other,
                          ID_SPOT spot) const
            {
                // no collision if parallax
                if (has_parallax_ || other.has_parallax_)
                    return false;

                auto position = other.coor_ + other.frame_->spots_[spot];
                position -= coor_;

                return frame_->sprite_.contains(position, mask);
            }
    };

    class Map
    {
        int w_cell_, h_cell_, w_, h_, stride_;
        std::vector<const AnimationData *> map_;

        std::pair<Point2D, const AnimationData *> get(Point2D pos) const
        {
            int x = (pos.real() / w_cell_).roundin();
            int y = (pos.imag() / h_cell_).roundin();

            if (x >= 0 && y >= 0 && x < w_ && y < h_)
                return {{pos.real() - x * w_cell_,
                         pos.imag() - y * h_cell_},
                        map_[x + stride_ * y]};
            else
                return {{}, nullptr};
        }

        bool contains(uint32_t timestamp,
                      ID_MASK mask,
                      const SpriteInstance &other,
                      ID_SPOT spot) const
        {
            // no collision if parallax
            if (other.has_parallax_)
                return false;
            auto position = other.coor_ + other.frame_->spots_[spot];
            auto [pos_in_cell, cell] = get(position);

            if (!cell)
                return false;

            const auto &sprite = cell->get(timestamp).sprite_;
            return sprite.contains(position, mask);
        }
    };

    class Action
    {
    protected:
        std::string name_;
        int phase_{0};

    public:
        Action(std::string name, int phase)
            : name_(name), phase_(phase)
        {
        }

        /* order to execute, the lower the highest priority */
        int phase() const
        {
            return phase_;
        }

        /* Initialisation on creation */
        virtual void newobject(State &st, StateObject &) const
        {
            return;
        }

        /* do something */
        virtual void execute(State &st, int self, const CollisionEvts &) const
        {
        }

        /* render informaion */
        virtual std::optional<SpriteInstance> graphic(const State &st, int self) const
        {
            return {};
        }
    };

    class Object;

    class MovingAlongPath : public Action
    {
    protected:
        Path &path_;
        fixed speed_;

    public:
        MovingAlongPath(std::string name, int phase, Path &path, fixed speed)
            : Action(name, phase), path_(path), speed_(speed)
        {
        }

        void newobject(State &st, StateObject &) const override;
        void execute(State &st, int self, const CollisionEvts &) const override;
    };

    class Walker : public Action //instance
    {
        enum DIRECTION {LEFT, RIGHT, RND};
        fixed speed_;
        bool fall_{false};
        fixed gravity_coeff_{1};
        DIRECTION direction_;
        
        Walker(std::string name, int phase, fixed speed, bool fall, fixed gravity_coeff, DIRECTION direction)
            : Action(name, phase), speed_(speed), fall_(fall), gravity_coeff_(gravity_coeff), direction_(direction)
        {
        }

        void newobject(State &st, StateObject &) const override;
        void execute(State &st, int self, const CollisionEvts &) const override;
    };

    class CopyPosition : public Action //instance
    {
        Point2D offset_;

        CopyPosition(std::string name, int phase, Point2D offset)
            : Action(name, phase), offset_(offset)
        {
        }

        void execute(State &st, int self, const CollisionEvts &) const override;
    };

    class Fall : public Action //instance
    {
        fixed max_speed_;
        Point2D direction_;
        Point2D rnd_;
        fixed gravity_coeff_{1};
        bool shall_bounce_;
        fixed absorb_;

        Fall(std::string name, int phase,
             fixed max_speed,
             Point2D direction,
             Point2D rnd,
             fixed gravity_coeff,
             bool shall_bounce,
             fixed absorb)
            : Action(name, phase),
              max_speed_(max_speed),
              direction_(direction),
              rnd_(rnd),
              gravity_coeff_(gravity_coeff),
              shall_bounce_(shall_bounce),
              absorb_(absorb)
        {
        }

        void newobject(State &st, StateObject &) const override;
        void execute(State &st, int self, const CollisionEvts &) const override;
    };

    class Seek : public Action //instance
    {
        Point2D direction_; //must be normalized, 0 if no direction
        fixed acceleration_;
        fixed max_speed_;
        fixed max_rotation_;
        bool object_;

        void execute(State &st, int self, const CollisionEvts &) const override;
    };

    class Plateform : public Action
    {
        void execute(State &st, int self, const CollisionEvts &) const override;
    };

    class Enemy : public Action
    {
        void execute(State &st, int self, const CollisionEvts &) const override;
    };

    class Hortense : public Action
    {
        void execute(State &st, int self, const CollisionEvts &) const override;
    };

    class Plateform : public Action
    {
        void execute(State &st, int self, const CollisionEvts &) const override;
    };

    class Spawner : public Action
    {
        std::vector<fixed> timestamps_;
        fixed interval_;
        fixed offset_;
        int obj_;
        int max_nb_elts_;
        int max_spawn_;

        void execute(State &st, int self, const CollisionEvts &) const override;
    };

    class Portal : public Action //instance
    {
        bool is_door_;
        bool automatic_;
        bool all_animated_;
        bool one_at_a_time_;

        void execute(State &st, int self, const CollisionEvts &) const override;
    };

    class ChangeAnimation : public Action
    {
        int var_index_;

        std::optional<SpriteInstance> graphic(const State &st, int self) const override;
    };

    class Object
    {
        using ActionPtr = std::unique_ptr<Action>;
        struct Compare
        {
            using is_transparent = void;
            bool operator()(const ActionPtr& action1, const ActionPtr& action2) const
            {
                return action1->phase() < action2->phase();
            }

            bool operator()(int phase, const ActionPtr& action2) const
            {
                return phase < action2->phase();
            }

            bool operator()(const ActionPtr& action1, int phase) const
            {
                return action1->phase() < phase;
            }
        };

        bool is_plateformable_;
        bool is_ennemy_;
        bool is_friendly_;
        bool is_hortense_;
        int type_;
        int depth_;
        fixed parallax_coeff_;
        GraphicData &graphic_;

        std::multiset<ActionPtr, Compare> actions_;

        void execute(State &st, int self, const CollisionEvts &evts, int phase)
        {
            for (auto [begin, end] = actions_.equal_range(phase);
                 begin != end;
                 ++begin)
            {
                const auto &action = *begin;
                action->execute(st, self, evts);
            }
        }

        int newobject(State &st, int src, Point2D pos)
        {
            StateObject so;
            so.pos_ = pos;
            so.type_ = type_;
            so.src_ = src;
            for (const auto &action : actions_)
                action->newobject(st, so);
            return st.allocate(so);
        };

        void graphic(const State &st, int self, std::vector<SpriteInstance> &sis)
        {
            for (const auto &action : actions_)
            {
                if (auto smth = action->graphic(st, self); smth)
                {
                    sis.emplace_back(*smth);
                    return;
                }
            }
            SpriteInstance si;
            const StateObject &so = st[self];
            si.sprite_ = &graphic_.animations_[so.state_].frames_[so.state_no_].sprite_;
            si.coor_ = so.pos_;
            si.depth_ = depth_;
            si.id_ = sis.size();
            si.parallax_coeff_ = parallax_coeff_;
            si.object_ = self;
            sis.emplace_back(si);
        }
    };

    static_assert(NUMBER_SPOTS <= 24);

    class Level
    {
        std::vector<GraphicData> graphics_;
        //smth music
        std::vector<Path> paths_;
        std::vector<Object> object_;
        std::vector<StateObject> static_objects_;
        Map map_;

        CollisionEvts collisions(ID_MASK id_mask,
                                 ID_SPOT id_spot,
                                 int type);
    };
}

//Touch(Every(condition),All(),'ground')
//
//Lambda(Every(chauve), Char(), And(Near(300),Equals(Var1('A'),5)), Set(Field, '+5'))