#include <inttypes.h>
#include <rtems.h>

rtems_task Task_1(
  rtems_task_argument argument
);

static const uint32_t Periods[] = { 1000, 200 };
static const rtems_name Task_name[] = {
  rtems_build_name( 'T', 'A', '1', ' ' ),
  rtems_build_name( 'T', 'A', '2', ' ' )
};
static const rtems_task_priority Prio[3] = { 2, 5 };
static uint32_t tsk_counter[] = { 0, 0 }; //basic, recovery
static rtems_id   Task_id[ 2 ];
