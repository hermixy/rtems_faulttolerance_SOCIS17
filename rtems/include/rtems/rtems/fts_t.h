#include <rtems/rtems/types.h>
#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>

#define NR_RANDS 128
// can protect up to P_TASK tasks
#define P_TASKS 10
#define BIT_7 128

/**
 * @brief The protection versions of a task
 *
 * The detection version can detect errors, the recovery version can correct errors.
 *
 */
typedef enum {
  BASIC,
  DETECTION,
  RECOVERY
} fts_version;
//fts_version gyrotask_vrsion = DETECTION;

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

rtems_id main_id[P_TASKS];

uint8_t flag[P_TASKS];
uint32_t faults; //nr of faults
rtems_id *period_pointers[P_TASKS];

/* Dynamic compensation specific data */
//just R-pattern for now
uint16_t o[P_TASKS]; //nr of unrel instances (chances)
uint16_t a[P_TASKS]; //nr of rel instances

uint16_t o_tolc[P_TASKS]; //nr of unrel instances (chances)
uint16_t a_tolc[P_TASKS]; //nr of rel instances

rtems_task BASIC_V(rtems_task_argument argument);

rtems_task DETECTION_V(rtems_task_argument argument);

rtems_task CORRECTION_V(rtems_task_argument argument);

static rtems_id   Task_id[ 5 ];

static const uint32_t Periods[] = { 1000, 1000, 1000, 1000, 1000 };
static const rtems_name Task_name[] = {
  rtems_build_name( 'M', 'A', 'N', ' '),
  rtems_build_name( 'B', 'A', 'S', ' '),
  rtems_build_name( 'C', 'O', 'R', ' '),
  rtems_build_name( 'R', 'E', 'C', ' '),
  rtems_build_name( 'R', 'M', 'T', ' ')
};

static const rtems_task_priority Prio[] = { 2, 1, 1, 1, 0 };

uint8_t rands_0_100[NR_RANDS];
uint8_t rand_count;
/**
 * @brief Register task for protection
 *
 * From the next activation on, the task with the given id will be protected,
 * using the technique specified in tech.
 *
 *1
 */

 void tolc_update_R(
   int i
 );

uint8_t fts_rtems_task_register_t(
  rtems_id *id, //id of the "main" task
  uint8_t m,
  uint8_t k,
  fts_tech tech,
  pattern_type pattern,
  uint8_t *pattern_start,
  uint8_t *pattern_end,
  uint8_t max_bitpos,
  rtems_task *basic,
  rtems_task *detection,
  rtems_task *recovery
);

/**
 * @brief Initialize avalible task versions
 *
 * Informs the FTS about the available task versions of the protected task.
 *1
 */
uint8_t fts_init_versions_t(void);

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
 *1
 */

uint8_t fts_task_status_t(
  rtems_id id
);


int8_t create_pattern_t(
  int i,
  pattern_type pattern
);

/**
 * @brief Changes the technique of a task
 *
 * Changes the fault tolerance technique of a specific task to the one
 * specified in the parameter.
 *1
 */
uint8_t fts_change_tech_t(
  rtems_id id,
  fts_tech tech
);

int16_t task_in_list_t(
  rtems_id id
);
