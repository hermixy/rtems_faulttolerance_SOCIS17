#include <rtems/rtems/fts_t.h>
// data structure
// list of task IDs

// void release_task(int list_index, int task_i ) // list_index: in list object, task_i: versions
// {
//   task_status( rtems_task_create(Task_name[task_i], Prio[task_i], RTEMS_MINIMUM_STACK_SIZE,
//   RTEMS_DEFAULT_MODES,RTEMS_DEFAULT_ATTRIBUTES, &Task_id[ task_i ]) );
//
//   task_status( rtems_task_start( Task_id[task_i], list.c[list_index], period_pointers[list_index]) );
//   return;
// }
void partitions_print(int i)
{
    printf("\nfts_t.c (partition print):\n");
    //print tolerance counters
    uint8_t m = 0;
    uint16_t out;
    printf("  o:");
    for(; m <= nr_part[i]; m++ )
    {
      out = tol_e[i].o_e[m];
      printf("%i ", out );
    }
    printf("\n\n  a:");
    m = 0;
    for(; m <= nr_part[i]; m++ )
    {
      out = tol_e[i].a_e[m];
      printf("%i ", out);
    }
    return;
}

void tol_counter_set(int i, uint8_t *start, uint8_t *end, uint8_t maxbit)
{
  uint8_t *it = start;
  uint8_t last_bit = 0;
  uint8_t partition = 0;

  for ( ;it <= end; it++ )
  {
    uint8_t bmask = BIT_7;

    for ( uint8_t j = 0; j <= 7; j++ )
    {
      if ( (j > maxbit) && (it == end) )
      {
        break;
      }

      if ( ((*it) & bmask) == 0 ) // count "0"s
      {
        if(last_bit == 1)
        {
          partition++;
        }
        tol_e[i].o_e[partition]++;
        last_bit = 0;
      }
      else // count the "1"s
      {
        tol_e[i].a_e[partition]++;
        last_bit = 1;
      }
      bmask >>= 1;
    }
  }
  nr_part[i] = partition;
  printf("\nfts_t.c (tol counter e): Nr. Partitions: %i\n", nr_part[i]);
  partitions_print(i);
  return;
}

// /* sets the tolerance counters */
// void tol_counter_set_R(int i, uint8_t *start, uint8_t *end, uint8_t maxbit )
// {
//   uint8_t *it = start;
//   for ( ;it <= end; it++ )
//   {
//     uint8_t bmask = BIT_7;
//
//     for( uint8_t j = 0; j <= 7; j++ )
//     {
//       if ( (j > maxbit) && (it == end) )
//       {
//         break;
//       }
//
//       if( ( (*it) & bmask) == 0 ) // count "0"s
//       {
//         o[i]++;
//       }
//       else // count the "1"s
//       {
//         a[i]++;
//       }
//       bmask >>= 1;
//     }
//
//   }
//   return;
// }

// void tolc_update_R(int i)
// {
//   o_tolc[i] = o[i];
//   a_tolc[i] =  a[i];
//   return;
// }

void tolc_update(int i)
{
  for(int j = 0; j <= nr_part[i]; j++)
  {
    tol_count_e[i].o_e[j] = tol_e[i].o_e[j];
    tol_count_e[i].a_e[j] = tol_e[i].a_e[j];
  }
  part_index[i] = 0;
  return;
}

void task_status(rtems_status_code s)
{
  switch(s)
  {
    case RTEMS_SUCCESSFUL  :
    printf("\nRTEMS_SUCCESSFUL\n");
    break;

    case RTEMS_INVALID_ADDRESS  :
    printf("\nRTEMS_INVALID_ADDRESS\n");
    break;

    case RTEMS_INVALID_ID  :
    printf("\nRTEMS_INVALID_ID\n");
    break;

    case RTEMS_INCORRECT_STATE  :
    printf("\nRTEMS_INCORRECT_STATE\n");
    break;

    case RTEMS_ILLEGAL_ON_REMOTE_OBJECT  :
    printf("\nRTEMS_ILLEGAL_ON_REMOTE_OBJECT\n");
    break;
  }
  return 0;
}

/* Check if a certain task is in the list */
int16_t task_in_list_t(
  rtems_id id
)
{
  for (int i = 0; i < list.task_list_index; i++)
  {
    if (list.task_list_id[i] == id)
    {
      return i;
    }
  }
  return -1;
}

/* call when error detected */
void fault_detection_routine(rtems_id id, fault_status fs)
{
  uint16_t i = task_in_list_t(id);
  if(fs == FAULT)
  {
  printf("\nRECOVERING FROM FAULT: i is %i\n", i );

  switch (list.task_list_tech[i])
  {
    case SDR:
        task_status( rtems_task_create(Task_name[ 3 ], Prio[3], RTEMS_MINIMUM_STACK_SIZE,
        RTEMS_DEFAULT_MODES,RTEMS_DEFAULT_ATTRIBUTES, &Task_id[ 3 ]) );

        running_id_r[i] = Task_id[ 3 ];
        printf("\nfts.c (detect_routine) ID next task (reliable): %i\n", running_id_r[i] );

        task_status( rtems_task_start( Task_id[ 3 ], list.c[i], period_pointers[i]) );
    break;

    case DRE:
      if (tol_count_e[i].o_e[part_index[i]] > 0)
      {
        // fault detected, so decrease tol counter
        tol_count_e[i].o_e[part_index[i]]--;
        printf("\n DETECT ROUTINE DRE: Tolerance Counter: %i\n", tol_count_e[i].o_e[part_index[i]]);
      }
      else // if tol counter is 0, release rel version
      {
        printf("\n DETECT ROUTINE DRE: Tolerance Counter: %i\n", tol_count_e[i].o_e[part_index[i]]);
        task_status( rtems_task_create(Task_name[ 3 ], Prio[3], RTEMS_MINIMUM_STACK_SIZE,
        RTEMS_DEFAULT_MODES,RTEMS_DEFAULT_ATTRIBUTES, &Task_id[ 3 ]) );

        running_id_r[i] = Task_id[ 3 ];
        printf("\nfts.c (detect_routine) ID next task (reliable): %i\n", running_id_r[i] );

        task_status( rtems_task_start( Task_id[ 3 ], list.c[i], period_pointers[i]) );
      }
    break;

    case DDR:
      if (tol_count_e[i].o_e[part_index[i]] > 0)
      {
        // fault detected, so decrease tol counter
        tol_count_e[i].o_e[part_index[i]]--;
        printf("\n DETECT ROUTINE DDR: Tolerance Counter: %i\n", tol_count_e[i].o_e[part_index[i]]);
      }
      else // already gave a try
      {
          task_status( rtems_task_create(Task_name[ 3 ], Prio[3], RTEMS_MINIMUM_STACK_SIZE,
          RTEMS_DEFAULT_MODES,RTEMS_DEFAULT_ATTRIBUTES, &Task_id[ 3 ]) );

          running_id_r[i] = Task_id[ 3 ];
          printf("\nfts.c (detect_routine) ID next task (reliable): %i\n", running_id_r[i] );

          task_status( rtems_task_start( Task_id[ 3 ], list.c[i], period_pointers[3]) );

          printf("CONCLUDED DDR RECOVERY");
      }
    break;

  }

  }
  // printf("\nWILL RESUME\n");
  // rtems_status_code status = rtems_task_resume(main_id[i]);
  // task_status(status);
  return;
}


/* Show the execution pattern in the console */
uint8_t show_pattern_t(
  uint8_t *p_curr,
  uint8_t *p_end,
  uint8_t maxbit
)
{
  printf("\n\nfts_h.c: The pattern is:\n");

  for (; p_curr <= p_end; p_curr++)
  {
    uint8_t b_mask = BIT_7;

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
  printf("\n\n");
  /* End output pattern */
  return 0;
}


/* Create and set the (m,k) pattern to the given memory adress */
int8_t create_pattern_t(
  int i,
  pattern_type pattern
)
{
  printf("\nfts_t.c (create_pattern):i is %i, m is %i, k is %i\n", i, list.m[i], list.k[i]);

  uint8_t *pattern_it;
  pattern_it = list.pattern_start[i];
  /*
  * for details on E and R pattern, check
  * http://ieeexplore.ieee.org/document/1661621/
  */

  if ( pattern == R_PATTERN )
  {
    int8_t k_minus_m = list.k[i]-list.m[i];
    //number of bytes are needed to insert "0"s
    uint zero_bytes = (k_minus_m)/8 + (k_minus_m % 8 != 0);
    /* insert "0"s */
    while( zero_bytes >= 1 )
    {
      *pattern_it = 0;
      if (zero_bytes > 1)
      {
        pattern_it++;
      }
      zero_bytes--;
    }

    uint8_t shift_r = (k_minus_m % 8);
    uint8_t ones_8 = 255;
    uint8_t sh_8 = ones_8 >> shift_r;
    *pattern_it |= sh_8;
    pattern_it++;

    /* insert remaining "1"s */
    for (;pattern_it <= list.pattern_end[i]; pattern_it++)
    {
      *pattern_it  = ones_8;
    }
  }

  /* evenly distributed "1"s
   * for details on E and R pattern, check
   * http://ieeexplore.ieee.org/document/1661621/
   */
   if ( pattern == E_PATTERN )
   {
    uint8_t set_byte = 0;
    //for ( int j = 0; j < list.k[i]; j++ )
    //{
    int j = 0;
    for (;(pattern_it <= list.pattern_end[i]); pattern_it++)
    {
      uint8_t bitmask = BIT_7;
      for (uint8_t it_bits = 0; (it_bits < 8) ; it_bits++)
      {
        int raman = ( (((j*(list.k[i]-list.m[i]))/list.k[i]) + ((j*(list.k[i]-list.m[i]))%list.k[i] != 0) )* list.k[i]/(list.k[i]-list.m[i]) );

        // for 'normal' e-pattern:
        //int raman = ( (((j*list.m[i])/list.k[i]) + ((j*list.m[i])%list.k[i] != 0) )* list.k[i]/list.m[i] );

        if (j != raman)
        {
          // insert 1;
          set_byte |= bitmask;
        }
        bitmask >>= 1;
        j++;
      }
      *pattern_it = set_byte;
    }
    //}
  }

  printf("\nfts_t.c (create_pattern): Address of first byte: %p\n", (void *)list.pattern_start[i]);
  printf("\nfts_t.c (create_pattern): Address of current byte: %p\n", (void *)list.curr_pos[i]);
  printf("\nfts_t.c (create_pattern): Address of last byte: %p\n", (void *)list.pattern_end[i]);

  printf("\nfts_t.c (create_pattern): ");
  uint8_t sh = show_pattern_t(list.pattern_start[i], list.pattern_end[i], list.max_bitpos[i]);
  return 1;
}

static fts_version static_next_version_t(
  int i
)
{
    printf("\nfts.c (static next version): Address of first byte: %p\n", (void *)list.pattern_start[i]);
    printf("\nfts.c (static next version): Address of current byte: %p\n", (void *)list.curr_pos[i]);
    printf("\nfts.c (static next version): Address of last byte: %p\n", (void *)list.pattern_end[i]);

    printf("\nfts.c (static_next_version):  index is %i\n", i);
    printf("\nIn static_next_version:");

    show_pattern_t(list.pattern_start[i], list.pattern_end[i], list.max_bitpos[i]);

    uint8_t bitpos = list.bitpos[i]; // bit to be read
    uint8_t c_byte = *(list.curr_pos[i]); // byte to be read
    uint8_t bit_mask_one = BIT_7 >> bitpos;

    uint8_t result_bit = c_byte & bit_mask_one;

    //if in last byte
    if (list.pattern_end[i] == list.curr_pos[i])
    {
      if (list.bitpos[i] < list.max_bitpos[i])
      {
        list.bitpos[i]++;
      }
      else
      {
        list.bitpos[i] = 0;
        list.curr_pos[i] = list.pattern_start[i];
      }
    }
    else
    {
      if ( list.bitpos[i] < 7 )
      {
        list.bitpos[i]++;
      }
      else
      {
        list.bitpos[i] = 0;
        list.curr_pos[i]++;
      }
    }

    fts_tech tech = list.task_list_tech[i];

    rtems_status_code status;
    if (tech == SRE)
    {
      if (result_bit == 0)
      {
        printf("\nfts_t.c (static_next_version):SRE BASIC, BIT: %i, BITPOS: %i\n", result_bit, list.bitpos[i]);
        status = rtems_task_create(Task_name[ 1 ], Prio[1], RTEMS_MINIMUM_STACK_SIZE,
          RTEMS_DEFAULT_MODES,RTEMS_DEFAULT_ATTRIBUTES, &Task_id[ 1 ]);
        task_status(status);

        /* store task id in variable */
        running_id_b[i] = Task_id[ 1 ];
        printf("\nfts.c (static_next_version) ID next task (basic): %i\n", running_id_b[i] );

        status = rtems_task_start( Task_id[ 1 ], list.b[i], period_pointers[i]);
        task_status(status);

        return BASIC;
      }
      printf("\nfts_t.c (static_next_version):SRE RECOVERY, BIT: %i, BITPOS: %i\n", result_bit, list.bitpos[i]);
       status = rtems_task_create(Task_name[ 3 ], Prio[3], RTEMS_MINIMUM_STACK_SIZE,
         RTEMS_DEFAULT_MODES,RTEMS_DEFAULT_ATTRIBUTES, &Task_id[ 3 ]);
       task_status(status);

       running_id_r[i] = Task_id[ 3 ];
       printf("\nfts.c (static_next_version) ID next task (recovery): %i\n", running_id_r[i] );

      status = rtems_task_start( Task_id[ 3 ], list.c[i], period_pointers[i]);
      task_status(status);

      return RECOVERY;
    }
    else //SDR
    {
      if (result_bit == 0)
      {
        printf("\nfts_t.c (static_next_version):SDR BASIC, BIT: %i, BITPOS: %i\n", result_bit, list.bitpos[i]);

        task_status( rtems_task_create(Task_name[ 1 ], Prio[1], RTEMS_MINIMUM_STACK_SIZE,
          RTEMS_DEFAULT_MODES,RTEMS_DEFAULT_ATTRIBUTES, &Task_id[ 1 ]) );

        running_id_b[i] = Task_id[ 1 ];
        printf("\nfts.c (static_next_version) ID next task (basic): %i\n", running_id_b[i] );

        task_status( rtems_task_start( Task_id[ 1 ], list.b[i], period_pointers[i]) );
        return BASIC;
      }
      else // bit is 1
      {
        printf("\nfts_t.c (static_next_version):SDR DETECTION, BIT: %i, BITPOS: %i\n", result_bit, list.bitpos[i]);
        //start a detection version
        task_status( rtems_task_create(Task_name[ 2 ], Prio[2], RTEMS_MINIMUM_STACK_SIZE,
          RTEMS_DEFAULT_MODES,RTEMS_DEFAULT_ATTRIBUTES, &Task_id[ 2 ]) );

        running_id_d[i] = Task_id[ 2 ];
        printf("\nfts.c (static_next_version) ID next task (detection): %i\n", running_id_d[i] );

        task_status(rtems_task_start( Task_id[ 2 ], list.d[i], period_pointers[i]));
        // if fault, create and start rel. version
        // argument is id of period

        // wait for new task to conclude ?
        return DETECTION;
      }
    //(list.pattern[i]->bitpos < list.pattern[i].max_bitpos)
    }
}
/***/

fts_version dynamic_next_version_t(
  int i
)
{
      printf("\nfts.c (dynamic_next_version):  index is %i\n", i);
      printf("\nIn dynamic_next_version:");

      show_pattern_t(list.pattern_start[i], list.pattern_end[i], list.max_bitpos[i]);

      //DRE or DDR, E or R pattern
      fts_tech tech = list.task_list_tech[i];
      pattern_type pattern = list.pattern[i];

      rtems_status_code status;

      if (tol_count_e[i].o_e[part_index[i]] > 0)
      {
        printf("\nfts.c (dynamic_next_version): tolerance counter: %i\n", tol_count_e[i].o_e[part_index[i]]);
        task_status( rtems_task_create(Task_name[ 2 ], Prio[2], RTEMS_MINIMUM_STACK_SIZE,
          RTEMS_DEFAULT_MODES,RTEMS_DEFAULT_ATTRIBUTES, &Task_id[ 2 ]) );

        running_id_d[i] = Task_id[ 2 ];
        printf("\nfts.c (dynamic_next_version) ID next task (detection): %i\n", running_id_d[i] );

        task_status(rtems_task_start( Task_id[ 2 ], list.d[i], period_pointers[i]));

        return DETECTION;
        // when fault, decrement o_tolc
      }
      else // tolerance counter depleted, only execute RE a times (or DET again)
      {
        if(tech == DRE)
        {
          printf("\nfts.c (dynamic_next_version): tolerance counter: %i\n", tol_count_e[i].o_e[part_index[i]]);
          task_status( rtems_task_create(Task_name[ 3 ], Prio[3], RTEMS_MINIMUM_STACK_SIZE,
            RTEMS_DEFAULT_MODES,RTEMS_DEFAULT_ATTRIBUTES, &Task_id[ 3 ]) );

          running_id_r[i] = Task_id[ 3 ];
          printf("\nfts.c (dynamic_next_version) ID next task (recovery): %i\n", running_id_r[i] );

          task_status(rtems_task_start( Task_id[ 3 ], list.c[i], period_pointers[i]));
          tol_count_e[i].a_e[part_index[i]]--;

          return RECOVERY;
        }
        else // DDR
        {
          //although tolerance counter depleted, still give  try
          task_status( rtems_task_create(Task_name[ 2 ], Prio[2], RTEMS_MINIMUM_STACK_SIZE,
            RTEMS_DEFAULT_MODES,RTEMS_DEFAULT_ATTRIBUTES, &Task_id[ 2 ]) );

          running_id_d[i] = Task_id[ 2 ];
          printf("\nfts.c (dynamic_next_version) ID next task (detection): %i\n", running_id_d[i] );

          task_status( rtems_task_start( Task_id[ 2 ], list.d[i], period_pointers[i]) );
          // printf("\nSTARTED\n");
          // //wait for detection to end
          // // while ( flag[i] == 0 )
          // // {
          // //   printf("\nwait\n");
          // // }
          // printf("\nWILL SUSPEND MYSELF\n");
          // task_status( rtems_task_suspend( RTEMS_SELF ) );
          // printf("\nGO ON\n");
          tol_count_e[i].a_e[part_index[i]]--;
          return DETECTION;
        }
      }
  //check if tolerance counter depleted
  //if not depleted, execute detection version
  //
}

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
)
{
  uint16_t i = list.task_list_index;
  if ((i < P_TASKS) && (task_in_list_t((*id)) == -1) && (m <= k) )
  {
    /* Put all information in the tasklist */
    // store ID
    list.task_list_id[i] = *id;
    //output
    printf("\nfts_t.c (register): ID %i\n", list.task_list_id[i]);
    // store (m,k)
    list.m[i] = m;
    list.k[i] = k;
    uint8_t m_m = list.m[i];
    uint8_t k_k = list.k[i];

    /* Output values for (m,k) */
    printf("\nfts_t.c: m = %i\n", m_m);
    printf("\nfts_t.c: k = %i\n", k_k);

    // store technique
    list.task_list_tech[i] = tech;

    // set pattern type, E- or R- pattern
    list.pattern[i] = pattern;

    list.pattern_start[i] = pattern_start;
    list.pattern_end[i] = pattern_end;
    list.curr_pos[i] = pattern_start;
    list.max_bitpos[i] = max_bitpos;
    list.bitpos[i] = 0;

    uint8_t pa = create_pattern_t(i, pattern);
    printf("\nfts_t.c (register): Tech %i\n", list.task_list_tech[i]);

    /*set tolerance counters if dynamic */
    if ( (list.task_list_tech[i] == DRE) || (list.task_list_tech[i] == DDR) )
    {
      tol_counter_set(i, pattern_start, pattern_end, max_bitpos);
      tolc_update(i);
    }

    list.b[i] = basic;
    list.d[i] = detection;
    list.c[i] = recovery;

    //initialize flag for DDR detection
    period_pointers[i] = id;

    list.task_list_index++;

    return 1;
  }
  return 0;
}

/***/

uint8_t fts_init_versions_t(void)
{
  return 1;
}

/***/

fts_version fts_compensate_t(
  rtems_id id
)
{
  printf("\nfts_t.c (comp) ID: %i\n", id);
  int16_t task_index = task_in_list_t(id);

  printf("\nfts_t.c (comp): list index %i\n", task_index);

  ok = 1;

  fts_version next_version;
  if (task_index != -1)
  {
    switch(list.task_list_tech[task_index])
    {
      case NONE :
        next_version = BASIC;
        //printf("\nfts_t.c (comp): NONE\n");
        break;

      case SRE :
        next_version = static_next_version_t(task_index);
        //printf("\nfts_t.c (comp): SRE\n");
        break;

      case SDR :
        next_version = static_next_version_t(task_index);
        //printf("\nfts_t.c (comp): SDR\n");
        break;

      case DRE :
        next_version = dynamic_next_version_t(task_index);
        //printf("\nfts_t.c (comp): DRE\n");
        break;

      case DDR :
        next_version =  dynamic_next_version_t(task_index);
        //printf("\nfts_t.c (comp): DDR\n");
    }
  }
  return next_version;
}



/* removing a task from the tasklist might be dangerous*/
/* TODO: might be wrong */
uint8_t fts_off_t(
  rtems_id id
)
{
  int16_t list_index = task_in_list_t(id);
  if(list_index != -1)
  {
    for (int16_t i = 0; i < list.task_list_index; i++)
    {
      list.task_list_id[i] = list.task_list_id[i+1];
      list.m[i] = list.m[i+1];
      list.k[i] = list.k[i+1];
      list.task_list_tech[i] = list.task_list_tech[i+1];

      list.pattern_start[i] = list.pattern_start[i+1];
      list.pattern_end[i] = list.pattern_end[i+1];
      list.curr_pos[i] = list.curr_pos[i+1];
      list.bitpos[i] = list.bitpos[i+1];
      list.max_bitpos[i] = list.max_bitpos[i+1];
    }
    list.task_list_index--;
    return 1;
  }
  else
  {
    return -1;
  }
}

/***/

uint8_t fts_task_status_t(
  rtems_id id
)
{
  return 1;
}

/***/

uint8_t fts_change_tech_t(
  rtems_id id,
  fts_tech tech
)
{
  int16_t list_index = task_in_list_t(id);
  if(list_index != -1)
  {
    list.task_list_tech[list_index] = tech;
    return 1;
  }
return 0;
}


/***/
