/** Main routine.
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

#include <iostream>

#include <villas/node/config.h>
#include <villas/copyright.hpp>
#include <villas/log.hpp>
#include <villas/version.hpp>
#include <villas/utils.hpp>
#include <villas/super_node.hpp>
#include <villas/node/exceptions.hpp>

using namespace villas;
using namespace villas::node;

static void usage()
{
	std::cout << "Usage: villas-test-config [OPTIONS] CONFIG" << std::endl
	          << "  CONFIG is the path to an optional configuration file" << std::endl
		  << "  OPTIONS is one or more of the following options:" << std::endl
	          << "    -d LVL  set debug level" << std::endl
	          << "    -V        show version and exit" << std::endl
	          << "    -h        show usage and exit" << std::endl << std::endl;

	print_copyright();
}

int main(int argc, char *argv[])
{
	SuperNode sn;
	Logger logger = logging.get("config-test");

	bool check = false;

	int c;
	while ((c = getopt (argc, argv, "hcV")) != -1) {
		switch (c) {
			case 'c':
				check = true;
				break;

			case 'V':
				print_version();
				exit(EXIT_SUCCESS);

			case 'h':
			case '?':
				usage();
				exit(c == '?' ? EXIT_FAILURE : EXIT_SUCCESS);
		}
	}

	if (argc - optind < 1) {
		usage();
		exit(EXIT_FAILURE);
	}

	try {
		sn.parse(argv[optind]);

		if (check)
			sn.check();

		return 0;
	}
	catch (ParseError &e) {
		logger->error("{}", e.what());

		return -1;
	}
	catch (ConfigError &e) {
		logger->error("{}", e.what());

		return -1;
	}
	catch (std::runtime_error &e) {
		logger->error("{}", e.what());

		return -1;
	}
}