#include <stdio.h>

#define BLKSIZE 4096

int main( int argc, char *argv[] )
{ FILE *infile, *outfile;
  int i, nbytes, sum;
  long filesize, chksum_offset;
  char block [BLKSIZE];

  /****************************************************** */

  printf("DIVA Checksum generator 2.01 INH 13-5-93\n");

  if (argc !=3)
  { printf("Usage: MKROM sourcefile destfile\n");
    printf("Converts a .COM or .ROM type file for loading into ROM\n");
    return 1;
  }

  if ( (infile=fopen(argv[1], "rb")) == NULL)
  { printf("Cannot open <%s> for read\n", argv[1]);
    return 1;
  }

  if ( (outfile=fopen(argv[2], "wb")) == NULL )
  { printf("Error: <%s> would not open for write\n",argv[2]);
    return 1;
  }

  /* Find size **************************** */

  fseek ( infile, 0L, SEEK_END);
  filesize = ftell(infile);
  rewind(infile);

  /* Process file block by block ********** */

  chksum_offset  = filesize - 17;
  sum = 0;

  do
  {
    nbytes = fread( block, sizeof(char), BLKSIZE, infile );
    for (i=0; i<nbytes; i++) sum += block[i];

    if ( chksum_offset >= 0 && chksum_offset < nbytes )
      block[ chksum_offset ] -= ( sum & 0xFF );

    fwrite ( block, sizeof(char), nbytes, outfile );
    chksum_offset -= nbytes;

  } while ( nbytes == BLKSIZE );

  fclose(infile);
  fclose(outfile);

  printf("File converted: %ld bytes\n", filesize);

  return 0;
}


