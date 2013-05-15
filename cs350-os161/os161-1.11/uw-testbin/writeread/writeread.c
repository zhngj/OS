/* Tim Brecht
 * Added :Sat  5 Jan 2013 15:19:15 EST
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../lib/testutils.h"

#define NUM_TIMES   (1)
#define NUM_INTS    (4*1024) 

int
main()
{
   int i, rc, fd;
   int write_array[NUM_INTS];
   int read_array[NUM_INTS];

   /* Uncomment this when having failures and for debugging */
   // TEST_VERBOSE_ON();

   /* Initialize the array */
   for (i=0; i<NUM_INTS; i++) {
     write_array[i] = i;
   }
//   printf("00000000000000000000000000000000000000\n");
   /* Open the file for writing */
   fd = open("WRITE_READ_FILE", O_WRONLY | O_CREAT);
   TEST_POSITIVE(fd, "Open file named WRITE_READ_FILE failed\n");
 // printf("00000000000000000000000000000000000000\n");
   
   for (i=0; i<NUM_TIMES; i++) {
   //printf("00000000000000000000000000000000000000\n");
		 rc = write(fd, write_array, sizeof(write_array));
//printf("1111111111111111111111111111111111111111\n");
     TEST_EQUAL(rc, sizeof(write_array), "Failed to write all of the array");
   }
//printf("00000000000000000000000000000000000000\n");
   close(fd);

  // printf("1111111111111111111111111111111111111111\n");
   /* Open the file */
   fd = open("WRITE_READ_FILE", O_RDONLY);
   TEST_POSITIVE(fd, "Open file named WRITE_READ_FILE failed\n");

   for (i=0; i<NUM_TIMES; i++) {
		 rc = read(fd, read_array, sizeof(read_array));
     TEST_EQUAL(rc, sizeof(read_array), "Failed to read all of the array");
     for (i=0; i<NUM_INTS; i++) { 
       TEST_EQUAL(read_array[i], write_array[i], "Value read not equal to value written");
     }
   }
   close(fd);
   fd = open("WRITE_READ_FILE", O_RDONLY);
   TEST_POSITIVE(fd, "Open file named WRITE_READ_FILE failed\n");
   rc = read(fd,(void*)0xffffffff,sizeof(read_array));
   printf("read addr test, return %d\n",rc);
   close(fd);
   fd = open("WRITE_READ_FILE", O_WRONLY | O_CREAT);
   rc = write(fd,(void*)0x40000000,sizeof(write_array));
   printf("write addr test, return %d\n",rc);
   close(fd);
   // invalid
   TEST_STATS();

   exit(0);
}
