/** Memory allocators.
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

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>

#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>

/* Required to allocate hugepages on Apple OS X */
#ifdef __MACH__
  #include <mach/vm_statistics.h>
#elif defined(__linux__)
  #include <villas/kernel/kernel.h>
#endif

#include <villas/log.h>
#include <villas/memory.h>
#include <villas/utils.h>

int memory_init(int hugepages)
{
	info("Initialize memory sub-system");

#ifdef __linux__
	int ret, pagecnt, pagesz;
	struct rlimit l;

	pagecnt = kernel_get_nr_hugepages();
	if (pagecnt < hugepages) { INDENT
		if (getuid() == 0) {
			kernel_set_nr_hugepages(hugepages);
			debug(LOG_MEM | 2, "Increased number of reserved hugepages from %d to %d", pagecnt, hugepages);
		}
		else {
			warn("Failed to reserved hugepages. Please re-run as super-user or reserve manually via:");
			warn("   $ echo %d > /proc/sys/vm/nr_hugepages", hugepages);

			return -1;
		}
	}

	pagesz = kernel_get_hugepage_size();
	if (pagesz < 0)
		return -1;

	/* Amount of memory which should be lockable */
	size_t lock = pagesz * hugepages;

	ret = getrlimit(RLIMIT_MEMLOCK, &l);
	if (ret)
		return ret;

	if (l.rlim_cur < lock) {
		if (l.rlim_max < lock) {
			if (getuid() != 0) {
				warn("Failed to in increase ressource limit of locked memory from %zu to %zu bytes", l.rlim_cur, lock);
				warn("Please re-run as super-user or raise manually via:");
				warn("   $ ulimit -Hl %zu", lock);

				return -1;
			}

			l.rlim_max = lock;
		}

		l.rlim_cur = lock;

		ret = setrlimit(RLIMIT_MEMLOCK, &l);
		if (ret)
			return ret;

		debug(LOG_MEM | 2, "Increased ressource limit of locked memory to %d bytes", pagesz * pagecnt);
	}
#endif
	return 0;
}

void * memory_alloc(struct memtype *m, size_t len)
{
	void *ptr = m->alloc(m, len, sizeof(void *));

	debug(LOG_MEM | 5, "Allocated %#zx bytes of %s memory: %p", len, m->name, ptr);

	return ptr;
}

void * memory_alloc_aligned(struct memtype *m, size_t len, size_t alignment)
{
	void *ptr = m->alloc(m, len, alignment);

	debug(LOG_MEM | 5, "Allocated %#zx bytes of %#zx-byte-aligned %s memory: %p", len, alignment, m->name, ptr);

	return ptr;
}

int memory_free(struct memtype *m, void *ptr, size_t len)
{
	debug(LOG_MEM | 5, "Releasing %#zx bytes of %s memory", len, m->name);

	return m->free(m, ptr, len);
}

static void * memory_heap_alloc(struct memtype *m, size_t len, size_t alignment)
{
	void *ptr;
	int ret;

	if (alignment < sizeof(void *))
		alignment = sizeof(void *);

	ret = posix_memalign(&ptr, alignment, len);

	return ret ? NULL : ptr;
}

int memory_heap_free(struct memtype *m, void *ptr, size_t len)
{
	free(ptr);

	return 0;
}

/** Allocate memory backed by hugepages with malloc() like interface */
static void * memory_hugepage_alloc(struct memtype *m, size_t len, size_t alignment)
{
	void *ret;
	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_PRIVATE | MAP_ANONYMOUS;

#ifdef __MACH__
	flags |= VM_FLAGS_SUPERPAGE_SIZE_2MB;
#elif defined(__linux__)
	flags |= MAP_HUGETLB;

	if (getuid() == 0)
		flags |= MAP_LOCKED;
#endif

	ret = mmap(NULL, len, prot, flags, -1, 0);
	if (ret == MAP_FAILED)
		return NULL;

	return ret;
}

static int memory_hugepage_free(struct memtype *m, void *ptr, size_t len)
{
	/** We must make sure that len is a multiple of the hugepage size
	 *
	 * See: https://lkml.org/lkml/2014/10/22/925
	 */
	len = ALIGN(len, HUGEPAGESIZE);

	return munmap(ptr, len);
}

void* memory_managed_alloc(struct memtype *m, size_t len, size_t alignment)
{
	/* Simple first-fit allocation */
	struct memblock *first = (struct memblock *) m->_vd;
	struct memblock *block;

	for (block = first; block != NULL; block = block->next) {
		if (block->flags & MEMBLOCK_USED)
			continue;

		char* cptr = (char *) block + sizeof(struct memblock);
		size_t avail = block->len;
		uintptr_t uptr = (uintptr_t) cptr;

		/* Check alignment first; leave a gap at start of block to assure
		 * alignment if necessary */
		uintptr_t rem = uptr % alignment;
		uintptr_t gap = 0;
		if (rem != 0) {
			gap = alignment - rem;
			if (gap > avail)
				continue; /* Next aligned address isn't in this block anymore */

			cptr += gap;
			avail -= gap;
		}

		if (avail >= len) {
			if (gap > sizeof(struct memblock)) {
				/* The alignment gap is big enough to fit another block.
				 * The original block descriptor is already at the correct
				 * position, so we just change its len and create a new block
				 * descriptor for the actual block we're handling. */
				block->len = gap - sizeof(struct memblock);
				struct memblock *newblock = (struct memblock *) (cptr - sizeof(struct memblock));
				newblock->prev = block;
				newblock->next = block->next;
				block->next = newblock;
				newblock->flags = 0;
				newblock->len = len;
				block = newblock;
			}
			else {
				/* The gap is too small to fit another block descriptor, so we
				 * must account for the gap length in the block length. */
				block->len = len + gap;
			}

			if (avail > len + sizeof(struct memblock)) {
				/* Imperfect fit, so create another block for the remaining part */
				struct memblock *newblock = (struct memblock *) (cptr + len);
				newblock->prev = block;
				newblock->next = block->next;
				block->next = newblock;
				if (newblock->next)
					newblock->next->prev = newblock;
				newblock->flags = 0;
				newblock->len = avail - len - sizeof(struct memblock);
			}
			else {
				/* If this block was larger than the requested length, but only
				 * by less than sizeof(struct memblock), we may have wasted
				 * memory by previous assignments to block->len. */
				block->len = avail;
			}

			block->flags |= MEMBLOCK_USED;

			return (void *) cptr;
		}
	}

	/* No suitable block found */
	return NULL;
}

int memory_managed_free(struct memtype *m, void *ptr, size_t len)
{
	struct memblock *first = (struct memblock *) m->_vd;
	struct memblock *block;
	char *cptr = ptr;

	for (block = first; block != NULL; block = block->next) {
		if (!(block->flags & MEMBLOCK_USED))
			continue;

		/* Since we may waste some memory at the start of a block to ensure
		 * alignment, ptr may not actually be the start of the block */
		if ((char *) block + sizeof(struct memblock) <= cptr &&
		    cptr < (char *) block + sizeof(struct memblock) + block->len) {
			/* Try to merge it with neighbouring free blocks */
			if (block->prev && !(block->prev->flags & MEMBLOCK_USED) &&
			    block->next && !(block->next->flags & MEMBLOCK_USED)) {
				/* Special case first: both previous and next block are unused */
				block->prev->len += block->len + block->next->len + 2 * sizeof(struct memblock);
				block->prev->next = block->next->next;
				if (block->next->next)
					block->next->next->prev = block->prev;
			}
			else if (block->prev && !(block->prev->flags & MEMBLOCK_USED)) {
				block->prev->len += block->len + sizeof(struct memblock);
				block->prev->next = block->next;
				if (block->next)
					block->next->prev = block->prev;
			}
			else if (block->next && !(block->next->flags & MEMBLOCK_USED)) {
				block->len += block->next->len + sizeof(struct memblock);
				block->next = block->next->next;
				if (block->next)
					block->next->prev = block;
			}
			else {
				/* no neighbouring free block, so just mark it as free */
				block->flags &= ~MEMBLOCK_USED;
			}

			return 0;
		}
	}

	return -1;
}

struct memtype * memtype_managed_init(void *ptr, size_t len)
{
	struct memtype *mt = ptr;
	struct memblock *mb;
	char *cptr = ptr;

	if (len < sizeof(struct memtype) + sizeof(struct memblock)) {
		info("memtype_managed_init: passed region too small");
		return NULL;
	}

	/* Initialize memtype */
	mt->name  = "managed";
	mt->flags = 0;
	mt->alloc = memory_managed_alloc;
	mt->free  = memory_managed_free;
	mt->alignment = 1;

	cptr += ALIGN(sizeof(struct memtype), sizeof(void *));

	/* Initialize first free memblock */
	mb = (struct memblock *) cptr;
	mb->prev = NULL;
	mb->next = NULL;
	mb->flags = 0;

	cptr += ALIGN(sizeof(struct memblock), sizeof(void *));

	mb->len = len - (cptr - (char *) ptr);

	mt->_vd = (void *) mb;

	return mt;
}

/* List of available memory types */
struct memtype memtype_heap = {
	.name = "heap",
	.flags = MEMORY_HEAP,
	.alloc = memory_heap_alloc,
	.free = memory_heap_free,
	.alignment = 1
};

struct memtype memtype_hugepage = {
	.name = "mmap_hugepages",
	.flags = MEMORY_MMAP | MEMORY_HUGEPAGE,
	.alloc = memory_hugepage_alloc,
	.free = memory_hugepage_free,
	.alignment = 21 /* 2 MiB hugepage */
};
