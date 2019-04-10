/** Format plugins.
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

class Format {

public:
	enum Flags {
		/* Bits 0-7 are reserved for for flags defined by enum sample_flags */
		FLUSH			= (1 << 8),	/**< Flush the output stream after each chunk of samples. */
		NONBLOCK		= (1 << 9),	/**< Dont block read() while waiting for new samples. */
		NEWLINES		= (1 << 10),	/**< The samples of this format are newline delimited. */
		DESTROY_SIGNALS		= (1 << 11),	/**< Signal descriptors are managed by this IO instance. Destroy them in destoy() */
		HAS_BINARY_PAYLOAD	= (1 << 12)	/**< This IO instance en/decodes binary payloads. */
	};

	enum Mode {
		STDIO,
		ADVIO,
		CUSTOM
	};

protected:
	enum state state;

	int flags;

	struct Direction {
		/** A format type can use this file handle or overwrite the
		 * format::{open,close,eof,rewind} functions and the private
		 * data in io::_vd.
		 */
		union {
			FILE *std;
			AFILE *adv;
		} stream;

		char *buffer;
		size_t buflen;
	} in, out;

	struct vlist *signals;			/**< Signal meta data for parsed samples by scan() */
	bool header_printed;

	enum Mode mode;

	void *_vd;
	const struct format_type *_vt;

public:
	int Format(struct vlist *signals, int flags);

	int Format(const char *dt, int flags);

	virtual ~Format()
	{ }

	int check();

	int open(const char *uri);

	int close();

	void header(const struct sample *smp);

	void footer();

	int print(struct sample *smps[], unsigned cnt);

	int scan(struct sample *smps[], unsigned cnt);

	int eof();

	void rewind();

	int flush();

	int fd();

	int streamOpen(const char *uri);

	int streamClose();

	int streamEof();

	void streamRewind();

	int streamFd();

	int streamFlush();

	FILE * streamInput();
	FILE * streamOutput();

	/** Parse samples from the buffer \p buf with a length of \p len bytes.
	 *
	 * @param buf[in]	The buffer of data which should be parsed / de-serialized.
	 * @param len[in]	The length of the buffer \p buf.
	 * @param rbytes[out]	The number of bytes which have been read from \p buf.
	 * @param smps[out]	The array of pointers to samples.
	 * @param cnt[in]	The number of pointers in the array \p smps.
	 *
	 * @retval >=0		The number of samples which have been parsed from \p buf and written into \p smps.
	 * @retval <0		Something went wrong.
	 */
	int sscan(const char *buf, size_t len, size_t *rbytes, struct sample *smps[], unsigned cnt);

	/** Print \p cnt samples from \p smps into buffer \p buf of length \p len.
	 *
	 * @param buf[out]	The buffer which should be filled with serialized data.
	 * @param len[in]	The length of the buffer \p buf.
	 * @param rbytes[out]	The number of bytes which have been written to \p buf. Ignored if NULL.
	 * @param smps[in]	The array of pointers to samples.
	 * @param cnt[in]	The number of pointers in the array \p smps.
	 *
	 * @retval >=0		The number of samples from \p smps which have been written into \p buf.
	 * @retval <0		Something went wrong.
	 */
	int sprint(char *buf, size_t len, size_t *wbytes, struct sample *smps[], unsigned cnt);
};

class LineFormat : public Format {

protected:
	char delimiter;				/**< Newline delimiter. */
	char separator;				/**< Column separator (used by csv and villas.human formats only) */

public:
	/** Print a header. */
	virtual void header(const struct sample *smp);

	/** Print a footer. */
	virtual void footer();

};

class FormatFactory : public plugin::Plugin {

public:
	using plugin::Plugin::Plugin;

	virtual Action * make(Session *s) = 0;
};

template<typename T>
class FormatPlugin : public FormatFactory {

public:
	using FormatFactory::FormatFactory;

	virtual Action * make(Session *s) {
		return new T(s);
	};
};

} // node
} // villas
