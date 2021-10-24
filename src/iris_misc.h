#ifndef IRIS_MISC_H
#define IRIS_MISC_H

#define IRIS_VERSION "0.01:b"

// todo: shouldn't be here, it's implementation detail
#define ANSI_ESPACE "\033["
#define ANSI_ESCAPE_ERROR ANSI_ESPACE"38;41m" // red bg, white fg
#define ANSI_ESCAPE_RESET ANSI_ESPACE"0m"

#endif
