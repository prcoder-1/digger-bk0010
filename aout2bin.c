#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#pragma pack(push, 1)
struct A_OUT
{
    uint16_t magic;
    uint16_t text_size;
    uint16_t data_size;
    uint16_t bss_size;
    uint16_t symbol_table_size;
    uint16_t entry_location;
    uint16_t unused;
    uint16_t relocation_info_suppressed;
};
#pragma pack(pop)

int main(int ac, char *av[])
{
    if (ac != 3)
    {
	printf("Usage: <a.out file name> <bin file name>%s\n", av[0]);
	return -1;
    }
    
    FILE *f = fopen(av[1], "rb");
    if (!f)
    {
	printf("Error opening file '%s' for reading.\n", av[1]);
	return -1;
    }
    
    fseek(f, 0L, SEEK_END);
    long file_len = ftell(f);
    fseek(f, 0L, SEEK_SET);
    
    uint8_t *buffer = malloc(file_len);
    if (!buffer)
    {
	printf("Error allocating %li bytes of memory.\n", file_len);
	return -1;
    }
    
    if (fread(buffer, file_len, 1, f) != 1)
    {
	printf("Error reading file '%s'.\n", av[1]);
	return -1;
    }
    
    fclose(f);
    
    struct A_OUT *a_ptr = (struct A_OUT *)buffer;
    printf("Magic number         = %04o\n", a_ptr->magic);
    printf("Text size            = %04o (%d bytes)\n", a_ptr->text_size, a_ptr->text_size);
    printf("Data size            = %04o (%d bytes)\n", a_ptr->data_size, a_ptr->data_size);
    printf("BSS  size            = %04o (%d bytes)\n", a_ptr->bss_size, a_ptr->bss_size);
    printf("Symbol table  size   = %04o (%d bytes)\n", a_ptr->symbol_table_size, a_ptr->symbol_table_size);
    printf("Entry location       = %04o\n", a_ptr->entry_location);
    printf("Rel. info suppressed = %04o\n", a_ptr->relocation_info_suppressed);

    uint8_t *text_ptr = (uint8_t *)(a_ptr + 1);
    uint16_t bin_size = a_ptr->text_size + a_ptr->bss_size + a_ptr->data_size;
    
    printf("\nBinary file length   = %04o (%d byes)\n", bin_size, bin_size);

    f = fopen(av[2], "wb");
    if (!f)
    {
	printf("Error opening file '%s' for writing.\n", av[2]);
	return -1;
    }

    if (fwrite(&(a_ptr->entry_location), sizeof(a_ptr->entry_location), 1, f) != 1 ||
        fwrite(&bin_size, sizeof(bin_size), 1, f) != 1 ||
        fwrite(text_ptr, bin_size, 1, f) != 1)
    {
	printf("Error writing file '%s'.\n", av[2]);
	return -1;
    }

    fclose(f);

    free(buffer);

    printf("\nBinary file '%s' generated.\n", av[2]);
            
    return 0;
}
