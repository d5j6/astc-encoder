// SPDX-License-Identifier: Apache-2.0
// ----------------------------------------------------------------------------
// Copyright 2019-2020 Arm Limited
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at:
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
// ----------------------------------------------------------------------------

/*
 * This module implements vector support for floats, ints, and control masks.
 * It provides access to both explicit vector width types, and flexible N-wide
 * types where N can be determined at compile time.
 *
 * The design of this module encourages use of vector length agnostic code, via
 * the vint, vfloat, and vmask types. These will take on the widest vector with
 * that is available at compile time. The current vector width is accessible
 * via the ASTCENC_SIMD_WIDTH constant.
 *
 * Explicit scalar types are acessible via the vint1, vfloat1, vmask1 types.
 * These are provided primarily for prototyping and algorithm debug of VLA
 * implementations.
 *
 * Explicit 4-wide types are accessible via the vint4, vfloat4, and vmask4
 * types. These are provided for use by VLA code, but are also expected to be
 * used as a fixed-width type can be supported via a reference C implementation
 * on platforms without SIMD intrinsics.
 *
 * TODO: This 4-wide support non-SIMD support isn't available yet. Plan to
 * replace the existing float4, etc with this.
 *
 * Explicit 8-wide types are accessible via the vint8, vfloat8, and vmask8
 * types. These are provide for use by VLA code, and are not expected to be
 * used as a fixed-width type in normal code. No reference C implementation is
 * provided on platforms without SIMD intrinsics.
 *
 * With the current implementation ISA support is provided for:
 *
 *     * 1-wide for scalar reference.
 *     * 4-wide for SSE2.
 *     * 4-wide for SSE4.2.
 *     * 8-wide for AVX2.
 *
 */

#ifndef ASTC_VECMATHLIB_H_INCLUDED
#define ASTC_VECMATHLIB_H_INCLUDED

#if ASTCENC_SSE != 0 || ASTCENC_AVX != 0
	#include <immintrin.h>
#endif

#if defined(_MSC_VER)
	#define ASTCENC_SIMD_INLINE __forceinline
#elif defined(__GNUC__) && !defined(__clang__)
	#define ASTCENC_SIMD_INLINE __attribute__((unused, always_inline)) inline
#else
	#define ASTCENC_SIMD_INLINE __attribute__((unused, always_inline, nodebug)) inline
#endif

#if ASTCENC_AVX >= 2
	/* If we have AVX2 expose 8-wide VLA, and 4-wide fixed width. */
	#include "astcenc_vecmathlib_avx2_8.h"
	#include "astcenc_vecmathlib_sse_4.h"

	#define ASTCENC_SIMD_WIDTH 8

	using vfloat = vfloat8;
	using vint = vint8;
	using vmask = vmask8;

	const auto loada = loada_8f;
	const auto load1a = load1a_8f;

#elif ASTCENC_SSE >= 20
	/* If we have SSE expose 4-wide VLA, and 4-wide fixed width. */
	#include "astcenc_vecmathlib_sse_4.h"

	#define ASTCENC_SIMD_WIDTH 4

	using vfloat = vfloat4;
	using vint = vint4;
	using vmask = vmask4;

	const auto loada = loada_4f;
	const auto load1a = load1a_4f;
#else
	/* If we have nothing expose 1-wide VLA. */
	#include "astcenc_vecmathlib_none_1.h"

	#define ASTCENC_SIMD_WIDTH 1

	using vfloat = vfloat1;
	using vint = vint1;
	using vmask = vmask1;

	const auto loada = loada_1f;
	const auto load1a = load1a_1f;
#endif

// Return x, with each lane having its sign flipped where the corresponding y lane is negative, i.e. msb(y) ? -x : x
ASTCENC_SIMD_INLINE vfloat changesign(vfloat x, vfloat y)
{
	vint ix = floatAsInt(x);
	vint iy = floatAsInt(y);
	vint signMask((int)0x80000000);
	vint r = ix ^ (iy & signMask);
	return intAsFloat(r);
}

// Fast atan implementation, with max error of 0.004883
ASTCENC_SIMD_INLINE vfloat atan(vfloat x)
{
	vmask c = abs(x) > vfloat(1.0f);
	vfloat z = changesign(vfloat(astc::PI_OVER_TWO), x);
	vfloat y = select(x, vfloat(1.0f) / x, c);
	y = y / (y * y * vfloat(0.28f) + vfloat(1.0f));
	return select(y, z - y, c);
}

ASTCENC_SIMD_INLINE vfloat atan2(vfloat y, vfloat x)
{
	vfloat z = atan(abs(y / x));
	vmask xmask = vmask(floatAsInt(x).m);
	return changesign(select(z, vfloat(astc::PI) - z, xmask), y);
}

#endif // #ifndef ASTC_VECMATHLIB_H_INCLUDED
