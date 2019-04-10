/** Comma-separated values.
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

#include <ctype.h>
#include <inttypes.h>
#include <string.h>

#include <villas/formats/csv.hpp>
#include <villas/sample.h>
#include <villas/signal.h>
#include <villas/timing.h>

using namespace villas::node::formats;

size_t Csv::printSingle(char *buf, size_t len, const struct sample *smp)
{
	size_t off = 0;
	struct signal *sig;

	if (flags & SAMPLE_HAS_TS_ORIGIN) {
		if ((smp->flags & SAMPLE_HAS_TS_RECEIVED) && (smp->flags & SAMPLE_HAS_TS_RECEIVED))
			off += snprintf(buf + off, len - off, "%ld%c%09ld", smp->ts.origin.tv_sec, separator, smp->ts.origin.tv_nsec);
		else
			off += snprintf(buf + off, len - off, "nan%cnan", separator);
	}

	if (flags & SAMPLE_HAS_OFFSET) {
		if (smp->flags & SAMPLE_HAS_TS_RECEIVED)
			off += snprintf(buf + off, len - off, "%c%.09f", separator, time_delta(&smp->ts.origin, &smp->ts.received));
		else
			off += snprintf(buf + off, len - off, "%cnan", separator);
	}

	if (flags & SAMPLE_HAS_SEQUENCE) {
		if (smp->flags & SAMPLE_HAS_SEQUENCE)
			off += snprintf(buf + off, len - off, "%c%" PRIu64, separator, smp->sequence);
		else
			off += snprintf(buf + off, len - off, "%cnan", separator);
	}

	if (flags & SAMPLE_HAS_DATA) {
		for (unsigned i = 0; i < smp->length; i++) {
			sig = (struct signal *) vlist_at_safe(smp->signals, i);
			if (!sig)
				break;

			off += snprintf(buf + off, len - off, "%c", separator);
			off += signal_data_snprint(&smp->data[i], sig, buf + off, len - off);
		}
	}

	off += snprintf(buf + off, len - off, "%c", delimiter);

	return off;
}

size_t Csv::scanSingle(const char *buf, size_t len, struct sample *smp)
{
	int ret;
	unsigned i = 0;
	const char *ptr = buf;
	char *end;

	double offset __attribute__((unused));

	smp->flags = 0;
	smp->signals = signals;

	smp->ts.origin.tv_sec = strtoul(ptr, &end, 10);
	if (end == ptr || *end == delimiter)
		goto out;

	ptr = end + 1;

	smp->ts.origin.tv_nsec = strtoul(ptr, &end, 10);
	if (end == ptr || *end == delimiter)
		goto out;

	ptr = end + 1;

	smp->flags |= SAMPLE_HAS_TS_ORIGIN;

	offset = strtof(ptr, &end);
	if (end == ptr || *end == delimiter)
		goto out;

	ptr = end + 1;

	smp->sequence = strtoul(ptr, &end, 10);
	if (end == ptr || *end == delimiter)
		goto out;

	smp->flags |= SAMPLE_HAS_SEQUENCE;

	for (ptr = end + 1, i = 0; i < smp->capacity; ptr = end + 1, i++) {
		if (*end == delimiter)
			goto out;

		struct signal *sig = (struct signal *) vlist_at_safe(smp->signals, i);
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

	return end - buf;
}

int Csv::print(char *buf, size_t len, size_t *wbytes, struct sample *smps[], unsigned cnt)
{
	unsigned i;
	size_t off = 0;

	for (i = 0; i < cnt && off < len; i++)
		off += csv_sprint_single(io, buf + off, len - off, smps[i]);

	if (wbytes)
		*wbytes = off;

	return i;
}

int Csv::scan(const char *buf, size_t len, size_t *rbytes, struct sample *smps[], unsigned cnt)
{
	unsigned i;
	size_t off = 0;

	for (i = 0; i < cnt && off < len; i++)
		off += csv_sscan_single(io, buf + off, len - off, smps[i]);

	if (rbytes)
		*rbytes = off;

	return i;
}

void Csv::header(char *buf, size_t len, size_t *wbytes)
{
	size_t pos = 0;

	pos += snprintf(buf + pos, len - pos, f, "# ");
	if (flags & SAMPLE_HAS_TS_ORIGIN)
		pos += snprintf(buf + pos, len - pos, "secs%cnsecs%c", separator, separator);

	if (flags & SAMPLE_HAS_OFFSET)
		pos += snprintf(buf + pos, len - pos, "offset%c", separator);

	if (flags & SAMPLE_HAS_SEQUENCE)
		pos += snprintf(buf + pos, len - pos, "sequence%c", separator);

	if (flags & SAMPLE_HAS_DATA) {
		for (unsigned i = 0; i < MIN(smp->length, vlist_length(signals)); i++) {
			struct signal *sig = (struct signal *) vlist_at(signals, i);

			if (sig->name)
				pos += snprintf(buf + pos, len - pos, "%s", sig->name);
			else
				pos += snprintf(buf + pos, len - pos, "signal%d", i);

			if (sig->unit)
				pos += snprintf(buf + pos, len - pos, "[%s]", sig->unit);

			if (i + 1 < smp->length)
				pos += snprintf(buf + pos, len - pos, "%c", separator);
		}
	}

	pos += snprintf(buf + pos, len - pos, "%c", delimiter);

	if (wbytes)
		*wbytes = pos;
}

/* Register plugin */
static FormatPlugin<CSV> tsv(
	"tsv",
	"Tabulator-separated values",
	IO_NEWLINES | SAMPLE_HAS_TS_ORIGIN | SAMPLE_HAS_SEQUENCE | SAMPLE_HAS_DATA,
	'\n', '\t'
);

static FormatPlugin<CSV> csv(
	"csv",
	"Comma-separated values",
	IO_NEWLINES | SAMPLE_HAS_TS_ORIGIN | SAMPLE_HAS_SEQUENCE | SAMPLE_HAS_DATA,
	'\n', ','
);
