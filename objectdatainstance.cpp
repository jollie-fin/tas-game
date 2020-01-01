#include "objectdatainstance.h"

void draw(std::vector<SpriteInstance> to_draw)
{
    std::sort(to_draw.begin(),
              to_draw.end(),
              [](const auto &si1, const auto &si2)
              {
                  return std::tie(si1.depth_, si1.id_)
                         < std::tie(si2.depth_, si2.id_);
              });

    std::vector<double> xs;
    xs.reserve(to_draw.size() * 4 * 2);
    std::vector<double> uv;
    uv.reserve(to_draw.size() * 4 * 2);
    std::vector<int> id;
    id.reserve(to_draw.size());

    for (const SpriteInstance &si : to_draw)
    {
        double x = si.coor_.real().to_float();
        double y = si.coor_.imag().to_float();
        auto *sprite = si.sprite_;
        double u = sprite->coor_.real();
        double v = sprite->coor_.imag();
        xs.insert(xs.end(), {x, y, x + 64, y, x + 64, y + 64, x, y + 64});
        uv.insert(uv.end(), {u, v, u + 64, v, u + 64, v + 64, u, v + 64});
        id.push_back(sprite->id_imag_);
    }
}

bool collides_with(const CollisionMask &mask1, Point2D offset1,
                   const CollisionMask &mask2, Point2D offset2)
{
    constexpr int mask_width = CollisionMask::width_;

    auto offset = offset2 - offset1;
    if (offset.real() < 0)
        return collides_with(mask2, offset2,
                             mask1, offset1);

    int xoffset = offset.real().integral();
    int yoffset = offset.imag().integral();

    if (xoffset >= mask_width
        || yoffset >= mask_width
        || yoffset <= -mask_width
        || yoffset <= -mask_width)
        return false;

    int foffset = xoffset % mask_width;
    int ioffset = xoffset / mask_width;
    int miny = std::max(0,
                        yoffset);
    int maxy = std::min(mask_width,
                        mask_width + yoffset);

    for (int y = miny; y < maxy; y++)
    {
        auto bmask1 = mask1.mask_[y];
        auto bmask2 = mask2.mask_[y - yoffset];
        if (bmask1
            & bmask2 >> foffset)
            return true;
    }
    return false;
}

std::vector<std::pair<int, int>> colliding(const std::vector<CollisionMaskInstance> &to_collide,
                                           std::function<bool (const CollisionMaskInstance &)> predicat)
{
    std::vector<std::pair<int, int>> result;
    std::vector<CollisionMaskInstance> restricted;

    std::copy_if(to_collide.begin(),
                 to_collide.end(),
                 std::back_inserter(restricted),
                 predicat);

    std::sort(restricted.begin(),
              restricted.end(),
              [](const auto &cmi1, const auto &cmi2)
              {
                  return cmi1.coor_.real() < cmi2.coor_.imag();
              });

    for (auto first = restricted.begin();
         first != restricted.end();
         ++first)
    {
        for (auto second = first;
             second != restricted.end()
             && second->coor_.real() - first->coor_.real() < 64;
             ++second)
        {
            if ((!first->is_background_
                 || !second->is_background_)
                && collides_with(*first->mask_, first->coor_,
                                 *second->mask_, second->coor_))
            {
                result.emplace_back(first->id_, second->id_);
            }
        }
    }
}
