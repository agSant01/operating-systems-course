#include <stdio.h>
#include <mycalllib.h>

int main(){

	int c=4, pid=0, deadline = 0, i;
	for(i = 0; i < c; i++){

		pid = fork();
		if(pid == 0){
			deadline = 100-i*10;
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
