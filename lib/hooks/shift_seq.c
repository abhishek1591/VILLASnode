/** Shift sequence number of samples
 *
 * @author Steffen Vogel <stvogel@eonerc.rwth-aachen.de>
 * @copyright 2017, Institute for Automation of Complex Power Systems, EONERC
 *********************************************************************************/

/** @addtogroup hooks Hook functions
 * @{
 */

#include "hook.h"
#include "plugin.h"

struct shift {
	int offset;
};

static int shift_seq_parse(struct hook *h, config_setting_t *cfg)
{
	struct shift *p = h->_vd;
	
	if (!config_setting_lookup_int(cfg, "offset", &p->offset))
		cerror(cfg, "Missing setting 'offset' for hook '%s'", plugin_name(h->_vt));
	
	return 0;
}

static int shift_seq_read(struct hook *h, struct sample *smps[], size_t *cnt)
{
	struct shift *p = h->_vd;

	for (int i = 0; i < *cnt; i++)
		smps[i]->sequence += p->offset;

	return 0;
}

static struct plugin p = {
	.name		= "shift_seq",
	.description	= "Shift sequence number of samples",
	.type		= PLUGIN_TYPE_HOOK,
	.hook		= {
		.priority = 99,
		.parse	= shift_seq_parse,
		.read	= shift_seq_read,
		.size	= sizeof(struct shift),
	}
};

REGISTER_PLUGIN(&p)

/** @} */