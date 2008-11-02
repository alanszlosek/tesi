#include <stdio.h>
#include <ncurses.h>
#include "tesi.h"

#define DEBUG


// CALLBACKS
void cbMoveCursor(void *pointer, int x, int y) {
#ifdef DEBUG
	fprintf(stderr, "cbMoveCursor: %d %d\n", x, y);
#endif
	wmove(pointer, y, x);
}

void cbPrintCharacter(void *pointer, char ch, int x, int y) {
#ifdef DEBUG
	//fprintf(stderr, "cbPrintCharacter: %c (%d)\n", ch, (int)ch);
#endif
	waddch(pointer, ch);
}

void cbEraseCharacter(void *pointer, int x, int y) {
#ifdef DEBUG
	fprintf(stderr, "cbEraseCharacter\n");
#endif
	wdelch(pointer);
}

void cbScrollUp(void *pointer) {
#ifdef DEBUG
	fprintf(stderr, "cbScrollUp\n");
#endif
	//scrollok(pointer, true);
	if(wscrl(pointer, 1) == ERR)
		fprintf(stderr, "Scroll error\n");
	//scrollok(pointer, false);
}
void cbScrollDown(void *pointer) {
#ifdef DEBUG
	fprintf(stderr, "cbScrollDown\n");
#endif
	//scrollok(pointer, true);
	if(wscrl(pointer, -1) == ERR)
		fprintf(stderr, "Scroll error\n");
	//scrollok(pointer, false);
}
void cbScrollRegion(void *pointer, int start, int end) {
#ifdef DEBUG
	fprintf(stderr, "cbScrollRegion\n");
#endif
	wsetscrreg(pointer, start, end);
}

void cbInsertLine(void *pointer, int y) {
#ifdef DEBUG
	fprintf(stderr, "cbInsertLine\n");
#endif
	winsertln(pointer);
}

void cbEraseLine(void *pointer, int y) {
#ifdef DEBUG
	fprintf(stderr, "cbEraseLine\n");
#endif
	wclrtoeol(pointer);
}

void cbBell(struct tesiObject *to) {
#ifdef DEBUG
	fprintf(stderr, "cbBell\n");
#endif
}

void cbAttributes(void *pointer, short bold, short blink, short inverse, short underline, short foreground, short background, short charset) {
	if(bold)
		wattron(pointer, A_BOLD);
	else
		wattroff(pointer, A_BOLD);
	if(inverse)
		wattron(pointer, A_STANDOUT);
	else
		wattroff(pointer, A_STANDOUT);
	if(foreground && background) {
		init_pair(7, foreground, background);
		wattron(pointer, COLOR_PAIR(7));
	}
}

int main(int argc, char **argv) {
	struct tesiObject *to;
	//char input[128],run[128];
	fd_set fds;
	int i, j, maxFd, size;
	int input;
	char ch;
	char *keySequence = NULL;
	struct timespec ts;
	WINDOW *wScreen;

	wScreen = initscr();
	noecho();
	cbreak();
	//raw();
	//
	keypad(wScreen, true);

	start_color();
	init_pair(1, COLOR_BLUE, COLOR_WHITE);
	attron(COLOR_PAIR(1));
	bkgd(' ' | COLOR_PAIR(1));

	/*
	if(scrollok(wScreen, true) == ERR)
		fprintf(stderr, "scrollok error\n");
	*/
	scrollok(wScreen, true);
	refresh();

	getmaxyx(wScreen, j, i);

 	to = newTesiObject("/bin/bash", i, j);
 	//to = newTesiObject("./test_suite/loop", i, j);
	to->pointer = wScreen;

	maxFd = to->fd_activity;
	if(to->fd_activity > maxFd)
		maxFd = to->fd_activity;

	ts.tv_sec = 0;
	ts.tv_nsec = 100000;

	to->callback_moveCursor = &cbMoveCursor;
	to->callback_printCharacter = &cbPrintCharacter;
	to->callback_eraseCharacter = &cbEraseCharacter;
	to->callback_scrollRegion = &cbScrollRegion;
	to->callback_scrollUp = &cbScrollUp;
	to->callback_scrollDown = &cbScrollDown;
	to->callback_insertLine = &cbInsertLine;
	to->callback_eraseLine = &cbEraseLine;
	to->callback_attributes = &cbAttributes;
	/*
	to->callback_scrollDown = &cbScrollDown;
	to->callback_bell = &cbBell;
	*/

	//write(to->pipeToChild[1], "env\nls\n",10);
	//strcpy(run, "/home/alan/coding/_testing/c/ncurses/ncurses2\n");
	//strcpy(run, "vim\n");
	//strcpy(run, "echo $TERM\n");
	//strcpy(run, "/home/alan/coding/_testing/c/beep\n");

	//write(to->pipeToChild[1], run, strlen(run));

	input = 0;
	while(input != '~') {
		FD_ZERO(&fds);
		FD_SET(0, &fds); // watch for keyboard input
		FD_SET(to->fd_activity, &fds); // watch for TESI output

		pselect(maxFd + 1, &fds, NULL, NULL, &ts, NULL);
		// see if TESI has data from it's child process waiting to be handled
		// if so, tell it to process it
		if(FD_ISSET(to->fd_activity, &fds)) {
			tesi_handleInput(to);
			// update the screen afterwards
			wrefresh(to->pointer);
		}

		if(FD_ISSET(0, &fds)) {
			input = wgetch(wScreen);
			//fprintf(stderr, "pressed: %c (%d)\n", input, input);
			//read(0, &in, 1);
			switch(input) {
				// unfortunately I don't know of any automated way to ask curses for the KEY_UP escape sequence for a foreign terminal
				// so we have to hard-code these values for now
				case KEY_UP: // send tesi sequence for keyup
					keySequence = "\x1Bk";
					write(to->fd_input, keySequence, 2);
					break;
				case KEY_DOWN:
					keySequence = "\x1Bj";
					write(to->fd_input, keySequence, 2);
					break;
				case KEY_LEFT:
					keySequence = "\x1Bh";
					write(to->fd_input, keySequence, 2);
					break;
				case KEY_RIGHT:
					keySequence = "\x1Bl";
					write(to->fd_input, keySequence, 2);
					break;
				case KEY_BACKSPACE:
					keySequence = "\x1Bj";
					write(to->fd_input, keySequence, 2);
					break;
					break;
				case KEY_ENTER:
					fprintf(stderr, "Enter key pressed\n");
					input = 10;
					write(to->fd_input, &ch, 1);
					break;
				default:
					// problem is, has_key detects for the current terminal
					// irrelevant for the attached tesi term
					/*
					if(has_key(input)) {
						fprintf(stderr, "key exists\n");

						keySequence = keybound(input, 0);
						if(keySequence)  {
							write(to->fd_input, &keySequence, strlen(keySequence));
							free(keySequence);
						}
					
					} else {
					*/
						fprintf(stderr, "key does not exist (%d)\n", input);
						ch = (char)input;
						write(to->fd_input, &ch, 1);
					//}
					break;
			}
			/*
			if(in == ('H' - '@'))
				fprintf(stderr, "backspace\n");
			if(in == '@')
				write(to->fd_input, longString, strlen(longString));
			else

			//write(to->pipeFromChild[1], &in, 1);
			write(to->fd_input, &in, 1);
			*/
		}
	}

	kill(to->pid, 9);
	waitpid(to->pid, NULL, 0);

	deleteTesiObject(to);
	endwin();
	return 0;
}
