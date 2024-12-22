#include "physics.h"

#include "../util.h"
#include "RamerDouglasPeucker.hpp"
#include "earcut.hpp"

#include <array>
#include <iostream>
#include <vector>

using mbPoint = std::array<double, 2>;

/** The first array is the main contour, and the following arrays are holes */
using mbPolygon = std::vector<std::vector<mbPoint>>;

typedef struct _PolyList {
	uint8_t	  index;
	mbPolygon polygon;
} PolyList;

#define Get_plist(plist_, i_, j_)  (plist_[(j_) * list_width + (i_)])
#define INVALID_VERTEX			   ((uint8_t)(-1))
#define VERTEX_IS_IN_BOUNDS_H(__x) (((__x) >= 0) && ((__x) < list_width))
#define VERTEX_IS_IN_BOUNDS_V(__y) (((__y) >= 0) && ((__y) < list_height))
#define VERTEX_IS_IN_BOUNDS(__x, __y)                                          \
	(VERTEX_IS_IN_BOUNDS_H(__x) && VERTEX_IS_IN_BOUNDS_V(__y))

typedef struct _CloudPoint {
	ssize_t i;
	ssize_t j;
	size_t	order;
} CloudPoint;

/* The following LookUpTables are used to encode the surrounding vertices of a
 * vertex (x) in the following bit pattern.
 * (0b76543210)
 *     765
 *     4x3
 *     210
 */

const uint8_t LUT_MSQ_VALID_VERTEX[] = {
	/* Tiny incisions */
	0b10111111,
	0b11101111,
	0b11110111,
	0b11111101,
	/* Inside vertices */
	0b01111111,
	0b11011111,
	0b11111011,
	0b11111110,
};
const uint8_t LUT_MSQR_VALID[] = {
	/* 90deg corners */
	0b01010000,
	0b01001000,
	0b00010010,
	0b00001010,
	/* Semi-corners */
	0b11001000,
	0b01110000,
	0b00001110,
	0b00010011,
	0b10010010,
	0b00101010,
	0b01010100,
	0b01001001,
	/* Tiny outlets */
	0b01011000,
	0b00011010,
	0b01001010,
	0b01010010,
};

std::vector<mbPoint> traverse_contour(uint8_t *plist, const size_t list_width,
									  const size_t list_height,
									  const size_t start_j,
									  const size_t start_i,
									  uint8_t	   polygon_index) {

	std::vector<mbPoint>	result = std::vector<mbPoint>();
	std::vector<CloudPoint> revisit_stack;

	/* Define directions clockwise */
	enum e_cwdir {
		cw_first		 = 1,
		cw_startfrom_top = 1,
		cw_startfrom_right,
		cw_startfrom_bottom,
		cw_startfrom_left,
		/* If it does not encounter a valid "+" vertex,
		 * go try for "x" vertex. */
		cw_first_x			 = 10,
		cw_startfrom_topleft = 10,
		cw_startfrom_topright,
		cw_startfrom_botright,
		cw_startfrom_botleft,
	};

	ssize_t j = start_j;
	ssize_t i = start_i;

	/* First iter comes from right, so start searching clockwise from
	 * top-left */
	size_t order		  = cw_startfrom_top;
	bool   already_passed = false;
	bool   first_time	  = true;
	while (!(!first_time && i == start_i && j == start_j)) {
		/* Closing polygon, break */

		/* Save point (do not add double points when not needed) */
		if (!already_passed) {
			if (!first_time)
				plist[j * list_width + i] = polygon_index;

			/* Do not unset the first vertex, because we have to detect it at
			 * the end */
			result.push_back((mbPoint){
				static_cast<double>(i),
				static_cast<double>(j),
			});

			if (!first_time) {
				/* Save checkpoint if there is a crosspath */
				uint8_t plus_neighbours = 0, x_neighbours = 0;

				/* Define "+" and "x" direction offsets */
				const ssize_t plus_offsets[4][2] = {
					{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
				const ssize_t cross_offsets[4][2] = {
					{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
				/* Check "+" neighbors */
				for (const auto &offset : plus_offsets) {
					ssize_t ni = i + offset[1], nj = j + offset[0];
					if (VERTEX_IS_IN_BOUNDS(ni, nj) &&
						Get_plist(plist, ni, nj) == INVALID_VERTEX) {
						plus_neighbours++;
					}
				}
				/* Check "x" neighbors */
				for (const auto &offset : cross_offsets) {
					ssize_t ni = i + offset[1], nj = j + offset[0];
					if (VERTEX_IS_IN_BOUNDS(ni, nj) &&
						Get_plist(plist, ni, nj) == INVALID_VERTEX) {
						x_neighbours++;
					}
				}

				/* Validate crosspath, it should not save a point that has two
				 * neighbours but one on each side, the neighbour count should
				 * be on each side independently */
				if (plus_neighbours > 1 || x_neighbours > 1)
					revisit_stack.push_back((CloudPoint){i, j, order});
			}

			first_time = false;
		}

		/* Iter neighbours in order */
		switch (order) {
		case cw_startfrom_top:
			/* Check top */
			if (VERTEX_IS_IN_BOUNDS_V(j - 1) &&
				Get_plist(plist, i, j - 1) == INVALID_VERTEX) {
				j--;
				order		   = cw_startfrom_left;
				already_passed = false;
				continue;
			}
		case cw_startfrom_right:
			/* Check right */
			if (VERTEX_IS_IN_BOUNDS_H(i + 1) &&
				Get_plist(plist, i + 1, j) == INVALID_VERTEX) {
				i++;
				order		   = cw_startfrom_top;
				already_passed = false;
				continue;
			}
		case cw_startfrom_bottom:
			/* Check bottom */
			if (VERTEX_IS_IN_BOUNDS_V(j + 1) &&
				Get_plist(plist, i, j + 1) == INVALID_VERTEX) {
				j++;
				order		   = cw_startfrom_right;
				already_passed = false;
				continue;
			}
		case cw_startfrom_left:
			if (VERTEX_IS_IN_BOUNDS_H(i - 1) &&
				Get_plist(plist, i - 1, j) == INVALID_VERTEX) {
				/* Check left */
				i--;
				order		   = cw_startfrom_bottom;
				already_passed = false;
				continue;
			}

			/* Check if cross is already visited */
			if (!already_passed) {
				already_passed = true;
				order		   = cw_first;
				continue;
			}

			/* Turn for the X shape */
		case cw_startfrom_topleft:
			if (VERTEX_IS_IN_BOUNDS(i - 1, j - 1) &&
				Get_plist(plist, i - 1, j - 1) == INVALID_VERTEX) {
				/* Check top-left */
				i--;
				j--;
				order		   = cw_startfrom_bottom;
				already_passed = false;
				continue;
			}
		case cw_startfrom_topright:
			if (VERTEX_IS_IN_BOUNDS(i + 1, j - 1) &&
				Get_plist(plist, i + 1, j - 1) == INVALID_VERTEX) {
				/* Check top-right */
				i++;
				j--;
				order		   = cw_startfrom_left;
				already_passed = false;
				continue;
			}
		case cw_startfrom_botright:
			if (VERTEX_IS_IN_BOUNDS(i + 1, j + 1) &&
				Get_plist(plist, i + 1, j + 1) == INVALID_VERTEX) {
				/* Check bottom-right */
				i++;
				j++;
				order		   = cw_startfrom_top;
				already_passed = false;
				continue;
			}
		case cw_startfrom_botleft:
			if (VERTEX_IS_IN_BOUNDS(i - 1, j + 1) &&
				Get_plist(plist, i - 1, j + 1) == INVALID_VERTEX) {
				/* Check bottom-left */
				i--;
				j++;
				order		   = cw_startfrom_right;
				already_passed = false;
				continue;
			}
		default:
			/* If no shape was able to find a valid vertex, then go for a
			 * revisit, or shut it down */
			if (!revisit_stack.empty()) {
				CloudPoint revisit = revisit_stack.back();

				i	  = revisit.i;
				j	  = revisit.j;
				order = revisit.order;

				already_passed = false;
				revisit_stack.pop_back();
				continue;
			}

			/* So, polygon not closed, kill */
			i = start_i;
			j = start_j;
			break;
		}
	}

	revisit_stack.clear();
	result.shrink_to_fit();

	return result;
}

std::vector<PolyList> create_pointlist_from_contour(
	size_t start_i, size_t end_i, size_t start_j, size_t end_j,
	bool (*is_valid)(size_t y, size_t x), uint8_t **contour_output) {

	const size_t list_width	 = end_i - start_i;
	const size_t list_height = end_j - start_j;

	uint8_t *plist =
		(uint8_t *)calloc(list_height * list_width, sizeof(uint8_t));

	for (size_t j = 0; j < list_height; ++j) {
		for (size_t i = 0; i < list_width; ++i) {
			const size_t centerv = j + start_j;
			const size_t centerh = i + start_i;

			if (!is_valid(centerv, centerh)) {
				plist[j * list_width + i] = 0;
				continue;
			}

			const size_t top	= centerv - 1;
			const size_t bottom = centerv + 1;
			const size_t left	= centerh - 1;
			const size_t right	= centerh + 1;

			uint8_t checksum =
				(VERTEX_IS_IN_BOUNDS(i - 1, j - 1) ? is_valid(top, left) << 7
												   : 0) |
				(VERTEX_IS_IN_BOUNDS(i, j - 1) ? is_valid(top, centerh) << 6
											   : 0) |
				(VERTEX_IS_IN_BOUNDS(i + 1, j - 1) ? is_valid(top, right) << 5
												   : 0) |
				(VERTEX_IS_IN_BOUNDS(i - 1, j) ? is_valid(centerv, left) << 4
											   : 0) |
				(VERTEX_IS_IN_BOUNDS(i + 1, j) ? is_valid(centerv, right) << 3
											   : 0) |
				(VERTEX_IS_IN_BOUNDS(i - 1, j + 1) ? is_valid(bottom, left) << 2
												   : 0) |
				(VERTEX_IS_IN_BOUNDS(i, j + 1) ? is_valid(bottom, centerh) << 1
											   : 0) |
				(VERTEX_IS_IN_BOUNDS(i + 1, j + 1) ? is_valid(bottom, right)
												   : 0);

			size_t sumup = 0;
			for (size_t bi = 0; bi < sizeof(checksum) * 8; bi++) {
				if (checksum & BIT(bi))
					sumup++;
			}

			if (VERTEX_IS_IN_BOUNDS_V(j - 1) && VERTEX_IS_IN_BOUNDS_V(j + 1) &&
				VERTEX_IS_IN_BOUNDS_H(i - 1) && VERTEX_IS_IN_BOUNDS_H(i + 1) &&
				sumup > 6) {
				uint8_t matched = 0;
				for (uint8_t lookupItem : LUT_MSQ_VALID_VERTEX) {
					if (checksum == lookupItem) {
						matched = INVALID_VERTEX;
						break;
					}
				}

				plist[j * list_width + i] = matched;
				continue;
			}

			/* Only mark as border if one of the surroundings is not solid
			 */
			plist[j * list_width + i] = INVALID_VERTEX;
		}
	}

	/* Separate each polygon inside the cloud */

	std::vector<PolyList> result;

	size_t	prev		 = 0;
	uint8_t curr_idx	 = 0;
	uint8_t outside_poly = 0;
	for (ssize_t j = 0; j < list_height; ++j) {
		/* Ray as line from 0 to i */
		size_t h_vertex_count = 0;

		for (ssize_t i = 0; i < list_width; ++i) {
			uint8_t current = Get_plist(plist, i, j);

			if (current == 0) {
				prev = current;
				continue;
			}
			if (current != INVALID_VERTEX) {
				if (prev != current)
					h_vertex_count++;
				prev		 = current;
				outside_poly = prev;
				continue;
			}

			/* Vertices inside polygon are vertices of that polygon */
			/* Given a ray in any direction towards the border, if the count
			 * of intersections is odd, it is inside another polygon */
			if (h_vertex_count % 2 != 0) {
				/* Append new list to polylist with index outside_poly */
				for (PolyList pl : result) {

					if (pl.index == outside_poly) {
						std::vector<mbPoint> newHole = traverse_contour(
							plist, list_width, list_height, j, i, outside_poly);

						pl.polygon.push_back(newHole);

						break;
					}
				}

				prev = 0;
				continue;
			}

			/* Iterate polygon */
			++curr_idx;

			std::vector<mbPoint> mainContour = traverse_contour(
				plist, list_width, list_height, j, i, curr_idx);

			/* The first array of a polygon it's the contour, and the
			 * following are holes */
			result.push_back((PolyList){
				curr_idx,
				mbPolygon{mainContour},
			});

			h_vertex_count++;
			prev = curr_idx;
		}
	}

	/* Select important vertices */
	for (PolyList &poly : result)
		for (std::vector<mbPoint> &path : poly.polygon) {
			for (auto it = path.begin(); it != path.end();) {
				const size_t i = static_cast<size_t>(it->at(0));
				const size_t j = static_cast<size_t>(it->at(1));

				/* Don't erase subchunk edges */
				if (i == 0 || j == 0 || i == list_width - 1 ||
					j == list_height - 1) {
					++it;
					continue;
				}

				const size_t top	= j - 1;
				const size_t bottom = j + 1;
				const size_t left	= i - 1;
				const size_t right	= i + 1;

				uint8_t checksum =
					(VERTEX_IS_IN_BOUNDS(left, top)
						 ? (Get_plist(plist, left, top) == poly.index) << 7
						 : 0) |
					(VERTEX_IS_IN_BOUNDS(i, top)
						 ? (Get_plist(plist, i, top) == poly.index) << 6
						 : 0) |
					(VERTEX_IS_IN_BOUNDS(right, top)
						 ? (Get_plist(plist, right, top) == poly.index) << 5
						 : 0) |
					(VERTEX_IS_IN_BOUNDS(left, j)
						 ? (Get_plist(plist, left, j) == poly.index) << 4
						 : 0) |
					(VERTEX_IS_IN_BOUNDS(right, j)
						 ? (Get_plist(plist, right, j) == poly.index) << 3
						 : 0) |
					(VERTEX_IS_IN_BOUNDS(left, bottom)
						 ? (Get_plist(plist, left, bottom) == poly.index) << 2
						 : 0) |
					(VERTEX_IS_IN_BOUNDS(i, bottom)
						 ? (Get_plist(plist, i, bottom) == poly.index) << 1
						 : 0) |
					(VERTEX_IS_IN_BOUNDS(right, bottom)
						 ? (Get_plist(plist, right, bottom) == poly.index)
						 : 0);

				size_t sumup = 0;
				for (size_t bi = 0; bi < sizeof(checksum) * 8; bi++) {
					if (checksum & BIT(bi))
						sumup++;
				}

				if (sumup < 2 || sumup > 4) {
					/* All narrow lines, or a lot of intersections are valid */
					++it;
					continue;
				}

				/* Verify corner */
				bool valid_corner = false;
				for (uint8 lut : LUT_MSQR_VALID) {
					if (checksum == lut) {
						valid_corner = true;
						break;
					}
				}

				if (valid_corner) {
					++it;
					continue;
				}

				/* Invalid, destroy */
				it = path.erase(it);
			}
		}

	/* Draw */
	memset(plist, 0, list_width * list_height);

	for (PolyList &polygon : result)
		for (std::vector<mbPoint> &path : polygon.polygon) {
			for (mbPoint &point : path) {
				const size_t i = static_cast<size_t>(point.at(0));
				const size_t j = static_cast<size_t>(point.at(1));

				plist[j * list_width + i] = polygon.index;
			}
		}

	if (!contour_output)
		free(plist);
	else
		*contour_output = plist;

	return result;
}

std::vector<PolyList>
rdp_simplify_from_contour(size_t start_i, size_t end_i, size_t start_j,
						  size_t	end_j, bool (*is_valid)(size_t x, size_t y),
						  uint8_t **contour_output) {

	std::vector<PolyList> inputPoints = create_pointlist_from_contour(
		start_i, end_i, start_j, end_j, is_valid, contour_output);

	std::vector<PolyList> result;

	/* Apply RDP to each path of each polylist */
	for (PolyList plist : inputPoints) {
		mbPolygon polyIn  = plist.polygon;
		mbPolygon polyOut = std::vector<std::vector<mbPoint>>();

		for (std::vector<mbPoint> path : polyIn) {
			std::vector<RDP::Point> vIn;
			std::vector<RDP::Point> vOut;

			for (mbPoint point : path)
				vIn.push_back(RDP::Point(point[0], point[1]));

			/* More than 1 oftens produces some gaps in b2ChainShape */
			RDP::RamerDouglasPeucker(vIn, 1.0, vOut);

			if (vOut.empty()) {
				continue;
			}

			std::vector<mbPoint> rOut;
			for (RDP::Point p : vOut) {
				rOut.push_back(mbPoint{p.first, p.second});
			}

			polyOut.push_back(rOut);
		}

		result.push_back((PolyList){plist.index, polyOut});
	}

	return result;
}

TriangleMesh *triangulate(size_t start_i, size_t end_i, size_t start_j,
						  size_t end_j, bool (*is_valid)(size_t x, size_t y)) {

	std::vector<PolyList> polylist = rdp_simplify_from_contour(
		start_i, end_i, start_j, end_j, is_valid, NULL);

	if (polylist.empty())
		return NULL;

	TriangleMesh *mesh = (TriangleMesh *)malloc(sizeof(TriangleMesh));
	mesh->triangles	   = (Triangle *)malloc(1024 * sizeof(Triangle));
	mesh->count		   = 0;

	for (PolyList plist : polylist) {
		if (plist.polygon.empty() || plist.polygon[0].size() < 3) {
			continue; // Skip invalid polygons
		}

		// Run tessellation
		// Returns array of indices that refer to the vertices of the input
		// polygon.
		// Three subsequent indices form a triangle. Output triangles are
		// clockwise.
		std::vector<uint32_t> indices = mapbox::earcut<uint32_t>(plist.polygon);

		/* Flatten the polygon into a single vector for easy indexing */
		std::vector<mbPoint> flattened;
		for (std::vector<mbPoint> path : plist.polygon) {
			flattened.insert(flattened.end(), path.begin(), path.end());
			path.clear();
		}

		for (size_t j = 0; j < indices.size(); j += 3) {
			Triangle *t = &mesh->triangles[mesh->count++];

			mbPoint p1 = flattened[indices[j]];
			mbPoint p2 = flattened[indices[j + 1]];
			mbPoint p3 = flattened[indices[j + 2]];

			t->p1.x = p1[0];
			t->p1.y = p1[1];
			t->p2.x = p2[0];
			t->p2.y = p2[1];
			t->p3.x = p3[0];
			t->p3.y = p3[1];
		}

		flattened.clear();
		indices.clear();
		plist.polygon.clear();
	}

	polylist.clear();

	return mesh;
}

CList *loopchain_from_contour(size_t start_i, size_t end_i, size_t start_j,
							  size_t end_j,
							  bool (*is_valid)(size_t x, size_t y),
							  uint8_t **contour_output) {

	std::vector<PolyList> inputPoints = rdp_simplify_from_contour(
		start_i, end_i, start_j, end_j, is_valid, contour_output);

	if (inputPoints.empty())
		return NULL;

	CList *result = (CList *)malloc(sizeof(CList));
	result->count = 0;
	result->data  = (void **)malloc(inputPoints.size() * sizeof(void *));

	for (PolyList plist : inputPoints) {
		if (plist.polygon.empty() || plist.polygon[0].empty())
			continue;

		/* Only take the first path, which is the main contour ignoring
		 * holes */
		std::vector<mbPoint> firstPath = plist.polygon[0];
		if (firstPath.empty())
			continue;

		PointList *pointlist = (PointList *)malloc(sizeof(PointList));
		pointlist->count	 = 0;
		pointlist->points =
			(Point2D *)malloc(firstPath.size() * sizeof(Point2D));

		for (mbPoint point : firstPath) {
			pointlist->points[pointlist->count].x	= point[0];
			pointlist->points[pointlist->count++].y = point[1];
		}

		result->data[result->count++] = pointlist;
	}

	return result;
}

void convert_triangle_to_box2d_units(TriangleMesh *mesh, double *centroid_x,
									 double *centroid_y) {
	if (!mesh || mesh->count == 0 || !mesh->triangles)
		return;

	/* Calculate centroid */
	double c_x = 0;
	double c_y = 0;
	if (!centroid_x || !centroid_y) {
		for (size_t i = 0; i < mesh->count; ++i) {
			Triangle *t = &mesh->triangles[i];
			c_x += (t->p1.x + t->p2.x + t->p3.x) / 3;
			c_y += (t->p1.y + t->p2.y + t->p3.y) / 3;
		}
		c_x /= mesh->count;
		c_y /= mesh->count;
	} else {
		c_x = *centroid_x;
		c_y = *centroid_y;
	}

	/* Map triangle coordinates to centroid based coordinates, converted to
	 * box2d units */
	for (size_t i = 0; i < mesh->count; ++i) {
		Triangle *t = &mesh->triangles[i];

		t->p1.x = X_TO_U(t->p1.x - c_x);
		t->p1.y = X_TO_U(t->p1.y - c_y);
		t->p2.x = X_TO_U(t->p2.x - c_x);
		t->p2.y = X_TO_U(t->p2.y - c_y);
		t->p3.x = X_TO_U(t->p3.x - c_x);
		t->p3.y = X_TO_U(t->p3.y - c_y);
	}
}

void convert_pointlist_to_box2d_units(PointList *mesh, double *centroid_x,
									  double *centroid_y) {
	if (!mesh || mesh->count == 0 || !mesh->points)
		return;

	/* Calculate centroid */
	double c_x = 0;
	double c_y = 0;
	if (!centroid_x || !centroid_y) {
		for (size_t i = 0; i < mesh->count; ++i) {
			Point2D *p = &mesh->points[i];
			c_x += p->x;
			c_y += p->y;
		}
		c_x /= mesh->count;
		c_y /= mesh->count;
	} else {
		c_x = *centroid_x;
		c_y = *centroid_y;
	}

	/* Map triangle coordinates to centroid based coordinates, converted to
	 * box2d units */
	for (size_t i = 0; i < mesh->count; ++i) {
		Point2D *t = &mesh->points[i];

		t->x = X_TO_U(t->x - c_x);
		t->y = X_TO_U(t->y - c_y);
	}
}
