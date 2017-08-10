#include <rtems/rtems/fts.h>

// can protect up to P_TASK tasks
#define P_TASKS 10
// data structure
// list of task IDs

struct Task_ID_List {
  rtems_id task_list_id[P_TASKS];
  m_k task_list_mk[P_TASKS];
  fts_tech task_list_tech[P_TASKS];
  bitstring_pattern pattern[P_TASKS];
  uint16_t task_list_index; //always is one position ahead of last filled
} list;

// typedef struct task_info {
//   unsigned int executions;
//   unsigned int detects;
//   unsigned int corrects;
// };


// if task is in list already, return 1, -1 else
int16_t task_in_list(
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

uint8_t show_pattern(uint8_t *p_curr, uint8_t *p_end, uint8_t maxbit)
{
  /* Start output pattern */
  printf("\n\nfts.c: The pattern is: ");

  for (; p_curr <= p_end; p_curr++)
  {
    uint8_t b_mask = 1;
    uint8_t c_byte = *p_curr;

    for (uint8_t i = 0; i < 8; i++)
    {
        uint8_t output = b_mask | c_byte;

        if (output == 0)
        {
          printf("0");
        }
        else
        {
          printf("1");
        }
        b_mask <<= 1;
    }
  }
  printf("\n\n");
  /* End output pattern */

  return 0;
}

// sets the sre execution pattern for a task
int8_t fts_set_sre_pattern(
  rtems_id id,
  bitstring_pattern pattern
)
{
  int sre_index = task_in_list(id);
  if( (sre_index != -1) && list.task_list_tech[sre_index] == SRE )
  {
    list.pattern[sre_index] = pattern;

    show_pattern(pattern.pattern_start, pattern.pattern_end, 7);

    uint8_t *p_curr = list.pattern[sre_index].pattern_start;
    uint8_t *p_end = list.pattern[sre_index].pattern_end;

    show_pattern(p_curr, p_end, 7);

    return 1;
  }
  return -1;
}

////

static fts_version sre_next_version(
  int i
)
{
    uint8_t bitpos = list.pattern[i].bitpos;
    uint8_t bit_mask_one = 1 << bitpos;
    uint8_t result_bit = (*(list.pattern[i].curr_pos)) & bit_mask_one;

    if (list.pattern[i].bitpos < 7)
    {
      list.pattern[i].bitpos++;
    }
    else
    {
      if (list.pattern[i].pattern_end == list.pattern[i].curr_pos)
      {
        list.pattern[i].bitpos = 0;
        list.pattern[i].curr_pos = list.pattern[i].pattern_start;
      }
      else
      {
      list.pattern[i].curr_pos++;
      list.pattern[i].bitpos = 0;
      }
    }

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
  if ((list.task_list_index < P_TASKS) && (task_in_list(id) == -1) && (mk.m <= mk.k))
  {
    // remove mk struct
    /* Test (m,k) output */
    m_k bmk = mk;
    uint8_t test_m = bmk.m;
    printf("\nfts.c: m = %i\n", test_m);

    /* Put all information in the tasklist */
    list.task_list_id[list.task_list_index] = id;
    list.task_list_mk[list.task_list_index] = mk;
    list.task_list_tech[list.task_list_index] = tech;
    printf("\nfts.c: list index is: %i\n",  list.task_list_index);
    list.task_list_index++;
    printf("\nfts.c: list index is: %i\n",  list.task_list_index);
    /* Generate (m,k)-pattern, then put in list */

    /* Output values for (m,k) */
    m_k currmk = list.task_list_mk[list.task_list_index];
    uint8_t m_m = currmk.m;
    uint8_t k_k = currmk.k;

    printf("\nfts.c: m = %i\n", m_m);

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
  int16_t task_index = task_in_list(id);
  fts_version next_version;
  if (task_index != -1)
  {
    switch(list.task_list_tech[task_index])
    {
      case NONE :
        next_version = BASIC;
        break;

      case SRE :
        next_version = sre_next_version(task_index);
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
  int16_t list_index = task_in_list(id);
  if(list_index != -1)
  {
    for (int16_t i = 0; i < list.task_list_index; i++)
    {
      list.task_list_id[i] = list.task_list_id[i+1];
      list.task_list_mk[i] = list.task_list_mk[i+1];
      list.task_list_tech[i] = list.task_list_tech[i+1];
      list.pattern[i] = list.pattern[i+1];
      //list.task_list_index--;
    }
    return 1;
  }
  else
  {
    return -1;
  }
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
  int16_t list_index = task_in_list(id);
  if(list_index != -1)
  {
    list.task_list_tech[list_index] = tech;
    return 1;
  }
return -1;
}


/***/
