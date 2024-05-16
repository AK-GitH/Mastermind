/*
 * MasterMind implementation: template; see comments below on which parts need to be completed
 * CW spec: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf
 * This repo: https://gitlab-student.macs.hw.ac.uk/f28hs-2021-22/f28hs-2021-22-staff/f28hs-2021-22-cwk2-sys

 * Compile: 
 gcc -c -o lcdBinary.o lcdBinary.c
 gcc -c -o master-mind.o master-mind.c
 gcc -o master-mind master-mind.o lcdBinary.o
 * Run:     
 sudo ./master-mind

 OR use the Makefile to build
 > make all
 and run
 > make run
 and test
 > make test

 ***********************************************************************
 * The Low-level interface to LED, button, and LCD is based on:
 * wiringPi libraries by
 * Copyright (c) 2012-2013 Gordon Henderson.
 ***********************************************************************
 * See:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
*/

/* ======================================================= */
/* SECTION: includes                                       */
/* ------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include <unistd.h>
#include <string.h>
#include <time.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

/* --------------------------------------------------------------------------- */
/* Config settings */
/* you can use CPP flags to e.g. print extra debugging messages */
/* or switch between different versions of the code e.g. digitalWrite() in Assembler */
#define DEBUG
#undef ASM_CODE

// =======================================================
// Tunables
// PINs (based on BCM numbering)
// For wiring see CW spec: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf
// GPIO pin for green LED
#define LED 13
// GPIO pin for red LED
#define LED2 5
// GPIO pin for button
#define BUTTON 19
// =======================================================
// delay for loop iterations (mainly), in ms
// in mili-seconds: 0.2s
#define DELAY   200
// in micro-seconds: 3s
#define TIMEOUT 3000000
// =======================================================
// APP constants   ---------------------------------
// number of colours and length of the sequence
#define COLS 3
#define SEQL 3
// =======================================================

// generic constants

#ifndef	TRUE
#  define	TRUE	(1==1)
#  define	FALSE	(1==2)
#endif

#define	PAGE_SIZE		(4*1024)
#define	BLOCK_SIZE		(4*1024)

#define	INPUT			 0
#define	OUTPUT			 1

#define	LOW			 0
#define	HIGH			 1

//User defined
static int timerPL = 0;

// =======================================================
// Wiring (see inlined initialisation routine)

#define STRB_PIN 24
#define RS_PIN   25
#define DATA0_PIN 23
#define DATA1_PIN 10
#define DATA2_PIN 27
#define DATA3_PIN 22

/* ======================================================= */
/* SECTION: constants and prototypes                       */
/* ------------------------------------------------------- */

// =======================================================
// char data for the CGRAM, i.e. defining new characters for the display

static unsigned char newChar [8] = 
{
  0b11111,
  0b10001,
  0b10001,
  0b10101,
  0b11111,
  0b10001,
  0b10001,
  0b11111,
} ;

/* Constants */

static const int colors = COLS;
static const int seqlen = SEQL;

static char* color_names[] = { "red", "green", "blue" };

static int* theSeq = NULL;

static int *seq1, *seq2, *cpy1, *cpy2;

/* --------------------------------------------------------------------------- */

// data structure holding data on the representation of the LCD
struct lcdDataStruct
{
  int bits, rows, cols ;
  int rsPin, strbPin ;
  int dataPins [8] ;
  int cx, cy ;
} ;

static int lcdControl;

/* ***************************************************************************** */
/* INLINED fcts from wiringPi/devLib/lcd.c: */
// HD44780U Commands (see Fig 11, p28 of the Hitachi HD44780U datasheet)

#define	LCD_CLEAR	0x01
#define	LCD_HOME	0x02
#define	LCD_ENTRY	0x04
#define	LCD_CTRL	0x08
#define	LCD_CDSHIFT	0x10
#define	LCD_FUNC	0x20
#define	LCD_CGRAM	0x40
#define	LCD_DGRAM	0x80

// Bits in the entry register

#define	LCD_ENTRY_SH		0x01
#define	LCD_ENTRY_ID		0x02

// Bits in the control register

#define	LCD_BLINK_CTRL		0x01
#define	LCD_CURSOR_CTRL		0x02
#define	LCD_DISPLAY_CTRL	0x04

// Bits in the function register

#define	LCD_FUNC_F	0x04
#define	LCD_FUNC_N	0x08
#define	LCD_FUNC_DL	0x10

#define	LCD_CDSHIFT_RL	0x04

// Mask for the bottom 64 pins which belong to the Raspberry Pi
//	The others are available for the other devices

#define	PI_GPIO_MASK	(0xFFFFFFC0)

static unsigned int gpiobase ;
static uint32_t *gpio;

static int timed_out = 0;

/* ------------------------------------------------------- */
// misc prototypes

int failure (int fatal, const char *message, ...);
void waitForEnter (void);
void waitForButton (uint32_t *gpio, int button);

/* ======================================================= */
/* SECTION: hardware interface (LED, button, LCD display)  */
/* ------------------------------------------------------- */
/* low-level interface to the hardware */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Either put them in a separate file, lcdBinary.c, and use   */
/* inline Assembler there, or use a standalone Assembler file */
/* You can also directly implement them here (inline Asm).    */
/* ********************************************************** */

/* These are just prototypes; you need to complete the code for each function */

/* send a @value@ (LOW or HIGH) on pin number @pin@; @gpio@ is the mmaped GPIO base address */
void digitalWrite(uint32_t *gpio, int pin, int value) {
    int fSel;

    if (value) {
        fSel = 7;
    } else {
        fSel = 10;
    }

    asm volatile (

        "LDR r2, [%[gpio], %[fSel], LSL #2]\n\t" 
        // Load gpio register dictated by offset Value (fSel) into r2
        "MOV r4, #1\n\t" 
        // Prepare the value 1
        "LSL r4, %[pin]\n\t"        
        // Shift the value to the appropriate bit position dictated by the pin number
        "STR r4, [%[gpio], %[fSel], LSL #2]\n\t" 
        // Store the modified bits back into the specific register dictated by gpset_offset
        :
        : [gpio] "r" (gpio), [pin] "r" (pin), [fSel] "r" (fSel)
        : "r2", "r3", "r4", "memory"
    );
}

/* set the @mode@ of a GPIO @pin@ to INPUT or OUTPUT; @gpio@ is the mmaped GPIO base address */
void pinMode(uint32_t *gpio, int pin, int mode) {

  //calculate offset value
  int fSel = pin / 10;
  //calculate shift value
  int shift = (pin % 10) * 3;


  if (OUTPUT) {
    
    //set mode to output
    asm volatile (
      "LDR r1, [%[gpio], %[fSel], LSL #2]\n\t"   
      //Load gpio registery dictated by offsetValue (fSel) into r1
      "MOV r2, #1\n\t"  
      //Load 1 into r2
      "LSL r2, r2, %[shift]\n\t"          
      //Shift r2 to the left amount of times dictated by shift variable
      "MOV r3, #7\n\t"                    
      //Prepare mask for clearing by loading 7 (111) into r3
      "LSL r3, r3, %[shift]\n\t"          
      //Shift the mask to the correct position dictated by shift variable
      "MVN r3, r3\n\t"                    
      //Negate the mask to create a clear mask (same thing as ~(111))
      "AND r1, r1, r3\n\t"                
      //Clear the bits for the specific pin 
      "ORR r1, r1, r2\n\t"                
      //Set the pin mode bits
      "STR r1, [%[gpio], %[fSel], LSL #2]\n\t"  
      //Store the bits that correspond to setting the mode of the given pin to input/output back into the registery
      :
      : [gpio] "r" (gpio), [fSel] "r" (fSel), [shift] "r" (shift)
      : "r1", "r2", "r3", "cc", "memory"
    );

  }

  //Set the GPIO pin to input mode
  else {
    asm volatile (
      "LDR r1, [%[gpio], %[fSel], LSL #2]\n\t"  
      //Load gpio registery dictated by offset Value (fSel) into r1
      "MOV r2, #7\n\t"  
      //Prepare mask for clearing by loading 7 (111) into r3
      "LSL r2, r2, %[shift]\n\t"  
      //Shift r2 to the left amount of times dictated by shift variable
      "MVN r2, r2\n\t"  
      //Negate the mask to create a clear mask (same thing as ~(111))
      "AND r1, r1, r2\n\t"  
      //Clear the bits for the specific pin
      "STR r1, [%[gpio], %[fSel], LSL #2]\n\t"  
      //Store the bits that correspond to setting the mode of the given pin to input/output back into the registery
      :
      : [gpio] "r" (gpio), [fSel] "r" (fSel), [shift] "r" (shift)
      : "r1", "r2", "cc", "memory" 
    );
  }
}

/* send a @value@ (LOW or HIGH) on pin number @pin@; @gpio@ is the mmaped GPIO base address */
/* can use digitalWrite(), depending on your implementation */
//digitalWrite was chosen in place of writeLED for this coursework.
void writeLED(uint32_t *gpio, int led, int value);


/* read a @value@ (LOW or HIGH) from pin number @pin@ (a button device); @gpio@ is the mmaped GPIO base address */
int readButton(uint32_t *gpio, int button) {


  int fSel = 13;
  //initialize state
  int state = 0;
  asm volatile(
      "LDR r2, [%[gpio], %[fSel], LSL #2]\n\t"
      //Load gpio registery dictated by offset Value (fSel) into r1
      "MOV r3, %[pin]\n"
      //Load pin number into r3
      "MOV r4, #1\n"
      //Load 1 into r4
      "LSL r4, R3\n"
      //Shift 1 an amount of times dictated by pin value
      "AND %[state], r2, r4\n"
      //AND operation between the shifted 1 in r2, and the current value of bits in the gpio registery
      : [state] "=r"(state) 
      : [pin] "r"(button) , [gpio] "r"(gpio), [fSel] "r" (fSel)
      : "r2", "r3", "r4", "cc"
  );
  
  //return if state is HIGH (1) or LOW (0)
  return state > 0;
}

/* wait for a button input on pin number @button@; @gpio@ is the mmaped GPIO base address */
/* can use readButton(), depending on your implementation */
void waitForButton (uint32_t *gpio, int button){

    //last state of the button initialised
    int last_state = readButton(gpio, BUTTON);
    while(1) {
      //current state of the button initialised
      int curr_state = readButton(gpio, BUTTON);
      //if state of the button changes (0 to 1), exit loop and end the function.
      if (curr_state != last_state) {
        break;
      }

      if (timerPL) {
          break;
      }

    }
}

/* ======================================================= */
/* SECTION: game logic                                     */
/* ------------------------------------------------------- */
/* AUX fcts of the game logic */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* initialise the secret sequence; by default it should be a random sequence */
void initSeq() {

  //Calculate the amount of memory needed per sequence length (seqLen) and allocate it into theSeq
  theSeq = (int *)malloc(seqlen * sizeof(int));

  //for each index of theSeq, assign a random number up to 3 with a lower bound of 1  
  for (int i = 0; i < seqlen; i++) {
    theSeq[i] = (rand() % 3) + 1;
  }

}

/* display the sequence on the terminal window, using the format from the sample run in the spec */
void showSeq(int *seq) {
  
  //print all sequence characters per numbers in seq
  for (int i = 0; i < seqlen; i++) {

    
    switch(seq[i]) {
      case 1: printf("R "); break;
      //if element at index i of seq == 1, print R
      case 2: printf("G "); break;
      //if element at index i of seq == 2, print G
      case 3: printf("B "); break;
      //if element at index i of seq == 3, print B

    }
  }

  //separate current showing of sequence from the next showing
  printf("\n");

}

#define NAN1 8
#define NAN2 9

/* counts how many entries in seq2 match entries in seq1 */
/* returns exact and approximate matches, either both encoded in one value, */
/* or as a pointer to a pair of values */
int* countMatches(int *seq1, int *seq2) {


    int *arr = (int *)malloc(2 * sizeof(int));

    int correctCounter = 0; 
    //initialise correctCounter for exact matches    

    int closeCounter = 0;
    //initialise closeCounter for approximate matches    

    int pass_matched[3] = {0}; 
    //initialise array to represent positions of password that are matches

    int guess_matched[3] = {0};
    //initialise array to represent positions of guess that are matches

    //Get number of exact matches
    asm(
        "MOV r0, #0\n\t"          
        //Initialise i = 0 for loop
        "MOV r6, #0\n\t"          
        //Initialise registery to hold value to store in correctCounter
        "MOV r7, #1\n\t"          
        //Load 1 into r7

        "_loop:\n\t"               
        //Create loop section and Start loop
            "CMP r0, #3\n\t"          
            //Compare counter (i)/(r0) with 3
            "BEQ _end\n\t"            
            //If loop counter reaches 3, go to section "_end"
            "LDR r1, [%[guess], r0, LSL #2]\n\t" 
            //Load next element of guess into r1
            "LDR r2, [%[pass], r0, LSL #2]\n\t"  
            //Load next element of password into r2
            "CMP r1, r2\n\t"          
            //Check the element at the position r0 is set to
            "BEQ _exactMatch\n\t"     
            //If elements are equal, go to exactMatch
            "ADD r0, r0, #1\n\t"      
            //Increment i
            "B _loop\n\t"             
            //Go back and continue the loop

        "_exactMatch:\n\t"
        //section "_exactMatch" start
        "ADD r6, r6, #1\n\t"            
        //Increase correctCounter at an exact match
        "STR r7, [%[guess_matched], r0, LSL #2]\n\t" 
        //Store 1 in guess_matched array, representing a match at position r0
        "STR r7, [%[pass_matched], r0, LSL #2]\n\t"
        //Store 1 in pass_matched array, representing a match at position r0
        "ADD r0, r0, #1\n\t"            
        //Increment i
        "B _loop\n\t"                   
        //Go back and continue the loop

        "_end:\n\t"
        //section "_end" start
        "STR r6, %[correctCounter]\n\t" 
        //Store r6 into correctCounter
        : [correctCounter] "=m" (correctCounter)
        : [guess] "r" (seq1), [pass] "r" (seq2), [pass_matched] "r" (pass_matched), [guess_matched] "r" (guess_matched)
        : "r0", "r1", "r2", "r6", "r7"
    ); 

    //Get number of approximate matches
    asm (

        "MOV r0, #0\n\t" 
        //Initialise i = 0 for loop
        "MOV r1, #0\n\t" 
        //Initisalise j = 0 for nested loop
        "MOV r5, #0\n\t" 
        //Initialise registery to hold value to store in closeCounter
        "MOV r6, #1\n\t"
        //Load 1 into r6

        "_loopi:\n\t" 
        //Create loop section "_loopi" and start loop
            "CMP r0, #3\n\t"
            //Compare counter (i)/(r0) with 3
            "BEQ _end1\n\t" 
            //If loop counter reaches 3, go to section "_end"
            "LDR r7, [%[guess_matched], r0, LSL #2]\n\t"
            //Load next value of guess_matched array at index r0 into r7
            "CMP r7, #0\n\t"
            //Compare r7 with 0
            "BNE _nexti\n\t"
            //If 1, there is an exact match at that position, move to the next element of guess_matched, otherwise, continue
            "_loopj:\n\t"
            //Start nested loop
                "CMP r1, #3\n\t" 
                //Compare counter (j)/(r1) with 3
                "BEQ _endloop\n\t"
                //If loop counter reaches 3, go to section "_endloop"
                "LDR r2, [%[pass_matched], r1, LSL #2]\n\t" 
                //Load next element of pass_matched into r2
                "LDR r3, [%[guess], r0, LSL #2]\n\t"
                //Load next element of guess into r3
                "LDR r4, [%[pass], r1, LSL #2]\n\t"
                //Load next element of pass into r4
                "CMP r2, #0\n\t" 
                //Check if the element of pass_matched holds a value, meaning that at that position, an exact match is identified
                "BNE _skip\n\t"
                //We want for the elements of pass and guess to match, but to not have an exact match at position i, meaning that we check if elements match, whilst their positions must be different
                //If they are an exact match, meaning the elements match up at the same position, go to section "_skip"
                "CMP r3, r4\n\t"
                //Check if element of guess at position i/r0 is equal to the element of pass at position j/r1
                "BNE _skip\n\t"
                //If they are not equal, go to section "_skip", which increases counter j, moving on to the next element of pass
                "ADD r5, r5, #1\n\t"
                //Incremenet loop counter j of nested loop
                "STR r6, [%[pass_matched], r1, LSL #2]\n\t"
                //If all conditions previous have been satisfied, store 1 at position r1/j in pass_matched
                "STR r6, [%[guess_matched], r0, LSL #2]\n\t"
                //If all conditions previous have been satisfied, store 1 at position r0/i in guess_matched
                "MOV r1, #0\n\t"
                //Reset loop counter j/r1 to 0
                "ADD r0, r0, #1\n\t"
                //Increment loop counter i/r0 
                "B _loopi\n\t"
                //Go back to the outer loop, representing a break statement within the inner loop

        "_endloop:\n\t"
        "ADD r0, r0, #1\n\t"
        //If nested loop counter reaches 3, increment outer loop counter
        "MOV r1, #0\n\t"
        //If condition (r1 = 3) matched, increase outer loop and restart inner
        "B _loopi\n\t"
        //Go to outer loop

        "_skip:\n\t"
        "ADD r1, r1, #1\n\t"
        //If conditions are not satisfied within the inner loop, increment counter r1/j
        "B _loopj\n\t"
        //Start inner loop again

        "_nexti:\n\t"
        "ADD r0, r0, #1\n\t"
        //If guess_matched contains 1, increment outer loop counter r0/i
        "B _loopi\n\t"
        //Go back to the beginning of the loop

        "_end1:\n\t"
        "STR r5, %[closeCounter]\n\t"
        //Store r5 into closeCounter
        : [closeCounter] "=m" (closeCounter)
        : [pass_matched] "r" (pass_matched), [guess] "r" (seq1), [pass] "r" (seq2), [guess_matched] "r" (guess_matched)
        : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"
    );

        
    
    //Store exact matches and approximate matches in guesses array
    arr[0] = correctCounter;
    arr[1] = closeCounter;
    //return guesses array
    return arr;
  
}

/* show the results from calling countMatches on seq1 and seq2 */
void showMatches(int /* or int* */ *code, /* only for debugging */ int *seq1, int *seq2, /* optional, to control layout */ int lcd_format) {
  /* ***  COMPLETE the code here  ***  */
  printf("%d exact\n", code[0]);
  printf("%d approximate\n", code[1]);
}

/* parse an integer value as a list of digits, and put them into @seq@ */
/* needed for processing command-line with options -s or -u            */
void readSeq(int *seq, int val) {

  char *valST;
  //Initialise string to store the value of val
  valST = (char *)malloc(seqlen * sizeof(char));
  //allocate enough memory into string per the size of the sequence
  sprintf(valST, "%d", val);
  //copy value from int val into string valST
  
  //copy each character of valST into seq
  for (int i = 0; i < seqlen; i++) {
    seq[i] = valST[i] - '0';
    //subtract ASCII value 48 from characters to get the actual int value
  }

  free(valST);
}

/* read a guess sequence fron stdin and store the values in arr */
/* only needed for testing the game logic, without button input */
int* readNum(int max) {

    
  int *digits;
  int numDigits = 0;
  int temp = max;

  //Count the number of digits in max
  while (temp != 0) {
      temp /= 10;
      numDigits++;
  }

  //Allocate memory for the array of digits
  digits = (int*)malloc(numDigits * sizeof(int));

  // Extract each digit and store in the array in reverse order
  for (int i = numDigits - 1; i >= 0; i--) {
      digits[i] = max % 10;
      max /= 10;
  }

  return digits;
}


/* ======================================================= */
/* SECTION: TIMER code                                     */
/* ------------------------------------------------------- */
/* TIMER code */

/* timestamps needed to implement a time-out mechanism */
static uint64_t startT, stopT;

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* you may need this function in timer_handler() below  */
/* use the libc fct gettimeofday() to implement it      */
uint64_t timeInMicroseconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)tv.tv_usec); // in us
}

void timer_handler(int signum) {
  stopT = timeInMicroseconds();
  timerPL = 1;
}

void initITimer(uint64_t timeout) {
  struct sigaction signalAct;
  struct itimerval tempTime;

  memset(&signalAct, 0, sizeof(signalAct));
  signalAct.sa_handler = &timer_handler;

  sigaction(SIGALRM, &signalAct, NULL);

  tempTime.it_value.tv_sec = timeout / 1000000; // Convert microseconds to seconds
  tempTime.it_value.tv_usec = timeout % 1000000; // Remaining microseconds
  tempTime.it_interval.tv_sec = 0;
  tempTime.it_interval.tv_usec = 0;
  setitimer(ITIMER_REAL, &tempTime, NULL);

  startT = timeInMicroseconds();
}

/* ======================================================= */
/* SECTION: Aux function                                   */
/* ------------------------------------------------------- */
/* misc aux functions */

int failure (int fatal, const char *message, ...)
{
  va_list argp ;
  char buffer [1024] ;

  if (!fatal) //  && wiringPiReturnCodes)
    return -1 ;

  va_start (argp, message) ;
  vsnprintf (buffer, 1023, message, argp) ;
  va_end (argp) ;

  fprintf (stderr, "%s", buffer) ;
  exit (EXIT_FAILURE) ;

  return 0 ;
}

/*
 * waitForEnter:
 *********************************************************************************
 */

void waitForEnter (void)
{
  printf ("Press ENTER to continue: ") ;
  (void)fgetc (stdin) ;
}

/*
 * delay:
 *	Wait for some number of milliseconds
 *********************************************************************************
 */

void delay (unsigned int howLong)
{
  struct timespec sleeper, dummy ;

  sleeper.tv_sec  = (time_t)(howLong / 1000) ;
  sleeper.tv_nsec = (long)(howLong % 1000) * 1000000 ;

  nanosleep (&sleeper, &dummy) ;
}

/* From wiringPi code; comment by Gordon Henderson
 * delayMicroseconds:
 *	This is somewhat intersting. It seems that on the Pi, a single call
 *	to nanosleep takes some 80 to 130 microseconds anyway, so while
 *	obeying the standards (may take longer), it's not always what we
 *	want!
 *
 *	So what I'll do now is if the delay is less than 100uS we'll do it
 *	in a hard loop, watching a built-in counter on the ARM chip. This is
 *	somewhat sub-optimal in that it uses 100% CPU, something not an issue
 *	in a microcontroller, but under a multi-tasking, multi-user OS, it's
 *	wastefull, however we've no real choice )-:
 *
 *      Plan B: It seems all might not be well with that plan, so changing it
 *      to use gettimeofday () and poll on that instead...
 *********************************************************************************
 */

void delayMicroseconds (unsigned int howLong)
{
  struct timespec sleeper ;
  unsigned int uSecs = howLong % 1000000 ;
  unsigned int wSecs = howLong / 1000000 ;

  /**/ if (howLong ==   0)
    return ;
#if 0
  else if (howLong  < 100)
    delayMicrosecondsHard (howLong) ;
#endif
  else
  {
    sleeper.tv_sec  = wSecs ;
    sleeper.tv_nsec = (long)(uSecs * 1000L) ;
    nanosleep (&sleeper, NULL) ;
  }
}

/* ======================================================= */
/* SECTION: LCD functions                                  */
/* ------------------------------------------------------- */
/* medium-level interface functions (all in C) */

/* from wiringPi:
 * strobe:
 *	Toggle the strobe (Really the "E") pin to the device.
 *	According to the docs, data is latched on the falling edge.
 *********************************************************************************
 */

void strobe (const struct lcdDataStruct *lcd)
{

  // Note timing changes for new version of delayMicroseconds ()
  digitalWrite (gpio, lcd->strbPin, 1) ; delayMicroseconds (50) ;
  digitalWrite (gpio, lcd->strbPin, 0) ; delayMicroseconds (50) ;
}

/*
 * sentDataCmd:
 *	Send an data or command byte to the display.
 *********************************************************************************
 */

void sendDataCmd (const struct lcdDataStruct *lcd, unsigned char data)
{
  register unsigned char myData = data ;
  unsigned char          i, d4 ;

  if (lcd->bits == 4)
  {
    d4 = (myData >> 4) & 0x0F;
    for (i = 0 ; i < 4 ; ++i)
    {
      digitalWrite (gpio, lcd->dataPins [i], (d4 & 1)) ;
      d4 >>= 1 ;
    }
    strobe (lcd) ;

    d4 = myData & 0x0F ;
    for (i = 0 ; i < 4 ; ++i)
    {
      digitalWrite (gpio, lcd->dataPins [i], (d4 & 1)) ;
      d4 >>= 1 ;
    }
  }
  else
  {
    for (i = 0 ; i < 8 ; ++i)
    {
      digitalWrite (gpio, lcd->dataPins [i], (myData & 1)) ;
      myData >>= 1 ;
    }
  }
  strobe (lcd) ;
}

/*
 * lcdPutCommand:
 *	Send a command byte to the display
 *********************************************************************************
 */

void lcdPutCommand (const struct lcdDataStruct *lcd, unsigned char command)
{
#ifdef DEBUG
  fprintf(stderr, "lcdPutCommand: digitalWrite(%d,%d) and sendDataCmd(%d,%d)\n", lcd->rsPin,   0, lcd, command);
#endif
  digitalWrite (gpio, lcd->rsPin,   0) ;
  sendDataCmd  (lcd, command) ;
  delay (2) ;
}

void lcdPut4Command (const struct lcdDataStruct *lcd, unsigned char command)
{
  register unsigned char myCommand = command ;
  register unsigned char i ;

  digitalWrite (gpio, lcd->rsPin,   0) ;

  for (i = 0 ; i < 4 ; ++i)
  {
    digitalWrite (gpio, lcd->dataPins [i], (myCommand & 1)) ;
    myCommand >>= 1 ;
  }
  strobe (lcd) ;
}

/*
 * lcdHome: lcdClear:
 *	Home the cursor or clear the screen.
 *********************************************************************************
 */

void lcdHome (struct lcdDataStruct *lcd)
{
#ifdef DEBUG
  fprintf(stderr, "lcdHome: lcdPutCommand(%d,%d)\n", lcd, LCD_HOME);
#endif
  lcdPutCommand (lcd, LCD_HOME) ;
  lcd->cx = lcd->cy = 0 ;
  delay (5) ;
}

void lcdClear (struct lcdDataStruct *lcd)
{
#ifdef DEBUG
  fprintf(stderr, "lcdClear: lcdPutCommand(%d,%d) and lcdPutCommand(%d,%d)\n", lcd, LCD_CLEAR, lcd, LCD_HOME);
#endif
  lcdPutCommand (lcd, LCD_CLEAR) ;
  lcdPutCommand (lcd, LCD_HOME) ;
  lcd->cx = lcd->cy = 0 ;
  delay (5) ;
}

/*
 * lcdPosition:
 *	Update the position of the cursor on the display.
 *	Ignore invalid locations.
 *********************************************************************************
 */

void lcdPosition (struct lcdDataStruct *lcd, int x, int y)
{
  // struct lcdDataStruct *lcd = lcds [fd] ;

  if ((x > lcd->cols) || (x < 0))
    return ;
  if ((y > lcd->rows) || (y < 0))
    return ;

  lcdPutCommand (lcd, x + (LCD_DGRAM | (y>0 ? 0x40 : 0x00)  /* rowOff [y] */  )) ;

  lcd->cx = x ;
  lcd->cy = y ;
}



/*
 * lcdDisplay: lcdCursor: lcdCursorBlink:
 *	Turn the display, cursor, cursor blinking on/off
 *********************************************************************************
 */

void lcdDisplay (struct lcdDataStruct *lcd, int state)
{
  if (state)
    lcdControl |=  LCD_DISPLAY_CTRL ;
  else
    lcdControl &= ~LCD_DISPLAY_CTRL ;

  lcdPutCommand (lcd, LCD_CTRL | lcdControl) ; 
}

void lcdCursor (struct lcdDataStruct *lcd, int state)
{
  if (state)
    lcdControl |=  LCD_CURSOR_CTRL ;
  else
    lcdControl &= ~LCD_CURSOR_CTRL ;

  lcdPutCommand (lcd, LCD_CTRL | lcdControl) ; 
}

void lcdCursorBlink (struct lcdDataStruct *lcd, int state)
{
  if (state)
    lcdControl |=  LCD_BLINK_CTRL ;
  else
    lcdControl &= ~LCD_BLINK_CTRL ;

  lcdPutCommand (lcd, LCD_CTRL | lcdControl) ; 
}

/*
 * lcdPutchar:
 *	Send a data byte to be displayed on the display. We implement a very
 *	simple terminal here - with line wrapping, but no scrolling. Yet.
 *********************************************************************************
 */

void lcdPutchar (struct lcdDataStruct *lcd, unsigned char data)
{
  digitalWrite (gpio, lcd->rsPin, 1) ;
  sendDataCmd  (lcd, data) ;

  if (++lcd->cx == lcd->cols)
  {
    lcd->cx = 0 ;
    if (++lcd->cy == lcd->rows)
      lcd->cy = 0 ;
    
    // TODO: inline computation of address and eliminate rowOff
    lcdPutCommand (lcd, lcd->cx + (LCD_DGRAM | (lcd->cy>0 ? 0x40 : 0x00)   /* rowOff [lcd->cy] */  )) ;
  }
}


/*
 * lcdPuts:
 *	Send a string to be displayed on the display
 *********************************************************************************
 */

void lcdPuts (struct lcdDataStruct *lcd, const char *string)
{
  while (*string)
    lcdPutchar (lcd, *string++) ;
}

/* ======================================================= */
/* SECTION: aux functions for game logic                   */
/* ------------------------------------------------------- */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* --------------------------------------------------------------------------- */
/* interface on top of the low-level pin I/O code */

/* blink the led on pin @led@, @c@ times */
void blinkN(uint32_t *gpio, int led, int c) { 

  if (c == 0) {
    return;
  }

  //initialise counter
  int i = 0;

  //blink led c times
  while (i < c) {
    digitalWrite(gpio, led, HIGH);
    //Turn LED on
    sleep(1);
    //Wait for one second
    digitalWrite(gpio, led, LOW);
    //Turn LED off
    sleep(1);
    //Wait for one second
    i++;
    //Increment counter representing number of blinks so far
  }
}

//helper function to count button presses for a certain duration
int countButtonPresses(uint32_t *gpio, int button) {

  //counter and duration initialised
  int counter = 0;
  uint64_t duration = 3000000;
  //timer started
  initITimer(duration);

  while (!timerPL) {
    //increase counter when button is pressed
    waitForButton(gpio, button);
    //Once button is pressed, increase counter
    counter++;
    usleep(300000);
    
  }
  //reset timer
  timerPL = 0;

  return counter - 1;
}
/* ======================================================= */
/* SECTION: main fct                                       */
/* ------------------------------------------------------- */

int main (int argc, char *argv[])
{ // this is just a suggestion of some variable that you may want to use
  struct lcdDataStruct *lcd ;
  int bits, rows, cols ;
  unsigned char func ;

  int found = 0, attempts = 0, i, j, code;
  int c, d, buttonPressed, rel, foo;
  int *attSeq;

  int pinLED = LED, pin2LED2 = LED2, pinButton = BUTTON;
  int fSel, shift, pin,  clrOff, setOff, off, res;
  int fd ;

  int  exact, contained;
  char str1[32];
  char str2[32];
  
  struct timeval t1, t2 ;
  int t ;

  char buf [32] ;

  // variables for command-line processing
  char str_in[20], str[20] = "some text";
  int verbose = 0, debug = 0, help = 0, opt_m = 0, opt_n = 0, opt_s = 0, unit_test = 0, *res_matches;
  
  // -------------------------------------------------------
  // process command-line arguments

  // see: man 3 getopt for docu and an example of command line parsing
  { // see the CW spec for the intended meaning of these options
    int opt;
    while ((opt = getopt(argc, argv, "hvdus:")) != -1) {
      switch (opt) {
      case 'v':
	verbose = 1;
	break;
      case 'h':
	help = 1;
	break;
      case 'd':
	debug = 1;
	break;
      case 'u':
	unit_test = 1;
	break;
      case 's':
	opt_s = atoi(optarg); 
	break;
      default: /* '?' */
	fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u <seq1> <seq2>] [-s <secret seq>]  \n", argv[0]);
	exit(EXIT_FAILURE);
      }
    }
  }

  if (help) {
    fprintf(stderr, "MasterMind program, running on a Raspberry Pi, with connected LED, button and LCD display\n"); 
    fprintf(stderr, "Use the button for input of numbers. The LCD display will show the matches with the secret sequence.\n"); 
    fprintf(stderr, "For full specification of the program see: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf\n"); 
    fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u <seq1> <seq2>] [-s <secret seq>]  \n", argv[0]);
    exit(EXIT_SUCCESS);
  }
  
  if (unit_test && optind >= argc-1) {
    fprintf(stderr, "Expected 2 arguments after option -u\n");
    exit(EXIT_FAILURE);
  }

  if (verbose && unit_test) {
    printf("1st argument = %s\n", argv[optind]);
    printf("2nd argument = %s\n", argv[optind+1]);
  }

  if (verbose) {
    fprintf(stdout, "Settings for running the program\n");
    fprintf(stdout, "Verbose is %s\n", (verbose ? "ON" : "OFF"));
    fprintf(stdout, "Debug is %s\n", (debug ? "ON" : "OFF"));
    fprintf(stdout, "Unittest is %s\n", (unit_test ? "ON" : "OFF"));
    if (opt_s)  fprintf(stdout, "Secret sequence set to %d\n", opt_s);
  }

  seq1 = (int*)malloc(seqlen*sizeof(int));
  seq2 = (int*)malloc(seqlen*sizeof(int));
  cpy1 = (int*)malloc(seqlen*sizeof(int));
  cpy2 = (int*)malloc(seqlen*sizeof(int));

  // check for -u option, and if so run a unit test on the matching function
  if (unit_test && argc > optind+1) { // more arguments to process; only needed with -u 
    strcpy(str_in, argv[optind]);
    opt_m = atoi(str_in);
    strcpy(str_in, argv[optind+1]);
    opt_n = atoi(str_in);
    // CALL a test-matches function; see testm.c for an example implementation
    readSeq(seq1, opt_m); // turn the integer number into a sequence of numbers
    readSeq(seq2, opt_n); // turn the integer number into a sequence of numbers
    if (verbose)
      fprintf(stdout, "Testing matches function with sequences %d and %d\n", opt_m, opt_n);
    res_matches = countMatches(seq1, seq2);
    showMatches(res_matches, seq1, seq2, 1);
    exit(EXIT_SUCCESS);
  } else {
    /* nothing to do here; just continue with the rest of the main fct */
  }

  if (opt_s) { // if -s option is given, use the sequence as secret sequence
    if (theSeq==NULL)
      theSeq = (int*)malloc(seqlen*sizeof(int));
    readSeq(theSeq, opt_s);
    if (verbose) {
      fprintf(stderr, "Running program with secret sequence:\n");
      showSeq(theSeq);
    }
  }
  
  // -------------------------------------------------------
  // LCD constants, hard-coded: 16x2 display, using a 4-bit connection
  bits = 4; 
  cols = 16; 
  rows = 2; 
  // -------------------------------------------------------

  printf ("Raspberry Pi LCD driver, for a %dx%d display (%d-bit wiring) \n", cols, rows, bits) ;

  if (geteuid () != 0)
    fprintf (stderr, "setup: Must be root. (Did you forget sudo?)\n") ;

  // init of guess sequence, and copies (for use in countMatches)
  attSeq = (int*) malloc(seqlen*sizeof(int));
  cpy1 = (int*)malloc(seqlen*sizeof(int));
  cpy2 = (int*)malloc(seqlen*sizeof(int));

  // -----------------------------------------------------------------------------
  // constants for RPi2
  gpiobase = 0x3F200000 ;

  // -----------------------------------------------------------------------------
  // memory mapping 
  // Open the master /dev/memory device

  if ((fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC) ) < 0)
    return failure (FALSE, "setup: Unable to open /dev/mem: %s\n", strerror (errno)) ;

  // GPIO:
  gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, gpiobase) ;
  if ((int32_t)gpio == -1)
    return failure (FALSE, "setup: mmap (GPIO) failed: %s\n", strerror (errno)) ;

  // -------------------------------------------------------
  // Configuration of LED and BUTTON

  /* ***  COMPLETE the code here  ***  */
  
  // -------------------------------------------------------
  // INLINED version of lcdInit (can only deal with one LCD attached to the RPi):
  // you can use this code as-is, but you need to implement digitalWrite() and
  // pinMode() which are called from this code
  // Create a new LCD:
  lcd = (struct lcdDataStruct *)malloc (sizeof (struct lcdDataStruct)) ;
  if (lcd == NULL)
    return -1 ;

  // hard-wired GPIO pins
  lcd->rsPin   = RS_PIN ;
  lcd->strbPin = STRB_PIN ;
  lcd->bits    = 4 ;
  lcd->rows    = rows ;  // # of rows on the display
  lcd->cols    = cols ;  // # of cols on the display
  lcd->cx      = 0 ;     // x-pos of cursor
  lcd->cy      = 0 ;     // y-pos of curosr

  lcd->dataPins [0] = DATA0_PIN ;
  lcd->dataPins [1] = DATA1_PIN ;
  lcd->dataPins [2] = DATA2_PIN ;
  lcd->dataPins [3] = DATA3_PIN ;
  // lcd->dataPins [4] = d4 ;
  // lcd->dataPins [5] = d5 ;
  // lcd->dataPins [6] = d6 ;
  // lcd->dataPins [7] = d7 ;

  // lcds [lcdFd] = lcd ;

  digitalWrite (gpio, lcd->rsPin,   0) ; pinMode (gpio, lcd->rsPin,   OUTPUT) ;
  digitalWrite (gpio, lcd->strbPin, 0) ; pinMode (gpio, lcd->strbPin, OUTPUT) ;

  for (i = 0 ; i < bits ; ++i)
  {
    digitalWrite (gpio, lcd->dataPins [i], 0) ;
    pinMode      (gpio, lcd->dataPins [i], OUTPUT) ;
  }
  delay (35) ; // mS

// Gordon Henderson's explanation of this part of the init code (from wiringPi):
// 4-bit mode?
//	OK. This is a PIG and it's not at all obvious from the documentation I had,
//	so I guess some others have worked through either with better documentation
//	or more trial and error... Anyway here goes:
//
//	It seems that the controller needs to see the FUNC command at least 3 times
//	consecutively - in 8-bit mode. If you're only using 8-bit mode, then it appears
//	that you can get away with one func-set, however I'd not rely on it...
//
//	So to set 4-bit mode, you need to send the commands one nibble at a time,
//	the same three times, but send the command to set it into 8-bit mode those
//	three times, then send a final 4th command to set it into 4-bit mode, and only
//	then can you flip the switch for the rest of the library to work in 4-bit
//	mode which sends the commands as 2 x 4-bit values.

  if (bits == 4)
  {
    func = LCD_FUNC | LCD_FUNC_DL ;			// Set 8-bit mode 3 times
    lcdPut4Command (lcd, func >> 4) ; delay (35) ;
    lcdPut4Command (lcd, func >> 4) ; delay (35) ;
    lcdPut4Command (lcd, func >> 4) ; delay (35) ;
    func = LCD_FUNC ;					// 4th set: 4-bit mode
    lcdPut4Command (lcd, func >> 4) ; delay (35) ;
    lcd->bits = 4 ;
  }
  else
  {
    failure(TRUE, "setup: only 4-bit connection supported\n");
    func = LCD_FUNC | LCD_FUNC_DL ;
    lcdPutCommand  (lcd, func     ) ; delay (35) ;
    lcdPutCommand  (lcd, func     ) ; delay (35) ;
    lcdPutCommand  (lcd, func     ) ; delay (35) ;
  }

  if (lcd->rows > 1)
  {
    func |= LCD_FUNC_N ;
    lcdPutCommand (lcd, func) ; delay (35) ;
  }

  // Rest of the initialisation sequence
  lcdDisplay     (lcd, TRUE) ;
  lcdCursor      (lcd, FALSE) ;
  lcdCursorBlink (lcd, FALSE) ;
  lcdClear       (lcd) ;

  lcdPutCommand (lcd, LCD_ENTRY   | LCD_ENTRY_ID) ;    // set entry mode to increment address counter after write
  lcdPutCommand (lcd, LCD_CDSHIFT | LCD_CDSHIFT_RL) ;  // set display shift to right-to-left

  // END lcdInit ------
  // -----------------------------------------------------------------------------
  // Start of game

  fprintf(stderr,"Printing welcome message on the LCD display ...\n");
  char *welcome = "Welcome to       ";
  char *master = "Mastermind!";
  lcdPuts(lcd, welcome);
  lcdPuts(lcd, master);
  sleep(2);
  lcdClear(lcd);

  /* initialise the secret sequence */
  if (!opt_s)
    initSeq();
  if (debug)
    showSeq(theSeq);

  
  lcdClear(lcd);
  pinMode(gpio, pinLED, OUTPUT);
  pinMode(gpio, pin2LED2, OUTPUT);
  digitalWrite(gpio, pinLED, LOW);
  digitalWrite(gpio, pin2LED2, LOW);
  char *strt = "Press enter to   start!";
  // optionally one of these 2 calls:
  lcdPuts(lcd, strt);
  waitForEnter(); 
  lcdClear(lcd);
  // waitForButton (gpio, pinButton) ;

  // -----------------------------------------------------------------------------
  // +++++ main loop
  while (!found) {
    attempts++;
    //increase number of attempts

    lcdClear(lcd);
    char disp[9];
    sprintf(disp, "Round: %d", attempts);
    lcdPuts(lcd, disp);
    sleep(2);
    lcdClear(lcd);

    char *st = "Start Pressing!";
    lcdPuts(lcd, st);

    for (int i = 0; i < seqlen; i++) {


      attSeq[i] = countButtonPresses(gpio, pinButton);
      lcdClear(lcd);
      //assign first input to first element of attSeq
      blinkN(gpio, pin2LED2, 1);
      //blink red once to signal end of first input
      printf("Number of presses: %d\n", attSeq[i]);
      blinkN(gpio, pinLED, attSeq[i]);
      //blink green number according to input
      
    }
    
    blinkN(gpio, pin2LED2, 2);
    //blink red twice to signal end of inputs
    int *results1 = countMatches(attSeq, theSeq);
    //get exact and approximate matches in results1
    blinkN(gpio, pinLED, results1[0]);
    //blink green number of exact matches
    blinkN(gpio, pin2LED2, 1);
    //blink red as a separator
    blinkN(gpio, pinLED, results1[1]);
    //blink green number of approximate matches
    blinkN(gpio, pin2LED2, 3);
    //blink red to represent the end of the round

    lcdClear(lcd);
    sprintf(disp, "Exact matches: %d", results1[0]);
    lcdPuts(lcd, disp);
    sleep(1);
    lcdClear(lcd);
    sprintf(disp, "Appr matches: %d", results1[1]);
    lcdPuts(lcd, disp);
    sleep(1);

    //if guess matches exactly with the password, make found = true, and break out of the loop
    if (results1[0] == 3) {
      found = 1;
      goto next;
    }
    /* ******************************************************* */
    /* ***  COMPLETE the code here  ***                        */
    /* this needs to implement the main loop of the game:      */
    /* check for button presses and count them                 */
    /* store the input numbers in the sequence @attSeq@        */
    /* compute the match with the secret sequence, and         */
    /* show the result                                         */
    /* see CW spec for details                                 */
    /* ******************************************************* */
  }

  next:
  if (found) {
    lcdClear(lcd);
    char *success = "SUCCESS!";
    lcdPuts(lcd, success);
    // Move cursor to the beginning of the second line
    lcdPuts(lcd, "Attempts: ");
    // Now you can print the value of attempts
    char disp[9]; // Assuming attempts fits within 9 characters
    sprintf(disp, "%d", attempts);
    lcdPuts(lcd, disp);
    sleep(3);
    lcdClear(lcd);
} else {
    fprintf(stdout, "Sequence not found\n");
}
return 0;

}