#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>
#include <rtems/rtems/fts.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/cpuuse.h>

#define MAX_IT


/* Initiate 24 bit pattern */
// bytes are initialized bottom to top
uint8_t end_p = 0; // first byte of pattern
uint8_t mid_p = 0; // second byte
uint8_t begin_p = 0; //last byte


 uint8_t * const  p_s = &begin_p; // address of first byte
//why can't I change the valie of *p_e when I do
//uint8_t * const p_e  = p_s+1;
uint8_t * const p_m  = &mid_p;
uint8_t * const p_e  = &end_p;


uint8_t setpatt = 0;
fts_tech curr_tech = SRE;

static const uint32_t Periods[] = { 1000, 200 };
static const rtems_name Task_name[] = {
  rtems_build_name( 'T', 'A', '1', ' ' ),
  rtems_build_name( 'T', 'A', '2', ' ' )
};

static const rtems_task_priority Prio[3] = { 2, 5 };
static rtems_id   Task_id[ 2 ];


/* Show pattern */
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

/* Task 1 - Test */
rtems_task Task_1(
  rtems_task_argument unused
)
{
  static uint32_t tsk_counter[] = { 0, 0 }; //basic, recovery
  static uint8_t runs = 1;
  while (runs <= 24) {


    printf("\n------------------------\n");
    printf("\nTask 1 starts!\n");


    rtems_id selfid = rtems_task_self();

    /* (m,k) test - set (m,k) */
    uint8_t m = 11;
    uint8_t k = 16;

    /* test fts_rtems_task_register */
    if (runs == 1) //only first run
    {
      /* Register Task to FTS */
      uint8_t reg_status = fts_rtems_task_register(selfid, m, k, curr_tech);
      if (reg_status == 1)
      {
        printf("\nT1: Task_1 was registered!\n");
      }
      else
      {
        printf("\nT: Task_1 already registered!\n");
      }
    }

    /* Test setting a pattern */



    /* Pattern initialization and registration */
    if (setpatt == 0) // only once
    {
        /* Print the addresses of pointers */
;

        /* TODO: Algorithm to set pattern  */
        //for (; p_curr <= p_e; p_curr++)
          //{
            uint8_t b_mask = 1;

            printf("\nT1: Address of p_s: %p\n", (void *)p_s);
            printf("\nT1: Address of p_m: %p\n", (void *)p_m);
            printf("\nT1: Address of p_e: %p\n", (void *)p_e);
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
              *p_s = 0;
              *p_m = 31;
              *p_e = 255;

          //}

          /* Build pattern struct */
          bitstring_pattern pattern = { .pattern_start = p_s, .pattern_end = p_e , .curr_pos = p_s, .bitpos = 0, .max_bitpos = 7};

          printf("\nT1: Address of p_s: %p\n", (void *)pattern.pattern_start);
          printf("\nT1: Address of p_e: %p\n", (void *)pattern.pattern_end);
          printf("\nT1: Address of current: %p\n", (void *)pattern.curr_pos);
          /* show pattern */
          s_p(p_s, p_e, 7);

          bitstring_pattern *p = &pattern;

          /* Set pattern to the FTS */
          int8_t bm_status = fts_set_sre_pattern(selfid, p);
          if (bm_status==1)
          {
              printf("\nT1: Pattern is stored.\n");
          }
          setpatt = 1;
    }

    /* Get execution mode for this task instance */
    fts_version next_mode = fts_get_mode(selfid);
    switch(next_mode)
    {
      case RECOVERY :
        printf("\nT1: RECOVERY\n");
        tsk_counter[1]++;
        break;

      case DETECTION :
        printf("\nT1: DETECTION\n");
        break;

      case BASIC :
        printf("\nT1: BASIC\n");
        tsk_counter[0]++;
        break;
    }
    printf("\nT1: nr of jobs: %i\n", runs);

  if (runs == 24)
  {
    printf("\nT1: #B: %i, #R: %i\n", tsk_counter[0], tsk_counter[1]);
  }

  runs++; // only a few jobs
  printf("\nTask 1 ends!\n");
}

};

/* Initialization: Create and start task */
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

  rtems_id id = rtems_task_self();
  printf("\nInit: fts.c: ID: %i\n", id);

  if ( status )
  {
    printf( "\nunable to create task1\n" );
  }

  int i = 0;
  i = fts_init_versions();
  printf( "\nInit: **FTS TEST**\n" );
  printf("\nInit: %d\n",i);
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
