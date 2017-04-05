/** Moving window / Recursive DFT implementation based on HLS
 *
 * @file
 * @author Steffen Vogel <stvogel@eonerc.rwth-aachen.de>
 * @copyright 2016, Steffen Vogel
 **********************************************************************************/
 
#ifndef _DFT_H_
#define _DFT_H_

#include <xilinx/xhls_dft.h>

/* Forward declaration */
struct ip;

struct dft {
	XHls_dft inst;
	
	int period; /* in samples */
	int num_harmonics;
	float *fharmonics;
	int decimation;
};

int dft_parse(struct ip *c);

int dft_init(struct ip *c);

void dft_destroy(struct ip *c);


#endif /* _DFT_H_ */