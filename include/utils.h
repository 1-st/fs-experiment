//
// Created by up0 on 2020/6/13.
//

#ifndef FS_INCLUDE_UTILS_H_
#define FS_INCLUDE_UTILS_H_

//字符串是否含有/
int has_slash(const char * path){
  while(*path!='\0'){
    if(*path=='/'){
      return 1;
    }
    path++;
  }
  return 0;
}
//去掉最后的/
void trim_slash(char *p) {
  while (*p++ != '\0');
  p -= 2;
  *p = '\0';
}

//得到路径最后一个文件名
// /a/b/c    -> c
// /a/b/     ->NULL

const char * path_get_name(const char * p){
  while (*p++ != '\0');
  p-=2;
  if(*p == '/'){
    return NULL;
  }
  while (*p--!='/');
  return p+2;
}

//去除一个绝对路径的最后一个参数
//  /a/b/c/  ->   /a/b/
//  /a/b/c   =>   /a/b/
// /         =>    /
void path_trim_last(char *p) {
  if(strlen(p)==1&&p[0]=='/'){
    return;
  }
  while (*p++ != '\0');
  p -= 2;
  *p = '\0';
  while (*p != '/') {
    *p-- = '\0';
  }
}

//对比文件名
int name_cmp(fcb f, const char *name) {
  char _name[90];
  strcpy(_name, f.filename);
  if(f.attribute==ATTRFILE){
    strcat(_name,".");
    strcat(_name, f.exname);
  }
  return strcmp(_name, name);
}

//检查文件名称是否有后缀名
int has_exname(const char *name) {
  int k = 0;
  while (*name++ != '\0') {
    if (*name == '.') {
      k = 1;
    }
  }
  return k;
}

//检查文件名是否合法
//合法的文件名后缀长度不超过2,前缀长度不超过79,不包含空格'/'
int check_valid_name(const char *name) {
  int file_name_len = 0;
  while (*name != '\0' && *name != '.') {
    if (*name == ' ' || *name == '\\') {
      goto has_invalid_char;
    }
    name++;
    file_name_len++;
  }
  int ex_name_len = 0;
  while (*name != '\0') {
    if (*name == ' ' || *name == '\\') {
      goto has_invalid_char;
    }
    name++;
    ex_name_len++;
  }
  ex_name_len--;
  if (file_name_len >= 80) {
    goto name_too_long;
  }
  if (ex_name_len >= 3) {
    goto ex_too_long;
  }
  return 1;

  ex_too_long:
  printf("文件后缀过长\n");
  return 0;

  name_too_long:
  printf("文件名字过长\n");
  return 0;

  has_invalid_char:
  printf("文件名含有非法字符\n");
  return 0;
}

//检查目录名是否合法
//合法的目录名不包含/
int check_valid_dirname(const char *dirname) {
  while (*dirname != '\0') {
    if (*dirname == '\\') {
      printf("目录名不包含\\\n");
      return 0;
    }
    dirname++;
  }
  return 1;
}

//复制文件名
void copy_filename(char *dest, const char *src) {
  while (*src != '\0') {
    if (*src == '.') {
      break;
    }
    *dest++ = *src++;
  }
  *dest = '\0';
}

//复制拓展名
void copy_exname(char *dest, const char *src) {
  while (*src++ != '.');
  while ((*dest++ = *src++));
}

//========================================
//以一种刁钻的形式存储年月日，小时分钟秒
unsigned short get_time() {
  time_t _time = time(NULL);
  struct tm *time;
  time = localtime(&_time);
  return time->tm_hour * 2048 + time->tm_min * 32 + time->tm_sec / 2;
}
unsigned short get_date() {
  time_t _time = time(NULL);
  struct tm *time;
  time = localtime(&_time);
  return (time->tm_year - 80) * 512 + (time->tm_mon + 1) * 32 + time->tm_mday;
}
void decode_time(unsigned short encoded, int *h, int *m, int *s) {
  (*h) = encoded / 2048;
  (*m) = (encoded % 2048) / 32;
  (*s) = (encoded % 32) * 2;
}
void decode_date(unsigned short encoded, int *y, int *m, int *d) {
  (*y) = encoded / 512 + 1980;
  (*m) = encoded % 512 / 32;
  (*d) = encoded % 32;
}
//========================================

#endif //FS_INCLUDE_UTILS_H_
