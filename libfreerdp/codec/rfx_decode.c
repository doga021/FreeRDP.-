/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * RemoteFX Codec Library - Decode
 *
 * Copyright 2011 Vic Lee
 * Copyright 2011 Norbert Federa <nfedera@thinstuff.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <freerdp/utils/stream.h>

#include "rfx_types.h"
#include "rfx_rlgr.h"
#include "rfx_differential.h"
#include "rfx_quantization.h"
#include "rfx_dwt.h"

#include "rfx_decode.h"

static void rfx_decode_format_rgb(INT16* r_buf, INT16* g_buf, INT16* b_buf,
	RDP_PIXEL_FORMAT pixel_format, BYTE* dst_buf)
{
	INT16* r = r_buf;
	INT16* g = g_buf;
	INT16* b = b_buf;
	BYTE* dst = dst_buf;
	int i;
	
	switch (pixel_format)
	{
		case RDP_PIXEL_FORMAT_B8G8R8A8:
			for (i = 0; i < 4096; i++)
			{
				*dst++ = (BYTE) (*b++);
				*dst++ = (BYTE) (*g++);
				*dst++ = (BYTE) (*r++);
				*dst++ = 0xFF;
			}
			break;
		case RDP_PIXEL_FORMAT_R8G8B8A8:
			for (i = 0; i < 4096; i++)
			{
				*dst++ = (BYTE) (*r++);
				*dst++ = (BYTE) (*g++);
				*dst++ = (BYTE) (*b++);
				*dst++ = 0xFF;
			}
			break;
		case RDP_PIXEL_FORMAT_B8G8R8:
			for (i = 0; i < 4096; i++)
			{
				*dst++ = (BYTE) (*b++);
				*dst++ = (BYTE) (*g++);
				*dst++ = (BYTE) (*r++);
			}
			break;
		case RDP_PIXEL_FORMAT_R8G8B8:
			for (i = 0; i < 4096; i++)
			{
				*dst++ = (BYTE) (*r++);
				*dst++ = (BYTE) (*g++);
				*dst++ = (BYTE) (*b++);
			}
			break;
		default:
			break;
	}
}

#define MINMAX(_v,_l,_h) ((_v) < (_l) ? (_l) : ((_v) > (_h) ? (_h) : (_v)))

void rfx_decode_ycbcr_to_rgb(INT16* y_r_buf, INT16* cb_g_buf, INT16* cr_b_buf)
{
	/* sint32 is used intentionally because we calculate with shifted factors! */
	sint32 y, cb, cr;
	sint32 r, g, b;
	int i;

	/**
	 * The decoded YCbCr coeffectients are represented as 11.5 fixed-point numbers:
	 *
	 * 1 sign bit + 10 integer bits + 5 fractional bits
	 *
	 * However only 7 integer bits will be actually used since the value range is [-128.0, 127.0].
	 * In other words, the decoded coeffectients is scaled by << 5 when intepreted as INT16.
	 * It was scaled in the quantization phase, so we must scale it back here.
	 */
	for (i = 0; i < 4096; i++)
	{
		y = y_r_buf[i];
		cb = cb_g_buf[i];
		cr = cr_b_buf[i];

#if 0
		/**
		 * This is the slow floating point version kept here for reference
		 */

		y = y + 4096; /* 128<<5=4096 so that we can scale the sum by >> 5 */

		r = y + cr*1.403f;
		g = y - cb*0.344f - cr*0.714f;
		b = y + cb*1.770f;

		y_r_buf[i]  = MINMAX(r>>5, 0, 255);
		cb_g_buf[i] = MINMAX(g>>5, 0, 255);
		cr_b_buf[i] = MINMAX(b>>5, 0, 255);
#else
		/**
		 * We scale the factors by << 16 into 32-bit integers in order to avoid slower
		 * floating point multiplications. Since the final result needs to be scaled
		 * by >> 5 we will extract only the upper 11 bits (>> 21) from the final sum.
		 * Hence we also have to scale the other terms of the sum by << 16.
		 *
		 * R: 1.403 << 16 = 91947
		 * G: 0.344 << 16 = 22544, 0.714 << 16 = 46792
		 * B: 1.770 << 16 = 115998
		 */

		y = (y+4096)<<16;

		r = y + cr*91947;
		g = y - cb*22544 - cr*46792;
		b = y + cb*115998;

		y_r_buf[i]  = MINMAX(r>>21, 0, 255);
		cb_g_buf[i] = MINMAX(g>>21, 0, 255);
		cr_b_buf[i] = MINMAX(b>>21, 0, 255);
#endif
	}
}

static void rfx_decode_component(RFX_CONTEXT* context, const uint32* quantization_values,
	const BYTE* data, int size, INT16* buffer)
{
	PROFILER_ENTER(context->priv->prof_rfx_decode_component);

	PROFILER_ENTER(context->priv->prof_rfx_rlgr_decode);
		rfx_rlgr_decode(context->mode, data, size, buffer, 4096);
	PROFILER_EXIT(context->priv->prof_rfx_rlgr_decode);

	PROFILER_ENTER(context->priv->prof_rfx_differential_decode);
		rfx_differential_decode(buffer + 4032, 64);
	PROFILER_EXIT(context->priv->prof_rfx_differential_decode);

	PROFILER_ENTER(context->priv->prof_rfx_quantization_decode);
		context->quantization_decode(buffer, quantization_values);
	PROFILER_EXIT(context->priv->prof_rfx_quantization_decode);

	PROFILER_ENTER(context->priv->prof_rfx_dwt_2d_decode);
		context->dwt_2d_decode(buffer, context->priv->dwt_buffer);
	PROFILER_EXIT(context->priv->prof_rfx_dwt_2d_decode);

	PROFILER_EXIT(context->priv->prof_rfx_decode_component);
}

void rfx_decode_rgb(RFX_CONTEXT* context, STREAM* data_in,
	int y_size, const uint32 * y_quants,
	int cb_size, const uint32 * cb_quants,
	int cr_size, const uint32 * cr_quants, BYTE* rgb_buffer)
{
	PROFILER_ENTER(context->priv->prof_rfx_decode_rgb);

	rfx_decode_component(context, y_quants, stream_get_tail(data_in), y_size, context->priv->y_r_buffer); /* YData */
	stream_seek(data_in, y_size);
	rfx_decode_component(context, cb_quants, stream_get_tail(data_in), cb_size, context->priv->cb_g_buffer); /* CbData */
	stream_seek(data_in, cb_size);
	rfx_decode_component(context, cr_quants, stream_get_tail(data_in), cr_size, context->priv->cr_b_buffer); /* CrData */
	stream_seek(data_in, cr_size);

	PROFILER_ENTER(context->priv->prof_rfx_decode_ycbcr_to_rgb);
		context->decode_ycbcr_to_rgb(context->priv->y_r_buffer, context->priv->cb_g_buffer, context->priv->cr_b_buffer);
	PROFILER_EXIT(context->priv->prof_rfx_decode_ycbcr_to_rgb);

	PROFILER_ENTER(context->priv->prof_rfx_decode_format_rgb);
		rfx_decode_format_rgb(context->priv->y_r_buffer, context->priv->cb_g_buffer, context->priv->cr_b_buffer,
			context->pixel_format, rgb_buffer);
	PROFILER_EXIT(context->priv->prof_rfx_decode_format_rgb);
	
	PROFILER_EXIT(context->priv->prof_rfx_decode_rgb);
}
