/* +++Date last modified: 05-Jul-1997 */

/*
**  BITFILES.C - reading/writing bit files
**
**  Public domain by Aare Tali
*/

#include <stdlib.h>
#include "bitops.h"

bfile *bfopen(char *name, char *mode)
{
      bfile * bf;

      bf = malloc(sizeof(bfile));
      if (NULL == bf)
            return NULL;
      bf->file = fopen(name, mode);
      if (NULL == bf->file)
      {
            free(bf);
            return NULL;
      }
      bf->rcnt = 0;
      bf->wcnt = 0;
      return bf;
}

int bfread(bfile *bf)
{
      if (0 == bf->rcnt)          /* read new byte */
      {
            bf->rbuf = (char)fgetc(bf->file);
            bf->rcnt = 8;
      }
      bf->rcnt--;
      return (bf->rbuf & (1 << bf->rcnt)) != 0;
}

void bfwrite(int bit, bfile *bf)
{
      if (8 == bf->wcnt)          /* write full byte */
      {
            fputc(bf->wbuf, bf->file);
            bf->wcnt = 0;
      }
      bf->wcnt++;
      bf->wbuf <<= 1;
      bf->wbuf |= bit & 1;
}

void bfclose(bfile *bf)
{
      fclose(bf->file);
      free(bf);
}
