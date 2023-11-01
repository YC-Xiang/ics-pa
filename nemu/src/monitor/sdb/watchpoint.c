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

#include "sdb.h"

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

bool check_watchpoints() {
  bool success = true;
  word_t val;
  WP* tmp = head;

  while (tmp) {
    val = expr(tmp->expr, &success);
    if (success == false) {
      Log_error("check_watchpoint failed!\n");
      return false;
    }
    if (val != tmp->val) {
      nemu_state.state = NEMU_STOP;
      printf("Hardware watchpoint%d: %s\n", tmp->NO, tmp->expr);
      printf("Old value = 0x%x\n", tmp->val);
      printf("New value = 0x%x\n", val);
      tmp->val = val;
      sdb_mainloop();
    }
    tmp = tmp->next;
  }
  return true;
}

WP* new_wp() {
  if (!free_->next)
    assert(0);

  WP *res = free_->next;
  free_->next = free_->next->next;
  res->next = NULL;

  if (!head)
    head = res;
  else {
    res->next = head->next;
    head->next = res;
  }

  return res;
}

static void insert_free(WP *wp){
  wp->next = free_->next;
  free_->next = wp;
}

void free_wp(int NO) {
  if (!head)
  {
    printf("There is no watchpoints in List head!\n");
    return;
  }

  if (head->NO == NO) {
    WP* buffer = head->next;
    insert_free(head);
    head = buffer;
    return;
  }

  WP *tmp = head;
  while (tmp->next) {
    if (tmp->next->NO == NO) {
      WP *save = tmp->next->next;
      insert_free(tmp->next);
      tmp->next = save;
      return;
    }
    tmp = tmp->next;
  }
  printf("Not found %sWatchPoint%d %s\n", ANSI_FG_YELLOW, NO, ANSI_NONE);
}

void show_watchpoints()
{
  WP* tmp = head;
  printf("Num     What\n");
  while (tmp)
  {
    printf("%d:      %s\n", tmp->NO, tmp->expr);
    tmp = tmp->next;
  }
}
