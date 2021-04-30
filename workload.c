// ****************************************************************************
//  workload.c
// ****************************************************************************
//
//   File Description:
//
//     A very simple programmable workload that takes a specified amount
//     of CPU and memory from your system
//
//
//
//
//
//
//
// ****************************************************************************
//   (C) 2021 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
// ****************************************************************************
//   This file is part of Workload.
//
//   Workload is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Workload is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Workload.  If not, see <https://www.gnu.org/licenses/>.
// ****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>


static void usage(const char *progname)
// ----------------------------------------------------------------------------
//   Display usage information
// ----------------------------------------------------------------------------
{
    printf("%s <cpu> <memory>\n"
           "  Run a workload that consumes roughly\n"
           "  <cpu> percent of system CPU and\n"
           "  <memory> MB of active memory\n"
           "  For memory, use 0 or nothing to get as much as possible,\n"
           "  and report when allocations fail\n",
           progname);
    exit(1);
}


typedef unsigned long tick_t;

tick_t tick(void)
// ----------------------------------------------------------------------------
//   Return the time elapsed in us.
// ----------------------------------------------------------------------------
{
    static tick_t initialTick = 0;
    struct timeval t;
    gettimeofday(&t, NULL);
    tick_t tick = t.tv_sec * 1000000 + t.tv_usec;
    if (!initialTick)
        initialTick = tick;
    return tick - initialTick;
}


int main(int argc, char **argv)
// ----------------------------------------------------------------------------
//   Process options and run the workload
// ----------------------------------------------------------------------------
{
    if (argc < 2 || argc > 4)
        usage(argv[0]);

    const size_t MB = 1024 * 1024;
    const size_t PAGE = 4 * 1024;

    double cpu = atof(argv[1]);
    size_t memory = argc > 2 ? (size_t) (atof(argv[2]) * MB) : 0;
    size_t increment = argc > 3 ? (size_t) (atof(argv[3]) * MB) : PAGE;
    size_t alloc = 0;

    char *ptr = NULL;
    tick_t busy = 0, sleeping = 0;
    tick_t print = tick();
    size_t loops = 0;
    size_t work_units = 0;
    size_t signaled = 0;
    double scale = 1000.0 * (100.0 - cpu) / cpu;
    double wanted = scale;
    char * rname = getenv("REPORT");
    if (rname)
        rname = mktemp(strdup(rname));

    if (memory)
        printf("Using %.2f%% CPU and %lu.%02luMB memory"
               " in %lu.%02luMB increments\n",
               cpu,
               memory / MB, memory * 100 / MB % 100,
               increment / MB, increment * 100 / MB % 100);
    else
        printf("Using %.2f%% CPU and unlimited memory"
               " in %lu.%02luMB increments\n",
               cpu,
               increment / MB, increment * 100 / MB % 100);

    while (++loops)
    {
        tick_t start = tick();
        if (!memory || alloc < memory)
        {
            alloc += increment;
            char *resized = realloc(ptr, alloc);
            if (!resized)
            {
                printf("Allocation failed at %lu MB\n", alloc / MB);
                alloc -= increment;
                increment /= 2;
            }
            else
            {
                ptr = resized;
            }
        }

        for (char *p = ptr; p < ptr + alloc; p += PAGE)
        {
            work_units++;
            *((tick_t *) p) = start + p - ptr;
        }

        tick_t duration = tick() - start;
        busy += duration;

        tick_t scaled = (tick_t) (scale * duration);
        struct timespec ts;
        ts.tv_sec = scaled / 1000000000;
        ts.tv_nsec = scaled % 1000000000;
        while (ts.tv_nsec || ts.tv_sec)
        {
            int rc = nanosleep(&ts, &ts);
            if (!rc)
                break;
            signaled++;
        }

        tick_t slept = tick() - (start + duration);
        sleeping += slept;

        tick_t total = start - print;
        if (total > 1000000)
        {
            double mcpu = 100.0 * busy / total;
            double measured = 1000.0 * (100.0 - mcpu) / mcpu;
            double target = scale * wanted / measured;
            if (target < 0.01 * wanted)
                target = 0.01 * wanted;
            else if (target > 100.0 * wanted)
                target = 100.0 * wanted;
            scale = 0.1 * target + 0.9 * scale;

            printf("Over %lu us, ratio=%lu.%02lu%%, scaling %5.2f%%, "
                   "%lu loops %lu signals, %lu MB memory\n",
                   total, 100 * busy/total, 10000 * busy/total % 100,
                   100.0 * scale / wanted,
                   loops, signaled, alloc / MB);

            if (rname)
            {
                FILE *report = fopen(rname, "w");
                fprintf(report, "%lu %lu\n", work_units, total);
                fclose(report);
            }

            print = start;
            busy = 0;
            sleeping = 0;
            loops = 0;
            signaled = 0;
            work_units = 0;
        }
    }
}
