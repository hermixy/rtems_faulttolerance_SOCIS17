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

uint16_t counts = 0;

static const uint32_t Periods[] = { 1000, 200 };
static const rtems_name Task_name[] = {
  rtems_build_name( 'T', 'A', '1', ' ' ),
  rtems_build_name( 'T', 'A', '2', ' ' )
};

static const rtems_task_priority Prio[3] = { 2, 5 };
static uint32_t tsk_counter[] = { 0, 0 }; //basic, recovery
static rtems_id   Task_id[ 2 ];

uint8_t s_p(uint8_t *p_curr, uint8_t *p_end, uint8_t maxbit)
{
  /* Start output pattern */
  printf("\nT1: The pattern is:\n");

  for (; p_curr <= p_end; p_curr++)
  {
    uint8_t b_mask = 128;
    uint8_t c_byte = *p_curr;

    for (uint8_t i = 0; i < 8; i++)
    {
        uint8_t output = b_mask & c_byte;

        if (output == 0)
        {
          printf("0");
        }
        else
        {
          printf("1");
        }
        b_mask >>= 1;
    }
    printf("\n");
    //break;
  }
}

rtems_task Task_1(
  rtems_task_argument unused
)
{
  uint8_t runs = 0;
  while (runs <= 8) {

    printf("\n---------\n");
    runs++;
    /* (m,k) test */
    rtems_id selfid = rtems_task_self();

    uint8_t m = 11;
    uint8_t k = 16;

    /* begin test fts_rtems_task_register */
    if (runs == 1)
    {
      uint8_t reg_status = fts_rtems_task_register(selfid, m, k, curr_tech);
      if (reg_status == 1)
      {
        printf("\nT1: Task_1 was registered!\n");
      }
      else
      {
        printf("\nT: Task_1 ALREADY registered!\n");
      }
    }

    /* end test fts_rtems_task_register */

    /* Test setting a pattern */
    /* Initiate 16 bit pattern */

    uint8_t begin_p = 0;
    uint8_t end_p = 0;

    uint8_t * const p_s = &begin_p;
    //WHY can't I change the valie of *p_e when I do
    //uint8_t * const p_e  = p_s+1;
    uint8_t * const p_e  = &end_p;
    if (setpatt == 0)
    {


        printf("\nT1: Address of p_s: %p\n", (void *)p_s);
        printf("\nT1: Address of p_e: %p\n", (void *)p_e);
        uint8_t *p_curr = p_s;

        //for (; p_curr <= p_e; p_curr++)
          //{
            uint8_t b_mask = 1;
            uint8_t c_byte = *p_curr;

            /*
            for (uint8_t i = 0; i < 8; i++)
            {

              if(i==2)
              {
                c_byte = b_mask & c_byte;
              }
              else
              {
                c_byte = b_mask | c_byte;
              }
              b_mask <<= 1;
              }
              *p_curr = *p_curr | c_byte;
              */
              *p_s = 248;
              *p_e = 3;

          //}

          bitstring_pattern pattern = { .pattern_start = p_s, .pattern_end = p_e , .curr_pos = p_s, .bitpos = 0, .max_bitpos = 7};

          s_p(p_s, p_e, 7); // show pattern

          bitstring_pattern *p = &pattern;

          int8_t bm_status = fts_set_sre_pattern(selfid, p);
          if (bm_status==1)
          {
              printf("\nT1: Pattern is stored.\n");
          }
          setpatt = 1;
    }

    fts_version next_mode = fts_get_mode(selfid);
    switch(next_mode)
    {
      case RECOVERY :
        printf("\nT1: RECOVERY\n");
        break;

      case DETECTION :
        printf("\nT1: DETECTION\n");
        break;

      case BASIC :
        printf("\nT1: BASIC\n");
        break;
    }
    counts++;
    printf("\nT1: nr of jobs: %i/n", counts);
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
    printf( "\nunable to create task1\n" );
  }

  int i = 0;
  i = fts_init_versions();
  printf( "\n**TEST**\n" );
  printf("%d\n",i);
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
