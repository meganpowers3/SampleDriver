
/**************************************************************
* Class:  CSC-415-01 Fall 2020
* Name: Em Powers
* Student ID: 92073439
* Project: Assignment 6 - Device Driver
*
* File: driver_tester.c
*
* driver_tester represents a userspace program that will use
* kernel functions to represent a calculator device program.
*
**************************************************************/

 // File libraries for program control

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include<stdio.h>
#include<sys/ioctl.h>
 
// We need to include these for ioctl definitions

#define WR_VALUE _IOW('a', 'a', int32_t*)
#define RD_VALUE _IOR('a', 'b', int32_t*)

#define BUFFER_LENGTH 256               // Formats how many numbers we can display
int32_t value, number1, number2, output, operation; // Our operational values


int main(){
   int ret, fd, num;
   
   printf("Starting device test code example...\n");

   fd = open("/dev/sampledriver", O_RDWR);             // Open the device with read/write access

   if (fd < 0){
      perror("Failed to open the device..."); // Error handling for device open
      return errno;
   }
   
   printf("1. Add \n2. Subtract \n3. Multiply \n4. Divide\n"); // Present calc options to user
   scanf("%d", &num);

   write(fd, (int32_t*) &num, sizeof(&num)); // We write to LKM based off of whatever number they inputted

   printf("Enter first value : "); // We're checking for the left hand side of the operation
   scanf("%d", &number1);

   printf("Enter second value : "); // The right-hand operation
   scanf("%d", &number2);

   printf("Writing to driver\n");

   // We need to perform two more writes for LHS/RHS
   write(fd, (int32_t*) &number1, sizeof(number1)); 
   write(fd, (int32_t*) &number2, sizeof(number2));

   // Perform the ioctl operation and place in value

   ioctl(fd, RD_VALUE, (int32_t*) &value);

   // We need to read the answer
   printf("Reading value from driver\n");
 
   read(fd, &output, BUFFER_LENGTH);
   printf("Value is : %d\n", output);

   // Finally, we close our device driver
   printf("Closing driver\n");
   close(fd);
}
