#pragma once

#include "MapUi.hpp"

#include <cstdint>
#include <cmath>
#include <map>

namespace gtasa {

struct TouchMotion { enum class Kind { None, Pan, Pinch } kind{Kind::None}; float x{}; float y{}; };
struct TouchEnd { bool singleTap{}; bool twoFingerTap{}; };

class TouchGestureState {
public:
    void begin(std::int64_t id, float x, float y) {
        points_[id] = {x, y}; starts_[id] = {x, y};
        if (points_.size() == 1) { moved_ = false; multi_ = false; twoFingerTap_ = false; }
        else if (points_.size() == 2) { pinchDistance_ = distance(); multi_ = true; twoFingerTap_ = !moved_; }
    }
    TouchMotion move(std::int64_t id, float x, float y) {
        auto current = points_.find(id); if (current == points_.end()) return {};
        const Point previous = current->second; current->second = {x, y};
        if (points_.size() == 1) {
            const Point start = starts_[id];
            const bool wasDragging = moved_;
            if (!moved_ && !exceedsTouchDragThreshold(start.x, start.y, x, y)) return {};
            moved_ = true;
            return {TouchMotion::Kind::Pan, wasDragging ? x - previous.x : x - start.x,
                    wasDragging ? y - previous.y : y - start.y};
        }
        if (points_.size() == 2) {
            const auto first = points_.begin(), second = std::next(first);
            const Point firstStart = starts_[first->first], secondStart = starts_[second->first];
            if (exceedsTouchDragThreshold(firstStart.x, firstStart.y, first->second.x, first->second.y) ||
                exceedsTouchDragThreshold(secondStart.x, secondStart.y, second->second.x, second->second.y)) twoFingerTap_ = false;
            const float now = distance();
            if (pinchDistance_ > 1.0f && std::abs(now - pinchDistance_) >= 2.0f) {
                moved_ = true; twoFingerTap_ = false;
                const float scale = now / pinchDistance_; pinchDistance_ = now;
                return {TouchMotion::Kind::Pinch, scale, 0.0f};
            }
            pinchDistance_ = now;
        }
        return {};
    }
    TouchEnd end(std::int64_t id) {
        if (points_.find(id) == points_.end()) return {};
        const bool single = points_.size() == 1 && isTapGesture(moved_, multi_);
        points_.erase(id); starts_.erase(id);
        const bool two = multi_ && points_.empty() && twoFingerTap_;
        if (points_.empty()) { multi_ = false; twoFingerTap_ = false; }
        return {single, two};
    }
private:
    struct Point { float x{}, y{}; };
    float distance() const { auto a = points_.begin(), b = std::next(a); const float dx = b->second.x-a->second.x, dy=b->second.y-a->second.y; return std::sqrt(dx*dx+dy*dy); }
    std::map<std::int64_t, Point> points_, starts_;
    bool moved_ = false, multi_ = false, twoFingerTap_ = false;
    float pinchDistance_ = 0.0f;
};

} // namespace gtasa
