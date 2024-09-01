// Guillaume Valadon <guillaume@valadon.net>
// C based binutils and custom helpers

#include <config.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bfd.h>
#include <dis-asm.h>
#include <time.h>

void buffer_to_rust(char *buffer);


// Silly macro that helps removing the unused warnings
#define UNUSED_VARIABLE(id) id=id


/*** Generic helpers ***/

/***
https://lore.kernel.org/lkml/20220801013834.156015-1-andres@anarazel.de/

At one point, libbfd init_disassemble_info started taking 4 arguments instead of 3.
The 4th argument is a styled fprintf function.

Here we will implement copy_buffer_styled, which will just be a wrapper that ignores style changes.
***/
int copy_buffer(void* useless, const char* format, ...) {
    // Use the parameter to prevent optimization
    (void)useless;  // mark as unused


    va_list ap;
    va_start(ap, format);
    
    int len = vsnprintf(NULL, 0, format, ap);
    
    va_end(ap);

    if (len < 0) {
        fprintf(stderr, "Warning: vsnprintf \n");
    }

    char * buffer_asm = malloc(len + 1);

    if (!buffer_asm) {
        fprintf(stderr, "Warning: buffer_asm malloc failed \n");
    }

    va_start(ap, format);
    // Use return value to check for truncation
    int result = vsnprintf(buffer_asm, len + 1, format, ap);
    
    va_end(ap);

    if (result < 0 || result >= len + 1) {
        fprintf(stderr, "Warning: Buffer overflow in copy_buffer\n");
        buffer_asm[len] = '\0';
    }

    buffer_to_rust(buffer_asm);
    
    // buffer_to_rust uses to_string() which is now owned (cloned) so we can safely free here.
    free(buffer_asm);
}

void copy_buffer_styled(void* user_data, enum disassembler_style style, const char* format, ...) {
    // Use the parameter to prevent optimization
    (void)user_data;  // mark as unused
    (void)style;

    va_list ap;
    va_start(ap, format);
    
    int len = vsnprintf(NULL, 0, format, ap);
    
    va_end(ap);

    if (len < 0) {
        fprintf(stderr, "Warning: vsnprintf \n");
    }

    char * buffer_asm = malloc(len + 1);

    if (!buffer_asm) {
        fprintf(stderr, "Warning: buffer_asm malloc failed \n");
    }

    va_start(ap, format);
    // Use return value to check for truncation
    int result = vsnprintf(buffer_asm, len + 1, format, ap);
    
    va_end(ap);

    if (result < 0 || result >= len + 1) {
        fprintf(stderr, "Warning: Buffer overflow in copy_buffer\n");
        buffer_asm[len] = '\0';
    }

    buffer_to_rust(buffer_asm);
    
    // buffer_to_rust uses to_string() which is now owned (cloned) so we can safely free here.
    free(buffer_asm);
}

void show_buffer(struct disassemble_info *info) {
    printf("len=%d - vma=%lu\n", info->buffer_length, info->buffer_vma);
    printf("%p\n", info->buffer);
    printf("%x\n", info->buffer[0]);
    printf("%x\n", info->buffer[1]);
    printf("%x\n", info->buffer[2]);
    printf("%x\n", info->buffer[3]);
}


/*** disassemble_info structure helpers ***/

disassemble_info* new_disassemble_info() {
    /* Return a new structure */
    struct disassemble_info *info = (disassemble_info *) calloc (1, sizeof(struct disassemble_info));
    return info;
}

bfd_boolean configure_disassemble_info(struct disassemble_info *info, asection *section, bfd *bfdFile) {
    if (info == NULL || section == NULL || bfdFile == NULL) {
        fprintf(stderr, "Error: Null pointer passed to configure_disassemble_info\n");
        return FALSE;
    }

    /* Construct and configure the disassembler_info class using stdout */
    init_disassemble_info(info, stdout,(fprintf_ftype) copy_buffer, (fprintf_styled_ftype) copy_buffer_styled);
    info->arch = bfd_get_arch (bfdFile);
    info->mach = bfd_get_mach (bfdFile);
    info->section = section;

    info->buffer_vma = section->vma;
    info->buffer_length = section->size;

    /* Set default flavor to intel for architectures that support it. */
    info->disassembler_options = "intel";

    return bfd_malloc_and_get_section (bfdFile, section, &info->buffer);
}

asection* configure_disassemble_info_buffer(
    struct disassemble_info *info,
    enum bfd_architecture arch,
    unsigned long mach,
    uint64_t vma,
    uint64_t length,
    uint8_t *buffer
) {
    if (info == NULL || buffer == NULL) {
        fprintf(stderr, "Error: Null pointer passed to configure_disassemble_info_buffer\n");
        return NULL;
    }

    init_disassemble_info(info, stdout, (fprintf_ftype) copy_buffer, (fprintf_styled_ftype) copy_buffer_styled);
    
    info->arch = arch;
    info->mach = mach;
    /* Configure the buffer that will be disassembled */
    info->buffer = buffer;
    info->buffer_length = length;
    info->buffer_vma = vma;

    // Use volatile to prevent optimization
    volatile int check = (info->buffer == NULL);
    if (check) {
        fprintf(stderr, "Error: Invalid configuration in configure_disassemble_info_buffer\n");
        return NULL;
    }

    asection *section = (asection*) calloc(1, sizeof(asection));
    if (section) {
        info->section = section;
        memset(section, 0, sizeof(asection));
        info->section->vma = vma;
    }

    /* Set default flavor to intel for architectures that support it. */
    info->disassembler_options = "intel";
    return (asection*) section;
}

typedef void (*print_address_func) (bfd_vma addr, struct disassemble_info *dinfo);
void set_print_address_func(struct disassemble_info *info, print_address_func print_function) {
    info->print_address_func = print_function;
}

asection* get_disassemble_info_section(struct disassemble_info *info) {
  return info->section;
}

unsigned long get_disassemble_info_section_vma(struct disassemble_info *info) {
  return info->section->vma;
}

void free_disassemble_info(struct disassemble_info *info, bool free_section) {
  /* Free the structure and allocated variable */
  if (info) {
    if (free_section && info->section)
      free(info->section);
    free(info);
  }
}


/*** bfd structure helpers ***/

unsigned long get_start_address(bfd *bfdFile) {
    return bfdFile->start_address;
}

unsigned int macro_bfd_big_endian(bfd *bfdFile) {
    return bfd_big_endian(bfdFile);
}


/*** bfd_arch_info structure helpers ***/

enum bfd_architecture get_arch(struct bfd_arch_info *arch_info) {
  return arch_info->arch;
}

unsigned long get_mach(struct bfd_arch_info *arch_info) {
  return arch_info->mach;
}


/*** section structure helpers ***/

unsigned long get_section_size(asection *section) {
    return section->size;
}
