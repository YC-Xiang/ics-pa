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

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUM, TK_PLUS, TK_MINUS, TK_MULTI, TK_DIV
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"-", TK_MINUS},           // minus
  {"\\*", TK_MULTI},         // multiply
  {"/", TK_DIV},           // divide
  {"==", TK_EQ},        // equal
  {"\\(", '('},         // left parentheses
  {"\\)", ')'},         // left parentheses
  {"[0-9]+", TK_NUM}    // number
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
            i, rules[i].regex, position, substr_len, substr_len, substr_start); // %.*s 接收两个参数，第一个为输出长度，第二个为字符串

        position += substr_len;

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case TK_NUM:
            if (substr_len > 32) {
              Log_error("number is too large!\n");
              return false;
            }
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0'; // string end with '\0'
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;
          default:
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
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

/* The expression is surrounded by a matched pair of parentheses.
  * If that is the case, just throw away the parentheses.
  */
static int check_parentheses(int p, int q) {
  int count = 0;
  int flag = 0;
  int flag1 = 0;

  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == '(')
      count++;
    else if (tokens[i].type == ')')
      count--;

    if (count < 0) { // ) is left than (
      Log_error("the expression is wrong!\n");
      return -2;
    }
    if ((count == 0) && (flag == 0)) { // if the most left ( is not eliminate by most right ) return -1.   (1+1) * (1+1)
      flag += 1;
      flag1 = i;
    }
  }

  if (count != 0) { // number of () is not equal
    Log_error("the expression is wrong!\n");
    return -2;
  }

  if ((count == 0) && (flag1 == q) && ((tokens[p].type == '(') && (tokens[q].type ==')'))) // eliminate a pair of ()
    return 0;

  return -1; // no need to handle
}

static int eval(int p, int q) {
  int ret;
  int main_op = TK_DIV; // assume beginning main_op is the max Token
  int op_pos = 1;
  int val1;
  int val2;
  int i = p;

  if (p > q) {
    Log_error("p=%d, q=%d, start position can't exceed end position!\n", p, q);
    return -1;
  }
  else if (p == q) {
    if (tokens[p].type != TK_NUM) {
      Log_error("Not a number!\n");
      return -1;
    }
    return atoi(tokens[p].str);
  }

  else if ((ret = check_parentheses(p, q)) == 0)
    return eval(p + 1, q - 1);
  else if (ret == -2)
    return -1;
  else {
    while (i >= p && i < q)
    {
      if (tokens[i].type >= TK_PLUS && tokens[i].type <= TK_DIV) {
        if (tokens[i].type <= main_op) {
          main_op = tokens[i].type;
          op_pos = i;
        }
      }

      if (tokens[i].type == '(') {
        int j = i + 1;
        while (tokens[j].type != ')')
          j++;
        i = j;
      }
      i++;
    }
    Log_info("op_pos=%d\n", op_pos);
    val1 = eval(p, op_pos - 1);
    val2 = eval(op_pos + 1, q);

    switch (main_op) {
      case TK_PLUS: return val1 + val2;
      case TK_MINUS: return val1 - val2;
      case TK_MULTI: return val1 * val2;
      case TK_DIV: return val1 / val2;
      default: assert(0);
    }
  }
  return 0;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  int ans = eval(0, nr_token - 1);

  return ans;
}
