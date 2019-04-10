/** Reading and writing simulation samples in various formats.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include <villas/io.hpp>
#include <villas/format.hpp>
#include <villas/utils.hpp>
#include <villas/sample.h>

int Io::printLines(struct sample *smps[], unsigned cnt)
{
	int ret;
	unsigned i;

	FILE *f = getOutoutStream();

	for (i = 0; i < cnt; i++) {
		size_t wbytes;

		ret = format->print(out.buffer, out.buflen, &wbytes, &smps[i], 1);
		if (ret < 0)
			return ret;

		fwrite(out.buffer, wbytes, 1, f);
	}

	return i;
}

int Io::scanLines(struct sample *smps[], unsigned cnt)
{
	int ret;
	unsigned i;

	FILE *f = getInputStream();

	for (i = 0; i < cnt; i++) {
		size_t rbytes;
		ssize_t bytes;
		char *ptr;

skip:		bytes = getdelim(&in.buffer, &in.buflen, format->getDelimiter(), f);
		if (bytes < 0)
			return -1; /* An error or eof occured */

		/* Skip whitespaces, empty and comment lines */
		for (ptr = in.buffer; isspace(*ptr); ptr++);

		if (ptr[0] == '\0' || ptr[0] == '#')
			goto skip;

		ret = format->scan(in.buffer, bytes, &rbytes, &smps[i], 1);
		if (ret < 0)
			return ret;
	}

	return i;
}

int Io::print(struct sample *smps[], unsigned cnt)
{
	int ret;

	assert(io->state == STATE_OPENED);

	if (!io->header_printed && cnt > 0)
		io_header(io, smps[0]);

	if (io->flags & IO_NEWLINES)
		ret = io_print_lines(io, smps, cnt);
	else if (io_type(io)->print)
		ret = io_type(io)->print(io, smps, cnt);
	else if (io_type(io)->sprint) {
		FILE *f = io_stream_output(io);
		size_t wbytes;

		ret = io_sprint(io, io->out.buffer, io->out.buflen, &wbytes, smps, cnt);

		fwrite(io->out.buffer, wbytes, 1, f);
	}
	else
		ret = -1;

	if (io->flags & IO_FLUSH)
		io_flush(io);

	return ret;
}

int Io::scan(struct sample *smps[], unsigned cnt)
{
	int ret;

	assert(io->state == STATE_OPENED);

	if (io->flags & IO_NEWLINES)
		ret = scanLines(smps, cnt);
	else {
		FILE *f = getInputStream();
		size_t bytes, rbytes;

		bytes = fread(io->in.buffer, 1, io->in.buflen, f);

		ret = format->scan(io->in.buffer, bytes, &rbytes, smps, cnt);
	}
	else
		ret = -1;

	return ret;
}

Io::Io(const FormatFactory *ff, struct vlist *signals, int flags)
{
	int ret;

	flags = flags | (io_type(io)->flags & ~SAMPLE_HAS_ALL);
	delimiter = io_type(io)->delimiter ? io_type(io)->delimiter : '\n';
	separator = io_type(io)->separator ? io_type(io)->separator : '\t';

	in.buflen =
	out.buflen = 4096;

	in.buffer = (char *) alloc(in.buflen);
	out.buffer = (char *) alloc(out.buflen);

	format = ff.make(signals, flags);

	state = STATE_INITIALIZED;
}

~Io::Io()
{
	assert(state == STATE_CLOSED || state == STATE_INITIALIZED || state == STATE_CHECKED);

	if (format)
		delete format;

	delete in.buffer;
	delete out.buffer;
}

void Io::open()
{
	size_t bytes;

	bytes = format->header(buffer.out, buffer_len.out);

	FILE *f = getOutputStream();

	fwrite(buffer.out, bytes, 1, f);
}

void Io::close()
{
	format->footer();
}

void StandardIo::open(const char *uri)
{
	int ret;

	if (uri.empty()) {
		flags |= IO_FLUSH;

		in.stream  = stdin;
		out.stream = stdout;
	}
	else {
		out.stream = fopen(uri.c_str(), "a+");
		if (out.stream == nullptr)
			throw RuntimeError();

		in.stream  = fopen(uri.c_str(), "r");
		if (in.stream == nullptr)
			throw RuntimeError();
	}

	Io::open()
}

#if 0
	/* Make stream non-blocking if desired */
	if (io->flags & IO_NONBLOCK) {
		int ret, fd, flags;

		fd = io_fd(io);
		if (fd < 0)
			return fd;

		flags = fcntl(fd, F_GETFL);
		if (flags < 0)
			return flags;

		flags |= O_NONBLOCK;

		ret = fcntl(fd, F_SETFL, flags);
		if (ret)
			return ret;
	}

	/* Enable line buffering on stdio */
	if (io->mode == IO_MODE_STDIO) {
		ret = setvbuf(io->in.stream.std, nullptr, _IOLBF, BUFSIZ);
		if (ret)
			return -1;

		ret = setvbuf(io->out.stream.std, nullptr, _IOLBF, BUFSIZ);
		if (ret)
			return -1;
	}

	return 0;
}
#endif

void AdvancedIo::open(const char *uri)
{
	out.stream = afopen(uri.c_str(), "a+");
	if (out.stream == nullptr)
		return -1;

	in.stream = afopen(uri.c_str(), "a+");
	if (in.stream == nullptr)
		return -2;

	Io::open()
}

static Io * Io::create(const std::string &uri)
{
	if (uri.empty() || uri == "-")
		return new StandardIo()
	else if (aislocal(uri))
		return new StandardIo(uri);
	else
		return new AdvancedIo(uri);
}

void AvancedIo::close()
{
	Io::close()

	ret = afclose(stream.in);
	if (ret)
		throw RuntimeError();

	ret = afclose(stream.out);
	if (ret)
		throw RuntimeError();
}

void StandardIo::close()
{
	Io::close();

	if (stream.in == stdin)
		return 0;

	ret = fclose(istream.n);
	if (ret)
		throw RuntimeError();

	ret = fclose(stream.out);
	if (ret)
		throw RuntimeError();
}

void AdvancedIo::flush()
{
	int ret;

	ret = afflush(stream.out);
	if (ret)
		throw RuntimeError();
}

void StandardIo::flush()
{
	int ret;

	ret = fflush(stream.out);
	if (ret)
		throw RuntimeError();
}

bool AdvancedIo::eof()
{
	return afeof(stream.in);
}

bool StandardIo::eof()
{
	return feof(stream.in);
}

void AdvancedIo::rewind()
{
	arewind(stream.in);
}

void StandardIo::rewind()
{
	rewind(stream.in);
}

int AdvancedIo::getFiledescriptor()
{
	return afileno(stream.in);
}

int StandardIo::getFiledescriptor()
{
	return fileno(stream.in);
}

FILE * AdvancedIo::getOutputStream()
{
	if (state != STATE_OPENED)
		return nullptr;

	return stream.out;
}

FILE * AdvancedIo::getInputStream()
{
	if (state != STATE_OPENED)
		return nullptr;

	return stream.in;
}

FILE * StandardIo::getOutputStream()
{
	if (state != STATE_OPENED)
		return nullptr;

	return stream.out;
}

FILE * StandardIo::getInputStream()
{
	if (state != STATE_OPENED)
		return nullptr;

	return stream.in;
}
