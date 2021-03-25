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
    if (argc < 2 || argc > 3)
        usage(argv[0]);

    const size_t MB = 1024 * 1024;
    const size_t PAGE = 4 * 1024;

    unsigned cpu = atoi(argv[1]);
    size_t memory = argc > 2 ? atoi(argv[2]) * MB : 0;
    size_t alloc = 0;

    char *ptr = NULL;
    tick_t busy = 0, sleeping = 0;
    tick_t print = tick();
    size_t loops = 0;
    double scale = 1000.0 * (100.0 - cpu) / cpu;
    double wanted = scale;

    while (++loops)
    {
        tick_t start = tick();
        if (!memory || alloc < memory)
        {
            alloc += PAGE;
            char *resized = realloc(ptr, alloc);
            if (!resized)
                printf("Allocation failed at %lu MB\n", alloc / MB);
            else
                ptr = resized;
        }

        for (char *p = ptr; p < ptr + alloc; p += PAGE)
            *((tick_t *) p) = start + p - ptr;

        tick_t duration = tick() - start;
        busy += duration;

        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = (tick_t) (scale * duration);
        nanosleep(&ts, NULL);

        tick_t slept = tick() - (start + duration);
        sleeping += slept;

        tick_t total = start - print;
        if (total > 1000000)
        {
            double mcpu = 100.0 * busy / total;
            double measured = 1000.0 * (100.0 - mcpu) / mcpu;
            scale *= wanted / measured;

            printf("Over %lu us, ratio=%lu.%02lu%%, %lu loops, %lu MB memory\n",
                   total, 100 * busy/total, 10000 * busy/total % 100,
                   loops, alloc / MB);

            print = start;
            busy = 0;
            sleeping = 0;
            loops = 0;
        }
    }
}
