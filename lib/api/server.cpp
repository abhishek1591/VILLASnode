/** Socket API endpoint.
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

#include <unistd.h>
#include <sys/types.h>

#include <exception>
#include <algorithm>

#if __GNUC__ <= 7 && !defined(__clang__)
  #include <experimental/filesystem>
#else
  #include <filesystem>
#endif

#include <villas/config.h>
#include <villas/node/exceptions.hpp>
#include <villas/utils.hpp>
#include <villas/super_node.hpp>
#include <villas/api/server.hpp>
#include <villas/api/sessions/socket.hpp>

using namespace villas;
using namespace villas::node::api;

#if __GNUC__ <= 7 && !defined(__clang__)
  namespace fs = std::experimental::filesystem;
#else
  namespace fs = std::filesystem;
#endif

Server::Server(Api *a) :
	state(State::INITIALIZED),
	api(a)
{
	logger = logging.get("api:server");
}

Server::~Server()
{
	assert(state != State::STARTED);
}

void Server::start()
{
	int ret;

	assert(state != State::STARTED);

	sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd < 0)
		throw SystemError("Failed to create Api socket");

	pollfd pfd = {
		.fd = sd,
		.events = POLLIN
	};

	pfds.push_back(pfd);

	struct sockaddr_un sun = getSocketAddress();

	ret = bind(sd, (struct sockaddr *) &sun, sizeof(struct sockaddr_un));
	if (ret)
		throw SystemError("Failed to bind API socket");

	ret = listen(sd, 5);
	if (ret)
		throw SystemError("Failed to listen on API socket");

	logger->info("Listening on UNIX socket: {}", sun.sun_path);

	state = State::STARTED;
}

struct sockaddr_un Server::getSocketAddress()
{
	struct sockaddr_un sun = { .sun_family = AF_UNIX };

	fs::path socketPath;

	if (getuid() == 0) {
		socketPath = PREFIX "/var/lib/villas";
	}
	else {
		std::string homeDir = getenv("HOME");

		socketPath = homeDir + "/.villas";
	}

	if (!fs::exists(socketPath)) {
		logging.get("api")->info("Creating directory for API socket: {}", socketPath);
		fs::create_directories(socketPath);
	}

	socketPath += "/node-" + api->getSuperNode()->getName() + ".sock";

	if (fs::exists(socketPath)) {
		logging.get("api")->info("Removing existing socket: {}", socketPath);
		fs::remove(socketPath);
	}

	strncpy(sun.sun_path, socketPath.c_str(), sizeof(sun.sun_path) - 1);

	return sun;
}

void Server::stop()
{
	int ret;

	assert(state == State::STARTED);

	ret = close(sd);
	if (ret)
		throw SystemError("Failed to close API socket");;

	state = State::STOPPED;
}

void Server::run(int timeout)
{
	int ret;

	assert(state == State::STARTED);

	auto len = pfds.size();

	ret = poll(pfds.data(), len, timeout);
	if (ret < 0)
		throw SystemError("Failed to poll on API socket");

	std::vector<sessions::Socket *> closing;

	for (unsigned i = 1; i < len; i++) {
		auto &pfd = pfds[i];

		/* pfds[0] is the server socket */
		auto s = sessions[pfd.fd];

		if (pfd.revents & POLLIN) {
			ret = s->read();
			if (ret < 0)
				closing.push_back(s);
		}

		if (pfd.revents & POLLOUT) {
			s->write();
		}
	}

	/* Destroy closed sessions */
	for (auto *s : closing)
		closeSession(s);

	/* Accept new connections */
	if (pfds[0].revents & POLLIN)
		acceptNewSession();
}

void Server::acceptNewSession() {
	int fd = ::accept(sd, nullptr, nullptr);

	auto s = new sessions::Socket(api, fd);

	pollfd pfd = {
		.fd = fd,
		.events = POLLIN | POLLOUT
	};

	pfds.push_back(pfd);
	sessions[fd] = s;

	api->sessions.push_back(s);
}

void Server::closeSession(sessions::Socket *s)
{
	int sd = s->getSd();

	sessions.erase(sd);
	api->sessions.remove(s);

	pfds.erase(std::remove_if(pfds.begin(), pfds.end(),
		[sd](const pollfd &p){ return p.fd == sd; })
	);

	delete s;
}
