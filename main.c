#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <threads.h>
#include <unistd.h>

int Quit = 0;
int Paused = 0;

cnd_t Pause_cnd;
mtx_t Pause_mtx;

void
raw(void)
{
	struct termios raw;
	tcgetattr(STDIN_FILENO, &raw);
	raw.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void
fix_cursor_and_quit(int signum)
{
    printf("\e[?25h"); /* unhide cursor */
    mtx_destroy(&Pause_mtx);
    cnd_destroy(&Pause_cnd);
    exit(0);
}

void
input(void)
{
    char c;
    struct sigaction action = {0};
    action.sa_handler = fix_cursor_and_quit;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    printf("\e[?25l"); /* hide cursor */
    while (read(STDIN_FILENO, &c, 1) == 1) {
        switch (c) {
            case 'q':
                fix_cursor_and_quit(0);
                return;

            case ' ':
                Paused = !Paused;
                if (!Paused)
                    cnd_signal(&Pause_cnd);
                break;

            default:
                break;
        }
    }
}

void
wait_on_paused()
{
    if (Paused) {
        mtx_lock(&Pause_mtx);
        cnd_wait(&Pause_cnd, &Pause_mtx);
        mtx_unlock(&Pause_mtx);
    }
}

enum {
    milliseconds = 0,
    seconds = 1,
    sleep_1ms = 1000,
    sleep_1s = 1000000
};

int
main(int argc, char* argv[])
{
    thrd_t input_thread;

    int ms_or_s;
    if (argc > 1 && strncmp(argv[1], "1", 1) == 0)
            ms_or_s = seconds;
    else
        ms_or_s = milliseconds;

    raw();
    thrd_create(&input_thread, (thrd_start_t)input, NULL);
    thrd_detach(input_thread);
    cnd_init(&Pause_cnd);
    mtx_init(&Pause_mtx, mtx_plain);

    for (int h = 0; ; h++) {

        for (int m = 0; m < 60; m++) {

            for (int s = 0; s < 60; s++) {
                /****************************/
                if (ms_or_s == milliseconds) {
                    for (int ms = 0; ms < 1000; ms++) {
                        printf("%d:%0*d:%0*d.%0*d\r", h, 2, m, 2, s, 3, ms);
                        fflush(stdout);
                        usleep(sleep_1ms);

                        wait_on_paused();
                    }
                } else {
                    printf("%0*d:%0*d:%0*d\r", 2, h, 2, m, 2, s);
                    fflush(stdout);
                    usleep(sleep_1s);

                    wait_on_paused();
                }
                /****************************/
            }

        }
    }

    return 0;
}
