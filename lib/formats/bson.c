/** Binary Javascript Object Notation.
 *
 * @author Steffen Vogel <stvogel@eonerc.rwth-aachen.de>
 * @copyright 2018, Institute for Automation of Complex Power Systems, EONERC
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

#include <bson.h>

void bson_iter_sample_data(bson_iter_t *iter, struct sample *smp)
{
	if (bson_iter_type(&iter) != BSON_TYPE_DOCUMENT)
		goto out;

	if (!bson_iter_init(&iter2, bson))
		goto out;

	for (int j = 0; j < smp->capacity; j++) {
		if (!bson_iter_next(&iter2)))
			break;

		switch (bson_iter_type(&iter2)) {
			case BSON_TYPE_DOUBLE:
				sample_set_data_format(smps, j, SAMPLE_DATA_FORMAT_FLOAT);
				smp->data[j].f = bson_iter_double(&iter2);
				break;

			case BSON_TYPE_INT32:
				sample_set_data_format(smp, j, SAMPLE_DATA_FORMAT_INT);
				smp->data[j].i = bson_iter_int32(&iter2);
				break;

			case BSON_TYPE_INT64:
				sample_set_data_format(smps, j, SAMPLE_DATA_FORMAT_INT);
				smp->data[j].i = bson_iter_int64(&iter2);
				break;

			default:
				goto out;
		}
	}

	smp->flags |= SAMPLE_HAS_DATA;
}

void bson_append_sample_data(bson_t *bson, const char *key, int key_length, const struct sample *smp)
{
	bson_t bson_arr;

	bson_append_array_begin(bson, key, key_length, &bson_data);

	for (int j = 0; j < smp->length; j++) {
		char str[16];
		const char *key;
		size_t len;

		len = bson_uint32_to_string(j, &key, str, sizeof(str));

		switch (sample_get_data_format(smp, j)) {
			case SAMPLE_DATA_FORMAT_FLOAT:
				bson_append_double(bson_data, key, len, smp->data[j].f);
				break;
			case SAMPLE_DATA_FORMAT_INT:
				bson_append_int32(bson_data, key, len, smp->data[j].i);
				break;
		}
	}

	bson_append_array_end(bson, &bson_data)
}

void bson_append_timespec(bson_t *bson, const char *key, int key_length, struct timespec *ts)
{
	bson_t bson_arr;

	bson_append_array_begin(&bson, key, key_length, &bson_arr);

	bson_append_int32(bson_arr, "0", ts->tv_sec);
	bson_append_int32(bson_arr, "1", ts->tv_nsec);

	bson_append_array_end(&bson, &bson_arr);
}

int bson_sprint(struct io *io, char *buf, size_t len, size_t *wbytes, struct sample *smps[], unsigned cnt)
{
	bson_t bson, *bson_ts, *bson_data;
	bson_init(&bson);

	for (int i = 0; i < cnt; i++) {
		bson_append_document_begin(&bson, "ts", 2, &bson_ts);

		if (smps[i]->flags & SAMPLE_HAS_TS_ORIGIN)
			bson_append_timespec(bson_ts, "origin", 6, &smps[i]->ts.origin);

		if (smps[i]->flags & SAMPLE_HAS_TS_RECEIVED)
			bson_append_timespec(bson_ts, "received", 8, &smps[i]->ts.received);

		bson_append_document_end(&bson, &bson_ts);

		if (smps[i]->flags & SAMPLE_HAS_SEQUENCE)
			bson_append_int64(&bson, "sequence", 8, smps[i]->sequence);

		bson_append_sample_data(&bson, "data", 4, smps[i]);
	}
}

int bson_sscan(struct io *io, char *buf, size_t len, size_t *rbytes, struct sample *smps[], unsigned cnt)
{
	bson_reader_t *reader;
	bson_iter_t iter, iter2;
	bson_t *bson;

	int i;
	bool eof;

	reader = bson_reader_new_from_data((const uint8_t *) buf, len);

	for (i = 0; i < cnt; i++) {
		if (!(bson = bson_reader_read(reader, &eof))
			goto out;

		if (!bson_iter_init(&iter, bson))
			continue;

   		while (bson_iter_next(&iter)) {
     			if      (strcmp(bson_iter_key(&iter), "sequence")) {
				if (bson_iter_type(&iter) != BSON_TYPE_INT64)
					goto out;

				smps[i]->sequence = bson_iter_int64(&iter);
				smps[i]->flags |= SAMPLE_HAS_SEQUENCE;
			}
			else if (strcmp(bson_iter_key(&iter), "ts")) {
				if (bson_iter_type(&iter) != BSON_TYPE_DOCUMENT)
					goto out;

				if (!bson_iter_init(&iter2, bson))
					goto out;

				while (bson_iter_next(&iter2))) {
					if      (strcmp(bson_iter_key(&iter2), "origin")) {

					}
					else if (strcmp(bson_iter_key(&iter2), "received")) {

					}
				}

			}
			else if (strcmp(bson_iter_key(&iter), "data")) {
				bson_iter_sample_data(&iter, smps[i]);
			}
		}
	}

out:	bson_reader_destroy(reader);

	if (!eof)
		return -1;

	return i;
}

static struct plugin p = {
	.name = "bson",
	.description = "Binary Javascript Object Notation",
	.type = PLUGIN_TYPE_FORMAT,
	.io = {
		.sprint	= bson_sprint,
		.sscan	= bson_sscan,
		.size	= 0,
		.flags	= FORMAT_TYPE_BINARY
	}
};

REGISTER_PLUGIN(&p);
