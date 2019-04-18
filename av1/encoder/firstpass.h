/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_AV1_ENCODER_FIRSTPASS_H_
#define AOM_AV1_ENCODER_FIRSTPASS_H_

#include "av1/common/enums.h"
#include "av1/common/onyxc_int.h"
#include "av1/encoder/lookahead.h"
#include "av1/encoder/ratectrl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DOUBLE_DIVIDE_CHECK(x) ((x) < 0 ? (x)-0.000001 : (x) + 0.000001)

#define MIN_ZERO_MOTION 0.95
#define MAX_SR_CODED_ERROR 40
#define MAX_RAW_ERR_VAR 2000
#define MIN_MV_IN_OUT 0.4

#define VLOW_MOTION_THRESHOLD 950

typedef struct {
  // Frame number in display order, if stats are for a single frame.
  // No real meaning for a collection of frames.
  double frame;
  // Weight assigned to this frame (or total weight for the collection of
  // frames) currently based on intra factor and brightness factor. This is used
  // to distribute bits betweeen easier and harder frames.
  double weight;
  // Intra prediction error.
  double intra_error;
  // Average wavelet energy computed using Discrete Wavelet Transform (DWT).
  double frame_avg_wavelet_energy;
  // Best of intra pred error and inter pred error using last frame as ref.
  double coded_error;
  // Best of intra pred error and inter pred error using golden frame as ref.
  double sr_coded_error;
  // Percentage of blocks with inter pred error < intra pred error.
  double pcnt_inter;
  // Percentage of blocks using (inter prediction and) non-zero motion vectors.
  double pcnt_motion;
  // Percentage of blocks where golden frame was the best reference. That is:
  // inter pred error using golden frame < inter pred error using last frame and
  // inter pred error using golden frame < intra pred error
  double pcnt_second_ref;
  // Percentage of blocks where intra and inter prediction errors were very
  // close. Note that this is a 'weighted count', that is, the so blocks may be
  // weighted by how close the two errors were.
  double pcnt_neutral;
  // Percentage of blocks that have almost no intra error residual
  // (i.e. are in effect completely flat and untextured in the intra
  // domain). In natural videos this is uncommon, but it is much more
  // common in animations, graphics and screen content, so may be used
  // as a signal to detect these types of content.
  double intra_skip_pct;
  // Image mask rows top and bottom.
  double inactive_zone_rows;
  // Image mask columns at left and right edges.
  double inactive_zone_cols;
  // Average of row motion vectors.
  double MVr;
  // Mean of absolute value of row motion vectors.
  double mvr_abs;
  // Mean of column motion vectors.
  double MVc;
  // Mean of absolute value of column motion vectors.
  double mvc_abs;
  // Variance of row motion vectors.
  double MVrv;
  // Variance of column motion vectors.
  double MVcv;
  // Value in range [-1,1] indicating fraction of row and column motion vectors
  // that point inwards (negative MV value) or outwards (positive MV value).
  // For example, value of 1 indicates, all row/column MVs are inwards.
  double mv_in_out_count;
  // Count of unique non-zero motion vectors.
  double new_mv_count;
  // Duration of the frame / collection of frames.
  double duration;
  // 1.0 if stats are for a single frame, OR
  // Number of frames in this collection for which the stats are accumulated.
  double count;
  // standard deviation for (0, 0) motion prediction error
  double raw_error_stdev;
} FIRSTPASS_STATS;

enum {
  KF_UPDATE,
  LF_UPDATE,
  GF_UPDATE,
  ARF_UPDATE,
  OVERLAY_UPDATE,
  INTNL_OVERLAY_UPDATE,  // Internal Overlay Frame
  INTNL_ARF_UPDATE,      // Internal Altref Frame
  FRAME_UPDATE_TYPES
} UENUM1BYTE(FRAME_UPDATE_TYPE);

#define FC_ANIMATION_THRESH 0.15
enum {
  FC_NORMAL = 0,
  FC_GRAPHICS_ANIMATION = 1,
  FRAME_CONTENT_TYPES = 2
} UENUM1BYTE(FRAME_CONTENT_TYPE);

typedef struct {
  unsigned char index;
  FRAME_UPDATE_TYPE update_type[MAX_STATIC_GF_GROUP_LENGTH + 1];
  unsigned char arf_src_offset[MAX_STATIC_GF_GROUP_LENGTH + 1];
  unsigned char arf_update_idx[MAX_STATIC_GF_GROUP_LENGTH + 1];
  unsigned char arf_pos_in_gf[MAX_STATIC_GF_GROUP_LENGTH + 1];
  unsigned char frame_disp_idx[MAX_STATIC_GF_GROUP_LENGTH + 1];
  unsigned char pyramid_level[MAX_STATIC_GF_GROUP_LENGTH + 1];
  int ref_frame_disp_idx[MAX_STATIC_GF_GROUP_LENGTH + 1][REF_FRAMES];
  int ref_frame_gop_idx[MAX_STATIC_GF_GROUP_LENGTH + 1][REF_FRAMES];
  unsigned char pyramid_height;
  unsigned char pyramid_lvl_nodes[MAX_PYRAMID_LVL];
  int bit_allocation[MAX_STATIC_GF_GROUP_LENGTH + 1];
  int size;
} GF_GROUP;

typedef struct {
  unsigned int section_intra_rating;
  FIRSTPASS_STATS total_stats;
  FIRSTPASS_STATS this_frame_stats;
  const FIRSTPASS_STATS *stats_in;
  const FIRSTPASS_STATS *stats_in_start;
  const FIRSTPASS_STATS *stats_in_end;
  FIRSTPASS_STATS total_left_stats;
  int first_pass_done;
  int64_t bits_left;
  double modified_error_min;
  double modified_error_max;
  double modified_error_left;
  double mb_av_energy;
  double frame_avg_haar_energy;

  // An indication of the content type of the current frame
  FRAME_CONTENT_TYPE fr_content_type;

  // Projected total bits available for a key frame group of frames
  int64_t kf_group_bits;

  // Error score of frames still to be coded in kf group
  int64_t kf_group_error_left;

  // The fraction for a kf groups total bits allocated to the inter frames
  double kfgroup_inter_fraction;

  int sr_update_lag;

  int kf_zeromotion_pct;
  int last_kfgroup_zeromotion_pct;
  int active_worst_quality;
  int baseline_active_worst_quality;
  int extend_minq;
  int extend_maxq;
  int extend_minq_fast;

  GF_GROUP gf_group;
} TWO_PASS;

struct AV1_COMP;
struct EncodeFrameParams;
struct AV1EncoderConfig;

void av1_init_first_pass(struct AV1_COMP *cpi);
void av1_rc_get_first_pass_params(struct AV1_COMP *cpi);
void av1_first_pass(struct AV1_COMP *cpi, const int64_t ts_duration);
void av1_end_first_pass(struct AV1_COMP *cpi);

void av1_twopass_zero_stats(FIRSTPASS_STATS *section);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_AV1_ENCODER_FIRSTPASS_H_
