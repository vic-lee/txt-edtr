/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/
#define CTRL_KEY(k) ((k)&0x1f)

/*** data ***/

struct termios orig_termios;

/*** terminal ***/

void die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disable_raw_mode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
}

void enable_raw_mode()
{
    /**
     * Turn off canonical mode (read byte by byte, not line by line).
     * 
     * Miscellaneous notes: 
     *   c_lflag: local flags (miscellaneous flags)
     *   c_iflag: input flags
     *   c_oflag: output flags
     *   c_cflag: control flags
     *   c_cc: control characters
     */

    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        die("tcgetattr");

    atexit(disable_raw_mode); /* call `disable_raw_mode` when exit; whether from main or by exit() */

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

char editor_read_key()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if (nread == 01 && errno != EAGAIN)
            die("read");
    }
    return c;
}

/*** output ***/

void editor_draw_rows()
{
    int y;
    for (y = 0; y < 24; y++)
    {
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

void editor_refresh_screen()
{
    /**
     * Note: 
     *   \x1b is the escape character, which corresponcs to 27.
     */

    write(STDOUT_FILENO, "\x1b[2J", 4); /* clear the screen w/ the `J` cmd */
    write(STDOUT_FILENO, "\x1b[H", 3);  /* reposition the cursor w/ the `H` cmd */

    editor_draw_rows();
    
    write(STDOUT_FILENO, "\x1b[H", 3); /* reposition cursor back to top left after drawing `~` */
}

/*** input ***/

void editor_process_keypress()
{
    char c = editor_read_key();

    switch (c)
    {
    case CTRL_KEY('q'):
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
        break;
    }
}

/*** init ***/

int main()
{
    enable_raw_mode();

    while (1)
    {
        editor_refresh_screen();
        editor_process_keypress();
    }

    return 0;
}
