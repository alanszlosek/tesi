#include <stdio.h>

int main() {
	int i, j, lines;
	char *s;
	char esc = 0x1B;
	lines = 48; //atoi(getenv("LINES"));
	printf("%cC", esc);
	sleep(2);
	j = lines - 1;
	for(i = 0; i < lines; i++) {
		printf("%d", i);
		if(i < j)
			printf("\n");
	}
	fflush(NULL);
	sleep(2);
	printf("%cU", esc);
	printf("%cU", esc);
	fflush(NULL);
	sleep(2);
	printf("%cD", esc);
	printf("%cD", esc);
	fflush(NULL);
	sleep(2);
	printf("\n");
	return 0;
}
