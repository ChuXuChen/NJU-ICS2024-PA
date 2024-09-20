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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <memory/paddr.h>

enum {
  TK_NOTYPE = 256, TK_EQ = 0, TK_NEQ = 1, TK_LE = 2, TK_GE = 3, TK_AND = 4, TK_OR = 5,
  TK_NUM = 6, TK_HEX = 7, TK_REGISTER = 8, TK_DEREFERENCE = 9, TK_NEGATIVE = 10,

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},							// spaces
  {"\\+", '+'},								// plus
  {"-", '-'},								// minus
  {"\\*", '*'},								// multiply
  {"/", '/'},								// divide
  {"==", TK_EQ},							// equal
  {"!=", TK_NEQ},							// not equal
  {"<=", TK_LE},							// less or equal
  {">=", TK_GE},							// greater or equal
  {"<", '<'},								// less
  {">", '>'},								// greater
  {"\\(", '('},								// left bracket
  {"\\)", ')'},								// right bracket
  {"[0-9]+", TK_NUM},							// decimal number
  {"0[xX][0-9a-fA-F]+", TK_HEX},					// hexadecimal number
  {"\\$[\\$0|ra|[s|g|t]p|t[0-6]|a[0-7]|s[0-9]|s1[0-1]]", TK_REGISTER},	// register
  {"&&", TK_AND},							// and
  {"\\|\\|", TK_OR},							// or
  {"!", '!'},								// not
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
	  case TK_NOTYPE:
	    break;
	  case '+':
	    tokens[nr_token++].type = '+';
	    break;
	  case '-':
	    tokens[nr_token++].type = '-';
	    break;
	  case '*':
	    tokens[nr_token++].type = '*';
	    break;
	  case '/':
	    tokens[nr_token++].type = '/';
	    break;
	  case '<':
	    tokens[nr_token++].type = '<';
	    break;
	  case '>':
	    tokens[nr_token++].type = '>';
	    break;
	  case '(':
	    tokens[nr_token++].type = '(';
	    break;
	  case ')':
	    tokens[nr_token++].type = ')';
	    break;
	  case '!':
	    tokens[nr_token++].type = '!';
	    break;
	  case TK_EQ:
	    tokens[nr_token].type = TK_EQ;
	    strcpy(tokens[nr_token++].str, "==");
	    break;
	  case TK_NEQ:
	    tokens[nr_token].type = TK_NEQ;
	    strcpy(tokens[nr_token++].str, "!=");
	    break;
	  case TK_LE:
	    tokens[nr_token].type = TK_LE;
	    strcpy(tokens[nr_token++].str, "<=");
	    break;
	  case TK_GE:
	    tokens[nr_token].type = TK_GE;
	    strcpy(tokens[nr_token++].str, ">=");
	    break;
	  case TK_NUM:
	    tokens[nr_token].type = TK_NUM;
	    strncpy(tokens[nr_token++].str, substr_start, substr_len);
	    break;
	  case TK_HEX:
	    tokens[nr_token].type = TK_HEX;
	    strncpy(tokens[nr_token++].str, substr_start, substr_len);
	    break;
	  case TK_AND:
	    tokens[nr_token].type = TK_AND;
	    strcpy(tokens[nr_token++].str, "&&");
	    break;
	  case TK_OR:
	    tokens[nr_token].type = TK_OR;
	    strcpy(tokens[nr_token++].str, "||");
	    break;
	  case TK_REGISTER:
	    tokens[nr_token].type = TK_REGISTER;
	    strncpy(tokens[nr_token++].str, substr_start, substr_len);
	    break;
          default:
	    printf("No rules can match token type \"%.*s\"\n", substr_len, substr_start);
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

int operator_list[15] = {0, 1, 2, 3, 4, 5, '+', '-', '*', '/', '<', '>', '!', 9, 10};
int operand_list[3] = {6, 7, 8};

struct operator_priority {
    int type;
    int priority;
}priority_list[] = {{0, 4}, {1, 4}, {2, 4}, {3, 4}, {4, 1}, {5, 1}, {'+', 3}, {'-', 3}, {'*', 2}, {'/', 2}, {'<', 4}, {'>', 4}, {'!', 0}, {9, 0}, {10, 0}};

int find_priority(int operator) {
    int priority = -2;
    for (int i = 0; i < 15; ++i) {
	if (priority_list[i].type == operator)
	    priority = priority_list[i].priority;
    }
    return priority;
}

bool is_operator(int p) {
    bool flag = false;
    for (int i = 0; i < 15; ++i) {
	if (p == operator_list[i])
	    flag = true;
    }
    return flag;
}

bool is_operand(int p) {
    bool flag = false;
    for (int i = 0; i < 3; ++i) {
	if (p == operand_list[i])
	    flag = true;
    }
    return flag;
}

word_t eval(int, int);

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  for (int i = 0 ; i < nr_token; ++i) {
    if (i == 0 || is_operator(tokens[i - 1].type)) {
	switch(tokens[i].type) {
	    case '-':
		tokens[i].type = TK_NEGATIVE;
		break;
	    case '*':
		tokens[i].type = TK_DEREFERENCE;
		break;
	    default:
		printf("No this unary operator");
		assert(0);
	}
    }
  return eval(0, nr_token - 1);
  }

  return 0;
}

bool check_parentheses(int p, int q) {
    int count = 0;
    for (int i = p; i < q + 1; ++i) {
	if (tokens[i].type == '(')
	    count++;
	else if (tokens[i].type == ')')
	    count--;
	if (count < 0)
	    return false;
    }
    if (count == 0 && tokens[p].type == '(' && tokens[q].type == ')')
	return true;
    return false;
}

int main_operator_subscript(int p, int q) {
    int count = 0, subscript = 0, priority = -1;
    for (int i = p; i < q + 1; ++i) {
	if (is_operand(tokens[i].type))
	    continue;
	if (tokens[i].type == '(') {
	    count++;
	    continue;
	}
	else if (tokens[i].type == ')') {
	    count--;
	    continue;
	}
	if (count != 0)
	    continue;
	int temp_priority = find_priority(tokens[i].type);
	if (priority <= temp_priority) {
	    subscript = i;
	}
    }
    return subscript;
}

word_t eval(int p, int q) {
    if (p > q) {
	/* Bad Expression */
	assert(0);
    } else if (p == q) {
	/* Single token */
	if (tokens[p].type == TK_NUM) {
	    return (word_t)strtol(tokens[p].str, NULL, 10);
	} else if (tokens[p].type == TK_HEX) {
	    char *str = tokens[p].str + 2;
	    return (word_t)strtol(str, NULL, 16);
	} else if (tokens[p].type == TK_REGISTER) {
	    bool *success = false;
	    word_t value = isa_reg_str2val(tokens[p].str, success);
	    if (!success)
		assert(0);
	    return value;
	} else if (is_operator(p)){
	    printf("Error:Duplicate operator");
	    assert(0);
	} else {
	    printf("Undefined token");
	    assert(0);
	}
    } else if (check_parentheses(p, q)) {
	/* Remove the outmost pair of parentheses if it wraps the entire expression */
	return eval(p + 1, q - 1);
    } else {
	int op = main_operator_subscript(p, q);
	word_t val1 = eval(p, op - 1);
	word_t val2 = eval(op + 1, q);
	switch(tokens[op].type) {
	    case '+':
		return val1 + val2;
	    case '-':
		return val1 - val2;
	    case '*':
		return val1 * val2;
	    case '/':
		if (val2 == 0) {
		    printf("ERROR:Divided by zero");
		    assert(0);
		} else {
		    return (sword_t)val1 / (sword_t)val2;
		}
	    case '<':
		return (sword_t)val1 < (sword_t)val2;
	    case '>':
		return (sword_t)val1 > (sword_t)val2;
	    case TK_EQ:
		return val1 == val2;
	    case TK_NEQ:
		return val1 != val2;
	    case TK_LE:
		return val1 <= val2;
	    case TK_GE:
		return val1 >= val2;
	    case TK_AND:
		return val1 && val2;
	    case TK_OR:
		return val1 || val2;
	    case '!':
		return !val2;
	    case TK_DEREFERENCE:
		return paddr_read(val2, 4);
	    case TK_NEGATIVE:
		return -val2;
	    default:
		printf("The operator is not supported");
		assert(0);
	}
    }
}


