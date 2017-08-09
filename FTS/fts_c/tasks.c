#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include "hea.h"
#include "rtems/cpuuse.h"
#include <rtems/rtems/fts.h>

uint8_t setpatt = 0;
fts_tech curr_tech = SRE;
uint8_t pattsize = 16;
uint8_t pattern_s;
m_k mk = {4,4};

rtems_task Task_1(
  rtems_task_argument unused
)
{
    rtems_id selfid = rtems_task_self();
    /*
    int8_t reg_status = fts_rtems_task_register(selfid, mk, curr_tech);
    if (reg_status == 1)
    {
      printf("\nTask_1 is registered!\n");
    }
    */
    if (setpatt == 0)
    {
        uint8_t *b = &pattern_s;
        uint8_t *e = b+2;

        for (; b < e; ++b)
          {
            uint8_t bitw = 1;
            for (uint8_t i = 0; i < 8; i++ )
            {
              *b = bitw & *b;
              bitw = bitw << 1;
            }
          }
          /*
          bitstring_pattern bmap = { .pattern_start = &pattern_s, .pattern_end = &pattern_s+pattsize-1, .curr_pos = &pattern_s, .bitpos = 0, .max_bitpos = 7};
          int8_t bm_status = fts_set_sre_pattern(selfid, bmap);
          if (bm_status==1)
          {
              printf("BMAP IS IN");
          }
          */
    }

    printf( "\nTASK_1\n\n");
};
