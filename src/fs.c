//
// Created by up0 on 2020/6/10.
//
#include "fs.h"
#include "utils.h"
#include "sds.h"

static struct MyFS fs;

//如果文件存在,从文件读取到fs.myvhard
//如果文件不存在, 调用my_format()初始化fs.myvhard
//接着从fs,myvhard读取引导FAT等信息,并打开根目录
FS_API void start_sys(char *path) {
  fs.myvhard = (unsigned char *) malloc(sizeof(unsigned char) * DISKSIZE);
  fs.startp = fs.myvhard + BLOCKSIZE * 5; //在我的实现中数据区开始位置完全固定,所以不需要在引导块添加数据区开始位置
  fs.path = strdup(path);
  for (int i = 0; i < MAXOPENFILE; i++) {
    fs.open_file_list[i].t_openfile = 0; // 0表示该打开项目未被占据
  }

  //读取文件,判断是否已经格式化
  fs.fd = open(path, O_RDWR);
  if (fs.fd == -1) {
    if (errno == ENOENT) {//文件不存在
      my_format();
    } else {//其他错误
      printf("错误号:%d %s %s:%d", errno, strerror(errno), __FILE__, __LINE__);
      exit(1);
    }
  } else {
    int n_read = read(fs.fd, fs.myvhard, DISKSIZE);
    if (n_read == -1) {
      printf("错误号:%d %s %s:%d", errno, strerror(errno), __FILE__, __LINE__);
      exit(1);
    } else if (n_read > 0) {
      block0 b;
      memcpy(&b, fs.myvhard, sizeof(b));
      if (strcmp(b.info, BLOCK0INFO) != 0) {
        my_format();
      }
    } else {
      printf("文件损坏\n");
      exit(1);
    }
  }//读取或创建fs.myvhard

  //读取引导
  memcpy(&fs.b0, fs.myvhard, sizeof(block0));

  fcb root;
  root.length = fs.b0.root_length;
  root.first = fs.b0.root;
  strcpy(root.filename, "/");
  root.attribute = ATTRDIR;
  root.free = 0;

  //初始化用户当前目录
  fs.cur_dir_id = 0;
  //打开根目录
  fs.open_file_list[0].fcb = root;
  fs.open_file_list[0].count = 0;
  fs.open_file_list[0].t_openfile = 1;
  strcpy(fs.open_file_list[0].dir, "/");

  //读取FAT
  memcpy(fs.fat1, fs.myvhard + BLOCKSIZE * 1, sizeof(fs.fat1));
  memcpy(fs.fat2, fs.myvhard + BLOCKSIZE * 2, sizeof(fs.fat2));

}

// 退出系统，做好相应备份工作,同时释放内存
FS_API void my_exit_sys() {
  //写入FAT
  memcpy(fs.fat1, fs.myvhard + BLOCKSIZE * 1, sizeof(fs.fat1));
  memcpy(fs.fat2, fs.myvhard + BLOCKSIZE * 2, sizeof(fs.fat2));

  if (fs.fd == -1) {//如果读取时文件不存在磁盘,则创建文件
    fs.fd = creat(fs.path, 0644);
    write(fs.fd, fs.myvhard, DISKSIZE);
  } else {
    lseek(fs.fd, 0, SEEK_SET);//将文件指针置于起始处
    write(fs.fd, fs.myvhard, DISKSIZE);
  }
  free(fs.myvhard);
  printf("虚拟fs保存至 %s\n", fs.path);
  free(fs.path);
  exit(0);
}

//列出指定目录下的所有文件
FS_API void my_ls(const char *path) {
  if (strlen(path) != 0) {
    char origin_path[80];
    strcpy(origin_path, fs.open_file_list[fs.cur_dir_id].dir);
    cd(path);
    ls();
    cd(origin_path);
  } else {
    ls();
  }
}

//创建文件
FS_API void my_create(const char *path) {
  create_file_path(path, ATTRFILE);
}

// 调用mkdir创建出一个文件夹
FS_API void my_mkdir(const char *path) {
  create_file_path(path, ATTRDIR);
}

//切换目录
FS_API void my_cd(const char *dirname) {
  cd(dirname);
}

//删除指定文件
FS_API void my_rm(const char *path) {
  cd_and_rm(path, ATTRFILE);
}

//删除指定目录
FS_API void my_rmdir(const char *path) {
  cd_and_rm(path, ATTRDIR);
}

// 打开文件
FS_API int my_open(const char *path) {
  if (has_slash(path)) {
    char origin_path[80];
    strcpy(origin_path, fs.open_file_list[fs.cur_dir_id].dir);
    char cd2[80];
    strcpy(cd2, path);
    path_trim_last(cd2);

    char *name = path_get_name((char *) path);
    if (name == NULL) {
      printf("需要文件名\n");
      return -1;
    } else {
      cd(cd2);
      int fd = open_file(name);
      if (fd != -1) {
        printf("文件打开成功 fd = %d\n", fd);
      } else {
        printf("文件打开失败\n");
      }
      cd(origin_path);
    }
  } else {//只有一个文件名
    int fd = open_file(path);
    if (fd != -1) {
      printf("文件打开成功 fd = %d\n", fd);
    } else {
      printf("文件打开失败\n");
    }
  }
  return 0;
}

// 关闭一个打开文件
FS_API void my_close(int fd) {
  if (fd < 0 || fd >= MAXOPENFILE) {
    printf("fd 超出范围\n");
    return;
  }
  if (fd == fs.cur_dir_id) {
    printf("不能关闭当前目录\n");
    return;
  }
  fs.open_file_list[fd].t_openfile = 0;
}

//读取一个文件
FS_API int my_read(int fd) {
  if (fd < 0 || fd >= MAXOPENFILE) {
    printf("fd 超出范围\n");
    return -1;
  }
  if (fs.open_file_list[fd].fcb.attribute == ATTRDIR) {
    printf("不能从目录读数据\n");
    return -1;
  }
  do_read(fd);
  return 0;
}

// 把键盘输入的信息写入一个打开文件
FS_API int my_write(int fd) {

  if (fd < 0 || fd >= MAXOPENFILE) {
    printf("fd 超出范围\n");
    return -1;
  }
  if (fs.open_file_list[fd].fcb.attribute == ATTRDIR) {
    printf("不能往目录写数据\n");
    return -1;
  }

  sds write = sdsempty();
  char buf[1024];
  printf("输入quit结束读数据\n");
  for (;;) {
    int nread = read(fileno(stdin), buf, 1024);
    if (nread == 0) {
      break;
    } else if (nread == -1) {
      perror("读错误\n");
      return -1;
    }
    if (strlen(buf) >= 4 && strncmp(buf, "quit", 4) == 0) {
      break;
    }
    write = sdscatlen(write, buf, nread);
  }
  do_write(fd, write);
  sdsfree(write);
  return 0;
}

//打印所有信息,用于调试
FS_API void my_ll() {
  for (int i = 0; i < MAXOPENFILE; i++) {
    if (fs.open_file_list[i].t_openfile == 1) {
      printf("%d : %s\t", i, fs.open_file_list[i].dir);
    }
  }
  printf("\n");
  fcb cur_dir_fcb = fs.open_file_list[fs.cur_dir_id].fcb;
  int n = 0;//当前目录下的文件数量,包含目录和一般文件
  fcb_n_pos *FCBs = get_all_fcb_n_pos(cur_dir_fcb, &n);
  for (int i = 0; i < n; i++) {
    fcb_n_pos f = FCBs[i];
    if (f.fcb.attribute == ATTRDIR || strcmp(f.fcb.exname, "\0") == 0) {
      if (f.fcb.free) {
        printf("\033[31m%s [%s] block:%d offset:%d first:%d len:%lu\033[0m\n",
               f.fcb.attribute == ATTRDIR ? "d" : " ", f.fcb.filename, f.block, f.offset, f.fcb.first, f.fcb.length);
      } else {
        printf("%s [%s] block:%d offset:%d first:%d len:%lu\n",
               f.fcb.attribute == ATTRDIR ? "d" : " ", f.fcb.filename, f.block, f.offset, f.fcb.first, f.fcb.length);
      }
    } else {
      if (f.fcb.free) {
        printf("\033[31m%s [%s.%s] block:%d offset:%d first:%d len:%lu\033[0m\n",
               f.fcb.attribute == ATTRDIR ? "d" : " ",
               f.fcb.filename,
               f.fcb.exname,
               f.block,
               f.offset,
               f.fcb.first,
               f.fcb.length);
      } else {
        printf("%s [%s.%s] block:%d offset:%d first:%d len:%lu\n",
               f.fcb.attribute == ATTRDIR ? "d" : " ",
               f.fcb.filename,
               f.fcb.exname,
               f.block,
               f.offset,
               f.fcb.first,
               f.fcb.length);
      }
    }
  }
  printf("\n");
}

//因为使用动态字符串,所以不需要长度参数
static int do_read(int fd) {
  int block = fs.open_file_list[fd].fcb.first;//当前读取到的盘块号
  int rest = (int) fs.open_file_list[fd].fcb.length;//剩下要复制的长度
  char buf[BLOCKSIZE + 1];
  while (rest > 0 && block != END) {
    int copy = rest > BLOCKSIZE ? BLOCKSIZE : rest;//这次复制的长度
    memcpy(buf, fs.startp + BLOCKSIZE * block, copy);
    block = fs.fat1[block].id;
    buf[copy] = '\0';
    printf("%s", buf);
    rest -= copy;
  }
  fflush(stdout);
  return 0;
}

//往文件末尾追加
static int do_write(int fd, char *write_from) {
  int block = fs.open_file_list[fd].fcb.first;
  int len = (int) fs.open_file_list[fd].fcb.length;
  int n = len / BLOCKSIZE;
  while (n--) {
    block = fs.fat1[block].id;
  }
  int block_cur = len % BLOCKSIZE;//最后一个盘块的当前文件写入位置
  int rest = sdslen(write_from);
  int write_cur = 0;//指向write_from
  while (rest > 0) {
    int write = rest > (BLOCKSIZE - block_cur) ? (BLOCKSIZE - block_cur) : rest;//要写入的长度
    memcpy(fs.startp + block * BLOCKSIZE + block_cur, write_from + write_cur, write);
    rest -= write;
    if (rest > 0) {
      int new_block = request_empty_block();
      add_block(block, new_block);
      block = new_block;
    } else {
      break;
    }
  }
  fs.open_file_list[fd].fcb.length += sdslen(write_from);
  update_openlist_fcb(fs.open_file_list[fd]);//将fcb的变动写入到磁盘上
  return 0;
}

//将openlist中文件fcb的变动写入到磁盘上
//由于文件长度信息只在文件fcb中存储,
//而文件fcb只在父目录中存储
//所以需要一种方式改写文件fcb
//这个例子不添加新的参数,通过openlist中的绝对路径找到文件所处的目录,修改文件fcb
static void update_openlist_fcb(useropen o) {
  char origin_dir[80];
  strcpy(origin_dir, fs.open_file_list[fs.cur_dir_id].dir);
  char dir2[80];
  strcpy(dir2, o.dir);
  path_trim_last(dir2);
  cd(dir2);
  {
    int total;
    fcb_n_pos *FCBs = get_good_fcb_n_pos(fs.open_file_list[fs.cur_dir_id].fcb, &total);
    for (int i = 0; i < total; i++) {
      if (strcmp(FCBs[i].fcb.filename, o.fcb.filename) == 0) {
        memcpy(fs.startp + BLOCKSIZE * FCBs[i].block + sizeof(fcb) * FCBs[i].offset, &o.fcb, sizeof(fcb));//写入磁盘
      }
    }
  }
  cd(origin_dir);
}

//删除指定目录的指定文件或者目录
static void cd_and_rm(const char *path, int attr) {
  if (has_slash(path)) {
    char origin_path[80];
    strcpy(origin_path, fs.open_file_list[fs.cur_dir_id].dir);
    char cd2[80];
    strcpy(cd2, path);
    path_trim_last(cd2);
    char *name = path_get_name((char *) path);
    if (name == NULL) {
      printf("需要文件名\n");
      return;
    } else {
      cd(cd2);
      rm(name, attr);
      cd(origin_path);
    }
  } else {//只有一个文件名
    rm(path, attr);
  }
}

//删除当前目录的指定文件或者目录
static void rm(const char *name, int attr) {
  fcb cur = fs.open_file_list[fs.cur_dir_id].fcb;
  int total;
  fcb_n_pos *FCBs = get_good_fcb_n_pos(cur, &total);
  int freed = 0;
  for (int i = 0; i < total; i++) {
    if (FCBs[i].fcb.attribute == attr && name_cmp(FCBs[i].fcb, name) == 0) {
      fcb origin;
      memcpy(&origin, fs.startp + BLOCKSIZE * FCBs[i].block + sizeof(fcb) * FCBs[i].offset, sizeof(fcb));
      origin.free = 1;
      memcpy(fs.startp + BLOCKSIZE * FCBs[i].block + sizeof(fcb) * FCBs[i].offset, &origin, sizeof(fcb));
      freed = 1;
      free_fat(origin.first);
    }
  }
  if (!freed) {
    printf("没有这个%s\n", attr == ATTRDIR ? "目录" : "文件");
  }
  //只增不减
  free(FCBs);
}

//打开当前目录下的文件
static int open_file(const char *name) {
  int total;
  fcb cur = fs.open_file_list[fs.cur_dir_id].fcb;
  fcb *FCBs = get_good_fcb(cur, &total);
  for (int i = 0; i < total; i++) {
    if (FCBs[i].attribute == ATTRFILE) {
      if (name_cmp(FCBs[i], name) == 0) {
        int fd = find_free_fd();
        if (fd == -1) {
          return -1;
        } else {
          useropen *o = &fs.open_file_list[fd];
          o->t_openfile = 1;
          o->fcb = FCBs[i];
          strcpy(o->dir, fs.open_file_list[fs.cur_dir_id].dir);
          strcat(o->dir, FCBs[i].filename);
          strcat(o->dir, FCBs[i].exname);
          o->count = 0;
          o->fcb_state = 0;
          return fd;
        }
      }
    }
  }
  return -1;
}

//找出一个未被使用的文件描述符
static int find_free_fd() {
  for (int i = 0; i < MAXOPENFILE; i++) {
    if (fs.open_file_list[i].t_openfile == 0) {
      return i;
    }
  }
  printf("未找到空闲fd\n");
  return -1;
}

//更新根目录的..
static void update_root_dir_pre(int add) {
  fs.b0.root_length -= sizeof(fcb);//更新引导
  memcpy(fs.myvhard, &fs.b0, sizeof(fs.b0));
  //更新..
  fcb pre;
  memcpy(&pre, fs.startp + sizeof(fcb), sizeof(fcb));
  pre.length += sizeof(fcb);
  memcpy(fs.startp + sizeof(fcb), &pre, sizeof(fcb));
}

//释放一个文件的所有fat
static void free_fat(int start) {
  int cur = start;
  while (fs.fat1[cur].id != END) {
    int origin = cur;
    cur = fs.fat1[cur].id;
    fs.fat1[origin].id = FREE;
  }
  fs.fat1[cur].id = FREE;
}

//列出当前目录下的所有文件
static void ls() {
  fcb cur_dir_fcb = fs.open_file_list[fs.cur_dir_id].fcb;
  int n = 0;//当前目录下的文件数量,包含目录和一般文件
  fcb *FCBs = get_good_fcb(cur_dir_fcb, &n);
  for (int i = 0; i < n; i++) {
    fcb fcb = FCBs[i];
    if (fcb.attribute == ATTRDIR) {
      printf("%-4s\t", FCBs[i].filename);
    } else if (fcb.attribute == ATTRFILE) {
      if (fcb.exname[0] != '\0') {
        printf("%s.%s\t", FCBs[i].filename, FCBs[i].exname);
      } else {
        printf("%-4s\t", FCBs[i].filename);
      }
    }
  }
  printf("\n");
}

// 格式化内存磁盘,不写入文件
static void my_format() {
  fat _fat[BLOCKNUM];
  //初始化FAT1
  for (int i = 0; i < BLOCKNUM; i++) {
    _fat[i].id = FREE;
  }
  _fat[0].id = END;//根目录文件
  memcpy(fs.myvhard + BLOCKSIZE * 1, _fat, sizeof(_fat));//FAT1
  memcpy(fs.myvhard + BLOCKSIZE * 2, _fat, sizeof(_fat));//FAT2

  //初始化根目录文件
  fcb f_self = {
      .length = sizeof(fcb) * 2,
      .attribute = ATTRDIR,
      .first = 0,
      .date = get_date(),
      .time = get_time(),
      .free = 0
  };
  fcb f_pre = f_self;//根目录文件的..目录指向自己
  //唯一的区别
  strcpy(f_self.filename, ".");
  strcpy(f_pre.filename, "..");

  memcpy(fs.startp, &f_self, sizeof(f_self));
  memcpy(fs.startp + sizeof(f_self), &f_pre, sizeof(f_pre));

  //初始化引导块
  block0 b = {
      .root_length = sizeof(fcb) * 2,
      .root = 0
  };
  strcpy(b.info, BLOCK0INFO);
  memcpy(fs.myvhard, &b, sizeof(b));
}

//helper 辅助函数

//打印终端提示
FS_API void print_tips() {
  char path[80];
  strcpy(path, fs.open_file_list[fs.cur_dir_id].dir);
  if (strlen(path) != 1) {
    trim_slash(path);
  }
  printf("[%s@hostname %s]", UNAME, path);
  fflush(stdout);
}

//根据路径创建文件或目录
static void create_file_path(const char *path, int attr) {
  if (has_slash(path)) {
    char origin_path[80];
    strcpy(origin_path, fs.open_file_list[fs.cur_dir_id].dir);
    char cd2[80];
    strcpy(cd2, path);
    path_trim_last(cd2);

    char *name = path_get_name((char *) path);
    if (name == NULL) {
      printf("需要文件名\n");
      return;
    } else {
      cd(cd2);
      create_file(&(fs.open_file_list[fs.cur_dir_id].fcb), name, attr);
      cd(origin_path);
    }
  } else {//只有一个文件名
    create_file(&(fs.open_file_list[fs.cur_dir_id].fcb), path, attr);
  }
}

//得到一个目录文件的所有有效fcb
//n　返回有效fcb数量
static fcb *get_good_fcb(fcb dir, int *n) {
  int total;
  fcb *FCB_total = get_all_fcb(dir, &total);
  int good = 0;
  for (int i = 0; i < total; i++) {
    if (!FCB_total[i].free) {
      good++;
    }
  }
  fcb *FCBs = (fcb *) malloc(sizeof(fcb) * good);
  for (int i = 0, ii = 0; i < total; i++) {
    if (!FCB_total[i].free) {
      FCBs[ii++] = FCB_total[i];
    }
  }
  free(FCB_total);
  (*n) = good;
  return FCBs;
}

//得到一个目录文件的有效fcb和有效fcb的位置
//n　返回有效fcb数量
static fcb_n_pos *get_good_fcb_n_pos(fcb dir, int *n) {
  int total;
  fcb_n_pos *FCB_total = get_all_fcb_n_pos(dir, &total);
  int good = 0;
  for (int i = 0; i < total; i++) {
    if (!FCB_total[i].fcb.free) {
      good++;
    }
  }
  fcb_n_pos *FCBs = (fcb_n_pos *) malloc(sizeof(fcb_n_pos) * good);
  for (int i = 0, ii = 0; i < total; i++) {
    if (!FCB_total[i].fcb.free) {
      FCBs[ii++] = FCB_total[i];
    }
  }
  free(FCB_total);
  (*n) = good;
  return FCBs;
}

//得到一个目录文件的所有fcb和fcb的位置
//n　返回fcb数量
static fcb_n_pos *get_all_fcb_n_pos(fcb dir, int *n) {
  int total = get_dir_total(dir);
  int each = BLOCKSIZE / sizeof(fcb);//一个盘块最多存放的fcb数量
  fcb_n_pos *FCB_total = (fcb_n_pos *) malloc(sizeof(fcb_n_pos) * total);
  int cur = 0;//当前读取到的fcb序号
  int cur_block = dir.first;//当前读取的盘块号
  while (cur_block != END && cur < total) {
    int rest = total - cur;//剩下要读的盘块数量
    int copy = rest > each ? each : rest;
    for (int i = 0; i < copy; i++) {
      memcpy(&((FCB_total + cur + i)->fcb),
             fs.startp + cur_block * BLOCKSIZE + i * sizeof(fcb), sizeof(fcb));
      FCB_total[cur + i].block = cur_block;
      FCB_total[cur + i].offset = i;
    }
    cur += copy;
    cur_block = fs.fat1[cur_block].id;
  }
  (*n) = total;
  return FCB_total;
}

//得到一个目录文件的所有fcb,不带位置
//n 返回fcb数量
//如果目录的length为0,表示除根目录之外的..目录
static fcb *get_all_fcb(fcb dir, int *n) {
  int total = get_dir_total(dir);
  int each = BLOCKSIZE / sizeof(fcb);//一个盘块最多存放的fcb数量
  fcb *FCB_total = (fcb *) malloc(sizeof(fcb) * total);
  int cur = 0;//当前读取到的fcb序号
  int cur_block = dir.first;//当前读取的盘块号
  while (cur_block != END && cur < total) {
    int rest = total - cur;//剩下要读的盘块数量
    int copy = rest > each ? each : rest;
    memcpy(FCB_total + cur,
           fs.startp + cur_block * BLOCKSIZE, sizeof(fcb) * copy);
    cur += copy;
    cur_block = fs.fat1[cur_block].id;
  }
  (*n) = total;
  return FCB_total;
}

//为一个空的fcb添加一个空文件或者空目录
static void add_empty_file(fcb father, fcb *new, const char *name, int attr) {
  if (attr == ATTRFILE) {
    copy_filename((*new).filename, name);
    if (has_exname(name)) {
      copy_exname((*new).exname, name);
    } else {
      for (int i = 0; i < 3; i++) {
        (*new).exname[i] = '\0';
      }
    }
    (*new).free = 0;
    (*new).first = request_empty_block();
    (*new).attribute = ATTRFILE;
    (*new).date = get_date();
    (*new).time = get_time();
    (*new).length = 0;//空文件长度为0
  } else if (attr == ATTRDIR) {
    int block = request_empty_block();
    copy_filename((*new).filename, name);
    (*new).free = 0;
    (*new).first = block;
    (*new).attribute = ATTRDIR;
    (*new).date = get_date();
    (*new).time = get_time();
    (*new).length = 2 * sizeof(fcb);

    //初始化.. .特殊目录
    fcb f_self = {
        .length = sizeof(fcb) * 2,
        .attribute = ATTRDIR,
        .first = block,
        .date = get_date(),
        .time = get_time(),
        .free = 0
    };
    strcpy(f_self.filename, ".");
    fcb f_pre = father;
    f_pre.length = 0;//父文件的大小变动不显示在子文件,所以所有文件显示父目录的大小都是0
    strcpy(f_pre.filename, "..");

    memcpy(fs.startp + BLOCKSIZE * block, &f_self, sizeof(f_self));
    memcpy(fs.startp + BLOCKSIZE * block + sizeof(f_self), &f_pre, sizeof(f_pre));
  }
}

//得到一个新的空盘块,并且设置表项为非空,失败返回-1
static int request_empty_block() {
  for (int i = 0; i < BLOCKNUM; i++) {
    if (fs.fat1[i].id == FREE) {
      fs.fat1[i].id = END;
      return i;
    }
  }
  return -1;
}

//添加盘块
static void add_block(int first, int block) {
  while (fs.fat1[first].id != END) {
    first = fs.fat1[first].id;
  }
  fs.fat1[first].id = block;
}

//将fcb映射到虚拟硬盘上
//block 盘块号
//offset 偏移量
static void set_fcb(fcb _fcb, int block, int offset) {
  memcpy(fs.startp + block * BLOCKSIZE + sizeof(_fcb) * offset,
         &_fcb, sizeof(_fcb));
};

//在某个目录创建文件或目录
static void create_file(fcb *dir, const char *name, int attr) {
  if (attr == ATTRDIR) {
    //检查目录名是否合法
    if (!check_valid_dirname(name)) {
      return;
    }
  } else if (attr == ATTRFILE) {
    //检查文件名是否合法
    if (!check_valid_name(name)) {
      return;
    }
  }
  int total;
  fcb_n_pos *FCB_total = get_all_fcb_n_pos(*dir, &total);
  int free_pos = -1;//空闲的fcb位置
  for (int i = 0; i < total; i++) {
    if (free_pos == -1 && FCB_total[i].fcb.free) {//检查到空闲fcb就记录下来
      free_pos = i;
    }
    if (!FCB_total[i].fcb.free && name_cmp(FCB_total[i].fcb, name) == 0) {
      printf("创建失败,有重名文件\n");
      return;
    }
  }
  int reused = 0;
  if (free_pos != -1) {//找到空闲的fcb
    reused = 1;
    fcb *free = &FCB_total[free_pos].fcb;
    int b = FCB_total[free_pos].block;
    int off = FCB_total[free_pos].offset;
    add_empty_file(*dir, free, name, attr);
    set_fcb(*free, b, off);//写入新fcb
  } else {//没有空闲fcb
    int each = BLOCKSIZE / sizeof(fcb);//一个盘块最多存放的fcb数量
    fcb new;
    add_empty_file(*dir, &new, name, attr);
    if (FCB_total[total - 1].offset < each - 1) {//最后一个盘块有空闲位置
      //在当前盘块最后一块的下一块写入新fcb
      set_fcb(new, FCB_total[total - 1].block, FCB_total[total - 1].offset + 1);
    } else {//没有空闲位置
      int block_id = request_empty_block();//为父fcb申请新的盘块
      add_block(dir->first, block_id);//为父fcb添加新的盘块
      set_fcb(new, block_id, 0);//写入新fcb,偏移量为0
    }
  }
  if (!reused) {//如果没有复用,则增加长度
    dir->length += sizeof(fcb);
    if (strcmp(fs.open_file_list[fs.cur_dir_id].dir, "/") == 0) {//根目录
      update_root_dir_pre(sizeof(fcb));
    }
    update_self_dir_len(*dir, sizeof(fcb));
  }
}

//更新.目录长度
static void update_self_dir_len(fcb f, int add) {
  fcb self;
  memcpy(&self, fs.startp + f.first * BLOCKSIZE, sizeof(fcb));
  self.length += add;
  memcpy(fs.startp + f.first * BLOCKSIZE, &self, sizeof(fcb));
}

//切换目录的核心函数
static int cd(const char *dirname) {
  useropen new_open = fs.open_file_list[fs.cur_dir_id];
  char *path;
  if (dirname[0] == '/') {//绝对路径
    if (strlen(dirname) > 1) {
      path = strdup(dirname + 1);
    } else {
      path = strdup("");
    }
    cd_root(&new_open);
  } else {//相对路径
    path = strdup(dirname);
  }
  int succ = 1;
  char *token = strtok(path, "/");
  if (token && !cd_once(&new_open, token)) {
    printf("没有这个目录\n");
    succ = 0;
  }
  while (token) {
    token = strtok(NULL, "/");
    if (token) {
      if (!cd_once(&new_open, token)) {
        printf("没有这个目录\n");
        succ = 0;
        break;
      }
    }
  }
  free(path);
  if (succ) {
    fs.open_file_list[fs.cur_dir_id] = new_open;
    return 1;
  }
  return 0;
}

//寻找一个子目录，切换一次目录
static int cd_once(useropen *save, const char *arg) {
  if (strcmp(arg, "..") == 0) {
    path_trim_last(save->dir);
  } else if (strcmp(arg, ".") == 0) {
    return 1;
  } else {
    strcat(save->dir, arg);
    strcat(save->dir, "/");
  }
  fcb cur = save->fcb;
  int total;
  fcb *FCBs = get_good_fcb(cur, &total);
  for (int i = 0; i < total; i++) {
    if (FCBs[i].attribute == ATTRDIR && strcmp(FCBs[i].filename, arg) == 0) {//找到想找的目录
      save->fcb = FCBs[i];
      save->count = 0;
      save->t_openfile = 1;
      //找到的是..目录
      if (save->fcb.length == 0) {
        int size = get_dir_total(save->fcb);
        save->fcb.length = sizeof(fcb) * size;
      }
      return 1;
    }
  }
  return 0;
}

//切换到根目录
static void cd_root(useropen *save) {
  fcb root;
  root.length = fs.b0.root_length;
  root.first = fs.b0.root;
  strcpy(root.filename, "/");
  root.attribute = ATTRDIR;
  root.free = 0;

  //打开根目录
  save->fcb = root;
  save->count = 0;
  save->t_openfile = 1;
  strcpy(save->dir, root.filename);
}

//得到一个目录的fcb数量
static int get_dir_total(fcb dir) {
  fcb self;
  memcpy(&self, fs.startp + dir.first * BLOCKSIZE, sizeof(fcb));
  return (int) (self.length / sizeof(fcb));//文件总fcb数量
}
