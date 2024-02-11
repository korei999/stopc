#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int Quit = 0;
int Paused = 0;
pthread_cond_t Pause_cv;
pthread_mutex_t Pause_ml;

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
    Quit = 1;
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
                    pthread_cond_signal(&Pause_cv);
                break;

            default:
                break;
        }
    }
}

enum arg {
    MILLISECONDS = 0,
    SECONDS = 1
};

int
main(int argc, char* argv[])
{
    pthread_t input_thread;
    int ms_or_s;
    if (argc > 1 && strncmp(argv[1], "1", 1) == 0)
            ms_or_s = SECONDS;
    else
        ms_or_s = MILLISECONDS;

    raw();
    pthread_create(&input_thread, NULL, (void*)input, NULL);
    pthread_cond_init(&Pause_cv, NULL);

    for (int h = 0; ; h++) {

        for (int m = 0; m < 60; m++) {

            for (int s = 0; s < 60; s++) {
                /****************************/
                if (ms_or_s == MILLISECONDS) {
                    for (int ms = 0; ms < 1000; ms++) {
                        if (Quit) {
                            pthread_cancel(input_thread);
                            goto exit;
                        }

                        if (Paused) {
                            pthread_mutex_lock(&Pause_ml);
                            pthread_cond_wait(&Pause_cv, &Pause_ml);
                            pthread_mutex_unlock(&Pause_ml);
                        }

                        printf("%d:%0*d:%0*d.%0*d\r", h, 2, m, 2, s, 3, ms);
                        fflush(stdout);
                        usleep(1000);
                    }
                } else {
                    if (Quit) {
                        pthread_cancel(input_thread);
                        goto exit;
                    }

                    if (Paused) {
                        pthread_mutex_lock(&Pause_ml);
                        pthread_cond_wait(&Pause_cv, &Pause_ml);
                        pthread_mutex_unlock(&Pause_ml);
                    }

                    printf("%0*d:%0*d:%0*d\r", 2, h, 2, m, 2, s);
                    fflush(stdout);
                    usleep(1000000);
                }
                /****************************/
            }

        }
    }

exit:
    pthread_join(input_thread, NULL);
}
