//
// Created by up0 on 2020/6/10.
//
#include <stdio.h>

#include "cmd.h" //用来处理命令行输入
#include "fs.h"

void handle_rmdir(int argc, const char **args) {
  if (argc != 1) {
    printf("rmdir 需且仅需一个参数\n");
  } else {
    my_rmdir(args[0]);
  }
}

void handle_mkdir(int argc, const char **args) {
  if (argc != 1) {
    printf("mkdir 需且仅需一个参数\n");
  } else {
    my_mkdir(args[0]);
  }
}

void handle_ls(int argc, const char **args) {
  if (argc == 1) {
    my_ls(args[0]);

  } else if (argc == 0) {
    my_ls("");
  } else {
    printf("ls 需且仅需一个参数\n");
  }
}

void handle_create(int argc, const char **args) {
  if (argc != 1) {
    printf("touch 需且仅需一个参数\n");
  } else {
    my_create(args[0]);
  }
}

void handle_rm(int argc, const char **args) {
  if (argc != 1) {
    printf("rm 需且仅需一个参数\n");
  } else {
    my_rm(args[0]);
  }
}

void handle_open(int argc, const char **args) {
  if (argc != 1) {
    printf("open 需且仅需一个参数\n");
  } else {
    my_open(args[0]);
  }
}

void handle_close(int argc, const char **args) {
  if (argc != 1) {
    printf("close 需且仅需一个参数\n");
  } else {
    my_close((int) strtol(args[0], NULL, 10));
  }
}

void handle_read(int argc, const char **args) {
  if (argc != 1) {
    printf("read 需且仅需一个参数\n");
  } else {
    my_read((int) strtol(args[0], NULL, 10));
  }
}

void handle_write(int argc, const char **args) {
  if (argc != 1) {
    printf("write 需且仅需一个参数\n");
  } else {
    my_write((int) strtol(args[0], NULL, 10));
  }
}

void handle_cd(int argc, const char **args) {
  if (argc != 1) {
    printf("cd 需且仅需一个参数\n");
  } else {
    my_cd(args[0]);
  }
}

void handle_exit(int argc, const char **args) {
  my_exit_sys();
}

void handle_ll(int argc, const char **args) {
  my_ll();
}

int main(int argc, const char *argv[]) {

  start_sys("../files/file");  //将文件系统加载到内存

  //Loop 循环解析输入,模拟shell
  Loop *c = loop_new();
  loop_before(print_tips);
  loop_batch(c, "touch", handle_create);
  loop_batch(c, "rm", handle_rm);             //loop_batch 可以注册多个参数的方法,如 rm a b c
  loop_batch(c, "rmdir", handle_rmdir);
  loop_batch(c, "mkdir", handle_mkdir);
  loop_route(c, "ls", handle_ls);
  loop_route(c, "ll", handle_ll);
  loop_batch(c, "open", handle_open);
  loop_batch(c, "close", handle_close);
  loop_route(c, "write", handle_write);
  loop_route(c, "read", handle_read);
  loop_route(c, "exit", handle_exit);
  loop_route(c, "cd", handle_cd);
  loop_run(c); //执行循环
  loop_free(c);

  return 0;
}