/** JSON serializtion of sample data.
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

#include <villas/sample.h>
#include <villas/compat.h>
#include <villas/signal.h>
#include <villas/formats/json.hpp>

using namespace villas::node::formats;

enum signal_type Json::detectFormat(json_t *val)
{
	int type = json_typeof(val);

	switch (type) {
		case JSON_REAL:
			return SIGNAL_TYPE_FLOAT;

		case JSON_INTEGER:
			return SIGNAL_TYPE_INTEGER;

		case JSON_TRUE:
		case JSON_FALSE:
			return SIGNAL_TYPE_BOOLEAN;

		case JSON_OBJECT:
			return SIGNAL_TYPE_COMPLEX; /* must be a complex number */

		default:
			return SIGNAL_TYPE_INVALID;
	}
}

json_t * Json::packTimestamps(struct sample *smp)
{
	json_t *json_ts = json_object();

#ifdef __arm__ // TODO: check
	const char *fmt = "[ i, i ]";
#else
	const char *fmt = "[ I, I ]";
#endif

	if (flags & SAMPLE_HAS_TS_ORIGIN) {
		if (smp->flags & SAMPLE_HAS_TS_ORIGIN)
			json_object_set(json_ts, "origin", json_pack(fmt, smp->ts.origin.tv_sec, smp->ts.origin.tv_nsec));
	}

	if (flags & SAMPLE_HAS_TS_RECEIVED) {
		if (smp->flags & SAMPLE_HAS_TS_RECEIVED)
			json_object_set(json_ts, "received", json_pack(fmt, smp->ts.received.tv_sec, smp->ts.received.tv_nsec));
	}

	return json_ts;
}

int Json::unpackTimestamps(json_t *json_ts, struct sample *smp)
{
	int ret;
	json_error_t err;
	json_t *json_ts_origin = nullptr;
	json_t *json_ts_received = nullptr;

	json_unpack_ex(json_ts, &err, 0, "{ s?: o, s?: o }",
		"origin",   &json_ts_origin,
		"received", &json_ts_received
	);

	if (json_ts_origin) {
		ret = json_unpack_ex(json_ts_origin, &err, 0, "[ I, I ]", &smp->ts.origin.tv_sec, &smp->ts.origin.tv_nsec);
		if (ret)
			return ret;

		smp->flags |= SAMPLE_HAS_TS_ORIGIN;
	}

	if (json_ts_received) {
		ret = json_unpack_ex(json_ts_received, &err, 0, "[ I, I ]", &smp->ts.received.tv_sec, &smp->ts.received.tv_nsec);
		if (ret)
			return ret;

		smp->flags |= SAMPLE_HAS_TS_RECEIVED;
	}

	return 0;
}

int Json::packSample(json_t **j, struct sample *smp)
{
	json_t *json_smp;
	json_error_t err;

	json_smp = json_pack_ex(&err, 0, "{ s: o }", "ts", packTimestamps(smp));

	if (flags & SAMPLE_HAS_SEQUENCE) {
		if (smp->flags & SAMPLE_HAS_SEQUENCE) {
			json_t *json_sequence = json_integer(smp->sequence);

			json_object_set(json_smp, "sequence", json_sequence);
		}
	}

	if (flags & SAMPLE_HAS_DATA) {
		json_t *json_data = json_array();

		for (unsigned i = 0; i < smp->length; i++) {
			enum signal_type fmt = sample_format(smp, i);

			json_t *json_value;
			switch (fmt) {
				case SIGNAL_TYPE_INTEGER:
					json_value = json_integer(smp->data[i].i);
					break;

				case SIGNAL_TYPE_FLOAT:
					json_value = json_real(smp->data[i].f);
					break;

				case SIGNAL_TYPE_BOOLEAN:
					json_value = json_boolean(smp->data[i].b);
					break;

				case SIGNAL_TYPE_COMPLEX:
					json_value = json_pack("{ s: f, s: f }",
						"real", creal(smp->data[i].z),
						"imag", cimag(smp->data[i].z)
					);
					break;

				case SIGNAL_TYPE_INVALID:
					return -1;
			}

			json_array_append(json_data, json_value);
		}

		json_object_set(json_smp, "data", json_data);
	}

	*j = json_smp;

	return 0;
}

int Json::packSamples(json_t **j, struct sample *smps[], unsigned cnt)
{
	int ret;
	json_t *json_smps = json_array();

	for (unsigned i = 0; i < cnt; i++) {
		json_t *json_smp;

		ret = packSample(&json_smp, smps[i]);
		if (ret)
			break;

		json_array_append(json_smps, json_smp);
	}

	*j = json_smps;

	return cnt;
}

int Json::unpackSample(json_t *json_smp, struct sample *smp)
{
	int ret;
	json_error_t err;
	json_t *json_data, *json_value, *json_ts = nullptr;
	size_t i;
	int64_t sequence = -1;

	smp->signals = signals;

	ret = json_unpack_ex(json_smp, &err, 0, "{ s?: o, s?: I, s: o }",
		"ts", &json_ts,
		"sequence", &sequence,
		"data", &json_data);
	if (ret)
		return ret;

	smp->flags = 0;
	smp->length = 0;

	if (json_ts) {
		ret = unpackTimestamps(json_ts, smp);
		if (ret)
			return ret;
	}

	if (!json_is_array(json_data))
		return -1;

	if (sequence >= 0) {
		smp->sequence = sequence;
		smp->flags |= SAMPLE_HAS_SEQUENCE;
	}

	json_array_foreach(json_data, i, json_value) {
		if (i >= smp->capacity)
			break;

		struct signal *sig = (struct signal *) vlist_at_safe(smp->signals, i);
		if (!sig)
			return -1;

		enum signal_type fmt = json_detect_format(json_value);
		if (sig->type != fmt) {
			error("Received invalid data type in JSON payload: Received %s, expected %s for signal %s (index %zu).",
				signal_type_to_str(fmt), signal_type_to_str(sig->type), sig->name, i);
			return -2;
		}

		ret = signal_data_parse_json(&smp->data[i], sig, json_value);
		if (ret)
			return -3;

		smp->length++;
	}

	if (smp->length > 0)
		smp->flags |= SAMPLE_HAS_DATA;

	return 0;
}

int Json::unpackSamples(json_t *json_smps, struct sample *smps[], unsigned cnt)
{
	int ret;
	json_t *json_smp;
	size_t i;

	if (!json_is_array(json_smps))
		return -1;

	json_array_foreach(json_smps, i, json_smp) {
		if (i >= cnt)
			break;

		ret = unpackSample(json_smp, smps[i]);
		if (ret < 0)
			break;
	}

	return i;
}

int Json::print(char *buf, size_t len, size_t *wbytes, struct sample *smps[], unsigned cnt)
{
	int ret;
	json_t *json;
	size_t wr;

	ret = packSamples(&json, smps, cnt);
	if (ret < 0)
		return ret;

	wr = json_dumpb(json, buf, len, 0);

	json_decref(json);

	if (wbytes)
		*wbytes = wr;

	return ret;
}

int Json::scan(const char *buf, size_t len, size_t *rbytes, struct sample *smps[], unsigned cnt)
{
	int ret;
	json_t *json;
	json_error_t err;

	json = json_loadb(buf, len, 0, &err);
	if (!json)
		return -1;

	ret = unpackSamples(json, smps, cnt);

	json_decref(json);

	if (ret < 0)
		return ret;

	if (rbytes)
		*rbytes = err.position;

	return ret;
}

/* Register plugin */
static FormatPlugin<Json> p(
	"json",
	"Javascript Object Notation",
	SAMPLE_HAS_TS_ORIGIN | SAMPLE_HAS_SEQUENCE | SAMPLE_HAS_DATA,
	'\n'
);
