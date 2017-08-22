#include <rtems/rtems/fts_t.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/cpuuse.h>

#define MAX_IT

/* Initiate 24 bit pattern */
// bytes are initialized bottom to top here
uint8_t end_p = 0; // first byte of pattern
//uint8_t mid_p = 0; // second byte
uint8_t begin_p = 0; //last byte


uint8_t * p_s = &begin_p; // address of first byte
uint8_t * p_e  = &end_p;

/* set (m,k) */
uint8_t m = 12;
uint8_t k = 16;

/* set fault rate */
uint8_t fault_rate = 20; //percent; fault per task
uint32_t faults_T1 = 0; //nr of faults
uint32_t maxruns = 16; //nr of maximum runs
uint32_t runs = 1;
fts_tech curr_tech = SRE;

static const uint32_t Periods[] = { 200, 500, 750, 1000 };
static const rtems_name Task_name[] = {
  rtems_build_name( 'M', 'A', 'N', ' '),
  rtems_build_name( 'B', 'A', 'S', ' '),
  rtems_build_name( 'C', 'O', 'R', ' '),
  rtems_build_name( 'R', 'E', 'C', ' ')
};

static const rtems_task_priority Prio[3] = { 2, 5, 7, 9 };
static rtems_id   Task_id[ 4 ];
static uint32_t tsk_counter[] = { 0, 0, 0 }; //basic, detection, recovery

/* initialize seeds for rng */
uint8_t seed = 5;

/* fault injection function */
fault_status fault_injection(void)
{
  if(fault_rate == 0)
  {
    return NO_FAULT;
  }
  /*generate a random number and return fault status 32767*/
  int random = rand() / (RAND_MAX / (100 + 1) + 1);
  printf("\nT1: Random NR is %i\n", random);

  if ( random <= fault_rate )
  {
    faults_T1++;
    return FAULT;
  }
  else
  {
    return NO_FAULT;
  }
}

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

        if (p_curr == p_end && i == maxbit)
        {
          i = 10;
        }

        b_mask >>= 1;
    }
    printf("\n");
    //break;
  }
}

/* basic version task */
rtems_task BASIC_V(
  rtems_task_argument unused
)
{
  printf("\n------------------------\n");
  printf("\nBasic version starts!\n");
  printf("\n------------------------\n");
  rtems_id id = rtems_task_self();
  printf("\nBasic: My ID is %i\n", id);

  fault_status fs_T1 = fault_injection();

  switch(fs_T1)
  {
    case FAULT :
      printf("\n***T1: A fault occurred!***\n");
      break;

    case NO_FAULT :
      printf("\nT1: No fault\n");
      break;
  }

  printf("\nBasic version ends!\n");
  rtems_task_delete( rtems_task_self() );
}

/* Detection task version */
rtems_task DETECTION_V(
  rtems_task_argument unused
)
{
  printf("\n------------------------\n");
  printf("\nDetection version starts!\n");
  printf("\n------------------------\n");
  rtems_id id = rtems_task_self();
  printf("\nDetection: My ID is %i\n", id);

  fault_status fs_T1 = fault_injection();

  switch(fs_T1)
  {
    case FAULT :
      printf("\n***T1: A fault occurred!***\n");
      break;

    case NO_FAULT :
      printf("\nT1: No fault\n");
      break;
  }

  printf("\nDetection version ends!\n");
  rtems_task_delete( rtems_task_self() );
}

/* correction task version */
rtems_task CORRECTION_V(
  rtems_task_argument unused
)
{
  printf("\n------------------------\n");
  printf("\nCorrection version starts!\n");
  printf("\n------------------------\n");
  rtems_id id = rtems_task_self();
  printf("\nCorrection: My ID is %i\n", id);

  fault_status fs_T1 = fault_injection();

  switch(fs_T1)
  {
    case FAULT :
      printf("\n***T1: A fault occurred!***\n");
      break;

    case NO_FAULT :
      printf("\nT1: No fault\n");
      break;
  }

  printf("\nCorrection version ends!\n");
  rtems_task_delete( rtems_task_self() );
}

/* Task 1 - Test */
rtems_task FTS_MANAGER(
  rtems_task_argument unused
)
{
  rtems_id selfid = rtems_task_self();
  while (1)
  {
    printf("\n*****************************\n");
    printf("\nFTS_MANAGER starts!\n");
    printf("\nID: %i\n", selfid);
    printf("\nRun: %i\n", runs);
    if ( runs == 1 )
    {
      srand(seed);
      /* Print the addresses of pointers */
      printf("\nInit: Address of p_s: %p\n", (void *)p_s);
      //printf("\nT1: Address of p_m: %p\n", (void *)p_m);
      printf("\nInit: Address of p_e: %p\n", (void *)p_e);

      /* register the task set */
      uint8_t u = fts_rtems_task_register_t(selfid, m, k, SRE, R_PATTERN, p_s, p_e, 7);

      if (u == 1)
      {
        printf("\nTask registered\n");
      }
    }

    /* Pattern initialization and registration */
    if(runs == maxruns)
    {
      printf("\nAll runs finished.\n");
      rtems_task_delete( rtems_task_self() );
    }
    runs++;
    printf("\nManager ends!\n");

  }
};

/* Initialization: Create and start task */
rtems_task Init(
  rtems_task_argument ignored
)
{
  rtems_id id = rtems_task_self();
  printf("\nInit: ID: %i\n", id);

  int index;
  rtems_status_code status;

  /* Create FTS manager and task modes */
  for ( index = 0; index < RTEMS_ARRAY_SIZE(Task_name); ++index )
  {
    status = rtems_task_create(
      Task_name[ index ], Prio[index], RTEMS_MINIMUM_STACK_SIZE, RTEMS_DEFAULT_MODES,
      RTEMS_DEFAULT_ATTRIBUTES, &Task_id[ index ]
    );
  }
  // start FTS Manager task
  status = rtems_task_start( Task_id[ 0 ], FTS_MANAGER, 0);
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
