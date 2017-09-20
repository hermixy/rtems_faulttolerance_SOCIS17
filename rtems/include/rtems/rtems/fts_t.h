#include <rtems/rtems/types.h>
#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>

/* set number of random values */
#define NR_RANDS 128

/* maximum number of protected tasks */
#define P_TASKS 10

/* 1000 0000 */
#define BIT_7 128

/* maximum number of partitions in E-pattern, if R-pattern is used, set to 1 */
#define PATTERN_PARTITIONS 16

/**
 * @brief The protection versions of a task
 *
 * The detection version can detect errors, the CORRECTION version can correct errors.
 *
 */
typedef enum {
  BASIC,
  DETECTION,
  CORRECTION
} fts_version;

/**
 * @brief Fault-tolerance technique of a task
 *
 * For a detailed description of the techniques, refer to:
 * http://ls12-www.cs.tu-dortmund.de/daes/media/documents/publications/downloads/2016-khchen-lctes.pdf
 */
typedef enum {
  NONE,
  SRE,
  SDR,
  DRE,
  DDR
} fts_tech;

/**
 * @brief (m,k)-pattern of a task
 *
 * If an R-pattern is used, the pattern first stores all "0"s and then the
 * "1"s follow, such as in {0,0,0,1,1,1}.
 * For E-patterns, the "1"s and "0"s are distributed evenly.
 *
 * For a more detailed description of R- and E-Pattern, refer to:
 * http://ls12-www.cs.tu-dortmund.de/daes/media/documents/publications/downloads/2016-khchen-lctes.pdf
 *
 */
typedef enum {
  R_PATTERN,
  E_PATTERN
} pattern_type;

/**
 * @brief status of existance of a fault
 *
 * If a fault was detected, the status is set to FAULT, and NO_FAULT when
 * no fault was detected.
 *
 */
typedef enum {
  FAULT,
  NO_FAULT
} fault_status;

/**
 * @brief status of existance of an error
 *
 * If an error was detected, the status is set to ERROR, and NO_ERROR when
 * no error was detected.
 *
 */
typedef enum {
  ERROR,
  NO_ERROR
} error_status;

/**
 * @brief mode indicator for dynamic compensation
 *
 * When set to TOLERANT, then the system can still tolerate errors
 * When set to SAFE, the system can not tolerante any more errors
 * and the correntness of the following instances needs to be guaranteed
 */
typedef enum {
  TOLERANT,
  SAFE
} mode_indicator;

/**
 * @brief (m,k) constaint of a task
 *
 * For a detailed description of (m,k) refer to:
 * http://ls12-www.cs.tu-dortmund.de/daes/media/documents/publications/downloads/2016-khchen-lctes.pdf
 */
typedef struct {
  uint8_t m;
  uint8_t k;
} m_k;

/**
 * @brief The list of all protected task IDs
 *
 * All the information needed for the FTS is stored in this data structure.
 */
struct Task_ID_List {
  /* IDs of protected tasks */
  rtems_id       task_list_id[P_TASKS];
  uint8_t        m[P_TASKS];
  uint8_t        k[P_TASKS];
  fts_tech       tech[P_TASKS];
  uint16_t       task_list_index; //always is one position ahead of last filled

  /* Pattern specific data */
  pattern_type   pattern[P_TASKS];
  uint8_t   	  *pattern_start[P_TASKS];
  uint8_t       *pattern_end[P_TASKS];
  uint8_t	      *curr_pos[P_TASKS];
  uint8_t 	     bitpos[P_TASKS];
  uint8_t        max_bitpos[P_TASKS];

  /* task pointers to the execution versions, basic, detection and correction version */
  rtems_task    *b[P_TASKS];
  rtems_task    *d[P_TASKS];
  rtems_task    *c[P_TASKS];
} list;

/* ID of task which creates and uses the period */
rtems_id main_id[P_TASKS];

/* pointer to the registered period ID */
rtems_id *period_pointers[P_TASKS];

/* IDs of tasks that can potentially be created within one period. All tasks created within one period need to be deleted at the start of the new period */
rtems_id running_id_b[P_TASKS];
rtems_id running_id_d[P_TASKS];
rtems_id running_id_c[P_TASKS];

/**
 * Tolerance counter data structure for dynamic compensation
 *
 * The number of partitions depends on the E-pattern.
 */
typedef struct TOLC {
  uint16_t tol_counter_o[PATTERN_PARTITIONS];
  uint16_t tol_counter_a[PATTERN_PARTITIONS];
} tol_counter;

/* Tolerance counter tol_counter is set once, and then never changed. */
tol_counter tol_counter_const[P_TASKS];
tol_counter tol_counter_temp[P_TASKS]; // counters

/* Number of partitions after setting partitions */
uint16_t nr_partitions[P_TASKS];
uint16_t partition_index[P_TASKS];

/* Task specifications for creation of the task versions */
struct Task_Specs {
  rtems_task_priority  initial_priority[P_TASKS];
  size_t               stack_size[P_TASKS];
  rtems_mode           initial_modes[P_TASKS];
  rtems_attribute      attribute_set[P_TASKS];
} task_specs;

/* Task specificatios for creation of task versions, for user */
typedef struct Task_Specs_User {
  rtems_task_priority  initial_priority;
  size_t               stack_size;
  rtems_mode           initial_modes;
  rtems_attribute      attribute_set;
} task_user_specs;

/* Struct that holds execution versions of a task  */
typedef struct Exec_Versions {
  rtems_task    *b;
  rtems_task    *d;
  rtems_task    *c;
} task_versions;

/* Struct that holds pattern information */
typedef struct Pattern_Info {
  pattern_type pattern;
  uint8_t *pattern_start;
  uint8_t *pattern_end;
  uint8_t max_bitpos;
} pattern_specs;

/* The three task versions; need to be implemented by the user */
/* Basic version only offers basic protection */
rtems_task BASIC_V(rtems_task_argument argument);
/* Detection version detects faults */
rtems_task DETECTION_V(rtems_task_argument argument);
/* Correction version version corrects errors induced by faults */
rtems_task CORRECTION_V(rtems_task_argument argument);

/* Task specific data */
static rtems_id   Task_id[ 3 ];
static const rtems_name Task_name[] = {
  rtems_build_name( 'B', 'A', 'S', ' '),
  rtems_build_name( 'D', 'E', 'T', ' '),
  rtems_build_name( 'C', 'O', 'R', ' ')
};
//static const rtems_task_priority Prio[] = { 1, 1, 1 };

/* total number of faults */
uint32_t faults;

/* random number array to simulate fault injection */
uint8_t rands_0_100[NR_RANDS];
uint8_t rand_count;


 /**
  * @brief Register task for protection
  *
  * From the next activation on, the task with the given id will be protected,
  * using the technique specified in tech.
  *
  *
  */
uint8_t fts_rtems_task_register_t(
  rtems_id *id, //id of the "main" task
  uint16_t m,
  uint16_t k,
  fts_tech tech,
  pattern_specs *pattern_s,
  task_versions *versions,
  task_user_specs *specs
);


/**
 * @brief Sign off from FTS
 *
 * Signs off a protected task from the FTS.
 *1
 */
uint8_t fts_off_t(
  rtems_id id
);

/**
 * @brief
 *
 * Gives information about whether the task with the given id
 * is registered for protection.
 *
 */

 int16_t task_in_list_t(
   rtems_id id
 );

 /**
  * @brief creates an (m,k) pattern
  *
  *
  */

int8_t create_pattern_t(
  int i,
  pattern_type pattern
);

/**
 * @brief Changes the technique of a task
 *
 * Changes the fault tolerance technique of a specific task to the one
 * specified in the parameter.
 *
 */
uint8_t fts_change_tech_t(
  rtems_id id,
  fts_tech tech
);

/**
 * @Gives info about whether the task with ID id is registered in FTS list
 *
 * Returns -1 if task is not in list, otherwise 1.
 *
 *
 */
int16_t task_in_list_t(
  rtems_id id
);
