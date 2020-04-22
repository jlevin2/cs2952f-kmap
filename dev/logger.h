#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"


#ifdef ENVOY
#define write_log(format,args...) fprintf(stderr, "[ENV] "RED format RESET, ## args);
//#define PRINTF(...) printf(BLU __VA_ARGS__ RESET)
#endif

#ifdef SERVICE
#define write_log(format,args...) fprintf(stderr, "[SER] "BLU format RESET, ## args);
//#define PRINTF(...) printf(MAG __VA_ARGS__ RESET)
#endif

