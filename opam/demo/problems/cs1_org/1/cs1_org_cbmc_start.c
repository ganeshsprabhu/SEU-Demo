#include <stdio.h>
#include <stdbool.h>
#include "/home/opam/demo/simulate_seu.h"
#include "/home/opam/demo/queue.h"

int p(int x, int y) {
	int output = 4;
	bool alarm = false;
	int count = 0;
	while (count < 7) {
		if (x > 10) {
			if (y == 1) {
				output = 2;
			} else {
			    output = 1;
		    }
		} else {
			output = output + 1;
		    alarm = true;
		}
		count++;
	}
	printf("alarm = %d\n", alarm);
	return output;
}

int main() {
    Queue q1;
    initQueue(&q1);
    Queue q2;
    initQueue(&q2);


	int output, x, y;

	int i=0;
	while(i<10)
	{
		output = p(x, y); // OriginalProgram
		i++;
	}
	return 0; 
}


// ----- Renamed Instrumented Function -----

int p_prime(int x , int y )
{
  int output ;
  int count ;
  {
  output = 4;
  count = 0;
  while (count < 7) {
    if (x > 10) {
      if (y == 1) {
        simulate_seu_main(& output);
        output = 2;
      } else {
        simulate_seu_main(& output);
        output = 1;
      }
    } else {
      simulate_seu_main(& output);
      output ++;
    }
    count ++;
  }
  {
  simulate_seu_main(& output);
  return (output);
  }
}
}
