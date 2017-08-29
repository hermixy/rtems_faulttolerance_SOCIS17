#include <rtems/rtems/fts_t.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/cpuuse.h>

#define MAX_IT

/* Initiate 24 bit pattern */
// bytes are initialized bottom to top here
uint8_t end_p = 0; // last byte of pattern
//uint8_t mid_p = 0; // second byte
uint8_t begin_p = 0; //first byte


uint8_t * p_s = &begin_p; // address of first byte
uint8_t * p_e  = &end_p;

/* rtems task pointers */
rtems_task *basic;
rtems_task *detection;
rtems_task *recovery;

/* set (m,k) */
uint8_t m = 12;
uint8_t k = 16;

/* set fault rate */
uint8_t fault_rate = 20; //percent; fault per task
uint32_t maxruns = 16; //nr of maximum runs
uint32_t runs = 1;
fts_tech curr_tech = SRE;
pattern_type patt = R_PATTERN;
/* MOVED TO FTS_T.H */

// static const uint32_t Periods[] = { 1000, 1000, 1000, 1000 };
// static const rtems_name Task_name[] = {
//   rtems_build_name( 'M', 'A', 'N', ' '),
//   rtems_build_name( 'B', 'A', 'S', ' '),
//   rtems_build_name( 'C', 'O', 'R', ' '),
//   rtems_build_name( 'R', 'E', 'C', ' ')
// };
//
// static const rtems_task_priority Prio[] = { 9, 4, 4, 4 };
// //static rtems_id   Task_id[ 4 ];
// static uint32_t tsk_counter[] = { 0, 0, 0 }; //basic, detection, recovery


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
  printf("\nT: Random NR is %i\n", random);

  if ( random <= fault_rate )
  {
    faults++;
    return FAULT;
  }
  else
  {
    return NO_FAULT;
  }
}

/* basic version task */
rtems_task BASIC_V(
  rtems_task_argument unused
)
{
  printf("\n------------------------\n");
  printf("\nBasic version starts!\n");
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
  printf("\n------------------------\n");
  tsk_counter[0]++;
  rtems_task_delete( rtems_task_self() );
}

/* Detection task version */
rtems_task DETECTION_V(
  rtems_task_argument unused
)
{
  printf("\n------------------------\n");
  printf("\nDetection version starts!\n");
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
  printf("\n------------------------\n");
  tsk_counter[1]++;
  rtems_task_delete( rtems_task_self() );
}

/* correction task version */
rtems_task CORRECTION_V(
  rtems_task_argument unused
)
{
  printf("\n------------------------\n");
  printf("\nCorrection version starts!\n");
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
  printf("\n------------------------\n");
  tsk_counter[2]++;
  rtems_task_delete( rtems_task_self() );
}

/* Task 1 - Test */
rtems_task FTS_MANAGER(
  rtems_task_argument unused
)
{
  rtems_status_code                          status;
  rtems_id                                   RM_period;
  rtems_id                                   selfid = rtems_task_self();
  rtems_rate_monotonic_period_status         period_status;
  srand(seed);

  printf("\nAddress of S: %p\n", (void *)p_s);
  printf("\nAddress of E: %p\n", (void *)p_e);

  /* Store addresses of task pointers */
  basic = &BASIC_V;
  detection = &DETECTION_V;
  recovery = &CORRECTION_V;

  /* Print addresses of task pointers */
  printf("\nAddress of B: %p\n", (void *)basic);
  printf("\nAddress of D: %p\n", (void *)detection);
  printf("\nAddress of R: %p\n", (void *)recovery);

  /* create a period and register the task set for FTS */
   status = rtems_rate_monotonic_create_fts( Task_name[ 4 ], &RM_period,
   m, k, curr_tech, patt, p_s, p_e, 7, basic, detection, recovery);
   task_status(status);

  while (1)
  {
    // status = rtems_rate_monotonic_get_status( RM_period, &period_status );
    // task_status(status);
    status = rtems_rate_monotonic_period( RM_period, 100 );

    printf("\n*****************************\n");
    printf("\nFTS_RM_TEST starts!\n");
    printf("\nID: %i\n", selfid);
    printf("\nRun: %i\n", runs);

    /* Pattern initialization and registration */
    if(runs == maxruns)
    {
      printf("\nAll runs finished.\n");
      printf("\nNr of jobs: %i\n", runs);
      printf("\nNr of faults: %i\n", faults );
      printf("\nFault rate: %i%%\n", fault_rate );
      printf("\n#B: %i, #D: %i, #R: %i\n", tsk_counter[0], tsk_counter[1], tsk_counter[2]);
      rtems_task_delete( rtems_task_self() );
    }
    runs++;
    printf("\nFTS_RM_TEST ends!\n");
    printf("\n*****************************\n");

    if( status == RTEMS_TIMEOUT )
    {
      printf("\nPERIOD TIMEOUT\n");
    }
  }
  printf("\nBROKE\n");
  status = rtems_rate_monotonic_delete( RM_period );
  if ( status != RTEMS_SUCCESSFUL )
  {
      printf( "rtems_rate_monotonic_delete failed with status of %d.\n", status );
      exit( 1 );
  }
  status = rtems_task_delete( selfid );    /* should not return */
  printf( "rtems_task_delete returned with status of %d.\n", status );
  exit( 1 );
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

  /* Create Task FTS */
  status = rtems_task_create(
  Task_name[0], Prio[0], RTEMS_MINIMUM_STACK_SIZE, RTEMS_DEFAULT_MODES,
  RTEMS_DEFAULT_ATTRIBUTES, &Task_id[0]
    );
  task_status(status);

  // start FTS Manager task
  status = rtems_task_start( Task_id[ 0 ], FTS_MANAGER, 0);
  task_status(status);
  rtems_task_delete( rtems_task_self() );
}

/* configuration information */

#include <bsp.h>

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_USE_DEVFS_AS_BASE_FILESYSTEM
#define CONFIGURE_MICROSECONDS_PER_TICK     1000
#define CONFIGURE_MAXIMUM_PERIODS           10

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_MAXIMUM_TASKS 10

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
/* end of file */
