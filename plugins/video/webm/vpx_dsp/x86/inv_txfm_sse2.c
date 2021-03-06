/*
 *  Copyright (c) 2015 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "./vpx_dsp_rtcd.h"
#include "vpx_dsp/x86/inv_txfm_sse2.h"
#include "vpx_dsp/x86/transpose_sse2.h"
#include "vpx_dsp/x86/txfm_common_sse2.h"

void vpx_idct4x4_16_add_sse2(const tran_low_t *input, uint8_t *dest,
                             int stride) {
  const __m128i eight = _mm_set1_epi16(8);
  __m128i in[2];

  // Rows
  in[0] = load_input_data(input);
  in[1] = load_input_data(input + 8);
  idct4_sse2(in);

  // Columns
  idct4_sse2(in);

  // Final round and shift
  in[0] = _mm_add_epi16(in[0], eight);
  in[1] = _mm_add_epi16(in[1], eight);
  in[0] = _mm_srai_epi16(in[0], 4);
  in[1] = _mm_srai_epi16(in[1], 4);

  recon_and_store4x4_sse2(in, dest, stride);
}

void vpx_idct4x4_1_add_sse2(const tran_low_t *input, uint8_t *dest,
                            int stride) {
  const __m128i zero = _mm_setzero_si128();
  int a;
  __m128i dc_value, d[2];

  a = (int)dct_const_round_shift(input[0] * cospi_16_64);
  a = (int)dct_const_round_shift(a * cospi_16_64);
  a = ROUND_POWER_OF_TWO(a, 4);

  dc_value = _mm_set1_epi16(a);

  // Reconstruction and Store
  d[0] = _mm_cvtsi32_si128(*(const int *)(dest));
  d[1] = _mm_cvtsi32_si128(*(const int *)(dest + stride * 3));
  d[0] = _mm_unpacklo_epi32(d[0],
                            _mm_cvtsi32_si128(*(const int *)(dest + stride)));
  d[1] = _mm_unpacklo_epi32(
      _mm_cvtsi32_si128(*(const int *)(dest + stride * 2)), d[1]);
  d[0] = _mm_unpacklo_epi8(d[0], zero);
  d[1] = _mm_unpacklo_epi8(d[1], zero);
  d[0] = _mm_add_epi16(d[0], dc_value);
  d[1] = _mm_add_epi16(d[1], dc_value);
  d[0] = _mm_packus_epi16(d[0], d[1]);

  *(int *)dest = _mm_cvtsi128_si32(d[0]);
  d[0] = _mm_srli_si128(d[0], 4);
  *(int *)(dest + stride) = _mm_cvtsi128_si32(d[0]);
  d[0] = _mm_srli_si128(d[0], 4);
  *(int *)(dest + stride * 2) = _mm_cvtsi128_si32(d[0]);
  d[0] = _mm_srli_si128(d[0], 4);
  *(int *)(dest + stride * 3) = _mm_cvtsi128_si32(d[0]);
}

void idct4_sse2(__m128i *in) {
  const __m128i k__cospi_p16_p16 = pair_set_epi16(cospi_16_64, cospi_16_64);
  const __m128i k__cospi_p16_m16 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i k__cospi_p24_m08 = pair_set_epi16(cospi_24_64, -cospi_8_64);
  const __m128i k__cospi_p08_p24 = pair_set_epi16(cospi_8_64, cospi_24_64);
  __m128i u[2];

  transpose_16bit_4x4(in);
  // stage 1
  u[0] = _mm_unpacklo_epi16(in[0], in[1]);
  u[1] = _mm_unpackhi_epi16(in[0], in[1]);
  u[0] = idct_calc_wraplow_sse2(k__cospi_p16_p16, k__cospi_p16_m16, u[0]);
  u[1] = idct_calc_wraplow_sse2(k__cospi_p08_p24, k__cospi_p24_m08, u[1]);

  // stage 2
  in[0] = _mm_add_epi16(u[0], u[1]);
  in[1] = _mm_sub_epi16(u[0], u[1]);
  in[1] = _mm_shuffle_epi32(in[1], 0x4E);
}

void iadst4_sse2(__m128i *in) {
  const __m128i k__sinpi_p01_p04 = pair_set_epi16(sinpi_1_9, sinpi_4_9);
  const __m128i k__sinpi_p03_p02 = pair_set_epi16(sinpi_3_9, sinpi_2_9);
  const __m128i k__sinpi_p02_m01 = pair_set_epi16(sinpi_2_9, -sinpi_1_9);
  const __m128i k__sinpi_p03_m04 = pair_set_epi16(sinpi_3_9, -sinpi_4_9);
  const __m128i k__sinpi_p03_p03 = _mm_set1_epi16((int16_t)sinpi_3_9);
  const __m128i kZero = _mm_set1_epi16(0);
  const __m128i k__DCT_CONST_ROUNDING = _mm_set1_epi32(DCT_CONST_ROUNDING);
  __m128i u[8], v[8], in7;

  transpose_16bit_4x4(in);
  in7 = _mm_srli_si128(in[1], 8);
  in7 = _mm_add_epi16(in7, in[0]);
  in7 = _mm_sub_epi16(in7, in[1]);

  u[0] = _mm_unpacklo_epi16(in[0], in[1]);
  u[1] = _mm_unpackhi_epi16(in[0], in[1]);
  u[2] = _mm_unpacklo_epi16(in7, kZero);
  u[3] = _mm_unpackhi_epi16(in[0], kZero);

  v[0] = _mm_madd_epi16(u[0], k__sinpi_p01_p04);  // s0 + s3
  v[1] = _mm_madd_epi16(u[1], k__sinpi_p03_p02);  // s2 + s5
  v[2] = _mm_madd_epi16(u[2], k__sinpi_p03_p03);  // x2
  v[3] = _mm_madd_epi16(u[0], k__sinpi_p02_m01);  // s1 - s4
  v[4] = _mm_madd_epi16(u[1], k__sinpi_p03_m04);  // s2 - s6
  v[5] = _mm_madd_epi16(u[3], k__sinpi_p03_p03);  // s2

  u[0] = _mm_add_epi32(v[0], v[1]);
  u[1] = _mm_add_epi32(v[3], v[4]);
  u[2] = v[2];
  u[3] = _mm_add_epi32(u[0], u[1]);
  u[4] = _mm_slli_epi32(v[5], 2);
  u[5] = _mm_add_epi32(u[3], v[5]);
  u[6] = _mm_sub_epi32(u[5], u[4]);

  v[0] = _mm_add_epi32(u[0], k__DCT_CONST_ROUNDING);
  v[1] = _mm_add_epi32(u[1], k__DCT_CONST_ROUNDING);
  v[2] = _mm_add_epi32(u[2], k__DCT_CONST_ROUNDING);
  v[3] = _mm_add_epi32(u[6], k__DCT_CONST_ROUNDING);

  u[0] = _mm_srai_epi32(v[0], DCT_CONST_BITS);
  u[1] = _mm_srai_epi32(v[1], DCT_CONST_BITS);
  u[2] = _mm_srai_epi32(v[2], DCT_CONST_BITS);
  u[3] = _mm_srai_epi32(v[3], DCT_CONST_BITS);

  in[0] = _mm_packs_epi32(u[0], u[1]);
  in[1] = _mm_packs_epi32(u[2], u[3]);
}

#define MULTIPLICATION_AND_ADD_2(lo_0, hi_0, cst0, cst1, res0, res1) \
  {                                                                  \
    res0 = idct_calc_wraplow_sse2(lo_0, hi_0, cst0);                 \
    res1 = idct_calc_wraplow_sse2(lo_0, hi_0, cst1);                 \
  }

#define IDCT8(in0, in1, in2, in3, in4, in5, in6, in7, out0, out1, out2, out3, \
              out4, out5, out6, out7)                                         \
  {                                                                           \
    /* Stage1 */                                                              \
    {                                                                         \
      const __m128i lo_17 = _mm_unpacklo_epi16(in1, in7);                     \
      const __m128i hi_17 = _mm_unpackhi_epi16(in1, in7);                     \
      const __m128i lo_35 = _mm_unpacklo_epi16(in3, in5);                     \
      const __m128i hi_35 = _mm_unpackhi_epi16(in3, in5);                     \
                                                                              \
      MULTIPLICATION_AND_ADD(lo_17, hi_17, lo_35, hi_35, stg1_0, stg1_1,      \
                             stg1_2, stg1_3, stp1_4, stp1_7, stp1_5, stp1_6)  \
    }                                                                         \
                                                                              \
    /* Stage2 */                                                              \
    {                                                                         \
      const __m128i lo_04 = _mm_unpacklo_epi16(in0, in4);                     \
      const __m128i hi_04 = _mm_unpackhi_epi16(in0, in4);                     \
      const __m128i lo_26 = _mm_unpacklo_epi16(in2, in6);                     \
      const __m128i hi_26 = _mm_unpackhi_epi16(in2, in6);                     \
                                                                              \
      MULTIPLICATION_AND_ADD(lo_04, hi_04, lo_26, hi_26, stg2_0, stg2_1,      \
                             stg2_2, stg2_3, stp2_0, stp2_1, stp2_2, stp2_3)  \
                                                                              \
      stp2_4 = _mm_add_epi16(stp1_4, stp1_5);                                 \
      stp2_5 = _mm_sub_epi16(stp1_4, stp1_5);                                 \
      stp2_6 = _mm_sub_epi16(stp1_7, stp1_6);                                 \
      stp2_7 = _mm_add_epi16(stp1_7, stp1_6);                                 \
    }                                                                         \
                                                                              \
    /* Stage3 */                                                              \
    {                                                                         \
      const __m128i lo_56 = _mm_unpacklo_epi16(stp2_6, stp2_5);               \
      const __m128i hi_56 = _mm_unpackhi_epi16(stp2_6, stp2_5);               \
                                                                              \
      stp1_0 = _mm_add_epi16(stp2_0, stp2_3);                                 \
      stp1_1 = _mm_add_epi16(stp2_1, stp2_2);                                 \
      stp1_2 = _mm_sub_epi16(stp2_1, stp2_2);                                 \
      stp1_3 = _mm_sub_epi16(stp2_0, stp2_3);                                 \
                                                                              \
      stp1_5 = idct_calc_wraplow_sse2(lo_56, hi_56, stg2_1);                  \
      stp1_6 = idct_calc_wraplow_sse2(lo_56, hi_56, stg2_0);                  \
    }                                                                         \
                                                                              \
    /* Stage4  */                                                             \
    out0 = _mm_add_epi16(stp1_0, stp2_7);                                     \
    out1 = _mm_add_epi16(stp1_1, stp1_6);                                     \
    out2 = _mm_add_epi16(stp1_2, stp1_5);                                     \
    out3 = _mm_add_epi16(stp1_3, stp2_4);                                     \
    out4 = _mm_sub_epi16(stp1_3, stp2_4);                                     \
    out5 = _mm_sub_epi16(stp1_2, stp1_5);                                     \
    out6 = _mm_sub_epi16(stp1_1, stp1_6);                                     \
    out7 = _mm_sub_epi16(stp1_0, stp2_7);                                     \
  }

void vpx_idct8x8_64_add_sse2(const tran_low_t *input, uint8_t *dest,
                             int stride) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i final_rounding = _mm_set1_epi16(1 << 4);
  const __m128i stg1_0 = pair_set_epi16(cospi_28_64, -cospi_4_64);
  const __m128i stg1_1 = pair_set_epi16(cospi_4_64, cospi_28_64);
  const __m128i stg1_2 = pair_set_epi16(-cospi_20_64, cospi_12_64);
  const __m128i stg1_3 = pair_set_epi16(cospi_12_64, cospi_20_64);
  const __m128i stg2_0 = pair_set_epi16(cospi_16_64, cospi_16_64);
  const __m128i stg2_1 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i stg2_2 = pair_set_epi16(cospi_24_64, -cospi_8_64);
  const __m128i stg2_3 = pair_set_epi16(cospi_8_64, cospi_24_64);

  __m128i in0, in1, in2, in3, in4, in5, in6, in7;
  __m128i stp1_0, stp1_1, stp1_2, stp1_3, stp1_4, stp1_5, stp1_6, stp1_7;
  __m128i stp2_0, stp2_1, stp2_2, stp2_3, stp2_4, stp2_5, stp2_6, stp2_7;
  int i;

  // Load input data.
  in0 = load_input_data(input);
  in1 = load_input_data(input + 8 * 1);
  in2 = load_input_data(input + 8 * 2);
  in3 = load_input_data(input + 8 * 3);
  in4 = load_input_data(input + 8 * 4);
  in5 = load_input_data(input + 8 * 5);
  in6 = load_input_data(input + 8 * 6);
  in7 = load_input_data(input + 8 * 7);

  // 2-D
  for (i = 0; i < 2; i++) {
    // 8x8 Transpose is copied from vpx_fdct8x8_sse2()
    TRANSPOSE_8X8(in0, in1, in2, in3, in4, in5, in6, in7, in0, in1, in2, in3,
                  in4, in5, in6, in7);

    // 4-stage 1D idct8x8
    IDCT8(in0, in1, in2, in3, in4, in5, in6, in7, in0, in1, in2, in3, in4, in5,
          in6, in7);
  }

  // Final rounding and shift
  in0 = _mm_adds_epi16(in0, final_rounding);
  in1 = _mm_adds_epi16(in1, final_rounding);
  in2 = _mm_adds_epi16(in2, final_rounding);
  in3 = _mm_adds_epi16(in3, final_rounding);
  in4 = _mm_adds_epi16(in4, final_rounding);
  in5 = _mm_adds_epi16(in5, final_rounding);
  in6 = _mm_adds_epi16(in6, final_rounding);
  in7 = _mm_adds_epi16(in7, final_rounding);

  in0 = _mm_srai_epi16(in0, 5);
  in1 = _mm_srai_epi16(in1, 5);
  in2 = _mm_srai_epi16(in2, 5);
  in3 = _mm_srai_epi16(in3, 5);
  in4 = _mm_srai_epi16(in4, 5);
  in5 = _mm_srai_epi16(in5, 5);
  in6 = _mm_srai_epi16(in6, 5);
  in7 = _mm_srai_epi16(in7, 5);

  RECON_AND_STORE(dest + 0 * stride, in0);
  RECON_AND_STORE(dest + 1 * stride, in1);
  RECON_AND_STORE(dest + 2 * stride, in2);
  RECON_AND_STORE(dest + 3 * stride, in3);
  RECON_AND_STORE(dest + 4 * stride, in4);
  RECON_AND_STORE(dest + 5 * stride, in5);
  RECON_AND_STORE(dest + 6 * stride, in6);
  RECON_AND_STORE(dest + 7 * stride, in7);
}

void vpx_idct8x8_1_add_sse2(const tran_low_t *input, uint8_t *dest,
                            int stride) {
  __m128i dc_value;
  const __m128i zero = _mm_setzero_si128();
  int a;

  a = (int)dct_const_round_shift(input[0] * cospi_16_64);
  a = (int)dct_const_round_shift(a * cospi_16_64);
  a = ROUND_POWER_OF_TWO(a, 5);

  dc_value = _mm_set1_epi16(a);

  RECON_AND_STORE(dest + 0 * stride, dc_value);
  RECON_AND_STORE(dest + 1 * stride, dc_value);
  RECON_AND_STORE(dest + 2 * stride, dc_value);
  RECON_AND_STORE(dest + 3 * stride, dc_value);
  RECON_AND_STORE(dest + 4 * stride, dc_value);
  RECON_AND_STORE(dest + 5 * stride, dc_value);
  RECON_AND_STORE(dest + 6 * stride, dc_value);
  RECON_AND_STORE(dest + 7 * stride, dc_value);
}

void idct8_sse2(__m128i *in) {
  const __m128i stg1_0 = pair_set_epi16(cospi_28_64, -cospi_4_64);
  const __m128i stg1_1 = pair_set_epi16(cospi_4_64, cospi_28_64);
  const __m128i stg1_2 = pair_set_epi16(-cospi_20_64, cospi_12_64);
  const __m128i stg1_3 = pair_set_epi16(cospi_12_64, cospi_20_64);
  const __m128i stg2_0 = pair_set_epi16(cospi_16_64, cospi_16_64);
  const __m128i stg2_1 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i stg2_2 = pair_set_epi16(cospi_24_64, -cospi_8_64);
  const __m128i stg2_3 = pair_set_epi16(cospi_8_64, cospi_24_64);

  __m128i in0, in1, in2, in3, in4, in5, in6, in7;
  __m128i stp1_0, stp1_1, stp1_2, stp1_3, stp1_4, stp1_5, stp1_6, stp1_7;
  __m128i stp2_0, stp2_1, stp2_2, stp2_3, stp2_4, stp2_5, stp2_6, stp2_7;

  // 8x8 Transpose is copied from vpx_fdct8x8_sse2()
  TRANSPOSE_8X8(in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7], in0,
                in1, in2, in3, in4, in5, in6, in7);

  // 4-stage 1D idct8x8
  IDCT8(in0, in1, in2, in3, in4, in5, in6, in7, in[0], in[1], in[2], in[3],
        in[4], in[5], in[6], in[7]);
}

void iadst8_sse2(__m128i *in) {
  const __m128i k__cospi_p02_p30 = pair_set_epi16(cospi_2_64, cospi_30_64);
  const __m128i k__cospi_p30_m02 = pair_set_epi16(cospi_30_64, -cospi_2_64);
  const __m128i k__cospi_p10_p22 = pair_set_epi16(cospi_10_64, cospi_22_64);
  const __m128i k__cospi_p22_m10 = pair_set_epi16(cospi_22_64, -cospi_10_64);
  const __m128i k__cospi_p18_p14 = pair_set_epi16(cospi_18_64, cospi_14_64);
  const __m128i k__cospi_p14_m18 = pair_set_epi16(cospi_14_64, -cospi_18_64);
  const __m128i k__cospi_p26_p06 = pair_set_epi16(cospi_26_64, cospi_6_64);
  const __m128i k__cospi_p06_m26 = pair_set_epi16(cospi_6_64, -cospi_26_64);
  const __m128i k__cospi_p08_p24 = pair_set_epi16(cospi_8_64, cospi_24_64);
  const __m128i k__cospi_p24_m08 = pair_set_epi16(cospi_24_64, -cospi_8_64);
  const __m128i k__cospi_m24_p08 = pair_set_epi16(-cospi_24_64, cospi_8_64);
  const __m128i k__cospi_p16_m16 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i k__cospi_p16_p16 = _mm_set1_epi16((int16_t)cospi_16_64);
  const __m128i k__const_0 = _mm_set1_epi16(0);
  const __m128i k__DCT_CONST_ROUNDING = _mm_set1_epi32(DCT_CONST_ROUNDING);

  __m128i u0, u1, u2, u3, u4, u5, u6, u7, u8, u9, u10, u11, u12, u13, u14, u15;
  __m128i v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15;
  __m128i w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13, w14, w15;
  __m128i s0, s1, s2, s3, s4, s5, s6, s7;
  __m128i in0, in1, in2, in3, in4, in5, in6, in7;

  // transpose
  array_transpose_8x8(in, in);

  // properly aligned for butterfly input
  in0 = in[7];
  in1 = in[0];
  in2 = in[5];
  in3 = in[2];
  in4 = in[3];
  in5 = in[4];
  in6 = in[1];
  in7 = in[6];

  // column transformation
  // stage 1
  // interleave and multiply/add into 32-bit integer
  s0 = _mm_unpacklo_epi16(in0, in1);
  s1 = _mm_unpackhi_epi16(in0, in1);
  s2 = _mm_unpacklo_epi16(in2, in3);
  s3 = _mm_unpackhi_epi16(in2, in3);
  s4 = _mm_unpacklo_epi16(in4, in5);
  s5 = _mm_unpackhi_epi16(in4, in5);
  s6 = _mm_unpacklo_epi16(in6, in7);
  s7 = _mm_unpackhi_epi16(in6, in7);

  u0 = _mm_madd_epi16(s0, k__cospi_p02_p30);
  u1 = _mm_madd_epi16(s1, k__cospi_p02_p30);
  u2 = _mm_madd_epi16(s0, k__cospi_p30_m02);
  u3 = _mm_madd_epi16(s1, k__cospi_p30_m02);
  u4 = _mm_madd_epi16(s2, k__cospi_p10_p22);
  u5 = _mm_madd_epi16(s3, k__cospi_p10_p22);
  u6 = _mm_madd_epi16(s2, k__cospi_p22_m10);
  u7 = _mm_madd_epi16(s3, k__cospi_p22_m10);
  u8 = _mm_madd_epi16(s4, k__cospi_p18_p14);
  u9 = _mm_madd_epi16(s5, k__cospi_p18_p14);
  u10 = _mm_madd_epi16(s4, k__cospi_p14_m18);
  u11 = _mm_madd_epi16(s5, k__cospi_p14_m18);
  u12 = _mm_madd_epi16(s6, k__cospi_p26_p06);
  u13 = _mm_madd_epi16(s7, k__cospi_p26_p06);
  u14 = _mm_madd_epi16(s6, k__cospi_p06_m26);
  u15 = _mm_madd_epi16(s7, k__cospi_p06_m26);

  // addition
  w0 = _mm_add_epi32(u0, u8);
  w1 = _mm_add_epi32(u1, u9);
  w2 = _mm_add_epi32(u2, u10);
  w3 = _mm_add_epi32(u3, u11);
  w4 = _mm_add_epi32(u4, u12);
  w5 = _mm_add_epi32(u5, u13);
  w6 = _mm_add_epi32(u6, u14);
  w7 = _mm_add_epi32(u7, u15);
  w8 = _mm_sub_epi32(u0, u8);
  w9 = _mm_sub_epi32(u1, u9);
  w10 = _mm_sub_epi32(u2, u10);
  w11 = _mm_sub_epi32(u3, u11);
  w12 = _mm_sub_epi32(u4, u12);
  w13 = _mm_sub_epi32(u5, u13);
  w14 = _mm_sub_epi32(u6, u14);
  w15 = _mm_sub_epi32(u7, u15);

  // shift and rounding
  v0 = _mm_add_epi32(w0, k__DCT_CONST_ROUNDING);
  v1 = _mm_add_epi32(w1, k__DCT_CONST_ROUNDING);
  v2 = _mm_add_epi32(w2, k__DCT_CONST_ROUNDING);
  v3 = _mm_add_epi32(w3, k__DCT_CONST_ROUNDING);
  v4 = _mm_add_epi32(w4, k__DCT_CONST_ROUNDING);
  v5 = _mm_add_epi32(w5, k__DCT_CONST_ROUNDING);
  v6 = _mm_add_epi32(w6, k__DCT_CONST_ROUNDING);
  v7 = _mm_add_epi32(w7, k__DCT_CONST_ROUNDING);
  v8 = _mm_add_epi32(w8, k__DCT_CONST_ROUNDING);
  v9 = _mm_add_epi32(w9, k__DCT_CONST_ROUNDING);
  v10 = _mm_add_epi32(w10, k__DCT_CONST_ROUNDING);
  v11 = _mm_add_epi32(w11, k__DCT_CONST_ROUNDING);
  v12 = _mm_add_epi32(w12, k__DCT_CONST_ROUNDING);
  v13 = _mm_add_epi32(w13, k__DCT_CONST_ROUNDING);
  v14 = _mm_add_epi32(w14, k__DCT_CONST_ROUNDING);
  v15 = _mm_add_epi32(w15, k__DCT_CONST_ROUNDING);

  u0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
  u1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
  u2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
  u3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
  u4 = _mm_srai_epi32(v4, DCT_CONST_BITS);
  u5 = _mm_srai_epi32(v5, DCT_CONST_BITS);
  u6 = _mm_srai_epi32(v6, DCT_CONST_BITS);
  u7 = _mm_srai_epi32(v7, DCT_CONST_BITS);
  u8 = _mm_srai_epi32(v8, DCT_CONST_BITS);
  u9 = _mm_srai_epi32(v9, DCT_CONST_BITS);
  u10 = _mm_srai_epi32(v10, DCT_CONST_BITS);
  u11 = _mm_srai_epi32(v11, DCT_CONST_BITS);
  u12 = _mm_srai_epi32(v12, DCT_CONST_BITS);
  u13 = _mm_srai_epi32(v13, DCT_CONST_BITS);
  u14 = _mm_srai_epi32(v14, DCT_CONST_BITS);
  u15 = _mm_srai_epi32(v15, DCT_CONST_BITS);

  // back to 16-bit and pack 8 integers into __m128i
  in[0] = _mm_packs_epi32(u0, u1);
  in[1] = _mm_packs_epi32(u2, u3);
  in[2] = _mm_packs_epi32(u4, u5);
  in[3] = _mm_packs_epi32(u6, u7);
  in[4] = _mm_packs_epi32(u8, u9);
  in[5] = _mm_packs_epi32(u10, u11);
  in[6] = _mm_packs_epi32(u12, u13);
  in[7] = _mm_packs_epi32(u14, u15);

  // stage 2
  s0 = _mm_add_epi16(in[0], in[2]);
  s1 = _mm_add_epi16(in[1], in[3]);
  s2 = _mm_sub_epi16(in[0], in[2]);
  s3 = _mm_sub_epi16(in[1], in[3]);
  u0 = _mm_unpacklo_epi16(in[4], in[5]);
  u1 = _mm_unpackhi_epi16(in[4], in[5]);
  u2 = _mm_unpacklo_epi16(in[6], in[7]);
  u3 = _mm_unpackhi_epi16(in[6], in[7]);

  v0 = _mm_madd_epi16(u0, k__cospi_p08_p24);
  v1 = _mm_madd_epi16(u1, k__cospi_p08_p24);
  v2 = _mm_madd_epi16(u0, k__cospi_p24_m08);
  v3 = _mm_madd_epi16(u1, k__cospi_p24_m08);
  v4 = _mm_madd_epi16(u2, k__cospi_m24_p08);
  v5 = _mm_madd_epi16(u3, k__cospi_m24_p08);
  v6 = _mm_madd_epi16(u2, k__cospi_p08_p24);
  v7 = _mm_madd_epi16(u3, k__cospi_p08_p24);

  w0 = _mm_add_epi32(v0, v4);
  w1 = _mm_add_epi32(v1, v5);
  w2 = _mm_add_epi32(v2, v6);
  w3 = _mm_add_epi32(v3, v7);
  w4 = _mm_sub_epi32(v0, v4);
  w5 = _mm_sub_epi32(v1, v5);
  w6 = _mm_sub_epi32(v2, v6);
  w7 = _mm_sub_epi32(v3, v7);

  v0 = _mm_add_epi32(w0, k__DCT_CONST_ROUNDING);
  v1 = _mm_add_epi32(w1, k__DCT_CONST_ROUNDING);
  v2 = _mm_add_epi32(w2, k__DCT_CONST_ROUNDING);
  v3 = _mm_add_epi32(w3, k__DCT_CONST_ROUNDING);
  v4 = _mm_add_epi32(w4, k__DCT_CONST_ROUNDING);
  v5 = _mm_add_epi32(w5, k__DCT_CONST_ROUNDING);
  v6 = _mm_add_epi32(w6, k__DCT_CONST_ROUNDING);
  v7 = _mm_add_epi32(w7, k__DCT_CONST_ROUNDING);

  u0 = _mm_srai_epi32(v0, DCT_CONST_BITS);
  u1 = _mm_srai_epi32(v1, DCT_CONST_BITS);
  u2 = _mm_srai_epi32(v2, DCT_CONST_BITS);
  u3 = _mm_srai_epi32(v3, DCT_CONST_BITS);
  u4 = _mm_srai_epi32(v4, DCT_CONST_BITS);
  u5 = _mm_srai_epi32(v5, DCT_CONST_BITS);
  u6 = _mm_srai_epi32(v6, DCT_CONST_BITS);
  u7 = _mm_srai_epi32(v7, DCT_CONST_BITS);

  // back to 16-bit intergers
  s4 = _mm_packs_epi32(u0, u1);
  s5 = _mm_packs_epi32(u2, u3);
  s6 = _mm_packs_epi32(u4, u5);
  s7 = _mm_packs_epi32(u6, u7);

  // stage 3
  u0 = _mm_unpacklo_epi16(s2, s3);
  u1 = _mm_unpackhi_epi16(s2, s3);
  u2 = _mm_unpacklo_epi16(s6, s7);
  u3 = _mm_unpackhi_epi16(s6, s7);

  s2 = idct_calc_wraplow_sse2(u0, u1, k__cospi_p16_p16);
  s3 = idct_calc_wraplow_sse2(u0, u1, k__cospi_p16_m16);
  s6 = idct_calc_wraplow_sse2(u2, u3, k__cospi_p16_p16);
  s7 = idct_calc_wraplow_sse2(u2, u3, k__cospi_p16_m16);

  in[0] = s0;
  in[1] = _mm_sub_epi16(k__const_0, s4);
  in[2] = s6;
  in[3] = _mm_sub_epi16(k__const_0, s2);
  in[4] = s3;
  in[5] = _mm_sub_epi16(k__const_0, s7);
  in[6] = s5;
  in[7] = _mm_sub_epi16(k__const_0, s1);
}

void vpx_idct8x8_12_add_sse2(const tran_low_t *input, uint8_t *dest,
                             int stride) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i final_rounding = _mm_set1_epi16(1 << 4);
  const __m128i stg1_0 = pair_set_epi16(cospi_28_64, -cospi_4_64);
  const __m128i stg1_1 = pair_set_epi16(cospi_4_64, cospi_28_64);
  const __m128i stg1_2 = pair_set_epi16(-cospi_20_64, cospi_12_64);
  const __m128i stg1_3 = pair_set_epi16(cospi_12_64, cospi_20_64);
  const __m128i stg2_0 = pair_set_epi16(cospi_16_64, cospi_16_64);
  const __m128i stg2_1 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i stg2_2 = pair_set_epi16(cospi_24_64, -cospi_8_64);
  const __m128i stg2_3 = pair_set_epi16(cospi_8_64, cospi_24_64);
  const __m128i stg3_0 = pair_set_epi16(-cospi_16_64, cospi_16_64);

  __m128i in0, in1, in2, in3, in4, in5, in6, in7;
  __m128i stp1_0, stp1_1, stp1_2, stp1_3, stp1_4, stp1_5, stp1_6, stp1_7;
  __m128i stp2_0, stp2_1, stp2_2, stp2_3, stp2_4, stp2_5, stp2_6, stp2_7;
  __m128i tmp0, tmp1, tmp2, tmp3;

  // Rows. Load 4-row input data.
  in0 = load_input_data(input);
  in1 = load_input_data(input + 8 * 1);
  in2 = load_input_data(input + 8 * 2);
  in3 = load_input_data(input + 8 * 3);

  // 8x4 Transpose
  TRANSPOSE_8X8_10(in0, in1, in2, in3, in0, in1);
  // Stage1
  {
    const __m128i lo_17 = _mm_unpackhi_epi16(in0, zero);
    const __m128i lo_35 = _mm_unpackhi_epi16(in1, zero);

    stp1_4 = idct_calc_wraplow_sse2(stg1_0, stg1_1, lo_17);
    stp1_5 = idct_calc_wraplow_sse2(stg1_2, stg1_3, lo_35);
  }

  // Stage2
  {
    const __m128i lo_04 = _mm_unpacklo_epi16(in0, zero);
    const __m128i lo_26 = _mm_unpacklo_epi16(in1, zero);

    stp2_0 = idct_calc_wraplow_sse2(stg2_0, stg2_1, lo_04);
    stp2_2 = idct_calc_wraplow_sse2(stg2_3, stg2_2, lo_26);

    tmp0 = _mm_add_epi16(stp1_4, stp1_5);
    tmp1 = _mm_sub_epi16(stp1_4, stp1_5);

    stp2_4 = tmp0;
    stp2_5 = _mm_unpacklo_epi64(tmp1, zero);
    stp2_6 = _mm_unpackhi_epi64(tmp1, zero);
  }

  // Stage3
  {
    const __m128i lo_56 = _mm_unpacklo_epi16(stp2_5, stp2_6);

    tmp0 = _mm_add_epi16(stp2_0, stp2_2);
    tmp1 = _mm_sub_epi16(stp2_0, stp2_2);
    stp1_2 = _mm_unpackhi_epi64(tmp1, tmp0);
    stp1_3 = _mm_unpacklo_epi64(tmp1, tmp0);
    stp1_5 = idct_calc_wraplow_sse2(stg3_0, stg2_0, lo_56);  // stg3_1 = stg2_0
  }

  // Stage4
  tmp0 = _mm_add_epi16(stp1_3, stp2_4);
  tmp1 = _mm_add_epi16(stp1_2, stp1_5);
  tmp2 = _mm_sub_epi16(stp1_3, stp2_4);
  tmp3 = _mm_sub_epi16(stp1_2, stp1_5);

  TRANSPOSE_4X8_10(tmp0, tmp1, tmp2, tmp3, in0, in1, in2, in3)

  IDCT8(in0, in1, in2, in3, zero, zero, zero, zero, in0, in1, in2, in3, in4,
        in5, in6, in7);
  // Final rounding and shift
  in0 = _mm_adds_epi16(in0, final_rounding);
  in1 = _mm_adds_epi16(in1, final_rounding);
  in2 = _mm_adds_epi16(in2, final_rounding);
  in3 = _mm_adds_epi16(in3, final_rounding);
  in4 = _mm_adds_epi16(in4, final_rounding);
  in5 = _mm_adds_epi16(in5, final_rounding);
  in6 = _mm_adds_epi16(in6, final_rounding);
  in7 = _mm_adds_epi16(in7, final_rounding);

  in0 = _mm_srai_epi16(in0, 5);
  in1 = _mm_srai_epi16(in1, 5);
  in2 = _mm_srai_epi16(in2, 5);
  in3 = _mm_srai_epi16(in3, 5);
  in4 = _mm_srai_epi16(in4, 5);
  in5 = _mm_srai_epi16(in5, 5);
  in6 = _mm_srai_epi16(in6, 5);
  in7 = _mm_srai_epi16(in7, 5);

  RECON_AND_STORE(dest + 0 * stride, in0);
  RECON_AND_STORE(dest + 1 * stride, in1);
  RECON_AND_STORE(dest + 2 * stride, in2);
  RECON_AND_STORE(dest + 3 * stride, in3);
  RECON_AND_STORE(dest + 4 * stride, in4);
  RECON_AND_STORE(dest + 5 * stride, in5);
  RECON_AND_STORE(dest + 6 * stride, in6);
  RECON_AND_STORE(dest + 7 * stride, in7);
}

#define IDCT16                                                                 \
  /* Stage2 */                                                                 \
  {                                                                            \
    const __m128i lo_1_15 = _mm_unpacklo_epi16(in[1], in[15]);                 \
    const __m128i hi_1_15 = _mm_unpackhi_epi16(in[1], in[15]);                 \
    const __m128i lo_9_7 = _mm_unpacklo_epi16(in[9], in[7]);                   \
    const __m128i hi_9_7 = _mm_unpackhi_epi16(in[9], in[7]);                   \
    const __m128i lo_5_11 = _mm_unpacklo_epi16(in[5], in[11]);                 \
    const __m128i hi_5_11 = _mm_unpackhi_epi16(in[5], in[11]);                 \
    const __m128i lo_13_3 = _mm_unpacklo_epi16(in[13], in[3]);                 \
    const __m128i hi_13_3 = _mm_unpackhi_epi16(in[13], in[3]);                 \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_1_15, hi_1_15, lo_9_7, hi_9_7, stg2_0, stg2_1,   \
                           stg2_2, stg2_3, stp2_8, stp2_15, stp2_9, stp2_14)   \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_5_11, hi_5_11, lo_13_3, hi_13_3, stg2_4, stg2_5, \
                           stg2_6, stg2_7, stp2_10, stp2_13, stp2_11, stp2_12) \
  }                                                                            \
                                                                               \
  /* Stage3 */                                                                 \
  {                                                                            \
    const __m128i lo_2_14 = _mm_unpacklo_epi16(in[2], in[14]);                 \
    const __m128i hi_2_14 = _mm_unpackhi_epi16(in[2], in[14]);                 \
    const __m128i lo_10_6 = _mm_unpacklo_epi16(in[10], in[6]);                 \
    const __m128i hi_10_6 = _mm_unpackhi_epi16(in[10], in[6]);                 \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_2_14, hi_2_14, lo_10_6, hi_10_6, stg3_0, stg3_1, \
                           stg3_2, stg3_3, stp1_4, stp1_7, stp1_5, stp1_6)     \
                                                                               \
    stp1_8_0 = _mm_add_epi16(stp2_8, stp2_9);                                  \
    stp1_9 = _mm_sub_epi16(stp2_8, stp2_9);                                    \
    stp1_10 = _mm_sub_epi16(stp2_11, stp2_10);                                 \
    stp1_11 = _mm_add_epi16(stp2_11, stp2_10);                                 \
                                                                               \
    stp1_12_0 = _mm_add_epi16(stp2_12, stp2_13);                               \
    stp1_13 = _mm_sub_epi16(stp2_12, stp2_13);                                 \
    stp1_14 = _mm_sub_epi16(stp2_15, stp2_14);                                 \
    stp1_15 = _mm_add_epi16(stp2_15, stp2_14);                                 \
  }                                                                            \
                                                                               \
  /* Stage4 */                                                                 \
  {                                                                            \
    const __m128i lo_0_8 = _mm_unpacklo_epi16(in[0], in[8]);                   \
    const __m128i hi_0_8 = _mm_unpackhi_epi16(in[0], in[8]);                   \
    const __m128i lo_4_12 = _mm_unpacklo_epi16(in[4], in[12]);                 \
    const __m128i hi_4_12 = _mm_unpackhi_epi16(in[4], in[12]);                 \
                                                                               \
    const __m128i lo_9_14 = _mm_unpacklo_epi16(stp1_9, stp1_14);               \
    const __m128i hi_9_14 = _mm_unpackhi_epi16(stp1_9, stp1_14);               \
    const __m128i lo_10_13 = _mm_unpacklo_epi16(stp1_10, stp1_13);             \
    const __m128i hi_10_13 = _mm_unpackhi_epi16(stp1_10, stp1_13);             \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_0_8, hi_0_8, lo_4_12, hi_4_12, stg4_0, stg4_1,   \
                           stg4_2, stg4_3, stp2_0, stp2_1, stp2_2, stp2_3)     \
                                                                               \
    stp2_4 = _mm_add_epi16(stp1_4, stp1_5);                                    \
    stp2_5 = _mm_sub_epi16(stp1_4, stp1_5);                                    \
    stp2_6 = _mm_sub_epi16(stp1_7, stp1_6);                                    \
    stp2_7 = _mm_add_epi16(stp1_7, stp1_6);                                    \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_9_14, hi_9_14, lo_10_13, hi_10_13, stg4_4,       \
                           stg4_5, stg4_6, stg4_7, stp2_9, stp2_14, stp2_10,   \
                           stp2_13)                                            \
  }                                                                            \
                                                                               \
  /* Stage5 */                                                                 \
  {                                                                            \
    const __m128i lo_6_5 = _mm_unpacklo_epi16(stp2_6, stp2_5);                 \
    const __m128i hi_6_5 = _mm_unpackhi_epi16(stp2_6, stp2_5);                 \
                                                                               \
    stp1_0 = _mm_add_epi16(stp2_0, stp2_3);                                    \
    stp1_1 = _mm_add_epi16(stp2_1, stp2_2);                                    \
    stp1_2 = _mm_sub_epi16(stp2_1, stp2_2);                                    \
    stp1_3 = _mm_sub_epi16(stp2_0, stp2_3);                                    \
                                                                               \
    stp1_5 = idct_calc_wraplow_sse2(lo_6_5, hi_6_5, stg4_1);                   \
    stp1_6 = idct_calc_wraplow_sse2(lo_6_5, hi_6_5, stg4_0);                   \
                                                                               \
    stp1_8 = _mm_add_epi16(stp1_8_0, stp1_11);                                 \
    stp1_9 = _mm_add_epi16(stp2_9, stp2_10);                                   \
    stp1_10 = _mm_sub_epi16(stp2_9, stp2_10);                                  \
    stp1_11 = _mm_sub_epi16(stp1_8_0, stp1_11);                                \
                                                                               \
    stp1_12 = _mm_sub_epi16(stp1_15, stp1_12_0);                               \
    stp1_13 = _mm_sub_epi16(stp2_14, stp2_13);                                 \
    stp1_14 = _mm_add_epi16(stp2_14, stp2_13);                                 \
    stp1_15 = _mm_add_epi16(stp1_15, stp1_12_0);                               \
  }                                                                            \
                                                                               \
  /* Stage6 */                                                                 \
  {                                                                            \
    const __m128i lo_10_13 = _mm_unpacklo_epi16(stp1_10, stp1_13);             \
    const __m128i hi_10_13 = _mm_unpackhi_epi16(stp1_10, stp1_13);             \
    const __m128i lo_11_12 = _mm_unpacklo_epi16(stp1_11, stp1_12);             \
    const __m128i hi_11_12 = _mm_unpackhi_epi16(stp1_11, stp1_12);             \
                                                                               \
    stp2_0 = _mm_add_epi16(stp1_0, stp2_7);                                    \
    stp2_1 = _mm_add_epi16(stp1_1, stp1_6);                                    \
    stp2_2 = _mm_add_epi16(stp1_2, stp1_5);                                    \
    stp2_3 = _mm_add_epi16(stp1_3, stp2_4);                                    \
    stp2_4 = _mm_sub_epi16(stp1_3, stp2_4);                                    \
    stp2_5 = _mm_sub_epi16(stp1_2, stp1_5);                                    \
    stp2_6 = _mm_sub_epi16(stp1_1, stp1_6);                                    \
    stp2_7 = _mm_sub_epi16(stp1_0, stp2_7);                                    \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_10_13, hi_10_13, lo_11_12, hi_11_12, stg6_0,     \
                           stg4_0, stg6_0, stg4_0, stp2_10, stp2_13, stp2_11,  \
                           stp2_12)                                            \
  }

#define IDCT16_10                                                              \
  /* Stage2 */                                                                 \
  {                                                                            \
    const __m128i lo_1_15 = _mm_unpacklo_epi16(in[1], zero);                   \
    const __m128i hi_1_15 = _mm_unpackhi_epi16(in[1], zero);                   \
    const __m128i lo_13_3 = _mm_unpacklo_epi16(zero, in[3]);                   \
    const __m128i hi_13_3 = _mm_unpackhi_epi16(zero, in[3]);                   \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_1_15, hi_1_15, lo_13_3, hi_13_3, stg2_0, stg2_1, \
                           stg2_6, stg2_7, stp1_8_0, stp1_15, stp1_11,         \
                           stp1_12_0)                                          \
  }                                                                            \
                                                                               \
  /* Stage3 */                                                                 \
  {                                                                            \
    const __m128i lo_2_14 = _mm_unpacklo_epi16(in[2], zero);                   \
    const __m128i hi_2_14 = _mm_unpackhi_epi16(in[2], zero);                   \
                                                                               \
    MULTIPLICATION_AND_ADD_2(lo_2_14, hi_2_14, stg3_0, stg3_1, stp2_4, stp2_7) \
                                                                               \
    stp1_9 = stp1_8_0;                                                         \
    stp1_10 = stp1_11;                                                         \
                                                                               \
    stp1_13 = stp1_12_0;                                                       \
    stp1_14 = stp1_15;                                                         \
  }                                                                            \
                                                                               \
  /* Stage4 */                                                                 \
  {                                                                            \
    const __m128i lo_0_8 = _mm_unpacklo_epi16(in[0], zero);                    \
    const __m128i hi_0_8 = _mm_unpackhi_epi16(in[0], zero);                    \
                                                                               \
    const __m128i lo_9_14 = _mm_unpacklo_epi16(stp1_9, stp1_14);               \
    const __m128i hi_9_14 = _mm_unpackhi_epi16(stp1_9, stp1_14);               \
    const __m128i lo_10_13 = _mm_unpacklo_epi16(stp1_10, stp1_13);             \
    const __m128i hi_10_13 = _mm_unpackhi_epi16(stp1_10, stp1_13);             \
                                                                               \
    MULTIPLICATION_AND_ADD_2(lo_0_8, hi_0_8, stg4_0, stg4_1, stp1_0, stp1_1)   \
    stp2_5 = stp2_4;                                                           \
    stp2_6 = stp2_7;                                                           \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_9_14, hi_9_14, lo_10_13, hi_10_13, stg4_4,       \
                           stg4_5, stg4_6, stg4_7, stp2_9, stp2_14, stp2_10,   \
                           stp2_13)                                            \
  }                                                                            \
                                                                               \
  /* Stage5 */                                                                 \
  {                                                                            \
    const __m128i lo_6_5 = _mm_unpacklo_epi16(stp2_6, stp2_5);                 \
    const __m128i hi_6_5 = _mm_unpackhi_epi16(stp2_6, stp2_5);                 \
                                                                               \
    stp1_2 = stp1_1;                                                           \
    stp1_3 = stp1_0;                                                           \
                                                                               \
    stp1_5 = idct_calc_wraplow_sse2(lo_6_5, hi_6_5, stg4_1);                   \
    stp1_6 = idct_calc_wraplow_sse2(lo_6_5, hi_6_5, stg4_0);                   \
                                                                               \
    stp1_8 = _mm_add_epi16(stp1_8_0, stp1_11);                                 \
    stp1_9 = _mm_add_epi16(stp2_9, stp2_10);                                   \
    stp1_10 = _mm_sub_epi16(stp2_9, stp2_10);                                  \
    stp1_11 = _mm_sub_epi16(stp1_8_0, stp1_11);                                \
                                                                               \
    stp1_12 = _mm_sub_epi16(stp1_15, stp1_12_0);                               \
    stp1_13 = _mm_sub_epi16(stp2_14, stp2_13);                                 \
    stp1_14 = _mm_add_epi16(stp2_14, stp2_13);                                 \
    stp1_15 = _mm_add_epi16(stp1_15, stp1_12_0);                               \
  }                                                                            \
                                                                               \
  /* Stage6 */                                                                 \
  {                                                                            \
    const __m128i lo_10_13 = _mm_unpacklo_epi16(stp1_10, stp1_13);             \
    const __m128i hi_10_13 = _mm_unpackhi_epi16(stp1_10, stp1_13);             \
    const __m128i lo_11_12 = _mm_unpacklo_epi16(stp1_11, stp1_12);             \
    const __m128i hi_11_12 = _mm_unpackhi_epi16(stp1_11, stp1_12);             \
                                                                               \
    stp2_0 = _mm_add_epi16(stp1_0, stp2_7);                                    \
    stp2_1 = _mm_add_epi16(stp1_1, stp1_6);                                    \
    stp2_2 = _mm_add_epi16(stp1_2, stp1_5);                                    \
    stp2_3 = _mm_add_epi16(stp1_3, stp2_4);                                    \
    stp2_4 = _mm_sub_epi16(stp1_3, stp2_4);                                    \
    stp2_5 = _mm_sub_epi16(stp1_2, stp1_5);                                    \
    stp2_6 = _mm_sub_epi16(stp1_1, stp1_6);                                    \
    stp2_7 = _mm_sub_epi16(stp1_0, stp2_7);                                    \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_10_13, hi_10_13, lo_11_12, hi_11_12, stg6_0,     \
                           stg4_0, stg6_0, stg4_0, stp2_10, stp2_13, stp2_11,  \
                           stp2_12)                                            \
  }

void vpx_idct16x16_256_add_sse2(const tran_low_t *input, uint8_t *dest,
                                int stride) {
  const __m128i final_rounding = _mm_set1_epi16(1 << 5);
  const __m128i zero = _mm_setzero_si128();

  const __m128i stg2_0 = pair_set_epi16(cospi_30_64, -cospi_2_64);
  const __m128i stg2_1 = pair_set_epi16(cospi_2_64, cospi_30_64);
  const __m128i stg2_2 = pair_set_epi16(cospi_14_64, -cospi_18_64);
  const __m128i stg2_3 = pair_set_epi16(cospi_18_64, cospi_14_64);
  const __m128i stg2_4 = pair_set_epi16(cospi_22_64, -cospi_10_64);
  const __m128i stg2_5 = pair_set_epi16(cospi_10_64, cospi_22_64);
  const __m128i stg2_6 = pair_set_epi16(cospi_6_64, -cospi_26_64);
  const __m128i stg2_7 = pair_set_epi16(cospi_26_64, cospi_6_64);

  const __m128i stg3_0 = pair_set_epi16(cospi_28_64, -cospi_4_64);
  const __m128i stg3_1 = pair_set_epi16(cospi_4_64, cospi_28_64);
  const __m128i stg3_2 = pair_set_epi16(cospi_12_64, -cospi_20_64);
  const __m128i stg3_3 = pair_set_epi16(cospi_20_64, cospi_12_64);

  const __m128i stg4_0 = pair_set_epi16(cospi_16_64, cospi_16_64);
  const __m128i stg4_1 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i stg4_2 = pair_set_epi16(cospi_24_64, -cospi_8_64);
  const __m128i stg4_3 = pair_set_epi16(cospi_8_64, cospi_24_64);
  const __m128i stg4_4 = pair_set_epi16(-cospi_8_64, cospi_24_64);
  const __m128i stg4_5 = pair_set_epi16(cospi_24_64, cospi_8_64);
  const __m128i stg4_6 = pair_set_epi16(-cospi_24_64, -cospi_8_64);
  const __m128i stg4_7 = pair_set_epi16(-cospi_8_64, cospi_24_64);

  const __m128i stg6_0 = pair_set_epi16(-cospi_16_64, cospi_16_64);

  __m128i in[16], l[16], r[16], *curr1;
  __m128i stp1_0, stp1_1, stp1_2, stp1_3, stp1_4, stp1_5, stp1_6, stp1_7,
      stp1_8, stp1_9, stp1_10, stp1_11, stp1_12, stp1_13, stp1_14, stp1_15,
      stp1_8_0, stp1_12_0;
  __m128i stp2_0, stp2_1, stp2_2, stp2_3, stp2_4, stp2_5, stp2_6, stp2_7,
      stp2_8, stp2_9, stp2_10, stp2_11, stp2_12, stp2_13, stp2_14, stp2_15;
  int i;

  curr1 = l;
  for (i = 0; i < 2; i++) {
    // 1-D idct

    // Load input data.
    in[0] = load_input_data(input);
    in[8] = load_input_data(input + 8 * 1);
    in[1] = load_input_data(input + 8 * 2);
    in[9] = load_input_data(input + 8 * 3);
    in[2] = load_input_data(input + 8 * 4);
    in[10] = load_input_data(input + 8 * 5);
    in[3] = load_input_data(input + 8 * 6);
    in[11] = load_input_data(input + 8 * 7);
    in[4] = load_input_data(input + 8 * 8);
    in[12] = load_input_data(input + 8 * 9);
    in[5] = load_input_data(input + 8 * 10);
    in[13] = load_input_data(input + 8 * 11);
    in[6] = load_input_data(input + 8 * 12);
    in[14] = load_input_data(input + 8 * 13);
    in[7] = load_input_data(input + 8 * 14);
    in[15] = load_input_data(input + 8 * 15);

    array_transpose_8x8(in, in);
    array_transpose_8x8(in + 8, in + 8);

    IDCT16

    // Stage7
    curr1[0] = _mm_add_epi16(stp2_0, stp1_15);
    curr1[1] = _mm_add_epi16(stp2_1, stp1_14);
    curr1[2] = _mm_add_epi16(stp2_2, stp2_13);
    curr1[3] = _mm_add_epi16(stp2_3, stp2_12);
    curr1[4] = _mm_add_epi16(stp2_4, stp2_11);
    curr1[5] = _mm_add_epi16(stp2_5, stp2_10);
    curr1[6] = _mm_add_epi16(stp2_6, stp1_9);
    curr1[7] = _mm_add_epi16(stp2_7, stp1_8);
    curr1[8] = _mm_sub_epi16(stp2_7, stp1_8);
    curr1[9] = _mm_sub_epi16(stp2_6, stp1_9);
    curr1[10] = _mm_sub_epi16(stp2_5, stp2_10);
    curr1[11] = _mm_sub_epi16(stp2_4, stp2_11);
    curr1[12] = _mm_sub_epi16(stp2_3, stp2_12);
    curr1[13] = _mm_sub_epi16(stp2_2, stp2_13);
    curr1[14] = _mm_sub_epi16(stp2_1, stp1_14);
    curr1[15] = _mm_sub_epi16(stp2_0, stp1_15);

    curr1 = r;
    input += 128;
  }
  for (i = 0; i < 2; i++) {
    int j;
    // 1-D idct
    array_transpose_8x8(l + i * 8, in);
    array_transpose_8x8(r + i * 8, in + 8);

    IDCT16

    // 2-D
    in[0] = _mm_add_epi16(stp2_0, stp1_15);
    in[1] = _mm_add_epi16(stp2_1, stp1_14);
    in[2] = _mm_add_epi16(stp2_2, stp2_13);
    in[3] = _mm_add_epi16(stp2_3, stp2_12);
    in[4] = _mm_add_epi16(stp2_4, stp2_11);
    in[5] = _mm_add_epi16(stp2_5, stp2_10);
    in[6] = _mm_add_epi16(stp2_6, stp1_9);
    in[7] = _mm_add_epi16(stp2_7, stp1_8);
    in[8] = _mm_sub_epi16(stp2_7, stp1_8);
    in[9] = _mm_sub_epi16(stp2_6, stp1_9);
    in[10] = _mm_sub_epi16(stp2_5, stp2_10);
    in[11] = _mm_sub_epi16(stp2_4, stp2_11);
    in[12] = _mm_sub_epi16(stp2_3, stp2_12);
    in[13] = _mm_sub_epi16(stp2_2, stp2_13);
    in[14] = _mm_sub_epi16(stp2_1, stp1_14);
    in[15] = _mm_sub_epi16(stp2_0, stp1_15);

    for (j = 0; j < 16; ++j) {
      // Final rounding and shift
      in[j] = _mm_adds_epi16(in[j], final_rounding);
      in[j] = _mm_srai_epi16(in[j], 6);
      RECON_AND_STORE(dest + j * stride, in[j]);
    }

    dest += 8;
  }
}

void vpx_idct16x16_1_add_sse2(const tran_low_t *input, uint8_t *dest,
                              int stride) {
  __m128i dc_value;
  const __m128i zero = _mm_setzero_si128();
  int a, i;

  a = (int)dct_const_round_shift(input[0] * cospi_16_64);
  a = (int)dct_const_round_shift(a * cospi_16_64);
  a = ROUND_POWER_OF_TWO(a, 6);

  dc_value = _mm_set1_epi16(a);

  for (i = 0; i < 16; ++i) {
    RECON_AND_STORE(dest + 0, dc_value);
    RECON_AND_STORE(dest + 8, dc_value);
    dest += stride;
  }
}

static void iadst16_8col(__m128i *in) {
  // perform 16x16 1-D ADST for 8 columns
  __m128i s[16], x[16], u[32], v[32];
  const __m128i k__cospi_p01_p31 = pair_set_epi16(cospi_1_64, cospi_31_64);
  const __m128i k__cospi_p31_m01 = pair_set_epi16(cospi_31_64, -cospi_1_64);
  const __m128i k__cospi_p05_p27 = pair_set_epi16(cospi_5_64, cospi_27_64);
  const __m128i k__cospi_p27_m05 = pair_set_epi16(cospi_27_64, -cospi_5_64);
  const __m128i k__cospi_p09_p23 = pair_set_epi16(cospi_9_64, cospi_23_64);
  const __m128i k__cospi_p23_m09 = pair_set_epi16(cospi_23_64, -cospi_9_64);
  const __m128i k__cospi_p13_p19 = pair_set_epi16(cospi_13_64, cospi_19_64);
  const __m128i k__cospi_p19_m13 = pair_set_epi16(cospi_19_64, -cospi_13_64);
  const __m128i k__cospi_p17_p15 = pair_set_epi16(cospi_17_64, cospi_15_64);
  const __m128i k__cospi_p15_m17 = pair_set_epi16(cospi_15_64, -cospi_17_64);
  const __m128i k__cospi_p21_p11 = pair_set_epi16(cospi_21_64, cospi_11_64);
  const __m128i k__cospi_p11_m21 = pair_set_epi16(cospi_11_64, -cospi_21_64);
  const __m128i k__cospi_p25_p07 = pair_set_epi16(cospi_25_64, cospi_7_64);
  const __m128i k__cospi_p07_m25 = pair_set_epi16(cospi_7_64, -cospi_25_64);
  const __m128i k__cospi_p29_p03 = pair_set_epi16(cospi_29_64, cospi_3_64);
  const __m128i k__cospi_p03_m29 = pair_set_epi16(cospi_3_64, -cospi_29_64);
  const __m128i k__cospi_p04_p28 = pair_set_epi16(cospi_4_64, cospi_28_64);
  const __m128i k__cospi_p28_m04 = pair_set_epi16(cospi_28_64, -cospi_4_64);
  const __m128i k__cospi_p20_p12 = pair_set_epi16(cospi_20_64, cospi_12_64);
  const __m128i k__cospi_p12_m20 = pair_set_epi16(cospi_12_64, -cospi_20_64);
  const __m128i k__cospi_m28_p04 = pair_set_epi16(-cospi_28_64, cospi_4_64);
  const __m128i k__cospi_m12_p20 = pair_set_epi16(-cospi_12_64, cospi_20_64);
  const __m128i k__cospi_p08_p24 = pair_set_epi16(cospi_8_64, cospi_24_64);
  const __m128i k__cospi_p24_m08 = pair_set_epi16(cospi_24_64, -cospi_8_64);
  const __m128i k__cospi_m24_p08 = pair_set_epi16(-cospi_24_64, cospi_8_64);
  const __m128i k__cospi_m16_m16 = _mm_set1_epi16((int16_t)-cospi_16_64);
  const __m128i k__cospi_p16_p16 = _mm_set1_epi16((int16_t)cospi_16_64);
  const __m128i k__cospi_p16_m16 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i k__cospi_m16_p16 = pair_set_epi16(-cospi_16_64, cospi_16_64);
  const __m128i k__DCT_CONST_ROUNDING = _mm_set1_epi32(DCT_CONST_ROUNDING);
  const __m128i kZero = _mm_set1_epi16(0);

  u[0] = _mm_unpacklo_epi16(in[15], in[0]);
  u[1] = _mm_unpackhi_epi16(in[15], in[0]);
  u[2] = _mm_unpacklo_epi16(in[13], in[2]);
  u[3] = _mm_unpackhi_epi16(in[13], in[2]);
  u[4] = _mm_unpacklo_epi16(in[11], in[4]);
  u[5] = _mm_unpackhi_epi16(in[11], in[4]);
  u[6] = _mm_unpacklo_epi16(in[9], in[6]);
  u[7] = _mm_unpackhi_epi16(in[9], in[6]);
  u[8] = _mm_unpacklo_epi16(in[7], in[8]);
  u[9] = _mm_unpackhi_epi16(in[7], in[8]);
  u[10] = _mm_unpacklo_epi16(in[5], in[10]);
  u[11] = _mm_unpackhi_epi16(in[5], in[10]);
  u[12] = _mm_unpacklo_epi16(in[3], in[12]);
  u[13] = _mm_unpackhi_epi16(in[3], in[12]);
  u[14] = _mm_unpacklo_epi16(in[1], in[14]);
  u[15] = _mm_unpackhi_epi16(in[1], in[14]);

  v[0] = _mm_madd_epi16(u[0], k__cospi_p01_p31);
  v[1] = _mm_madd_epi16(u[1], k__cospi_p01_p31);
  v[2] = _mm_madd_epi16(u[0], k__cospi_p31_m01);
  v[3] = _mm_madd_epi16(u[1], k__cospi_p31_m01);
  v[4] = _mm_madd_epi16(u[2], k__cospi_p05_p27);
  v[5] = _mm_madd_epi16(u[3], k__cospi_p05_p27);
  v[6] = _mm_madd_epi16(u[2], k__cospi_p27_m05);
  v[7] = _mm_madd_epi16(u[3], k__cospi_p27_m05);
  v[8] = _mm_madd_epi16(u[4], k__cospi_p09_p23);
  v[9] = _mm_madd_epi16(u[5], k__cospi_p09_p23);
  v[10] = _mm_madd_epi16(u[4], k__cospi_p23_m09);
  v[11] = _mm_madd_epi16(u[5], k__cospi_p23_m09);
  v[12] = _mm_madd_epi16(u[6], k__cospi_p13_p19);
  v[13] = _mm_madd_epi16(u[7], k__cospi_p13_p19);
  v[14] = _mm_madd_epi16(u[6], k__cospi_p19_m13);
  v[15] = _mm_madd_epi16(u[7], k__cospi_p19_m13);
  v[16] = _mm_madd_epi16(u[8], k__cospi_p17_p15);
  v[17] = _mm_madd_epi16(u[9], k__cospi_p17_p15);
  v[18] = _mm_madd_epi16(u[8], k__cospi_p15_m17);
  v[19] = _mm_madd_epi16(u[9], k__cospi_p15_m17);
  v[20] = _mm_madd_epi16(u[10], k__cospi_p21_p11);
  v[21] = _mm_madd_epi16(u[11], k__cospi_p21_p11);
  v[22] = _mm_madd_epi16(u[10], k__cospi_p11_m21);
  v[23] = _mm_madd_epi16(u[11], k__cospi_p11_m21);
  v[24] = _mm_madd_epi16(u[12], k__cospi_p25_p07);
  v[25] = _mm_madd_epi16(u[13], k__cospi_p25_p07);
  v[26] = _mm_madd_epi16(u[12], k__cospi_p07_m25);
  v[27] = _mm_madd_epi16(u[13], k__cospi_p07_m25);
  v[28] = _mm_madd_epi16(u[14], k__cospi_p29_p03);
  v[29] = _mm_madd_epi16(u[15], k__cospi_p29_p03);
  v[30] = _mm_madd_epi16(u[14], k__cospi_p03_m29);
  v[31] = _mm_madd_epi16(u[15], k__cospi_p03_m29);

  u[0] = _mm_add_epi32(v[0], v[16]);
  u[1] = _mm_add_epi32(v[1], v[17]);
  u[2] = _mm_add_epi32(v[2], v[18]);
  u[3] = _mm_add_epi32(v[3], v[19]);
  u[4] = _mm_add_epi32(v[4], v[20]);
  u[5] = _mm_add_epi32(v[5], v[21]);
  u[6] = _mm_add_epi32(v[6], v[22]);
  u[7] = _mm_add_epi32(v[7], v[23]);
  u[8] = _mm_add_epi32(v[8], v[24]);
  u[9] = _mm_add_epi32(v[9], v[25]);
  u[10] = _mm_add_epi32(v[10], v[26]);
  u[11] = _mm_add_epi32(v[11], v[27]);
  u[12] = _mm_add_epi32(v[12], v[28]);
  u[13] = _mm_add_epi32(v[13], v[29]);
  u[14] = _mm_add_epi32(v[14], v[30]);
  u[15] = _mm_add_epi32(v[15], v[31]);
  u[16] = _mm_sub_epi32(v[0], v[16]);
  u[17] = _mm_sub_epi32(v[1], v[17]);
  u[18] = _mm_sub_epi32(v[2], v[18]);
  u[19] = _mm_sub_epi32(v[3], v[19]);
  u[20] = _mm_sub_epi32(v[4], v[20]);
  u[21] = _mm_sub_epi32(v[5], v[21]);
  u[22] = _mm_sub_epi32(v[6], v[22]);
  u[23] = _mm_sub_epi32(v[7], v[23]);
  u[24] = _mm_sub_epi32(v[8], v[24]);
  u[25] = _mm_sub_epi32(v[9], v[25]);
  u[26] = _mm_sub_epi32(v[10], v[26]);
  u[27] = _mm_sub_epi32(v[11], v[27]);
  u[28] = _mm_sub_epi32(v[12], v[28]);
  u[29] = _mm_sub_epi32(v[13], v[29]);
  u[30] = _mm_sub_epi32(v[14], v[30]);
  u[31] = _mm_sub_epi32(v[15], v[31]);

  v[0] = _mm_add_epi32(u[0], k__DCT_CONST_ROUNDING);
  v[1] = _mm_add_epi32(u[1], k__DCT_CONST_ROUNDING);
  v[2] = _mm_add_epi32(u[2], k__DCT_CONST_ROUNDING);
  v[3] = _mm_add_epi32(u[3], k__DCT_CONST_ROUNDING);
  v[4] = _mm_add_epi32(u[4], k__DCT_CONST_ROUNDING);
  v[5] = _mm_add_epi32(u[5], k__DCT_CONST_ROUNDING);
  v[6] = _mm_add_epi32(u[6], k__DCT_CONST_ROUNDING);
  v[7] = _mm_add_epi32(u[7], k__DCT_CONST_ROUNDING);
  v[8] = _mm_add_epi32(u[8], k__DCT_CONST_ROUNDING);
  v[9] = _mm_add_epi32(u[9], k__DCT_CONST_ROUNDING);
  v[10] = _mm_add_epi32(u[10], k__DCT_CONST_ROUNDING);
  v[11] = _mm_add_epi32(u[11], k__DCT_CONST_ROUNDING);
  v[12] = _mm_add_epi32(u[12], k__DCT_CONST_ROUNDING);
  v[13] = _mm_add_epi32(u[13], k__DCT_CONST_ROUNDING);
  v[14] = _mm_add_epi32(u[14], k__DCT_CONST_ROUNDING);
  v[15] = _mm_add_epi32(u[15], k__DCT_CONST_ROUNDING);
  v[16] = _mm_add_epi32(u[16], k__DCT_CONST_ROUNDING);
  v[17] = _mm_add_epi32(u[17], k__DCT_CONST_ROUNDING);
  v[18] = _mm_add_epi32(u[18], k__DCT_CONST_ROUNDING);
  v[19] = _mm_add_epi32(u[19], k__DCT_CONST_ROUNDING);
  v[20] = _mm_add_epi32(u[20], k__DCT_CONST_ROUNDING);
  v[21] = _mm_add_epi32(u[21], k__DCT_CONST_ROUNDING);
  v[22] = _mm_add_epi32(u[22], k__DCT_CONST_ROUNDING);
  v[23] = _mm_add_epi32(u[23], k__DCT_CONST_ROUNDING);
  v[24] = _mm_add_epi32(u[24], k__DCT_CONST_ROUNDING);
  v[25] = _mm_add_epi32(u[25], k__DCT_CONST_ROUNDING);
  v[26] = _mm_add_epi32(u[26], k__DCT_CONST_ROUNDING);
  v[27] = _mm_add_epi32(u[27], k__DCT_CONST_ROUNDING);
  v[28] = _mm_add_epi32(u[28], k__DCT_CONST_ROUNDING);
  v[29] = _mm_add_epi32(u[29], k__DCT_CONST_ROUNDING);
  v[30] = _mm_add_epi32(u[30], k__DCT_CONST_ROUNDING);
  v[31] = _mm_add_epi32(u[31], k__DCT_CONST_ROUNDING);

  u[0] = _mm_srai_epi32(v[0], DCT_CONST_BITS);
  u[1] = _mm_srai_epi32(v[1], DCT_CONST_BITS);
  u[2] = _mm_srai_epi32(v[2], DCT_CONST_BITS);
  u[3] = _mm_srai_epi32(v[3], DCT_CONST_BITS);
  u[4] = _mm_srai_epi32(v[4], DCT_CONST_BITS);
  u[5] = _mm_srai_epi32(v[5], DCT_CONST_BITS);
  u[6] = _mm_srai_epi32(v[6], DCT_CONST_BITS);
  u[7] = _mm_srai_epi32(v[7], DCT_CONST_BITS);
  u[8] = _mm_srai_epi32(v[8], DCT_CONST_BITS);
  u[9] = _mm_srai_epi32(v[9], DCT_CONST_BITS);
  u[10] = _mm_srai_epi32(v[10], DCT_CONST_BITS);
  u[11] = _mm_srai_epi32(v[11], DCT_CONST_BITS);
  u[12] = _mm_srai_epi32(v[12], DCT_CONST_BITS);
  u[13] = _mm_srai_epi32(v[13], DCT_CONST_BITS);
  u[14] = _mm_srai_epi32(v[14], DCT_CONST_BITS);
  u[15] = _mm_srai_epi32(v[15], DCT_CONST_BITS);
  u[16] = _mm_srai_epi32(v[16], DCT_CONST_BITS);
  u[17] = _mm_srai_epi32(v[17], DCT_CONST_BITS);
  u[18] = _mm_srai_epi32(v[18], DCT_CONST_BITS);
  u[19] = _mm_srai_epi32(v[19], DCT_CONST_BITS);
  u[20] = _mm_srai_epi32(v[20], DCT_CONST_BITS);
  u[21] = _mm_srai_epi32(v[21], DCT_CONST_BITS);
  u[22] = _mm_srai_epi32(v[22], DCT_CONST_BITS);
  u[23] = _mm_srai_epi32(v[23], DCT_CONST_BITS);
  u[24] = _mm_srai_epi32(v[24], DCT_CONST_BITS);
  u[25] = _mm_srai_epi32(v[25], DCT_CONST_BITS);
  u[26] = _mm_srai_epi32(v[26], DCT_CONST_BITS);
  u[27] = _mm_srai_epi32(v[27], DCT_CONST_BITS);
  u[28] = _mm_srai_epi32(v[28], DCT_CONST_BITS);
  u[29] = _mm_srai_epi32(v[29], DCT_CONST_BITS);
  u[30] = _mm_srai_epi32(v[30], DCT_CONST_BITS);
  u[31] = _mm_srai_epi32(v[31], DCT_CONST_BITS);

  s[0] = _mm_packs_epi32(u[0], u[1]);
  s[1] = _mm_packs_epi32(u[2], u[3]);
  s[2] = _mm_packs_epi32(u[4], u[5]);
  s[3] = _mm_packs_epi32(u[6], u[7]);
  s[4] = _mm_packs_epi32(u[8], u[9]);
  s[5] = _mm_packs_epi32(u[10], u[11]);
  s[6] = _mm_packs_epi32(u[12], u[13]);
  s[7] = _mm_packs_epi32(u[14], u[15]);
  s[8] = _mm_packs_epi32(u[16], u[17]);
  s[9] = _mm_packs_epi32(u[18], u[19]);
  s[10] = _mm_packs_epi32(u[20], u[21]);
  s[11] = _mm_packs_epi32(u[22], u[23]);
  s[12] = _mm_packs_epi32(u[24], u[25]);
  s[13] = _mm_packs_epi32(u[26], u[27]);
  s[14] = _mm_packs_epi32(u[28], u[29]);
  s[15] = _mm_packs_epi32(u[30], u[31]);

  // stage 2
  u[0] = _mm_unpacklo_epi16(s[8], s[9]);
  u[1] = _mm_unpackhi_epi16(s[8], s[9]);
  u[2] = _mm_unpacklo_epi16(s[10], s[11]);
  u[3] = _mm_unpackhi_epi16(s[10], s[11]);
  u[4] = _mm_unpacklo_epi16(s[12], s[13]);
  u[5] = _mm_unpackhi_epi16(s[12], s[13]);
  u[6] = _mm_unpacklo_epi16(s[14], s[15]);
  u[7] = _mm_unpackhi_epi16(s[14], s[15]);

  v[0] = _mm_madd_epi16(u[0], k__cospi_p04_p28);
  v[1] = _mm_madd_epi16(u[1], k__cospi_p04_p28);
  v[2] = _mm_madd_epi16(u[0], k__cospi_p28_m04);
  v[3] = _mm_madd_epi16(u[1], k__cospi_p28_m04);
  v[4] = _mm_madd_epi16(u[2], k__cospi_p20_p12);
  v[5] = _mm_madd_epi16(u[3], k__cospi_p20_p12);
  v[6] = _mm_madd_epi16(u[2], k__cospi_p12_m20);
  v[7] = _mm_madd_epi16(u[3], k__cospi_p12_m20);
  v[8] = _mm_madd_epi16(u[4], k__cospi_m28_p04);
  v[9] = _mm_madd_epi16(u[5], k__cospi_m28_p04);
  v[10] = _mm_madd_epi16(u[4], k__cospi_p04_p28);
  v[11] = _mm_madd_epi16(u[5], k__cospi_p04_p28);
  v[12] = _mm_madd_epi16(u[6], k__cospi_m12_p20);
  v[13] = _mm_madd_epi16(u[7], k__cospi_m12_p20);
  v[14] = _mm_madd_epi16(u[6], k__cospi_p20_p12);
  v[15] = _mm_madd_epi16(u[7], k__cospi_p20_p12);

  u[0] = _mm_add_epi32(v[0], v[8]);
  u[1] = _mm_add_epi32(v[1], v[9]);
  u[2] = _mm_add_epi32(v[2], v[10]);
  u[3] = _mm_add_epi32(v[3], v[11]);
  u[4] = _mm_add_epi32(v[4], v[12]);
  u[5] = _mm_add_epi32(v[5], v[13]);
  u[6] = _mm_add_epi32(v[6], v[14]);
  u[7] = _mm_add_epi32(v[7], v[15]);
  u[8] = _mm_sub_epi32(v[0], v[8]);
  u[9] = _mm_sub_epi32(v[1], v[9]);
  u[10] = _mm_sub_epi32(v[2], v[10]);
  u[11] = _mm_sub_epi32(v[3], v[11]);
  u[12] = _mm_sub_epi32(v[4], v[12]);
  u[13] = _mm_sub_epi32(v[5], v[13]);
  u[14] = _mm_sub_epi32(v[6], v[14]);
  u[15] = _mm_sub_epi32(v[7], v[15]);

  v[0] = _mm_add_epi32(u[0], k__DCT_CONST_ROUNDING);
  v[1] = _mm_add_epi32(u[1], k__DCT_CONST_ROUNDING);
  v[2] = _mm_add_epi32(u[2], k__DCT_CONST_ROUNDING);
  v[3] = _mm_add_epi32(u[3], k__DCT_CONST_ROUNDING);
  v[4] = _mm_add_epi32(u[4], k__DCT_CONST_ROUNDING);
  v[5] = _mm_add_epi32(u[5], k__DCT_CONST_ROUNDING);
  v[6] = _mm_add_epi32(u[6], k__DCT_CONST_ROUNDING);
  v[7] = _mm_add_epi32(u[7], k__DCT_CONST_ROUNDING);
  v[8] = _mm_add_epi32(u[8], k__DCT_CONST_ROUNDING);
  v[9] = _mm_add_epi32(u[9], k__DCT_CONST_ROUNDING);
  v[10] = _mm_add_epi32(u[10], k__DCT_CONST_ROUNDING);
  v[11] = _mm_add_epi32(u[11], k__DCT_CONST_ROUNDING);
  v[12] = _mm_add_epi32(u[12], k__DCT_CONST_ROUNDING);
  v[13] = _mm_add_epi32(u[13], k__DCT_CONST_ROUNDING);
  v[14] = _mm_add_epi32(u[14], k__DCT_CONST_ROUNDING);
  v[15] = _mm_add_epi32(u[15], k__DCT_CONST_ROUNDING);

  u[0] = _mm_srai_epi32(v[0], DCT_CONST_BITS);
  u[1] = _mm_srai_epi32(v[1], DCT_CONST_BITS);
  u[2] = _mm_srai_epi32(v[2], DCT_CONST_BITS);
  u[3] = _mm_srai_epi32(v[3], DCT_CONST_BITS);
  u[4] = _mm_srai_epi32(v[4], DCT_CONST_BITS);
  u[5] = _mm_srai_epi32(v[5], DCT_CONST_BITS);
  u[6] = _mm_srai_epi32(v[6], DCT_CONST_BITS);
  u[7] = _mm_srai_epi32(v[7], DCT_CONST_BITS);
  u[8] = _mm_srai_epi32(v[8], DCT_CONST_BITS);
  u[9] = _mm_srai_epi32(v[9], DCT_CONST_BITS);
  u[10] = _mm_srai_epi32(v[10], DCT_CONST_BITS);
  u[11] = _mm_srai_epi32(v[11], DCT_CONST_BITS);
  u[12] = _mm_srai_epi32(v[12], DCT_CONST_BITS);
  u[13] = _mm_srai_epi32(v[13], DCT_CONST_BITS);
  u[14] = _mm_srai_epi32(v[14], DCT_CONST_BITS);
  u[15] = _mm_srai_epi32(v[15], DCT_CONST_BITS);

  x[0] = _mm_add_epi16(s[0], s[4]);
  x[1] = _mm_add_epi16(s[1], s[5]);
  x[2] = _mm_add_epi16(s[2], s[6]);
  x[3] = _mm_add_epi16(s[3], s[7]);
  x[4] = _mm_sub_epi16(s[0], s[4]);
  x[5] = _mm_sub_epi16(s[1], s[5]);
  x[6] = _mm_sub_epi16(s[2], s[6]);
  x[7] = _mm_sub_epi16(s[3], s[7]);
  x[8] = _mm_packs_epi32(u[0], u[1]);
  x[9] = _mm_packs_epi32(u[2], u[3]);
  x[10] = _mm_packs_epi32(u[4], u[5]);
  x[11] = _mm_packs_epi32(u[6], u[7]);
  x[12] = _mm_packs_epi32(u[8], u[9]);
  x[13] = _mm_packs_epi32(u[10], u[11]);
  x[14] = _mm_packs_epi32(u[12], u[13]);
  x[15] = _mm_packs_epi32(u[14], u[15]);

  // stage 3
  u[0] = _mm_unpacklo_epi16(x[4], x[5]);
  u[1] = _mm_unpackhi_epi16(x[4], x[5]);
  u[2] = _mm_unpacklo_epi16(x[6], x[7]);
  u[3] = _mm_unpackhi_epi16(x[6], x[7]);
  u[4] = _mm_unpacklo_epi16(x[12], x[13]);
  u[5] = _mm_unpackhi_epi16(x[12], x[13]);
  u[6] = _mm_unpacklo_epi16(x[14], x[15]);
  u[7] = _mm_unpackhi_epi16(x[14], x[15]);

  v[0] = _mm_madd_epi16(u[0], k__cospi_p08_p24);
  v[1] = _mm_madd_epi16(u[1], k__cospi_p08_p24);
  v[2] = _mm_madd_epi16(u[0], k__cospi_p24_m08);
  v[3] = _mm_madd_epi16(u[1], k__cospi_p24_m08);
  v[4] = _mm_madd_epi16(u[2], k__cospi_m24_p08);
  v[5] = _mm_madd_epi16(u[3], k__cospi_m24_p08);
  v[6] = _mm_madd_epi16(u[2], k__cospi_p08_p24);
  v[7] = _mm_madd_epi16(u[3], k__cospi_p08_p24);
  v[8] = _mm_madd_epi16(u[4], k__cospi_p08_p24);
  v[9] = _mm_madd_epi16(u[5], k__cospi_p08_p24);
  v[10] = _mm_madd_epi16(u[4], k__cospi_p24_m08);
  v[11] = _mm_madd_epi16(u[5], k__cospi_p24_m08);
  v[12] = _mm_madd_epi16(u[6], k__cospi_m24_p08);
  v[13] = _mm_madd_epi16(u[7], k__cospi_m24_p08);
  v[14] = _mm_madd_epi16(u[6], k__cospi_p08_p24);
  v[15] = _mm_madd_epi16(u[7], k__cospi_p08_p24);

  u[0] = _mm_add_epi32(v[0], v[4]);
  u[1] = _mm_add_epi32(v[1], v[5]);
  u[2] = _mm_add_epi32(v[2], v[6]);
  u[3] = _mm_add_epi32(v[3], v[7]);
  u[4] = _mm_sub_epi32(v[0], v[4]);
  u[5] = _mm_sub_epi32(v[1], v[5]);
  u[6] = _mm_sub_epi32(v[2], v[6]);
  u[7] = _mm_sub_epi32(v[3], v[7]);
  u[8] = _mm_add_epi32(v[8], v[12]);
  u[9] = _mm_add_epi32(v[9], v[13]);
  u[10] = _mm_add_epi32(v[10], v[14]);
  u[11] = _mm_add_epi32(v[11], v[15]);
  u[12] = _mm_sub_epi32(v[8], v[12]);
  u[13] = _mm_sub_epi32(v[9], v[13]);
  u[14] = _mm_sub_epi32(v[10], v[14]);
  u[15] = _mm_sub_epi32(v[11], v[15]);

  u[0] = _mm_add_epi32(u[0], k__DCT_CONST_ROUNDING);
  u[1] = _mm_add_epi32(u[1], k__DCT_CONST_ROUNDING);
  u[2] = _mm_add_epi32(u[2], k__DCT_CONST_ROUNDING);
  u[3] = _mm_add_epi32(u[3], k__DCT_CONST_ROUNDING);
  u[4] = _mm_add_epi32(u[4], k__DCT_CONST_ROUNDING);
  u[5] = _mm_add_epi32(u[5], k__DCT_CONST_ROUNDING);
  u[6] = _mm_add_epi32(u[6], k__DCT_CONST_ROUNDING);
  u[7] = _mm_add_epi32(u[7], k__DCT_CONST_ROUNDING);
  u[8] = _mm_add_epi32(u[8], k__DCT_CONST_ROUNDING);
  u[9] = _mm_add_epi32(u[9], k__DCT_CONST_ROUNDING);
  u[10] = _mm_add_epi32(u[10], k__DCT_CONST_ROUNDING);
  u[11] = _mm_add_epi32(u[11], k__DCT_CONST_ROUNDING);
  u[12] = _mm_add_epi32(u[12], k__DCT_CONST_ROUNDING);
  u[13] = _mm_add_epi32(u[13], k__DCT_CONST_ROUNDING);
  u[14] = _mm_add_epi32(u[14], k__DCT_CONST_ROUNDING);
  u[15] = _mm_add_epi32(u[15], k__DCT_CONST_ROUNDING);

  v[0] = _mm_srai_epi32(u[0], DCT_CONST_BITS);
  v[1] = _mm_srai_epi32(u[1], DCT_CONST_BITS);
  v[2] = _mm_srai_epi32(u[2], DCT_CONST_BITS);
  v[3] = _mm_srai_epi32(u[3], DCT_CONST_BITS);
  v[4] = _mm_srai_epi32(u[4], DCT_CONST_BITS);
  v[5] = _mm_srai_epi32(u[5], DCT_CONST_BITS);
  v[6] = _mm_srai_epi32(u[6], DCT_CONST_BITS);
  v[7] = _mm_srai_epi32(u[7], DCT_CONST_BITS);
  v[8] = _mm_srai_epi32(u[8], DCT_CONST_BITS);
  v[9] = _mm_srai_epi32(u[9], DCT_CONST_BITS);
  v[10] = _mm_srai_epi32(u[10], DCT_CONST_BITS);
  v[11] = _mm_srai_epi32(u[11], DCT_CONST_BITS);
  v[12] = _mm_srai_epi32(u[12], DCT_CONST_BITS);
  v[13] = _mm_srai_epi32(u[13], DCT_CONST_BITS);
  v[14] = _mm_srai_epi32(u[14], DCT_CONST_BITS);
  v[15] = _mm_srai_epi32(u[15], DCT_CONST_BITS);

  s[0] = _mm_add_epi16(x[0], x[2]);
  s[1] = _mm_add_epi16(x[1], x[3]);
  s[2] = _mm_sub_epi16(x[0], x[2]);
  s[3] = _mm_sub_epi16(x[1], x[3]);
  s[4] = _mm_packs_epi32(v[0], v[1]);
  s[5] = _mm_packs_epi32(v[2], v[3]);
  s[6] = _mm_packs_epi32(v[4], v[5]);
  s[7] = _mm_packs_epi32(v[6], v[7]);
  s[8] = _mm_add_epi16(x[8], x[10]);
  s[9] = _mm_add_epi16(x[9], x[11]);
  s[10] = _mm_sub_epi16(x[8], x[10]);
  s[11] = _mm_sub_epi16(x[9], x[11]);
  s[12] = _mm_packs_epi32(v[8], v[9]);
  s[13] = _mm_packs_epi32(v[10], v[11]);
  s[14] = _mm_packs_epi32(v[12], v[13]);
  s[15] = _mm_packs_epi32(v[14], v[15]);

  // stage 4
  u[0] = _mm_unpacklo_epi16(s[2], s[3]);
  u[1] = _mm_unpackhi_epi16(s[2], s[3]);
  u[2] = _mm_unpacklo_epi16(s[6], s[7]);
  u[3] = _mm_unpackhi_epi16(s[6], s[7]);
  u[4] = _mm_unpacklo_epi16(s[10], s[11]);
  u[5] = _mm_unpackhi_epi16(s[10], s[11]);
  u[6] = _mm_unpacklo_epi16(s[14], s[15]);
  u[7] = _mm_unpackhi_epi16(s[14], s[15]);

  in[7] = idct_calc_wraplow_sse2(u[0], u[1], k__cospi_m16_m16);
  in[8] = idct_calc_wraplow_sse2(u[0], u[1], k__cospi_p16_m16);
  in[4] = idct_calc_wraplow_sse2(u[2], u[3], k__cospi_p16_p16);
  in[11] = idct_calc_wraplow_sse2(u[2], u[3], k__cospi_m16_p16);
  in[6] = idct_calc_wraplow_sse2(u[4], u[5], k__cospi_p16_p16);
  in[9] = idct_calc_wraplow_sse2(u[4], u[5], k__cospi_m16_p16);
  in[5] = idct_calc_wraplow_sse2(u[6], u[7], k__cospi_m16_m16);
  in[10] = idct_calc_wraplow_sse2(u[6], u[7], k__cospi_p16_m16);

  in[0] = s[0];
  in[1] = _mm_sub_epi16(kZero, s[8]);
  in[2] = s[12];
  in[3] = _mm_sub_epi16(kZero, s[4]);
  in[12] = s[5];
  in[13] = _mm_sub_epi16(kZero, s[13]);
  in[14] = s[9];
  in[15] = _mm_sub_epi16(kZero, s[1]);
}

static void idct16_8col(__m128i *in) {
  const __m128i k__cospi_p30_m02 = pair_set_epi16(cospi_30_64, -cospi_2_64);
  const __m128i k__cospi_p02_p30 = pair_set_epi16(cospi_2_64, cospi_30_64);
  const __m128i k__cospi_p14_m18 = pair_set_epi16(cospi_14_64, -cospi_18_64);
  const __m128i k__cospi_p18_p14 = pair_set_epi16(cospi_18_64, cospi_14_64);
  const __m128i k__cospi_p22_m10 = pair_set_epi16(cospi_22_64, -cospi_10_64);
  const __m128i k__cospi_p10_p22 = pair_set_epi16(cospi_10_64, cospi_22_64);
  const __m128i k__cospi_p06_m26 = pair_set_epi16(cospi_6_64, -cospi_26_64);
  const __m128i k__cospi_p26_p06 = pair_set_epi16(cospi_26_64, cospi_6_64);
  const __m128i k__cospi_p28_m04 = pair_set_epi16(cospi_28_64, -cospi_4_64);
  const __m128i k__cospi_p04_p28 = pair_set_epi16(cospi_4_64, cospi_28_64);
  const __m128i k__cospi_p12_m20 = pair_set_epi16(cospi_12_64, -cospi_20_64);
  const __m128i k__cospi_p20_p12 = pair_set_epi16(cospi_20_64, cospi_12_64);
  const __m128i k__cospi_p16_p16 = _mm_set1_epi16((int16_t)cospi_16_64);
  const __m128i k__cospi_p16_m16 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i k__cospi_p24_m08 = pair_set_epi16(cospi_24_64, -cospi_8_64);
  const __m128i k__cospi_p08_p24 = pair_set_epi16(cospi_8_64, cospi_24_64);
  const __m128i k__cospi_m08_p24 = pair_set_epi16(-cospi_8_64, cospi_24_64);
  const __m128i k__cospi_p24_p08 = pair_set_epi16(cospi_24_64, cospi_8_64);
  const __m128i k__cospi_m24_m08 = pair_set_epi16(-cospi_24_64, -cospi_8_64);
  const __m128i k__cospi_m16_p16 = pair_set_epi16(-cospi_16_64, cospi_16_64);
  __m128i u[16], s[16], t[16];

  // stage 1
  s[0] = in[0];
  s[1] = in[8];
  s[2] = in[4];
  s[3] = in[12];
  s[4] = in[2];
  s[5] = in[10];
  s[6] = in[6];
  s[7] = in[14];
  s[8] = in[1];
  s[9] = in[9];
  s[10] = in[5];
  s[11] = in[13];
  s[12] = in[3];
  s[13] = in[11];
  s[14] = in[7];
  s[15] = in[15];

  // stage 2
  u[0] = _mm_unpacklo_epi16(s[8], s[15]);
  u[1] = _mm_unpackhi_epi16(s[8], s[15]);
  u[2] = _mm_unpacklo_epi16(s[9], s[14]);
  u[3] = _mm_unpackhi_epi16(s[9], s[14]);
  u[4] = _mm_unpacklo_epi16(s[10], s[13]);
  u[5] = _mm_unpackhi_epi16(s[10], s[13]);
  u[6] = _mm_unpacklo_epi16(s[11], s[12]);
  u[7] = _mm_unpackhi_epi16(s[11], s[12]);

  s[8] = idct_calc_wraplow_sse2(u[0], u[1], k__cospi_p30_m02);
  s[15] = idct_calc_wraplow_sse2(u[0], u[1], k__cospi_p02_p30);
  s[9] = idct_calc_wraplow_sse2(u[2], u[3], k__cospi_p14_m18);
  s[14] = idct_calc_wraplow_sse2(u[2], u[3], k__cospi_p18_p14);
  s[10] = idct_calc_wraplow_sse2(u[4], u[5], k__cospi_p22_m10);
  s[13] = idct_calc_wraplow_sse2(u[4], u[5], k__cospi_p10_p22);
  s[11] = idct_calc_wraplow_sse2(u[6], u[7], k__cospi_p06_m26);
  s[12] = idct_calc_wraplow_sse2(u[6], u[7], k__cospi_p26_p06);

  // stage 3
  t[0] = s[0];
  t[1] = s[1];
  t[2] = s[2];
  t[3] = s[3];
  u[0] = _mm_unpacklo_epi16(s[4], s[7]);
  u[1] = _mm_unpackhi_epi16(s[4], s[7]);
  u[2] = _mm_unpacklo_epi16(s[5], s[6]);
  u[3] = _mm_unpackhi_epi16(s[5], s[6]);

  t[4] = idct_calc_wraplow_sse2(u[0], u[1], k__cospi_p28_m04);
  t[7] = idct_calc_wraplow_sse2(u[0], u[1], k__cospi_p04_p28);
  t[5] = idct_calc_wraplow_sse2(u[2], u[3], k__cospi_p12_m20);
  t[6] = idct_calc_wraplow_sse2(u[2], u[3], k__cospi_p20_p12);
  t[8] = _mm_add_epi16(s[8], s[9]);
  t[9] = _mm_sub_epi16(s[8], s[9]);
  t[10] = _mm_sub_epi16(s[11], s[10]);
  t[11] = _mm_add_epi16(s[10], s[11]);
  t[12] = _mm_add_epi16(s[12], s[13]);
  t[13] = _mm_sub_epi16(s[12], s[13]);
  t[14] = _mm_sub_epi16(s[15], s[14]);
  t[15] = _mm_add_epi16(s[14], s[15]);

  // stage 4
  u[0] = _mm_unpacklo_epi16(t[0], t[1]);
  u[1] = _mm_unpackhi_epi16(t[0], t[1]);
  u[2] = _mm_unpacklo_epi16(t[2], t[3]);
  u[3] = _mm_unpackhi_epi16(t[2], t[3]);
  u[4] = _mm_unpacklo_epi16(t[9], t[14]);
  u[5] = _mm_unpackhi_epi16(t[9], t[14]);
  u[6] = _mm_unpacklo_epi16(t[10], t[13]);
  u[7] = _mm_unpackhi_epi16(t[10], t[13]);

  s[0] = idct_calc_wraplow_sse2(u[0], u[1], k__cospi_p16_p16);
  s[1] = idct_calc_wraplow_sse2(u[0], u[1], k__cospi_p16_m16);
  s[2] = idct_calc_wraplow_sse2(u[2], u[3], k__cospi_p24_m08);
  s[3] = idct_calc_wraplow_sse2(u[2], u[3], k__cospi_p08_p24);
  s[9] = idct_calc_wraplow_sse2(u[4], u[5], k__cospi_m08_p24);
  s[14] = idct_calc_wraplow_sse2(u[4], u[5], k__cospi_p24_p08);
  s[10] = idct_calc_wraplow_sse2(u[6], u[7], k__cospi_m24_m08);
  s[13] = idct_calc_wraplow_sse2(u[6], u[7], k__cospi_m08_p24);
  s[4] = _mm_add_epi16(t[4], t[5]);
  s[5] = _mm_sub_epi16(t[4], t[5]);
  s[6] = _mm_sub_epi16(t[7], t[6]);
  s[7] = _mm_add_epi16(t[6], t[7]);
  s[8] = t[8];
  s[15] = t[15];
  s[11] = t[11];
  s[12] = t[12];

  // stage 5
  t[0] = _mm_add_epi16(s[0], s[3]);
  t[1] = _mm_add_epi16(s[1], s[2]);
  t[2] = _mm_sub_epi16(s[1], s[2]);
  t[3] = _mm_sub_epi16(s[0], s[3]);
  t[4] = s[4];
  t[7] = s[7];

  u[0] = _mm_unpacklo_epi16(s[5], s[6]);
  u[1] = _mm_unpackhi_epi16(s[5], s[6]);
  t[5] = idct_calc_wraplow_sse2(u[0], u[1], k__cospi_m16_p16);
  t[6] = idct_calc_wraplow_sse2(u[0], u[1], k__cospi_p16_p16);

  t[8] = _mm_add_epi16(s[8], s[11]);
  t[9] = _mm_add_epi16(s[9], s[10]);
  t[10] = _mm_sub_epi16(s[9], s[10]);
  t[11] = _mm_sub_epi16(s[8], s[11]);
  t[12] = _mm_sub_epi16(s[15], s[12]);
  t[13] = _mm_sub_epi16(s[14], s[13]);
  t[14] = _mm_add_epi16(s[13], s[14]);
  t[15] = _mm_add_epi16(s[12], s[15]);

  // stage 6
  s[0] = _mm_add_epi16(t[0], t[7]);
  s[1] = _mm_add_epi16(t[1], t[6]);
  s[2] = _mm_add_epi16(t[2], t[5]);
  s[3] = _mm_add_epi16(t[3], t[4]);
  s[4] = _mm_sub_epi16(t[3], t[4]);
  s[5] = _mm_sub_epi16(t[2], t[5]);
  s[6] = _mm_sub_epi16(t[1], t[6]);
  s[7] = _mm_sub_epi16(t[0], t[7]);
  s[8] = t[8];
  s[9] = t[9];

  u[0] = _mm_unpacklo_epi16(t[10], t[13]);
  u[1] = _mm_unpackhi_epi16(t[10], t[13]);
  u[2] = _mm_unpacklo_epi16(t[11], t[12]);
  u[3] = _mm_unpackhi_epi16(t[11], t[12]);

  s[10] = idct_calc_wraplow_sse2(u[0], u[1], k__cospi_m16_p16);
  s[13] = idct_calc_wraplow_sse2(u[0], u[1], k__cospi_p16_p16);
  s[11] = idct_calc_wraplow_sse2(u[2], u[3], k__cospi_m16_p16);
  s[12] = idct_calc_wraplow_sse2(u[2], u[3], k__cospi_p16_p16);
  s[14] = t[14];
  s[15] = t[15];

  // stage 7
  in[0] = _mm_add_epi16(s[0], s[15]);
  in[1] = _mm_add_epi16(s[1], s[14]);
  in[2] = _mm_add_epi16(s[2], s[13]);
  in[3] = _mm_add_epi16(s[3], s[12]);
  in[4] = _mm_add_epi16(s[4], s[11]);
  in[5] = _mm_add_epi16(s[5], s[10]);
  in[6] = _mm_add_epi16(s[6], s[9]);
  in[7] = _mm_add_epi16(s[7], s[8]);
  in[8] = _mm_sub_epi16(s[7], s[8]);
  in[9] = _mm_sub_epi16(s[6], s[9]);
  in[10] = _mm_sub_epi16(s[5], s[10]);
  in[11] = _mm_sub_epi16(s[4], s[11]);
  in[12] = _mm_sub_epi16(s[3], s[12]);
  in[13] = _mm_sub_epi16(s[2], s[13]);
  in[14] = _mm_sub_epi16(s[1], s[14]);
  in[15] = _mm_sub_epi16(s[0], s[15]);
}

void idct16_sse2(__m128i *in0, __m128i *in1) {
  array_transpose_16x16(in0, in1);
  idct16_8col(in0);
  idct16_8col(in1);
}

void iadst16_sse2(__m128i *in0, __m128i *in1) {
  array_transpose_16x16(in0, in1);
  iadst16_8col(in0);
  iadst16_8col(in1);
}

void vpx_idct16x16_10_add_sse2(const tran_low_t *input, uint8_t *dest,
                               int stride) {
  const __m128i final_rounding = _mm_set1_epi16(1 << 5);
  const __m128i zero = _mm_setzero_si128();

  const __m128i stg2_0 = pair_set_epi16(cospi_30_64, -cospi_2_64);
  const __m128i stg2_1 = pair_set_epi16(cospi_2_64, cospi_30_64);
  const __m128i stg2_6 = pair_set_epi16(cospi_6_64, -cospi_26_64);
  const __m128i stg2_7 = pair_set_epi16(cospi_26_64, cospi_6_64);

  const __m128i stg3_0 = pair_set_epi16(cospi_28_64, -cospi_4_64);
  const __m128i stg3_1 = pair_set_epi16(cospi_4_64, cospi_28_64);

  const __m128i stg4_0 = pair_set_epi16(cospi_16_64, cospi_16_64);
  const __m128i stg4_1 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i stg4_4 = pair_set_epi16(-cospi_8_64, cospi_24_64);
  const __m128i stg4_5 = pair_set_epi16(cospi_24_64, cospi_8_64);
  const __m128i stg4_6 = pair_set_epi16(-cospi_24_64, -cospi_8_64);
  const __m128i stg4_7 = pair_set_epi16(-cospi_8_64, cospi_24_64);

  const __m128i stg6_0 = pair_set_epi16(-cospi_16_64, cospi_16_64);
  __m128i in[16], l[16];
  __m128i stp1_0, stp1_1, stp1_2, stp1_3, stp1_4, stp1_5, stp1_6, stp1_8,
      stp1_9, stp1_10, stp1_11, stp1_12, stp1_13, stp1_14, stp1_15, stp1_8_0,
      stp1_12_0;
  __m128i stp2_0, stp2_1, stp2_2, stp2_3, stp2_4, stp2_5, stp2_6, stp2_7,
      stp2_8, stp2_9, stp2_10, stp2_11, stp2_12, stp2_13, stp2_14;
  __m128i tmp0, tmp1, tmp2, tmp3;
  int i;
  // First 1-D inverse DCT
  // Load input data.
  in[0] = load_input_data(input);
  in[1] = load_input_data(input + 8 * 2);
  in[2] = load_input_data(input + 8 * 4);
  in[3] = load_input_data(input + 8 * 6);

  TRANSPOSE_8X4(in[0], in[1], in[2], in[3], in[0], in[1]);

  // Stage2
  {
    const __m128i lo_1_15 = _mm_unpackhi_epi16(in[0], zero);
    const __m128i lo_13_3 = _mm_unpackhi_epi16(zero, in[1]);

    stp2_8 = idct_calc_wraplow_sse2(stg2_0, stg2_1, lo_1_15);
    stp2_11 = idct_calc_wraplow_sse2(stg2_6, stg2_7, lo_13_3);
  }

  // Stage3
  {
    const __m128i lo_2_14 = _mm_unpacklo_epi16(in[1], zero);

    stp1_4 = idct_calc_wraplow_sse2(stg3_0, stg3_1, lo_2_14);
    stp1_13 = _mm_unpackhi_epi64(stp2_11, zero);
    stp1_14 = _mm_unpackhi_epi64(stp2_8, zero);
  }

  // Stage4
  {
    const __m128i lo_0_8 = _mm_unpacklo_epi16(in[0], zero);
    const __m128i lo_9_14 = _mm_unpacklo_epi16(stp2_8, stp1_14);
    const __m128i lo_10_13 = _mm_unpacklo_epi16(stp2_11, stp1_13);

    tmp0 = idct_madd_round_shift_sse2(lo_0_8, stg4_0);
    tmp1 = idct_madd_round_shift_sse2(lo_0_8, stg4_1);
    stp1_0 = _mm_packs_epi32(tmp0, tmp0);
    stp1_1 = _mm_packs_epi32(tmp1, tmp1);
    stp2_9 = idct_calc_wraplow_sse2(stg4_4, stg4_5, lo_9_14);
    stp2_10 = idct_calc_wraplow_sse2(stg4_6, stg4_7, lo_10_13);

    stp2_6 = _mm_unpackhi_epi64(stp1_4, zero);
  }

  // Stage5 and Stage6
  {
    tmp0 = _mm_add_epi16(stp2_8, stp2_11);
    tmp1 = _mm_sub_epi16(stp2_8, stp2_11);
    tmp2 = _mm_add_epi16(stp2_9, stp2_10);
    tmp3 = _mm_sub_epi16(stp2_9, stp2_10);

    stp1_9 = _mm_unpacklo_epi64(tmp2, zero);
    stp1_10 = _mm_unpacklo_epi64(tmp3, zero);
    stp1_8 = _mm_unpacklo_epi64(tmp0, zero);
    stp1_11 = _mm_unpacklo_epi64(tmp1, zero);

    stp1_13 = _mm_unpackhi_epi64(tmp3, zero);
    stp1_14 = _mm_unpackhi_epi64(tmp2, zero);
    stp1_12 = _mm_unpackhi_epi64(tmp1, zero);
    stp1_15 = _mm_unpackhi_epi64(tmp0, zero);
  }

  // Stage6
  {
    const __m128i lo_6_5 = _mm_unpacklo_epi16(stp2_6, stp1_4);
    const __m128i lo_10_13 = _mm_unpacklo_epi16(stp1_10, stp1_13);
    const __m128i lo_11_12 = _mm_unpacklo_epi16(stp1_11, stp1_12);

    stp1_6 = idct_calc_wraplow_sse2(stg4_0, stg4_1, lo_6_5);
    tmp0 = idct_madd_round_shift_sse2(lo_10_13, stg6_0);
    tmp1 = idct_madd_round_shift_sse2(lo_10_13, stg4_0);
    tmp2 = idct_madd_round_shift_sse2(lo_11_12, stg6_0);
    tmp3 = idct_madd_round_shift_sse2(lo_11_12, stg4_0);

    stp2_10 = _mm_packs_epi32(tmp0, zero);
    stp2_13 = _mm_packs_epi32(tmp1, zero);
    stp2_11 = _mm_packs_epi32(tmp2, zero);
    stp2_12 = _mm_packs_epi32(tmp3, zero);

    tmp0 = _mm_add_epi16(stp1_0, stp1_4);
    tmp1 = _mm_sub_epi16(stp1_0, stp1_4);
    tmp2 = _mm_add_epi16(stp1_1, stp1_6);
    tmp3 = _mm_sub_epi16(stp1_1, stp1_6);

    stp2_0 = _mm_unpackhi_epi64(tmp0, zero);
    stp2_1 = _mm_unpacklo_epi64(tmp2, zero);
    stp2_2 = _mm_unpackhi_epi64(tmp2, zero);
    stp2_3 = _mm_unpacklo_epi64(tmp0, zero);
    stp2_4 = _mm_unpacklo_epi64(tmp1, zero);
    stp2_5 = _mm_unpackhi_epi64(tmp3, zero);
    stp2_6 = _mm_unpacklo_epi64(tmp3, zero);
    stp2_7 = _mm_unpackhi_epi64(tmp1, zero);
  }

  // Stage7. Left 8x16 only.
  l[0] = _mm_add_epi16(stp2_0, stp1_15);
  l[1] = _mm_add_epi16(stp2_1, stp1_14);
  l[2] = _mm_add_epi16(stp2_2, stp2_13);
  l[3] = _mm_add_epi16(stp2_3, stp2_12);
  l[4] = _mm_add_epi16(stp2_4, stp2_11);
  l[5] = _mm_add_epi16(stp2_5, stp2_10);
  l[6] = _mm_add_epi16(stp2_6, stp1_9);
  l[7] = _mm_add_epi16(stp2_7, stp1_8);
  l[8] = _mm_sub_epi16(stp2_7, stp1_8);
  l[9] = _mm_sub_epi16(stp2_6, stp1_9);
  l[10] = _mm_sub_epi16(stp2_5, stp2_10);
  l[11] = _mm_sub_epi16(stp2_4, stp2_11);
  l[12] = _mm_sub_epi16(stp2_3, stp2_12);
  l[13] = _mm_sub_epi16(stp2_2, stp2_13);
  l[14] = _mm_sub_epi16(stp2_1, stp1_14);
  l[15] = _mm_sub_epi16(stp2_0, stp1_15);

  // Second 1-D inverse transform, performed per 8x16 block
  for (i = 0; i < 2; i++) {
    int j;
    array_transpose_4X8(l + 8 * i, in);

    IDCT16_10

    // Stage7
    in[0] = _mm_add_epi16(stp2_0, stp1_15);
    in[1] = _mm_add_epi16(stp2_1, stp1_14);
    in[2] = _mm_add_epi16(stp2_2, stp2_13);
    in[3] = _mm_add_epi16(stp2_3, stp2_12);
    in[4] = _mm_add_epi16(stp2_4, stp2_11);
    in[5] = _mm_add_epi16(stp2_5, stp2_10);
    in[6] = _mm_add_epi16(stp2_6, stp1_9);
    in[7] = _mm_add_epi16(stp2_7, stp1_8);
    in[8] = _mm_sub_epi16(stp2_7, stp1_8);
    in[9] = _mm_sub_epi16(stp2_6, stp1_9);
    in[10] = _mm_sub_epi16(stp2_5, stp2_10);
    in[11] = _mm_sub_epi16(stp2_4, stp2_11);
    in[12] = _mm_sub_epi16(stp2_3, stp2_12);
    in[13] = _mm_sub_epi16(stp2_2, stp2_13);
    in[14] = _mm_sub_epi16(stp2_1, stp1_14);
    in[15] = _mm_sub_epi16(stp2_0, stp1_15);

    for (j = 0; j < 16; ++j) {
      // Final rounding and shift
      in[j] = _mm_adds_epi16(in[j], final_rounding);
      in[j] = _mm_srai_epi16(in[j], 6);
      RECON_AND_STORE(dest + j * stride, in[j]);
    }

    dest += 8;
  }
}

#define LOAD_DQCOEFF(reg, input)  \
  {                               \
    reg = load_input_data(input); \
    input += 8;                   \
  }

#define IDCT32_34                                                              \
  /* Stage1 */                                                                 \
  {                                                                            \
    const __m128i lo_1_31 = _mm_unpacklo_epi16(in[1], zero);                   \
    const __m128i hi_1_31 = _mm_unpackhi_epi16(in[1], zero);                   \
                                                                               \
    const __m128i lo_25_7 = _mm_unpacklo_epi16(zero, in[7]);                   \
    const __m128i hi_25_7 = _mm_unpackhi_epi16(zero, in[7]);                   \
                                                                               \
    const __m128i lo_5_27 = _mm_unpacklo_epi16(in[5], zero);                   \
    const __m128i hi_5_27 = _mm_unpackhi_epi16(in[5], zero);                   \
                                                                               \
    const __m128i lo_29_3 = _mm_unpacklo_epi16(zero, in[3]);                   \
    const __m128i hi_29_3 = _mm_unpackhi_epi16(zero, in[3]);                   \
                                                                               \
    MULTIPLICATION_AND_ADD_2(lo_1_31, hi_1_31, stg1_0, stg1_1, stp1_16,        \
                             stp1_31);                                         \
    MULTIPLICATION_AND_ADD_2(lo_25_7, hi_25_7, stg1_6, stg1_7, stp1_19,        \
                             stp1_28);                                         \
    MULTIPLICATION_AND_ADD_2(lo_5_27, hi_5_27, stg1_8, stg1_9, stp1_20,        \
                             stp1_27);                                         \
    MULTIPLICATION_AND_ADD_2(lo_29_3, hi_29_3, stg1_14, stg1_15, stp1_23,      \
                             stp1_24);                                         \
  }                                                                            \
                                                                               \
  /* Stage2 */                                                                 \
  {                                                                            \
    const __m128i lo_2_30 = _mm_unpacklo_epi16(in[2], zero);                   \
    const __m128i hi_2_30 = _mm_unpackhi_epi16(in[2], zero);                   \
                                                                               \
    const __m128i lo_26_6 = _mm_unpacklo_epi16(zero, in[6]);                   \
    const __m128i hi_26_6 = _mm_unpackhi_epi16(zero, in[6]);                   \
                                                                               \
    MULTIPLICATION_AND_ADD_2(lo_2_30, hi_2_30, stg2_0, stg2_1, stp2_8,         \
                             stp2_15);                                         \
    MULTIPLICATION_AND_ADD_2(lo_26_6, hi_26_6, stg2_6, stg2_7, stp2_11,        \
                             stp2_12);                                         \
                                                                               \
    stp2_16 = stp1_16;                                                         \
    stp2_19 = stp1_19;                                                         \
                                                                               \
    stp2_20 = stp1_20;                                                         \
    stp2_23 = stp1_23;                                                         \
                                                                               \
    stp2_24 = stp1_24;                                                         \
    stp2_27 = stp1_27;                                                         \
                                                                               \
    stp2_28 = stp1_28;                                                         \
    stp2_31 = stp1_31;                                                         \
  }                                                                            \
                                                                               \
  /* Stage3 */                                                                 \
  {                                                                            \
    const __m128i lo_4_28 = _mm_unpacklo_epi16(in[4], zero);                   \
    const __m128i hi_4_28 = _mm_unpackhi_epi16(in[4], zero);                   \
                                                                               \
    const __m128i lo_17_30 = _mm_unpacklo_epi16(stp1_16, stp1_31);             \
    const __m128i hi_17_30 = _mm_unpackhi_epi16(stp1_16, stp1_31);             \
    const __m128i lo_18_29 = _mm_unpacklo_epi16(stp1_19, stp1_28);             \
    const __m128i hi_18_29 = _mm_unpackhi_epi16(stp1_19, stp1_28);             \
                                                                               \
    const __m128i lo_21_26 = _mm_unpacklo_epi16(stp1_20, stp1_27);             \
    const __m128i hi_21_26 = _mm_unpackhi_epi16(stp1_20, stp1_27);             \
    const __m128i lo_22_25 = _mm_unpacklo_epi16(stp1_23, stp1_24);             \
    const __m128i hi_22_25 = _mm_unpackhi_epi16(stp1_23, stp2_24);             \
                                                                               \
    MULTIPLICATION_AND_ADD_2(lo_4_28, hi_4_28, stg3_0, stg3_1, stp1_4,         \
                             stp1_7);                                          \
                                                                               \
    stp1_8 = stp2_8;                                                           \
    stp1_11 = stp2_11;                                                         \
    stp1_12 = stp2_12;                                                         \
    stp1_15 = stp2_15;                                                         \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_17_30, hi_17_30, lo_18_29, hi_18_29, stg3_4,     \
                           stg3_5, stg3_6, stg3_4, stp1_17, stp1_30, stp1_18,  \
                           stp1_29)                                            \
    MULTIPLICATION_AND_ADD(lo_21_26, hi_21_26, lo_22_25, hi_22_25, stg3_8,     \
                           stg3_9, stg3_10, stg3_8, stp1_21, stp1_26, stp1_22, \
                           stp1_25)                                            \
                                                                               \
    stp1_16 = stp2_16;                                                         \
    stp1_31 = stp2_31;                                                         \
    stp1_19 = stp2_19;                                                         \
    stp1_20 = stp2_20;                                                         \
    stp1_23 = stp2_23;                                                         \
    stp1_24 = stp2_24;                                                         \
    stp1_27 = stp2_27;                                                         \
    stp1_28 = stp2_28;                                                         \
  }                                                                            \
                                                                               \
  /* Stage4 */                                                                 \
  {                                                                            \
    const __m128i lo_0_16 = _mm_unpacklo_epi16(in[0], zero);                   \
    const __m128i hi_0_16 = _mm_unpackhi_epi16(in[0], zero);                   \
                                                                               \
    const __m128i lo_9_14 = _mm_unpacklo_epi16(stp2_8, stp2_15);               \
    const __m128i hi_9_14 = _mm_unpackhi_epi16(stp2_8, stp2_15);               \
    const __m128i lo_10_13 = _mm_unpacklo_epi16(stp2_11, stp2_12);             \
    const __m128i hi_10_13 = _mm_unpackhi_epi16(stp2_11, stp2_12);             \
                                                                               \
    MULTIPLICATION_AND_ADD_2(lo_0_16, hi_0_16, stg4_0, stg4_1, stp2_0,         \
                             stp2_1);                                          \
                                                                               \
    stp2_4 = stp1_4;                                                           \
    stp2_5 = stp1_4;                                                           \
    stp2_6 = stp1_7;                                                           \
    stp2_7 = stp1_7;                                                           \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_9_14, hi_9_14, lo_10_13, hi_10_13, stg4_4,       \
                           stg4_5, stg4_6, stg4_4, stp2_9, stp2_14, stp2_10,   \
                           stp2_13)                                            \
                                                                               \
    stp2_8 = stp1_8;                                                           \
    stp2_15 = stp1_15;                                                         \
    stp2_11 = stp1_11;                                                         \
    stp2_12 = stp1_12;                                                         \
                                                                               \
    stp2_16 = _mm_add_epi16(stp1_16, stp1_19);                                 \
    stp2_17 = _mm_add_epi16(stp1_17, stp1_18);                                 \
    stp2_18 = _mm_sub_epi16(stp1_17, stp1_18);                                 \
    stp2_19 = _mm_sub_epi16(stp1_16, stp1_19);                                 \
    stp2_20 = _mm_sub_epi16(stp1_23, stp1_20);                                 \
    stp2_21 = _mm_sub_epi16(stp1_22, stp1_21);                                 \
    stp2_22 = _mm_add_epi16(stp1_22, stp1_21);                                 \
    stp2_23 = _mm_add_epi16(stp1_23, stp1_20);                                 \
                                                                               \
    stp2_24 = _mm_add_epi16(stp1_24, stp1_27);                                 \
    stp2_25 = _mm_add_epi16(stp1_25, stp1_26);                                 \
    stp2_26 = _mm_sub_epi16(stp1_25, stp1_26);                                 \
    stp2_27 = _mm_sub_epi16(stp1_24, stp1_27);                                 \
    stp2_28 = _mm_sub_epi16(stp1_31, stp1_28);                                 \
    stp2_29 = _mm_sub_epi16(stp1_30, stp1_29);                                 \
    stp2_30 = _mm_add_epi16(stp1_29, stp1_30);                                 \
    stp2_31 = _mm_add_epi16(stp1_28, stp1_31);                                 \
  }                                                                            \
                                                                               \
  /* Stage5 */                                                                 \
  {                                                                            \
    const __m128i lo_6_5 = _mm_unpacklo_epi16(stp2_6, stp2_5);                 \
    const __m128i hi_6_5 = _mm_unpackhi_epi16(stp2_6, stp2_5);                 \
    const __m128i lo_18_29 = _mm_unpacklo_epi16(stp2_18, stp2_29);             \
    const __m128i hi_18_29 = _mm_unpackhi_epi16(stp2_18, stp2_29);             \
                                                                               \
    const __m128i lo_19_28 = _mm_unpacklo_epi16(stp2_19, stp2_28);             \
    const __m128i hi_19_28 = _mm_unpackhi_epi16(stp2_19, stp2_28);             \
    const __m128i lo_20_27 = _mm_unpacklo_epi16(stp2_20, stp2_27);             \
    const __m128i hi_20_27 = _mm_unpackhi_epi16(stp2_20, stp2_27);             \
                                                                               \
    const __m128i lo_21_26 = _mm_unpacklo_epi16(stp2_21, stp2_26);             \
    const __m128i hi_21_26 = _mm_unpackhi_epi16(stp2_21, stp2_26);             \
                                                                               \
    stp1_0 = stp2_0;                                                           \
    stp1_1 = stp2_1;                                                           \
    stp1_2 = stp2_1;                                                           \
    stp1_3 = stp2_0;                                                           \
                                                                               \
    stp1_5 = idct_calc_wraplow_sse2(lo_6_5, hi_6_5, stg4_1);                   \
    stp1_6 = idct_calc_wraplow_sse2(lo_6_5, hi_6_5, stg4_0);                   \
                                                                               \
    stp1_4 = stp2_4;                                                           \
    stp1_7 = stp2_7;                                                           \
                                                                               \
    stp1_8 = _mm_add_epi16(stp2_8, stp2_11);                                   \
    stp1_9 = _mm_add_epi16(stp2_9, stp2_10);                                   \
    stp1_10 = _mm_sub_epi16(stp2_9, stp2_10);                                  \
    stp1_11 = _mm_sub_epi16(stp2_8, stp2_11);                                  \
    stp1_12 = _mm_sub_epi16(stp2_15, stp2_12);                                 \
    stp1_13 = _mm_sub_epi16(stp2_14, stp2_13);                                 \
    stp1_14 = _mm_add_epi16(stp2_14, stp2_13);                                 \
    stp1_15 = _mm_add_epi16(stp2_15, stp2_12);                                 \
                                                                               \
    stp1_16 = stp2_16;                                                         \
    stp1_17 = stp2_17;                                                         \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_18_29, hi_18_29, lo_19_28, hi_19_28, stg4_4,     \
                           stg4_5, stg4_4, stg4_5, stp1_18, stp1_29, stp1_19,  \
                           stp1_28)                                            \
    MULTIPLICATION_AND_ADD(lo_20_27, hi_20_27, lo_21_26, hi_21_26, stg4_6,     \
                           stg4_4, stg4_6, stg4_4, stp1_20, stp1_27, stp1_21,  \
                           stp1_26)                                            \
                                                                               \
    stp1_22 = stp2_22;                                                         \
    stp1_23 = stp2_23;                                                         \
    stp1_24 = stp2_24;                                                         \
    stp1_25 = stp2_25;                                                         \
    stp1_30 = stp2_30;                                                         \
    stp1_31 = stp2_31;                                                         \
  }                                                                            \
                                                                               \
  /* Stage6 */                                                                 \
  {                                                                            \
    const __m128i lo_10_13 = _mm_unpacklo_epi16(stp1_10, stp1_13);             \
    const __m128i hi_10_13 = _mm_unpackhi_epi16(stp1_10, stp1_13);             \
    const __m128i lo_11_12 = _mm_unpacklo_epi16(stp1_11, stp1_12);             \
    const __m128i hi_11_12 = _mm_unpackhi_epi16(stp1_11, stp1_12);             \
                                                                               \
    stp2_0 = _mm_add_epi16(stp1_0, stp1_7);                                    \
    stp2_1 = _mm_add_epi16(stp1_1, stp1_6);                                    \
    stp2_2 = _mm_add_epi16(stp1_2, stp1_5);                                    \
    stp2_3 = _mm_add_epi16(stp1_3, stp1_4);                                    \
    stp2_4 = _mm_sub_epi16(stp1_3, stp1_4);                                    \
    stp2_5 = _mm_sub_epi16(stp1_2, stp1_5);                                    \
    stp2_6 = _mm_sub_epi16(stp1_1, stp1_6);                                    \
    stp2_7 = _mm_sub_epi16(stp1_0, stp1_7);                                    \
                                                                               \
    stp2_8 = stp1_8;                                                           \
    stp2_9 = stp1_9;                                                           \
    stp2_14 = stp1_14;                                                         \
    stp2_15 = stp1_15;                                                         \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_10_13, hi_10_13, lo_11_12, hi_11_12, stg6_0,     \
                           stg4_0, stg6_0, stg4_0, stp2_10, stp2_13, stp2_11,  \
                           stp2_12)                                            \
                                                                               \
    stp2_16 = _mm_add_epi16(stp1_16, stp1_23);                                 \
    stp2_17 = _mm_add_epi16(stp1_17, stp1_22);                                 \
    stp2_18 = _mm_add_epi16(stp1_18, stp1_21);                                 \
    stp2_19 = _mm_add_epi16(stp1_19, stp1_20);                                 \
    stp2_20 = _mm_sub_epi16(stp1_19, stp1_20);                                 \
    stp2_21 = _mm_sub_epi16(stp1_18, stp1_21);                                 \
    stp2_22 = _mm_sub_epi16(stp1_17, stp1_22);                                 \
    stp2_23 = _mm_sub_epi16(stp1_16, stp1_23);                                 \
                                                                               \
    stp2_24 = _mm_sub_epi16(stp1_31, stp1_24);                                 \
    stp2_25 = _mm_sub_epi16(stp1_30, stp1_25);                                 \
    stp2_26 = _mm_sub_epi16(stp1_29, stp1_26);                                 \
    stp2_27 = _mm_sub_epi16(stp1_28, stp1_27);                                 \
    stp2_28 = _mm_add_epi16(stp1_27, stp1_28);                                 \
    stp2_29 = _mm_add_epi16(stp1_26, stp1_29);                                 \
    stp2_30 = _mm_add_epi16(stp1_25, stp1_30);                                 \
    stp2_31 = _mm_add_epi16(stp1_24, stp1_31);                                 \
  }                                                                            \
                                                                               \
  /* Stage7 */                                                                 \
  {                                                                            \
    const __m128i lo_20_27 = _mm_unpacklo_epi16(stp2_20, stp2_27);             \
    const __m128i hi_20_27 = _mm_unpackhi_epi16(stp2_20, stp2_27);             \
    const __m128i lo_21_26 = _mm_unpacklo_epi16(stp2_21, stp2_26);             \
    const __m128i hi_21_26 = _mm_unpackhi_epi16(stp2_21, stp2_26);             \
                                                                               \
    const __m128i lo_22_25 = _mm_unpacklo_epi16(stp2_22, stp2_25);             \
    const __m128i hi_22_25 = _mm_unpackhi_epi16(stp2_22, stp2_25);             \
    const __m128i lo_23_24 = _mm_unpacklo_epi16(stp2_23, stp2_24);             \
    const __m128i hi_23_24 = _mm_unpackhi_epi16(stp2_23, stp2_24);             \
                                                                               \
    stp1_0 = _mm_add_epi16(stp2_0, stp2_15);                                   \
    stp1_1 = _mm_add_epi16(stp2_1, stp2_14);                                   \
    stp1_2 = _mm_add_epi16(stp2_2, stp2_13);                                   \
    stp1_3 = _mm_add_epi16(stp2_3, stp2_12);                                   \
    stp1_4 = _mm_add_epi16(stp2_4, stp2_11);                                   \
    stp1_5 = _mm_add_epi16(stp2_5, stp2_10);                                   \
    stp1_6 = _mm_add_epi16(stp2_6, stp2_9);                                    \
    stp1_7 = _mm_add_epi16(stp2_7, stp2_8);                                    \
    stp1_8 = _mm_sub_epi16(stp2_7, stp2_8);                                    \
    stp1_9 = _mm_sub_epi16(stp2_6, stp2_9);                                    \
    stp1_10 = _mm_sub_epi16(stp2_5, stp2_10);                                  \
    stp1_11 = _mm_sub_epi16(stp2_4, stp2_11);                                  \
    stp1_12 = _mm_sub_epi16(stp2_3, stp2_12);                                  \
    stp1_13 = _mm_sub_epi16(stp2_2, stp2_13);                                  \
    stp1_14 = _mm_sub_epi16(stp2_1, stp2_14);                                  \
    stp1_15 = _mm_sub_epi16(stp2_0, stp2_15);                                  \
                                                                               \
    stp1_16 = stp2_16;                                                         \
    stp1_17 = stp2_17;                                                         \
    stp1_18 = stp2_18;                                                         \
    stp1_19 = stp2_19;                                                         \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_20_27, hi_20_27, lo_21_26, hi_21_26, stg6_0,     \
                           stg4_0, stg6_0, stg4_0, stp1_20, stp1_27, stp1_21,  \
                           stp1_26)                                            \
    MULTIPLICATION_AND_ADD(lo_22_25, hi_22_25, lo_23_24, hi_23_24, stg6_0,     \
                           stg4_0, stg6_0, stg4_0, stp1_22, stp1_25, stp1_23,  \
                           stp1_24)                                            \
                                                                               \
    stp1_28 = stp2_28;                                                         \
    stp1_29 = stp2_29;                                                         \
    stp1_30 = stp2_30;                                                         \
    stp1_31 = stp2_31;                                                         \
  }

#define IDCT32                                                                 \
  /* Stage1 */                                                                 \
  {                                                                            \
    const __m128i lo_1_31 = _mm_unpacklo_epi16(in[1], in[31]);                 \
    const __m128i hi_1_31 = _mm_unpackhi_epi16(in[1], in[31]);                 \
    const __m128i lo_17_15 = _mm_unpacklo_epi16(in[17], in[15]);               \
    const __m128i hi_17_15 = _mm_unpackhi_epi16(in[17], in[15]);               \
                                                                               \
    const __m128i lo_9_23 = _mm_unpacklo_epi16(in[9], in[23]);                 \
    const __m128i hi_9_23 = _mm_unpackhi_epi16(in[9], in[23]);                 \
    const __m128i lo_25_7 = _mm_unpacklo_epi16(in[25], in[7]);                 \
    const __m128i hi_25_7 = _mm_unpackhi_epi16(in[25], in[7]);                 \
                                                                               \
    const __m128i lo_5_27 = _mm_unpacklo_epi16(in[5], in[27]);                 \
    const __m128i hi_5_27 = _mm_unpackhi_epi16(in[5], in[27]);                 \
    const __m128i lo_21_11 = _mm_unpacklo_epi16(in[21], in[11]);               \
    const __m128i hi_21_11 = _mm_unpackhi_epi16(in[21], in[11]);               \
                                                                               \
    const __m128i lo_13_19 = _mm_unpacklo_epi16(in[13], in[19]);               \
    const __m128i hi_13_19 = _mm_unpackhi_epi16(in[13], in[19]);               \
    const __m128i lo_29_3 = _mm_unpacklo_epi16(in[29], in[3]);                 \
    const __m128i hi_29_3 = _mm_unpackhi_epi16(in[29], in[3]);                 \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_1_31, hi_1_31, lo_17_15, hi_17_15, stg1_0,       \
                           stg1_1, stg1_2, stg1_3, stp1_16, stp1_31, stp1_17,  \
                           stp1_30)                                            \
    MULTIPLICATION_AND_ADD(lo_9_23, hi_9_23, lo_25_7, hi_25_7, stg1_4, stg1_5, \
                           stg1_6, stg1_7, stp1_18, stp1_29, stp1_19, stp1_28) \
    MULTIPLICATION_AND_ADD(lo_5_27, hi_5_27, lo_21_11, hi_21_11, stg1_8,       \
                           stg1_9, stg1_10, stg1_11, stp1_20, stp1_27,         \
                           stp1_21, stp1_26)                                   \
    MULTIPLICATION_AND_ADD(lo_13_19, hi_13_19, lo_29_3, hi_29_3, stg1_12,      \
                           stg1_13, stg1_14, stg1_15, stp1_22, stp1_25,        \
                           stp1_23, stp1_24)                                   \
  }                                                                            \
                                                                               \
  /* Stage2 */                                                                 \
  {                                                                            \
    const __m128i lo_2_30 = _mm_unpacklo_epi16(in[2], in[30]);                 \
    const __m128i hi_2_30 = _mm_unpackhi_epi16(in[2], in[30]);                 \
    const __m128i lo_18_14 = _mm_unpacklo_epi16(in[18], in[14]);               \
    const __m128i hi_18_14 = _mm_unpackhi_epi16(in[18], in[14]);               \
                                                                               \
    const __m128i lo_10_22 = _mm_unpacklo_epi16(in[10], in[22]);               \
    const __m128i hi_10_22 = _mm_unpackhi_epi16(in[10], in[22]);               \
    const __m128i lo_26_6 = _mm_unpacklo_epi16(in[26], in[6]);                 \
    const __m128i hi_26_6 = _mm_unpackhi_epi16(in[26], in[6]);                 \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_2_30, hi_2_30, lo_18_14, hi_18_14, stg2_0,       \
                           stg2_1, stg2_2, stg2_3, stp2_8, stp2_15, stp2_9,    \
                           stp2_14)                                            \
    MULTIPLICATION_AND_ADD(lo_10_22, hi_10_22, lo_26_6, hi_26_6, stg2_4,       \
                           stg2_5, stg2_6, stg2_7, stp2_10, stp2_13, stp2_11,  \
                           stp2_12)                                            \
                                                                               \
    stp2_16 = _mm_add_epi16(stp1_16, stp1_17);                                 \
    stp2_17 = _mm_sub_epi16(stp1_16, stp1_17);                                 \
    stp2_18 = _mm_sub_epi16(stp1_19, stp1_18);                                 \
    stp2_19 = _mm_add_epi16(stp1_19, stp1_18);                                 \
                                                                               \
    stp2_20 = _mm_add_epi16(stp1_20, stp1_21);                                 \
    stp2_21 = _mm_sub_epi16(stp1_20, stp1_21);                                 \
    stp2_22 = _mm_sub_epi16(stp1_23, stp1_22);                                 \
    stp2_23 = _mm_add_epi16(stp1_23, stp1_22);                                 \
                                                                               \
    stp2_24 = _mm_add_epi16(stp1_24, stp1_25);                                 \
    stp2_25 = _mm_sub_epi16(stp1_24, stp1_25);                                 \
    stp2_26 = _mm_sub_epi16(stp1_27, stp1_26);                                 \
    stp2_27 = _mm_add_epi16(stp1_27, stp1_26);                                 \
                                                                               \
    stp2_28 = _mm_add_epi16(stp1_28, stp1_29);                                 \
    stp2_29 = _mm_sub_epi16(stp1_28, stp1_29);                                 \
    stp2_30 = _mm_sub_epi16(stp1_31, stp1_30);                                 \
    stp2_31 = _mm_add_epi16(stp1_31, stp1_30);                                 \
  }                                                                            \
                                                                               \
  /* Stage3 */                                                                 \
  {                                                                            \
    const __m128i lo_4_28 = _mm_unpacklo_epi16(in[4], in[28]);                 \
    const __m128i hi_4_28 = _mm_unpackhi_epi16(in[4], in[28]);                 \
    const __m128i lo_20_12 = _mm_unpacklo_epi16(in[20], in[12]);               \
    const __m128i hi_20_12 = _mm_unpackhi_epi16(in[20], in[12]);               \
                                                                               \
    const __m128i lo_17_30 = _mm_unpacklo_epi16(stp2_17, stp2_30);             \
    const __m128i hi_17_30 = _mm_unpackhi_epi16(stp2_17, stp2_30);             \
    const __m128i lo_18_29 = _mm_unpacklo_epi16(stp2_18, stp2_29);             \
    const __m128i hi_18_29 = _mm_unpackhi_epi16(stp2_18, stp2_29);             \
                                                                               \
    const __m128i lo_21_26 = _mm_unpacklo_epi16(stp2_21, stp2_26);             \
    const __m128i hi_21_26 = _mm_unpackhi_epi16(stp2_21, stp2_26);             \
    const __m128i lo_22_25 = _mm_unpacklo_epi16(stp2_22, stp2_25);             \
    const __m128i hi_22_25 = _mm_unpackhi_epi16(stp2_22, stp2_25);             \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_4_28, hi_4_28, lo_20_12, hi_20_12, stg3_0,       \
                           stg3_1, stg3_2, stg3_3, stp1_4, stp1_7, stp1_5,     \
                           stp1_6)                                             \
                                                                               \
    stp1_8 = _mm_add_epi16(stp2_8, stp2_9);                                    \
    stp1_9 = _mm_sub_epi16(stp2_8, stp2_9);                                    \
    stp1_10 = _mm_sub_epi16(stp2_11, stp2_10);                                 \
    stp1_11 = _mm_add_epi16(stp2_11, stp2_10);                                 \
    stp1_12 = _mm_add_epi16(stp2_12, stp2_13);                                 \
    stp1_13 = _mm_sub_epi16(stp2_12, stp2_13);                                 \
    stp1_14 = _mm_sub_epi16(stp2_15, stp2_14);                                 \
    stp1_15 = _mm_add_epi16(stp2_15, stp2_14);                                 \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_17_30, hi_17_30, lo_18_29, hi_18_29, stg3_4,     \
                           stg3_5, stg3_6, stg3_4, stp1_17, stp1_30, stp1_18,  \
                           stp1_29)                                            \
    MULTIPLICATION_AND_ADD(lo_21_26, hi_21_26, lo_22_25, hi_22_25, stg3_8,     \
                           stg3_9, stg3_10, stg3_8, stp1_21, stp1_26, stp1_22, \
                           stp1_25)                                            \
                                                                               \
    stp1_16 = stp2_16;                                                         \
    stp1_31 = stp2_31;                                                         \
    stp1_19 = stp2_19;                                                         \
    stp1_20 = stp2_20;                                                         \
    stp1_23 = stp2_23;                                                         \
    stp1_24 = stp2_24;                                                         \
    stp1_27 = stp2_27;                                                         \
    stp1_28 = stp2_28;                                                         \
  }                                                                            \
                                                                               \
  /* Stage4 */                                                                 \
  {                                                                            \
    const __m128i lo_0_16 = _mm_unpacklo_epi16(in[0], in[16]);                 \
    const __m128i hi_0_16 = _mm_unpackhi_epi16(in[0], in[16]);                 \
    const __m128i lo_8_24 = _mm_unpacklo_epi16(in[8], in[24]);                 \
    const __m128i hi_8_24 = _mm_unpackhi_epi16(in[8], in[24]);                 \
                                                                               \
    const __m128i lo_9_14 = _mm_unpacklo_epi16(stp1_9, stp1_14);               \
    const __m128i hi_9_14 = _mm_unpackhi_epi16(stp1_9, stp1_14);               \
    const __m128i lo_10_13 = _mm_unpacklo_epi16(stp1_10, stp1_13);             \
    const __m128i hi_10_13 = _mm_unpackhi_epi16(stp1_10, stp1_13);             \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_0_16, hi_0_16, lo_8_24, hi_8_24, stg4_0, stg4_1, \
                           stg4_2, stg4_3, stp2_0, stp2_1, stp2_2, stp2_3)     \
                                                                               \
    stp2_4 = _mm_add_epi16(stp1_4, stp1_5);                                    \
    stp2_5 = _mm_sub_epi16(stp1_4, stp1_5);                                    \
    stp2_6 = _mm_sub_epi16(stp1_7, stp1_6);                                    \
    stp2_7 = _mm_add_epi16(stp1_7, stp1_6);                                    \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_9_14, hi_9_14, lo_10_13, hi_10_13, stg4_4,       \
                           stg4_5, stg4_6, stg4_4, stp2_9, stp2_14, stp2_10,   \
                           stp2_13)                                            \
                                                                               \
    stp2_8 = stp1_8;                                                           \
    stp2_15 = stp1_15;                                                         \
    stp2_11 = stp1_11;                                                         \
    stp2_12 = stp1_12;                                                         \
                                                                               \
    stp2_16 = _mm_add_epi16(stp1_16, stp1_19);                                 \
    stp2_17 = _mm_add_epi16(stp1_17, stp1_18);                                 \
    stp2_18 = _mm_sub_epi16(stp1_17, stp1_18);                                 \
    stp2_19 = _mm_sub_epi16(stp1_16, stp1_19);                                 \
    stp2_20 = _mm_sub_epi16(stp1_23, stp1_20);                                 \
    stp2_21 = _mm_sub_epi16(stp1_22, stp1_21);                                 \
    stp2_22 = _mm_add_epi16(stp1_22, stp1_21);                                 \
    stp2_23 = _mm_add_epi16(stp1_23, stp1_20);                                 \
                                                                               \
    stp2_24 = _mm_add_epi16(stp1_24, stp1_27);                                 \
    stp2_25 = _mm_add_epi16(stp1_25, stp1_26);                                 \
    stp2_26 = _mm_sub_epi16(stp1_25, stp1_26);                                 \
    stp2_27 = _mm_sub_epi16(stp1_24, stp1_27);                                 \
    stp2_28 = _mm_sub_epi16(stp1_31, stp1_28);                                 \
    stp2_29 = _mm_sub_epi16(stp1_30, stp1_29);                                 \
    stp2_30 = _mm_add_epi16(stp1_29, stp1_30);                                 \
    stp2_31 = _mm_add_epi16(stp1_28, stp1_31);                                 \
  }                                                                            \
                                                                               \
  /* Stage5 */                                                                 \
  {                                                                            \
    const __m128i lo_6_5 = _mm_unpacklo_epi16(stp2_6, stp2_5);                 \
    const __m128i hi_6_5 = _mm_unpackhi_epi16(stp2_6, stp2_5);                 \
    const __m128i lo_18_29 = _mm_unpacklo_epi16(stp2_18, stp2_29);             \
    const __m128i hi_18_29 = _mm_unpackhi_epi16(stp2_18, stp2_29);             \
                                                                               \
    const __m128i lo_19_28 = _mm_unpacklo_epi16(stp2_19, stp2_28);             \
    const __m128i hi_19_28 = _mm_unpackhi_epi16(stp2_19, stp2_28);             \
    const __m128i lo_20_27 = _mm_unpacklo_epi16(stp2_20, stp2_27);             \
    const __m128i hi_20_27 = _mm_unpackhi_epi16(stp2_20, stp2_27);             \
                                                                               \
    const __m128i lo_21_26 = _mm_unpacklo_epi16(stp2_21, stp2_26);             \
    const __m128i hi_21_26 = _mm_unpackhi_epi16(stp2_21, stp2_26);             \
                                                                               \
    stp1_0 = _mm_add_epi16(stp2_0, stp2_3);                                    \
    stp1_1 = _mm_add_epi16(stp2_1, stp2_2);                                    \
    stp1_2 = _mm_sub_epi16(stp2_1, stp2_2);                                    \
    stp1_3 = _mm_sub_epi16(stp2_0, stp2_3);                                    \
                                                                               \
    stp1_5 = idct_calc_wraplow_sse2(lo_6_5, hi_6_5, stg4_1);                   \
    stp1_6 = idct_calc_wraplow_sse2(lo_6_5, hi_6_5, stg4_0);                   \
                                                                               \
    stp1_4 = stp2_4;                                                           \
    stp1_7 = stp2_7;                                                           \
                                                                               \
    stp1_8 = _mm_add_epi16(stp2_8, stp2_11);                                   \
    stp1_9 = _mm_add_epi16(stp2_9, stp2_10);                                   \
    stp1_10 = _mm_sub_epi16(stp2_9, stp2_10);                                  \
    stp1_11 = _mm_sub_epi16(stp2_8, stp2_11);                                  \
    stp1_12 = _mm_sub_epi16(stp2_15, stp2_12);                                 \
    stp1_13 = _mm_sub_epi16(stp2_14, stp2_13);                                 \
    stp1_14 = _mm_add_epi16(stp2_14, stp2_13);                                 \
    stp1_15 = _mm_add_epi16(stp2_15, stp2_12);                                 \
                                                                               \
    stp1_16 = stp2_16;                                                         \
    stp1_17 = stp2_17;                                                         \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_18_29, hi_18_29, lo_19_28, hi_19_28, stg4_4,     \
                           stg4_5, stg4_4, stg4_5, stp1_18, stp1_29, stp1_19,  \
                           stp1_28)                                            \
    MULTIPLICATION_AND_ADD(lo_20_27, hi_20_27, lo_21_26, hi_21_26, stg4_6,     \
                           stg4_4, stg4_6, stg4_4, stp1_20, stp1_27, stp1_21,  \
                           stp1_26)                                            \
                                                                               \
    stp1_22 = stp2_22;                                                         \
    stp1_23 = stp2_23;                                                         \
    stp1_24 = stp2_24;                                                         \
    stp1_25 = stp2_25;                                                         \
    stp1_30 = stp2_30;                                                         \
    stp1_31 = stp2_31;                                                         \
  }                                                                            \
                                                                               \
  /* Stage6 */                                                                 \
  {                                                                            \
    const __m128i lo_10_13 = _mm_unpacklo_epi16(stp1_10, stp1_13);             \
    const __m128i hi_10_13 = _mm_unpackhi_epi16(stp1_10, stp1_13);             \
    const __m128i lo_11_12 = _mm_unpacklo_epi16(stp1_11, stp1_12);             \
    const __m128i hi_11_12 = _mm_unpackhi_epi16(stp1_11, stp1_12);             \
                                                                               \
    stp2_0 = _mm_add_epi16(stp1_0, stp1_7);                                    \
    stp2_1 = _mm_add_epi16(stp1_1, stp1_6);                                    \
    stp2_2 = _mm_add_epi16(stp1_2, stp1_5);                                    \
    stp2_3 = _mm_add_epi16(stp1_3, stp1_4);                                    \
    stp2_4 = _mm_sub_epi16(stp1_3, stp1_4);                                    \
    stp2_5 = _mm_sub_epi16(stp1_2, stp1_5);                                    \
    stp2_6 = _mm_sub_epi16(stp1_1, stp1_6);                                    \
    stp2_7 = _mm_sub_epi16(stp1_0, stp1_7);                                    \
                                                                               \
    stp2_8 = stp1_8;                                                           \
    stp2_9 = stp1_9;                                                           \
    stp2_14 = stp1_14;                                                         \
    stp2_15 = stp1_15;                                                         \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_10_13, hi_10_13, lo_11_12, hi_11_12, stg6_0,     \
                           stg4_0, stg6_0, stg4_0, stp2_10, stp2_13, stp2_11,  \
                           stp2_12)                                            \
                                                                               \
    stp2_16 = _mm_add_epi16(stp1_16, stp1_23);                                 \
    stp2_17 = _mm_add_epi16(stp1_17, stp1_22);                                 \
    stp2_18 = _mm_add_epi16(stp1_18, stp1_21);                                 \
    stp2_19 = _mm_add_epi16(stp1_19, stp1_20);                                 \
    stp2_20 = _mm_sub_epi16(stp1_19, stp1_20);                                 \
    stp2_21 = _mm_sub_epi16(stp1_18, stp1_21);                                 \
    stp2_22 = _mm_sub_epi16(stp1_17, stp1_22);                                 \
    stp2_23 = _mm_sub_epi16(stp1_16, stp1_23);                                 \
                                                                               \
    stp2_24 = _mm_sub_epi16(stp1_31, stp1_24);                                 \
    stp2_25 = _mm_sub_epi16(stp1_30, stp1_25);                                 \
    stp2_26 = _mm_sub_epi16(stp1_29, stp1_26);                                 \
    stp2_27 = _mm_sub_epi16(stp1_28, stp1_27);                                 \
    stp2_28 = _mm_add_epi16(stp1_27, stp1_28);                                 \
    stp2_29 = _mm_add_epi16(stp1_26, stp1_29);                                 \
    stp2_30 = _mm_add_epi16(stp1_25, stp1_30);                                 \
    stp2_31 = _mm_add_epi16(stp1_24, stp1_31);                                 \
  }                                                                            \
                                                                               \
  /* Stage7 */                                                                 \
  {                                                                            \
    const __m128i lo_20_27 = _mm_unpacklo_epi16(stp2_20, stp2_27);             \
    const __m128i hi_20_27 = _mm_unpackhi_epi16(stp2_20, stp2_27);             \
    const __m128i lo_21_26 = _mm_unpacklo_epi16(stp2_21, stp2_26);             \
    const __m128i hi_21_26 = _mm_unpackhi_epi16(stp2_21, stp2_26);             \
                                                                               \
    const __m128i lo_22_25 = _mm_unpacklo_epi16(stp2_22, stp2_25);             \
    const __m128i hi_22_25 = _mm_unpackhi_epi16(stp2_22, stp2_25);             \
    const __m128i lo_23_24 = _mm_unpacklo_epi16(stp2_23, stp2_24);             \
    const __m128i hi_23_24 = _mm_unpackhi_epi16(stp2_23, stp2_24);             \
                                                                               \
    stp1_0 = _mm_add_epi16(stp2_0, stp2_15);                                   \
    stp1_1 = _mm_add_epi16(stp2_1, stp2_14);                                   \
    stp1_2 = _mm_add_epi16(stp2_2, stp2_13);                                   \
    stp1_3 = _mm_add_epi16(stp2_3, stp2_12);                                   \
    stp1_4 = _mm_add_epi16(stp2_4, stp2_11);                                   \
    stp1_5 = _mm_add_epi16(stp2_5, stp2_10);                                   \
    stp1_6 = _mm_add_epi16(stp2_6, stp2_9);                                    \
    stp1_7 = _mm_add_epi16(stp2_7, stp2_8);                                    \
    stp1_8 = _mm_sub_epi16(stp2_7, stp2_8);                                    \
    stp1_9 = _mm_sub_epi16(stp2_6, stp2_9);                                    \
    stp1_10 = _mm_sub_epi16(stp2_5, stp2_10);                                  \
    stp1_11 = _mm_sub_epi16(stp2_4, stp2_11);                                  \
    stp1_12 = _mm_sub_epi16(stp2_3, stp2_12);                                  \
    stp1_13 = _mm_sub_epi16(stp2_2, stp2_13);                                  \
    stp1_14 = _mm_sub_epi16(stp2_1, stp2_14);                                  \
    stp1_15 = _mm_sub_epi16(stp2_0, stp2_15);                                  \
                                                                               \
    stp1_16 = stp2_16;                                                         \
    stp1_17 = stp2_17;                                                         \
    stp1_18 = stp2_18;                                                         \
    stp1_19 = stp2_19;                                                         \
                                                                               \
    MULTIPLICATION_AND_ADD(lo_20_27, hi_20_27, lo_21_26, hi_21_26, stg6_0,     \
                           stg4_0, stg6_0, stg4_0, stp1_20, stp1_27, stp1_21,  \
                           stp1_26)                                            \
    MULTIPLICATION_AND_ADD(lo_22_25, hi_22_25, lo_23_24, hi_23_24, stg6_0,     \
                           stg4_0, stg6_0, stg4_0, stp1_22, stp1_25, stp1_23,  \
                           stp1_24)                                            \
                                                                               \
    stp1_28 = stp2_28;                                                         \
    stp1_29 = stp2_29;                                                         \
    stp1_30 = stp2_30;                                                         \
    stp1_31 = stp2_31;                                                         \
  }

// Only upper-left 8x8 has non-zero coeff
void vpx_idct32x32_34_add_sse2(const tran_low_t *input, uint8_t *dest,
                               int stride) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i final_rounding = _mm_set1_epi16(1 << 5);

  // idct constants for each stage
  const __m128i stg1_0 = pair_set_epi16(cospi_31_64, -cospi_1_64);
  const __m128i stg1_1 = pair_set_epi16(cospi_1_64, cospi_31_64);
  const __m128i stg1_6 = pair_set_epi16(cospi_7_64, -cospi_25_64);
  const __m128i stg1_7 = pair_set_epi16(cospi_25_64, cospi_7_64);
  const __m128i stg1_8 = pair_set_epi16(cospi_27_64, -cospi_5_64);
  const __m128i stg1_9 = pair_set_epi16(cospi_5_64, cospi_27_64);
  const __m128i stg1_14 = pair_set_epi16(cospi_3_64, -cospi_29_64);
  const __m128i stg1_15 = pair_set_epi16(cospi_29_64, cospi_3_64);

  const __m128i stg2_0 = pair_set_epi16(cospi_30_64, -cospi_2_64);
  const __m128i stg2_1 = pair_set_epi16(cospi_2_64, cospi_30_64);
  const __m128i stg2_6 = pair_set_epi16(cospi_6_64, -cospi_26_64);
  const __m128i stg2_7 = pair_set_epi16(cospi_26_64, cospi_6_64);

  const __m128i stg3_0 = pair_set_epi16(cospi_28_64, -cospi_4_64);
  const __m128i stg3_1 = pair_set_epi16(cospi_4_64, cospi_28_64);
  const __m128i stg3_4 = pair_set_epi16(-cospi_4_64, cospi_28_64);
  const __m128i stg3_5 = pair_set_epi16(cospi_28_64, cospi_4_64);
  const __m128i stg3_6 = pair_set_epi16(-cospi_28_64, -cospi_4_64);
  const __m128i stg3_8 = pair_set_epi16(-cospi_20_64, cospi_12_64);
  const __m128i stg3_9 = pair_set_epi16(cospi_12_64, cospi_20_64);
  const __m128i stg3_10 = pair_set_epi16(-cospi_12_64, -cospi_20_64);

  const __m128i stg4_0 = pair_set_epi16(cospi_16_64, cospi_16_64);
  const __m128i stg4_1 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i stg4_4 = pair_set_epi16(-cospi_8_64, cospi_24_64);
  const __m128i stg4_5 = pair_set_epi16(cospi_24_64, cospi_8_64);
  const __m128i stg4_6 = pair_set_epi16(-cospi_24_64, -cospi_8_64);

  const __m128i stg6_0 = pair_set_epi16(-cospi_16_64, cospi_16_64);

  __m128i in[32], col[32];
  __m128i stp1_0, stp1_1, stp1_2, stp1_3, stp1_4, stp1_5, stp1_6, stp1_7,
      stp1_8, stp1_9, stp1_10, stp1_11, stp1_12, stp1_13, stp1_14, stp1_15,
      stp1_16, stp1_17, stp1_18, stp1_19, stp1_20, stp1_21, stp1_22, stp1_23,
      stp1_24, stp1_25, stp1_26, stp1_27, stp1_28, stp1_29, stp1_30, stp1_31;
  __m128i stp2_0, stp2_1, stp2_2, stp2_3, stp2_4, stp2_5, stp2_6, stp2_7,
      stp2_8, stp2_9, stp2_10, stp2_11, stp2_12, stp2_13, stp2_14, stp2_15,
      stp2_16, stp2_17, stp2_18, stp2_19, stp2_20, stp2_21, stp2_22, stp2_23,
      stp2_24, stp2_25, stp2_26, stp2_27, stp2_28, stp2_29, stp2_30, stp2_31;
  int i;

  // Load input data. Only need to load the top left 8x8 block.
  in[0] = load_input_data(input);
  in[1] = load_input_data(input + 32);
  in[2] = load_input_data(input + 64);
  in[3] = load_input_data(input + 96);
  in[4] = load_input_data(input + 128);
  in[5] = load_input_data(input + 160);
  in[6] = load_input_data(input + 192);
  in[7] = load_input_data(input + 224);

  array_transpose_8x8(in, in);
  IDCT32_34

  // 1_D: Store 32 intermediate results for each 8x32 block.
  col[0] = _mm_add_epi16(stp1_0, stp1_31);
  col[1] = _mm_add_epi16(stp1_1, stp1_30);
  col[2] = _mm_add_epi16(stp1_2, stp1_29);
  col[3] = _mm_add_epi16(stp1_3, stp1_28);
  col[4] = _mm_add_epi16(stp1_4, stp1_27);
  col[5] = _mm_add_epi16(stp1_5, stp1_26);
  col[6] = _mm_add_epi16(stp1_6, stp1_25);
  col[7] = _mm_add_epi16(stp1_7, stp1_24);
  col[8] = _mm_add_epi16(stp1_8, stp1_23);
  col[9] = _mm_add_epi16(stp1_9, stp1_22);
  col[10] = _mm_add_epi16(stp1_10, stp1_21);
  col[11] = _mm_add_epi16(stp1_11, stp1_20);
  col[12] = _mm_add_epi16(stp1_12, stp1_19);
  col[13] = _mm_add_epi16(stp1_13, stp1_18);
  col[14] = _mm_add_epi16(stp1_14, stp1_17);
  col[15] = _mm_add_epi16(stp1_15, stp1_16);
  col[16] = _mm_sub_epi16(stp1_15, stp1_16);
  col[17] = _mm_sub_epi16(stp1_14, stp1_17);
  col[18] = _mm_sub_epi16(stp1_13, stp1_18);
  col[19] = _mm_sub_epi16(stp1_12, stp1_19);
  col[20] = _mm_sub_epi16(stp1_11, stp1_20);
  col[21] = _mm_sub_epi16(stp1_10, stp1_21);
  col[22] = _mm_sub_epi16(stp1_9, stp1_22);
  col[23] = _mm_sub_epi16(stp1_8, stp1_23);
  col[24] = _mm_sub_epi16(stp1_7, stp1_24);
  col[25] = _mm_sub_epi16(stp1_6, stp1_25);
  col[26] = _mm_sub_epi16(stp1_5, stp1_26);
  col[27] = _mm_sub_epi16(stp1_4, stp1_27);
  col[28] = _mm_sub_epi16(stp1_3, stp1_28);
  col[29] = _mm_sub_epi16(stp1_2, stp1_29);
  col[30] = _mm_sub_epi16(stp1_1, stp1_30);
  col[31] = _mm_sub_epi16(stp1_0, stp1_31);
  for (i = 0; i < 4; i++) {
    int j;
    // Transpose 32x8 block to 8x32 block
    array_transpose_8x8(col + i * 8, in);
    IDCT32_34

    // 2_D: Calculate the results and store them to destination.
    in[0] = _mm_add_epi16(stp1_0, stp1_31);
    in[1] = _mm_add_epi16(stp1_1, stp1_30);
    in[2] = _mm_add_epi16(stp1_2, stp1_29);
    in[3] = _mm_add_epi16(stp1_3, stp1_28);
    in[4] = _mm_add_epi16(stp1_4, stp1_27);
    in[5] = _mm_add_epi16(stp1_5, stp1_26);
    in[6] = _mm_add_epi16(stp1_6, stp1_25);
    in[7] = _mm_add_epi16(stp1_7, stp1_24);
    in[8] = _mm_add_epi16(stp1_8, stp1_23);
    in[9] = _mm_add_epi16(stp1_9, stp1_22);
    in[10] = _mm_add_epi16(stp1_10, stp1_21);
    in[11] = _mm_add_epi16(stp1_11, stp1_20);
    in[12] = _mm_add_epi16(stp1_12, stp1_19);
    in[13] = _mm_add_epi16(stp1_13, stp1_18);
    in[14] = _mm_add_epi16(stp1_14, stp1_17);
    in[15] = _mm_add_epi16(stp1_15, stp1_16);
    in[16] = _mm_sub_epi16(stp1_15, stp1_16);
    in[17] = _mm_sub_epi16(stp1_14, stp1_17);
    in[18] = _mm_sub_epi16(stp1_13, stp1_18);
    in[19] = _mm_sub_epi16(stp1_12, stp1_19);
    in[20] = _mm_sub_epi16(stp1_11, stp1_20);
    in[21] = _mm_sub_epi16(stp1_10, stp1_21);
    in[22] = _mm_sub_epi16(stp1_9, stp1_22);
    in[23] = _mm_sub_epi16(stp1_8, stp1_23);
    in[24] = _mm_sub_epi16(stp1_7, stp1_24);
    in[25] = _mm_sub_epi16(stp1_6, stp1_25);
    in[26] = _mm_sub_epi16(stp1_5, stp1_26);
    in[27] = _mm_sub_epi16(stp1_4, stp1_27);
    in[28] = _mm_sub_epi16(stp1_3, stp1_28);
    in[29] = _mm_sub_epi16(stp1_2, stp1_29);
    in[30] = _mm_sub_epi16(stp1_1, stp1_30);
    in[31] = _mm_sub_epi16(stp1_0, stp1_31);

    for (j = 0; j < 32; ++j) {
      // Final rounding and shift
      in[j] = _mm_adds_epi16(in[j], final_rounding);
      in[j] = _mm_srai_epi16(in[j], 6);
      RECON_AND_STORE(dest + j * stride, in[j]);
    }

    dest += 8;
  }
}

void vpx_idct32x32_1024_add_sse2(const tran_low_t *input, uint8_t *dest,
                                 int stride) {
  const __m128i final_rounding = _mm_set1_epi16(1 << 5);
  const __m128i zero = _mm_setzero_si128();

  // idct constants for each stage
  const __m128i stg1_0 = pair_set_epi16(cospi_31_64, -cospi_1_64);
  const __m128i stg1_1 = pair_set_epi16(cospi_1_64, cospi_31_64);
  const __m128i stg1_2 = pair_set_epi16(cospi_15_64, -cospi_17_64);
  const __m128i stg1_3 = pair_set_epi16(cospi_17_64, cospi_15_64);
  const __m128i stg1_4 = pair_set_epi16(cospi_23_64, -cospi_9_64);
  const __m128i stg1_5 = pair_set_epi16(cospi_9_64, cospi_23_64);
  const __m128i stg1_6 = pair_set_epi16(cospi_7_64, -cospi_25_64);
  const __m128i stg1_7 = pair_set_epi16(cospi_25_64, cospi_7_64);
  const __m128i stg1_8 = pair_set_epi16(cospi_27_64, -cospi_5_64);
  const __m128i stg1_9 = pair_set_epi16(cospi_5_64, cospi_27_64);
  const __m128i stg1_10 = pair_set_epi16(cospi_11_64, -cospi_21_64);
  const __m128i stg1_11 = pair_set_epi16(cospi_21_64, cospi_11_64);
  const __m128i stg1_12 = pair_set_epi16(cospi_19_64, -cospi_13_64);
  const __m128i stg1_13 = pair_set_epi16(cospi_13_64, cospi_19_64);
  const __m128i stg1_14 = pair_set_epi16(cospi_3_64, -cospi_29_64);
  const __m128i stg1_15 = pair_set_epi16(cospi_29_64, cospi_3_64);

  const __m128i stg2_0 = pair_set_epi16(cospi_30_64, -cospi_2_64);
  const __m128i stg2_1 = pair_set_epi16(cospi_2_64, cospi_30_64);
  const __m128i stg2_2 = pair_set_epi16(cospi_14_64, -cospi_18_64);
  const __m128i stg2_3 = pair_set_epi16(cospi_18_64, cospi_14_64);
  const __m128i stg2_4 = pair_set_epi16(cospi_22_64, -cospi_10_64);
  const __m128i stg2_5 = pair_set_epi16(cospi_10_64, cospi_22_64);
  const __m128i stg2_6 = pair_set_epi16(cospi_6_64, -cospi_26_64);
  const __m128i stg2_7 = pair_set_epi16(cospi_26_64, cospi_6_64);

  const __m128i stg3_0 = pair_set_epi16(cospi_28_64, -cospi_4_64);
  const __m128i stg3_1 = pair_set_epi16(cospi_4_64, cospi_28_64);
  const __m128i stg3_2 = pair_set_epi16(cospi_12_64, -cospi_20_64);
  const __m128i stg3_3 = pair_set_epi16(cospi_20_64, cospi_12_64);
  const __m128i stg3_4 = pair_set_epi16(-cospi_4_64, cospi_28_64);
  const __m128i stg3_5 = pair_set_epi16(cospi_28_64, cospi_4_64);
  const __m128i stg3_6 = pair_set_epi16(-cospi_28_64, -cospi_4_64);
  const __m128i stg3_8 = pair_set_epi16(-cospi_20_64, cospi_12_64);
  const __m128i stg3_9 = pair_set_epi16(cospi_12_64, cospi_20_64);
  const __m128i stg3_10 = pair_set_epi16(-cospi_12_64, -cospi_20_64);

  const __m128i stg4_0 = pair_set_epi16(cospi_16_64, cospi_16_64);
  const __m128i stg4_1 = pair_set_epi16(cospi_16_64, -cospi_16_64);
  const __m128i stg4_2 = pair_set_epi16(cospi_24_64, -cospi_8_64);
  const __m128i stg4_3 = pair_set_epi16(cospi_8_64, cospi_24_64);
  const __m128i stg4_4 = pair_set_epi16(-cospi_8_64, cospi_24_64);
  const __m128i stg4_5 = pair_set_epi16(cospi_24_64, cospi_8_64);
  const __m128i stg4_6 = pair_set_epi16(-cospi_24_64, -cospi_8_64);

  const __m128i stg6_0 = pair_set_epi16(-cospi_16_64, cospi_16_64);

  __m128i in[32], col[128], zero_idx[16];
  __m128i stp1_0, stp1_1, stp1_2, stp1_3, stp1_4, stp1_5, stp1_6, stp1_7,
      stp1_8, stp1_9, stp1_10, stp1_11, stp1_12, stp1_13, stp1_14, stp1_15,
      stp1_16, stp1_17, stp1_18, stp1_19, stp1_20, stp1_21, stp1_22, stp1_23,
      stp1_24, stp1_25, stp1_26, stp1_27, stp1_28, stp1_29, stp1_30, stp1_31;
  __m128i stp2_0, stp2_1, stp2_2, stp2_3, stp2_4, stp2_5, stp2_6, stp2_7,
      stp2_8, stp2_9, stp2_10, stp2_11, stp2_12, stp2_13, stp2_14, stp2_15,
      stp2_16, stp2_17, stp2_18, stp2_19, stp2_20, stp2_21, stp2_22, stp2_23,
      stp2_24, stp2_25, stp2_26, stp2_27, stp2_28, stp2_29, stp2_30, stp2_31;
  int i, j, i32;

  for (i = 0; i < 4; i++) {
    i32 = (i << 5);
    // First 1-D idct
    // Load input data.
    LOAD_DQCOEFF(in[0], input);
    LOAD_DQCOEFF(in[8], input);
    LOAD_DQCOEFF(in[16], input);
    LOAD_DQCOEFF(in[24], input);
    LOAD_DQCOEFF(in[1], input);
    LOAD_DQCOEFF(in[9], input);
    LOAD_DQCOEFF(in[17], input);
    LOAD_DQCOEFF(in[25], input);
    LOAD_DQCOEFF(in[2], input);
    LOAD_DQCOEFF(in[10], input);
    LOAD_DQCOEFF(in[18], input);
    LOAD_DQCOEFF(in[26], input);
    LOAD_DQCOEFF(in[3], input);
    LOAD_DQCOEFF(in[11], input);
    LOAD_DQCOEFF(in[19], input);
    LOAD_DQCOEFF(in[27], input);

    LOAD_DQCOEFF(in[4], input);
    LOAD_DQCOEFF(in[12], input);
    LOAD_DQCOEFF(in[20], input);
    LOAD_DQCOEFF(in[28], input);
    LOAD_DQCOEFF(in[5], input);
    LOAD_DQCOEFF(in[13], input);
    LOAD_DQCOEFF(in[21], input);
    LOAD_DQCOEFF(in[29], input);
    LOAD_DQCOEFF(in[6], input);
    LOAD_DQCOEFF(in[14], input);
    LOAD_DQCOEFF(in[22], input);
    LOAD_DQCOEFF(in[30], input);
    LOAD_DQCOEFF(in[7], input);
    LOAD_DQCOEFF(in[15], input);
    LOAD_DQCOEFF(in[23], input);
    LOAD_DQCOEFF(in[31], input);

    // checking if all entries are zero
    zero_idx[0] = _mm_or_si128(in[0], in[1]);
    zero_idx[1] = _mm_or_si128(in[2], in[3]);
    zero_idx[2] = _mm_or_si128(in[4], in[5]);
    zero_idx[3] = _mm_or_si128(in[6], in[7]);
    zero_idx[4] = _mm_or_si128(in[8], in[9]);
    zero_idx[5] = _mm_or_si128(in[10], in[11]);
    zero_idx[6] = _mm_or_si128(in[12], in[13]);
    zero_idx[7] = _mm_or_si128(in[14], in[15]);
    zero_idx[8] = _mm_or_si128(in[16], in[17]);
    zero_idx[9] = _mm_or_si128(in[18], in[19]);
    zero_idx[10] = _mm_or_si128(in[20], in[21]);
    zero_idx[11] = _mm_or_si128(in[22], in[23]);
    zero_idx[12] = _mm_or_si128(in[24], in[25]);
    zero_idx[13] = _mm_or_si128(in[26], in[27]);
    zero_idx[14] = _mm_or_si128(in[28], in[29]);
    zero_idx[15] = _mm_or_si128(in[30], in[31]);

    zero_idx[0] = _mm_or_si128(zero_idx[0], zero_idx[1]);
    zero_idx[1] = _mm_or_si128(zero_idx[2], zero_idx[3]);
    zero_idx[2] = _mm_or_si128(zero_idx[4], zero_idx[5]);
    zero_idx[3] = _mm_or_si128(zero_idx[6], zero_idx[7]);
    zero_idx[4] = _mm_or_si128(zero_idx[8], zero_idx[9]);
    zero_idx[5] = _mm_or_si128(zero_idx[10], zero_idx[11]);
    zero_idx[6] = _mm_or_si128(zero_idx[12], zero_idx[13]);
    zero_idx[7] = _mm_or_si128(zero_idx[14], zero_idx[15]);

    zero_idx[8] = _mm_or_si128(zero_idx[0], zero_idx[1]);
    zero_idx[9] = _mm_or_si128(zero_idx[2], zero_idx[3]);
    zero_idx[10] = _mm_or_si128(zero_idx[4], zero_idx[5]);
    zero_idx[11] = _mm_or_si128(zero_idx[6], zero_idx[7]);
    zero_idx[12] = _mm_or_si128(zero_idx[8], zero_idx[9]);
    zero_idx[13] = _mm_or_si128(zero_idx[10], zero_idx[11]);
    zero_idx[14] = _mm_or_si128(zero_idx[12], zero_idx[13]);

    if (_mm_movemask_epi8(_mm_cmpeq_epi32(zero_idx[14], zero)) == 0xFFFF) {
      col[i32 + 0] = _mm_setzero_si128();
      col[i32 + 1] = _mm_setzero_si128();
      col[i32 + 2] = _mm_setzero_si128();
      col[i32 + 3] = _mm_setzero_si128();
      col[i32 + 4] = _mm_setzero_si128();
      col[i32 + 5] = _mm_setzero_si128();
      col[i32 + 6] = _mm_setzero_si128();
      col[i32 + 7] = _mm_setzero_si128();
      col[i32 + 8] = _mm_setzero_si128();
      col[i32 + 9] = _mm_setzero_si128();
      col[i32 + 10] = _mm_setzero_si128();
      col[i32 + 11] = _mm_setzero_si128();
      col[i32 + 12] = _mm_setzero_si128();
      col[i32 + 13] = _mm_setzero_si128();
      col[i32 + 14] = _mm_setzero_si128();
      col[i32 + 15] = _mm_setzero_si128();
      col[i32 + 16] = _mm_setzero_si128();
      col[i32 + 17] = _mm_setzero_si128();
      col[i32 + 18] = _mm_setzero_si128();
      col[i32 + 19] = _mm_setzero_si128();
      col[i32 + 20] = _mm_setzero_si128();
      col[i32 + 21] = _mm_setzero_si128();
      col[i32 + 22] = _mm_setzero_si128();
      col[i32 + 23] = _mm_setzero_si128();
      col[i32 + 24] = _mm_setzero_si128();
      col[i32 + 25] = _mm_setzero_si128();
      col[i32 + 26] = _mm_setzero_si128();
      col[i32 + 27] = _mm_setzero_si128();
      col[i32 + 28] = _mm_setzero_si128();
      col[i32 + 29] = _mm_setzero_si128();
      col[i32 + 30] = _mm_setzero_si128();
      col[i32 + 31] = _mm_setzero_si128();
      continue;
    }

    // Transpose 32x8 block to 8x32 block
    array_transpose_8x8(in, in);
    array_transpose_8x8(in + 8, in + 8);
    array_transpose_8x8(in + 16, in + 16);
    array_transpose_8x8(in + 24, in + 24);

    IDCT32

    // 1_D: Store 32 intermediate results for each 8x32 block.
    col[i32 + 0] = _mm_add_epi16(stp1_0, stp1_31);
    col[i32 + 1] = _mm_add_epi16(stp1_1, stp1_30);
    col[i32 + 2] = _mm_add_epi16(stp1_2, stp1_29);
    col[i32 + 3] = _mm_add_epi16(stp1_3, stp1_28);
    col[i32 + 4] = _mm_add_epi16(stp1_4, stp1_27);
    col[i32 + 5] = _mm_add_epi16(stp1_5, stp1_26);
    col[i32 + 6] = _mm_add_epi16(stp1_6, stp1_25);
    col[i32 + 7] = _mm_add_epi16(stp1_7, stp1_24);
    col[i32 + 8] = _mm_add_epi16(stp1_8, stp1_23);
    col[i32 + 9] = _mm_add_epi16(stp1_9, stp1_22);
    col[i32 + 10] = _mm_add_epi16(stp1_10, stp1_21);
    col[i32 + 11] = _mm_add_epi16(stp1_11, stp1_20);
    col[i32 + 12] = _mm_add_epi16(stp1_12, stp1_19);
    col[i32 + 13] = _mm_add_epi16(stp1_13, stp1_18);
    col[i32 + 14] = _mm_add_epi16(stp1_14, stp1_17);
    col[i32 + 15] = _mm_add_epi16(stp1_15, stp1_16);
    col[i32 + 16] = _mm_sub_epi16(stp1_15, stp1_16);
    col[i32 + 17] = _mm_sub_epi16(stp1_14, stp1_17);
    col[i32 + 18] = _mm_sub_epi16(stp1_13, stp1_18);
    col[i32 + 19] = _mm_sub_epi16(stp1_12, stp1_19);
    col[i32 + 20] = _mm_sub_epi16(stp1_11, stp1_20);
    col[i32 + 21] = _mm_sub_epi16(stp1_10, stp1_21);
    col[i32 + 22] = _mm_sub_epi16(stp1_9, stp1_22);
    col[i32 + 23] = _mm_sub_epi16(stp1_8, stp1_23);
    col[i32 + 24] = _mm_sub_epi16(stp1_7, stp1_24);
    col[i32 + 25] = _mm_sub_epi16(stp1_6, stp1_25);
    col[i32 + 26] = _mm_sub_epi16(stp1_5, stp1_26);
    col[i32 + 27] = _mm_sub_epi16(stp1_4, stp1_27);
    col[i32 + 28] = _mm_sub_epi16(stp1_3, stp1_28);
    col[i32 + 29] = _mm_sub_epi16(stp1_2, stp1_29);
    col[i32 + 30] = _mm_sub_epi16(stp1_1, stp1_30);
    col[i32 + 31] = _mm_sub_epi16(stp1_0, stp1_31);
  }
  for (i = 0; i < 4; i++) {
    // Second 1-D idct
    j = i << 3;

    // Transpose 32x8 block to 8x32 block
    array_transpose_8x8(col + j, in);
    array_transpose_8x8(col + j + 32, in + 8);
    array_transpose_8x8(col + j + 64, in + 16);
    array_transpose_8x8(col + j + 96, in + 24);

    IDCT32

    // 2_D: Calculate the results and store them to destination.
    in[0] = _mm_add_epi16(stp1_0, stp1_31);
    in[1] = _mm_add_epi16(stp1_1, stp1_30);
    in[2] = _mm_add_epi16(stp1_2, stp1_29);
    in[3] = _mm_add_epi16(stp1_3, stp1_28);
    in[4] = _mm_add_epi16(stp1_4, stp1_27);
    in[5] = _mm_add_epi16(stp1_5, stp1_26);
    in[6] = _mm_add_epi16(stp1_6, stp1_25);
    in[7] = _mm_add_epi16(stp1_7, stp1_24);
    in[8] = _mm_add_epi16(stp1_8, stp1_23);
    in[9] = _mm_add_epi16(stp1_9, stp1_22);
    in[10] = _mm_add_epi16(stp1_10, stp1_21);
    in[11] = _mm_add_epi16(stp1_11, stp1_20);
    in[12] = _mm_add_epi16(stp1_12, stp1_19);
    in[13] = _mm_add_epi16(stp1_13, stp1_18);
    in[14] = _mm_add_epi16(stp1_14, stp1_17);
    in[15] = _mm_add_epi16(stp1_15, stp1_16);
    in[16] = _mm_sub_epi16(stp1_15, stp1_16);
    in[17] = _mm_sub_epi16(stp1_14, stp1_17);
    in[18] = _mm_sub_epi16(stp1_13, stp1_18);
    in[19] = _mm_sub_epi16(stp1_12, stp1_19);
    in[20] = _mm_sub_epi16(stp1_11, stp1_20);
    in[21] = _mm_sub_epi16(stp1_10, stp1_21);
    in[22] = _mm_sub_epi16(stp1_9, stp1_22);
    in[23] = _mm_sub_epi16(stp1_8, stp1_23);
    in[24] = _mm_sub_epi16(stp1_7, stp1_24);
    in[25] = _mm_sub_epi16(stp1_6, stp1_25);
    in[26] = _mm_sub_epi16(stp1_5, stp1_26);
    in[27] = _mm_sub_epi16(stp1_4, stp1_27);
    in[28] = _mm_sub_epi16(stp1_3, stp1_28);
    in[29] = _mm_sub_epi16(stp1_2, stp1_29);
    in[30] = _mm_sub_epi16(stp1_1, stp1_30);
    in[31] = _mm_sub_epi16(stp1_0, stp1_31);

    for (j = 0; j < 32; ++j) {
      // Final rounding and shift
      in[j] = _mm_adds_epi16(in[j], final_rounding);
      in[j] = _mm_srai_epi16(in[j], 6);
      RECON_AND_STORE(dest + j * stride, in[j]);
    }

    dest += 8;
  }
}

void vpx_idct32x32_1_add_sse2(const tran_low_t *input, uint8_t *dest,
                              int stride) {
  __m128i dc_value;
  const __m128i zero = _mm_setzero_si128();
  int a, j;

  a = (int)dct_const_round_shift(input[0] * cospi_16_64);
  a = (int)dct_const_round_shift(a * cospi_16_64);
  a = ROUND_POWER_OF_TWO(a, 6);

  dc_value = _mm_set1_epi16(a);

  for (j = 0; j < 32; ++j) {
    RECON_AND_STORE(dest + 0 + j * stride, dc_value);
    RECON_AND_STORE(dest + 8 + j * stride, dc_value);
    RECON_AND_STORE(dest + 16 + j * stride, dc_value);
    RECON_AND_STORE(dest + 24 + j * stride, dc_value);
  }
}
