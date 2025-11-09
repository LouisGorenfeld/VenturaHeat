#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
   FILE *fp;
   FILE *out;
   int len;
   unsigned char c;
   int b = 0;
   int wrap = 10;
	
   if (argc < 3)
		return 1;

	fp = fopen(argv[1], "rb");
	out = fopen(argv[3], "w");

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

        fprintf(out, "unsigned char %s[] = {\n", argv[2]);

	while(1)
	{
		if ((b % wrap) == 0)
			fprintf(out, "\t");
			
		c = fgetc(fp);
		fprintf(out, "0x%02x", (unsigned int)c);

		if (feof(fp))
		{
			fprintf(out, "\n};\n\n");
			break;
		}
		else
		{
			fprintf(out, ", ");
		}

		if ((b++ % wrap) == wrap - 1)
			fprintf(out, "\n");
	}

	fclose(fp);
	fclose(out);

	return 0;
}

