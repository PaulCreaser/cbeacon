#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "cbeacon.h"

/*********************************************************************************************************************************/

static long long int start_time=0;
static int other_count = 0;

/**
* @brief 
*/
inline static void set_start_time()
{
  struct timeval start;
  gettimeofday(&start, NULL);
  start_time = (start.tv_sec)*1000000 + start.tv_usec;
}

/**
* @brief 
*
* @return 
*/
inline static long long int get_average_time()
{
	other_count++;
	long long int current_time;
	struct timeval current;
  	gettimeofday(&current, NULL);
    current_time = (current.tv_sec)*1000000 + current.tv_usec;
	return (current_time-start_time)/other_count; 
}

/**
* @brief :w
*
*
* @param data
* @param len
*/
static void callback3(int8_t *data, int len) {
}

/**
* @brief 
*
* @param argc
* @param argv
*
* @return 
*/
int main(int argc, char **argv)
{
  set_start_time();
  if (cbeacon_init() == -1 ) {
        return -1;
  }
  cbeacon_setcb(&callback3);
  if (cbeacon_start(false) == -1 ) {
        return -1;
  }
  cbeacon_stop();
  return 0;
}
