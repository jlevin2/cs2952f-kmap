#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define RESET "\x1B[0m"


#include <time.h>

struct timespec spec;

#ifdef ENVOY
// #define write_log(format, args...)
#define write_log(format, args...)                                             \
    clock_gettime(CLOCK_REALTIME, &spec); \
    fprintf(stderr, "[ENV][%lu] " RED format RESET, spec.tv_nsec, ##args);
#endif

#ifdef SERVICE
#define write_log(format, args...)                                             \
    clock_gettime(CLOCK_REALTIME, &spec); \
    fprintf(stderr, "[SERV][%lu] " BLU format RESET, spec.tv_nsec, ##args);
#endif
