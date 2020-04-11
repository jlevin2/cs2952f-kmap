#include "logger.h"

void logger(const char* message) {
    time_t now;
    time(&now);
#ifdef ENVOY
    fprintf(stderr, "%s [%s]: %s\n", ctime(&now), "ENVOY", message);
#endif
#ifdef SERVICE
    fprintf(stderr, "%s [%s]: %s\n", ctime(&now), "SERVICE", message);
#endif
}