#ifndef _RAMERDOUGLASPEUCKER_H
#define _RAMERDOUGLASPEUCKER_H

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace RDP {

typedef std::pair<double, double> Point;

double PerpendicularDistance(const RDP::Point &pt, const RDP::Point &lineStart,
							 const RDP::Point &lineEnd);

void RamerDouglasPeucker(const std::vector<RDP::Point> &pointList,
						 double epsilon, std::vector<RDP::Point> &out);

}; // namespace RDP

#endif // _RAMERDOUGLASPEUCKER_H
