/*****************************************************************************

		Flick FLI-format Animation Viewer v1.2		  19 Feb 1994
		--------------------------------------


This program plays FLI/FLC-format bitmapped animation files on any ECS
or AGA Amiga running OS2.04 or higher.  FLI/FLC-format files are
produced by Autodesk Animator and Autodesk 3D Studio on a PC, as well
as by other programs.

The files in this archive may be distributed anywhere provided they are
unmodified and are not sold for profit.

Ownership and copyright of all files remains with the author:

	Peter McGavin, 86 Totara Crescent, Lower Hutt, New Zealand.
	e-mail: peterm@maths.grace.cri.nz

*****************************************************************************/

#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>


#include <dos/dos.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <intuition/intuition.h>
#include <libraries/asl.h>
#include <libraries/lowlevel.h>
#include <devices/gameport.h>
#include <devices/timer.h>
#include <devices/keymap.h>
#include <devices/input.h>
#include <devices/inputevent.h>

 


#include <stdlib.h>
#include <sys/stat.h>
#include "modexlib.h"

#define NBITS 4			/* for each r, g, b */
#define NSIDE (1<<NBITS)
#define NPALETTE 256      /* # of palette entries */

typedef enum {R, G, B} rgb_type;

struct box_type {                  /* Defines a parallelopiped */
  UBYTE start[3];
  UBYTE end[3];
};

struct node_type {
  struct box_type box;		/* corners of the rectangular box */
  UWORD n;			/* total number of pixels in box */
  rgb_type long_primary;	/* longest direction (red, green or blue) */
  ULONG badness;		/* spread of pixels within box */
  UWORD primaryhist[3][NSIDE];	/* 3 histograms (one for each of R,G,B) */
  UBYTE palette[3];		/* centre of gravity */
};


static UWORD hist[NSIDE][NSIDE][NSIDE];    /* 3D histogram (cube) */

static void update_entry (struct node_type *node)
/* given a rectangular box of given dimensions (in node), calculate and
   fill in all the other parts of the node. */
{
  UWORD r, g, b, i, mean;
  ULONG c, wsum, bad;
  LONG signed_diff;
  rgb_type primary;

  /* build histogram for each primary */
  memset (node->primaryhist, 0, 3 * NSIDE * sizeof(UWORD));
  node->n = 0;
  for (r = node->box.start[R]; r <= node->box.end[R]; r++)
    for (g = node->box.start[G]; g <= node->box.end[G]; g++)
      for (b = node->box.start[B]; b <= node->box.end[B]; b++) {
        c = hist[r][g][b];
        node->primaryhist[R][r] += c;
        node->primaryhist[G][g] += c;
        node->primaryhist[B][b] += c;
        node->n += c;
      }

  if (node->n == 0)
    Error ("Empty box!  This should never happen.");

  /* shrink the box */
  for (primary = R; primary <= B; primary++) {
    i = node->box.start[primary];
    while (node->primaryhist[primary][i] == 0)
      i++;
    node->box.start[primary] = i;
    i = node->box.end[primary];
    while (node->primaryhist[primary][i] == 0)
      i--;
    node->box.end[primary] = i;
  }

  /* compute the badness & choose the primary with the greatest badness */
  node->badness = 0;
  for (primary = R; primary <= B; primary++) {
    wsum = 0;
    for (i = node->box.start[primary]; i <= node->box.end[primary]; i++)
      wsum += (node->primaryhist[primary][i] * (ULONG)i);
    mean = ((wsum << 1) + node->n) / (node->n << 1); /* round(wsum/node->n) */
    node->palette[primary] = (UBYTE)(mean << 4);
    bad = 0;
    for (i = node->box.start[primary]; i <= node->box.end[primary]; i++) {
      signed_diff = ((WORD)mean) - ((WORD)(i));
      bad += node->primaryhist[primary][i] * (ULONG)(signed_diff * signed_diff);
    }
    if (bad >= node->badness) {
      node->badness = bad;
      node->long_primary = primary;
    }
  }
}


static void cut (struct node_type *node0, struct node_type *node1)
/* cut node0 into 2 pieces in the node0->long_primary direction and store
   the 2 pieces back into node0 and node1 */
{
  ULONG sum, goal;
  UWORD split;

  if (node0->badness == 0)
    Error ("Badness == 0!  This should never happen");

  /* find the median */
  goal = node0->n >> 1;	/* how many we want on each side of the cut */
  sum = 0;
  split = node0->box.start[node0->long_primary];
  while (sum <= goal)
    sum += node0->primaryhist[node0->long_primary][split++];

  /* go back a bit if necessary to get a better balance */
  if ((sum - goal) >
      (goal - (sum - node0->primaryhist[node0->long_primary][split - 1])))
    sum -= node0->primaryhist[node0->long_primary][--split];

  /* copy box */
  node1->box = node0->box;

  /* set new dimensions of boxes */
  node0->box.end[node0->long_primary] = split - 1;
  node1->box.start[node0->long_primary] = split;

  /* update the nodes */
  update_entry (node0);
  update_entry (node1);
}


static struct node_type node[32];          /* node list */

void median_cut (UBYTE *palette, ULONG *table, UBYTE *xlate)
{
  UWORD idx, this_idx, worst_idx, c, r, g, b, best_idx, max_nodes;
  WORD dr, dg, db;
  ULONG max_badness, max_n, dist, best_dist, lr, lg, lb, *l;
  rgb_type primary;
  UBYTE *p;

  max_nodes = 32;    /* 32 colours for EHB */

  /* build histogram from palette (unlike conventional median split
     algorithm which uses pixels from image, but that would be too slow) */
  memset (hist, 0, NSIDE * NSIDE * NSIDE * sizeof(UWORD));
  p = palette;
  for (c = 0; c < NPALETTE; c++) {
    r = *p++ >> 4;
    g = *p++ >> 4;
    b = *p++ >> 4;
    if (r < 8 && g < 8 && b < 8)	/* EHB if possible */
      hist[r << 1][g << 1][b << 1]++;
    else
      hist[r][g][b]++;
  }

  /* set up an initial box containing the entire colour cube */
  for (primary = R; primary <= B; primary++) {
    node[0].box.start[primary] = 0;
    node[0].box.end[primary] = NSIDE - 1;
  }
  update_entry (&node[0]);

  /* locate and subdivide the worst box until there are not enough
     palette entries for more boxes or all the boxes are too small to
     subdivide further */
  for (this_idx = 1; this_idx < max_nodes; this_idx++) {
    max_badness = 0;
    max_n = 0;
    for (idx = 0; idx < this_idx; idx++)
      if (node[idx].badness > max_badness ||
          (node[idx].badness == max_badness && node[idx].n > max_n)) {
        max_badness = node[idx].badness;
        max_n = node[idx].n;
        worst_idx = idx;
      }
    if (max_badness == 0)
      break;
    cut (&node[worst_idx], &node[this_idx]);
  }

  /* fill the output table */
  l = table;
  for (idx = 0; idx < this_idx; idx++) {
    lr = node[idx].palette[R];
    lr += (lr<<8);
    lr += (lr<<16);
    *l++ = lr;
    lg = node[idx].palette[G];
    lg += (lg<<8);
    lg += (lg<<16);
    *l++ = lg;
    lb = node[idx].palette[B];
    lb += (lb<<8);
    lb += (lb<<16);
    *l++ = lb;
  }

  /* calculate pixel translation table */
  p = palette;
  for (c = 0; c < NPALETTE; c++) {
    r = *p++;
    g = *p++;
    b = *p++;
    best_dist = 32760;
    for (idx = 0; idx < this_idx; idx++) {
      dr = ((WORD)r) - (WORD)node[idx].palette[R];
      dg = ((WORD)g) - (WORD)node[idx].palette[G];
      db = ((WORD)b) - (WORD)node[idx].palette[B];
      if ((dist = dr * dr + dg * dg + db * db) < best_dist) {
        best_dist = dist;
        best_idx = idx;
      }
      dr = ((WORD)r) - (WORD)(node[idx].palette[R] >> 1);
      dg = ((WORD)g) - (WORD)(node[idx].palette[G] >> 1);
      db = ((WORD)b) - (WORD)(node[idx].palette[B] >> 1);
      if ((dist = dr * dr + dg * dg + db * db) < best_dist) {
        best_dist = dist;
        best_idx = idx + 32;	/* EHB colour entry */
      }
    }
    xlate[c] = best_idx;
  }
}
