#include <stdio.h>
#include <ncurses.h>

int main() {
	WINDOW *main;
	int i,j;

	main = initscr();
	scrollok(main, TRUE);
	refresh();
	getmaxyx(main, j, i);

	move(j - 1, 0);
	addstr("2nd to last\nlast - if this is visible, the screen scrolled up");
	refresh();
	getchar();
	endwin();
	return 0;
}
