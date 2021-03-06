/* -LICENSE-START-
** Copyright (c) 2013 Blackmagic Design
**
** Permission is hereby granted, free of charge, to any person or organization
** obtaining a copy of the software and accompanying documentation covered by
** this license (the "Software") to use, reproduce, display, distribute,
** execute, and transmit the Software, and to prepare derivative works of the
** Software, and to permit third-parties to whom the Software is furnished to
** do so, all subject to the following:
** 
** The copyright notices in the Software and this entire statement, including
** the above license grant, this restriction and the following disclaimer,
** must be included in all copies of the Software, in whole or in part, and
** all derivative works of the Software, unless such copies or derivative
** works are solely in the form of machine-executable object code generated by
** a source language processor.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
** -LICENSE-END-
*/
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
#include <linux/vmalloc.h>
#endif
#include "bm_mm.h"
#include "bm_util.h"

struct bm_user_mem
{
	size_t length;
	struct page* pages[0]; // First address
};

struct bm_dma_list
{
	uint32_t contig:1,
	       length:31;
	dma_addr_t addrs[0]; // First address
};

struct bm_mmap
{
	vm_address_t vaddr;
	vm_address_t paddr;
	bm_user_mem_t umem; // Must be last member in struct
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
	#define bm_pci_dma_mapping_error(dev, addr) pci_dma_mapping_error(addr)
#else
	#define bm_pci_dma_mapping_error(dev, addr) pci_dma_mapping_error(dev, addr)
#endif

static bm_mm_stats_t statistics = { {0}, {0}, {0}, {0} };

static inline size_t get_num_pages(void* addr, size_t size)
{
	vm_address_t first = ((vm_address_t)addr) >> PAGE_SHIFT;
	vm_address_t last = ((vm_address_t)addr + size - 1) >> PAGE_SHIFT;
	return last - first + 1;
}

static void bm_put_user_pages(bm_user_mem_t* umem, bool dirty)
{
	size_t i;
	struct page *p;
	
	for (i = 0; i < umem->length; i++)
	{
		p = umem->pages[i];
		if (p)
		{
			if (dirty)
				SetPageDirty(p);

			put_page(p);
			bm_atomic_sub(&statistics.pages_held, 1);
		}
	}
}

void bm_mm_put_user_pages(bm_user_mem_t* umem, bool dirty)
{
	bm_put_user_pages(umem, dirty);
	kfree(umem);
}

static bool bm_get_user_pages(bm_user_mem_t* umem, struct task_struct* task, void* address, size_t length, bool write)
{
	int ret;
	umem->length = length;

	if (!task->mm)
		return false;

	down_read(&task->mm->mmap_sem);
		ret = get_user_pages(task, task->mm, (unsigned long)address & PAGE_MASK, umem->length, write, 0, umem->pages, NULL);
	up_read(&task->mm->mmap_sem);

	if (ret < length)
	{
		bm_mm_put_user_pages(umem, 0);
		return false;
	}

	bm_atomic_add(&statistics.pages_held, length);

	return true;
}

bm_user_mem_t* bm_mm_get_user_pages(struct task_struct* task, void* address, vm_size_t size, bool write)
{
	bm_user_mem_t* umem;
	size_t length = get_num_pages(address, size);

	if (!task)
		return NULL;

	umem = kzalloc(sizeof(bm_user_mem_t) + length * sizeof(struct page*), GFP_KERNEL);
	if (!umem)
		return NULL;

	if (!bm_get_user_pages(umem, task, address, length, write))
		return NULL;

	return umem;
}

bm_mmap_t* bm_mmap_map(struct task_struct* task, void* address, vm_size_t size, bool write)
{
	bm_mmap_t* mmap;
	size_t length = get_num_pages(address, size);

	if (!task)
		return NULL;

	mmap = kzalloc(sizeof(bm_mmap_t) + length * sizeof(struct page*), GFP_KERNEL);
	if (!mmap)
		return NULL;

	if (!bm_get_user_pages(&mmap->umem, task, address, length, write))
		return NULL;

	mmap->paddr = (vm_address_t)vmap(mmap->umem.pages, mmap->umem.length, VM_MAP, PAGE_SHARED);
	if (!mmap->paddr)
	{
		bm_put_user_pages(&mmap->umem, 0);
		return NULL;
	}

	bm_atomic_add(&statistics.pages_vmapped, mmap->umem.length);

	mmap->vaddr = mmap->paddr + offset_in_page(address);

	return mmap;
}

void bm_mmap_unmap(bm_mmap_t* mmap)
{
	bm_atomic_sub(&statistics.pages_vmapped, mmap->umem.length);
	vunmap((void*)mmap->paddr);
	bm_put_user_pages(&mmap->umem, 0);
	kfree(mmap);
}

vm_address_t bm_mmap_get_vaddress(bm_mmap_t* mmap)
{
	return mmap->vaddr;
}

static bm_dma_list_t* alloc_dma_list(size_t n_pages)
{
	bm_dma_list_t* dlist;
	dlist = kzalloc(sizeof(struct bm_dma_list) + (sizeof(dma_addr_t) * n_pages), GFP_KERNEL);
	if (dlist)
	{
		dlist->length = n_pages;
	}
	return dlist;
}

bm_dma_list_t* bm_dma_map_user_buffer(bm_pci_device_t* pci, bm_user_mem_t* umem, bm_dma_direction_t dir)
{
	bm_dma_list_t* dlist;
	size_t i;

	dlist = alloc_dma_list(umem->length);
	if (!dlist)
		return NULL;

	for (i = 0; i < umem->length; ++i)
	{
		dlist->addrs[i] = pci_map_page(pci->pdev, umem->pages[i], 0, PAGE_SIZE, dir);
		if (bm_pci_dma_mapping_error(pci->pdev, dlist->addrs[i]))
		{
			bm_dma_unmap_buffer(pci, dlist, dir);
			return NULL;
		}
		bm_atomic_add(&statistics.pages_mapped, 1);
	}

	return dlist;
}

bm_dma_list_t* bm_dma_map_kernel_buffer(bm_pci_device_t* pci, void* addr, vm_size_t size, bm_dma_direction_t dir)
{
	bm_dma_list_t* dlist;

	dlist = alloc_dma_list(1);
	if (!dlist)
		return NULL;

	dlist->contig = 1;
	dlist->length = size;

	dlist->addrs[0] = pci_map_single(pci->pdev, addr, size, dir);
	if (bm_pci_dma_mapping_error(pci->pdev, dlist->addrs[0]))
	{
		bm_dma_unmap_buffer(pci, dlist, dir);
		return NULL;
	}

	bm_atomic_add(&statistics.memory_mapped, size);

	return dlist;
}

bm_dma_list_t* bm_dma_map_kernel_buffer_vmalloc(bm_pci_device_t* pci, void* addr, vm_size_t size, bm_dma_direction_t dir)
{
	size_t n_pages = get_num_pages(addr, size);
	vm_address_t aligned_addr = (vm_address_t)addr & PAGE_MASK;
	bm_dma_list_t* dlist;
	size_t offset = 0;
	size_t i;

	dlist = alloc_dma_list(n_pages);
	if (!dlist)
		return NULL;

	dlist->contig = 0;

	for (i = 0; i < n_pages; ++i, offset += PAGE_SIZE)
	{
		struct page* page = vmalloc_to_page((void*)(aligned_addr + offset));
		dlist->addrs[i] = pci_map_page(pci->pdev, page, 0, PAGE_SIZE, dir);
		if (bm_pci_dma_mapping_error(pci->pdev, dlist->addrs[i]))
		{
			bm_dma_unmap_buffer(pci, dlist, dir);
			return NULL;
		}
		bm_atomic_add(&statistics.pages_mapped, 1);
	}

	return dlist;
}

void bm_dma_unmap_buffer(bm_pci_device_t* pci, bm_dma_list_t* dlist, bm_dma_direction_t dir)
{
	size_t i;
	if (dlist->contig)
	{
		pci_unmap_single(pci->pdev, dlist->addrs[0], dlist->length, dir);
		bm_atomic_sub(&statistics.memory_mapped, dlist->length);
	}
	else
	{
		for (i = 0; i < dlist->length; ++i)
		{
			if (dlist->addrs[i])
			{
				pci_unmap_page(pci->pdev, dlist->addrs[i], PAGE_SIZE, dir);
				bm_atomic_sub(&statistics.pages_mapped, 1);
			}
		}
	}

	kfree(dlist);
}

addr64_t bm_dma_get_physical_segment(const bm_dma_list_t* dlist, void* addr, vm_offset_t offset, uint32_t* len)
{
	size_t page_n;
	vm_offset_t page_offset;

	if (dlist->contig)
	{
		if (len && *len > dlist->length - offset)
			*len = dlist->length - offset;
		return dlist->addrs[0] + offset;
	}

	page_n = (offset_in_page(addr) + offset) >> PAGE_SHIFT;
	page_offset = offset_in_page((vm_address_t)addr + offset);

	if (page_n > dlist->length)
		return 0;

	if (len)
	{
		if (*len > PAGE_SIZE - page_offset)
			*len = PAGE_SIZE - page_offset;
	}

	return dlist->addrs[page_n] + page_offset;
}

vm_address_t bm_mm_phys_to_virt(addr64_t phys)
{
	return (vm_address_t)phys_to_virt(phys);
}

const bm_mm_stats_t* bm_mm_statistics(void)
{
	return &statistics;
}
