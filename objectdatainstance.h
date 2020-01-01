#pragma once

#include "objectdata.h"

#include <vector>
#include <functional>
#include <algorithm>


void draw(std::vector<SpriteInstance> to_draw);

std::vector<std::pair<int, int>>
    colliding(const std::vector<CollisionMaskInstance> &to_collide,
              std::function<bool (const CollisionMaskInstance &)> predicat);