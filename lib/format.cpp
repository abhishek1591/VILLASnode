#include <villas/format.hpp>

using namespace villas::node;

Format::Format(struct vlist *sigs, int flgs) :
	signals(sigs),
	flags(flgs),
	state(STATE_INITIALIZED)
{ }

Format::Format(const std::string &dt, int flags) :
	flags(flgs),
	state(STATE_INITIALIZED)
{
	int ret;
	struct vlist *signals;

	signals = (struct vlist *) alloc(sizeof(struct vlist));
	signals->state = STATE_DESTROYED;

	ret = vlist_init(signals);
	if (ret)
		return ret;

	ret = signal_list_generate2(signals, dt);
	if (ret)
		return ret;

	flags |= DESTROY_SIGNALS;

	return io_init(io, fmt, signals, flags);
}

~Format::Format()
{
	if (io->flags & DESTROY_SIGNALS) {
		ret = vlist_destroy(io->signals, (dtor_cb_t) signal_decref, false);
		if (ret)
			return ret;

		free(signals);
	}
}
