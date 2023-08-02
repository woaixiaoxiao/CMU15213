#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
  int valid;
  int tag;
  int time_stamp;
} cache_line;

// cache中set的数量为2^s
int s, S;
// 每个cache有多少行
int E;
// 每行可以存储2^b位数据
int b;
// tag的位数
int tag_count;
// 文件名
char *tracefile = NULL;
// 读取文件的指针
FILE *file_ptr;
// 是否输入可选参数h和v
int h_flag = 0, v_flag = 0;
// cache，得到S和E之后，就可以给它malloc空间
// S行，每个S有E行
cache_line **cache;
// 记录答案，命中，不命中，淘汰的次数
int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;
// 时间戳
int time_stamp = 0;

void LoadOrEvict(cache_line *cur_set, int index, int tag) {
  cur_set[index].valid = 1;
  cur_set[index].tag = tag;
  cur_set[index].time_stamp = time_stamp++;
}

void FindCache(int set_num, int tag) {
  cache_line *cur_set = cache[set_num];
  int empty_index = -1;
  int min_ts_line_index = 0;
  for (int i = 0; i < E; i++) {
    // 当前行存在于cache中，即valid=1，并且tag相同
    if (cur_set[i].valid == 1 && cur_set[i].tag == tag) {
      hit_count++;
      // 记得更新时间戳
      cur_set[i].time_stamp = time_stamp++;
      return;
    }
    // 如果存在空行，即valid=0；
    if (cur_set[i].valid == 0) {
      empty_index = i;
    }
    // 记录时间戳最小的，实在不行就要去替换了
    if (cur_set[i].time_stamp < cur_set[min_ts_line_index].time_stamp) {
      min_ts_line_index = i;
    }
  }
  // 如果没有命中，那么就发生了miss
  miss_count++;
  // 载入某一行，如果有空行，就载入空行
  // 如果没有空行，则载入时间戳最小的，这就发生了evict
  if (empty_index != -1) {
    LoadOrEvict(cur_set, empty_index, tag);
  } else {
    LoadOrEvict(cur_set, min_ts_line_index, tag);
    eviction_count++;
  }
}

void Func(char command, int address, int size) {
  int set_num = (address >> b) & ((1 << s) - 1);
  int tag = (address >> (b + s)) & ((1 << (32 - b - s)) - 1);
  FindCache(set_num, tag);
  if (command == 'M') {
    FindCache(set_num, tag);
  }
}

int parser(int argc, char *argv[]) {
  // 解析参数
  int opt;
  while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
    switch (opt) {
    case 'h':
      h_flag = 1;
      break;
    case 'v':
      v_flag = 1;
      break;
    case 's':
      s = atoi(optarg);
      break;
    case 'E':
      E = atoi(optarg);
      break;
    case 'b':
      b = atoi(optarg);
      break;
    case 't':
      tracefile = optarg;
      break;
    case '?':
      printf("未知选项或缺少参数\n");
      return -1;
    }
  }
  return 0;
}

int GetOperation() {
  // 读取文件，获得访问记录
  file_ptr = fopen(tracefile, "r");
  if (file_ptr == NULL) {
    printf("无法打开文件 %s\n", tracefile);
    return -1;
  }
  char buffer[100];
  char command;
  int address;
  int size;
  while (fgets(buffer, sizeof(buffer), file_ptr)) {
    if (buffer[0] == 'I') {
      continue;
    }
    if (sscanf(buffer, " %c %x,%d", &command, &address, &size) == 3) {
      // 成功获取操作
      Func(command, address, size);
      // printf("%c %x,%d\n", command, address, size);
    } else {
      // 输入不合法
      return -1;
    }
  }
  fclose(file_ptr);
  return 0;
}

void Init() {
  S = 1 << s;
  cache = (cache_line **)malloc(S * sizeof(cache_line *));
  for (int i = 0; i < S; i++) {
    cache[i] = (cache_line *)malloc(E * sizeof(cache_line));
    // 记得初始化每一行的tag为0
    for (int j = 0; j < E; j++) {
      cache[i][j].tag = 0;
    }
  }
}

void Destory() {
  for (int i = 0; i < S; i++) {
    free(cache[i]);
  }
  free(cache);
}

int main(int argc, char *argv[]) {
  // 解析命令行参数
  int par_res = parser(argc, argv);
  if (par_res == -1) {
    return 1;
  }
  // 创造cache，初始化cache
  Init();
  // 读取文件，获得操作，进行操作
  int get_res = GetOperation();
  if (get_res == -1) {
    return 1;
  }
  // 输出结果
  printSummary(hit_count, miss_count, eviction_count);
  // 释放malloc申请的变量
  Destory();
  return 0;
}
