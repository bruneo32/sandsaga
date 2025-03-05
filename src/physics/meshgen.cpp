#include "physics.h"

#include "../util.h"
#include "RamerDouglasPeucker.hpp"
#include "earcut.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <queue>
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

#define plist_idx(i_, j_) ((j_) * list_width + (i_))

#pragma pack(push, 1)
typedef union MSQ {
	uint8_t raw;
	struct {
		bit top_left	 : 1;
		bit top_right	 : 1;
		bit bottom_left	 : 1;
		bit bottom_right : 1;
		bit __padding	 : 4; /* Alignment */
	};
} MSQ;
#pragma pack(pop)

static inline MSQ getSquareValue(uint8_t *plist, const ssize_t list_width,
								 const ssize_t list_height, ssize_t pX,
								 ssize_t pY, uint8_t current_index) {
	/* Checking the 2x2 pixel grid, assigning these values to each pixel, if
	they are the same as `current_index`.
	+---+---+
	| 1 | 2 |
	+---+---+
	| 4 | 8 | <- current pixel (pX,pY)
	+---+---+
	*/

	MSQ msq;

	msq.top_left = VERTEX_IS_IN_BOUNDS(pX - 1, pY - 1) &&
				   plist[plist_idx(pX - 1, pY - 1)] == current_index;
	msq.top_right = VERTEX_IS_IN_BOUNDS(pX, pY - 1) &&
					plist[plist_idx(pX, pY - 1)] == current_index;
	msq.bottom_left = VERTEX_IS_IN_BOUNDS(pX - 1, pY) &&
					  plist[plist_idx(pX - 1, pY)] == current_index;
	msq.bottom_right = VERTEX_IS_IN_BOUNDS(pX, pY) &&
					   plist[plist_idx(pX, pY)] == current_index;

	return msq;
}

CList *polygonlist_from_contour(uint8_t **debug_plist, ssize_t start_i,
								ssize_t end_i, ssize_t start_j, ssize_t end_j,
								bool (*is_valid)(ssize_t x, ssize_t y)) {
	const ssize_t list_width  = end_i - start_i;
	const ssize_t list_height = end_j - start_j;

	uint8_t *plist =
		(uint8_t *)calloc(list_height * list_width, sizeof(uint8_t));

	/* == Separate polygons == */
	uint8_t current_index = 1;
	for (ssize_t j = 0; j < list_height; ++j) {
		for (ssize_t i = 0; i < list_width; ++i) {
			/* Continue until we find a valid not visited cell */
			if (plist[plist_idx(i, j)] != 0 ||
				!is_valid(i + start_i, j + start_j))
				continue;

			/* New polygon */

			/* Flood fill using BFS */
			std::queue<Point> Q;
			Q.push((Point){(int)i, (int)j});

			while (!Q.empty()) {
				/* Pop from the queue */
				const Point p = Q.front();
				Q.pop();

				/* Skip invalid or already visited cells */
				if (!VERTEX_IS_IN_BOUNDS(p.x, p.y) ||
					plist[plist_idx(p.x, p.y)] != 0 ||
					!is_valid(p.x + start_i, p.y + start_j))
					continue;

				/* Set self */
				plist[plist_idx(p.x, p.y)] = current_index;

				/* Queue neighbours */
				if (p.x < list_width - 1)
					Q.push((Point){p.x + 1, p.y});
				if (p.x > 0)
					Q.push((Point){p.x - 1, p.y});
				if (p.y < list_height - 1)
					Q.push((Point){p.x, p.y + 1});
				if (p.y > 0)
					Q.push((Point){p.x, p.y - 1});
			}

			if (current_index++ == UINT8_MAX) {
				/* Overflow! It was 255 and now it's 0,
				 * so we have to stop, we cannot have more than 255 polygons */
				i = list_width;
				j = list_height;
			}
		}
	}

	/* == Discard little vertices == */
	for (ssize_t j = 0; j < list_height; ++j) {
		for (ssize_t i = 0; i < list_width; ++i) {
			uint8_t mindex = plist[plist_idx(i, j)];
			if (mindex == 0)
				continue;

			uint8_t sumup = 0;

			for (ssize_t jj = j - 1; jj <= j + 1; ++jj) {
				for (ssize_t ii = i - 1; ii <= i + 1; ++ii) {
					if (VERTEX_IS_IN_BOUNDS(ii, jj) &&
						plist[plist_idx(ii, jj)] == mindex)
						sumup++;
				}
			}

			/* Disable little vertex */
			if (sumup <= 4)
				plist[plist_idx(i, j)] = 0;
		}
	}

	/* == Get countour of every polygon using marching squares == */
	std::vector<PolyList> polylist;

	bool passed_idx[current_index] = {0};

	size_t total_paths = 0;

	for (ssize_t j = 0; j < list_height; ++j) {
		for (ssize_t i = 0; i < list_width; ++i) {
			uint8_t poly_idx = plist[plist_idx(i, j)];

			bool is_hole = false;
			if (poly_idx == 0) {
				/* Skip on the border, a valid hole should be fully enclosed */
				if (i == 0 || j == 0 || i == list_width - 1 ||
					j == list_height - 1)
					continue;

				/* Check if it's a hole inside another polygon */
				if (plist[plist_idx(i - 1, j)] != 0) {
					uint8_t parent_idx = plist[plist_idx(i - 1, j)];

					/* Throw ray: up */
					size_t cx_up = 0;
					for (ssize_t jj = j - 1; jj >= 0; --jj)
						if (plist[plist_idx(i, jj)] == parent_idx &&
							plist[plist_idx(i, jj + 1)] != parent_idx)
							cx_up++;

					/* Throw ray: right */
					size_t cx_right = 0;
					for (ssize_t ii = i + 1; ii < list_width; ++ii)
						if (plist[plist_idx(ii, j)] == parent_idx &&
							plist[plist_idx(ii - 1, j)] != parent_idx)
							cx_right++;

					/* Throw ray: down */
					size_t cx_down = 0;
					for (ssize_t jj = j + 1; jj < list_height; ++jj)
						if (plist[plist_idx(i, jj)] == parent_idx &&
							plist[plist_idx(i, jj - 1)] != parent_idx)
							cx_down++;

					/* Throw ray: left */
					size_t cx_left = 0;
					for (ssize_t ii = i - 1; ii >= 0; --ii)
						if (plist[plist_idx(ii, j)] == parent_idx &&
							plist[plist_idx(ii + 1, j)] != parent_idx)
							cx_left++;

					/* If all the rays counted an even number of vertices, then
					 * it is a hole incarcerated inside it's parent polygon,
					 * otherwise there is an open spot in the hole. */
					if ((cx_up & 1) && (cx_right & 1) && (cx_down & 1) &&
						(cx_left & 1)) {
						/* It's a hole, trace it's contour */
						is_hole	 = true;
						poly_idx = parent_idx;
					}
				}

				/* If it's not a hole, continue */
				if (!is_hole)
					continue;
			}

			/* If this is already visited, continue looking */
			if (!is_hole && passed_idx[poly_idx - 1])
				continue;

			/* New polygon */
			PolyList *pl = NULL;

			/* The first path in a polygon list is the main contour, the next
			 * ones are holes */
			if (!is_hole) {
				/* Create a new polygon list */
				pl		  = new PolyList();
				pl->index = poly_idx;
				pl->polygon.clear();
			} else {
				/* Find the polygon list to append the hole */
				for (PolyList &p : polylist) {
					if (p.index == poly_idx) {
						pl = &p;
						break;
					}
				}
			}

			if (!pl)
				continue;

			if (is_hole) {
				/* Watch out! We could be adding a hole that already
				 * exists! */
				bool hole_exists = false;
				for (size_t p = 1 /* Skip main contour */;
					 !hole_exists && p < pl->polygon.size(); ++p) {
					for (mbPoint &point : pl->polygon[p]) {
						if (point[0] == X_TO_U((double)i - 0.5) &&
							point[1] == X_TO_U((double)j - 0.5)) {
							hole_exists = true;
							break;
						}
					}
				}
				/* Do not trace a new contour if the hole already exists */
				if (hole_exists)
					continue;
			}

			/* Create new path */
			pl->polygon.push_back(std::vector<mbPoint>());
			std::vector<mbPoint> &contourVector = pl->polygon.back();
			total_paths++; /* Save counting for later */

			ssize_t startX = i;
			ssize_t startY = j;
			ssize_t pX	   = startX;
			ssize_t pY	   = startY;
			ssize_t stepX  = 0;
			ssize_t stepY  = 0;
			ssize_t prevX  = 0;
			ssize_t prevY  = 0;

			bool closedLoop = false;
			while (!closedLoop) {
				const MSQ squareValue =
					getSquareValue(plist, list_width, list_height, pX, pY,
								   (!is_hole) ? poly_idx : 0);

				switch (squareValue.raw) {
					/* Going UP with these cases:
						+---+---+   +---+---+   +---+---+
						| 1 |   |   | 1 |   |   | 1 |   |
						+---+---+   +---+---+   +---+---+
						|   |   |   | 4 |   |   | 4 | 8 |
						+---+---+  	+---+---+  	+---+---+
					*/
				case 1:
				case 5:
				case 13:
					stepX = 0;
					stepY = -1;
					break;

					/* Going DOWN with these cases:
						+---+---+   +---+---+   +---+---+
						|   |   |   |   | 2 |   | 1 | 2 |
						+---+---+   +---+---+   +---+---+
						|   | 8 |   |   | 8 |   |   | 8 |
						+---+---+  	+---+---+  	+---+---+
					*/
				case 8:
				case 10:
				case 11:
					stepX = 0;
					stepY = 1;
					break;

					/* Going LEFT with these cases:
						+---+---+   +---+---+   +---+---+
						|   |   |   |   |   |   |   | 2 |
						+---+---+   +---+---+   +---+---+
						| 4 |   |   | 4 | 8 |   | 4 | 8 |
						+---+---+  	+---+---+  	+---+---+
					*/
				case 4:
				case 12:
				case 14:
					stepX = -1;
					stepY = 0;
					break;

					/* Going RIGHT with these cases:
						+---+---+   +---+---+   +---+---+
						|   | 2 |   | 1 | 2 |   | 1 | 2 |
						+---+---+   +---+---+   +---+---+
						|   |   |   |   |   |   | 4 |   |
						+---+---+  	+---+---+  	+---+---+
					*/
				case 2:
				case 3:
				case 7:
					stepX = 1;
					stepY = 0;
					break;

				case 6:
					/* Special saddle point case 1:
						+---+---+
						|   | 2 |
						+---+---+
						| 4 |   |
						+---+---+
						going LEFT if coming from UP
						else going RIGHT
					*/
					if (prevX == 0 && prevY == -1) {
						stepX = -1;
						stepY = 0;
					} else {
						stepX = 1;
						stepY = 0;
					}
					break;

				case 9:
					/* Special saddle point case 2:
						+---+---+
						| 1 |   |
						+---+---+
						|   | 8 |
						+---+---+
						going UP if coming from RIGHT
						else going DOWN
						*/
					if (prevX == 1 && prevY == 0) {
						stepX = 0;
						stepY = -1;
					} else {
						stepX = 0;
						stepY = 1;
					}
					break;
				}

				/* Moving onto next point */
				pX += stepX;
				pY += stepY;
				/* Save contour point */
				/* This point should be (P-0.5) to match the desired grid */
				contourVector.push_back((mbPoint){
					X_TO_U((double)pX - 0.5),
					X_TO_U((double)pY - 0.5),
				});

				/* If we returned to the first point visited, the loop has
				 * finished */
				if (pX == startX && pY == startY)
					closedLoop = true;

				/* Set previous direction */
				prevX = stepX;
				prevY = stepY;
			}

			/* Contour over, set passed to true and save it */
			if (!is_hole && contourVector.size() > 2) {
				/* Reverse the order of the points is important, because the
				 * Emanuele's code generates a counter-clockwise polygon, which
				 * has its normals pointing inside the polygon, this is cool for
				 * holes, but for the main contour we want the order of the
				 * points clockwise. */
				std::reverse(contourVector.begin(), contourVector.end());
				passed_idx[poly_idx - 1] = true;
				polylist.push_back(*pl);
			}
		}
	}

	/* Reduce number of points in each polygon */
	for (PolyList &poly : polylist) {
		for (std::vector<mbPoint> &path : poly.polygon) {
			std::vector<RDP::Point> vIn;
			std::vector<RDP::Point> vOut;

			for (mbPoint &point : path)
				vIn.push_back(RDP::Point(point[0], point[1]));

			RDP::RamerDouglasPeucker(vIn, 0.24, vOut);

			path.clear();
			for (RDP::Point &point : vOut)
				path.push_back(mbPoint{point.first, point.second});
		}
	}

	/* Prepare result */
	CList *result = (CList *)malloc(sizeof(CList));
	result->count = 0;

	result->data = (void **)malloc(total_paths * sizeof(void *));

	for (const PolyList &poly : polylist) {
		/* Append holes as separate polygons, since this is a chain of vertices
		 * and not a triangulation, it doesn't matter */
		for (const std::vector<mbPoint> &path : poly.polygon) {
			if (path.empty())
				continue;

			/* Only take the first path, which is the main contour ignoring
			 * holes */

			PointList *pointlist = (PointList *)malloc(sizeof(PointList));
			pointlist->count	 = 0;
			pointlist->points =
				(Point2D *)malloc(path.size() * sizeof(Point2D));

			for (const mbPoint &point : path) {
				pointlist->points[pointlist->count].x	= point[0];
				pointlist->points[pointlist->count++].y = point[1];
			}

			result->data[result->count++] = pointlist;
		}
	}

	/* Free stuff */
	if (debug_plist)
		/* FIXME: Remove debug_plist */
		*debug_plist = plist;
	else
		free(plist);

	return result;
}

void center_pointlist(PointList *mesh, double centroid_u, double centroid_v) {
	if (!mesh || mesh->count == 0 || !mesh->points)
		return;

	/* Map vertex coordinates to centroid based coordinates */
	for (size_t i = 0; i < mesh->count; ++i) {
		Point2D *t = &mesh->points[i];

		t->x = t->x - centroid_u;
		t->y = t->y - centroid_v;
	}
}

void center_pointlist_auto(PointList *mesh, double *centroid_u,
						   double *centroid_v) {
	if (!mesh || mesh->count == 0 || !mesh->points)
		return;

	/* Calculate centroid */
	double c_x = 0;
	double c_y = 0;
	for (size_t i = 0; i < mesh->count; ++i) {
		Point2D *p = &mesh->points[i];
		c_x += p->x;
		c_y += p->y;
	}
	c_x /= mesh->count;
	c_y /= mesh->count;

	if (centroid_u) {
		*centroid_u = c_x;
	}
	if (centroid_v)
		*centroid_v = c_y;

	center_pointlist(mesh, c_x, c_y);
}
