/** Hook-releated functions.
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

#include <villas/plugin.hpp>
#include <villas/hook.hpp>
#include <villas/hook_list.hpp>
#include <villas/list.h>
#include <villas/log.h>

using namespace villas;
using namespace villas::node;

int hook_list_init(vlist *hs)
{
	int ret;

	ret = vlist_init(hs);
	if (ret)
		return ret;

	return 0;
}

static int hook_destroy(Hook *h)
{
	delete h;

	return 0;
}

int hook_list_destroy(vlist *hs)
{
	int ret;

	ret = vlist_destroy(hs, (dtor_cb_t) hook_destroy, false);
	if (ret)
		return ret;

	return 0;
}

void hook_list_parse(vlist *hs, json_t *cfg, int mask, struct path *o, struct node *n)
{
	if (!json_is_array(cfg))
		throw ConfigError(cfg, "node-config-hook", "Hooks must be configured as a list of hook objects");

	size_t i;
	json_t *json_hook;
	json_array_foreach(cfg, i, json_hook) {
		int ret;
		const char *type;
		Hook *h;
		json_error_t err;

		ret = json_unpack_ex(json_hook, &err, 0, "{ s: s }", "type", &type);
		if (ret)
			throw ConfigError(json_hook, err, "node-config-hook", "Failed to parse hook");

		auto hf = plugin::Registry::lookup<HookFactory>(type);
		if (!hf)
			throw ConfigError(json_hook, "node-config-hook", "Unkown hook type '{}'", type);

		if (!(hf->getFlags() & mask))
			throw ConfigError(json_hook, "node-config-hook", "Hook '{}' not allowed here", type);

		h = hf->make(o, n);
		h->parse(json_hook);
		h->check();

		vlist_push(hs, h);
	}
}

static int hook_cmp_priority(const Hook *a, const Hook *b)
{
	return a->getPriority() - b->getPriority();
}

static int hook_is_enabled(const Hook *h)
{
	return h->isEnabled() ? 0 : -1;
}

void hook_list_prepare(vlist *hs, vlist *sigs, int m, struct path *p, struct node *n)
{
	assert(hs->state == State::INITIALIZED);

	if (!m)
		goto skip_add;

	/* Add internal hooks if they are not already in the list */
	for (auto f : plugin::Registry::lookup<HookFactory>()) {
		if ((f->getFlags() & m) == m) {
			auto h = f->make(p, n);

			vlist_push(hs, h);
		}
	}

skip_add:
	/* Remove filters which are not enabled */
	vlist_filter(hs, (dtor_cb_t) hook_is_enabled);

	/* We sort the hooks according to their priority */
	vlist_sort(hs, (cmp_cb_t) hook_cmp_priority);

	for (size_t i = 0; i < vlist_length(hs); i++) {
		Hook *h = (Hook *) vlist_at(hs, i);

		h->prepare(sigs);

		sigs = h->getSignals();
	}
}

int hook_list_process(vlist *hs, sample *smps[], unsigned cnt)
{
	unsigned current, processed = 0;

	if (vlist_length(hs) == 0)
		return cnt;

	for (current = 0; current < cnt; current++) {
		sample *smp = smps[current];

		for (size_t i = 0; i < vlist_length(hs); i++) {
			Hook *h = (Hook *) vlist_at(hs, i);

			auto ret = h->process(smp);
			switch (ret) {
				case Hook::Reason::ERROR:
					return -1;

				case Hook::Reason::OK:
					continue;

				case Hook::Reason::SKIP_SAMPLE:
					goto skip;

				case Hook::Reason::STOP_PROCESSING:
					goto stop;
			}
		}

		smps[processed++] = smp;
skip: {}
	}

stop:	return processed;
}

void hook_list_periodic(vlist *hs)
{
	for (size_t j = 0; j < vlist_length(hs); j++) {
		Hook *h = (Hook *) vlist_at(hs, j);

		h->periodic();
	}
}

void hook_list_start(vlist *hs)
{
	for (size_t i = 0; i < vlist_length(hs); i++) {
		Hook *h = (Hook *) vlist_at(hs, i);

		h->start();
	}
}

void hook_list_stop(vlist *hs)
{
	for (size_t i = 0; i < vlist_length(hs); i++) {
		Hook *h = (Hook *) vlist_at(hs, i);

		h->stop();
	}
}

vlist * hook_list_get_signals(vlist *hs)
{
	Hook *h = (Hook *) vlist_last(hs);

	return h->getSignals();
}
