/** Message related functions
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

#include <villas/format.hpp>

namespace villas {
namespace node {
namespace formats {

class VillasBinary : public Format {

public:
	enum flags {
		WEB	= (1 << 16) /**< Use webmsg format (everying little endian) */
	};

	size_t print(char *buf, size_t len, const struct sample *smps[], unsigned cnt);
	size_t scan(const char *buf, size_t len, struct sample *smps[], unsigned cnt);
};

} // namespace formats
} // namespace node
} // namespace villas