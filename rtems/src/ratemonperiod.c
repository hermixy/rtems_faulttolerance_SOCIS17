/**
 *  @file
 *
 *  @brief Rate Monotonic Support
 *  @ingroup ClassicRateMon
 */

/**
 *  COPYRIGHT (c) 1989-2010.
 *  On-Line Applications Research Corporation (OAR).
 *  Copyright (c) 2016 embedded brains GmbH.
 *  COPYRIGHT (c) 2016 Kuan-Hsun Chen.
 *  COPYRIGHT (c) 2017 Mikail Yayla.
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/rtems/ratemonimpl.h>
#include <rtems/score/schedulerimpl.h>
#include <rtems/score/todimpl.h>
#include <rtems/rtems/fts_t.h>

bool _Rate_monotonic_Get_status(
  const Rate_monotonic_Control *the_period,
  Timestamp_Control            *wall_since_last_period,
  Timestamp_Control            *cpu_since_last_period
)
{
  Timestamp_Control        uptime;
  Thread_Control          *owning_thread = the_period->owner;
  Timestamp_Control        used;

  /*
   *  Determine elapsed wall time since period initiated.
   */
  _TOD_Get_uptime( &uptime );
  _Timestamp_Subtract(
    &the_period->time_period_initiated, &uptime, wall_since_last_period
  );

  /*
   *  Determine cpu usage since period initiated.
   */
  _Thread_Get_CPU_time_used( owning_thread, &used );

  /*
   *  The cpu usage info was reset while executing.  Can't
   *  determine a status.
   */
  if ( _Timestamp_Less_than( &used, &the_period->cpu_usage_period_initiated ) )
    return false;

   /* used = current cpu usage - cpu usage at start of period */
  _Timestamp_Subtract(
    &the_period->cpu_usage_period_initiated,
    &used,
    cpu_since_last_period
  );

  return true;
}

static void _Rate_monotonic_Release_postponed_job(
  Rate_monotonic_Control *the_period,
  Thread_Control         *owner,
  rtems_interval          next_length,
  ISR_lock_Context       *lock_context
)
{
  Per_CPU_Control      *cpu_self;
  Thread_queue_Context  queue_context;

  --the_period->postponed_jobs;
  _Scheduler_Release_job(
    owner,
    &the_period->Priority,
    the_period->latest_deadline,
    &queue_context
  );

  cpu_self = _Thread_Dispatch_disable_critical( lock_context );
  _Rate_monotonic_Release( the_period, lock_context );
  _Thread_Priority_update( &queue_context );
  _Thread_Dispatch_enable( cpu_self );

  /* call FTS to schedule tasks */
  fts_version ver = fts_compensate_t(the_period->Object.id);
}

static void _Rate_monotonic_Release_job(
  Rate_monotonic_Control *the_period,
  Thread_Control         *owner,
  rtems_interval          next_length,
  ISR_lock_Context       *lock_context
)
{
  Per_CPU_Control      *cpu_self;
  Thread_queue_Context  queue_context;
  uint64_t              deadline;

  cpu_self = _Thread_Dispatch_disable_critical( lock_context );

  deadline = _Watchdog_Per_CPU_insert_relative(
    &the_period->Timer,
    cpu_self,
    next_length
  );
  _Scheduler_Release_job(
    owner,
    &the_period->Priority,
    deadline,
    &queue_context
  );
  /* I think here is best YY */

  _Rate_monotonic_Release( the_period, lock_context );
  _Thread_Priority_update( &queue_context );
  _Thread_Dispatch_enable( cpu_self );

  /* call FTS to schedule tasks */
  fts_version ver = fts_compensate_t(the_period->Object.id);
}

void _Rate_monotonic_Restart(
  Rate_monotonic_Control *the_period,
  Thread_Control         *owner,
  ISR_lock_Context       *lock_context
)
{
  /*
   *  Set the starting point and the CPU time used for the statistics.
   */
  _TOD_Get_uptime( &the_period->time_period_initiated );
  _Thread_Get_CPU_time_used( owner, &the_period->cpu_usage_period_initiated );

  _Rate_monotonic_Release_job(
    the_period,
    owner,
    the_period->next_length,
    lock_context
  );
  /* OR FTS Code here? */
  /* YES I think so - M*/
}

static void _Rate_monotonic_Update_statistics(
  Rate_monotonic_Control    *the_period
)
{
  Timestamp_Control          executed;
  Timestamp_Control          since_last_period;
  Rate_monotonic_Statistics *stats;
  bool                       valid_status;

  /*
   *  Assume we are only called in states where it is appropriate
   *  to update the statistics.  This should only be RATE_MONOTONIC_ACTIVE
   *  and RATE_MONOTONIC_EXPIRED.
   */

  /*
   *  Update the counts.
   */
  stats = &the_period->Statistics;
  stats->count++;

  if ( the_period->state == RATE_MONOTONIC_EXPIRED )
    stats->missed_count++;

  /*
   *  Grab status for time statistics.
   */
  valid_status =
    _Rate_monotonic_Get_status( the_period, &since_last_period, &executed );
  if (!valid_status)
    return;

  /*
   *  Update CPU time
   */
  _Timestamp_Add_to( &stats->total_cpu_time, &executed );

  if ( _Timestamp_Less_than( &executed, &stats->min_cpu_time ) )
    stats->min_cpu_time = executed;

  if ( _Timestamp_Greater_than( &executed, &stats->max_cpu_time ) )
    stats->max_cpu_time = executed;

  /*
   *  Update Wall time
   */
  _Timestamp_Add_to( &stats->total_wall_time, &since_last_period );

  if ( _Timestamp_Less_than( &since_last_period, &stats->min_wall_time ) )
    stats->min_wall_time = since_last_period;

  if ( _Timestamp_Greater_than( &since_last_period, &stats->max_wall_time ) )
    stats->max_wall_time = since_last_period;
}

static rtems_status_code _Rate_monotonic_Get_status_for_state(
  rtems_rate_monotonic_period_states state
)
{
  switch ( state ) {
    case RATE_MONOTONIC_INACTIVE:
      return RTEMS_NOT_DEFINED;
    case RATE_MONOTONIC_EXPIRED:
      return RTEMS_TIMEOUT;
    default:
      _Assert( state == RATE_MONOTONIC_ACTIVE );
      return RTEMS_SUCCESSFUL;
  }
}

static rtems_status_code _Rate_monotonic_Activate(
  Rate_monotonic_Control *the_period,
  rtems_interval          length,
  Thread_Control         *executing,
  ISR_lock_Context       *lock_context
)
{
  the_period->postponed_jobs = 0;
  the_period->state = RATE_MONOTONIC_ACTIVE;
  the_period->next_length = length;
  _Rate_monotonic_Restart( the_period, executing, lock_context );
  return RTEMS_SUCCESSFUL;
}

static rtems_status_code _Rate_monotonic_Block_while_active(
  Rate_monotonic_Control *the_period,
  rtems_interval          length,
  Thread_Control         *executing,
  ISR_lock_Context       *lock_context
)
{
  Per_CPU_Control *cpu_self;
  bool             success;

  /*
   *  Update statistics from the concluding period.
   */
  _Rate_monotonic_Update_statistics( the_period );

  /*
   *  This tells the _Rate_monotonic_Timeout that this task is
   *  in the process of blocking on the period and that we
   *  may be changing the length of the next period.
   */
  the_period->next_length = length;
  executing->Wait.return_argument = the_period;
  _Thread_Wait_flags_set( executing, RATE_MONOTONIC_INTEND_TO_BLOCK );

  cpu_self = _Thread_Dispatch_disable_critical( lock_context );
  _Rate_monotonic_Release( the_period, lock_context );

  _Thread_Set_state( executing, STATES_WAITING_FOR_PERIOD );

  success = _Thread_Wait_flags_try_change_acquire(
    executing,
    RATE_MONOTONIC_INTEND_TO_BLOCK,
    RATE_MONOTONIC_BLOCKED
  );
  if ( !success ) {
    _Assert(
      _Thread_Wait_flags_get( executing ) == RATE_MONOTONIC_READY_AGAIN
    );
    _Thread_Unblock( executing );
  }

  _Thread_Dispatch_enable( cpu_self );
  return RTEMS_SUCCESSFUL;
}

/*
 * There are two possible cases: one is that the previous deadline is missed,
 * The other is that the number of postponed jobs is not 0, but the current
 * deadline is still not expired, i.e., state = RATE_MONOTONIC_ACTIVE.
 */
static rtems_status_code _Rate_monotonic_Block_while_expired(
  Rate_monotonic_Control *the_period,
  rtems_interval          length,
  Thread_Control         *executing,
  ISR_lock_Context       *lock_context
)
{
  /*
   * No matter the just finished jobs in time or not,
   * they are actually missing their deadlines already.
   */
  the_period->state = RATE_MONOTONIC_EXPIRED;

  /*
   * Update statistics from the concluding period
   */
  _Rate_monotonic_Update_statistics( the_period );

  the_period->state = RATE_MONOTONIC_ACTIVE;
  the_period->next_length = length;

  _Rate_monotonic_Release_postponed_job(
      the_period,
      executing,
      length,
      lock_context
  );
  return RTEMS_TIMEOUT;
}

/* This is the function that is called by the user when starting a period */
/* It takes the period ID and the length of the time interval */
rtems_status_code rtems_rate_monotonic_period(
  rtems_id       id,
  rtems_interval length
)
{
  /* Control block to manage periods, struct in ratemon.h */
  Rate_monotonic_Control            *the_period;
  /* A lock */
  ISR_lock_Context                   lock_context;
  /* A variable for a TCB */
  Thread_Control                    *executing;
  /* Status codes */
  rtems_status_code                  status;
  rtems_rate_monotonic_period_states state;

  the_period = _Rate_monotonic_Get( id, &lock_context );
  if ( the_period == NULL ) {
    return RTEMS_INVALID_ID;
  }

  /* check if period is registered in the FTS */
  int16_t i = task_in_list_t(the_period->Object.id);
  if (i != -1)
  {
    printf("\n<<<<delete all tasks from last period>>>>\n");
    /*at beginning of new period, delete all tasks from last period if their ID is not 0 */
    printf("\nrunning_id_b: %i\n",running_id_b[i]  );
    printf("\nrunning_id_d: %i\n",running_id_d[i]  );
    printf("\nrunning_id_c: %i\n",running_id_c[i]  );
    /**
     * If the ID is 0, which happens when the variables running_id_b/d/c are not set,
     * then don't delete the task, otherwise the task with ID==0 will be deleted,
     * which is possibly the first task in the system.
     *
     * Delete all tasks at the beginning of a new period.
     */
    if (running_id_b[i] != 0)
    {
      task_status( rtems_task_delete( running_id_b[i] ) );
      running_id_b[i] = 0;
    }

    if (running_id_d[i] != 0)
    {
      task_status( rtems_task_delete( running_id_d[i] ) );
      running_id_d[i] = 0;
    }

    if (running_id_c[i] != 0)
    {
      task_status( rtems_task_delete( running_id_c[i] ) );
      running_id_c[i] = 0;
    }
  }

  /* If dynamic compensation is used, replenish tolerance counters when they are depleted */
  if( (i != -1) && ( (list.tech[i] == DRE) || (list.tech[i] == DDR) )  )
  {
    if ( ((tol_counter_temp[i]).tol_counter_a[partition_index[i]]) == 0)
    {
      /* replenish only if at end of last partition (a is 0) */
      if ( (nr_partitions[i] == partition_index[i]) && (tol_counter_temp[i].tol_counter_a[nr_partitions[i]] == 0) )
      {
        tolc_update(i);
        printf("\nTOLERANCE COUNTER was at end for index %i, UPDATED\n", i);
      }
      else
      {
        /* was not in last partition */
        printf("\nNEXT PARTITION\n");
        partition_index[i]++;
      }
    }
  }

  /*task which executes needs to be the owner which was set in create earlier*/

  executing = _Thread_Executing;
  if ( executing != the_period->owner ) {
    _ISR_lock_ISR_enable( &lock_context );
    return RTEMS_NOT_OWNER_OF_RESOURCE;
  }
  /* Begin critical sequence with locking period control block */
  _Rate_monotonic_Acquire_critical( the_period, &lock_context );

  /* save current state of period */
  state = the_period->state;

  /* if length of period is equal to (WATCHDOG_NO_TIMEOUT?) exceeded length ?... */
  if ( length == RTEMS_PERIOD_STATUS ) {
    /* if true, immediately return status and end function */
    status = _Rate_monotonic_Get_status_for_state( state );
    /* release lock from Perdioc CB */
    _Rate_monotonic_Release( the_period, &lock_context );
  } else {
    /* first check period state */
    switch ( state ) {
      //if period is active
      case RATE_MONOTONIC_ACTIVE:

        if( the_period->postponed_jobs > 0 ){
          /*
           * If the number of postponed jobs is not 0, it means the
           * previous postponed instance is finished without exceeding
           * the current period deadline.
           *
           * Do nothing on the watchdog deadline assignment but release the
           * next remaining postponed job.
           */
           //block period while expired
          status = _Rate_monotonic_Block_while_expired(
            the_period,
            length,
            executing,
            &lock_context
          );
          /* if there are no postponed jobs */
        }else{
          /*
           * Normal case that no postponed jobs and no expiration, so wait for
           * the period and update the deadline of watchdog accordingly.
           * block while active
           */
          status = _Rate_monotonic_Block_while_active(
            the_period,
            length,
            executing,
            &lock_context
          );
        }
        break;
        //if inactive, activate immediately
      case RATE_MONOTONIC_INACTIVE:
        status = _Rate_monotonic_Activate(
          the_period,
          length,
          executing,
          &lock_context
        );
        /* FTS RELEASE TASKS */
        /* Ask fts scheduler to start a task ? */
        /* do I put in ready queue by "hand" ? or use create and start task ?*/
        break;
      default:
        /*
         * As now this period was already TIMEOUT, there must be at least one
         * postponed job recorded by the watchdog. The one which exceeded
         * the previous deadlines was just finished.
         *
         * Maybe there is more than one job postponed due to the preemption or
         * the previous finished job.
         * EXPIRED
         */
        _Assert( state == RATE_MONOTONIC_EXPIRED );
        status = _Rate_monotonic_Block_while_expired(
          the_period,
          length,
          executing,
          &lock_context
        );
        break;
    }
  }

  return status;
}
