/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char *buf_p;
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static int choose(int n) {
  return rand() % n;
}

static void gen_space() {
  switch (choose(3)) {
    case 0:
    case 1: break;
    case 2: *buf_p = ' '; buf_p++; break;
  }
}

static void gen_num() {
  int tmp = rand() % 100 + 1;
  sprintf(buf_p, "%d", tmp);
  if (tmp > 0 && tmp < 10)
    buf_p += 1;
  else if (tmp >= 10 && tmp < 100)
    buf_p += 2;
  else if (tmp == 100)
    buf_p += 3;
}

static void gen(int operator) {
    *buf_p = operator;
    buf_p++;
}

static void gen_rand_op() {
  switch (choose(4)) {
    case 0: *buf_p = '+'; buf_p++; break;
    case 1: *buf_p = '-'; buf_p++; break;
    case 2: *buf_p = '*'; buf_p++; break;
    case 3: *buf_p = '/'; buf_p++; break;
    default: printf("wrong op!\n");
  }
}

static void gen_rand_expr(int deep) {
  if ((strlen(buf) > 65535 - 10000) || deep > 10) {
    gen_space();
    gen_num();
    gen_space();
    return;
  }

  switch (choose(3)) {
    case 0:
      gen_space();
      gen_num();
      gen_space();
      break;
    case 1:
      gen('(');
      gen_space();
      gen_rand_expr(deep + 1);
      gen_space();
      gen(')');
      break;
    default:
      gen_rand_expr(deep + 1);
      gen_space();
      gen_rand_op();
      gen_space();
      gen_rand_expr(deep + 1);
      break;
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    buf_p = buf;
    memset(buf, 0, 65536); // fixme: clear buf each cycle. this op is stupid.
    gen_rand_expr(0);

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -Werror -o /tmp/.expr"); // add -Werror, if div by 0, system() return will not be 0.
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r"); // execute .expr, store results to stream fp
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
