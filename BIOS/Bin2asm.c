#include <stdio.h>

int main(int argc, char*argv[])
{
  FILE  *f_in, *f_out;
  char  b;

  if (argc!=3)
  {
    printf("Syntax: bin2asm <infile> <outfile>\n");
  }

  f_in = fopen(argv[1], "rb");
  f_out= fopen(argv[2], "wb");
  if (f_in == NULL)
    printf("Couldn't open input file\n");
  if (f_out == NULL)
    printf("Couldn't open output file\n");
  if (f_in == NULL || f_out == NULL)
    exit(1);

  while (!feof(f_in))
    fprintf(f_out, " DB %d\n", fgetc(f_in));

  fclose(f_in);
  fclose(f_out);
}

