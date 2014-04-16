#ifndef TBS_H
#define TBS_H

#ifndef MODULE_NAME
# error "Set MODULE_NAME first"
#endif

#define DEBUG(fmt, ...) do { \
        fprintf(stdout, "%s: [D] ", MODULE_NAME); \
        fprintf(stdout, fmt, ##__VA_ARGS__); \
        fprintf(stdout, "\n"); \
    } while(0)

#define INFO(fmt, ...) do { \
        fprintf(stderr, "%s: ", MODULE_NAME); \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while(0)

#define WARNING(fmt, ...) do { \
        fprintf(stderr, "%s: [W] ", MODULE_NAME); \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while(0)

#endif // TBS_H
