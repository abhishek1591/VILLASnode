/** Print hook.
 *
 * @author Steffen Vogel <stvogel@eonerc.rwth-aachen.de>
 * @copyright 2017, Institute for Automation of Complex Power Systems, EONERC
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

#include <string.h>

#include <villas/hook.h>
#include <villas/plugin.h>
#include <villas/path.h>
#include <villas/sample.h>
#include <villas/io.h>

struct print {
	struct io io;
	struct format_type *format;

	char *prefix;
	char *uri;
};

static int print_init(struct hook *h)
{
	struct print *p = (struct print *) h->_vd;

	p->uri = NULL;
	p->prefix = NULL;
	p->format = format_type_lookup("villas.human");

	return 0;
}

static int print_start(struct hook *h)
{
	struct print *p = (struct print *) h->_vd;
	int ret;

	ret = io_init(&p->io, p->format, h->node, SAMPLE_HAS_ALL);
	if (ret)
		return ret;

	ret = io_open(&p->io, p->uri);
	if (ret)
		return ret;

	return 0;
}

static int print_stop(struct hook *h)
{
	struct print *p = (struct print *) h->_vd;
	int ret;

	ret = io_close(&p->io);
	if (ret)
		return ret;

	return 0;
}

static int print_parse(struct hook *h, json_t *cfg)
{
	struct print *p = (struct print *) h->_vd;
	const char *format = NULL, *prefix = NULL, *uri = NULL;
	int ret;
	json_error_t err;

	ret = json_unpack_ex(cfg, &err, 0, "{ s?: s, s?: s, s?: s }",
		"output", &uri,
		"prefix", &prefix,
		"format", &format
	);
	if (ret)
		jerror(&err, "Failed to parse configuration of hook '%s'", plugin_name(h->_vt));

	if (prefix)
		p->prefix = strdup(prefix);

	if (uri)
		p->uri = strdup(uri);

	if (format) {
		p->format = format_type_lookup(format);
		if (!p->format)
			jerror(&err, "Invalid format '%s'", format);
	}

	return 0;
}

static int print_read(struct hook *h, struct sample *smps[], unsigned *cnt)
{
	struct print *p = (struct print *) h->_vd;

	if (p->prefix)
		printf("%s", p->prefix);
	else if (h->node)
		printf("Node %s read: ", node_name(h->node));

	io_print(&p->io, smps, *cnt);

	return 0;
}

static int print_write(struct hook *h, struct sample *smps[], unsigned *cnt)
{
	struct print *p = (struct print *) h->_vd;

	if (p->prefix)
		printf("%s", p->prefix);
	else if (h->node)
		printf("Node %s write: ", node_name(h->node));

	io_print(&p->io, smps, *cnt);

	return 0;
}

static int print_process(struct hook *h, struct sample *smps[], unsigned *cnt)
{
	struct print *p = (struct print *) h->_vd;

	if (p->prefix)
		printf("%s", p->prefix);
	else if (h->path)
		printf("Path %s process: ", path_name(h->path));

	io_print(&p->io, smps, *cnt);

	return 0;
}

static int print_destroy(struct hook *h)
{
	int ret;
	struct print *p = (struct print *) h->_vd;

	if (p->uri)
		free(p->uri);

	if (p->prefix)
		free(p->prefix);

	ret = io_destroy(&p->io);
	if (ret)
		return ret;

	return 0;
}

static struct plugin p = {
	.name		= "print",
	.description	= "Print the message to stdout",
	.type		= PLUGIN_TYPE_HOOK,
	.hook		= {
		.flags		= HOOK_NODE | HOOK_PATH,
		.priority	= 99,
		.init		= print_init,
		.parse		= print_parse,
		.destroy	= print_destroy,
		.start		= print_start,
		.stop		= print_stop,
		.read   	= print_read,
		.write  	= print_write,
		.process	= print_process,
		.size		= sizeof(struct print)
	}
};

REGISTER_PLUGIN(&p)

/** @} */
