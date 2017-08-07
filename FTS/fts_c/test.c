/*
 *  Classic API Hello World
 */

#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>
#include <rtems/rtems/fts.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/cpuuse.h>
//#include <rtems/macros.h>
//#include "test_support.h"

const char rtems_test_name[] = "FTS1";
static const uint32_t Periods[] = { 1000, 200 };
static const rtems_name Task_name[] = {
  rtems_build_name( 'T', 'A', '1', ' ' ),
  rtems_build_name( 'T', 'A', '2', ' ' )
};
static const rtems_task_priority Prio[3] = { 2, 5 };
static uint32_t tsk_counter[] = { 0, 0 }; //basic, recovery
static rtems_id   Task_id[ 2 ];


rtems_task Init(
  rtems_task_argument ignored
)
{
  uint32_t     index;
  rtems_status_code status;

  /* Create two tasks */
for ( index = 0; index < RTEMS_ARRAY_SIZE(Task_name); ++index ){
  // create task in dormant state
  status = rtems_task_create(
    Task_name[ index ], Prio[index], RTEMS_MINIMUM_STACK_SIZE, RTEMS_DEFAULT_MODES,
    RTEMS_DEFAULT_ATTRIBUTES, &Task_id[ index ]
  );

  //directive_failed( status, "rtems_task_create loop" );
}

  int i = 0;
  i = fts_init_versions();
  printf( "\n\n***TEST***\n" );
  printf("%d\n",i);
  exit( 0 );
}

/* configuration information */

#include <bsp.h>

/* NOTICE: the clock driver is explicitly disabled */
#define CONFIGURE_APPLICATION_DOES_NOT_NEED_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_USE_DEVFS_AS_BASE_FILESYSTEM
#define CONFIGURE_MICROSECONDS_PER_TICK     1000

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_MAXIMUM_TASKS 2

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
/* end of file */
