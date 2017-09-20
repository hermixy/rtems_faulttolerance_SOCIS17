/**
 * @file
 *
 * @brief Create a Period
 * @ingroup ClassicRateMon Rate Monotonic Scheduler
 */

/*
 *  COPYRIGHT (c) 1989-2007.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/system.h>
#include <rtems/rtems/status.h>
#include <rtems/rtems/support.h>
#include <rtems/score/isr.h>
#include <rtems/rtems/ratemonimpl.h>
#include <rtems/score/thread.h>
#include <rtems/score/watchdogimpl.h>

#include <rtems/rtems/fts_t.h>
/*
 *  rtems_rate_monotonic_create
 *
 *  This directive creates a rate monotonic timer and performs
 *  some initialization.
 *
 *  Input parameters:
 *    name - name of period
 *    id   - pointer to rate monotonic id
 *
 *  Output parameters:
 *    id               - rate monotonic id
 *    RTEMS_SUCCESSFUL - if successful
 *    error code       - if unsuccessful
 */

rtems_status_code rtems_rate_monotonic_create(
  rtems_name  name,
  rtems_id   *id
)
{
  Rate_monotonic_Control *the_period;

  if ( !rtems_is_name_valid( name ) )
    return RTEMS_INVALID_NAME;

  if ( !id )
    return RTEMS_INVALID_ADDRESS;

  the_period = _Rate_monotonic_Allocate();

  if ( !the_period ) {
    _Objects_Allocator_unlock();
    return RTEMS_TOO_MANY;
  }

  _ISR_lock_Initialize( &the_period->Lock, "Rate Monotonic Period" );
  _Priority_Node_initialize( &the_period->Priority, 0 );
  _Priority_Node_set_inactive( &the_period->Priority );

  the_period->owner = _Thread_Get_executing();
  the_period->state = RATE_MONOTONIC_INACTIVE;

  _Watchdog_Preinitialize( &the_period->Timer, _Per_CPU_Get_by_index( 0 ) );
  _Watchdog_Initialize( &the_period->Timer, _Rate_monotonic_Timeout );

  _Rate_monotonic_Reset_statistics( the_period );

  _Objects_Open(
    &_Rate_monotonic_Information,
    &the_period->Object,
    (Objects_Name) name
  );

  *id = the_period->Object.id;
  _Objects_Allocator_unlock();
  return RTEMS_SUCCESSFUL;
}

/* */
rtems_status_code rtems_rate_monotonic_create_fts(
  rtems_name  name,
  rtems_id   *id,
  uint8_t m,
  uint8_t k,
  fts_tech tech,
  pattern_specs *pattern_s,
  task_versions *versions,
  task_user_specs *specs
)
{
  /* Rate monotonic control block, check ratemon.h */
  /* The ID of the period is stored in a Object_Control struct (which is in the Rate_monotonic_Control struct, which stores name and ID of period, as well as Node)*/
  Rate_monotonic_Control *the_period;

  /* Check if name is valid */
  if ( !rtems_is_name_valid( name ) )
    return RTEMS_INVALID_NAME;

  /* Check if ID of calling task is valid */
  if ( !id )
    return RTEMS_INVALID_ADDRESS;

  /* Allocates period control block from inactive chain (DLL) of free period control blocks, such as in ratemon.h */
  the_period = _Rate_monotonic_Allocate();

  /* Unlocks the object allocator mutex if there are too many in chain */
  if ( !the_period ) {
    _Objects_Allocator_unlock();
    return RTEMS_TOO_MANY;
  }

  /* Initialize ISR lock to begin critical section */
  _ISR_lock_Initialize( &the_period->Lock, "Rate Monotonic Period" );

  /* the_period is a priority_node struct, includes priority and a node object (rbtree + chain node) to build the priority aggregate, see priority.h */
  /* in priorotyimpl.h, takes priority_node and actual priority. why 0 ? */
  _Priority_Node_initialize( &the_period->Priority, 0 );
  /* sets inactive in tree */
  _Priority_Node_set_inactive( &the_period->Priority );
  /* the task who called the create, return the thread control block of the executing thread; SET OWNER by setting owner's TCB pointer! */
  the_period->owner = _Thread_Get_executing();
  /* Set period as inactive */
  the_period->state = RATE_MONOTONIC_INACTIVE;
  /* Preinitialize watchdog, must be called before watchdog used, pass timer */
  //what is per_cpu
  _Watchdog_Preinitialize( &the_period->Timer, _Per_CPU_Get_by_index( 0 ) );
  /* Watchdog is a timer, for timeout: routine is invoked when the period represented by ID expires */
  _Watchdog_Initialize( &the_period->Timer, _Rate_monotonic_Timeout );

  _Rate_monotonic_Reset_statistics( the_period );
  /* places object control pointer and object name into Object information Table */
  _Objects_Open(
    &_Rate_monotonic_Information,
    &the_period->Object,
    (Objects_Name) name
  );
  /* Object is Objects_Control type and has id, node and name. Are stored in a chain */
  // but this was not set ? maybe later...
  *id = the_period->Object.id;
  /* No need to lock object anymore, finished setting data */
  _Objects_Allocator_unlock();

  /* Set FTS data */
  uint8_t reg = fts_rtems_task_register_t(
  id, //id of the period. // in threadq.h, the struct of the TCB is defined
  m,
  k,
  tech,
  pattern_s,
  versions,
  specs
  );

  if ( reg == 0 ) {
    printf("\nCould not register ID in the FTS\n");
    return RTEMS_INVALID_NUMBER;
  }
  return RTEMS_SUCCESSFUL;
}
