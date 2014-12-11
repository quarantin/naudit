#ifndef NAUDIT_H
#define NAUDIT_H

#define NAUDIT_VERSION "0.1"

#ifndef KEY_TAB
#define KEY_TAB 0x09
#endif

#ifndef KEY_TAB_SHIFTED
#define KEY_TAB_SHIFTED 0x161
#endif

#ifndef KEY_ESC
#define KEY_ESC 0x1B
#endif

typedef void free_t (void *);

typedef char *(*canonicalize_t) (const char *);

#endif /* NAUDIT_H */
