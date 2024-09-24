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
#include "sdb.h"
#include "memory/paddr.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
void wp_display();
void wp_set(char*, word_t);
void wp_delete(int);

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

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "si [N], Execute N(default one) step", cmd_si },
  { "info", "info SUBCMD, Print current state of (r)register or (w)watchpoint", cmd_info },
  { "x", "x N EXPR, Print data from memory address EXPR to EXPR+4N per 4 Bytes", cmd_x },
  { "p", "p EXPR, Caculate the value of expression EXPR", cmd_p },
  { "w", "w EXPR, Stop executing when EXPR changed", cmd_w },
  { "d", "d N, Delete Nrd watchpoints", cmd_d },

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

static int cmd_si(char *args) {
    /* extract the first argument */
    char *arg = strtok(NULL, " ");
    if (arg == NULL)
	// execute one step by default
	cpu_exec(1);
    else {
	char *endptr;
	int step = strtol(arg, &endptr, 10);
	/* the parameter of step count should be a positive integer */
	if (*endptr != '\0')
	    printf("Please input a positive integer instead of \"%s\"\n", arg);
	else {
	    cpu_exec(step);
	}
    }
    return 0;
}

static int cmd_info(char *args) {
    /* extract the first argument */
    char *arg = strtok(NULL, " ");
    if (*arg == 'r')
	isa_reg_display();
    else if (*arg == 'w')
	wp_display();
    else
	printf("Unknown options and please input \"help info\"\n");
    return 0;
}

static int cmd_x(char *args) {
    /* extract the first argument */
    char *arg1 = strtok(NULL, " ");
    char *arg2 = strtok(NULL, "#");
    if (arg1 == NULL)
	arg1 = "1";
    if (arg2 == NULL)
	arg2 = "0X80000000";
    char *endptr;
    int num = strtol(arg1, &endptr, 10);
    bool success = false;
    paddr_t addr = expr(arg2, &success);
    if (*endptr != '\0') {
        printf("N should be a decimal positive integer\n");
	assert(0);
    } else if (!success) {
        printf("Invalid EXPR\n");
	assert(0);
    }
    for (int i = 0; i < num; ++i) {
        printf("0X%x---%d\n", addr, paddr_read(addr, 4));
        addr += 4;
    }
    return 0;
}

static int cmd_p(char *args) {
    bool success = false;
    word_t res = expr(args, &success);
    if (!success) {
	printf("Invalid expression\n");
	assert(0);
    }
    else
	printf("EXPR is %u\n", res);
    return 0;
}

static int cmd_w(char *args) {
    bool success;
    char args_copy[128];
    strcpy(args_copy, args);
    word_t value = expr(args, &success);
    if (!success) {
	printf("Invalid expression\n");
	assert(0);
    }
    wp_set(args_copy, value);
    return 0;
}

static int cmd_d(char *args) {
    char *endptr;
    int n = strtol(args, &endptr, 10);
    if (*endptr != '\0') {
	printf("Please input in the format like \"d N\", N is a positive integer\n");
	return 0;
    }
    wp_delete(n);
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

void test_expr() {
    FILE *fp = fopen("/home/ics/ics2024/nemu/tools/gen-expr/build/input", "r");
    char *line;
    bool success = false;
    size_t len = 0;
    word_t true_value = 0;
    if (fp == NULL) {
	printf("Please generate the test file firstly\n");
	assert(0);
    }
    while(true) {
	if (fscanf(fp, "%u", &true_value) == -1) break;
	ssize_t read = getline(&line, &len, fp);
	line[read - 1] = '\0';
	word_t res = expr(line, &success);
        assert(success);
	if (res != true_value) {
	    printf("Wrong answer\nThe calculation results is %u but true value is %u\n", res, true_value);
	    assert(0);
	}
	memset(line, '\0', read);
    }
    fclose(fp);
    Log("EXPR test pass");
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Test the function of expression. */
  test_expr();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
