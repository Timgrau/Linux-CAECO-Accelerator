#ifndef _SUM_330_H_
#define _SUM_330_H_

#include <stdio.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <assert.h>

#define N 330
// 32-bit stream with side channel
typedef ap_axis<32, 2, 5, 6> side_chan;
void sum_330(hls::stream<side_chan> &in_stream,
		hls::stream<side_chan> &out_stream);

#endif
