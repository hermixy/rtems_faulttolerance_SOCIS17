#include <rtems/rtems/types.h>/**
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
 * @brief Set static pattern for SRE
 *
 * This data structure is a bitmap which stores the execution pattern of a task.
 * If there is a "1" in the bitmap, the reliable version is executed, and if
 * there is a "0", the unreliable version is executed. The way of iteration is from
 * left to right and lower to higher memory address.
 * With pattern_start, the starting address of the bitmap is specified, and with
 * pattern_end, the ending address.
 * curr_pos stores the current position in the bitmap, and bitpos the current position
 * in the current byte. max_bitpos specifies the last bit-position that should be read
 * in the last byte.
 *
 * For a detailed description of the SRE bitmap refer to:
 * http://ls12-www.cs.tu-dortmund.de/daes/media/documents/publications/downloads/2016-khchen-lctes.pdf
 */
typedef struct Static_Pattern {
	uint8_t   	  *pattern_start;
	uint8_t       *pattern_end;
	uint8_t	            *curr_pos;
	uint8_t 	           bitpos;
  uint8_t        max_bitpos;
} bitstring_pattern;

/**
 * @brief Pointers to the corresponding task versions
 *
 * For a detailed description of the task versions refer to:
 * http://ls12-www.cs.tu-dortmund.de/daes/media/documents/publications/downloads/2016-khchen-lctes.pdf
 */
typedef struct Task_Versions {
  void (*basic_pointer)();
  error_status (*detection_pointer)();
  void (*recovery_pointer)();
} task_versions;

/**
 * @brief Register task for protection
 *
 * From the next activation on, the task with the given id will be protected,
 * using the technique specified in tech.
 *
 *1
 */
//uint8_t fts_rtems_task_register(
  //rtems_id id,
  //uint8_t m,
  //uint8_t k,
  //fts_tech tech
//);

/**
 * @brief Initialize avalible task versions
 *
 * Informs the FTS about the available task versions of the protected task.
 *1
 */
uint8_t fts_init_versions(void);

/**
 * @brief Return next execution version
 *
 * Returns the execution mode the protected task has to execute next.
 *1
 */
fts_version fts_get_mode(
  rtems_id id
);

/**
 * @brief Sign off from FTS
 *
 * Signs off a protected task from the FTS.
 *1
 */
uint8_t fts_off(
  rtems_id id
);

/**
 * @brief
 *
 * Gives information about whether the task with the given id
 * is registered for protection.
 *1
 */
uint8_t fts_task_status(
  rtems_id id
);

/**
 * @brief Changes the technique of a task
 *
 * Changes the fault tolerance technique of a specific task to the one
 * specified in the parameter.
 *1
 */
uint8_t fts_change_tech(
  rtems_id id,
  fts_tech tech
);

/**
 * @brief sets the sre pattern
 *
 *
 *
 *1
 */
int8_t fts_set_sre_pattern(
  rtems_id id,
  bitstring_pattern *bmap
);
