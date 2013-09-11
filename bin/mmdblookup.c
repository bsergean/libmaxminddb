#include "maxminddb.h"
#include <assert.h>
#include <getopt.h>
#include <libgen.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define LOCAL

#define VERSION "2.0.0"

/* *INDENT-OFF* */
/* --prototypes automatically generated by dev-bin/regen-prototypes.pl - don't remove this comment */
LOCAL void usage(char *program, int exit_code, const char *error);
LOCAL char **get_options(int argc, char **argv, char **mmdb_file,
                         char **ip_address, int *verbose,
                         int *lookup_path_length);
LOCAL MMDB_s open_or_die(const char *fname, int mode);
LOCAL MMDB_s dump_meta(MMDB_s *mmdb);
LOCAL MMDB_lookup_result_s lookup_or_die(MMDB_s *mmdb, const char *ipstr);
/* --prototypes end - don't remove this comment-- */
/* *INDENT-ON* */

int main(int argc, char **argv)
{
    char *mmdb_file = NULL;
    char *ip_address = NULL;
    int verbose = 0;
    int lookup_path_length = 0;

    char **lookup_path =
        get_options(argc, argv, &mmdb_file, &ip_address, &verbose,
                    &lookup_path_length);

    MMDB_s mmdb = open_or_die(mmdb_file, MMDB_MODE_MMAP);

    if (verbose) {
        dump_meta(&mmdb);
    }

    MMDB_lookup_result_s result = lookup_or_die(&mmdb, ip_address);
    MMDB_entry_data_list_s *entry_data_list = NULL;

    int exit_code = 0;
    if (result.found_entry) {
        int status;
        if (lookup_path_length) {
            MMDB_entry_data_s entry_data;
            status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);
            if (MMDB_SUCCESS == status) {
                if (entry_data.offset) {
                    MMDB_entry_s entry =
                    { .mmdb = &mmdb, .offset = entry_data.offset };
                    status = MMDB_get_entry_data_list(&entry, &entry_data_list);
                } else {
                    fprintf(
                        stdout,
                        "\n  No data was found at the lookup path you provided\n\n");
                }
            }
        } else {
            status = MMDB_get_entry_data_list(&result.entry, &entry_data_list);
        }

        if (MMDB_SUCCESS != status) {
            fprintf(stderr, "Got an error looking up the entry data - %s\n",
                    MMDB_strerror(status));
            exit_code = 5;
            goto end;
        }

        if (NULL != entry_data_list) {
            fprintf(stdout, "\n");
            MMDB_dump_entry_data_list(stdout, entry_data_list, 2);
            fprintf(stdout, "\n");
        }
    } else {
        fprintf(stderr,
                "\n  Could not find an entry for this IP address (%s)\n\n",
                ip_address);
        exit_code = 6;
    }

 end:
    MMDB_free_entry_data_list(entry_data_list);
    MMDB_close(&mmdb);
    free(lookup_path);
    exit(exit_code);
}

LOCAL void usage(char *program, int exit_code, const char *error)
{
    if (NULL != error) {
        fprintf(stderr, "\n  *ERROR: %s\n", error);
    }

    char *usage = "\n"
                  "  %s --file /path/to/file.mmdb --ip 1.2.3.4 [path to lookup]\n"
                  "\n"
                  "  This application accepts the following options:\n"
                  "\n"
                  "      --file (-f)     The path to the MMDB file. Required.\n"
                  "\n"
                  "      --ip (-i)       The IP address to look up. Required.\n"
                  "\n"
                  "      --verbose (-v)  Turns on verbose output. Specifically, this causes this\n"
                  "                      application to output the database metadata.\n"
                  "\n"
                  "      --version       Print the program's version number and exit.\n"
                  "\n"
                  "      --help (-h -?)  Show usage information.\n"
                  "\n";

    fprintf(stderr, usage, program);
    exit(exit_code);
}

LOCAL char **get_options(int argc, char **argv, char **mmdb_file,
                         char **ip_address, int *verbose,
                         int *lookup_path_length)
{
    static int help = 0;
    static int version = 0;

    while (1) {
        static struct option options[] = {
            { "file",    required_argument, 0, 'f' },
            { "ip",      required_argument, 0, 'i' },
            { "verbose", no_argument,       0, 'v' },
            { "version", no_argument,       0, 'n' },
            { "help",    no_argument,       0, 1   },
            { "?",       no_argument,       0, 1   },
            { 0,         0,                 0, 0   }
        };

        int opt_index;
        int opt_char = getopt_long(argc, argv, "f:i:vh?", options, &opt_index);

        if (-1 == opt_char) {
            break;
        }

        if ('f' == opt_char) {
            *mmdb_file = optarg;
        } else if ('i' == opt_char) {
            *ip_address = optarg;
        } else if ('v' == opt_char) {
            *verbose = 1;
        } else if ('n' == opt_char) {
            version = 1;
        } else if ('h' == opt_char || '?' == opt_char) {
            help = 1;
        }
    }

    char *program = basename(argv[0]);

    if (help) {
        usage(program, 0, NULL);
    }

    if (version) {
        fprintf(stdout, "\n  %s version %s\n\n", program, VERSION);
        exit(0);
    }

    if (NULL == *mmdb_file) {
        usage(program, 1, "You must provide a filename with --file");
    }

    if (NULL == *ip_address) {
        usage(program, 1, "You must provide an IP address with --ip");
    }

    char **lookup_path = malloc(sizeof(char *) * ((argc - optind) + 1));
    int i;
    for (i = 0; i < argc - optind; i++) {
        lookup_path[i] = argv[i + optind];
        (*lookup_path_length)++;
    }
    lookup_path[i] = NULL;

    return lookup_path;
}

LOCAL MMDB_s open_or_die(const char *fname, int mode)
{
    MMDB_s mmdb;
    int status = MMDB_open(fname, MMDB_MODE_MMAP, &mmdb);

    if (MMDB_SUCCESS != status) {
        fprintf(stderr, "\n  Can't open %s\n\n", fname);
        exit(2);
    }

    return mmdb;
}

LOCAL MMDB_s dump_meta(MMDB_s *mmdb)
{
    const char *meta_dump = "\n"
                            "  Database metadata\n"
                            "    Node count:    %i\n"
                            "    Record size:   %i bits\n"
                            "    IP version:    IPv%i\n"
                            "    Binary format: %i.%i\n"
                            "    Build epoch:   %llu (%s)\n"
                            "    Type:          %s\n"
                            "    Languages:     ";

    char date[40];
    strftime(date, 40, "%F %T UTC",
             gmtime((const time_t *)&mmdb->metadata.build_epoch));

    fprintf(stdout, meta_dump,
            mmdb->metadata.node_count,
            mmdb->metadata.record_size,
            mmdb->metadata.ip_version,
            mmdb->metadata.binary_format_major_version,
            mmdb->metadata.binary_format_minor_version,
            mmdb->metadata.build_epoch,
            date,
            mmdb->metadata.database_type);

    for (int i = 0; i < mmdb->metadata.languages.count; i++) {
        fprintf(stdout, "%s", mmdb->metadata.languages.names[i]);
        if (i < mmdb->metadata.languages.count - 1) {
            fprintf(stdout, " ");
        }
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "    Description:\n");
    for (int i = 0; i < mmdb->metadata.description.count; i++) {
        fprintf(stdout, "      %s:   %s\n",
                mmdb->metadata.description.descriptions[i]->language,
                mmdb->metadata.description.descriptions[i]->description);
    }
    fprintf(stdout, "\n");
}

LOCAL MMDB_lookup_result_s lookup_or_die(MMDB_s *mmdb, const char *ipstr)
{
    int gai_error, mmdb_error;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(mmdb, ipstr, &gai_error, &mmdb_error);

    if (0 != gai_error) {
        fprintf(stderr, "\n  Error from call to getaddrinfo for %s - %s\n\n", ipstr,
                gai_strerror(gai_error));
        exit(3);
    }

    if (MMDB_SUCCESS != mmdb_error) {
        fprintf(stderr, "\n  Got an error from the maxminddb library: %s\n\n",
                MMDB_strerror(mmdb_error));
        exit(4);
    }

    return result;
}
