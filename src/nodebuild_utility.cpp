/*
** nodebuild_utility.cpp
**
** Miscellaneous node builder utility functions.
**
**---------------------------------------------------------------------------
** Copyright 2002 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
** 4. When not used as part of ZDoom or a ZDoom derivative, this code will be
**    covered by the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or (at
**    your option) any later version.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include <stdlib.h>
#ifdef _MSC_VER
#include <malloc.h>
#endif
#include <string.h>
#include <stdio.h>

#include "nodebuild.h"
#include "m_bbox.h"
#include "r_main.h"
#include "i_system.h"

static const int PO_LINE_START = 1;
static const int PO_LINE_EXPLICIT = 5;

#if 0
#define D(x) x
#else
#define D(x) do{}while(0)
#endif

#if 0
#define P(x) x
#else
#define P(x) do{}while(0)
#endif

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

angle_t FNodeBuilder::PointToAngle (fixed_t x, fixed_t y)
{
	const double rad2bam = double(1<<30) / M_PI;
	double ang = atan2 (double(y), double(x));
	if (ang < 0.0)
	{
		ang = 2*M_PI+ang;
	}
	return angle_t(ang * rad2bam) << 1;
}

void FNodeBuilder::FindUsedVertices (vertex_t *oldverts, int max)
{
	size_t *map = (size_t *)alloca (max*sizeof(size_t));
	int i;
	FPrivVert newvert;

	memset (&map[0], -1, sizeof(size_t)*max);

	newvert.segs = DWORD_MAX;
	newvert.segs2 = DWORD_MAX;

	for (i = 0; i < Level.NumLines; ++i)
	{
		int v1 = Level.Lines[i].v1 - oldverts;
		int v2 = Level.Lines[i].v2 - oldverts;

		if (map[v1] == (size_t)-1)
		{
			newvert.x = oldverts[v1].x;
			newvert.y = oldverts[v1].y;
			map[v1] = SelectVertexExact (newvert);
		}
		if (map[v2] == (size_t)-1)
		{
			newvert.x = oldverts[v2].x;
			newvert.y = oldverts[v2].y;
			map[v2] = SelectVertexExact (newvert);
		}

		Level.Lines[i].v1 = (vertex_t *)map[v1];
		Level.Lines[i].v2 = (vertex_t *)map[v2];
	}
}

int FNodeBuilder::SelectVertexExact (FPrivVert &vertex)
{
	for (size_t i = 0; i < Vertices.Size(); ++i)
	{
		if (Vertices[i].x == vertex.x && Vertices[i].y == vertex.y)
		{
			return (int)i;
		}
	}
	return (int)Vertices.Push (vertex);
}

// For every sidedef in the map, create a corresponding seg.

void FNodeBuilder::MakeSegsFromSides ()
{
	FPrivSeg *share1, *share2;
	FPrivSeg seg;
	int i, j;

	seg.next = DWORD_MAX;
	seg.loopnum = 0;
	seg.partner = DWORD_MAX;

	if (Level.NumLines == 0)
	{
		I_Error ("Map is empty.\n");
	}

	for (i = 0; i < Level.NumLines; ++i)
	{
		share1 = NULL;
		if (Level.Lines[i].sidenum[0] != NO_INDEX)
		{
			WORD backside;

			seg.linedef = i;
			seg.sidedef = Level.Lines[i].sidenum[0];
			backside = Level.Lines[i].sidenum[1];
			seg.frontsector = Level.Lines[i].frontsector;
			seg.backsector = Level.Lines[i].backsector;
			seg.v1 = (size_t)Level.Lines[i].v1;
			seg.v2 = (size_t)Level.Lines[i].v2;
			seg.nextforvert = Vertices[seg.v1].segs;
			seg.nextforvert2 = Vertices[seg.v2].segs2;
			share1 = CheckSegForDuplicate (&seg);
			if (share1 == NULL)
			{
				j = (int)Segs.Push (seg);
				Vertices[seg.v1].segs = j;
				Vertices[seg.v2].segs2 = j;
			}
			else
			{
				Printf ("Linedefs %d and %d share endpoints.\n", i, share1->linedef);
			}
		}
		else
		{
			Printf ("Linedef %d does not have a front side.\n", i);
		}

		if (Level.Lines[i].sidenum[1] != NO_INDEX)
		{
			WORD backside;

			seg.linedef = i;
			seg.sidedef = Level.Lines[i].sidenum[1];
			backside = Level.Lines[i].sidenum[0];
			seg.frontsector = Level.Lines[i].backsector;
			seg.backsector = Level.Lines[i].frontsector;
			seg.v1 = (size_t)Level.Lines[i].v2;
			seg.v2 = (size_t)Level.Lines[i].v1;
			seg.nextforvert = Vertices[seg.v1].segs;
			seg.nextforvert2 = Vertices[seg.v2].segs2;
			share2 = CheckSegForDuplicate (&seg);
			if (share2 == NULL)
			{
				j = (int)Segs.Push (seg);
				Vertices[seg.v1].segs = j;
				Vertices[seg.v2].segs2 = j;

				if (Level.Lines[i].sidenum[0] != NO_INDEX && share1 == NULL)
				{
					Segs[j-1].partner = j;
					Segs[j].partner = j-1;
				}
			}
			else if (share2->linedef != share1->linedef)
			{
				Printf ("Linedefs %d and %d share endpoints.\n", i, share2->linedef);
			}
		}
	}
}

// Check for another seg with the same start and end vertices as this one.
// Combined with its use above, this will find two-sided lines that are shadowed
// by another one- or two-sided line, and it will also find one-sided lines that
// shadow each other. It will not find one-sided lines that share endpoints but
// face opposite directions. Although they should probably be a single two-sided
// line, leaving them in will not generate bad nodes.

FNodeBuilder::FPrivSeg *FNodeBuilder::CheckSegForDuplicate (const FNodeBuilder::FPrivSeg *check)
{
	DWORD segnum;

	// Check for segs facing the same direction
	for (segnum = check->nextforvert; segnum != DWORD_MAX; segnum = Segs[segnum].nextforvert)
	{
		if (Segs[segnum].v2 == check->v2)
		{
			return &Segs[segnum];
		}
	}
	return NULL;
}

// Group colinear segs together so that only one seg per line needs to be checked
// by SelectSplitter().

void FNodeBuilder::GroupSegPlanes ()
{
	const int bucketbits = 12;
	FPrivSeg *buckets[1<<bucketbits] = { 0 };
	int i, planenum;

	for (i = 0; i < (int)Segs.Size(); ++i)
	{
		FPrivSeg *seg = &Segs[i];
		seg->next = i+1;
		seg->hashnext = NULL;
	}

	Segs[Segs.Size()-1].next = DWORD_MAX;

	for (i = planenum = 0; i < (int)Segs.Size(); ++i)
	{
		FPrivSeg *seg = &Segs[i];
		fixed_t x1 = Vertices[seg->v1].x;
		fixed_t y1 = Vertices[seg->v1].y;
		fixed_t x2 = Vertices[seg->v2].x;
		fixed_t y2 = Vertices[seg->v2].y;
		angle_t ang = PointToAngle (x2 - x1, y2 - y1);

		if (ang >= 1u<<31)
			ang += 1u<<31;

		FPrivSeg *check = buckets[ang >>= 31-bucketbits];

		while (check != NULL)
		{
			fixed_t cx1 = Vertices[check->v1].x;
			fixed_t cy1 = Vertices[check->v1].y;
			fixed_t cdx = Vertices[check->v2].x - cx1;
			fixed_t cdy = Vertices[check->v2].y - cy1;
			if (PointOnSide (x1, y1, cx1, cy1, cdx, cdy) == 0 &&
				PointOnSide (x2, y2, cx1, cy1, cdx, cdy) == 0)
			{
				break;
			}
			check = check->hashnext;
		}
		if (check != NULL)
		{
			seg->planenum = check->planenum;
			const FSimpleLine *line = &Planes[seg->planenum];
			if (line->dx != 0)
			{
				if ((line->dx > 0 && x2 > x1) || (line->dx < 0 && x2 < x1))
				{
					seg->planefront = true;
				}
				else
				{
					seg->planefront = false;
				}
			}
			else
			{
				if ((line->dy > 0 && y2 > y1) || (line->dy < 0 && y2 < y1))
				{
					seg->planefront = true;
				}
				else
				{
					seg->planefront = false;
				}
			}
		}
		else
		{
			seg->hashnext = buckets[ang];
			buckets[ang] = seg;
			seg->planenum = planenum++;
			seg->planefront = true;

			FSimpleLine pline = { Vertices[seg->v1].x,
								  Vertices[seg->v1].y,
								  Vertices[seg->v2].x - Vertices[seg->v1].x,
								  Vertices[seg->v2].y - Vertices[seg->v1].y };
			Planes.Push (pline);
		}
	}

	D(Printf ("%d planes from %d segs\n", planenum, Segs.Size()));

	planenum = (planenum+7)/8;
	PlaneChecked.Reserve (planenum);
}

// Find "loops" of segs surrounding polyobject's origin. Note that a polyobject's origin
// is not solely defined by the polyobject's anchor, but also by the polyobject itself.
// For the split avoidance to work properly, you must have a convex, complete loop of
// segs surrounding the polyobject origin. All the maps in hexen.wad have complete loops of
// segs around their polyobjects, but they are not all convex: The doors at the start of MAP01
// and some of the pillars in MAP02 that surround the entrance to MAP06 are not convex.
// Heuristic() uses some special weighting to make these cases work properly.

void FNodeBuilder::FindPolyContainers (TArray<FPolyStart> &spots, TArray<FPolyStart> &anchors)
{
	int loop = 1;

	for (size_t i = 0; i < spots.Size(); ++i)
	{
		FPolyStart *spot = &spots[i];
		fixed_t bbox[4];

		if (GetPolyExtents (spot->polynum, bbox))
		{
			FPolyStart *anchor;

			size_t j;

			for (j = 0; j < anchors.Size(); ++j)
			{
				anchor = &anchors[j];
				if (anchor->polynum == spot->polynum)
				{
					break;
				}
			}

			if (j < anchors.Size())
			{
				vertex_t mid;
				vertex_t center;

				mid.x = bbox[BOXLEFT] + (bbox[BOXRIGHT]-bbox[BOXLEFT])/2;
				mid.y = bbox[BOXBOTTOM] + (bbox[BOXTOP]-bbox[BOXBOTTOM])/2;

				center.x = mid.x - anchor->x + spot->x;
				center.y = mid.y - anchor->y + spot->y;

				// Scan right for the seg closest to the polyobject's center after it
				// gets moved to its start spot.
				fixed_t closestdist = FIXED_MAX;
				DWORD closestseg = 0;

				P(Printf ("start %d,%d -- center %d, %d\n", spot->x>>16, spot->y>>16, center.x>>16, center.y>>16));

				for (size_t j = 0; j < Segs.Size(); ++j)
				{
					FPrivSeg *seg = &Segs[j];
					FPrivVert *v1 = &Vertices[seg->v1];
					FPrivVert *v2 = &Vertices[seg->v2];
					fixed_t dy = v2->y - v1->y;

					if (dy == 0)
					{ // Horizontal, so skip it
						continue;
					}
					if ((v1->y < center.y && v2->y < center.y) || (v1->y > center.y && v2->y > center.y))
					{ // Not crossed
						continue;
					}

					fixed_t dx = v2->x - v1->x;

					if (PointOnSide (center.x, center.y, v1->x, v1->y, dx, dy) <= 0)
					{
						fixed_t t = DivScale30 (center.y - v1->y, dy);
						fixed_t sx = v1->x + MulScale30 (dx, t);
						fixed_t dist = sx - spot->x;

						if (dist < closestdist && dist >= 0)
						{
							closestdist = dist;
							closestseg = (long)j;
						}
					}
				}
				if (closestseg >= 0)
				{
					loop = MarkLoop (closestseg, loop);
					P(Printf ("Found polyobj in sector %d (loop %d)\n", Segs[closestseg].frontsector,
						Segs[closestseg].loopnum));
				}
			}
		}
	}
}

int FNodeBuilder::MarkLoop (DWORD firstseg, int loopnum)
{
	DWORD seg;
	sector_t *sec = Segs[firstseg].frontsector;

	if (Segs[firstseg].loopnum != 0)
	{ // already marked
		return loopnum;
	}

	seg = firstseg;

	do
	{
		FPrivSeg *s1 = &Segs[seg];

		s1->loopnum = loopnum;

		P(Printf ("Mark seg %d (%d,%d)-(%d,%d)\n", seg,
				Vertices[s1->v1].x>>16, Vertices[s1->v1].y>>16,
				Vertices[s1->v2].x>>16, Vertices[s1->v2].y>>16));

		DWORD bestseg = DWORD_MAX;
		DWORD tryseg = Vertices[s1->v2].segs;
		angle_t bestang = ANGLE_MAX;
		angle_t ang1 = PointToAngle (Vertices[s1->v2].x - Vertices[s1->v1].x,
			Vertices[s1->v2].y - Vertices[s1->v1].y);

		while (tryseg != DWORD_MAX)
		{
			FPrivSeg *s2 = &Segs[tryseg];

			if (s2->frontsector == sec)
			{
				angle_t ang2 = PointToAngle (Vertices[s2->v1].x - Vertices[s2->v2].x,
					Vertices[s2->v1].y - Vertices[s2->v2].y);
				angle_t angdiff = ang2 - ang1;

				if (angdiff < bestang && angdiff > 0)
				{
					bestang = angdiff;
					bestseg = tryseg;
				}
			}
			tryseg = s2->nextforvert;
		}

		seg = bestseg;
	} while (seg != DWORD_MAX && Segs[seg].loopnum == 0);

	return loopnum + 1;
}

// Find the bounding box for a specific polyobject.

bool FNodeBuilder::GetPolyExtents (int polynum, fixed_t bbox[4])
{
	size_t i;

	bbox[BOXLEFT] = bbox[BOXBOTTOM] = FIXED_MAX;
	bbox[BOXRIGHT] = bbox[BOXTOP] = FIXED_MIN;

	// Try to find a polyobj marked with a start line
	for (i = 0; i < Segs.Size(); ++i)
	{
		if (Level.Lines[Segs[i].linedef].special == PO_LINE_START &&
			Level.Lines[Segs[i].linedef].args[0] == polynum)
		{
			break;
		}
	}

	if (i < Segs.Size())
	{
		vertex_t start;
		size_t vert;

		vert = Segs[i].v1;

		start.x = Vertices[vert].x;
		start.y = Vertices[vert].y;

		do
		{
			AddSegToBBox (bbox, &Segs[i]);
			vert = Segs[i].v2;
			i = Vertices[vert].segs;
		} while (i != DWORD_MAX && (Vertices[vert].x != start.x || Vertices[vert].y != start.y));

		return true;
	}

	// Try to find a polyobj marked with explicit lines
	bool found = false;

	for (i = 0; i < Segs.Size(); ++i)
	{
		if (Level.Lines[Segs[i].linedef].special == PO_LINE_EXPLICIT &&
			Level.Lines[Segs[i].linedef].args[0] == polynum)
		{
			AddSegToBBox (bbox, &Segs[i]);
			found = true;
		}
	}
	return found;
}

void FNodeBuilder::AddSegToBBox (fixed_t bbox[4], const FPrivSeg *seg)
{
	FPrivVert *v1 = &Vertices[seg->v1];
	FPrivVert *v2 = &Vertices[seg->v2];

	if (v1->x < bbox[BOXLEFT])		bbox[BOXLEFT] = v1->x;
	if (v1->x > bbox[BOXRIGHT])		bbox[BOXRIGHT] = v1->x;
	if (v1->y < bbox[BOXBOTTOM])	bbox[BOXBOTTOM] = v1->y;
	if (v1->y > bbox[BOXTOP])		bbox[BOXTOP] = v1->y;

	if (v2->x < bbox[BOXLEFT])		bbox[BOXLEFT] = v2->x;
	if (v2->x > bbox[BOXRIGHT])		bbox[BOXRIGHT] = v2->x;
	if (v2->y < bbox[BOXBOTTOM])	bbox[BOXBOTTOM] = v2->y;
	if (v2->y > bbox[BOXTOP])		bbox[BOXTOP] = v2->y;
}
