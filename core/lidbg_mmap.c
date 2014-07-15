#include <linux/types.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>

//kmalloc
#define USE_KMALLOC


void *mmap_buf;
int mmap_size;

int mmap_alloc(int require_buf_size)
{
    struct page *page;
    mmap_size = PAGE_ALIGN(require_buf_size);

#ifdef USE_KMALLOC //for kmalloc
    mmap_buf = kzalloc(mmap_size, GFP_KERNEL);
    if (!mmap_buf)
    {
        return -1;
    }
    for (page = virt_to_page(mmap_buf ); page < virt_to_page(mmap_buf + mmap_size); page++)
    {
        SetPageReserved(page);
    }
#else //for vmalloc
    mmap_buf  = vmalloc(mmap_size);
    if (!mmap_buf )
    {
        return -1;
    }
    for (i = 0; i < mmap_size; i += PAGE_SIZE)
    {
        SetPageReserved(vmalloc_to_page((void *)(((unsigned long)mmap_buf) + i)));
    }
#endif

    return 0;
}
/*
//2.????????

int test_mmap()
{
  mmap_fd = open("/dev/mmap_dev", O_RDWR);
  mmap_ptr = mmap(NULL, mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, mmap_fd, 0);
  if (mmap_ptr == MAP_FAILED) {
    return -1;
  }
  return 0;
}
*/

//3.????????: ??file_operations?mmap??
static int mmap_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int ret;
    unsigned long pfn;
    unsigned long start = vma->vm_start;
    unsigned long size = PAGE_ALIGN(vma->vm_end - vma->vm_start);

    if (size > mmap_size || !mmap_buf)
    {
        return -EINVAL;
    }

#ifdef USE_KMALLOC
    return remap_pfn_range(vma, start, (virt_to_phys(mmap_buf) >> PAGE_SHIFT), size, PAGE_SHARED);
#else
    /* loop over all pages, map it page individually */
    while (size > 0)
    {
        pfn = vmalloc_to_pfn(mmap_buf);
        if ((ret = remap_pfn_range(vma, start, pfn, PAGE_SIZE, PAGE_SHARED)) < 0)
        {
            return ret;
        }
        start += PAGE_SIZE;
        mmap_buf += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
#endif
    return 0;
}

/*
//4.??????????

void test_munmap()
{
  munmap(mmap_ptr, mmap_size);
  close(mmap_fd);
}
*/

//5.????????; ?????????munmap?????????
void mmap_free()
{
#ifdef USE_KMALLOC
    struct page *page;
    for (page = virt_to_page(mmap_buf); page < virt_to_page(mmap_buf + mmap_size); page++)
    {
        ClearPageReserved(page);
    }
    kfree(mmap_buf);
#else
    int i;
    for (i = 0; i < mmap_size; i += PAGE_SIZE)
    {
        ClearPageReserved(vmalloc_to_page((void *)(((unsigned long)mmap_buf) + i)));
    }
    vfree(mmap_buf);
#endif
    mmap_buf = NULL;
}




