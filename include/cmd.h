//
// Created by up0 on 2020/6/10.
//
#ifndef FS__Loop_H_
#define FS__Loop_H_

//命令行循环解析,依赖sds字符串 vec动态数组和linux TAILQ

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "sds.h"
#include "vec.h"
#include "queue.h"

//命令行解析库回调函数
//参数为sds数组

//执行回调
typedef void (*CB)(int, const char **);
//执行前回调
typedef void(*CB_before)();

static CB_before cbb = NULL;

enum Mode{
  once = 1,
  batch
};

struct LoopList {
  char *name;
  enum Mode mode;
  CB cb;
  TAILQ_ENTRY(LoopList) next;
};

//定义命令列表的表头
typedef TAILQ_HEAD(Loop, LoopList) Loop;

//新建一个命令行解析类
Loop *loop_new() {
  Loop *head = (Loop *) malloc(sizeof(Loop));
  TAILQ_INIT(head);
  return head;
}

//删除所有命令链表
void loop_free(Loop *head) {
  struct LoopList *node;
  TAILQ_FOREACH(node, head, next) {
    TAILQ_REMOVE(head, node, next);
    free(node);
  }
}

//注册执行前回调
void loop_before(CB_before cb){
  if(cbb==NULL){
    cbb = cb;
  }
}
//如果在堆上分配name变量，请自行清除
void loop_route(Loop *head, const char *name, const CB cb) {
  struct LoopList *c = (struct LoopList *) malloc(sizeof(struct LoopList));
  c->name = strdup(name);
  c->cb = cb;
  c->mode = once;
  TAILQ_INSERT_TAIL(head, c, next);
}


void loop_batch(Loop *head, const char *name, const CB cb) {
  struct LoopList *c = (struct LoopList *) malloc(sizeof(struct LoopList));
  c->name = strdup(name);
  c->cb = cb;
  c->mode = batch;
  TAILQ_INSERT_TAIL(head, c, next);
}

//执行命令解析循环
void loop_run(Loop *head) {
  char buf[1024];
  for (;;) {
    if(cbb!=NULL){
      cbb();
    }
    sds line = sdsempty();//存储每次输入的命令，动态长度
    int nread = read(fileno(stdin), buf, 1024);
    if (nread == 0) {
      break;
    } else if (nread == -1) {
      perror("LoopList error: reading from standard input");
      exit(1);
    }
    line = sdscatlen(line, buf, nread);

    int count = 1;
    vec_void_t v;
    vec_init(&v);
    char *token = strtok(line, " ");
    vec_push(&v, token);
    while (token) {
      token = strtok(NULL, " ");
      if (token) {
        vec_push(&v, token);
        count++;
      }
    }

    int flag = 0;
    if (strcmp(v.data[v.length - 1], "\n") == 0) { //去除\n被当做一个分组的情况
      count--;
      if (count == 0) { //没有输入的情况
        goto next;
      }
      flag = 1;
    }

    char **tokens = (char **) malloc(sizeof(char *) * count);
    for (int i = 0; i < count; i++) {
      tokens[i] = (char *) v.data[i];
    }

    if (!flag) {
      //去除最后的\n
      tokens[count - 1][strlen(tokens[count - 1]) - 1] = '\0';
    }

    struct LoopList *node;
    TAILQ_FOREACH(node, head, next) {
      if (strcmp(node->name, tokens[0]) == 0) {
        if(node->mode == once){
          node->cb(count - 1, (const char **) (tokens + 1));
          goto next;
        }else if(node->mode == batch){
          for(int i = 1;i<count;i++){
            node->cb(1, (const char **) (tokens + i));
          }
          goto next;
        }
      }
    }
    printf("LoopList error : no such LoopList:%s\n", tokens[0]);

    next: //进入下一轮循环
    sdsfree(line);
    vec_deinit(&v);
  }
}

#endif //FS__Loop_H_
