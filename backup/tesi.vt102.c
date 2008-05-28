#include "tesi.h"
// tesInterpreter
/*
Interprets DEC vt102 console escape sequences.
*/

#define DEBUG 1

/*
handleInput
	- reads data from file descriptor into buffer
	-

interpretSequence
	- once sequence has been completely read from buffer, it's passed here
	- it is interpreted and makes calls to appropriate callbacks

*/

// needs to be renamed, prefixed with tesi_
void tesi_handleInput(struct tesiObject *to) {
	char input[64];
	char *pointer;
	int lengthRead;
	int i,j;
	FILE *f;

	lengthRead = read(to->ptyMaster, input, 64);

	f = fopen("output", "a+");
	fwrite(input, lengthRead, 1, f);
	fclose(f);

	pointer = input;
	for(i = 0; i < lengthRead; i++, pointer++) {
		if(*pointer == 0) // skip NULL for unicode?
			continue;

		if((*pointer >= 1 && *pointer <= 31) || *pointer == 127) {
			tesi_handleControlCharacter(to, *pointer);
			continue;
		}

		if(to->partialSequence) {
			// keep track of the sequence type. some
			if(to->sequenceLength == 0) {
				if(*pointer == '[')
					to->sequenceType = 0;
				if(*pointer == '(')
					to->sequenceType = 1;
				if(*pointer == ')')
					to->sequenceType = 2;
			}
			to->sequence[ to->sequenceLength++ ] = *pointer;
			to->sequence[ to->sequenceLength ] = 0;
			if(
				(*pointer >= 'a' && *pointer <= 'z') ||
				(*pointer >= 'A' && *pointer <= 'Z') ||
				*pointer == '@' ||
				*pointer == '`' ||
				*pointer == '=' ||
				*pointer == '>'
			) // done with sequence
				to->partialSequence = 0;

			if(to->sequenceLength > 1 && to->sequenceType > 0)
				to->partialSequence = 0;

			//Might as well try to parse parameters right here

			if(to->partialSequence == 0) {
#ifdef DEBUG
				fprintf(stderr, "Sequence: %s\n", to->sequence);
#endif

				tesi_interpretSequence(to);
				to->sequenceLength = 0;
				to->parametersLength = 0;
			}
		} else { // standard text, not part of any escape sequence
			if(to->partialSequence == 0) {
				// watch out for newlines and such
				if(to->insertMode == 0 && to->callback_printCharacter) {
					to->callback_printCharacter(to->pointer, *pointer); // + to->alternativeChar);
					to->x++;
					tesi_limitCursor(to);
				}
				if(to->insertMode == 1 && to->callback_insertCharacter) {
					to->callback_insertCharacter(to->pointer, *pointer); // + to->alternativeChar);
				}
			}
		}
	}
}

/*
 * tesi_handleControlCharacter
 * Handles carriage return, newline (line feed), backspace, tab and others
 * Also starts a new escape sequence when ASCII 27 is read
 * */
int tesi_handleControlCharacter(struct tesiObject *to, char c) {
	int i;
	// entire switch from Rote
	switch (c) {
		case '\r': // carriage return ('M' - '@')
			to->x = 0;
#ifdef DEBUG
			fprintf(stderr, "Carriage return\n");
#endif
			if(to->callback_moveCursor)
				to->callback_moveCursor(to->pointer, to->x, to->y);
			break;
		case '\n':  // line feed, cud1 down one line ('J' - '@')
			to->y++;
			//if(to->insertMode == 0 && to->linefeedMode == 1)
				to->x = 0;
#ifdef DEBUG
			fprintf(stderr, "Newline\n");
#endif
			tesi_limitCursor(to);
			if(to->callback_moveCursor)
				to->callback_moveCursor(to->pointer, to->x, to->y);
			break;
	 	case 8: // backspace cub1 cursor back 1 ('H' - '@')
	 		// what do i do about wrapping back up to previous line?
	 		// where should that be handled
	 		// just move cursor, don't print space
			if (to->x > 0)
				to->x--;
			tesi_limitCursor(to);
			if(to->callback_moveCursor)
				to->callback_moveCursor(to->pointer, to->x, to->y);
			if(to->callback_eraseCharacter)
				to->callback_eraseCharacter(to->pointer);
			break;
		case '\t': // ht - horizontal tab, ('I' - '@')
#ifdef DEBUG
			fprintf(stderr, "Tab\n");
#endif
			for(i = 0; i < (to->x % 8) + 8; i++, to->x++) {
				if(tesi_limitCursor(to))
					break;
				if(to->callback_moveCursor)
					to->callback_moveCursor(to->pointer, to->x, to->y);
				if(to->callback_printCharacter)
					to->callback_printCharacter(to->pointer, ' ');
			}
	 		break;

		case '\x1B': // begin escape sequence (aborting previous one if any)
			to->partialSequence = 1;
			// possibly flush buffer
			break;

		case ('N' - '@'): // start alternate character set
			//to->alternativeChar = 110;
#ifdef DEBUG
			fprintf(stderr, "Alternative character set on\n");
#endif
			break;
		case ('O' - '@'): // end alternate character set
			to->alternativeChar = 0;
#ifdef DEBUG
			fprintf(stderr, "Alternative character set off\n");
#endif
			break;

		case '\a': // bell ('G' - '@')
	 		// do nothing for now... maybe a visual bell would be nice?
			if(to->callback_bell)
				to->callback_bell(to->pointer);
			break;

		default:
#ifdef DEBUG
			fprintf(stderr, "Unrecognized control char: %d (^%c)\n", c, c + '@');
#endif
			return false;
			break;
	}
	return true;
}

/*
 * This skips the [ present in most sequences
 */
void tesi_interpretSequence(struct tesiObject *to) {
	char *p = (to->sequence+1);
	char *q; // another pointer
	char *secondChar = to->sequence; // used to test for ? after [
	char operation = to->sequence[to->sequenceLength - 1];
	int firstParam, secondParam, previousLength;
	int i,j;

	firstParam = secondParam = 0;

#ifdef DEBUG
	//fprintf(stderr, "Operation: %c\n", operation);
#endif

	// preliminary reset of parameters
	for(i=0; i<6; i++)
		to->parameters[i] = 0;
	to->parametersLength = 0;

	if(*secondChar == '?')
		p++;

	// parse numeric parameters
	q = p;
	while ((*p >= '0' && *p <= '9') || *p == ';') {
		if (*p == ';') {
			//if (to->parametersLength >= MAX_CSI_ES_PARAMS)
				//return;
			to->parametersLength++;
		} else {
			j = to->parameters[ to->parametersLength ];
			j = j * 10;
			j += *p - '0';
			to->parameters[ to->parametersLength ] = j;
		}
		p++;
	}
	if(p != q)
		to->parametersLength++;

	/*
	// not all have 1 added to them, so can't negate all
	for(i = 0; i < to->parametersLength; i++) {
		fprintf(stderr, "param: %d\n", to->parameters[i]);
	}
	*/

	// premature opt?
	if(to->parametersLength > 0)
		firstParam = to->parameters[0];

	if(to->parametersLength > 1)
		secondParam = to->parameters[1];


	switch (operation) {
 		case 'm': // it's a 'set attribute' sequence
#ifdef DEBUG
				fprintf(stderr, "Set attributes\n");
#endif
			tesi_processAttributes(to);
			break;
		case '=':
		case '>':
			break;
		case '0': // last part of enable alternate character set
			break;
	}

	// CURSOR RELATED
	/*
	a-e, A-G, `
	ADF = -firstParam
	BeCaEG`d = positive firstParam
	*/
	if( (operation >= 'A' && operation <= 'H') || (operation >= 'a' && operation <= 'h')) {
		switch(operation) {
			case 'A': // cuu. up # lines and cuu1
				if(to->parametersLength == 0)
					to->y--;
				else
					to->y -= to->parameters[0];
				break;
			case 'B': // cud. down # of lines
			//case 'e':
				if(to->sequenceType == 0)
					to->y += to->parameters[0];
				else { // first part of alternate character set on
				}
				break;
			case 'C': // cuf. # to right and cuf1
			//case 'a':
				if(to->parametersLength == 0)
					to->x++;
				else
					to->x += to->parameters[0];
				break;
			case 'D': // cub. # to left
				to->x -= firstParam; // firstParam is negative by default, so add it
				break;
			/* not present for vt102
			case 'd':
				to->y = firstParam - 1;
				break;
			case 'E': // ???
				to->x += firstParam;
				to->y = 0;
				break;
			case 'F':
				to->x -= firstParam;
				to->y = 0;
				break;
			*/
			case 'g': // tbc clear tab stops
				if(firstParam == 3) {
				}
				break;
			case 'G': // horizontal position
				to->x = to->parameters[0];
				break;

			case 'H': // cup. move to col, row, cursor to home
				// parameters should be 1 more than the real value
				if(to->parametersLength == 0)
					to->x = to->y = 0;
				else {
					to->x = to->parameters[1] - 1;
					to->y = to->parameters[0] - 1;
				}
#ifdef DEBUG
				fprintf(stderr, "Move cursor (x,y): %d %d\n", to->x, to->y);
#endif
				break;

			case 'h': // enter specified modes
				for(i = 0; i < to->parametersLength; i++) {
					switch(to->parameters[i]) {
						case 1:
							to->cursorKeyMode = 1;
#ifdef DEBUG
							fprintf(stderr, "Enter cursor key mode\n");
#endif
							break;
						case 4: // insert mode or smooth scroll mode
							if(*secondChar != '?') // make sure not setting smooth scroll mode
								to->insertMode = 1;

#ifdef DEBUG
							fprintf(stderr, "Enter insert mode\n");
#endif
							break;
						case 6: // origin mode. on - home is top-left of scrolling region. off - home is screen
							break;
						case 7:
							if(*secondChar == '?') {
								// autowrap mode on
							}
							break;
						case 20: // linefeed mode
							to->linefeedMode = 1;
#ifdef DEBUG
							fprintf(stderr, "Enter linefeed mode\n");
#endif
							break;
					}
				}
				break;

			case '7': // sc save cursor
				to->x2 = to->x;
				to->y2 = to->y;
				break;
			case '8': // rc restore cursor
				to->x = to->x2;
				to->y = to->y2;
				break;
		}
		// limit cursor to boundaries
		tesi_limitCursor(to);
		if(to->callback_moveCursor)
			to->callback_moveCursor(to->pointer, to->x, to->y);
	}

	// LINE RELATED
	/*
	r-u, K-X
	*/
	if( (operation >= 'J' && operation <= 'X') || (operation >= 'r' && operation <= 'u') || operation == '@') {
		switch(operation) {
			case 'J':  // ed. 'erase display' sequence. actually, clear to end of screen from current position
#ifdef DEBUG
				fprintf(stderr, "Clear to end of screen\n");
#endif

				for(i = 0; i < to->height; i++) {
					if(to->callback_moveCursor)
						to->callback_moveCursor(to->pointer, 0, i);
					if(to->callback_eraseLine)
						to->callback_eraseLine(to->pointer);
				}
				to->x = to->y = 0;
				if(to->callback_moveCursor)
						to->callback_moveCursor(to->pointer, to->x, to->y);
				break;

	 		case 'K': // erase line
#ifdef DEBUG
				fprintf(stderr, "Erase line\n");
#endif
				/*
				if(to->callback_moveCursor)
					to->callback_moveCursor(to->pointer, 0, to->y);
				*/
				if(to->callback_eraseLine)
					to->callback_eraseLine(to->pointer);
				/*
				if(to->callback_moveCursor)
					to->callback_moveCursor(to->pointer, to->x, to->y);
				*/
				/*
				switch (firstParam) {
					case 1:
						erase_start = 0;
						erase_end = rt->ccol;
						break;
					case 2:
						erase_start = 0;
						erase_end = rt->cols - 1;
						break;
					default:
						erase_start = rt->ccol;
						erase_end = rt->cols - 1;
						break;
				}
				*/
				break;
			case 'L': // insert line
				if(to->callback_insertLine)
					to->callback_insertLine(to->pointer);
				break;

			case 'l': // exit insert mode
				for(i = 0; i < to->parametersLength; i++) {
					switch(to->parameters[i]) {
						case 1:
							to->cursorKeyMode = 0;
#ifdef DEBUG
							fprintf(stderr, "Exit cursor key mode\n");
#endif
							break;
						case 4:
							if(*secondChar != '?')
								to->insertMode = 0;
#ifdef DEBUG
							fprintf(stderr, "Exit insert mode\n");
#endif
							break;
						case 20: // linefeed mode
							to->linefeedMode = 0;
#ifdef DEBUG
							fprintf(stderr, "Exit linefeed mode\n");
#endif
							break;
					}
				}
				break;

			case 'M': // dl1 delete 1 line
				if(to->sequenceLength == 1) {
					// scroll down
				} else {
					// delete 1 line
				}
				break;

			case 'P': // dch1 delete 1 character
				break;
			/*
			case 'X': // erase chars
				break;
			*/

			case 'r': // change scrolling region
				// csr %1, need to add 1 to first two parameters
				if(to->parametersLength == 2) {
					i = to->parameters[0] - 1;
					j = to->parameters[1] - 1;
					to->scrollBegin = i;
					to->scrollEnd = j;
					if(to->callback_scrollRegion)
						to->callback_scrollRegion(to->pointer, i,j);
				} else {
					//0, 0
				}

#ifdef DEBUG
				fprintf(stderr, "Change scrolling region from line %d to %d\n", i, j);
#endif
				break;
		}
	}
/*
#ifdef DEBUG
	default:
		//fprintf(stderr, "Unrecogized CSI: <%s>\n", rt->pd->esbuf);
		break;
#endif
*/
}


void tesi_processAttributes(struct tesiObject *to) {
	short bold, underline, blink, reverse, foreground, background, charset, i;

	bold = underline = blink = reverse = foreground = background = charset = 0;

	for(i = 0; i < to->parametersLength; i++) {
		switch(to->parameters[i]) {
			case 0: // all off
				break;
			case 1: // bold
				bold = 1;
				break;
			case 4: // underline
				underline = 1;
				break;
			case 5: // blinking
				blink = 1;
				break;
			case 7: // negative OR reverse video
				reverse = 1;
				break;
			case 8: // invisible
				// invisible = 1;
				break;
			case 10: // the ASCII character set is the current 7-bit display character set (default) - SCO Console only.
				break;
			case 11: // map Hex 00-7F of the PC character set codes to the current 7-bit display character set. SCO Console only.
				break;
			case 12: // Map Hex 80-FF of the current character set to the current 7-bit display character set - SCO Console only.
				break;
			case 22: // bold off
				bold = 0;
				break;
			case 24: // underline off
				underline = 0;
				break;
			case 25: // blinking off
				blink = 0;
				break;
			case 27: // reverse off
				reverse = 0;
				break;
			case 28: // invisible off
				//invisible = 0;
				break;
		}
	}

	if(to->callback_attributes)
		to->callback_attributes(to->pointer, bold, blink, reverse, underline, foreground, background, charset);
}

void tesi_bufferPush(struct tesiObject *to, char c) {
	to->sequence[ to->sequenceLength++ ] = c;
	to->sequence[ to->sequenceLength ] = 0;
}

int tesi_limitCursor(struct tesiObject *to) {
	int a = to->x;
	int b = to->y;

	if(to->x == to->width) {
		to->x = 0;
		to->y++;
	}
	if(to->x < 0)
		to->x = 0;
#ifdef DEBUG
	if(a != to->x)
		fprintf(stderr, "Cursor out of bounds in X direction: %d\n",a);
#endif

	if(to->y == to->height) {
		to->y = to->height - 1;
		if(to->callback_scrollUp)
			to->callback_scrollUp(to->pointer);
	}
	if(to->y < 0)
		to->y = 0;
#ifdef DEBUG
	if(b != to->y)
		fprintf(stderr, "Cursor out of bounds in Y direction: %d\n",b);
#endif
	if(a != to->x || b != to->y) {
		if(to->callback_moveCursor)
			to->callback_moveCursor(to->pointer, to->x, to->y);
		return 1;
	}
	return 0;
}


struct tesiObject* newTesiObject(char *command, int width, int height) {
	struct tesiObject *to;
	struct winsize ws;
	char message[256];
	char *ptySlave;
	to = malloc(sizeof(struct tesiObject));
	if(to == NULL)
		return NULL;

	to->partialSequence = 0;
	to->sequence = malloc(sizeof(char) * 40); // escape sequence buffer
	to->sequence[0] = 0;
	to->sequenceLength = 0;

	to->parametersLength = 0;

	to->x = to->y = to->x2 = to->y2 = 0; // cursor x,y and window width,height
	to->width = width;
	to->height = height;
	to->scrollBegin = 0;
	to->scrollEnd = height - 1;
	//to->state = 0;
	to->alternativeChar = 0;
	to->linefeedMode = 1;
	to->insertMode = 0;

	to->callback_printCharacter = NULL;
	to->callback_insertCharacter = NULL;
	to->callback_eraseLine = NULL;
	to->callback_eraseCharacter = NULL;
	to->callback_moveCursor = NULL;
	to->callback_attributes = NULL;
	to->callback_scrollUp = NULL;
	to->callback_scrollDown = NULL;
	to->callback_bell = NULL;

	to->pid = 0;
	to->ptyMaster = posix_openpt(O_RDWR|O_NOCTTY);

	/*
	if(pipe(to->pipeFromChild) == -1)
		_exit(EXIT_FAILURE);
	if(pipe(to->pipeToChild) == -1)
		_exit(EXIT_FAILURE);
	*/

	to->fd_activity = to->ptyMaster; // descriptor to check whether the process has sent output
	to->fd_input = to->ptyMaster; // descriptor for sending input to child process


	// welcome message
	/*
	sprintf(message, "echo \"This %d x %d terminal is controlled by TESI...\"\n", width, height);
	message[255] = 0;
	write(to->fd_input, message, strlen(message));
	*/

	grantpt(to->ptyMaster);
	unlockpt(to->ptyMaster);
	to->pid = fork();
	if(to->pid == 0) { // inside child
		ptySlave = ptsname(to->ptyMaster);
		to->ptySlave = open(ptySlave, O_RDWR);
		// dup fds to stdin and stdout, or something like, then exec /bin/bash
		dup2(to->ptySlave, fileno(stdin));
		dup2(to->ptySlave, fileno(stdout));
		dup2(to->ptySlave, fileno(stderr));

		//clearenv(); // don't clear because of lines?
		setenv("TERM","vt102",1);
		sprintf(message, "%d", width);
		setenv("COLUMNS", message, 1);
		sprintf(message, "%d", height);
		setenv("LINES", message, 1);

		//fflush(stdout); // flush output now because it will be cleared upon execv

		if(execl("/bin/bash", "/bin/bash", NULL) == -1)
			exit(EXIT_FAILURE); // exit and become zombie until parent cares....
	}
	return to;
}
/*
 * Why does this take a void pointer?
 * So that you don't have to cast before you pass the parameter
 * */
void deleteTesiObject(void *p) {
	struct tesiObject *to = (struct tesiObject*) p;

	close(to->pipeFromChild);
	close(to->pipeToChild);

	free(to->sequence);
	free(to->command[0]);
	if(to->command[1])
		free(to->command[1]);
	free(to);
}
