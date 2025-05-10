#define _POSIX_C_SOURCE 200809L

#if defined(__linux__) || defined(__APPLE__)
#if __has_include(<ncurses.h>)
#include <ncurses.h>
#elif __has_include(<curses.h>)
#include <curses.h>
#else
#error "Curses library not found"
#endif
#else
#error "Only POSIX-compatible systems supported"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

#define MAX_LINES 10000
#define MAX_COLS 1024
#define KEY_ESC 27
#define KEY_CTRL_Q 17

char *lines[MAX_LINES];
int dirty[MAX_LINES];
int line_count = 0;

char filename[PATH_MAX];
char tempname[PATH_MAX];

int cx = 0, cy = 0;
int rowoff = 0, coloff = 0;
int screen_rows, screen_cols;

void cleanup(void)
{
    endwin();
    for (int i = 0; i < line_count; i++)
        free(lines[i]);
}

void atomic_save(void)
{
    int needsave = 0;
    for (int i = 0; i < line_count; i++)
        if (dirty[i])
        {
            needsave = 1;
            break;
        }

    if (!needsave)
        return;

    snprintf(tempname, sizeof(tempname), "%s.tmp", filename);
    int fd = open(tempname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return;

    for (int i = 0; i < line_count; i++)
    {
        write(fd, lines[i], strlen(lines[i]));
        if (i < line_count - 1)
            write(fd, "\n", 1);
    }
    fsync(fd);
    close(fd);
    rename(tempname, filename);
    memset(dirty, 0, sizeof(dirty));
}

void refresh_screen(void)
{
    getmaxyx(stdscr, screen_rows, screen_cols);
    clear();
    for (int y = 0; y < screen_rows - 1; y++)
    {
        int fy = y + rowoff;
        if (fy >= line_count)
            continue;
        int len = strlen(lines[fy]);
        if (coloff < len)
            mvaddnstr(y, 0, lines[fy] + coloff, screen_cols);
    }
    attron(A_REVERSE);
    mvprintw(screen_rows - 1, 0, "ESC/Ctrl+Q to quit | Ln %d, Col %d%s | By Rahul Dange",
             cy + 1, cx + 1, dirty[cy] ? " *" : "");
    clrtoeol();
    attroff(A_REVERSE);
    move(cy - rowoff, cx - coloff);
    refresh();
}

void editor_scroll(void)
{
    if (cy < rowoff)
        rowoff = cy;
    if (cy >= rowoff + screen_rows - 1)
        rowoff = cy - screen_rows + 2;
    if (cx < coloff)
        coloff = cx;
    if (cx >= coloff + screen_cols)
        coloff = cx - screen_cols + 1;
}

void insert_char(int ch)
{
    if (ch == '\t')
    {
        for (int i = 0; i < 4; i++)
            insert_char(' ');
        return;
    }

    char *line = lines[cy];
    int len = strlen(line);
    if (len >= MAX_COLS - 2)
        return;

    char *newl = malloc(len + 2);
    memcpy(newl, line, cx);
    newl[cx] = ch;
    strcpy(newl + cx + 1, line + cx);
    free(line);
    lines[cy] = newl;
    cx++;
    dirty[cy] = 1;
    atomic_save();
}

void insert_newline(void)
{
    if (line_count >= MAX_LINES)
        return;

    char *line = lines[cy];
    char *right = strdup(line + cx);
    line[cx] = '\0';

    for (int i = line_count; i > cy + 1; i--)
    {
        lines[i] = lines[i - 1];
        dirty[i] = dirty[i - 1];
    }
    lines[cy + 1] = right;
    line_count++;
    dirty[cy] = dirty[cy + 1] = 1;
    cy++;
    cx = 0;
    atomic_save();
}

void delete_char(void)
{
    char *line = lines[cy];
    if (cx > 0)
    {
        memmove(line + cx - 1, line + cx, strlen(line) - cx + 1);
        cx--;
        dirty[cy] = 1;
        atomic_save();
    }
    else if (cy > 0)
    {
        int prevlen = strlen(lines[cy - 1]);
        int curlen = strlen(line);
        if (prevlen + curlen >= MAX_COLS - 2)
            return;

        char *merged = malloc(prevlen + curlen + 1);
        strcpy(merged, lines[cy - 1]);
        strcat(merged, line);

        free(lines[cy - 1]);
        free(line);
        lines[cy - 1] = merged;

        for (int i = cy; i < line_count - 1; i++)
        {
            lines[i] = lines[i + 1];
            dirty[i] = dirty[i + 1];
        }
        line_count--;
        cy--;
        cx = prevlen;
        dirty[cy] = 1;
        atomic_save();
    }
}

void move_cursor(int key)
{
    size_t len = strlen(lines[cy]);
    switch (key)
    {
    case KEY_LEFT:
        if (cx > 0)
            cx--;
        else if (cy > 0)
        {
            cy--;
            cx = strlen(lines[cy]);
        }
        break;
    case KEY_RIGHT:
        if ((size_t)cx < len)
            cx++;
        else if (cy < line_count - 1)
        {
            cy++;
            cx = 0;
        }
        break;
    case KEY_UP:
        if (cy > 0)
            cy--;
        break;
    case KEY_DOWN:
        if (cy < line_count - 1)
            cy++;
        break;
    case KEY_HOME:
        cx = 0;
        break;
    case KEY_END:
        cx = len;
        break;
    }
    len = strlen(lines[cy]);
    if ((size_t)cx > len)
        cx = len;
}

void load_file(const char *path)
{
    strncpy(filename, path, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';

    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        lines[0] = strdup("");
        line_count = 1;
        return;
    }

    char buf[MAX_COLS];
    char long_line[MAX_COLS * 4] = {0};
    size_t long_len = 0;

    while (fgets(buf, sizeof(buf), fp) && line_count < MAX_LINES)
    {
        size_t len = strlen(buf);
        int is_complete = (len > 0 && buf[len - 1] == '\n');
        if (is_complete)
            buf[--len] = '\0';

        if (long_len + len >= sizeof(long_line) - 1)
        {
            fprintf(stderr, "Line too long, truncating\n");
            break;
        }

        memcpy(long_line + long_len, buf, len);
        long_len += len;

        if (is_complete || feof(fp))
        {
            long_line[long_len] = '\0';

            char *linebuf = malloc(strlen(long_line) * 4 + 1);
            char *src = long_line, *dst = linebuf;
            while (*src)
            {
                if (*src == '\t')
                {
                    strcpy(dst, "    ");
                    dst += 4;
                }
                else if (*src >= 32 && *src <= 126)
                {
                    *dst++ = *src;
                }
                else
                {
                    *dst++ = '?';
                }
                src++;
            }
            *dst = '\0';
            lines[line_count++] = strdup(linebuf);
            free(linebuf);
            long_len = 0;
        }
    }

    fclose(fp);
    if (line_count == 0)
    {
        lines[0] = strdup("");
        line_count = 1;
    }
    memset(dirty, 0, sizeof(dirty));
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    load_file(argv[1]);

    initscr();
    setenv("TERM", "xterm-256color", 1);
    raw();
    noecho();
    keypad(stdscr, TRUE);
    getmaxyx(stdscr, screen_rows, screen_cols);

    while (1)
    {
        editor_scroll();
        refresh_screen();
        int ch = getch();
        if (ch == KEY_ESC || ch == KEY_CTRL_Q)
            break;
        else if (ch == KEY_BACKSPACE || ch == 127)
            delete_char();
        else if (ch == '\n')
            insert_newline();
        else if (ch == '\t')
        {
            insert_char(' ');
            insert_char(' ');
            insert_char(' ');
            insert_char(' ');
        }
        else if (ch >= 32 && ch <= 126)
            insert_char(ch);
        else
            move_cursor(ch);
    }

    cleanup();
    atomic_save();
    return 0;
}
