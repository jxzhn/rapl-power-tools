#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>
//#include "../powercap/inc/powercap-rapl.h"
#include <powercap/powercap-rapl.h>

//#define OUTPUT_TO_FILE
//#define CPU_FREQUENCY_NEEDED
//#define ALWAYS_FLUSH

#ifndef OUTPUT_TO_FILE
#undef CPU_FREQUENCY_NEEDED
#endif

int main(int argc, char* const argv[]) {
    unsigned int times = UINT_MAX;
    unsigned int interval = 1000000;
    int opt;
#ifndef OUTPUT_TO_FILE
    while ((opt = getopt(argc, argv, "i:t:")) != -1) { // parameter phrasing
#else
    unsigned file_index = 0;
    while ((opt = getopt(argc, argv, "i:t:f:")) != -1) {
#endif
        switch (opt) {
            case 'i':
                interval = 1000 * atoi(optarg);
                if (interval < 50000) {
                    printf("Measurement interval should be not less than 50ms\n");
                    exit(1);
                }
                break;
            case 't':
                times = atoi(optarg);
                break;
#ifdef OUTPUT_TO_FILE
            case 'f':
                file_index = atoi(optarg);
                break;
            default:
                printf("Usage: [-i [interval(ms)] optional] [-t [times] optional] [-f [index] optional]\n");
#else
            default:
                printf("Usage: [-i [interval(ms)] optional] [-t [times] optional]\n");
#endif
                exit(1);
                break;
        }
    }
#ifdef OUTPUT_TO_FILE
    char filename[100];
    sprintf(filename, "powerlog_%d.txt", file_index);
    FILE* output = fopen(filename, "w");
#ifdef CPU_FREQUENCY_NEEDED
    sprintf(filename, "freqlog_%d.txt", file_index);
    FILE* freqout = fopen(filename, "w");
#endif
#else
    FILE* output = stdout;
#endif
    // get number of top-level (parent) RAPL instances
    uint32_t count = powercap_rapl_get_num_packages();
    if (count == 0) {
        // none found (maybe the kernel module isn't loaded?)
        perror("powercap_rapl_get_num_packages");
        return -1;
    }
    powercap_rapl_pkg* pkgs = malloc(count * sizeof(powercap_rapl_pkg));
    // initialize
    uint32_t i;
    for (i = 0; i < count; i++) {
        if (powercap_rapl_init(i, &pkgs[i], 1)) { // read-only
            // could be that you don't have write privileges
            perror("powercap_rapl_init");
            return -1;
        }
    }
    // do a bunch of stuff with the interface here,
    // e.g., enable desired zones and get/set power caps...
    uint64_t* pkg_max = malloc(count * sizeof(uint64_t));
    uint64_t* dram_max = malloc(count * sizeof(uint64_t));
    for (i = 0; i < count; ++i) {
        powercap_rapl_get_max_energy_range_uj(&pkgs[i], POWERCAP_RAPL_ZONE_PACKAGE, &pkg_max[i]);
        powercap_rapl_get_max_energy_range_uj(&pkgs[i], POWERCAP_RAPL_ZONE_DRAM, &dram_max[i]);
    }
    uint64_t* pkg_msrs = malloc(count * sizeof(uint64_t));
    uint64_t* dram_msrs = malloc(count * sizeof(uint64_t));
    struct timeval* last_msr_times = malloc(count * sizeof(struct timeval));
    for (i = 0; i < count; ++i) {
        gettimeofday(&last_msr_times[i], NULL);
        powercap_rapl_get_energy_uj(&pkgs[i], POWERCAP_RAPL_ZONE_PACKAGE, &pkg_msrs[i]);
        powercap_rapl_get_energy_uj(&pkgs[i], POWERCAP_RAPL_ZONE_DRAM, &dram_msrs[i]);
        if (i != 0) fprintf(output, "    ");
        fprintf(output, "PKG%02d    total =   pakage(W) +     dram(W)", i);
    }
    fprintf(output, "\n");
#ifdef ALWAYS_FLUSH
    fflush(output);
#endif
    uint64_t tmpmsr1, tmpmsr2;
    struct timeval tmptime;
    unsigned int t;
    for (t = 0; t < times; ++t) {
        usleep(interval);
        for (i = 0; i < count; ++i) {
            gettimeofday(&tmptime, NULL);
            powercap_rapl_get_energy_uj(&pkgs[i], POWERCAP_RAPL_ZONE_PACKAGE, &tmpmsr1);
            powercap_rapl_get_energy_uj(&pkgs[i], POWERCAP_RAPL_ZONE_DRAM, &tmpmsr2);
            long long elapsed = (tmptime.tv_sec - last_msr_times[i].tv_sec) * 1000000LL + (tmptime.tv_usec - last_msr_times[i].tv_usec);
            double pkg_watts, dram_watts;
            if (tmpmsr1 < pkg_msrs[i]) { // RAPL overflow
                pkg_watts = (tmpmsr1 + pkg_max[i] - pkg_msrs[i]) / (double)elapsed;
            } else {
                pkg_watts = (tmpmsr1 - pkg_msrs[i]) / (double)elapsed;
            }
            if (tmpmsr2 < dram_msrs[i]) { // RAPL overflow
                dram_watts = (tmpmsr2 + dram_max[i] - dram_msrs[i]) / (double)elapsed;
            } else {
                dram_watts = (tmpmsr2 - dram_msrs[i]) / (double)elapsed;
            }
            if (i != 0) fprintf(output, "    ");
            fprintf(output, "#%04u %8lg = %8lg(W) + %8lg(W)", t, pkg_watts + dram_watts, pkg_watts, dram_watts);
            last_msr_times[i] = tmptime;
            pkg_msrs[i] = tmpmsr1;
            dram_msrs[i] = tmpmsr2; 
        }
        fprintf(output, "\n");
#ifdef CPU_FREQUENCY_NEEDED
        FILE* freqpipe = popen("cat /proc/cpuinfo|grep cpu\\ MHz|sed -e 's/.*:[^0-9]//'","r");
        if (!freqpipe) {
            perror("cpu frequency read fail");
            exit(1);
        }
        fprintf(freqout, "#%u", t);
        double freq;
        while (fscanf(freqpipe, "%lf", &freq) != EOF) {
            fprintf(freqout, " %lg", freq);
        }
        fprintf(freqout, "\n");
        fclose(freqpipe);
#endif
#ifdef ALWAYS_FLUSH
        fflush(output);
#ifdef CPU_FREQUENCY_NEEDED
        fflush(freqout);
#endif
#endif
    }
    free(pkg_max);
    free(dram_max);
    free(last_msr_times);
    free(pkg_msrs);
    free(dram_msrs);
#ifdef OUTPUT_TO_FILE
    fclose(output);
#ifdef CPU_FREQUENCY_NEEDED
    fclose(freqout);
#endif
#endif
    // now cleanup
    for (i = 0; i < count; i++) {
        if (powercap_rapl_destroy(&pkgs[i])) {
            perror("powercap_rapl_destroy");
        }
    }
    free(pkgs);
    return 0;
}