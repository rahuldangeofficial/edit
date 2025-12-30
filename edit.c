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
#include <signal.h>

#define MAX_LINES 10000
#define MAX_COLS 1024
#define KEY_ESC 27
#define KEY_CTRL_Q 17

char *lines[MAX_LINES];
int dirty[MAX_LINES];
int line_count = 0;

char filename[PATH_MAX];
char tempname[PATH_MAX];
volatile sig_atomic_t quit_flag = 0;

int cx = 0, cy = 0;
int rowoff = 0, coloff = 0;
int screen_rows, screen_cols;

void cleanup(void)
{
    endwin();
    for (int i = 0; i < line_count; i++)
        if (lines[i]) {
            free(lines[i]);
            lines[i] = NULL;
        }
}

void signal_handler(int sig)
{
    quit_flag = 1;
}

void atomic_save(void)
{
    int needsave = 0;
    for (int i = 0; i < line_count; i++)
        if (dirty[i] && lines[i])
        {
            needsave = 1;
            break;
        }

    if (!needsave)
        return;

    snprintf(tempname, sizeof(tempname), "%s.tmp", filename);
    int fd = open(tempname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(stderr, "Error creating temp file: %s\n", strerror(errno));
        return;
    }

    for (int i = 0; i < line_count; i++)
    {
        if (!lines[i]) continue;
        size_t len = strlen(lines[i]);
        if (write(fd, lines[i], len) != (ssize_t)len) {
            close(fd);
            unlink(tempname);
            fprintf(stderr, "Error writing to temp file: %s\n", strerror(errno));
            return;
        }
        if (i < line_count - 1) {
            if (write(fd, "\n", 1) != 1) {
                close(fd);
                unlink(tempname);
                fprintf(stderr, "Error writing newline to temp file: %s\n", strerror(errno));
                return;
            }
        }
    }
    
    if (fsync(fd) != 0) {
        close(fd);
        unlink(tempname);
        fprintf(stderr, "Error syncing temp file: %s\n", strerror(errno));
        return;
    }
    
    close(fd);
    if (rename(tempname, filename) != 0) {
        unlink(tempname);
        fprintf(stderr, "Error renaming temp file: %s\n", strerror(errno));
        return;
    }
    memset(dirty, 0, sizeof(dirty));
}

void refresh_screen(void)
{
    getmaxyx(stdscr, screen_rows, screen_cols);
    clear();
    
    if (screen_rows <= 0 || screen_cols <= 0)
        return;
        
    for (int y = 0; y < screen_rows - 1 && y < MAX_LINES; y++)
    {
        int fy = y + rowoff;
        if (fy >= line_count || fy < 0 || !lines[fy])
            continue;
            
        int len = strlen(lines[fy]);
        if (coloff < len && coloff >= 0)
        {
            int max_chars = screen_cols;
            if (len - coloff < max_chars)
                max_chars = len - coloff;
                
            if (max_chars > 0)
                mvaddnstr(y, 0, lines[fy] + coloff, max_chars);
        }
    }
    
    attron(A_REVERSE);
    char status[256];
    snprintf(status, sizeof(status), "ESC/Ctrl+Q to quit | Ln %d, Col %d%s | By Rahul Dange",
             cy + 1, cx + 1, (cy >= 0 && cy < MAX_LINES && dirty[cy]) ? " *" : "");
    mvprintw(screen_rows - 1, 0, "%s", status);
    clrtoeol();
    attroff(A_REVERSE);
    
    if (cy >= 0 && cy < MAX_LINES && cx >= 0 && cx < MAX_COLS)
        move(cy - rowoff, cx - coloff);
    refresh();
}

void editor_scroll(void)
{
    if (cy < 0) cy = 0;
    if (cx < 0) cx = 0;
    if (rowoff < 0) rowoff = 0;
    if (coloff < 0) coloff = 0;
    
    if (cy < rowoff)
        rowoff = cy;
    if (cy >= rowoff + screen_rows - 1)
        rowoff = cy - screen_rows + 2;
    if (cx < coloff)
        coloff = cx;
    if (cx >= coloff + screen_cols)
        coloff = cx - screen_cols + 1;
        
    if (rowoff < 0) rowoff = 0;
    if (coloff < 0) coloff = 0;
}

void insert_char(int ch)
{
    if (cy < 0 || cy >= MAX_LINES || cx < 0 || cx >= MAX_COLS)
        return;
        
    if (!lines[cy]) {
        lines[cy] = strdup("");
        if (!lines[cy]) {
            fprintf(stderr, "Memory allocation failed\n");
            return;
        }
    }
        
    if (ch == '\t')
    {
        for (int i = 0; i < 4; i++)
            insert_char(' ');
        return;
    }

    char *line = lines[cy];
    if (!line) return;
    
    size_t len = strlen(line);
    if (len >= (size_t)(MAX_COLS - 2) || cx > (int)len)
        return;

    char *newl = malloc(len + 2);
    if (!newl) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    
    if (cx > (int)len) cx = len;
    
    memcpy(newl, line, cx);
    newl[cx] = ch;
    strcpy(newl + cx + 1, line + cx);
    free(line);
    lines[cy] = newl;
    
    if (cx < MAX_COLS - 1) cx++;
    dirty[cy] = 1;
    atomic_save();
}

void insert_newline(void)
{
    if (cy < 0 || cy >= MAX_LINES || line_count >= MAX_LINES)
        return;
        
    if (!lines[cy]) {
        lines[cy] = strdup("");
        if (!lines[cy]) {
            fprintf(stderr, "Memory allocation failed\n");
            return;
        }
    }

    char *line = lines[cy];
    if (!line) return;
    
    size_t line_len = strlen(line);
    if (cx > (int)line_len) cx = line_len;
    
    char *right = strdup(line + cx);
    if (!right) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    
    line[cx] = '\0';

    for (int i = line_count; i > cy + 1; i--)
    {
        if (i - 1 >= 0 && i - 1 < MAX_LINES) {
            lines[i] = lines[i - 1];
            dirty[i] = dirty[i - 1];
        }
    }
    
    if (cy + 1 < MAX_LINES) {
        lines[cy + 1] = right;
        dirty[cy + 1] = 1;
    }
    
    if (line_count < MAX_LINES) line_count++;
    dirty[cy] = 1;
    
    if (cy < MAX_LINES - 1) cy++;
    cx = 0;
    atomic_save();
}

void delete_char(void)
{
    if (cy < 0 || cy >= MAX_LINES || cx < 0)
        return;
        
    if (!lines[cy]) {
        lines[cy] = strdup("");
        if (!lines[cy]) {
            fprintf(stderr, "Memory allocation failed\n");
            return;
        }
        return;
    }
        
    char *line = lines[cy];
    size_t len = strlen(line);
    
    if (cx > 0)
    {
        if (cx > (int)len) cx = len;
        memmove(line + cx - 1, line + cx, len - cx + 1);
        if (cx > 0) cx--;
        dirty[cy] = 1;
        atomic_save();
    }
    else if (cy > 0 && cy < MAX_LINES)
    {
        if (!lines[cy - 1]) {
            lines[cy - 1] = strdup("");
            if (!lines[cy - 1]) {
                fprintf(stderr, "Memory allocation failed\n");
                return;
            }
        }
        
        size_t prevlen = strlen(lines[cy - 1]);
        if (prevlen + len >= (size_t)(MAX_COLS - 2))
            return;

        char *merged = malloc(prevlen + len + 1);
        if (!merged) {
            fprintf(stderr, "Memory allocation failed\n");
            return;
        }
        
        strcpy(merged, lines[cy - 1]);
        strcat(merged, line);

        free(lines[cy - 1]);
        free(line);
        lines[cy - 1] = merged;
        lines[cy] = NULL;

        for (int i = cy; i < line_count - 1 && i < MAX_LINES - 1; i++)
        {
            lines[i] = lines[i + 1];
            dirty[i] = dirty[i + 1];
        }
        
        if (line_count > 0) line_count--;
        if (cy > 0) cy--;
        cx = prevlen;
        if (cy >= 0 && cy < MAX_LINES) dirty[cy] = 1;
        atomic_save();
    }
}

void move_cursor(int key)
{
    if (cy < 0) cy = 0;
    if (cx < 0) cx = 0;
    if (cy >= line_count) cy = line_count - 1;
    if (cy >= MAX_LINES) cy = MAX_LINES - 1;
    
    if (!lines[cy]) {
        lines[cy] = strdup("");
        if (!lines[cy]) {
            fprintf(stderr, "Memory allocation failed\n");
            return;
        }
    }
        
    size_t len = strlen(lines[cy]);
    
    switch (key)
    {
    case KEY_LEFT:
        if (cx > 0)
            cx--;
        else if (cy > 0)
        {
            cy--;
            if (cy >= 0 && cy < MAX_LINES && lines[cy])
                cx = strlen(lines[cy]);
            else
                cx = 0;
        }
        break;
    case KEY_RIGHT:
        if ((size_t)cx < len)
            cx++;
        else if (cy < line_count - 1 && cy < MAX_LINES - 1)
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
        if (cy < line_count - 1 && cy < MAX_LINES - 1)
            cy++;
        break;
    case KEY_HOME:
        cx = 0;
        break;
    case KEY_END:
        cx = len;
        break;
    }
    
    if (cy >= 0 && cy < MAX_LINES && lines[cy]) {
        len = strlen(lines[cy]);
        if ((size_t)cx > len)
            cx = len;
    }
    
    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    if (cx >= MAX_COLS) cx = MAX_COLS - 1;
    if (cy >= MAX_LINES) cy = MAX_LINES - 1;
}

void load_file(const char *path)
{
    strncpy(filename, path, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';

    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        lines[0] = strdup("");
        if (!lines[0]) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
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
            if (!linebuf) {
                fprintf(stderr, "Memory allocation failed\n");
                fclose(fp);
                exit(1);
            }
            
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
            
            lines[line_count] = strdup(linebuf);
            if (!lines[line_count]) {
                fprintf(stderr, "Memory allocation failed\n");
                free(linebuf);
                fclose(fp);
                exit(1);
            }
            line_count++;
            free(linebuf);
            long_len = 0;
        }
    }

    fclose(fp);
    if (line_count == 0)
    {
        lines[0] = strdup("");
        if (!lines[0]) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        line_count = 1;
    }
    memset(dirty, 0, sizeof(dirty));
}

int main(int argc, char *argv[])
{
    if (argc != 2 || !argv[1] || strlen(argv[1]) == 0)
    {
        fprintf(stderr, "Usage: %s <file>\n", argv[0] ? argv[0] : "edit");
        return 1;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    load_file(argv[1]);

    setenv("TERM", "xterm-256color", 1);
    setenv("ESCDELAY", "25", 1);         
    
    if (initscr() == NULL) {
        fprintf(stderr, "Failed to initialize ncurses\n");
        cleanup();
        return 1;
    }
    
    raw();
    noecho();
    keypad(stdscr, TRUE);
    getmaxyx(stdscr, screen_rows, screen_cols);
    
    if (screen_rows <= 0 || screen_cols <= 0) {
        fprintf(stderr, "Invalid terminal size\n");
        cleanup();
        return 1;
    }

    while (!quit_flag)
    {
        if (cy < 0) cy = 0;
        if (cx < 0) cx = 0;
        if (cy >= line_count) cy = line_count - 1;
        if (cy >= MAX_LINES) cy = MAX_LINES - 1;
        
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
        else if (ch == KEY_UP || ch == KEY_DOWN || ch == KEY_LEFT || ch == KEY_RIGHT ||
                 ch == KEY_HOME || ch == KEY_END)
            move_cursor(ch);
    }

    cleanup();
    atomic_save();
    return 0;
}
