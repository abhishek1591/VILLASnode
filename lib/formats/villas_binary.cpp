/** Message related functions.
 *
 * @author Steffen Vogel <stvogel@eonerc.rwth-aachen.de>
 * @copyright 2014-2019, Institute for Automation of Complex Power Systems, EONERC
 * @license GNU General Public License (version 3)
 *
 * VILLASnode
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *********************************************************************************/

#include <string.h>

#include <villas/formats/villas_binary.hpp>
#include <villas/formats/msg.h>
#include <villas/formats/msg_format.h>
#include <villas/sample.h>
#include <villas/utils.hpp>

using namespace villas::node::formats;

int VillasBinary::print(char *buf, size_t len, size_t *wbytes, struct sample *smps[], unsigned cnt)
{
	int ret;
	unsigned i = 0;
	char *ptr = buf;

	for (i = 0; i < cnt; i++) {
		struct msg *msg = (struct msg *) ptr;
		struct sample *smp = smps[i];

		if (ptr + MSG_LEN(smp->length) > buf + len)
			break;

		ret = msg_from_sample(msg, smp, smp->signals);
		if (ret)
			return ret;

		if (flags & VILLAS_BINARY_WEB) {
			/** @todo convert to little endian */
		}
		else
			msg_hton(msg);

		ptr += MSG_LEN(smp->length);
	}

	if (wbytes)
		*wbytes = ptr - buf;

	return i;
}

int VillasBinary::scan(const char *buf, size_t len, size_t *rbytes, struct sample *smps[], unsigned cnt)
{
	int ret, values;
	unsigned i = 0;
	const char *ptr = buf;

	if (len % 4 != 0) {
		warning("Packet size is invalid: %zd Must be multiple of 4 bytes.", len);
		return -1;
	}

	for (i = 0; i < cnt; i++) {
		struct msg *msg = (struct msg *) ptr;
		struct sample *smp = smps[i];

		smp->signals = signals;

		/* Complete buffer has been parsed */
		if (ptr == buf + len)
			break;

		/* Check if header is still in buffer bounaries */
		if (ptr + sizeof(struct msg) > buf + len) {
			warning("Invalid msg received: reason=1");
			break;
		}

		values = (flags & VILLAS_BINARY_WEB) ? msg->length : ntohs(msg->length);

		/* Check if remainder of message is in buffer boundaries */
		if (ptr + MSG_LEN(values) > buf + len) {
			warning("Invalid msg received: reason=2, msglen=%zu, len=%zu, ptr=%p, buf=%p, i=%u", MSG_LEN(values), len, ptr, buf, i);
			break;
		}

		if (flags & VILLAS_BINARY_WEB) {
			/** @todo convert from little endian */
		}
		else
			msg_ntoh(msg);

		ret = msg_to_sample(msg, smp, signals);
		if (ret) {
			warning("Invalid msg received: reason=3, ret=%d", ret);
			break;
		}

		ptr += MSG_LEN(smp->length);
	}

	if (rbytes)
		*rbytes = ptr - buf;

	return i;
}

/* Register plugins */
static FormatPlugin<Json> p1(
	"villas.binary",
	"VILLAS binary network format",
	IO_HAS_BINARY_PAYLOAD | SAMPLE_HAS_TS_ORIGIN | SAMPLE_HAS_SEQUENCE | SAMPLE_HAS_DATA
);

static FormatPlugin<Json> p2(
	"villas.web",
	"VILLAS binary network format for WebSockets",
	IO_HAS_BINARY_PAYLOAD | VILLAS_BINARY_WEB | SAMPLE_HAS_TS_ORIGIN | SAMPLE_HAS_SEQUENCE | SAMPLE_HAS_DATA
);
