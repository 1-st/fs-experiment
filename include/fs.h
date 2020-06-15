//
// Created by up0 on 2020/6/10.
//

#ifndef FS__FS_H_
#define FS__FS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define FREE 0         //FAT中的文件空闲标志
#define END 65535      //FAT中的文件结束标志
#define DISKSIZE 1024000   //虚拟磁盘大小
#define BLOCKNUM 995   //数据块数量
#define BLOCKSIZE 1024 //数据块单块长度
#define MAXOPENFILE 10 //用户最多打开的文件数量

#define FS_API   //用户操作直接造访的函数
#define UNAME "root"  //shell提示的用户名字
#define BLOCK0INFO "size:1024;len:1000;" //自己加的信息
#define ATTRDIR 0 //定义文件属性为目录
#define ATTRFILE 1 //定义文件属性为文件

typedef struct FAT {
  unsigned short id;
} fat;

typedef struct FCB {
  char free; // 此fcb是否已被删除，因为把一个fcb从磁盘块上删除是很费事的，所以选择利用fcb的free标号来标记其是否被删除
  char filename[80];
  char exname[3];
  unsigned short time;
  unsigned short date;
  unsigned short first; // 文件起始盘块号
  unsigned long length; // 文件的实际长度
  unsigned char attribute; // 文件属性字段：为简单起见，我们只为文件设置了两种属性：值为 0 时表示目录文件，值为 1 时表示数据文件
} fcb;

typedef struct USEROPEN {
  fcb fcb; // 文件的 FCB 中的内容
  int count; // 读写指针在文件中的位置 本实例并未使用
  char fcb_state; // 是否修改了文件的 FCB 的内容，如果修改了置为 1，否则为 0 本实例并未使用
  char t_openfile; // 表示该用户打开表项是否为空，若值为 0，表示为空，否则表示已被某打开文件占据
  char dir[80]; // 打开文件的绝对路径名，这样方便快速检查出指定文件是否已经打开
} useropen;

typedef struct BLOCK0 {
  char info[200];      //自定义信息
  unsigned short root; //根目录的盘块号
  unsigned long root_length;// 根目录的长度
//  unsigned char *start_block;
} block0;

//定义虚拟文件系统结构体，保存一些状态
struct MyFS {
  //打开的实际系统文件
  int fd;
  //指向虚拟磁盘内存区的开始
  unsigned char *myvhard;
  //数据区开始的位置
  unsigned char *startp;
  //用户打开的文件
  useropen open_file_list[MAXOPENFILE];
  //虚拟磁盘文件路径
  char *path;
  // 指向用户打开文件表中的当前目录所在打开文件表项的位置
  int cur_dir_id;
  //block0
  block0 b0;
  //fat表
  fat fat1[BLOCKNUM];
  fat fat2[BLOCKNUM];
};

//包装fcb和fcb的位置信息
typedef struct fcb_n_pos {
  int block;
  int offset;
  fcb fcb;
} fcb_n_pos;

FS_API void start_sys(char *path);
FS_API void my_exit_sys();
FS_API void my_create(const char *filename);
FS_API void my_rm(const char *path);
FS_API void my_mkdir(const char *dirname);
FS_API void my_rmdir(const char *path);
FS_API void my_ls(const char *path);
FS_API void my_ll();
FS_API void my_cd(const char *dirname);
FS_API int my_open(const char *path);
FS_API void my_close(int fd);
FS_API int my_read(int fd);
FS_API int my_write(int fd);
FS_API void print_tips();

static void my_format();
static int do_read(int fd);
static int do_write(int fd, char *text);

//辅助函数

//打开文件的辅助函数
static int open_file(const char *name);
//得到一个目录文件下的所有非空fcb
static fcb *get_good_fcb(fcb dir, int *n);
//为一个空的fcb分配一个新文件,同时修改fcb
static void add_empty_file(fcb father, fcb *fcb, const char *name, int attr);
//得到一个新的空fat表项,并且设置表项为非空,失败返回-1
static int request_empty_block();
//将fcb映射到虚拟磁盘上
static void set_fcb(fcb fcb, int block, int offset);
//得到目录文件所有的fcb和位置
static fcb_n_pos *get_all_fcb_n_pos(fcb dir, int *n);
//得到目录文件所有的fcb
static fcb *get_all_fcb(fcb dir, int *n);
//添加新的盘块
static void add_block(int first, int block);
//在fcb指定的目录创建文件或目录
static void create_file(fcb *dir, const char *name, int attr);
//根据路径创建文件或目录
static void create_file_path(const char *path, int attr);
//更新.目录长度
static void update_self_dir_len(fcb c, int len);
//寻找一个子目录，切换一次目录
static int cd_once(useropen *o, const char *name);
//切换到根目录
static void cd_root(useropen *o);
//得到一个目录的fcb数量
static int get_dir_total(fcb dir);
//切换目录的核心函数
static int cd(const char *dirname);
//列出当前目录下的所有文件
static void ls();
//删除当前目录的指定文件或者目录
static void rm(const char *path, int attr);
//删除指定目录的指定文件或者目录
static void cd_and_rm(const char *path, int attr);
//得到一个目录文件的有效fcb和有效fcb的位置
static fcb_n_pos *get_good_fcb_n_pos(fcb dir, int *n);
//释放一个文件的所有fat
static void free_fat(int start);
//更新根目录的..
static void update_root_dir_pre(int add);
//找出一个未被使用的文件描述符
static int find_free_fd();
//将openlist中fcb的变动写入到磁盘上
static void update_openlist_fcb(useropen o);

#endif //FS__FS_H_
