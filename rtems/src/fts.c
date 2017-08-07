#include <rtems/rtems/fts.h>

// can protect up to 255 tasks
#define P_TASKS 10
// data structure
// list of task IDs

struct Task_ID_List {
  rtems_id task_list_id[P_TASKS];
  m_k task_list_mk[P_TASKS];
  fts_tech task_list_tech[P_TASKS];
  bitstring_pattern bmap[P_TASKS];
  int task_list_index;
} list;

// typedef struct task_info {
//   unsigned int executions;
//   unsigned int detects;
//   unsigned int corrects;
// };


// if task is in list already, return 1, -1 else
int task_in_list(
  rtems_id id
)
{
  for(int i = 0; i < list.task_list_index; i++)
  {
    if (list.task_list_id[i] == id)
    {
      return i;
    }
  }
  return -1;
}

// sets the sre execution pattern for a task
int set_sre_pattern(
  rtems_id id,
  bitstring_pattern bmap
)
{
  int sre_index = task_in_list(id);
  if( (sre_index != -1) && list.task_list_tech[sre_index] == SRE )
  {
    list.bmap[sre_index] = bmap;
    return 1;
  }
  return 0;
}

fts_version sre_next_version(
  int i
)
{
    uint8_t bitpos = list.bmap[i].bitpos;
    uint8_t bit_mask_one = 1 << bitpos;
    uint8_t result_bit = (*(list.bmap[i].curr_pos)) & bit_mask_one;

    if (result_bit == 0)
    {
      return BASIC;
    }
    return RECOVERY;
}



/***/

uint8_t fts_rtems_task_register(
  rtems_id id,
  m_k mk,
  fts_tech tech
)
{
  if((list.task_list_index < P_TASKS) && (task_list_index(id) == -1) && (mk.m <= mk.k))
  {
    list.task_list_id[list.task_list_index] = id;
    list.task_list_mk[list.task_list_index] = mk;
    list.task_list_tech[list.task_list_index] = tech;
    list.task_list_index++;
    return 1;
  }
  return 0;
}

/***/

uint8_t fts_init_versions(void)
{
  return 1;
}

/***/

fts_version fts_get_mode(
  rtems_id id
)
{
  int task_index = task_in_list(id);
  fts_version next_version;
  if (task_index != -1)
  {
    switch(list.task_list_tech[id])
    {
      case NONE :
        next_version = BASIC;
        break;

      case SRE :
        next_version = RECOVERY;
        break;

      case SDR :
        next_version = RECOVERY;
        break;

      case DRE :
        next_version = RECOVERY;
        break;

      case DDR :
        next_version = RECOVERY;
        break;
    }
  }
  return next_version;
}

/***/

uint8_t fts_off(
  rtems_id id
)
{
  return 1;
}

/***/

uint8_t fts_task_status(
  rtems_id id
)
{
  return 1;
}

/***/

uint8_t fts_change_tech(
  rtems_id id,
  fts_tech tech
)
{

  int list_index = task_in_list(id);
  if(list_index != -1)
  {
    list.task_list_tech[list_index] = tech;
    return 1;
  }
return 0;
}


/***/
