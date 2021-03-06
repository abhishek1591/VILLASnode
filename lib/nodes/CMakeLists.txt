# CMakeLists.
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

set(NODE_SRC)

if(LIBNL3_ROUTE_FOUND)
    list(APPEND LIBRARIES PkgConfig::LIBNL3_ROUTE)
    list(APPEND INCLUDE_DIRS ${LIBNL3_ROUTE_INCLUDE_DIRS})
endif()

if(WITH_NODE_INFLUXDB)
    list(APPEND NODE_SRC influxdb.cpp)
endif()

if(WITH_NODE_STATS)
    list(APPEND NODE_SRC stats.cpp)
endif()

if(WITH_NODE_SIGNAL)
    list(APPEND NODE_SRC signal_generator.cpp)
endif()

if(WITH_NODE_LOOPBACK)
    list(APPEND NODE_SRC loopback.cpp)
endif()

if(WITH_NODE_TEST_RTT)
    list(APPEND NODE_SRC test_rtt.cpp)
endif()

if(WITH_NODE_SOCKET)
    list(APPEND NODE_SRC socket.cpp)
endif()

if(WITH_NODE_FILE)
    list(APPEND NODE_SRC file.cpp)
endif()

if(WITH_NODE_EXEC)
    list(APPEND NODE_SRC exec.cpp)
endif()

# Enable Universal Library for Linux DAQ devices (libuldaq)
if(WITH_NODE_ULDAQ)
    list(APPEND NODE_SRC uldaq.cpp)
    list(APPEND INCLUDE_DIRS ${LIBULDAQ_INCLUDE_DIRS})
    list(APPEND LIBRARIES PkgConfig::LIBULDAQ uldaq)
endif()

# Enable shared memory node-type
if(WITH_NODE_SHMEM)
    list(APPEND NODE_SRC shmem.cpp)

    if(CMAKE_SUSTEM_NAME STREQUAL Linux)
        list(APPEND LIBRARIES rt)
    endif()
endif()

# Enable IEC61850 node-types when libiec61850 is available
if(WITH_NODE_IEC61850)
    list(APPEND NODE_SRC iec61850_sv.cpp iec61850.cpp)
    list(APPEND INCLUDE_DIRS ${LIBIEC61850_INCLUDE_DIRS})
    list(APPEND LIBRARIES PkgConfig::LIBIEC61850 ${LIBIEC61850_LIBRARIES})
endif()

# Enable OPAL-RT Asynchronous Process support (will result in 32bit binary!!!)
if(WITH_NODE_OPAL)
    list(APPEND NODE_SRC opal.cpp)
    list(APPEND INCLUDE_DIRS ${OPAL_INCLUDE_DIRS})
    list(APPEND LIBRARIES ${OPAL_LIBRARIES})
endif()

# Enable nanomsg node type when libnanomsg is available
if(WITH_NODE_NANOMSG)
    list(APPEND NODE_SRC nanomsg.cpp)
    list(APPEND INCLUDE_DIRS ${NANOMSG_INCLUDE_DIRS})
    list(APPEND LIBRARIES PkgConfig::NANOMSG)
endif()

# Enable ZeroMQ node type when libzmq is available
if(WITH_NODE_ZEROMQ)
    list(APPEND NODE_SRC zeromq.cpp)
    list(APPEND INCLUDE_DIRS ${LIBZMQ_INCLUDE_DIRS})
    list(APPEND LIBRARIES PkgConfig::LIBZMQ)
endif()

# Enable NGSI support
if(WITH_NODE_NGSI)
    list(APPEND NODE_SRC ngsi.cpp)
    list(APPEND INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
    list(APPEND LIBRARIES ${CURL_LIBRARIES})
endif()

# Enable WebSocket support
if(WITH_NODE_WEBSOCKET)
    list(APPEND NODE_SRC websocket.cpp)
    list(APPEND INCLUDE_DIRS ${LIBWEBSOCKETS_INCLUDE_DIRS})
    list(APPEND LIBRARIES ${LIBWEBSOCKETS_LDLIBS})
endif()

# Enable AMQP support
if(WITH_NODE_AMQP)
    list(APPEND NODE_SRC amqp.cpp)
    list(APPEND INCLUDE_DIRS ${RABBITMQ_C_INCLUDE_DIRS})
    list(APPEND LIBRARIES PkgConfig::RABBITMQ_C)
endif()

# Enable MQTT support
if(WITH_NODE_MQTT)
    list(APPEND NODE_SRC mqtt.cpp)
    list(APPEND INCLUDE_DIRS ${MOSQUITTO_INCLUDE_DIRS})
    list(APPEND LIBRARIES ${MOSQUITTO_LIBRARIES})
endif()

# Enable Comedi support
if(WITH_NODE_COMEDI)
    list(APPEND NODE_SRC comedi.cpp)
    list(APPEND INCLUDE_DIRS ${COMEDILIB_INCLUDE_DIRS})
    list(APPEND LIBRARIES PkgConfig::COMEDILIB)
endif()

# Enable Infiniband support
if(WITH_NODE_INFINIBAND)
    list(APPEND NODE_SRC infiniband.cpp)
    list(APPEND INCLUDE_DIRS ${IBVERBS_INCLUDE_DIRS} ${RDMACM_INCLUDE_DIRS})
    list(APPEND LIBRARIES ${IBVERBS_LIBRARIES} ${RDMACM_LIBRARIES})
endif()

# Enable RTP node type when libre is available
if(WITH_NODE_RTP)
    list(APPEND NODE_SRC rtp.cpp)
    list(APPEND INCLUDE_DIRS ${RE_INCLUDE_DIRS})
    list(APPEND LIBRARIES PkgConfig::RE)
endif()

add_library(nodes STATIC ${NODE_SRC})
target_include_directories(nodes PUBLIC ${INCLUDE_DIRS})
target_link_libraries(nodes INTERFACE ${LIBRARIES} PUBLIC villas-common)
