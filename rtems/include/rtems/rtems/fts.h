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
 * right to left and top to bottom.
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
	uint8_t * const  	   pattern_start;
	uint8_t * const      pattern_end;
	uint8_t	     *curr_pos;
	uint8_t 	    bitpos;
  uint8_t       max_bitpos;
} bitstring_pattern;

/**
 * @brief Register task for protection
 *
 * From the next activation on, the task with the given id will be protected,
 * using the technique specified in tech.
 *
 *1
 */
uint8_t fts_rtems_task_register(
  rtems_id id,
  uint8_t m,
  uint8_t k,
  fts_tech tech
);

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
