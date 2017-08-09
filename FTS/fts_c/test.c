#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>
#include <rtems/rtems/fts.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/cpuuse.h>


uint8_t setpatt = 0;
fts_tech curr_tech = SRE;
uint8_t pattsize = 16;
uint8_t pattern_s;
m_k mk = {4,4};
uint16_t counts = 0;

static const uint32_t Periods[] = { 1000, 200 };
static const rtems_name Task_name[] = {
  rtems_build_name( 'T', 'A', '1', ' ' ),
  rtems_build_name( 'T', 'A', '2', ' ' )
};

static const rtems_task_priority Prio[3] = { 2, 5 };
static uint32_t tsk_counter[] = { 0, 0 }; //basic, recovery
static rtems_id   Task_id[ 2 ];

rtems_task Task_1(
  rtems_task_argument unused
)
{
  while (1) {
    /* code */

    rtems_id selfid = rtems_task_self();

    uint8_t reg_status = fts_rtems_task_register(selfid, mk, curr_tech);
    if (reg_status == 1)
    {
      printf("\nTask_1 was registered!\n");
    }
    else{
      printf("\nTask_1 ALREADY registered!\n");
    }

    if (setpatt == 0)
    {
        uint8_t *b = &pattern_s;
        uint8_t *e = b+2;

        for (; b < e; ++b)
          {
            uint8_t bitw = 1;
            for (uint8_t i = 0; i < 8; i++ )
            {
              *b = bitw | *b;
              bitw = bitw << 1;
            }
          }

          bitstring_pattern bmap = { .pattern_start = &pattern_s, .pattern_end = &pattern_s+pattsize-1, .curr_pos = &pattern_s, .bitpos = 0, .max_bitpos = 7};
          int8_t bm_status = fts_set_sre_pattern(selfid, bmap);
          if (bm_status==1)
          {
              printf("BMAP IS IN");
          }
          setpatt = 1;
    }

    fts_version next_mode = fts_get_mode(selfid);
    switch(next_mode)
    {
      case RECOVERY :
        printf("RECOVERY\n");
        break;

      case DETECTION :
        printf("DETECTION\n");
        break;

      case BASIC :
        printf("BASIC\n");
        break;
    }
    counts++;
    printf("%i/n", counts);
  }
};


rtems_task Init(
  rtems_task_argument ignored
)
{
      uint32_t     index=1;
      rtems_status_code status;

      /* create task in dormant state */
      status = rtems_task_create(
      Task_name[ index ],
      Prio[index],
      RTEMS_MINIMUM_STACK_SIZE,
      RTEMS_DEFAULT_MODES,
      RTEMS_DEFAULT_ATTRIBUTES,
      &Task_id[ index ]
    );

  /* put tasks in ready state */
  status = rtems_task_start(Task_id[index], Task_1, index);

  if ( status )
  {
    printf( "unable to create task1\n" );
  }

  int i = 0;
  i = fts_init_versions();
  printf( "\n**TEST**\n" );
  printf("%d\n",i);
  printf( "\n**TEST**\n" );
  rtems_task_delete( rtems_task_self() );

}

/* configuration information */

#include <bsp.h>

/* NOTICE: the clock driver is explicitly disabled */
#define CONFIGURE_APPLICATION_DOES_NOT_NEED_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_USE_DEVFS_AS_BASE_FILESYSTEM
#define CONFIGURE_MICROSECONDS_PER_TICK     1000
#define CONFIGURE_MAXIMUM_PERIODS           3

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_MAXIMUM_TASKS 3

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
/* end of file */
