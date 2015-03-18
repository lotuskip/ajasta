#include <ncurses.h>
#include <sys/time.h>

#define N_TIMERS ('z'-'a'+1)
#define MAX_NAME_LEN 20
#define MILL 1000000

const char runsym[2] = { ' ', '*' };

struct Timer {
	struct timeval started, stored, split;
	char name[MAX_NAME_LEN];
	char running, has_split, used;
};

/* Substract y from x, put result in result */
void timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y);
/* Add y to x. */
void timeval_add(struct timeval *x, struct timeval *y);


int main(void)
{
	int i, input;
	struct Timer timers[N_TIMERS];
	struct timeval tmp, now;
	char inmode = 0; /* 0: normal, 1: pressed '!', 2: pressed '?' */

	/* Init ncurses stuff */
	initscr();
	nodelay(stdscr, TRUE);
	refresh();

	/* Init timers */
	for (i = 0; i < N_TIMERS; ++i)
	{
		timers[i].running = timers[i].has_split = timers[i].used = 0;
		timers[i].name[0] = '\0';
		timers[i].stored.tv_sec = timers[i].stored.tv_usec = 0;
	}

	for (;;) /* Main loop */
	{
		erase();

		/* Print timers */
		for (i = 0; i < N_TIMERS; ++i)
		{
			if (timers[i].used) /* Print only those that have been used/named */
			{
				printw("%c%c (%s):",
					i+'a',
					runsym[(int)(timers[i].running)],
					timers[i].name);

				if (timers[i].has_split)
				{
					printw(" [%i:%02i.%03i]",
						timers[i].split.tv_sec/60,
						timers[i].split.tv_sec%60,
						timers[i].split.tv_usec/1000);
				}
				if (timers[i].running)
				{
					gettimeofday(&now, 0);
					timeval_subtract(&tmp, &now, &timers[i].started);
				}
				else
					tmp.tv_sec = tmp.tv_usec = 0;
				timeval_add(&tmp, &timers[i].stored);
				printw(" %i:%02i.%03i",
					tmp.tv_sec/60,
					tmp.tv_sec%60,
					tmp.tv_usec/1000);
				printw("\n");
			}
		}
		if (inmode == 1)
			printw("!");
		else if (inmode == 2)
			printw("?");
		refresh();

		/* Get & process input */
		if ((input = getch()) != ERR)
		{
			if (input >= 'a' && input <= 'z')
			{
				input -= 'a';
				if (inmode == 0) /* start/stop */
				{
					if ((timers[input].running = !timers[input].running)) /* start */
					{
						gettimeofday(&timers[input].started, 0);
						timers[input].used = 1;
					}
					else /* stopping, store elapsed time */
					{
						gettimeofday(&now, 0);
						timeval_subtract(&tmp, &now, &timers[input].started);
						timeval_add(&timers[input].stored, &tmp);
					}
				}
				else if (inmode == 1) /* null timer */
				{
					timers[input].stored.tv_sec = timers[input].stored.tv_usec = 0;
					timers[input].has_split = 0;
					gettimeofday(&timers[input].started, 0);
				}
				else if (inmode == 2) /* name timer */
				{
					nodelay(stdscr, FALSE);
					printw(":");
					refresh();
					getnstr(timers[input].name, MAX_NAME_LEN);
					nodelay(stdscr, TRUE);
					timers[input].used = 1;
				}
				inmode = 0;
			}
			else if (inmode == 1 && input == '!') /* reset all */
			{
				for (i = 0; i < N_TIMERS; ++i)
					if(timers[i].used)
					{
						timers[i].stored.tv_sec = timers[i].stored.tv_usec = 0;
						timers[i].has_split = 0;
						gettimeofday(&timers[i].started, 0);
					}
				inmode = 0;
			}
			else if (inmode == 0) /* these keys only valid in this input mode */
			{
				if (input >= 'A' && input <= 'Z') /* take split time */
				{
					input -= 'A';
					if (timers[input].running)
					{
						gettimeofday(&now, 0);
						timeval_subtract(&timers[input].split, &now, &timers[input].started);
						timeval_add(&timers[input].split, &timers[input].stored);
					}
					else /* stopped timer; split becomes stored value */
					{
						timers[input].split.tv_sec = timers[input].stored.tv_sec;
						timers[input].split.tv_usec = timers[input].stored.tv_usec;
					}
					timers[input].has_split = 1;
				}
				else if (input == '!') /* stop timer; next ask for which */
					inmode = 1;
				else if (input == '?') /* name timer; next ask for which */
					inmode = 2;
				else if (input == '#') /* quit */
					break;
				else if (input == '.') /* stop all */
				{
					gettimeofday(&now, 0);
					for (i = 0; i < N_TIMERS; ++i)
						if (timers[i].running)
						{
							timers[i].running = 0;
							timeval_subtract(&tmp, &now, &timers[i].started);
							timeval_add(&timers[i].stored, &tmp);
						}
				}
				else if (input == ':') /* split all */
				{
					gettimeofday(&now, 0);
					for (i = 0; i < N_TIMERS; ++i)
						if (timers[i].running)
						{
							timeval_subtract(&timers[i].split, &now, &timers[i].started);
							timeval_add(&timers[i].split, &timers[i].stored);
							timers[i].has_split = 1;
						}
				}
				else if (input == ' ') /* start all used */
				{
					for (i = 0; i < N_TIMERS; ++i)
						if (timers[i].used && !timers[i].running)
						{
							timers[i].running = 1;
							gettimeofday(&timers[i].started, 0);
						}
				}
			}
			else
				inmode = 0; /* invalid input in non-zero input mode */
		}
	}

	endwin(); /* Deinit ncurses */

	return 0;
}

void timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
	int nsec;
	if (x->tv_usec < y->tv_usec)
	{
		nsec = (y->tv_usec - x->tv_usec)/MILL + 1;
		y->tv_usec -= MILL*nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > MILL)
	{
		nsec = (x->tv_usec - y->tv_usec)/MILL;
		y->tv_usec += MILL*nsec;
		y->tv_sec -= nsec;
	}
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;
}

void timeval_add(struct timeval *x, struct timeval *y)
{
	x->tv_sec += y->tv_sec;
	x->tv_usec += y->tv_usec;
	if (x->tv_usec > MILL)
	{
		x->tv_sec++;
		x->tv_usec %= MILL;
	}
}
