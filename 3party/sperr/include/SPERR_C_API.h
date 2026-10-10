/*
 * This header provides C API for SPERR.
 * This API is supposed to be used in non-C++ (e.g., C, Fortran, Python) projects.
 */

#ifndef SPERR_C_API_H
#define SPERR_C_API_H

#include <stddef.h> /* for size_t */
#include <stdint.h> /* for fixed-width integers */

#ifdef __cplusplus
namespace C_API {
extern "C" {
#endif

/*
 * The memory management is a little tricy. The following requirement applies to the output
 * buffer `dst` of all the C API functions.
 *
 * The output is stored in `dst`, which is a pointer pointing to another pointer
 * held by the caller.  The other pointer should be NULL; otherwise this function will fail!
 * Upon success, `dst` will contain a buffer of length `dst_len` in case of compression,
 * and (dimx x dimy x dimz) of floats (or doubles) in case of decompression.
 * The caller of this function is responsible of free'ing `dst` using free().
 *
 */

/*
 * Compress a a 2D slice targetting different quality controls (modes):
 *    mode == 1 --> fixed bit-per-pixel (BPP)
 *    mode == 2 --> fixed peak signal-to-noise ratio (PSNR)
 *    mode == 3 --> fixed point-wise error (PWE)
 *
 *    The output bitstream can *optionally* include a header field which contains information:
 *    1) the input slice dimension,
 *    2) if the input data is in single or double precision, and
 *    3) a flag indicating that it is a 2D slice (not 3D or 1D).
 *    This header field is 10 bytes in size, and is perfectly fine to be not included if the
 *    information above is known, for example, in the case where a large number of same-sized 2D
 *    slices are to be processed.
 *
 * Return value meanings:
 *  0: success
 *  1: `dst` is not pointing to a NULL pointer!
 *  2: one or more of the parameters are not supported.
 * -1: other error
 */
int sperr_comp_2d(
    const void* src,    /* Input: buffer that contains a 2D slice */
    int is_float,       /* Input: input buffer type: 1 == float, 0 == double */
    size_t dimx,        /* Input: X (fastest-varying) dimension */
    size_t dimy,        /* Input: Y (slowest-varying) dimension */
    int mode,           /* Input: compression mode to use */
    double quality,     /* Input: target quality */
    int out_inc_header, /* Input: include a header in the output bitstream? 1 == yes, 0 == no */
    void** dst,         /* Output: buffer for the output bitstream, allocated by this function */
    size_t* dst_len);   /* Output: length of `dst` in byte */

/*
 * Decompress a 2D SPERR-compressed buffer that is produced by sperr_comp_2d().
 *  Note that this bitstream shoult NOT contain a header. I.e., a bitstream produced by
 *  sperr_comp_2d() with `out_inc_header = 0`, or with `out_inc_header = 1` and has its
 *  first 10 bytes stipped.
 *
 * Return value meanings:
 *  0: success
 *  1: `dst` not pointing to a NULL pointer!
 * -1: other error
 */
int sperr_decomp_2d(
    const void* src,  /* Input: buffer that contains a compressed bitstream AND no header! */
    size_t src_len,   /* Input: length of the input bitstream in byte */
    int output_float, /* Input: output data type: 1 == float, 0 == double */
    size_t dimx,      /* Input: X (fast-varying) dimension */
    size_t dimy,      /* Input: Y (slowest-varying) dimension */
    void** dst);      /* Output: buffer for the output 2D slice, allocated by this function */

/*
 * Parse the header of a bitstream and extract various information. The bitstream can be produced
 * by sperr_comp_3d(), or by sperr_comp_2d() with the `out_inc_header` option on.
 */
void sperr_parse_header(
    const void* src, /* Input: a SPERR bitstream */
    size_t* dimx,    /* Output: X dimension length */
    size_t* dimy,    /* Output: Y dimension length */
    size_t* dimz,    /* Output: Z dimension length (2D slices will have dimz == 1) */
    int* is_float);  /* Output: if the original input is in float (1) or double (0) precision */

/*
 * Compress a a 3D volume targetting different quality controls (modes):
 *   mode == 1 --> fixed bit-per-pixel (BPP)
 *   mode == 2 --> fixed peak signal-to-noise ratio (PSNR)
 *   mode == 3 --> fixed point-wise error (PWE)
 *
 * Return value meanings:
 *  0: success
 *  1: `dst` is not pointing to a NULL pointer!
 *  2: one or more parameters isn't valid.
 * -1: other error
 */
int sperr_comp_3d(
    const void* src,  /* Input: buffer that contains a 3D volume */
    int is_float,     /* Input: input buffer type: 1 == float, 0 = double */
    size_t dimx,      /* Input: X (fastest-varying) dimension */
    size_t dimy,      /* Input: Y dimension */
    size_t dimz,      /* Input: Z (slowest-varying) dimension */
    size_t chunk_x,   /* Input: preferred chunk dimension in X */
    size_t chunk_y,   /* Input: preferred chunk dimension in Y */
    size_t chunk_z,   /* Input: preferred chunk dimension in Z */
    int mode,         /* Input: compression mode to use */
    double quality,   /* Input: target quality */
    size_t nthreads,  /* Input: number of OpenMP threads to use. 0 means using all threads. */
    void** dst,       /* Output: buffer for the output bitstream, allocated by this function */
    size_t* dst_len); /* Output: length of `dst` in byte */

/*
 * Decompress a 3D SPERR-compressed buffer that is produced by sperr_comp_3d().
 *
 * Return value meanings:
 *  0: success
 *  1: `dst` is not pointing to a NULL pointer!
 * -1: other error
 */
int sperr_decomp_3d(
    const void* src,  /* Input: buffer that contains a compressed bitstream */
    size_t src_len,   /* Input: length of the input bitstream in byte */
    int output_float, /* Input: output data type: 1 == float, 0 == double */
    size_t nthreads,  /* Input: number of OMP threads to use. 0 means using all threads. */
    size_t* dimx,     /* Output: X (fast-varying) dimension */
    size_t* dimy,     /* Output: Y dimension */
    size_t* dimz,     /* Output: Z (slowest-varying) dimension */
    void** dst);      /* Output: buffer for the output 3D slice, allocated by this function */

/*
 * Truncate a 3D SPERR-compressed bitstream to a percentage of its original length.
 *    Note on `src_len`: it does not to be the full length of the original bitstream, rather,
 *    it can be just long enough for the requested truncation:
 *  - one chunk: (full_bitstream_length * percent + 64) bytes.
 *  - multiple chunks: probably easier to just use the full bitstream length.
 *
 * Return value meanings:
 *  0: success
 *  1: `dst` is not pointing to a NULL pointer!
 * -1: other error
 */
int sperr_trunc_3d(
    const void* src,  /* Input: buffer that contains a compressed bitstream */
    size_t src_len,   /* Input: length of the input bitstream in byte */
    unsigned pct,     /* Input: percentage of the bitstream to keep (1 <= pct <= 100) */
    void** dst,       /* Output: buffer for the truncated bitstream, allocated by this function */
    size_t* dst_len); /* Output: length of `dst` in byte */

#ifdef __cplusplus
} /* end of extern "C" */
} /* end of namespace C_API */
#endif

#endif
