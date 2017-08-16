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

/* Show the execution pattern in the console */
uint8_t show_pattern(
  uint8_t *p_curr,
  uint8_t *p_end,
  uint8_t maxbit
)
{
  printf("\n\nfts.c: The pattern is:\n");

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

/* */
int16_t task_in_list(
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
// if task is in list already, return 1, -1 else

int8_t create_pattern(
  rtems_id id,
  pattern_type pattern,
  uint8_t *pattern_s,
  uint8_t *pattern_e,
  uint8_t bitpos_max
)
{
  printf("\nfts.c (create_pattern): in 1\n");
  /* end-start
  * int nr_of_bytes = pattern_e - pattern_s;
  * if (nr_of_bytes * 8 >= list.k[i])
  * {
  *   ;
  * }
  */
  int16_t i = task_in_list(id);
  list.pattern[i]->pattern_start = pattern_s;
  list.pattern[i]->pattern_end = pattern_e;
  list.pattern[i]->max_bitpos = bitpos_max;

  printf("\nfts.c (create_pattern): m is %i, k is %i\n", list.m[i], list.k[i]);

  uint8_t *pattern_it = pattern_s;

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
    for (;pattern_it <= pattern_e; pattern_it++)
    {
      *pattern_it  = ones_8;
    }
  }
  /* evenly distributed "1"s */

  if ( pattern == E_PATTERN )
  {
    uint8_t set_byte = 0;
    //for ( int j = 0; j < list.k[i]; j++ )
    //{
    int j = 0;
    for (;(pattern_it <= pattern_e); pattern_it++)
    {
      uint8_t bitmask = BIT_7;
      for (uint8_t it_bits = 0; (it_bits < 8) ; it_bits++)
      {
        int raman = ( (((j*(list.k[i]-list.m[i]))/list.k[i]) + ((j*(list.k[i]-list.m[i]))%list.k[i] != 0) )* list.k[i]/(list.k[i]-list.m[i]) );

        // 'normal' e-pattern
        //int raman = ( (((j*list.m[i])/list.k[i]) + ((j*list.m[i])%list.k[i] != 0) )* list.k[i]/list.m[i] );

        if (j != raman)
        {
          // insert 1;
          set_byte |= bitmask;
          // first bit is 0
        }
        bitmask >>= 1;
        j++;
      }
      *pattern_it = set_byte;
    }
    //}
  }
  printf("\nfts.c (create_pattern): ");
  uint8_t sh = show_pattern(pattern_s, pattern_e, bitpos_max);
  return 1;
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

    show_pattern(p->pattern_start, p->pattern_end, p->max_bitpos);

    uint8_t *p_curr = list.pattern[sre_index]->pattern_start;
    uint8_t *p_end = list.pattern[sre_index]->pattern_end;

    printf("\n(set_sre_pattern)\n");
    show_pattern(p_curr, p_end, list.pattern[sre_index]->max_bitpos);
    printf("\nfts.c (set pattern): Address of first byte: %p\n", (void *)list.pattern[sre_index]->pattern_start);
    printf("\nfts.c (set pattern): Address of current byte: %p\n", (void *)list.pattern[sre_index]->curr_pos);
    printf("\nfts.c (set pattern): Address of last byte: %p\n", (void *)list.pattern[sre_index]->pattern_end);

    return 1;
}

////

static fts_version sre_next_version(
  int i
)
{
    printf("\nfts.c (sre_next_version):  index is %i\n", i);
    printf("\nfts.c (sre_next_version):\n");

    printf("\nIn sre_next_version:");
    show_pattern(list.pattern[i]->pattern_start, list.pattern[i]->pattern_end, list.pattern[i]->max_bitpos);

    uint8_t bitpos = list.pattern[i]->bitpos; // bit to be read
    uint8_t c_byte = *(list.pattern[i]->curr_pos); // byte to be read
    uint8_t bit_mask_one = BIT_7 >> bitpos;

    uint8_t result_bit = c_byte & bit_mask_one;
    printf("\nfts.c (sre next version): Address of first byte: %p\n", (void *)list.pattern[i]->pattern_start);
    printf("\nfts.c (sre next version): Address of current byte: %p\n", (void *)list.pattern[i]->curr_pos);
    printf("\nfts.c (sre next version): Address of last byte: %p\n", (void *)list.pattern[i]->pattern_end);

    //if in last byte
    if (list.pattern[i]->pattern_end == list.pattern[i]->curr_pos)
    {
      if (list.pattern[i]->bitpos < list.pattern[i]->max_bitpos)
      {
        list.pattern[i]->bitpos++;
      }
      else
      {
        list.pattern[i]->bitpos = 0;
        list.pattern[i]->curr_pos = list.pattern[i]->pattern_start;
      }
    }
    else
    {
      if ( list.pattern[i]->bitpos < 7 )
      {
        list.pattern[i]->bitpos++;
      }
      else
      {
        list.pattern[i]->bitpos = 0;
        list.pattern[i]->curr_pos++;
      }
    }



    //(list.pattern[i]->bitpos < list.pattern[i].max_bitpos)

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
    /* Put all information in the tasklist */
    list.task_list_id[list.task_list_index] = id;
    //output
    printf("\nfts.c (register): ID %i\n", list.task_list_id[list.task_list_index]);

    list.task_list_tech[list.task_list_index] = tech;
    //output
    printf("\nfts.c (register): Tech %i\n", list.task_list_tech[list.task_list_index]);

    list.m[list.task_list_index] = m_;
    list.k[list.task_list_index] = k_;

    uint8_t m_m = list.m[list.task_list_index];
    uint8_t k_k = list.k[list.task_list_index];

    printf("\nfts.c (register): List index %i\n", list.task_list_index);
    list.task_list_index++;

    /* Generate (m,k)-pattern, then put in list */

    /*  */

    /* Output values for (m,k) and ID */
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
  printf("\nfts.c (get mode) ID: %i\n", id);
  int16_t task_index = task_in_list(id);

  printf("\nfts.c (get mode): list index %i\n", task_index);

  fts_version next_version;
  if (task_index != -1)
  {
    switch(list.task_list_tech[task_index])
    {
      case NONE :
        next_version = BASIC;
        printf("\nfts.c (get_mode): NONE\n");
        break;

      case SRE :
        /*
        ;
        char *ptr = (char *)0x0200724C;
        ;
        printf("fts.c (get_mode): Problematic address at: %p, value is: %i \n",ptr, *ptr);
        ; */
        next_version = sre_next_version(task_index);
        printf("\nfts.c (get_mode): SRE\n");
        break;

      case SDR :
        next_version = RECOVERY;
        printf("\nfts.c (get_mode): SDR\n");
        break;

      case DRE :
        next_version = RECOVERY;
        printf("\nfts.c (get_mode): DRE\n");
        break;

      case DDR :
        next_version = RECOVERY;
        printf("\nfts.c (get_mode): DDR\n");
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
