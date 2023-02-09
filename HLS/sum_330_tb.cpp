#include "sum_330.h"

/**
 * @author Timo Grautstück
 *
 * Loop: 1000x
 * 	=> int(1000 / 330) = 3
 * 	=> 1000 % 330 = 10
 *
 * Result:
 * 	1. => 330*100
 * 	2. => 330*100
 * 	3. => 330*100
 * 	4. => 10*100
 */
/**
 * Stream starts if:
 * 	1. TVALID & TREADY = 1
 */
int main() {
	int i = 100;
	int ret = 0;
	hls::stream<side_chan> in, out;
	side_chan tmp1, tmp2;

	for (int j = 0; j < 1000; j++) {
		tmp1.data = i;
		tmp1.keep = 1;
		tmp1.strb = 1;
		tmp1.user = 1;
		tmp1.last = 0;
		tmp1.id   = 0;
		tmp1.dest = 1;

		if (j == 999)
			tmp1.last = 1;

		in.write(tmp1);
	}
	printf("\n");

	// Calculate:
	sum_330(in, out);

	// Result:
	while(!out.empty()) {
		out.read(tmp2);
		ret = tmp2.data.to_int();
		printf("RESULT: %d\t", ret);
	}

	printf("\n");
}
