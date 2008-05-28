#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Text_Display.H>
#include <stdio.h>


void tesi_printCharacter(void *p, char c) {
	char in[129];
	
	snprintf(in, 128, "%c", c);
	//gtk_text_buffer_get_end_iter(buffer, &i);
	printf("Print %c\n", c);

	gtk_text_buffer_insert(buffer, &iter, in, 1);
}
void tesi_eraseCharacter(void *p) {
	GtkTextBuffer *buffer;
	GtkTextIter i;
	/*
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(p));
	gtk_text_iter_forward_to_line_end(&i);
	gtk_text_buffer_get_iter_at_line_index(buffer, &iter, y, x);
	gtk_text_buffer_delete(buffer, &iter, &iter);
	*/
}
void tesi_moveCursor(void *p, int x, int y) {
	/*
	Force moving of cursor
	If line doesn't exist, start at last line and loop while adding newlines
	If line does exist, but column doesn't, go to line and add spaces at end of line
	*/
	//GtkTextBuffer *buffer;
	//buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(p));

	printf("Move Cursor to x,y: %d,%d\n", x, y);

	gtk_text_buffer_get_iter_at_line_index(buffer, &iter, y, 0);
	while(gtk_text_iter_get_line(&iter) < y) { // loop and fill out contents to destination line
		gtk_text_buffer_get_end_iter(buffer, &iter);
		gtk_text_buffer_insert(buffer, &iter, "\n", 1);
	}

	//gtk_text_buffer_get_iter_at_line_index(buffer, &iter, y, x);
	gtk_text_iter_forward_to_line_end(&iter);
	while(gtk_text_iter_get_line_offset(&iter) < x) { // loop and fill out contents to destination column
		gtk_text_buffer_insert(buffer, &iter, " ", 1);
        }

	
}
void tesi_insertLine(void *p) {
	printf("Insert Line\n");
}
void tesi_eraseLine(void *p) {
	GtkTextIter s, e;
	//GtkTextBuffer *buffer;
	//buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(p));
	
	//gtk_text_buffer_get_end_iter(buffer, &iter);
	s = e = iter;
	gtk_text_iter_backward_line(&s); // move to start of current line (delimiter)
	gtk_text_iter_forward_to_line_end(&e);
	gtk_text_iter_backward_char(&e);
	gtk_text_buffer_delete(buffer, &s, &e);
	printf("Erase Line\n");
}

gboolean g_tesi_handleInput(gpointer data) {
	tesi_handleInput( (struct tesiObject*) data);
	return TRUE;
}

int main() {
	struct tesiObject *t;

	char buffer[128];
	Fl_Window *win = new Fl_Window(640, 480);
	Fl_Text_Buffer *buff = new Fl_Text_Buffer();
	Fl_Text_Display *disp = new Fl_Text_Display(20, 20, 640-40, 480-40, "Display");

	disp->buffer(buff);

	/*
	t = newTesiObject("/bin/bash", 70, 60);
	t->pointer = buff;
	t->callback_printCharacter = &tesi_printCharacter;
	t->callback_eraseCharacter = &tesi_eraseCharacter;
	t->callback_moveCursor = &tesi_moveCursor;
	t->callback_insertLine = &tesi_insertLine;
	t->callback_eraseLine = &tesi_eraseLine;

	g_timeout_add(100, &g_tesi_handleInput, t);
	*/


	buff->text("\n\n\n\n");

	sprintf(buffer, "%d", buff->line_start(2));
	buff->insert(0, buffer);
	win->resizable(*disp);
	win->show();

	/*
	kill(t->pid, SIGKILL);
	deleteTesiObject(t);
	*/

	return(Fl::run());
}
