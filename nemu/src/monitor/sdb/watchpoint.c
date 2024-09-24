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
  char *EXPR;
  word_t last_value;
  word_t current_value;
  char *is_changed;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

WP* new_wp() {
    assert(free_ != NULL);
    WP *new_wp = free_;
    free_ = free_->next;
    new_wp->next = head;
    head = new_wp;
    return new_wp;
}

void free_wp(WP *wp) {
    assert(wp != NULL);
    WP *h = head;
    if (h == wp) {
	if (h->next == NULL)
	    head = NULL;
	else
	    h = h->next;
    } else {
	while (h && h->next != wp)
	    h = h->next;
	if (h == NULL) {
	    printf("No this watchpoint to free\n");
	    return;
	}
	h->next = wp->next;
    }
    wp->next = free_;
    free_ = wp;
}

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].last_value = 0;
    wp_pool[i].current_value = 0;
    wp_pool[i].is_changed = "False";
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

void wp_display() {
    WP *h = head;
    if (h == NULL) {
	printf("No watchpoints");
	return;
    }
    printf("NO\tEXPR\tlast_value\tcurrent_value\tis_changed\n");
    while (h) {
	printf("%d\t%s\t%u\t%u\t%s\n", h->NO, h->EXPR, h->last_value, h->current_value, h->is_changed);
	h = h -> next;
    }
}

void wp_set(char *args, word_t value) {
    WP *wp = new_wp();
    wp->EXPR = args;
    wp->last_value = value;
    wp->current_value = value;
    wp->is_changed = "False";
    printf("Set %drd watchpoint in %s, its value is %u\n", wp->NO, wp->EXPR, wp->current_value);
}

void wp_delete(int n) {
    WP *p = &wp_pool[n];
    free_wp(p);
    printf("Delete %drd watchpoint", n);    
}
