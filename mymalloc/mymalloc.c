#include <mymalloc.h>

#define ALIGN8(x) (((x) + 7) & ~7)
#define IS_ALLOCATED(s) (((s) & 1) != 0)
#define MARK_ALLOC(s) ((s) | 1)
#define MARK_FREE(s) ((s) & ~((size_t)1))
#define MIN_BLOCK (sizeof(free_block_t) + sizeof(size_t))

spinlock_t big_lock = {.status = UNLOCKED};
typedef struct free_block_t
{
    size_t size;
    struct free_block_t *next;
} free_block_t;
static free_block_t *free_list_head = NULL;

void *mymalloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    // 向上对齐 payload + 头部 + 尾部 各一个 size_t
    size_t total = ALIGN8(size) + 2 * sizeof(size_t);

    // 开锁！
    spin_lock(&big_lock);

    // 遍历找到符合大小的 free block
    free_block_t *prev = NULL;
    free_block_t *curr = free_list_head;
    while (curr)
    {
        size_t curr_sz = MARK_FREE(curr->size);
        if (curr_sz >= total)
        {
            free_block_t *chosen = curr;

            if (curr_sz - total >= MIN_BLOCK)
            {
                free_block_t *new_free = (free_block_t *)((char *)chosen + total);
                new_free->size = MARK_FREE(curr_sz - total);

                *(size_t *)((char *)new_free + MARK_FREE(new_free->size) - sizeof(size_t)) = new_free->size;
                new_free->next = chosen->next;

                // 标志占用
                chosen->size = MARK_ALLOC(total);

                *(size_t *)((char *)chosen + total - sizeof(size_t)) = chosen->size;

                // prev 指向新建的 free block
                if (prev)
                {
                    prev->next = new_free;
                }
                else
                {
                    free_list_head = new_free;
                }
            }
            else
            {
                // 标志占用
                chosen->size = MARK_ALLOC(curr_sz);

                *(size_t *)((char *)chosen + curr_sz - sizeof(size_t)) = chosen->size;

                if (prev)
                {
                    prev->next = curr->next;
                }
                else
                {
                    free_list_head = curr->next;
                }
            }
            spin_unlock(&big_lock);
            return (char *)chosen + sizeof(size_t);
        } // end if (curr_sz >= total)

        // 遍历更新
        prev = curr;
        curr = curr->next;
    }
    spin_unlock(&big_lock);
    return NULL;
}

void myfree(void *ptr)
{
    if (!ptr)
    {
        return;
    }
    free_block_t *block = (free_block_t *)((char *)ptr - sizeof(size_t));
    size_t sz = MARK_ALLOC(block->size); // 带 alloc 标志
    block->size = MARK_FREE(sz);
    *(size_t *)((char *)block + MARK_FREE(sz) - sizeof(size_t)) = block->size;
    spin_lock(&big_lock);

    block->next = free_list_head;
    free_list_head = block;

    free_block_t *next = (free_block_t *)((char *)block + MARK_FREE(block->size));
    if (!IS_ALLOCATED(next->size))
    {
        free_block_t *p = free_list_head, *q = NULL;
        while (p && p != next)
        {
            q = p;
            p = p->next;
        }
        if (p)
        {
            if (q)
                q->next = p->next;
            else
                free_list_head = p->next;
        }
        size_t new_sz = MARK_FREE(block->size) + MARK_FREE(next->size);
        block->size = new_sz;
        *(size_t *)((char *)block + new_sz - sizeof(size_t)) = new_sz;
    }

    spin_unlock(&big_lock);
}
