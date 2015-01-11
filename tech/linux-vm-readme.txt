
MM
---
The definition of the struct mm_struct is in linux/sched.h.

A mm structure defines an address space. Each process or clone sharing
an address space has a pointer to an MM from its task structure. The
number of tasks sharing an address space is maintained in "count",
which is atomically manipulated by mmget/mmput.

The "mmap" field is a list of vma's, each of which tracks an allocated
address range in the address space. The vma's are kept in increasing
address order, chained via vm_next. When the number of vma's gets to
be more than AVL_MIN_MAP_COUNT, a parallel avl tree is created out
of them for faster searching. This tree is rooted at "mmap_avl". The
"mmap_cache" keeps a pointer to the last vma found, and cuts down
on tree/list searches due to locality of reference. The number of vma's
are in "map_count". The "mmap_sem" guards addition/deletion of vma's
and changes to any fields in any vma in the list. Note that the code
guards against having more than MAX_MAP_COUNT vma's in a space.

The "pgd" is a kernel pointer into the page directory. Often, this is
just a software structure (as in the MIPS, where tlb's are updated
and the hardware does not really look at the page directory), and often
it is viewed by the hardware (as in ia32).

The "context" field is a processor mmu field, which is used in some
processors (MIPS, sparc), but not on others. Similarly, the "segments"
field is also a processor specific field, which is used just on ia32
to track the LDT. The "cpu_vm_mask" is also just used on the sparcs.

Currently, the "def_flags" has only one possible value, ie VM_LOCKED,
and indicates whether future-locking is enable on the address space.
The "locked_vm" tracks the total number of locked pages (sum of pages
in locked vma's, somewhat different for the stack) in the address
space for accounting/reservation reasons. The "total_vm" tracks the
total address space summed over all the vma's. The "rss" tracks the
number of incore pages belonging to the address space.

The "swap_address" and "swap_cnt" fields are used to determine how
many pages to steal, and from which address range, when memory runs
low. The "swap_address" is maintained as a rotor. The "swap_cnt" field
is used to search for the best process to steal pages from depending
on the process' address space "rss".

The "arg_start", "arg_end", "env_start" and "env_end" are no misnomers,
and maintain the user's virtual address to the argv[] and env[] for the
executing image on its stack, and can be queried via procfs. The fields
"start_code", "end_code", "start_data" and "end_data" are similar. These
are set up at exec time, when the kernel creates the stack for the program
and populates it with the argv/env arrays. The "end_code" field is used
in the brk() system call to make sure that the break area does not run
into code, and that the data is within rlimits boundary. The "start_stack"
is also set at exec time, and is only used by shared memory to make
sure some growing place is left for the stack (WHY NOT FOR OTHER MMAPS?)
"start_brk" is also set at exec time, but the "brk" value changes as
the program issues brk system call to get more data space.

VMA
----
The definition of a vma is in linux/mm.h.

Each vma tracks an address range allocated to the address space. The
address range is maintained in "vm_start" and "vm_end".

The "vm_mm" points back into the owning memory manager (ie, address space).
It is mostly used to update address space specific fields, like "rss",
"locked_vm" and "total_vm", as well as locate the page directory.

The "vm_next" field is used to track the vma's in increasing order of
address ranges in a singly linked list. This list is used for searching the
vma corresponding to any given range ... to decrease search time, when the
list grows to have more than AVL_MIN_MAP_COUNT vma's, a parallel avl tree
is also maintained via the "vm_avl_left", "vm_avl_right" and "vm_avl_height"
fields.

The "vm_next_share" and "vm_pprev_share" help in managing a doubly linked
list of vma's. The shm code uses this to track all the vma's that map a
single shared object (although this is only recreational, not productive).
If the underlying object is a file/device, the mmap code tracks all the
vma's mapping that object via these fields - this is used if the file is
being truncated to visit all the vma's and shoot down the affected pages
in them.

When the vma represents a file/device, the "vm_file" is a pointer to the
file data structure. The "vm_offset" is the offset into the file where the
mapping starts.

The "vm_pte" is only used when the vma has an underlying shm object, and
the shm code uses it to track the shm id that the vma is mapping. This is
used when a no-page fault happens, so that the shm code can go look at the
proper data structures for the shm id and update the pte accordingly.

The "vm_ops" field is a pointer to a set of operations defined by the
manager of the underlying object. For example, the vm_ops for a shm area,
for a shared file mapping and a provate file mapping all are different.
Drivers are also free to define their own operations when a vma maps
the corresponding device or driver memory.

"vm_page_prot" is always protection_map[vm_flags & 0xf]. The lower 4
bits of vm_flags maintain the VM_READ/WRITE/EXEC (ie PROT_READ/WRITE/
EXEC) permissions of the mapping and the VM_SHARED flag, which indicates
whether the address range shares its changes with the underlying
object. "vm_page_prot" is used to set up the pte protections while
updating the pte's. The protection_map[] defines what bits are put
in the pte corresponding to shared and private mappings with the
specified rwx privileges.

The flags VM_MAYREAD and VM_MAYEXEC track what maximum protections
are available via mprotect - they are set on all mappings except
for MAP_SHARED writeonly file mappings (note that MAP_PRIVATE mappings
are not allowed to nonreadable files). The VM_MAYWRITE flag is turned
on for all non-file mappings, for MAP_PRIVATE file mappings, as
well as for MAP_SHARED mappings on files opened for write, but not
on MAP_SHARED (readonly) mappings on files opened readonly. It
is a scheme to demote MAP_SHARED readonly mappings to MAP_PRIVATE
readonly mappings. The VM_SHARED mappings share any changes in the
mm context with the underlying object, and are set by MAP_SHARED
mappings (except the case for readonly MAP_SHARED mappings that
will never be promoted to have write perms). The VM_MAYSHARE flag
indicates that the mapping was created by a MAP_SHARED mapping,
although the mm might never be able to change any of the pages
of the underlying object. VM_DENYWRITE mappings are normally
mappings set up by the kernel internally, as during mapping a.out
(or libraries and interpreters) and is an indicator that the
underlying file can not be modified while the mapping is around.
VM_LOCKED indicates that the pages are locked and can not be
stolen. VM_EXECUTABLE indicates that the mapping is for the
executable part of a.out.

The VM_LOCKED flag indicates that the pages of the vma have been faulted
and locked into memory, and none of these can be stolen when memory runs
low on the system. The VM_GROWSDOWN flag is only for use by the vma managing
the stack, and indicates that growing the vma decreases "vm_start" (instead
of increasing "vm_end"). The counterflag, VM_GROWSUP, does not seem used.

The VM_IO flag denotes that the vma is ampping an io device, which should
not be dumped if the program core dumps. At least on the m68k, it also
seems that these vma's can not incure page faults (meaning the drivers
must validate the pte's at mapping time).

The VM_SHM flag is turned on by shm code and by drivers. It indicates that
a vma can not be merged with vma's mapping neighbouring address ranges,
unless the offsets line up properly (as during merging vma's of file
mappings.

PAGES
-----
The definition of a page is in linux/mm.h.

Page usage in the system:

include/linux/mm.h has a bunch of comments about how pages are maintained
and the meanings of the various bits in the "flags". Essentially, each
page in the system is maintained via a "struct page". Absent (or
inaccessible) pages are marked PG_reserved (for example, on the ia32,
0x9F000 ... really 0xA0000, to 1Mb is reserved for the video memory).
These pages are then not allocated by the os for any data.

All other pages in the system are usable. Drivers/slabcode/filesystems
can grab a page, and then maintain/alter the different fields in the
page structure. Another user of pages is the buffer system, which divvies
up a physical page into multiple buffer-data area - these pages have
their "buffers" field pointing to a chain of the headers of the buffers
whose data areas are within the page. Another significant user of pages
is the shared memory subsystem. At low memory conditions, it is possible
to reclaim some/all of these pages (shm_swap, kmem_cache_reap etc).
The PG_Slab bit is used to indicate the case when a page is allocates to
the slab cache, and is used mostly for sanity purposes in the slab cache
allocation/free code.

By far, the most complicated uses of a page are by the filesystem code
that cache file data in the pages. And by the swap code that caches
disk-copies of data that it has swapped into/outof core.

For information on how the shm code, file data code and swap code manage
these pages and the fields in the structure, look into the corresponding
sections.

Page free list management:

All allocatable, free pages in the system are put in the free list. The
free list is managed as an array of free-lists, each of which keeps track
of 1, 2, 4, 8 ... contiguous free pages. Whenever a page is freed, a
buddy algoritm is invoked to coalesce it into the biggest possible free
large page. Page allocation and freeing always add the (large) page into
the head of the list.

Note that free_area[i] will track (via the page "next"/"prev" pointers)
any pfn that is a multiple of 2^i, if consecutive 2^i pages starting from
that pfn are free. Also, this page will not appear in the lower order
freelists, ie, even though pfns 8 .. 16 might be free, pfn 8 will appear
only in free_area[3], not in the lower order ones. To implement the buddy
algorithm, free_area[i] has a bitmap that indicates whether each of the
possible pfns that can be present in this list are actually present in
this list or not. In the previous example for free pfns 8 .. 16, only
pfn 8 will be present in free_area[3] with defined "next"/"prev" pointers,
all the other pages will not be in any free list, and will have undefined
"next"/"prev" pointers.

Page allocation and freeing are protected by a single spinlock
page_alloc_lock.

Page fields:

When a page is on the free list, the "next"/"prev" pointers are used
to link free pages together.

When a page belongs to a file, the "inode" field is set correspondingly.
Note that for anonymous pages that are being stolen by kswapd, the "inode"
is set to "swapper_inode", which is a pseudo file that maintains pages
that have unmodified disk copies of swap. This association might also be
temporary (as used by shm). In either case, the "next/prev" pointers form
a list of pages belonging to the inode (except for the temporary shm case).
In the case of files, the "offset" if the offset into the file whose data
is present in the page; for the swap case, the "offset" is a swap handle
which identifies where on the swap device the page has been copied to.
The "next_hash"/"pprev_hash" form a has list on the (inode/offset) in either
case (except temporary shm).

The "wait" field is used to track tasks waiting for a page to become
unlocked, ie, for io to complete on the page.

If the page is allocate to the buffer subsystem, the "buffers" points
to a list of buffer headers, each of whose data area is in the page. At
least, shrink_mmap uses this to steal buffer pages and to call into the
buffer code to clean up the association.

The "count" field manages the number of references to the page. These
consist of user references (since there's no shared page tables, that
would be the number of pte's that point to the page), as well as kernel
references (as in the hold obtained on a page to put it in the file/swap
cache).

Page flags:

PG_locked indicates that io has been scheduled on a page. After the io
(sync or async) is completed (or can not be done due to buffer header
scarcity), the buffer routines clear the bit. Places which set the
bit are:
1. when a page is being stolen from a process, and it is being put
in the swapcache and rw_swap_page is being invoked on it.
2. The shm code sets the bit when is is swapping in/out a page via
rw_swap_page_nocache.
3. Async swapping via read_swap_cache_async sets the bit.
4. generic_readpage, which is the readpage operation to read in a page
of a file.
5. generic_file_write sets the bit on the page it is going to be writing
to.
When a page is locked, it can not be stolen. For inode pages, locked
pages can not be removed from the cache (invalidate_inode_pages), and
truncation has to wait for the pages to become unlocked. Locked pages
have to be waited for, before they can be returned via a page/swap cache
search.

PG_skip is a sparc processor specific flag.

The PG_Slab bit is used to indicate the case when a page is allocated to
the slab cache, and is used mostly for sanity purposes in the slab cache
allocation/free code.

The PG_error is cleared before the buffer routines start io on a page
and might be set at the end of async io (EXACTLY WHEN?) and is cleared
whenever a page is put in the file cache (not the swapcache though).
This bit is checked in the file read case to make sure an io error did
not occur.

The PG_swap_cache bit indicates that a page is in the swapcache, ie, it
is unmodified with respect to its copy on the swap device. Whenever an
anonymous page is being stolen by kswapd, and whenever such a page is
swapped in, the page is put in the swapcache (temporary shm case). For
such pages, the PG_swap_unlock_after will also be set right before
io is started on the page, so that the io done routines can invoke
swap_after_unlock_page (to unlock the swap pages) [[ rw_swap_page_base
sets this bit if PG_swap_cache is set, but all pages coming into
rw_swap_page_base must have PG_swap_cache set, so why the checking? ]]

The PG_free_after bit is used to indicate to the buffer iodone routine
that the page needs to be freed. Sync/async swap and generic_readpage
set this bit before invoking the read/write brw_page routine, since both
cases grab a page reference before the io starts. The PG_decr_after bit
is similar, and is used to accurately maintain the number of async swap
io's in flight in "nr_async_pages".

The PG_referenced bit is used to decide whether to steal a page or not
and indicates whether a physical page has been accessed recently or not.
Pages in the file/swap cache are marked PG_referenced when a search for
them is made in the cache, so that shrink_mmap leaves these pages alone.
The buffer subsystem also marks some dirty buffers PG_referenced so that
the pages containing the data area are not freed up by shrink_mmap. Also,
when kswapd encounters a young pte, it turns on the PG_referenced bit as
a later hint to shrink_mmap to leave the page alone.

The PG_uptodate bit is set by the buffer iodone routines when an io
completes (both read and write) successfully. Though this bit is set/
cleared on both swapcache and filecache pages, it is only ever checked
on file pages in the "nopage" and "read" paths to make sure that valid
contents were read in fron the disk.

The PG_DMA bit indicates whether the page is in a physical address range
that all devices can do dma to. This is used in the page stealing/kswapd
path to determine whether to steal a page depending on whether there is
need for dmaable memory or regular memory.

FILE CACHE AND FILE/VM INTERACTIONS
------------------------------------
Each inode has a list of vma's mapping the file in "i_mmap". This is
used if the file is being truncated to visit all the vma's and shoot
down the affected pages in them.

The inode also has a pagecache "i_pages", # pages in the list in
"i_nrpages", the pages being chained via the "next/prev" fields.
These pages are also in a hash list (page_hash_table) linked
by the "pprev_hash/next_hash" fields. Each page has a pointer to
the inode it belongs to in the "inode" field, and an "offset"
into the file. Just because a page is in the pagecache does not mean
it has the right data ... it may be PG_locked, indicating io is on
progress to the page, or it may not have PG_uptodate data.
When a page is put in the cache, its PG_error, PG_referenced and PG_uptodate
flags are cleared. The filecache establishes a reference count on
the page structure.

Things which add pages into the hash Q are filemap_nopage
when it has to allocate a page and read in file contents, try_to_read_ahead
when it does not get a cache hit and wants to use any passed in page for
readahead, generic_file_write/do_generic_file_read when it has to allocate
pages to satisfy the user request, and when get_cached_page allocates a
page for the file.

Things which delete pages from the file cache are
1. invalidate_inode_pages, which funnily, is not coming out of msync
MS_INVALIDATE, which seems to be *not* deleting the page from the cache.
2. truncate_inode_pages, which is called when an inode is being deleted by
the filesystem, and to handle truncate() system call.
3. remove_inode_page, when shrink_mmap releases a file page, and when a page
is removed from the swapcache.

SWAP CACHE
----------
The swap cache maintains a pesudo-filecache of pages that are clean with
respect to their copies on the swap device. The kernel uses a pseudo inode
"swapper_inode" to associate these pages with. The swapcache establishes a
reference count on the page structure (using the underlying filecache
routines). Three things indicate that a page is in the swap cache - the
page's inode points to swapper_inode, the page has the PG_swap_cache flag
set, and the page is in the swapper_inode cache/hashQ (temporary shm
swapcache pages do not satisfy the last criteria).

A page is added to the swapcache/hash Q by
1.read_swap_cache_async/swapin_readahead/swap_in when a swap page is read
in from disk. The page is added to the cache before the io is started,
hence though the page is in the SwapCache, it may not have the uptodate
contents. This is anyway indicated by setting of the PG_locked bit, which
is cleared after the io completes - this is the synchronization between
read_swap_cache_async reading the page and lookup_swap_cache finding it.
[[ When rw_swap_page_base starts to read
a page from disk, it clears PG_uptodate to indicate the page contents are
not valid (though this PG_uptodate clearing is done again in brw_page).
SwapCache pages do not need to look at this bit. Why is it being cleared? ]]
2. in try_to_swap_out by kswapd/pagestealers on non-file and non-shm
pages while trying to free pages.

A page is removed from the swapcache/hash Q by
remove_from_swap_cache/delete_from_swap_cache when
1. shrink_mmap finds a page in the swapcache that has a reference only
from the swapcache, meaning that the page has already been stolen and
written out to disk (note that after_unlock_page drops the ref count on
a page after io is done to counter the ref inc that was done in
rw_swap_page_base).
2. do_wp_page, when we are grabbing a page to write to it, so that the
old copy on swap is invalidated.
3. swap_in, when we know the page we swapped in was dirtied hence we are
setting write perms on the page instead of setting read perms and taking
a fault on write whence we remove it from the swapcache.
4. free_page_and_swap_cache, when a page is being freed, and no one other
than the swapcache has a reference to it (except ongoing io). For example,
while we are trying to swap_in, and someone else (like a clone member or
the debugger) has already done it for us. Or when we are freeing a range
of user pages (unmap/exit/remap). Or in put_page, when we have handled a
not present page fault, identify the page to drop into the pte, but find
that it has already been dropped in while we were sleeping identifying/
initing the page that we now hold.
5. try_to_unuse, when we are trying to delete a swap device.

When a page is in the swapcache/hash Q, its "offset" field tracks the swap
handle which identifies where on the swap device it is. When the page
is stolen, this swapentry is put in the pte, so that the process can
do a do_swap_page on a "page-not-present" fault. Note that the SwapCache
is searched for by the same underlying routines that do PageCache
searching (__find_page), hence when a swap_in happens, the code ends
up setting PG_referenced, waiting for PG_locked to go off [[ but not
PG_uptodate? ]]

Each page in the swap cache has an associated swap handle, which was
obtained by a call to get_swap_page(). Each swap page has a reference
count, indicating how many different tasks/data structures have a
reference on that swap page. One reference count comes from the swapcache
itself. Plus one for each process sharing the page. Note that if the user's
virtual page is not in core, the swap_count will not have any reference
corresponding to the swapcache.

PAGE STEALING/KSWAPD
--------------------
When memory runs low, and a process can not find a free page, it wakes
up kswapd, the memory stealer, as well as it itself tries to steal some
pages. This includes trying to trim and free up some pages held in the
slab and inode cache, trying to rip away user reference to pages, and
trying to rip away kernel references to pages.

The "swap_out" routine identifies a process to steal memory from (based
on rss), and then rotors thru its address space selecting a page to steal.
Recently accessed pages (young ptes) are not stolen, else, the user's
reference to the page is deleted. Depending on whether the page is a shm
or file page (really whether the controlling vma has a "swapout" routine),
different actions are taken: dirty file pages are handed to kpiod to be
written out asynchronously, shm pages have no associated action, and
anon pages are also asynchronously pushed to swap. For the anon case, the
swap handle is stored in the pte, for the other cases, the pte is cleared.

For pages controlled by vma's with their own swapout routines (shm,
map-shared files), the pte is cleared when the page is stolen, so
"do_no_page" is invoked, so if the vma has a swapout routine, it
better also have a nopage routine. The only other case under which
do_no_page is invoked is if the page is anonymous, but it has not
been allocated yet.

The real page freeing is done by shrink_mmap: it scans physical memory
sequentially, and tries to rip away kernel references to the pages one
by one. It can do this for only one kernel reference. It attempts to
release unused buffer pages, swapcache pages and filecache pages, by
ripping the page away from the corresponding data structures.

Anon page stealing:

The way a non-file, non-shm page gets freed up by kswapd/stealers is
this: try_to_swap_out adds the page to the swapcache/hash Q, does an async
rw_swap_page, and frees the user reference. The page is cleaned up by
the swap io routines, and then ends up with just a swapcache reference.
A shrink_mmap at that stage will find it and put it in the free list.

When a page is in the swapcache/hash Q, its "offset" field tracks the swap
handle which identifies where on the swap device it is. When the page
is stolen, this swapentry is put in the pte, so that the process can
do a do_swap_page on a "page-not-present" fault. Note that the SwapCache
is searched for by the same underlying routines that do PageCache
searching (__find_page), hence when a swap_in happens, the code ends
up setting PG_referenced, waiting for PG_locked to go off [[ but not
PG_uptodate? ]]

Shm page stealing:

shm pages are swapped a little differently. The shm data structure
keeps a reference on each page in core controlled by it.
try_to_swap_out just releases the reference from user program on these
pages, cleaning the pte and shm_swapout does no io. do_try_to_free_pages
invokes shm_swap, which will identify pages that it controls, and which
no one else (including any other user) has a reference on, and free the
page after getting a swaphandle for it and completing io on it (since
this temporarily "puts" the page in the swapcache, a temp refcount inc
is done to protect the page from shrink_mmap freeing it out). These
pages are not put in the swapcache. When a user wants to access the
page, he invokes shm_nopage which will fire off a swap in request,
again making sure the page is not put into the swapcache. The shm code
uses the rw_swap_page_nocache code to mimic putting a page in the swap
cache (to fool rw_swap_page) by setting inode/offset/PG_swap_cache then
doing the rw_swap_page, and clearing off the inode/PG_swap_cache. Hence,
for shm pages too, the offset gives the swaphandle. Note that
rw_swap_page_nocache has to handle being invoked for locked page on
which io is in progress. So that shrink_mmap does not erroneously try
to free out a shm page that has been temporarily marked "inswapcache",
the shm swapping code temporarily bumps the page reference count.

File page stealing:

file pages are handled almost similarly. The filemap_swapout/
filemap_write_page increments the page reference count, passing the
page to kpiod to do a do_write_page and free the page. All the time,
the page is in the inode's page cache, and later shrink_mmap will find
the page and free it and remove it from the page cache.
