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
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

uint32_t nr_buf = 0;

int choose(int n) {
    return nr_buf < 80 ? rand() % n : 0;
}

void gen_blank(int num) {
    for (int i = 0; i < num; ++i) {
	buf[nr_buf++] = ' ';
    }
}

void gen(char c) {
    gen_blank(choose(3));
    buf[nr_buf++] = c;
    gen_blank(choose(3));
}

void gen_num() {
    int rand = choose(1000);
    int a = rand % 10, b = ((rand - a) / 10) % 10, c = (rand - a - 10 * b) / 100;
    gen_blank(choose(3));
    if (c > 0)
	buf[nr_buf++] = c + '0';
    if (b > 0)
	buf[nr_buf++] = b + '0';
    buf[nr_buf++] = a + '0';
    gen_blank(choose(3));
}

void gen_rand_op() {
    char op_list[4] = {'+', '-', '*', '/'};
    gen_blank(choose(3));
    buf[nr_buf++] = op_list[choose(4)];
    gen_blank(choose(3));
}

static void gen_rand_expr() {
  // buf[0] = '\0';
  if (nr_buf > 65535) {
    printf("Buffer overflow\n");
  }
  switch (choose(3)) {
    case 0:
	gen_num();
	break;
    case 1:
	gen('(');
	gen_rand_expr();
	gen(')');
	break;
    default:
	gen_rand_expr();
	gen_rand_op();
	gen_rand_expr();
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
    nr_buf = 0;
    memset(buf, '\0', sizeof(buf));
    
    gen_rand_expr();

    buf[nr_buf] = '\0';

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -Wall -Werror -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
