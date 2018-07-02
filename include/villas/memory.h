/** Memory allocators.
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

#include <stddef.h>
#include <stdint.h>

#include <villas/memory_type.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct node;

enum memblock_flags {
	MEMBLOCK_USED = 1,
};

/** Descriptor of a memory block. Associated block always starts at
 * &m + sizeof(struct memblock). */
struct memblock {
	struct memblock *prev;
	struct memblock *next;
	size_t len; /**<Length of the block; doesn't include the descriptor itself */
	int flags;
};

/** @todo Unused for now */
struct memzone {
	struct memory_type *const type;

	void *addr;
	uintptr_t physaddr;
	size_t len;
};

/** Initilialize memory subsystem */
int memory_init(int hugepages);

/** Allocate \p len bytes memory of type \p m.
 *
 * @retval NULL If allocation failed.
 * @retval <>0  If allocation was successful.
 */
void * memory_alloc(struct memory_type *m, size_t len);

void * memory_alloc_aligned(struct memory_type *m, size_t len, size_t alignment);

int memory_free(struct memory_type *m, void *ptr, size_t len);

#ifdef __cplusplus
}
#endif
