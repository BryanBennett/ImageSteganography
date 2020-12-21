/**
 * This program either hides or recovers hidden text within an image (ppm file). The binary of the characters from the hidden text are 
 * stored in the n = "bit" least-significant bits of the RGB bytes associated with each pixel in the image. 
 * ':)' is hidden at the end of the ppm file to indicate the end of the text file during the recovery process
 * "ppm_read_write.h" is used to process ppm image into struct which is stored in dynamic memory
 * "get_image_args.h" is used to parse command line arguments. Appropriate error messages are printed when necessary
 *
 * @author Bryan Bennett {@literal <bennbc16@wfu.edu>}
 * @date Oct. 26, 2020
 * @assignment Lab 5
 * @course CSC 250
 **/

#include "get_image_args.h"
#include "ppm_read_write.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Function declarations */
int hide_message(struct Image *img, unsigned int bit, char text_file_name[]);
int read_message(struct Image *img, unsigned int bit, char text_file_name[]);
unsigned char extract_char(unsigned char **pixel_comp, unsigned int *pixel_index, int bit);

/* Main: Calls appropriate functions depending on whether "hide" or "recover" is specified. Also tracks errors */
int main(int argc, char *argv[]){
    
    /* Variable declarations */
    struct Image *img = NULL;                                                               /* Pointer to the image struct stored in dynamic memory */
    char img_file_name[20];                                                                 /* File name for image, initialized by process_image_args */
    char txt_file_name[20];                                                                 /* File name for text file, initialized by process_image_args */
    int hide;                                                                               /* Indicates whether you are hiding (1) or recovering (0) */
    int bit;                                                                                /* Indicates number of least-sig bits to read/overwrite at end of each RGB byte */
    int status_ok;                                                                          /* 0 indicates error, 1 indicates proper functionality */
    int final_return_code = 0;                                                              /* What main returns. Zero if status_ok = 1 */
    
    status_ok = process_image_args(argc, argv, &hide, &bit, img_file_name, txt_file_name);  /* process_image_args parses command line arguments. Located in get_image_args */
    if ( !status_ok ) {                                                                     /* Check if process_image_args returned an error */
        printf("Process_image_args failed to retreive proper arguments.\n");
        final_return_code++;
    }

    status_ok = read_image(&img, img_file_name);                                            /* Converts ppm to dynamically-allocated struct 'img' */
    if ( !status_ok ) {                                                                     /* Check if read_image returned an error, if so increment final_return_code. Memory is freed later */
        printf("PPM could not be opened or read.\n");
        final_return_code++;
    }

    /* If process_image_args and read_image worked, determine whether to hide or recover */
    if ( !final_return_code ) {
        if (hide) {                                                                         /* Hide. Call hide_message and write_image */
            status_ok = hide_message(img, (unsigned int)bit, txt_file_name);                /* Modifies least-sig bits inside struct of pixels */   
            if ( !status_ok ) {                                                             /* Check if hide_message returned error */
                printf("hide_message returned an error.\n");
                final_return_code++;
            }  
            status_ok = write_image(img_file_name, img);                                    /* Converts this struct of pixels to a ppm file */
        }
        else {                                                                              /* Hide = 0, so recover */
            status_ok = read_message(img, (unsigned int)bit, txt_file_name);                /* Interprets chars stored in img, prints to txt_file_name */
        }
    }

    /* Free dynamic memory for data pointer in image struct and the image itself */
    free(img->data);
    free(img);

    /* If write_image or read_message returned errors */
    if ( !status_ok ) {
        printf("Error in either write_image or read_message.\n");
        final_return_code++;
    }
    
    return final_return_code;                                                               /* Returns zero if status_ok was never false */
}

/* Function definitions */

/* Opens and reads text file, stores characters in the last "n = bit" bits of each RGB byte in each pixel within img.data */
/* Returns 1 if no errors, zero otherwise */
int hide_message(struct Image *img, unsigned int bit, char text_file_name[])
{
    /* Variable declarations */
    unsigned char *pixel_comp = (unsigned char *) img->data;                            /* Cast img.data as a list of unsigned chars, so we can navigate each byte */
    int num_chars = 0;                                                                  /* Number of characters pulled from txt file and written-to ppm */
    unsigned int i = 0;                                                                 /* Tracks index in pixel_comp, also acts as num_pixels */
    int j = 0;                                                                          /* Used in for-loop to navigate individual bits in each text char. Signed int because it is compared to -1 for loop navigation */
    unsigned char c;                                                                    /* The character read from txt file */
    unsigned char tmp;                                                                  /* Used to manipulate and shift 'c' for storage in pixel_comp */
    FILE *fptr = NULL;                                                                  /* Pointer to text file that is read */
    unsigned char bitmask = ((unsigned char)0xff) >> (unsigned char)(8u - bit);         /* Bit mask, where all bits are 0 except last "n=bit" bits */

    int theoretical_max_chars = ( (img->width) * (img->height) * (3) * (bit)) / 8;      /* Used for stopping criteria. This represents the max number of characters we could hide in ppm, including :) at end */
    int more_space = 1;                                                                 /* Acts as a boolean for while loop termination. Ensures there is more space left in ppm for char storage */
    int status_ok = 1;                                                                  /* Acts as boolean for error detection (1 means all ok). This is returned to main */
    unsigned char last_c = 0;                                                           /* Used to detect if :) exists in text file, which may cause improper early termination during recovery process */
    
    unsigned int k = 0;                                                                 /* Used in post-whileloop forloop to add smiley face */
    unsigned char char_smiley[3] = {':',')','\0'};                                      /* unsigned char array to add :) at the end */
    
    fptr = fopen(text_file_name, "re");                                                 /* Open txt file to read */

    if ( !fptr ) {                                                                      /* Text file cannot be opened or read */
        printf("Unable to open and read text file '%s'. Breaking\n", text_file_name);
        status_ok = 0;
    }

    /* While loop navigates text file. Nested forloop pulls apart each char and puts bits in last "n=bit" bits of each RGB byte */
    c = fgetc(fptr);
    while ((!feof(fptr)) && (more_space) ) {                                            /* while there are more chars in txt file, and space in PPM to place them */

        for (j = (8 - bit); j > -1; j = j - bit){                                       /* j acts as the index within each char, and performs necessary bitshifts */
            tmp = (unsigned int)(c >> (unsigned int)j) & (unsigned int)bitmask;         /* Shift char by j, AND it with bitmask to get last "n=bit" bits */
            pixel_comp[i] = (pixel_comp[i] & (unsigned int)~bitmask) | tmp;             /* AND pixel_comp[i] with the negated bitmask to wipe last n=bit bits, OR it with tmp to place in new bits */
            i++;
        }
        
        num_chars++;                                                                    /* Increment number of characters, as one has just been placed in pixel_comp */

        if (num_chars == (theoretical_max_chars - 2)) {                                 /* Check if there is space for more chars to be placed in PPM during next iteration. "-2" to leave space for ':)' */
            printf("WARNING: No more space for additional character storage. Ending 'hide_message' before the end of text file\n");
            more_space = 0;
        }

        last_c = c;                                                                     /* Store c in last c to detect if ':)' exists in text file */
        c = fgetc(fptr);                                                                /* Get next char */

        if ( (last_c == ':') && (c == ')') ) {                                          /* See if ':)' is in txt file, to warn user early termination may occur during recovery process */
            printf("WARNING: ':)' detected within text file prior to EOF. This may cause premature exit during recovery process\n");
            printf("Text file will continue to be hidden in ppm, but recovery process may terminate early\n");
        }
        
    }

    /* Add smiley face at the end. Nexted forloop the same as one used in while loop */
    for (k = 0; k < 2; k++) {
        for (j = (8 - bit); j > -1; j = j - bit){
            tmp = (unsigned int)(char_smiley[k] >> (unsigned int)j) & (unsigned int)bitmask;
            pixel_comp[i] = (pixel_comp[i] & (unsigned int)~bitmask) | tmp;
            i++;
        }
        num_chars++;
    }

    /* Print char and pixel counts, close text file and return status_ok */
    printf("%d characters hidden in %u pixels\n", num_chars, i);
    fclose(fptr);
    return status_ok;
}


/* Parses img.data, combines last "n = bit" bits of each RGB byte in each pixel to chars and prints to text_file_name */
/* Returns 1 if no errors, zero otherwise */
int read_message(struct Image *img, unsigned int bit, char text_file_name[])
{
    /* Variable declarations */ 
    unsigned char *pixel_comp = (unsigned char *) img->data;                                /* Cast img.data as list of chars so we can navigate each byte */
    int num_chars = 0;                                                                      /* Number of characters decoded from ppm data, including ':)' */
    unsigned int i = 0;                                                                     /* Index in pixel_comp, acts as num_pixels as well */
    unsigned char c = 0;                                                                    /* The character extracted from pixel_comp */
    FILE *fptr = NULL;                                                                      /* Text file to write recovered text to */

    int theoretical_max_chars = ( (img->width) * (img->height) * (3) * (bit)) / 8;          /* Maximum amount of chars that could be stored in ppm, used to warn user if original text file could have been truncated */
    int status_ok = 1;                                                                      /* Boolean for detection. Status_ok = 1 means all is well */

    fptr = fopen(text_file_name, "we");                                                     /* Try and open write-to file, if it doesn't work tell user */
    if ( !fptr ) {
        printf("Unable to open and write-to text file '%s'. Breaking\n", text_file_name);
        status_ok = 0;
    }

    /* Convert pixel_comp data to a character using extract_char method, which returns a zero if ':)' detected */
    /* Navigate ppm data until extract_char returns zero, updating num_chars and pushing c to txt file */
    c = extract_char(&pixel_comp, &i, bit);
    
    while (c) {
        fputc(c, fptr);
        num_chars++;        
        c = extract_char(&pixel_comp, &i, bit);
    }
    num_chars = num_chars + 2;                                                              /* Takes into account ':)', adds two to num_chars */

    /* If the number of characters pulled before ':)' = maximum number of charactes ppm can hold, tell user original file may have been truncated */
    if (num_chars == theoretical_max_chars) {
        printf("WARNING: Maximum character storage capacity (end of PPM file) reached.\n");
        printf("There is a chance the original text file was truncated during the hiding process\n");
    }    

    /* Print pixel, char, and file information, close txt file and return */
    printf("%d characters recovered from %u pixels\n", num_chars, i);
    printf("Extracted text printed to text file '%s'\n", text_file_name);
    fclose(fptr);
    return status_ok;
}


/* Navigates pixel_comp (img cast as char) using pixel_index and forms chars from last "n = bit" bits within each RGB byte */
/* Also checks for termination criteria ':)', which if detected returns 0 to let read_message know */
unsigned char extract_char(unsigned char **pixel_comp, unsigned int *pixel_index, int bit)
{
    /* Variable declarations */ 
    int j = 0;                                                                          /* Used as an index to know where you are within the new char you are forming */
    unsigned int k = *pixel_index;                                                      /* k is an index within pixel_comp, which we use to update pixel_index if ':)' not detected */
    unsigned int m = k + (8 / bit);                                                     /* m is an index within pixel_comp, which we use to evaluate the next char and check for ':)' */
    unsigned char c = 0;                                                                /* The character we are forming */
    unsigned char c_next = 0;                                                           /* The next character we will form, used to detect ':)' */
    unsigned char bitmask = (unsigned char)(0xff) >> (unsigned char)(8u - bit);         /* Bitmask, where all 8 bits are 0 except the "n = bit" least-significant bits */
    
    /* Navigates pixel_comp, forms character and next character using bit shifting and bitmask */
    for ( j = (8 - bit); j > -1; j = j - bit) {                                         /* j is also used to bit-shift (from most-sig bits to least). This is why it is defined in this way */
        c = (((*pixel_comp)[k] & (unsigned int)bitmask) << (unsigned int)j) | c;           /* AND pixel_comp with bitmask, shift to correct position in char and OR it with itself to retain all 8 bits */
        c_next = (((*pixel_comp)[m] & (unsigned int)bitmask) << (unsigned int)j) | c_next; /* Same process as above, but do so with the next character to be formed */
        k++;
        m++;
    }

    /* Check if termination criteria are met, i.e. ':)' detected. If so, set c = 0 as we don't want to print it */
    if (( c == ':' ) && ( c_next == ')')) {
        c = 0;
        *pixel_index = m;                                                               /* Since ':)' detected, we know this is EOF so pixel_index = the index after ')' */
    }
    else {
        *pixel_index = k;                                                               /* Not end of recovery process, so pixel_index is the index after c */
    }

    return c;
}
