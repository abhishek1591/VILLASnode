/** JSON serializtion for RESERVE project.
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

#include <villas/sample.h>
#include <villas/node.h>
#include <villas/signal.h>
#include <villas/compat.h>
#include <villas/timing.h>
#include <villas/formats/json_reserve.hpp>

using namespace villas::node::formats;

int JsonReserve::packSample(json_t **j, struct sample *smp)
{
	json_error_t err;
	json_t *json_data, *json_name, *json_unit, *json_value;
	json_t *json_created = nullptr;
	json_t *json_sequence = nullptr;

	if (smp->flags & SAMPLE_HAS_TS_ORIGIN)
		json_created = json_integer(time_to_double(&smp->ts.origin) * 1e3);

	if (smp->flags & SAMPLE_HAS_SEQUENCE)
		json_sequence = json_integer(smp->sequence);

	json_data = json_array();

	for (unsigned i = 0; i < smp->length; i++) {
		struct signal *sig;

		sig = (struct signal *) vlist_at_safe(smp->signals, i);
		if (!sig)
			return -1;

		if (sig->name)
			json_name = json_string(sig->name);
		else {
			char name[32];
			snprintf(name, 32, "signal_%d", i);

			json_name = json_string(name);
		}

		if (sig->unit)
			json_unit = json_string(sig->unit);
		else
			json_unit = nullptr;

		json_value = json_pack_ex(&err, 0, "{ s: o, s: f }",
			"name", json_name,
			"value", smp->data[i].f
		);
		if (!json_value)
			continue;

		if (json_unit)
			json_object_set(json_value, "unit", json_unit);
		if (json_created)
			json_object_set(json_value, "created", json_created);
		if (json_sequence)
			json_object_set(json_value, "sequence", json_sequence);

		json_array_append(json_data, json_value);
	}

	if (json_created)
		json_decref(json_created);
	if (json_sequence)
		json_decref(json_sequence);

	*j = json_pack_ex(&err, 0, "{ s: o }",
		"measurements", json_data
	);
	if (*j == nullptr)
		return -1;

	return 0;
}

int JsonReserve::unpackSample(json_t *j, struct sample *smp)
{
	int ret, idx;
	double created = -1;
	json_error_t err;
	json_t *json_value, *json_data = nullptr;
	json_t *json_origin = nullptr, *json_target = nullptr;
	size_t i;

	ret = json_unpack_ex(j, &err, 0, "{ s?: o, s?: o, s?: o, s?: o }",
		"origin", &json_origin,
		"target", &json_target,
		"measurements", &json_data,
		"setpoints", &json_data
	);
	if (ret)
		return -1;

	if (!json_data || !json_is_array(json_data))
		return -1;

	smp->flags = 0;
	smp->length = 0;

	json_array_foreach(json_data, i, json_value) {
		const char *name, *unit = nullptr;
		double value;

		ret = json_unpack_ex(json_value, &err, 0, "{ s: s, s?: s, s: F, s?: F }",
			"name", &name,
			"unit", &unit,
			"value", &value,
			"created", &created
		);
		if (ret)
			return -1;

		struct signal *sig;

		sig = (struct signal *) vlist_lookup(signals, name);
		if (sig) {
			if (!sig->enabled)
				continue;

			idx = vlist_index(signals, sig);
		}
		else {
			ret = sscanf(name, "signal_%d", &idx);
			if (ret != 1)
				continue;
		}

		if (idx < 0)
			return -1;

		if (idx < (int) smp->capacity) {
			smp->data[idx].f = value;

			if (idx >= (int) smp->length)
				smp->length = idx + 1;
		}
	}

	if (smp->length > 0)
		smp->flags |= SAMPLE_HAS_DATA;

	if (created > 0) {
		smp->ts.origin = time_from_double(created * 1e-3);
		smp->flags |= SAMPLE_HAS_TS_ORIGIN;
	}

	return smp->length > 0 ? 1 : 0;
}

/* Register plugin */
static FormatPlugin<JsonReserve> p(
	"json.reserve",
	"RESERVE JSON format"
);
