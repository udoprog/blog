/**
 * This code is release into the public domain.
 * If you find it useful i would like to be given a notice as the original author.
 * Please contact me at: johnjohn.tedro@gmail.com
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
 
typedef struct utf8_decoder_t {
  int i, w;
  unsigned long state;
  int pos;
} utf8_decoder_t;
 
#define NUL 0x0
#define UNK -0x1
 
// use only the first six bits of a byte to signify value.
// first argument singifies which byte in the series and by significance.
#define trail_byte(i, b) (((b) & 0x3f) << (6 * (i)))
 
// index the buffer using a specific stateholder.
#define current(u, b) *((u)->pos + (b))
 
// thread-safe coroutine statements, no static variables.
#define COBegin(u)      switch(u->state) { case 0:
#define COReturn(u, v)    u->state = __LINE__;\
                          return (v);\
                        case __LINE__:
#define COEnd           }
 
int utf8_decoder(utf8_decoder_t *u, long *buffer, uint8_t c) {
  COBegin(u);
  while (1) {
    if ((c >> 7) == 0x0) {
      current(u, buffer) = c;
      u->i = 0;
    }
    // first byte says length two
    // that is 110xxxxx
    else if ((c >> 5) == (0x3 << 1)) {
      u->i = 1;
      current(u, buffer) = trail_byte(u->i, c & 0x1f);
    }
    // first byte says length three
    // that is 1110xxxx
    else if ((c >> 4) == (0x7 << 1)) {
      u->i = 2;
      current(u, buffer) = trail_byte(u->i, c & 0x0f);
    }
    // first byte says length four
    // that is 11110xxx
    else if ((c >> 3) == (0x0f << 1)) {
      u->i = 3;
      current(u, buffer) = trail_byte(u->i, c & 0x07);
    }
    // first byte says length five
    // that is 111110xx
    else if ((c >> 2) == (0x1f << 1)) {
      u->i = 4;
      current(u, buffer) = trail_byte(u->i, c & 0x03);
    }
    // first byte says length six
    // that is 1111110x
    else if ((c >> 1) == (0x3f << 1)) {
      u->i = 5;
      current(u, buffer) = trail_byte(u->i, c & 0x01);
    }
    // first byte does not signify unicode.
    else {
      goto error;
    }
 
    u->w = u->i;
 
    while (u->i-- > 0) {
      COReturn(u, 1);
      // decode failure part mismatch trailing byte is not 10xxxxxx.
      // where the xxxxxx parts signify payload.
      if ((c >> 6) != (0x1 << 1)) { goto reject; }
      // rfc states that no unicode character is longer than this.
      // code can obviously not be less than 0 either.
      //if (u->code > 0x10ffff || u->code < 0x0)  { goto error; }
 
      // next part is an actual unicode part.
      current(u, buffer) += trail_byte(u->i, c);
    }
 
    // check for overly long sequence.
    switch (u->w) {
      case 1: if (current(u, buffer) < 0x00000080) goto error; break;
      case 2: if (current(u, buffer) < 0x00000800) goto error; break;
      case 3: if (current(u, buffer) < 0x00010000) goto error; break;
      case 4: if (current(u, buffer) < 0x00200000) goto error; break;
      case 5: if (current(u, buffer) < 0x04000000) goto error; break;
    }
 
    // invalid utf-8 sequences.
    if (current(u, buffer) >= 0xD800 && current(u, buffer) <= 0xDFFF) goto error;
 
    u->pos++;
    COReturn(u, 1);
    continue;
error:
    current(u, buffer) = UNK;
    u->pos++;
    COReturn(u, 1);
    continue;
reject:
    current(u, buffer) = UNK;
    u->pos++;
    continue;
  }
 
  COEnd;
}
 
int main() {
  // this is the only requirement to get the coroutine working properly.
  utf8_decoder_t u = {.state = 0};
 
  // two character loockahead buffer.
  long buffer[2];
 
  int i;
  long c;
 
  while (!feof(stdin)) {
    utf8_decoder(&u, buffer, getc(stdin));
 
    // since we are using lookahead buffer,
    // only read when we have something.
    if (u.pos <= 0) {
      continue;
    }
 
    // read the number of characters we have.
    for (i = 0; i < u.pos; i++) {
      c = buffer[i];
      if (c == UNK) {
        printf("<?>");
        continue;
      }
 
      if (c == NUL) {
        printf("<NUL>");
        continue;
      }
 
      if (c < 0x80) {
        printf("%c", c);
      }
      else {
        printf("<U+%04x>", c);
      }
    }
 
    u.pos = 0;
  }
 
 
  return 0;
}
