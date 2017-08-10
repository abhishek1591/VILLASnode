/** API session.
 *
 * @file
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

#pragma once

#include <stdbool.h>
#include <jansson.h>

#include "common.h"
#include "web/buffer.h"

enum api_version {
	API_VERSION_UNKOWN	= 0,
	API_VERSION_1		= 1
};

enum api_mode {
	API_MODE_WS,	/**< This API session was established over a WebSocket connection. */
	API_MODE_HTTP	/**< This API session was established via a HTTP REST request. */
};

/** A connection via HTTP REST or WebSockets to issue API actions. */
struct api_session {
	enum api_mode mode;
	enum api_version version;

	int runs;

	struct {
		struct web_buffer body;		/**< HTTP body / WS payload */
	} request;

	struct {
		struct web_buffer body;		/**< HTTP body / WS payload */
		struct web_buffer headers;	/**< HTTP headers */
	} response;
	
	struct {
		char name[64];
		char ip[64];
	} peer;

	bool completed;				/**< Did we receive the complete body yet? */

	enum state state;

	struct api *api;
};

int api_session_init(struct api_session *s, struct api *a, enum api_mode m);

int api_session_destroy(struct api_session *s);

int api_session_run_command(struct api_session *s, json_t *req, json_t **resp);
