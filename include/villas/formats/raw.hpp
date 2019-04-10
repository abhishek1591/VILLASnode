/** RAW IO format
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

#include <stdlib.h>

#include <villas/format.hpp>

/* float128 is currently not yet supported as htole128() functions a missing */
#if defined(__GNUC__) && defined(__linux__)
  #define HAS_128BIT
#endif

namespace villas {
namespace node {
namespace formats {

class Raw : public Format {

public:
	enum flags {
		/** Treat the first three values as: sequenceno, seconds, nanoseconds */
		FAKE_HEADER	= (1 << 16),
		BIG_ENDIAN	= (1 << 17),	/**< Encode data in big-endian byte order */

		BITS_8		= (3 << 24),	/**< Pack each value as a byte. */
		BITS_16		= (4 << 24),	/**< Pack each value as a word. */
		BITS_32		= (5 << 24),	/**< Pack each value as a double word. */
		BITS_64		= (6 << 24), 	/**< Pack each value as a quad word. */
	#ifdef HAS_128BIT
		128		= (7 << 24)  /**< Pack each value as a double quad word. */
	#endif
	};

	size_t print(char *buf, size_t len, const struct sample *smps[], unsigned cnt);
	size_t scan(const char *buf, size_t len, struct sample *smps[], unsigned cnt);
};

} // namespace formats
} // namespace node
} // namespace villas
