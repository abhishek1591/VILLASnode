/** The internal datastructure for a sample of simulation data.
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

#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#include <villas/utils.hpp>
#include <villas/timing.h>
#include <villas/sample.h>
#include <villas/signal.h>
#include <villas/formats/villas_human.hpp>

using namespace villas::node::formats;

size_t VillasHuman::printSingle(char *buf, size_t len, const struct sample *smp)
{
	size_t off = 0;
	struct signal *sig;

	if (io->flags & SAMPLE_HAS_TS_ORIGIN) {
		if (smp->flags & SAMPLE_HAS_TS_ORIGIN) {
			off += snprintf(buf + off, len - off, "%llu", (unsigned long long) smp->ts.origin.tv_sec);
			off += snprintf(buf + off, len - off, ".%09llu", (unsigned long long) smp->ts.origin.tv_nsec);
		}
		else
			off += snprintf(buf + off, len - off, "nan.nan");
	}

	if (io->flags & SAMPLE_HAS_OFFSET) {
		if (smp->flags & SAMPLE_HAS_TS_RECEIVED)
			off += snprintf(buf + off, len - off, "%+e", time_delta(&smp->ts.origin, &smp->ts.received));
	}

	if (io->flags & SAMPLE_HAS_SEQUENCE) {
		if (io->flags & SAMPLE_HAS_SEQUENCE)
			off += snprintf(buf + off, len - off, "(%" PRIu64 ")", smp->sequence);
	}

	if (io->flags & SAMPLE_HAS_DATA) {
		for (unsigned i = 0; i < smp->length; i++) {
			sig = (struct signal *) vlist_at_safe(smp->signals, i);
			if (!sig)
				break;

			off += snprintf(buf + off, len - off, "%c", io->separator);
			off += signal_data_snprint(&smp->data[i], sig, buf + off, len - off);
		}
	}

	off += snprintf(buf + off, len - off, "%c", io->delimiter);

	return off;
}

size_t VillasHuman::scanSingle(const char *buf, size_t len, struct sample *smp)
{
	int ret;
	char *end;
	const char *ptr = buf;

	double offset = 0;

	smp->flags = 0;
	smp->signals = io->signals;

	/* Format: Seconds.NanoSeconds+Offset(SequenceNumber) Value1 Value2 ...
	 * RegEx: (\d+(?:\.\d+)?)([-+]\d+(?:\.\d+)?(?:e[+-]?\d+)?)?(?:\((\d+)\))?
	 *
	 * Please note that only the seconds and at least one value are mandatory
	 */

	/* Mandatory: seconds */
	smp->ts.origin.tv_sec = (uint32_t) strtoul(ptr, &end, 10);
	if (ptr == end || *end == io->delimiter)
		return -1;

	smp->flags |= SAMPLE_HAS_TS_ORIGIN;

	/* Optional: nano seconds */
	if (*end == '.') {
		ptr = end + 1;

		smp->ts.origin.tv_nsec = (uint32_t) strtoul(ptr, &end, 10);
		if (ptr == end)
			return -3;
	}
	else
		smp->ts.origin.tv_nsec = 0;

	/* Optional: offset / delay */
	if (*end == '+' || *end == '-') {
		ptr = end;

		offset = strtof(ptr, &end); /* offset is ignored for now */
		if (ptr != end)
			smp->flags |= SAMPLE_HAS_OFFSET;
		else
			return -4;
	}

	/* Optional: sequence */
	if (*end == '(') {
		ptr = end + 1;

		smp->sequence = strtoul(ptr, &end, 10);
		if (ptr != end)
			smp->flags |= SAMPLE_HAS_SEQUENCE;
		else
			return -5;

		if (*end == ')')
			end++;
	}

	unsigned i;
	for (ptr = end + 1, i = 0; i < smp->capacity; ptr = end + 1, i++) {

		if (*end == io->delimiter)
			goto out;

		struct signal *sig = (struct signal *) vlist_at_safe(io->signals, i);
		if (!sig)
			goto out;

		ret = signal_data_parse_str(&smp->data[i], sig, ptr, &end);
		if (ret || end == ptr) /* There are no valid values anymore. */
			goto out;
	}

out:	if (*end == delimiter)
		end++;

	smp->length = i;
	if (smp->length > 0)
		smp->flags |= SAMPLE_HAS_DATA;

	if (smp->flags & SAMPLE_HAS_OFFSET) {
		struct timespec off = time_from_double(offset);
		smp->ts.received = time_add(&smp->ts.origin, &off);

		smp->flags |= SAMPLE_HAS_TS_RECEIVED;
	}

	return end - buf;
}

int VillasHuman::print(char *buf, size_t len, size_t *wbytes, struct sample *smps[], unsigned cnt)
{
	unsigned i;
	size_t off = 0;

	for (i = 0; i < cnt && off < len; i++)
		off += printSingle(buf + off, len - off, smps[i]);

	if (wbytes)
		*wbytes = off;

	return i;
}

int VillasHuman::scan(const char *buf, size_t len, size_t *rbytes, struct sample *smps[], unsigned cnt)
{
	unsigned i;
	size_t off = 0;

	for (i = 0; i < cnt && off < len; i++)
		off += scanSingle(buf + off, len - off, smps[i]);

	if (rbytes)
		*rbytes = off;

	return i;
}

void VillasHuman::header(char *buf, size_t len, size_t *wbytes)
{
	pos += snprintf(buf + pos, len - pos, "# ");

	if (flags & SAMPLE_HAS_TS_ORIGIN)
		pos += snprintf(buf + pos, len - pos, "seconds.nanoseconds");

	if (flags & SAMPLE_HAS_OFFSET)
		pos += snprintf(buf + pos, len - pos, "+offset");

	if (flags & SAMPLE_HAS_SEQUENCE)
		pos += snprintf(buf + pos, len - pos, "(sequence)");

	if (flags & SAMPLE_HAS_DATA) {
		for (unsigned i = 0; i < MIN(smp->length, vlist_length(signals)); i++) {
			struct signal *sig = (struct signal *) vlist_at(signals, i);

			if (sig->name)
				pos += snprintf(buf + pos, len - pos, "%c%s", separator, sig->name);
			else
				pos += snprintf(buf + pos, len - pos, "%csignal%d", separator, i);

			if (sig->unit)
				pos += snprintf(buf + pos, len - pos, "[%s]", sig->unit);
		}
	}

	pos += snprintf(buf + pos, len - pos, "%c", delimiter);

	if (wbytes)
		*wbytes = pos;
}

/* Register plugin */
static FormatPlugin<VillasHuman> p(
	"villas.human",
	"VILLAS human readable format",
	IO_NEWLINES | SAMPLE_HAS_TS_ORIGIN | SAMPLE_HAS_SEQUENCE | SAMPLE_HAS_DATA
	'\n', '\t'
);
