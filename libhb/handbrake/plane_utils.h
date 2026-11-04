/* plane_utils.h

   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_PLANE_UTILS_H
#define HANDBRAKE_PLANE_UTILS_H

/**
 * Operations on single-plane images
 *
 * None of these functions allocate or free plane memory except where needed
 * for temporary, internal processing such as analysis
 *
 * Functions may access memory outside of the main image region, thus the
 * caller's memory allocation must account for the specified border
 */

// Copies an image with its border from one bordered image buffer to another
void hb_plane_copy(const void *src_image, void *dst_image,
                   const int w, const int h,
                   const int border, const int bps);

// Replicates the edge pixels of the main image region of a bordered image
// buffer into the surrounding border region
void hb_plane_border(void *image, const int w, const int h,
                     const int border, const int bps);

// Copies the main image region from a bordered plane buffer to a non-bordered
// plane buffer, effectively cropping the source to discard the border region
void hb_plane_deborder(const void *src_image, const int src_w,
                       const int border,
                       void *dst_image, const int dst_w, const int dst_s,
                       const int dst_h, const int bps);

// Copies source edge detail to the destination
void hb_plane_edgeboost(const void *src_image, void *dst_image,
                        const int w, const int h,
                        const int border, const int bps);

// Blends pixel values from source and destination into the destination
void hb_plane_blend(const void *src_image, void *dst_image,
                    const int w, const int h,
                    const int border, const int bps,
                    const double mix, const int round);

#endif // HANDBRAKE_PLANE_UTILS_H
