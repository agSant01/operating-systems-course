#include <stdio.h>
#include <mycalllib.h>

int main(){

	int c=4, pid=0, deadline = 0, i;

	time_t t;
	srand((unsigned) time(&t));

	for(i = 0; i < c; i++){

		deadline = (rand() % 7) + 3;
		deadline*=10;

		pid = fork();
		if(pid == 0){
			mycall(deadline);
			sleep(1);
			break;

		}
	}
	if(pid == 0){
		printf("This is child with deadline %d\n",deadline);
	}
	return 0;
}

