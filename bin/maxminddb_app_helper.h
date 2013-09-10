#ifndef GEODB_HELPER
#define GEODB_HELPER (1)
#include "maxminddb.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

/* dummy content */

#define DIE(...)                      \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        exit(1);                      \
    } while (0)

#define FREE_LIST(...)                                               \
    do {                                                             \
        {                                                            \
            void *ptr[] = { __VA_ARGS__ };                           \
            for (int i = 0; i < sizeof(ptr) / sizeof(void *); i++) { \
                if (ptr[i]) {                                        \
                    free(ptr[i]); }                                  \
            }                                                        \
        }                                                            \
    } while (0)

    /* *INDENT-OFF* */
    /* --prototypes automatically generated by dev-bin/regen-prototypes.pl - don't remove this comment */
    extern char *bytesdup(MMDB_s *mmdb, MMDB_entry_data_s const *const entry_data);
    extern void usage(char *prg);
    extern void dump_meta(MMDB_s mmdb);
    extern MMDB_s open_or_die(const char *fname, int mode);
    extern MMDB_lookup_result_s lookup_or_die(MMDB_s *mmdb, const char *ipstr);
    extern void dump_ipinfo(const char *ipstr, MMDB_lookup_result_s *ipinfo);
    /* --prototypes end - don't remove this comment-- */
    /* *INDENT-ON* */

#endif
