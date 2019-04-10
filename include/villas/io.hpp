/** Stream-based IO for Samples.
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

#include <villas/plugin.hpp>

namespace villas {
namespace node {

class Io {

protected:
	bool header_printed;

	Format *format;

public:
	Io(const char *format)

	Io(Format *f) :
		format(f)
	{ }

	int open(const char *uri);

	int close();

	int print(struct sample *smps[], unsigned cnt);

	int scan(struct sample *smps[], unsigned cnt);

	int eof();

	void rewind();

	int flush();

	int fd();

	FILE * getInputStream();
	FILE * getOutputStream();
};

class StdIo : public Io {

protected:
	FILE *in, *out;

};

class AdvIo : public Io {

protected:
	AFILE *in, *out;

};

} // namespace node
} // namespace villas
