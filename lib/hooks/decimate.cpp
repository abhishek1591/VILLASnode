/** Decimate hook.
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

/** @addtogroup hooks Hook functions
 * @{
 */

#include <villas/hooks/decimate.hpp>

namespace villas {
namespace node {

void DecimateHook::start()
{
	assert(state == State::PREPARED);

	counter = 0;

	state = State::STARTED;
}

void DecimateHook::parse(json_t *cfg)
{
	int ret;
	json_error_t err;

	assert(state != State::STARTED);

	ret = json_unpack_ex(cfg, &err, 0, "{ s: i }",
		"ratio", &ratio
	);
	if (ret)
		throw ConfigError(cfg, err, "node-config-hook-decimate");

	state = State::PARSED;
}

Hook::Reason DecimateHook::process(sample *smp)
{
	assert(state == State::STARTED);

	if (ratio && counter++ % ratio != 0)
		return Hook::Reason::SKIP_SAMPLE;

	return Reason::OK;
}

/* Register hook */
static HookPlugin<DecimateHook> p(
	"decimate",
	"Downsamping by integer factor",
	(int) Hook::Flags::NODE_READ | (int) Hook::Flags::NODE_WRITE | (int) Hook::Flags::PATH,
	99
);

} /* namespace node */
} /* namespace villas */

/** @} */
