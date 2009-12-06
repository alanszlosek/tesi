TESI

TESI is a combination of a "Terminal Escape Sequence Interpreter" and the Terminfo definition for the terminal it interprets.

In short: if you're writing an application that needs a terminal window, don't want to use the controls Gtk or Qt provide, and would rather do the text drawing yourself, check out TESI.

The TESI terminal was specifically designed to be modelled in software. The escape sequences are few (29 compared to xterm's 180) and libraries like ncurses that understand terminfo definitions have no problem running on TESI terminals. It was a goal that the sequences be extremely easy to interpret and be recognisable to humans (for easy debugging and coding). Thus, the sequence for moving the cursor ends in 'M' for 'move' and the sequence for clearing the screen ends in 'C', and so on.

The interface to and from a TESI terminal was also designed to be easy to use. Great care was taken to utilize great ideas (like the callback functions of libiterm) and improve upon others (the difficulty of working with libiterm and the limiting assumptions made by libROTE). The full source code is around 600 lines.


SUMMARY OF SUPPORTED ESCAPE SEQUENCES

# Move cursor to X,Y
# Move cursor home
# Clear screen
# Clear to end of line
# Insert # of lines after cursor
# Delete current line
# Set attributes (bold, underline, blink, inverse, standound, colors)
# Change scrolling region
# Scroll up/down
# Cursor up, down, left, right


HISTORY

# Idea for split-screen console application was born by Tom and shared with Alan
# Tom and Alan begin brainstorming and planning the design
# Alan learns about fork() and pipes and uses them to construct a proof of concept
# Alan finds libiterm and integrates with Knox to accomplish split-screen
# Integration had issues with scrolling, so an alternative was sought
# And so began the slow process of learning from libROTE to implement a vt102 interpreter from scratch
# Realization that pipes aren't the way to go for two way communication because applications know they aren't talking to a terminal. Thus, PTYs are needed. After 2 weeks of sparse testing and research, the PTY code works.
# Realized that emulating vt102 is a waste of time. The desire for a modern terminal with sequences more approprate to software emulation is born
# Stumble across Red Gritty Brick and realize my dream is not original or unfounded
# Begin jotting down preliminary escape sequences in Probability class
# Wonder if, when I first integrated with libiterm, the bug with scrolling was due to an error of mine ... This didn't destroy my dedication to TESI because TESI is easier to use, and thus is a worthwhile effort.
# Refine escape sequences, adding those for: setting color, toggling display attributes (bold, underline, etc)
# Find it surpisingly easy to implement a new terminal via a terminfo definition. Altered TESI to use it and interpret for it.
# Attributes mostly work, scrolling, cursor moving, etc. Except out of bounds wrapping in some cases.
# Until TESI is done, modified Knox to once again uses libiterm




Since libiterm isn't present on most Linux/Unix systems, I want TESI-based applications to be compilable and usable by users that don't have the ability to install new libraries. The TESI terminfo definition can be stored right in your home directory.