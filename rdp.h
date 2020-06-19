#ifndef RDP_H
#define RDP_H

#include <math.h>
#include <vector>

typedef struct point_tag {
  double x;
  double y;
} point_t;

bool operator==(const point_t &a, const point_t &b) { a.x == b.x &&a.y == b.y; }

// Returns the distance from point p to the line between p1 and p2
double perpendicular_distance(point_t p, point_t p1, point_t p2) {
  double dx = p2.x - p1.x;
  double dy = p2.y - p1.y;
  double d = sqrt(dx * dx + dy * dy);
  return fabs(p.x * dy - p.y * dx + p2.x * p1.y - p2.y * p1.x) / d;
}

void print_points(std::vector<point_t> &points) {

  for (std::vector<point_t>::iterator it = points.begin(); it != points.end();
       ++it)
    Serial.printf(" (%g, %g)", (*it).x, (*it).y);

  Serial.printf("\n");
}

// Simplify a vector of points using the Ramer–Douglas–Peucker algorithm
// (non-recursive).
std::vector<point_t> DouglasPeucker(const std::vector<point_t> &pts,
                                    const double tolerance) {

  std::vector<point_t> result_pts;

  struct StackElement {
    point_t pt;
    size_t idx;
  };

  {
    std::vector<StackElement> dpStack;
    result_pts.resize(0);
    // size == 8 is some arbitrary size at which simplification probably
    // isn't worth it.
    if (pts.empty() || pts.size() <= 10) {
      return std::vector<point_t>(pts);
    }

    // dpStack.reserve(pts.size());

    point_t anchor = pts.front();
    size_t anchor_idx = 0;
    point_t floater = pts.back();
    size_t floater_idx = pts.size() - 2;
    // many items submitted to this are closed loops,
    // whose first + last points are equivalent. this
    // will cause divide by zero errors when finding distance below
    if (floater == anchor) {
      floater = pts[floater_idx - 1];
    }

    result_pts.push_back(anchor);

    StackElement elem{
        floater,
        floater_idx,
    };

    dpStack.push_back(elem);

    while (!dpStack.empty()) {

      double max_distSq = 0.0;
      point_t furthest = anchor;
      size_t furthest_idx = anchor_idx;

      // find point furthest from line seg created by (anchor, floater) and note
      // it
      for (size_t i = anchor_idx + 1; i < floater_idx; ++i) {
        const double dist = static_cast<double>(
            perpendicular_distance(pts[i], pts[anchor_idx], pts[floater_idx]));
        if (dist > max_distSq) {
          max_distSq = dist;
          furthest = pts[i];
          furthest_idx = i;
        }
      }

      // remove point if less than tolerance
      if (max_distSq <= tolerance) {
        result_pts.push_back(dpStack.back().pt);
        dpStack.pop_back();
        anchor = floater;
        anchor_idx = floater_idx;
        if (!dpStack.empty()) {
          floater = dpStack.back().pt;
          floater_idx = dpStack.back().idx;
        }
      } else {
        floater = furthest;
        floater_idx = furthest_idx;
        elem.pt = floater;
        elem.idx = floater_idx;
        dpStack.push_back(elem);
      }
    }
  }

  return result_pts;
}

#endif // RDP_H
