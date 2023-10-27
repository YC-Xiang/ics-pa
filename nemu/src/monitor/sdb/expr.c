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
#include <memory/paddr.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_OR, TK_AND, TK_NEQ, TK_EQ, TK_PLUS, TK_MINUS, TK_MULTI, TK_DIV, TK_DEREF, // don't insert other tokens between TK_OR to TK_DEREF
  TK_NUM, TK_HEX, TK_REG,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"-", TK_MINUS},           // minus
  {"\\*", TK_MULTI},         // multiply and dereference
  {"/", TK_DIV},           // divide
  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ},        // not equal
  {"\\(", '('},         // left parentheses
  {"\\)", ')'},         // left parentheses
  {"0x[0-9a-fA-F]+", TK_HEX}, // match hex first, or the first 0 will be recognized as TK_NUM
  {"[0-9]+", TK_NUM},    // number
  {"^\\$[a-z0-9]{2}$", TK_REG},
  {"&&", TK_AND},
  {"\\|\\|", TK_OR}
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

#define TOKENS_LEN 64
static Token tokens[TOKENS_LEN] __attribute__((used)) = {};
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
          case TK_REG:
          case TK_HEX:
          case TK_NUM:
            if (substr_len > 32) {
              Log_error("number is too large!\n");
              return false;
            }
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0'; // string end with '\0'.
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;
          default:
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
        }
        if (nr_token > TOKENS_LEN)
          Log_error("too much tokens!\n");

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
      assert(0);
    }
    if ((count == 0) && (flag == 0)) { // if the most left ( is not eliminate by most right ) return -1.   (1+1) * (1+1)
      flag += 1;
      flag1 = i;
    }
  }

  if (count != 0) { // number of () is not equal
    Log_error("the expression is wrong!\n");
    assert(0);
  }

  if ((count == 0) && (flag1 == q) && ((tokens[p].type == '(') && (tokens[q].type ==')'))) // eliminate a pair of ()
    return true;

  return false; // no need to handle
}

static word_t eval(int p, int q) {
  int main_op = TK_DEREF; // assume beginning main_op is the max Token
  int op_pos = -1;
  word_t val1 = 0;
  word_t val2 = 0;
  int count = 0;
  int i = p;
  bool success = true;
  word_t val;

  Log_info("p=%d q=%d\n", p, q);
  if (p > q) {
    Log_error("p=%d, q=%d, start position can't exceed end position!\n", p, q);
    assert(0);
  }
  else if (p == q) {
    if (tokens[p].type != TK_NUM && tokens[p].type != TK_HEX && tokens[p].type != TK_REG) {
      Log_error("Not a number!\n");
      assert(0);
    }
    if (tokens[p].type == TK_REG) {
      val = isa_reg_str2val(tokens[p].str, &success);
      if (success != true) {
        Log_error("isa_reg_str2val error!\n");
        assert(0);
      }
      else
        return val;
    }
    return strtol(tokens[p].str, NULL, 0);
  }

  else if ((check_parentheses(p, q)) == true)
    return eval(p + 1, q - 1);
  else {
    while (i >= p && i < q)
    {
      if (tokens[i].type == TK_MULTI && (i == 0 || tokens[i - 1].type < TK_NUM)) { // 解引用前面的token不可能是num/hex/reg
        tokens[i].type = TK_DEREF;
      }

      if (tokens[i].type >= TK_OR && tokens[i].type <= TK_DEREF) {
        if (tokens[i].type <= main_op) {
          main_op = tokens[i].type;
          op_pos = i;
        }
      }

      if (tokens[i].type == '(') { // if encounter '(', skip to the pair of ')'
        count++;
        int j = i + 1;
        while (count) {
          if (tokens[j].type == '(')
            count++;
          else if (tokens[j].type == ')')
            count--;
          if (count == 0)
            break;
          j++;
        }
        i = j;
      }
      i++;
    }

    Log_info("op_pos=%d, main op=%d\n", op_pos, tokens[op_pos].type);

    if (tokens[op_pos].type == TK_DEREF) {
      val1 = eval(op_pos + 1, q);
    }
    else {
      val1 = eval(p, op_pos - 1);
      val2 = eval(op_pos + 1, q);
    }


    switch (main_op) {
      case TK_PLUS: return val1 + val2;
      case TK_MINUS: return val1 - val2;
      case TK_MULTI: return val1 * val2;
      case TK_DIV: return val1 / val2;
      case TK_EQ: return val1 == val2;
      case TK_NEQ: return val1 != val2;
      case TK_AND: return val1 && val2;
      case TK_OR: return val1 || val2;
      case TK_DEREF: return paddr_read(val1, 4);
      default: assert(0);
    }
  }
  assert(0);

  return 0;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  word_t ans = eval(0, nr_token - 1);

  return ans;
}
