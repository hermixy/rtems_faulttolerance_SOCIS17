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
uint8_t * p_s = &begin_p;
uint8_t * p_e  = &end_p;
pattern_type patt = E_PATTERN;
uint8_t maxbit = 7;
pattern_specs p_specs;

/* task versions struct containing the task pointers */
task_versions versions;

/* task user specs */
task_user_specs task_sp;

/* set (m,k) */
uint8_t m = 12;
uint8_t k = 16;

/* set fault rate */
uint8_t fault_rate = 100; //percent; fault per task
uint32_t maxruns = 32; //nr of maximum runs
uint32_t runs = 1;
fts_tech curr_tech = DDR;

static uint32_t tsk_counter[] = { 0, 0, 0 }; //basic, detection, correction

/* initialize seeds for rng */
uint8_t seed = 5;


/* fill array with random numbers */
void rand_nr_list(void)
{
  for ( uint16_t i = 0; i < NR_RANDS; i++ )
  {
    rands_0_100[i] = rand() / (RAND_MAX / (100 + 1) + 1);
  }
  return;
}

/* get a random number out of the random number array */
uint8_t get_rand(void)
{
  /* rand_count always indexes the random nr which was not used yet */
  return rands_0_100[rand_count++];
}

/* get fault status */
fault_status get_fault(uint8_t rand_nr)
{
  if(fault_rate == 0)
  {
    return NO_FAULT;
  }

  if (fault_rate == 100)
  {
    faults++;
    return FAULT;
  }

  printf("\nRandom NR is %i\n", rand_nr);

  if ( rand_nr <= fault_rate )
  {
    faults++;
    return FAULT;
  }
  else
  {
    return NO_FAULT;
  }
}

/* display all random values */
void show_rands(void)
{
  printf("\nNr of rands: %i\n", NR_RANDS);
  for (int i = 0; i < NR_RANDS; i++)
  {
    if (i == 0)
    {
      printf("\nRandom numbers:\n");
    }
    printf("%i\n", rands_0_100[i]);
  }
}

/* basic task version */
rtems_task BASIC_V(
  rtems_task_argument argument
)
{
  printf("\n------------------------\n");
  printf("\nBasic version starts!\n");
  rtems_id id = rtems_task_self();
  printf("\nBasic: My ID is %i\n", id);
  id = *((rtems_id*)argument );
  printf("Basic: ID of my Period is %i\n", id);

  fault_status fs_T1 = get_fault(get_rand());

  switch(fs_T1)
  {
    case FAULT :
      printf("\n***B: A fault occurred!***\n");
      break;

    case NO_FAULT :
      printf("\nB: No fault\n");
      break;
  }

  printf("\nBasic version ends!\n");
  printf("\n------------------------\n");
  tsk_counter[0]++;
  rtems_task_delete( rtems_task_self() );
}

/* Detection task version */
rtems_task DETECTION_V(
  rtems_task_argument argument
)
{
  printf("\n------------------------\n");
  printf("\nDetection version starts!\n");
  rtems_id id = rtems_task_self();
  printf("\nDetection: My ID is %i\n", id);
  id = *((rtems_id*)argument );
  printf("Detection: ID of my Period is %i\n", id);

  fault_status fs_T1 = get_fault(get_rand());

  switch(fs_T1)
  {
    case FAULT :
      printf("\n***D: A fault occurred!***\n");
    break;

    case NO_FAULT :
      printf("\nD: No fault\n");
    break;
  }

  /* Call the fault detection routine at the end of the detection version */
  fault_detection_routine(id, fs_T1);

  printf("\nDetection version ends!\n");
  printf("\n------------------------\n");
  tsk_counter[1]++;
  rtems_task_delete( rtems_task_self() );
}

/* correction task version */
rtems_task CORRECTION_V(
  rtems_task_argument argument
)
{
  printf("\n------------------------\n");
  printf("\nCorrection version starts!\n");
  rtems_id id = rtems_task_self();
  printf("\nCorrection: My ID is %i\n", id);
  id = *((rtems_id*)argument );
  printf("Correction: ID of my Period is %i\n", id);

  fault_status fs_T1 = get_fault(get_rand());

  switch(fs_T1)
  {
    case FAULT :
      printf("\n***C: A fault occurred!***\n");
    break;

    case NO_FAULT :
      printf("\nC: No fault\n");
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
  rtems_name                                 rm_name = rtems_build_name( 'R', 'M', 'T', ' ');
  rtems_id                                   selfid = rtems_task_self();
  rtems_rate_monotonic_period_status         period_status;

  faults = 0;
  srand(seed);
  rand_nr_list();
  show_rands();

  printf("\nAddress of S: %p\n", (void *)p_s);
  printf("\nAddress of E: %p\n", (void *)p_e);

  /* Store addresses of task pointers */
  versions.b = &BASIC_V;
  versions.d = &DETECTION_V;
  versions.c = &CORRECTION_V;

  /* Print addresses of task pointers */
  printf("\nAddress of B: %p\n", (void *)versions.b);
  printf("\nAddress of D: %p\n", (void *)versions.d);
  printf("\nAddress of R: %p\n", (void *)versions.d);

  /* Pattern specifications */
  p_specs.pattern = patt;
  p_specs.pattern_start = p_s;
  p_specs.pattern_end = p_e;
  p_specs.max_bitpos = maxbit;

  rtems_task_priority prio = 1;
  /* task version specification */
  task_sp.initial_priority = prio;
  task_sp.stack_size = RTEMS_MINIMUM_STACK_SIZE;
  task_sp.initial_modes = RTEMS_DEFAULT_MODES;
  task_sp.attribute_set = RTEMS_DEFAULT_ATTRIBUTES;

  /* create a period and register the task set for RM-FTS */
  status = rtems_rate_monotonic_create_fts( rm_name, &RM_period,
   m, k, curr_tech, &p_specs, &versions, &task_sp );
  task_status(status);

  while (1)
  {
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

  rtems_name fts_test = rtems_build_name( 'M', 'A', 'N', ' ');
  rtems_id fts_test_id;

  status = rtems_task_create( fts_test, 2, RTEMS_MINIMUM_STACK_SIZE, RTEMS_DEFAULT_MODES,
  RTEMS_DEFAULT_ATTRIBUTES, &fts_test_id );
  task_status(status);

  // start FTS Manager task
  status = rtems_task_start( fts_test_id, FTS_MANAGER, 0);
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
