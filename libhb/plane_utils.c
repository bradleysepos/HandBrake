/* plane_utils.c

   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/**
 * Operations on single-plane images
 *
 * None of these functions allocate or free plane memory except where needed
 * for temporary, internal processing such as analysis
 *
 * Functions may access memory outside of the main image region, thus the
 * caller's memory allocation must account for the specified border
 */

#include "handbrake/handbrake.h"
#include "handbrake/plane_utils.h"

// Copies an image including its border
#define DEF_PLANE_COPY_FUNC(name, nbits)                                      \
static void name##_##nbits(const void *src_image, void *dst_image,            \
                           const int w, const int h,                          \
                           const int border)                                  \
{                                                                             \
    typedef uint##nbits##_t pixel;                                            \
    const int bw = w + 2 * border;                                            \
    const int bw_size = bw * sizeof(pixel);                                   \
    const pixel *src = (const pixel *)(src_image) - border;                   \
    pixel *dst = (pixel *)(dst_image) - border;                               \
                                                                              \
    for (int y = -border; y < h + border; y++)                                \
    {                                                                         \
        memcpy(dst + y * bw, src + y * bw, bw_size);                          \
    }                                                                         \
}

DEF_PLANE_COPY_FUNC(plane_copy, 8)
DEF_PLANE_COPY_FUNC(plane_copy, 16)

/**
 * Copies an image with its border from one bordered image buffer to another
 *
 * @param src_image  Pointer to source image origin without its border
 * @param dst_image  Pointer to destination image origin without its border
 * @param w          Width of image without its border
 * @param h          Height of image without its border
 * @param border     Size of border extending past one edge of the image
 * @param bps        Bits per sample (1 for 8-bit, 2 for 16-bit)
 */
void hb_plane_copy(const void *src_image, void *dst_image,
                   const int w, const int h,
                   const int border, const int bps)
{
    if (bps > 1)
    {
        plane_copy_16(src_image, dst_image, w, h, border);
    }
    else
    {
        plane_copy_8(src_image, dst_image, w, h, border);
    }
}

// Edge-replicates the main image region into the surrounding border region
#define DEF_PLANE_BORDER_FUNC(name, nbits)                                    \
static void name##_##nbits(void *in_image, const int w, const int h,          \
                           const int border)                                  \
{                                                                             \
    typedef uint##nbits##_t pixel;                                            \
    const int bw = w + 2 * border;                                            \
    const int bw_size = bw * sizeof(pixel);                                   \
    pixel *image = (pixel *)(in_image);                                       \
    pixel *image_off = image - border;                                        \
    pixel *row;                                                               \
                                                                              \
    for (int y = 0; y < h; y++)                                               \
    {                                                                         \
        row = image + y * bw;                                                 \
        for (int x = 0; x < border; x++)                                      \
        {                                                                     \
            *(row - x - 1) = *(row + x);                                      \
            *(row + x + w) = *(row - x + (w-1));                              \
        }                                                                     \
    }                                                                         \
    for (int y = 0; y < border; y++)                                          \
    {                                                                         \
        memcpy(image_off - (y+1) * bw, image_off +       y * bw, bw_size);    \
        memcpy(image_off + (y+h) * bw, image_off + (h-y-1) * bw, bw_size);    \
    }                                                                         \
}

DEF_PLANE_BORDER_FUNC(plane_border, 8)
DEF_PLANE_BORDER_FUNC(plane_border, 16)

/**
 * Replicates the edge pixels of the main image region of a bordered image
 * buffer into the surrounding border region
 *
 * @param image   Pointer to bordered image origin without its border
 * @param w       Width of image without its border
 * @param h       Height of image without its border
 * @param border  Size of border extending past one edge of the image
 * @param bps     Bits per sample (1 for 8-bit, 2 for 16-bit)
 */
void hb_plane_border(void *image, const int w, const int h,
                     const int border, const int bps)
{
    if (bps > 1)
    {
        plane_border_16(image, w, h, border);
    }
    else
    {
        plane_border_8(image, w, h, border);
    }
}

// Copies a main image region from a bordered image buffer, removing its border
#define DEF_PLANE_DEBORDER_FUNC(name, nbits)                                  \
static void name##_##nbits(const void *src_image, const int src_w,            \
                           const int border,                                  \
                           void *dst_image, const int dst_w,                  \
                           const int dst_s, const int dst_h)                  \
{                                                                             \
    typedef uint##nbits##_t pixel;                                            \
    const int bw = src_w + 2 * border;                                        \
    const int w = src_w < dst_w ? src_w : dst_w;                              \
    const int w_size = w * sizeof(pixel);                                     \
    const pixel *src = (const pixel *)(src_image);                            \
    pixel *dst = (pixel *)(dst_image);                                        \
                                                                              \
    for (int y = 0; y < dst_h; y++)                                           \
    {                                                                         \
        memcpy(dst + y * dst_s, src + y * bw, w_size);                        \
    }                                                                         \
}

DEF_PLANE_DEBORDER_FUNC(plane_deborder, 8)
DEF_PLANE_DEBORDER_FUNC(plane_deborder, 16)

/**
 * Copies the main image region from a bordered image buffer to a non-bordered
 * image buffer, effectively cropping the source to discard the border region
 *
 * @param src_image  Pointer to bordered source image origin without its border
 * @param src_w      Width of source image without its border
 * @param border     Size of border extending past one edge of the image
 * @param dst_image  Pointer to destination image
 * @param dst_w      Width of destination image
 * @param dst_s      Stride of destination image
 * @param dst_h      Height of destination image
 * @param bps        Bits per sample (1 for 8-bit, 2 for 16-bit)
 */
void hb_plane_deborder(const void *src_image, const int src_w, const int border,
                       void *dst_image, const int dst_w, const int dst_s,
                       const int dst_h, const int bps)
{
    if (bps > 1)
    {
        plane_deborder_16(src_image, src_w, border, dst_image, dst_w, dst_s, dst_h);
    }
    else
    {
        plane_deborder_8(src_image, src_w, border, dst_image, dst_w, dst_s, dst_h);
    }
}

// Edge boost function to copy source edge detail to the destination
#define DEF_PLANE_EDGEBOOST_FUNC(name, nbits, nbits2)                         \
static void name##_##nbits(const void *src_image, void *dst_image,            \
                           const int w, const int h,                          \
                           const int border)                                  \
{                                                                             \
    typedef uint##nbits##_t pixel;                                            \
    typedef uint##nbits2##_t pixel_2;                                         \
    const pixel *src = (const pixel *)(src_image);                            \
    pixel *dst = (pixel *)(dst_image);                                        \
    const int bw = w + 2 * border;                                            \
    const int bh = h + 2 * border;                                            \
                                                                              \
    /* Custom kernel */                                                       \
    const int kernel_size = 3;                                                \
    const int kernel[3][3] = {{-31, 0, 31},                                   \
                              {-44, 0, 44},                                   \
                              {-31, 0, 31}};                                  \
    const double kernel_coef = 1.0 / 126.42;                                  \
                                                                              \
    /* Detect edges */                                                        \
    const int offset_min = -((kernel_size - 1) /2);                           \
    const int offset_max =   (kernel_size + 1) /2;                            \
    pixel_2 p1;                                                               \
    pixel_2 p2;                                                               \
    pixel *mask_mem = calloc(bw * bh, sizeof(pixel));                         \
    if (mask_mem == NULL)                                                     \
    {                                                                         \
        return;                                                               \
    }                                                                         \
    pixel *mask = mask_mem + border + bw * border;                            \
    for (int y = 0; y < h; y++)                                               \
    {                                                                         \
        for (int x = 0; x < w; x++)                                           \
        {                                                                     \
            p1 = 0;                                                           \
            p2 = 0;                                                           \
            for (int k = offset_min; k < offset_max; k++)                     \
            {                                                                 \
                for (int j = offset_min; j < offset_max; j++)                 \
                {                                                             \
                    p1 += kernel[j+1][k+1] * *(src + bw*(y+j) + (x+k));       \
                    p2 += kernel[k+1][j+1] * *(src + bw*(y+j) + (x+k));       \
                }                                                             \
            }                                                                 \
            p1 = p1 > 0 ? p1 : -p1;                                           \
            p2 = p2 > 0 ? p2 : -p2;                                           \
            p1 = (pixel_2)(((double)(p1) * kernel_coef) + 128);               \
            p2 = (pixel_2)(((double)(p2) * kernel_coef) + 128);               \
            *(mask + bw*y + x) = (pixel)(p1 + p2);                            \
            if (*(mask + bw*y + x) > 160)                                     \
            {                                                                 \
                *(mask + bw*y + x) = 235;                                     \
            }                                                                 \
            else if (*(mask + bw*y + x) > 16)                                 \
            {                                                                 \
                *(mask + bw*y + x) = 128;                                     \
            }                                                                 \
            else                                                              \
            {                                                                 \
                *(mask + bw*y + x) = 16;                                      \
            }                                                                 \
        }                                                                     \
    }                                                                         \
                                                                              \
    /* Post-process and output */                                             \
    int pixels;                                                               \
    for (int y = 0; y < h; y++)                                               \
    {                                                                         \
        const pixel *s = src + y * bw;                                        \
        pixel *d = dst + y * bw;                                              \
        for (int x = 0; x < w; x++)                                           \
        {                                                                     \
            if (*(mask + bw*y + x) > 16)                                      \
            {                                                                 \
                /* Count nearby edge pixels */                                \
                pixels = 0;                                                   \
                for (int k = offset_min; k < offset_max; k++)                 \
                {                                                             \
                    for (int j = offset_min; j < offset_max; j++)             \
                    {                                                         \
                        if (*(mask + bw*(y+j) + (x+k)) > 16)                  \
                        {                                                     \
                            pixels++;                                         \
                        }                                                     \
                    }                                                         \
                }                                                             \
                /* Remove false positive */                                   \
                if (pixels < 3)                                               \
                {                                                             \
                    *(mask + bw*y + x) = 16;                                  \
                }                                                             \
                /* Filter output */                                           \
                if (*(mask + bw*y + x) > 16)                                  \
                {                                                             \
                    if (*(mask + bw*y + x) == 235)                            \
                    {                                                         \
                        d[x] = ((pixel_2)(3) * s[x] + (pixel_2)(1) * d[x])    \
                                 / (pixel_2)(4);                              \
                    }                                                         \
                    else                                                      \
                    {                                                         \
                        d[x] = ((pixel_2)(2) * s[x] + (pixel_2)(3) * d[x])    \
                                 / (pixel_2)(5);                              \
                    }                                                         \
                }                                                             \
            }                                                                 \
        }                                                                     \
    }                                                                         \
                                                                              \
    free(mask_mem);                                                           \
}

DEF_PLANE_EDGEBOOST_FUNC(plane_edgeboost, 8, 16)
DEF_PLANE_EDGEBOOST_FUNC(plane_edgeboost, 16, 32)

/**
 * Copies source edge detail to the destination
 *
 * @param src_image  Pointer to source image origin without its border
 * @param dst_image  Pointer to destination image origin without its border
 * @param w          Width of image without its border
 * @param h          Height of image without its border
 * @param border     Size of border extending past one edge of the image
 * @param bps        Bits per sample (1 for 8-bit, 2 for 16-bit)
 */
void hb_plane_edgeboost(const void *src_image, void *dst_image,
                        const int w, const int h,
                        const int border, const int bps)
{
    if (bps > 1)
    {
        plane_edgeboost_16(src_image, dst_image, w, h, border);
    }
    else
    {
        plane_edgeboost_8(src_image, dst_image, w, h, border);
    }
}

// Floating point math pixel blend, optionally rounded half-up
#define DEF_PLANE_BLEND_FUNC(name, nbits)                                     \
static void name##_##nbits(const void *src_image, void *dst_image,            \
                           const int w, const int h,                          \
                           const int border,                                  \
                           const double mix, const int round)                 \
{                                                                             \
    /* Return early if mix fully preserves destination image (no blend) */    \
    if (mix >= 1.0)                                                           \
    {                                                                         \
        return;                                                               \
    }                                                                         \
                                                                              \
    typedef uint##nbits##_t pixel;                                            \
    const int bw = w + 2 * border;                                            \
    const double mix2 = 1.0 - mix;                                            \
    const double bias = round ? 0.5 : 0.0;                                    \
    const pixel *src = (const pixel *)(src_image);                            \
    pixel *dst = (pixel *)(dst_image);                                        \
    const pixel *s;                                                           \
    pixel *d;                                                                 \
                                                                              \
    for (int y = 0; y < h; y++)                                               \
    {                                                                         \
        s = src + y * bw;                                                     \
        d = dst + y * bw;                                                     \
        for (int x = 0; x < w; x++)                                           \
        {                                                                     \
            d[x] = (pixel)(mix * d[x] + mix2 * s[x] + bias);                  \
        }                                                                     \
    }                                                                         \
}

DEF_PLANE_BLEND_FUNC(plane_blend, 8)
DEF_PLANE_BLEND_FUNC(plane_blend, 16)

// Integer math pixel blend, wet and dry values must be small to avoid overflow
#define DEF_PLANE_BLEND_I_FUNC(name, nbits, nbits2)                           \
static void name##_##nbits(const void *src_image, void *dst_image,            \
                           const int w, const int h,                          \
                           const int border,                                  \
                           const int wet, const int dry)                      \
{                                                                             \
    /* Return early if mix fully preserves destination image (no blend) */    \
    if (dry <= 0)                                                             \
    {                                                                         \
        return;                                                               \
    }                                                                         \
                                                                              \
    typedef uint##nbits##_t pixel;                                            \
    typedef uint##nbits2##_t pixel_2;                                         \
    const int bw = w + 2 * border;                                            \
    const pixel_2 wet2 = wet;                                                 \
    const pixel_2 dry2 = dry;                                                 \
    const pixel_2 den2 = wet2 + dry2;                                         \
    const pixel *src = (const pixel *)(src_image);                            \
    pixel *dst = (pixel *)(dst_image);                                        \
    const pixel *s;                                                           \
    pixel *d;                                                                 \
                                                                              \
    for (int y = 0; y < h; y++)                                               \
    {                                                                         \
        s = src + y * bw;                                                     \
        d = dst + y * bw;                                                     \
        for (int x = 0; x < w; x++)                                           \
        {                                                                     \
            d[x] = (pixel)((wet2 * d[x] + dry2 * s[x]) / den2);               \
        }                                                                     \
    }                                                                         \
}

DEF_PLANE_BLEND_I_FUNC(plane_blend_i, 8, 16)
DEF_PLANE_BLEND_I_FUNC(plane_blend_i, 16, 32)

/**
 * Blends pixel values from source and destination into the destination
 *
 * @param src_image  Pointer to source image origin without its border
 * @param dst_image  Pointer to destination image origin without its border
 * @param w          Width of image without its border
 * @param h          Height of image without its border
 * @param border     Size of border extending past one edge of the image
 * @param bps        Bits per sample (1 for 8-bit, 2 for 16-bit)
 * @param mix        Blend ratio (0.0 = dst replaced by src, 1.0 = dst fully preserved)
 * @param round      Whether to round (1) or truncate (0) the resulting pixel values
 */
void hb_plane_blend(const void *src_image, void *dst_image,
                    const int w, const int h,
                    const int border, const int bps,
                    const double mix, const int round)
{
    // No-op where result would not change destination image
    if (mix >= 1.0)
    {
        return;
    }

    // Direct source copy where mix is 0
    if (mix <= 0.0)
    {
        if (bps > 1)
        {
            plane_copy_16(src_image, dst_image, w, h, border);
        }
        else
        {
            plane_copy_8(src_image, dst_image, w, h, border);
        }
        return;
    }

    // Integer math fast path for mix values that are a multiple of 1/100
    // Only applicable where rounding is not requested (truncate mode)
    if (!round)
    {
        int k = (int)(mix * 100.0 + 0.5);
        if (k >= 1 && k <= 99 && mix == (double)(k) / 100.0)
        {
            if (bps > 1)
            {
                plane_blend_i_16(src_image, dst_image, w, h, border, k, 100 - k);
            }
            else
            {
                plane_blend_i_8(src_image, dst_image, w, h, border, k, 100 - k);
            }
            return;
        }
    }

    // Float math path for all other mix values and rounding
    if (bps > 1)
    {
        plane_blend_16(src_image, dst_image, w, h, border, mix, round);
    }
    else
    {
        plane_blend_8(src_image, dst_image, w, h, border, mix, round);
    }
}
