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

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

} WP;

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

void free_wp(WP *wp) {
  if (!head->next && head->NO == wp->NO) {
    wp->next = free_->next;
    free_->next = wp;
    head = head->next;
    return;
  }

  WP *tmp = head;
  while (tmp->next) {
    if (tmp->next->NO == wp->NO) {
      WP *save = tmp->next->next;
      wp->next = free_->next;
      free_->next = wp;
      tmp->next = save;
      return;
    }
    tmp = tmp->next;
  }
}

/* TODO: Implement the functionality of watchpoint */
