#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include "hea.h"
#include "rtems/cpuuse.h"

rtems_task Task_1(
  rtems_task_argument unused
)
{
    printf( "\nTASK_1\n\n");
};
