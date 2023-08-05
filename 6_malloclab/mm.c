/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
// 一个内存块4个字节
#define WSIZE 4
// 8字节
#define DSIZE 8
// 堆申请虚拟内存时的默认大小
#define CHUNKSIZE (1 << 12)
// max函数
#define MAX(x, y) ((x) > (y) ? (x) : (y))
// 通过size和alloc计算头部和尾部具体的值，即打包
#define PACK(size, alloc) ((size) | (alloc))
// 获取地址p处的32位unsign int值
#define GET(p) (*(unsigned int *)(p))
// 将地址p处的值置为val
#define PUT(p, val) (*(unsigned int *)(p) = (val))
// 获取地址p处的size，这里p应该是指向头或者指向尾
#define GET_SIZE(p) (GET(p) & (~0x7))
// 获取这个块是否已经被分配
#define GET_ALLOC(p) (GET(p) & 0x1)
// 根据空闲块真正的起始位置，找到这个空闲块的头部和尾部
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
// 找到下一块和上一块的起始地址，真正的起始地址，而不是头部或者尾部
// 一个块真正的起始位置，加上这个块的首部记录的长度，就直接飞到下一块真正的起始位置去了
// 一个块真正的起始位置，减去上一块的首部记录的上次，直接飞到上一块真正的起始位置去了
#define HDRP_PRE(bp) ((char *)(bp)-DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(HDRP_PRE(bp)))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// 第一个格子放1 第二个格子放2,3 第三个格子放4 5 6 7
// 即第i个格子放[2^i,2^(i+1))
#define LIST_MAX_SIZE 16
void *segregated_free_lists[LIST_MAX_SIZE];
// 这里组织空闲链表的方式是按大致的长度来的，
// 而空闲链表之间的next和pre的关系其实是存在bp开始的第一和第二块中
// 向p地址指向的块填入地址ptr
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))
// 给出前驱和后继结点的地址所在的地址
#define GET_PRE_PTR(ptr) ((char *)(ptr))
#define GET_SUCC_PTR(ptr) ((char *)(ptr) + WSIZE)
// 获得前驱和后继结点
#define GET_PRE(ptr) (*(char **)(ptr))
#define GET_SUCC(ptr) (*(char **)(GET_SUCC_PTR(ptr)))

int find_pos(size_t size) {
  int pos = 0;
    for (pos = 0; pos < LIST_MAX_SIZE - 1; pos++) {
      if (size > 1) {
        size = size >> 1;
      } else {
        break;
      }
    }
  return pos;
}

static void insert_node(void *ptr, size_t size) {
  // 首先找到这个size应该在哪个格子里
  int pos = find_pos(size);
  // 扫描这个格子里存的链表，找到应该插入的位置
  void *pre_ptr = NULL;
  void *cur_ptr = segregated_free_lists[pos];
  while (cur_ptr != NULL && size > GET_SIZE(HDRP(cur_ptr))) {
    pre_ptr = cur_ptr;
    cur_ptr = GET_SUCC(cur_ptr);
  }
  // 分情况讨论，正常来说，我们应该插入pre_ptr和cur_ptr之间
  // 前驱比自己小，后续比自己大
  // 如果pre_ptr为空
  if (pre_ptr == NULL) {
    // 如果cur_ptr也为空，说明这个链表就是空的，直接插入即可
    if (cur_ptr == NULL) {
      segregated_free_lists[pos] = ptr;
      SET_PTR(GET_PRE_PTR(ptr), NULL);
      SET_PTR(GET_SUCC_PTR(ptr), NULL);
    } else {
      // cur_ptr不为空，说明要插入的是第一个位置
      segregated_free_lists[pos] = ptr;
      SET_PTR(GET_PRE_PTR(cur_ptr), ptr);
      SET_PTR(GET_SUCC_PTR(ptr), cur_ptr);
      SET_PTR(GET_PRE_PTR(ptr), NULL);
    }
    // pre_ptr不为空
  } else {
    // 如果cur_ptr为空，说明是在链表尾部插入
    if (cur_ptr == NULL) {
      SET_PTR(GET_SUCC_PTR(pre_ptr), ptr);
      SET_PTR(GET_PRE_PTR(ptr), pre_ptr);
      SET_PTR(GET_SUCC_PTR(ptr), NULL);
    } else {
      // 如果cur_ptr不为空，说明是在链表中间插入
      SET_PTR(GET_SUCC_PTR(pre_ptr), ptr);
      SET_PTR(GET_PRE_PTR(ptr), pre_ptr);
      SET_PTR(GET_SUCC_PTR(ptr), cur_ptr);
      SET_PTR(GET_PRE_PTR(cur_ptr), ptr);
    }
  }
}
// 在空闲链表中删除这个ptr
static void delete_node(void *bp) {
  // 首先找到这个ptr在数组中的下标
  int pos = find_pos(GET_SIZE(HDRP(bp)));
  // 开始分类讨论删除
  // 如果这个节点的pre为null
  if (GET_PRE(bp) == NULL) {
    // 如果这个节点的succ也为null
    if (GET_SUCC(bp) == NULL) {
      segregated_free_lists[pos] = NULL;
    } else {
      // 如果succ不为null
      segregated_free_lists[pos] = GET_SUCC(bp);
      SET_PTR(GET_PRE_PTR(GET_SUCC(bp)), NULL);
    }
  } else {
    // 节点的pre不为null
    // 如果节点的succ为null
    if (GET_SUCC(bp) == NULL) {
      SET_PTR(GET_SUCC_PTR(GET_PRE(bp)), NULL);
    } else {
      // 如果succ不为null，即中间节点
      SET_PTR(GET_SUCC_PTR(GET_PRE(bp)), GET_SUCC(bp));
      SET_PTR(GET_PRE_PTR(GET_SUCC(bp)), GET_PRE(bp));
    }
  }
}

static void *coalesce(void *bp) {
  // 首先获取前后内存块的状态
  int is_pre_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
  int pre_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
  int is_next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  int next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
  int cur_size = GET_SIZE(HDRP(bp));
  // 根据状态分类讨论
  int new_size;
  // 前后均分配了
  if (is_pre_alloc && is_next_alloc) {
    return bp;
  } else if (is_pre_alloc && !is_next_alloc) {
    // 前面分配了，后面没分配，所以和后面合并
    // 首先，在空闲块数组中删除这两个块
    delete_node(bp);
    delete_node(NEXT_BLKP(bp));
    // 然后修改首部和尾部
    new_size = cur_size + next_size;
  } else if (!is_pre_alloc && is_next_alloc) {
    delete_node(bp);
    delete_node(PREV_BLKP(bp));
    new_size = pre_size + cur_size;
    // 修改当前的bp，因为和前面合并了，现在的bp应该指向前面的块的头部
    bp = PREV_BLKP(bp);
  } else {
    // 前后都是空的
    delete_node(bp);
    delete_node(PREV_BLKP(bp));
    delete_node(NEXT_BLKP(bp));
    new_size = pre_size + cur_size + next_size;
    bp = PREV_BLKP(bp);
  }
  // 修改当前合并后的空闲块的首部和尾部
  PUT(HDRP(bp), PACK(new_size, 0));
  // 只要设置好了头部，那么尾部就可以直接操作
  PUT(FTRP(bp), PACK(new_size, 0));
  // 将这个空闲块插入空闲块数组
  insert_node(bp, new_size);
  return bp;
}

static void *extend_heap(size_t size) {
  // word_count必须是偶数
  size = ALIGN(size);
  // 使用mem_sbrk去申请空间
  void *bp;
  if ((bp = mem_sbrk(size)) == (void *)-1) {
    return NULL;
  }
  // 将新申请的内存加到已有的堆上去
  // 现在有个指针bp，它指向的新申请的内存的真正的起始位置
  // 正常来说，这个bp的前一块就是之前堆的结尾块，是没用的，现在直接拿来做新申请的块的首部
  // 然后又将新申请的块的最后一块变成堆的尾部
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
  // 将这个新的空闲块插入到我们的列表里
  insert_node(bp, size);
  return coalesce(bp);
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  // 初始化记录空闲块的数组
  for (int i = 0; i < LIST_MAX_SIZE; i++) {
    segregated_free_lists[i] = NULL;
  }
  // 先请求空间来初始化堆的结构
  static char *heap_listp;
  if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)(-1)) {
    return -1;
  }
  PUT(heap_listp, 0);
  PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
  heap_listp += 2 * WSIZE;
  // 为堆申请一个chunksize的虚存空间
  if (extend_heap(CHUNKSIZE) == NULL) {
    return -1;
  }
  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

static void *place(void *bp, size_t size) {
  // 在bp中分配size大小的空闲块走
  // 先在数组中删除bp空闲块
  delete_node(bp);
  // 获得bp块的长度
  size_t free_size = GET_SIZE(HDRP(bp));
  // 如果剩下的小于2*DSIZE，那就不用再插入回去了
  if (free_size - size < 2 * DSIZE) {
    PUT(HDRP(bp), PACK(free_size, 1));
    PUT(FTRP(bp), PACK(free_size, 1));
  } else if (size >= 96) {
    PUT(HDRP(bp), PACK(free_size - size, 0));
    PUT(FTRP(bp), PACK(free_size - size, 0));
    insert_node(bp, free_size - size);
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
  } else {
    // 注意，这里是把前半部分给分配了，不能修改bp，最后还是要返回bp
    // 前半部分要分配，后半部分重新插入
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
    // 修改bp指向后半段
    PUT(HDRP(NEXT_BLKP(bp)), PACK(free_size - size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(free_size - size, 0));
    // 重新放入空闲块数组
    insert_node(NEXT_BLKP(bp), free_size - size);
  }
  return bp;
}

void *mm_malloc(size_t size) {
  // 首先调整size为合法值，最小为2*DSIZE，否则一定要是8的倍数
  if (size == 0) {
    return NULL;
  } else if (size <= DSIZE) {
    size = 2 * DSIZE;
  } else {
    size = ALIGN(size + DSIZE);
  }
  // 根据size去空闲块数组里找最合适的那个
  int pos = find_pos(size);
  void *fit_ptr = NULL;
  while (pos < LIST_MAX_SIZE) {
    // 去当前项里面找
    void *cur_ptr = segregated_free_lists[pos];
    while (cur_ptr != NULL) {
      if (GET_SIZE(HDRP(cur_ptr)) < size) {
        cur_ptr = GET_SUCC(cur_ptr);
      } else {
        fit_ptr = cur_ptr;
        break;
      }
    }
    if (fit_ptr != NULL) {
      break;
    }
    pos++;
  }
  // 如果没有足够大的，说明堆要扩充空间了
  if (fit_ptr == NULL) {
    if ((fit_ptr = extend_heap(MAX(size, CHUNKSIZE))) == NULL) {
      return NULL;
    }
  }
  // 在该空闲块中分配size大小的块
  fit_ptr = place(fit_ptr, size);
  return fit_ptr;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
  // 修改标志位
  size_t size = GET_SIZE(HDRP(ptr));
  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));
  // 插入空闲列表
  insert_node(ptr, size);
  // 尝试合并
  coalesce(ptr);
}

void *mm_realloc(void *ptr, size_t size) {
  // 首先检查size的合法性
  if (size == 0) {
    return NULL;
  }
  // 修改size使其对齐
  if (size <= DSIZE) {
    size = 2 * DSIZE;
  } else {
    size = ALIGN(size + DSIZE);
  }
  // 计算当前块的大小与要求的size的差值
  int cur_size = GET_SIZE(HDRP(ptr));
  int change_size = cur_size - size;
  // 如果size小于等于当前长度，则不需要重新分配
  if (change_size >= 0) {
    return ptr;
  }
  // 如果当前块后面就是结尾
  if (GET_SIZE(HDRP(NEXT_BLKP(ptr))) == 0) {
    // 扩展
    if (extend_heap(MAX(change_size, CHUNKSIZE)) == NULL) {
      return NULL;
    }
    // 扩展成功，修改头尾部
    delete_node(NEXT_BLKP(ptr));
    PUT(HDRP(ptr), PACK(cur_size + MAX(change_size, CHUNKSIZE), 1));
    PUT(FTRP(ptr), PACK(cur_size + MAX(change_size, CHUNKSIZE), 1));
    return ptr;
  }
  // 如果当前块后面有个free块，尝试去合并，看看是不是可以
  if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr)))) {
    // 如果加起来长度够的话
    int next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
    if (cur_size + next_size >= size) {
      delete_node(NEXT_BLKP(ptr));
      PUT(HDRP(ptr), PACK(cur_size + next_size, 1));
      PUT(FTRP(ptr), PACK(cur_size + next_size, 1));
      return ptr;
    }
  }
  // 最后一步了，只能去重新申请
  void *new_ptr = mm_malloc(size);
  memcpy(new_ptr, ptr, cur_size);
  mm_free(ptr);
  return new_ptr;
}