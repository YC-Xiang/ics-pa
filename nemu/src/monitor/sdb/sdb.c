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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <memory/paddr.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_si(char *args) {
  int n;

  if (args == NULL) {
    cpu_exec(1);
  }
  else {
  n = atoi(args);
  if (n == 0)
    printf("Please enter a number\n"); // args not a number or number0，atoi both return 0.
  cpu_exec(n);
  }

  return 0;
}

static int cmd_info(char *args) {
  if (args == NULL)
    Log_error("args is NULL, please enter info r or info w\n");

  else if (*args == 'r')
    isa_reg_display();
  //else if (*args == 'w')

  return 0;
}

static int cmd_x(char *args) {
  if (args == NULL) {
    Log_error("args is NULL, please enter X N EXPR\n");
    return 0;
  }
  char *x_num = strtok(args, " ");
  int num = atoi(x_num);

  char *args_right = x_num + strlen(x_num) + 1;
  paddr_t addr = strtoul(args_right, NULL, 16);

  uint8_t* mem = guest_to_host(addr);

  printf("0x%8x: ", addr);
  for (int i = 0; i < num; i++) {
    printf("0x");
    for (int j = 3; j >= 0; j--){
      printf("%02x", mem[j + 4 * i]);
    }
    printf("\t");
  }
  printf("\n");

  return 0;
}

static int cmd_p(char *args) {
  bool ret = true;
  word_t val;

  if (args == NULL) {
    Log_error("args is NULL, please enter P EXPR\n");
    return 0;
  }

  val = expr(args, &ret);

  if (ret == false) {
    Log_error("expression evaluation failed!\n");
    return 0;
  }

  printf("val=0x%x\n", val);

  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Execution one step of the program", cmd_si },
  { "info", "Print registers/watchpoints", cmd_info },
  { "x", "Print memory value", cmd_x },
  { "p", "expression evaluation", cmd_p },
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

#ifdef CONFIG_EXPR_TEST
  char buff[512] = {};
  bool ret = true;
  FILE *fp = fopen("/home/yuchengxiang/ics2022/nemu/tools/gen-expr/build/input", "r");
  if (fp == NULL)
    printf("open file error!\n");
  while (fgets(buff, sizeof(buff), fp)) {
    printf("%s\n", buff);
    for (int i = 0; i < sizeof(buff); i++)
    {
      if (buff[i] == 10)
        buff[i] = '\0';
    }

    char *tmp = buff;
    while (*tmp != ' ')
      tmp++;
    expr(++tmp, &ret);
    if (ret == false) {
      Log_error("expression evaluation failed!\n");
      assert(0);
    }
    else
      printf("pass!\n");
    memset(buff, 0, sizeof(buff));
  }
#endif

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
