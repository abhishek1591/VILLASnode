/** Signal meta data.
 *
 * @file
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

#pragma once

#include <jansson.h>

#include <limits>
#include <atomic>
#include <complex>

#include <cstdint>

/* "I" defined by complex.h collides with a define in OpenSSL */
#undef I

/* Forward declarations */
struct vlist;
struct node;
struct mapping_entry;

/** A signal value.
 *
 * Data is in host endianess!
 */
union signal_data {
	double f;		/**< Floating point values. */
	int64_t i;		/**< Integer values. */
	bool b;			/**< Boolean values. */
	std::complex<float> z;	/**< Complex values. */

	signal_data()
	{ }

	static union signal_data nan()
	{
		union signal_data d;

		d.f = std::numeric_limits<double>::quiet_NaN();

		return d;
	}
};

enum class SignalType {
	INVALID	= 0,	/**< Signal type is invalid. */
	FLOAT	= 1,	/**< See signal_data::f */
	INTEGER	= 2,	/**< See signal_data::i */
	BOOLEAN = 3,	/**< See signal_data::b */
	COMPLEX = 4	/**< See signal_data::z */
};

/** Signal descriptor.
 *
 * This data structure contains meta data about samples values in struct sample::data
 */
struct signal {
	char *name;		/**< The name of the signal. */
	char *unit;		/**< The unit of the signal. */

	union signal_data init;	/**< The initial value of the signal. */

	int enabled;

	std::atomic<int> refcnt;	/**< Reference counter. */

	enum SignalType type;
};

/** Initialize a signal with default values. */
int signal_init(struct signal *s);

/** Destroy a signal and release memory. */
int signal_destroy(struct signal *s);

/** Allocate memory for a new signal, and initialize it with provided values. */
struct signal * signal_create(const char *name, const char *unit, enum SignalType fmt);

/** Destroy and release memory of signal. */
int signal_free(struct signal *s);

/** Increase reference counter. */
int signal_incref(struct signal *s);

/** Decrease reference counter. */
int signal_decref(struct signal *s);

/** Copy a signal. */
struct signal * signal_copy(struct signal *s);

/** Parse signal description. */
int signal_parse(struct signal *s, json_t *cfg);

/** Initialize signal from a mapping_entry. */
int signal_init_from_mapping(struct signal *s, const struct mapping_entry *me, unsigned index);

int signal_list_init(struct vlist *list);
int signal_list_destroy(struct vlist *list);
int signal_list_parse(struct vlist *list, json_t *cfg);
int signal_list_generate(struct vlist *list, unsigned len, enum SignalType fmt);
int signal_list_generate2(struct vlist *list, const char *dt);
void signal_list_dump(const struct vlist *list, const union signal_data *data, unsigned len);
int signal_list_copy(struct vlist *dst, const struct vlist *src);

enum SignalType signal_type_from_str(const char *str);

enum SignalType signal_type_from_fmtstr(char c);

const char * signal_type_to_str(enum SignalType fmt);

enum SignalType signal_type_detect(const char *val);

/** Convert signal data from one description/format to another. */
void signal_data_cast(union signal_data *data, const struct signal *from, const struct signal *to);

/** Print value of a signal to a character buffer. */
int signal_data_print_str(const union signal_data *data, const struct signal *sig, char *buf, size_t len);

int signal_data_parse_str(union signal_data *data, const struct signal *sig, const char *ptr, char **end);

int signal_data_parse_json(union signal_data *data, const struct signal *sig, json_t *cfg);

void signal_data_set(union signal_data *data, const struct signal *sig, double val);
