#include "sum_330.h"
/**
 * @author Timo Grautstueck
 */
void sum_330(hls::stream<side_chan> &in, hls::stream<side_chan> &out) {
#pragma HLS INTERFACE axis port=in
#pragma HLS INTERFACE axis port=out
#pragma HLS INTERFACE ap_ctrl_none port=return // no control interface

	side_chan tmp1, tmp2;
	int cnt = 0, sum = 0;

	while (true) {
#pragma HLS PIPELINE
		tmp1 = in.read();
		sum += tmp1.data.to_int();
		cnt++;
		if (tmp1.last) {
			cnt = 0;
			tmp2.data = sum;
			tmp2.keep = -1;
			tmp2.strb = tmp1.strb;
			tmp2.user = tmp1.user;
			tmp2.last = 1;
			tmp2.id   = tmp1.id;
			tmp2.dest = tmp1.dest;
			out.write(tmp2);
			sum = 0;
			break;
		}
		if (cnt == N) {
			cnt = 0;
			tmp2.data = sum;
			tmp2.keep = tmp1.keep;
			tmp2.strb = tmp1.strb;
			tmp2.user = tmp1.user;
			tmp2.last = tmp1.last;
			tmp2.id   = tmp1.id;
			tmp2.dest = tmp1.dest;
			out.write(tmp2);
			sum = 0;
		}
	}
}
