/** Node-type for shared memory communication.
 *
 * @file
 * @author Georg Martin Reinke <georg.reinke@rwth-aachen.de>
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

#include <fcntl.h>
#include <pthread.h>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <villas/kernel/kernel.h>
#include <villas/log.h>
#include <villas/shmem.h>
#include <villas/nodes/shmem.hpp>
#include <villas/plugin.h>
#include <villas/timing.h>
#include <villas/utils.hpp>

using namespace villas::utils;

int shmem_parse(struct node *n, json_t *cfg)
{
	struct shmem *shm = (struct shmem *) n->_vd;
	const char *val, *mode_str = nullptr;

	int ret;
	json_t *json_exec = nullptr;
	json_error_t err;

	int len = MAX(vlist_length(&n->in.signals), vlist_length(&n->out.signals));

	/* Default values */
	shm->conf.queuelen = MAX(DEFAULT_SHMEM_QUEUELEN, n->in.vectorize);
	shm->conf.samplelen = len;
	shm->conf.polling = false;
	shm->exec = nullptr;

	ret = json_unpack_ex(cfg, &err, 0, "{ s: { s: s }, s: { s: s }, s?: i, s?: o, s?: s }",
		"out",
			"name", &shm->out_name,
		"in",
			"name", &shm->in_name,
		"queuelen", &shm->conf.queuelen,
		"exec", &json_exec,
		"mode", &mode_str
	);
	if (ret)
		jerror(&err, "Failed to parse configuration of node %s", node_name(n));

	if (mode_str) {
		if (!strcmp(mode_str, "polling"))
			shm->conf.polling = true;
		else if (!strcmp(mode_str, "pthread"))
			shm->conf.polling = false;
		else
			error("Unknown mode '%s' in node %s", mode_str, node_name(n));
	}

	if (json_exec) {
		if (!json_is_array(json_exec))
			error("Setting 'exec' of node %s must be an array of strings", node_name(n));

		shm->exec = (char **) alloc(sizeof(char *) * (json_array_size(json_exec) + 1));

		size_t i;
		json_t *json_val;
		json_array_foreach(json_exec, i, json_val) {
			val = json_string_value(json_val);
			if (!val)
				error("Setting 'exec' of node %s must be an array of strings", node_name(n));

			shm->exec[i] = strdup(val);
		}

		shm->exec[i] = nullptr;
	}

	return 0;
}

int shmem_start(struct node *n)
{
	struct shmem *shm = (struct shmem *) n->_vd;
	int ret;

	if (shm->exec) {
		ret = spawn(shm->exec[0], shm->exec);
		if (!ret)
			serror("Failed to spawn external program");

		sleep(1);
	}

	ret = shmem_int_open(shm->out_name, shm->in_name, &shm->intf, &shm->conf);
	if (ret < 0)
		serror("Opening shared memory interface failed (%d)", ret);

	return 0;
}

int shmem_stop(struct node *n)
{
	struct shmem* shm = (struct shmem *) n->_vd;

	return shmem_int_close(&shm->intf);
}

int shmem_read(struct node *n, struct sample *smps[], unsigned cnt, unsigned *release)
{
	struct shmem *shm = (struct shmem *) n->_vd;
	int recv;
	struct sample *shared_smps[cnt];

	do {
		recv = shmem_int_read(&shm->intf, shared_smps, cnt);
	} while (recv == 0);

	if (recv < 0) {
		/* This can only really mean that the other process has exited, so close
		 * the interface to make sure the shared memory object is unlinked */

		info("Shared memory segment has been closed.");

		n->state = State::STOPPING;

		return recv;
	}

	sample_copy_many(smps, shared_smps, recv);
	sample_decref_many(shared_smps, recv);

	/** @todo: signal descriptions are currently not shared between processes */
	for (int i = 0; i < recv; i++)
		smps[i]->signals = &n->in.signals;

	return recv;
}

int shmem_write(struct node *n, struct sample *smps[], unsigned cnt, unsigned *release)
{
	struct shmem *shm = (struct shmem *) n->_vd;
	struct sample *shared_smps[cnt]; /* Samples need to be copied to the shared pool first */
	int avail, pushed, copied;

	avail = sample_alloc_many(&shm->intf.write.shared->pool, shared_smps, cnt);
	if (avail != (int) cnt)
		warning("Pool underrun for shmem node %s", shm->out_name);

	copied = sample_copy_many(shared_smps, smps, avail);
	if (copied < avail)
		warning("Outgoing pool underrun for node %s", node_name(n));

	pushed = shmem_int_write(&shm->intf, shared_smps, copied);
	if (pushed != avail)
		warning("Outgoing queue overrun for node %s", node_name(n));

	return pushed;
}

char * shmem_print(struct node *n)
{
	struct shmem *shm = (struct shmem *) n->_vd;
	char *buf = nullptr;

	strcatf(&buf, "out_name=%s, in_name=%s, queuelen=%d, polling=%s",
		shm->out_name, shm->in_name, shm->conf.queuelen, shm->conf.polling ? "yes" : "no");

	if (shm->exec) {
		strcatf(&buf, ", exec='");

		for (int i = 0; shm->exec[i]; i++)
			strcatf(&buf, shm->exec[i+1] ? "%s " : "%s", shm->exec[i]);

		strcatf(&buf, "'");
	}

	return buf;
}

static struct plugin p;

__attribute__((constructor(110)))
static void register_plugin() {
	if (plugins.state == State::DESTROYED)
		vlist_init(&plugins);

	p.name			= "shmem";
	p.description		= "POSIX shared memory interface with external processes";
	p.type			= PluginType::NODE;
	p.node.instances.state	= State::DESTROYED;
	p.node.vectorize	= 0;
	p.node.size		= sizeof(struct shmem);
	p.node.parse		= shmem_parse;
	p.node.print		= shmem_print;
	p.node.start		= shmem_start;
	p.node.stop		= shmem_stop;
	p.node.read		= shmem_read;
	p.node.write		= shmem_write;


	vlist_init(&p.node.instances);
	vlist_push(&plugins, &p);
}

__attribute__((destructor(110)))
static void deregister_plugin() {
	if (plugins.state != State::DESTROYED)
		vlist_remove_all(&plugins, &p);
}
