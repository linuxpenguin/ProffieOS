#ifndef COMMON_STRFUN_H
#define COMMON_STRFUN_H

#include "stdout.h"

const char *startswith(const char *prefix, const char* x) {
  while (*prefix) {
    if (toLower(*x) != toLower(*prefix)) return nullptr;
    prefix++;
    x++;
  }
  return x;
}

int cmpdir(const char *a, const char *b) {
  while (toLower(*a) == toLower(*b)) {
    if (!*a) return 0;
    a++;
    b++;
  }
  if (*a == '/' && *b == 0) return 0;
  if (*b == '/' && *a == 0) return 0;
  return toLower(*a) - toLower(*b);
}

int parse1hex(const char* x) {
  int ret = toLower(*x);
  if (ret > 'a') return ret - 'a' + 10;
  return ret - '0';
}

int parse2hex(const char* x) {
  return (parse1hex(x) << 4) | parse1hex(x+1);
}

bool endswith(const char *postfix, const char* x) {
  size_t l = strlen(x);
  if (l < strlen(postfix)) return false;
  x = x + l - strlen(postfix);
  while (*postfix) {
    if (toLower(*x) != toLower(*postfix)) return false;
    postfix++;
    x++;
  }
  return true;
}

const char* SkipSpace(const char* str) {
  while (*str == ' ' || *str == '\t') str++;
  return str;
}

const char* SkipWord(const char* str) {
  str = SkipSpace(str);
  while (*str != ' ' && *str != '\t' && *str) str++;
  return str;
}

int CountWords(const char* str) {
  int words = 0;
  while (*str) {
    str = SkipWord(str);
    words++;
  }
  return words;
}

float parsefloat(const char* s) {
  float ret = 0.0;
  float sign = 1.0;
  float mult = 1.0;
  s = SkipSpace(s);
  if (*s == '-') {
    sign = -1.0;
    s++;
  }
  while (*s) {
    int c = *s;
    if (c >= '0' && c <= '9') {
      if (mult == 1.0) {
	ret = (c - '0') + 10 * ret;
      } else {
	ret += (c - '0') * mult;
	mult /= 10.0;
      }
      s++;
    } else if (c == '.') {
      if (mult != 1.0) return ret * sign;
      // Time to read decimals.
      mult /= 10.0;
      s++;
    } else {
      return ret * sign;
    }
  }
  return ret * sign;
}

// Exactly one %s
const char* Sprintf(const char* pattern, const char* arg) {
  char* ret = (char*) malloc(strlen(pattern) + strlen(arg) + 1);
  if (!ret) return "";
  char *pct = strchr((char *)pattern, '%');
  memcpy(ret, pattern, pct - pattern);
  strcat(ret, arg);
  strcat(ret, pct + 2);
  return ret;
}

// Exactly one %s
bool Sscanf(const char* str, const char* pattern, const char** arg) {
  const char *pct = strchr(pattern, '%');
  int l = pct - pattern;
  if (strlen(str) < strlen(pattern) - 2) return false;
  if (memcmp(str, pattern, l)) return false;
  const char *tail = pct + 2;
  if (!endswith(pct + 2, str)) return false;
  int len = strlen(str) - strlen(pattern) - 2;
  char* ret = (char *)malloc(len + 1);
  if (!ret) return false;
  memcpy(ret, str + l, l);
  ret[len] = 0;
  *arg = ret;
  return true;
}

struct StringPiece {
  StringPiece(const char* s) : str(s), len(strlen(s)) {}
  StringPiece(const char* s, size_t l) : str(s), len(l) {}
  StringPiece(const char* begin, const char* end) : str(begin), len(end - begin) {}
  StringPiece() : str(0), len(0) {}
  int cmp(const StringPiece& other) const {
    const char* a = str;
    const char* b = other.str;
    size_t l = 0;
    while (true) {
      if (l == len && l == other.len) return 0;
      if (l == len) return -1;
      if (l == other.len) return 1;
      char A = toLower(*a);
      char B = toLower(*b);
      if (A != B) return A < B ? -1 : 1;
      a++;
      b++;
      l++;
    }
  }
  bool operator<(const StringPiece& other) const { return cmp(other) < 0; }
  bool operator>(const StringPiece& other) const { return cmp(other) > 0; }
  bool operator<=(const StringPiece& other) const { return cmp(other) <= 0; }
  bool operator>=(const StringPiece& other) const { return cmp(other) <= 0; }
  bool operator==(const StringPiece& other) const { return cmp(other) == 0; }
  bool operator!=(const StringPiece& other) const { return cmp(other) != 0; }

  explicit operator bool() const { return len != 0; }

  void printTo(Print& p) {
    p.write((const uint8_t*)str, len);
  }

  const char* str;
  size_t len;
};

const char* format_pattern(const char* pattern, const char* value) {
  size_t stars = 0;
  for (const char *p = pattern;*p;p++) if (*p == '*') stars++;
  size_t vlen = strlen(value);
  size_t len = strlen(pattern) + stars * (vlen - 1) + 1;
  char* ret = (char*) malloc(len);
  char* out = ret;
  for (const char *p = pattern;*p;p++) {
    if (*p == '*') {
      strcpy(out, value);
      out+=vlen;
    } else {
      *out = *p;
      out++;
    }
  }
  *out = 0;
  return ret;
}

StringPiece match_pattern(const char* pattern, const char* value) {
  size_t stars = 0;
  for (const char *p = pattern;*p;p++) if (*p == '*') stars++;
  size_t vlen = strlen(value);
  size_t plen = strlen(pattern);
  int ret_length = vlen - (plen - stars);
					   
  if (ret_length < 0) return StringPiece();
  if (ret_length % stars) return StringPiece();
  ret_length /= stars;
  const char* v = value;
  StringPiece ret;
  for (const char *p = pattern;*p;p++) {
    if (*p == '*') {
      if (!ret) {
	ret = StringPiece(v, ret_length);
      } else {
	if (ret != StringPiece(v, ret_length)) {
	  return StringPiece();
	}
      }
      v += ret_length;
    } else {
      if (*v != *p) return StringPiece();
      v++;
    }
  }
  return ret;
}


#endif
