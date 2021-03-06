# CMakeLists.txt.
#
# @author Steffen Vogel <stvogel@eonerc.rwth-aachen.de>
# @copyright 2014-2019, Institute for Automation of Complex Power Systems, EONERC
# @license GNU General Public License (version 3)
#
# VILLASnode
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
###################################################################################

set(TEST_SRC
	config_json.cpp
	io.cpp
	json.cpp
	main.cpp
	mapping.cpp
	memory.cpp
	pool.cpp
	queue.cpp
	queue_signalled.cpp
	signal.cpp
)

add_executable(unit-tests ${TEST_SRC})
target_link_libraries(unit-tests PUBLIC
	PkgConfig::CRITERION
	Threads::Threads
	villas
)

add_custom_target(run-unit-tests
	COMMAND $<TARGET_FILE:unit-tests> ${CRITERION_OPTS}
	USES_TERMINAL
)

add_dependencies(tests unit-tests)
add_dependencies(run-tests run-unit-tests)

add_dependencies(run-unit-tests unit-tests)
