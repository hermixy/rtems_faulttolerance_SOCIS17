#include <rtems/rtems/fts.h>

// can protect up to P_TASK tasks
#define P_TASKS 10
#define BIT_7 128
// data structure
// list of task IDs

struct Task_ID_List {
  rtems_id task_list_id[P_TASKS];
  uint8_t m[P_TASKS];
  uint8_t k[P_TASKS];
  fts_tech task_list_tech[P_TASKS];
  bitstring_pattern *pattern[P_TASKS];
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

/* Show the execution pattern in the console */
uint8_t show_pattern(uint8_t *p_curr, uint8_t *p_end, uint8_t maxbit)
{
  printf("\nfts.c: The pattern is:\n");

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
        b_mask >>= 1;
    }
    printf("\n");
    //break;
  }
  printf("\n\n");
  /* End output pattern */
  return 0;
}

// sets the sre execution pattern for a task
int8_t fts_set_sre_pattern(
  rtems_id id,
  bitstring_pattern *p
)
{
    int sre_index = task_in_list(id);
  //if( (sre_index != -1) && list.task_list_tech[sre_index] == SRE )
  //{
    list.pattern[sre_index] = p;

    show_pattern(p->pattern_start, p->pattern_end, 7);

    uint8_t *p_curr = list.pattern[sre_index]->pattern_start;
    uint8_t *p_end = list.pattern[sre_index]->pattern_end;

    show_pattern(p_curr, p_end, 7);

    return 1;
}

////

static fts_version sre_next_version(
  int i
)
{
    uint8_t bitpos = list.pattern[i]->bitpos; // bit to be read
    uint8_t c_byte = list.pattern[i]->curr_pos; // byte to be read
    uint8_t bit_mask_one = BIT_7 >> bitpos;

    uint8_t result_bit = c_byte & bit_mask_one;

    if (list.pattern[i]->bitpos < 7)
    {
      list.pattern[i]->bitpos++;
    }
    else
    {
      if (list.pattern[i]->pattern_end == c_byte)
      {
        list.pattern[i]->bitpos = 0;
        list.pattern[i]->curr_pos = list.pattern[i]->pattern_start;
      }
      else
      {
      list.pattern[i]->bitpos = 0;
      list.pattern[i]->curr_pos++;
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
  uint8_t m_,
  uint8_t k_,
  fts_tech tech
)
{
  if ((list.task_list_index < P_TASKS) && (task_in_list(id) == -1) && (m_ <= k_) )
  {
    /* Test (m,k) output */

    printf("\nfts.c: test m = %i\n", m_);
    printf("\nfts.c: test k = %i\n", k_);

    /* Put all information in the tasklist */
    printf("\nfts.c: ID: %i\n", list.task_list_id[list.task_list_index]);
    list.m[list.task_list_index] = m_;

    list.k[list.task_list_index] = k_;

    list.task_list_index++;
    /* Generate (m,k)-pattern, then put in list */

    /*  */

    /* Output values for (m,k) */
    uint8_t m_m = list.m[list.task_list_index];
    uint8_t k_k = list.k[list.task_list_index];

    printf("\nfts.c: m = %i\n", m_m);
    printf("\nfts.c: k = %i\n", k_k);
    /*  */

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
      list.m[i] = list.m[i+1];
      list.k[i] = list.k[i+1];
      list.task_list_tech[i] = list.task_list_tech[i+1];
      list.pattern[i] = list.pattern[i+1];

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
