

#include "revpid.h"
#include "revpi_control_lib.h"

#define _GNU_SOURCE

#include <sched.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>



static sig_atomic_t stopped;
static pthread_mutex_t settings_lock;


struct revpi_control_setting predefined_settings[] = {
	{
		.type = REVPI_CTRL_SETTING_DIGITAL0,
	}, 
	{
		.type = REVPI_CTRL_SETTING_DIGITAL1,
	}, 
	{
		.type = REVPI_CTRL_SETTING_DIGITAL2,
	}, 
	{
		.type = REVPI_CTRL_SETTING_DIGITAL3,
	}, 
	{
		.type = REVPI_CTRL_SETTING_DIGITAL4,
	}, 
	{
		.type = REVPI_CTRL_SETTING_DIGITAL5,
	}, 
	{
		.type = REVPI_CTRL_SETTING_DIGITAL6,
	}, 
	{
		.type = REVPI_CTRL_SETTING_DIGITAL7,
	}, 
};


struct revpi_control_setting test_settings[] = {
	{
		.type = REVPI_CTRL_SETTING_DIGITAL0,
	}, 
	{
		.type = REVPI_CTRL_SETTING_DIGITAL1,
	}, 
	{
		.type = REVPI_CTRL_SETTING_DIGITAL2,
	}, 
	{
		.type = REVPI_CTRL_SETTING_DIGITAL3,
	}, 
	{
		.type = REVPI_CTRL_SETTING_DIGITAL4,
	}, 
	{
		.type = REVPI_CTRL_SETTING_DIGITAL5,
	}, 
	{
		.type = REVPI_CTRL_SETTING_DIGITAL6,
	}, 
	{
		.type = REVPI_CTRL_SETTING_DIGITAL7,
	}, 
	{
		.type = REVPI_CTRL_SETTING_CORE_TEMP,
	}, 
};


static void sighandler(int sig)
{
	if (sig == SIGTERM)
		stopped = true;
}


static void *din_polling(void *arg)
{
        struct timespec start; 
        struct timespec end; 
        long delta;
        long max_delta = 0;
	unsigned long long num_loops = 0;
	unsigned long long avg = 0;
	unsigned int num_settings;
	int ret;

#if 0
	num_settings = sizeof(test_settings) / sizeof(struct revpi_control_setting);
#else
	num_settings = 9;
#endif

	while (!stopped) {
		
		clock_gettime(CLOCK_MONOTONIC, &start);
		pthread_mutex_lock(&settings_lock);
		ret = revpi_control_read_settings(test_settings, num_settings);
		pthread_mutex_unlock(&settings_lock);
		clock_gettime(CLOCK_MONOTONIC, &end);

		if (ret) {
			fprintf(stderr, "failed to read settings, aborting\n");
			stopped = true;
		} 

		delta = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);

		if (delta > max_delta) {
			max_delta = delta;
			fprintf(stderr, "*** max delta is now %li\n", max_delta);
		}

#if 0
		for (i = 0; i < num_settings; i++) {
			fprintf(stderr, "type: %i, val: %i\n", test_settings[i].type,
				test_settings[i].value);
		}
#endif
		

#if 0
		sleep(1);
#endif

#if 0
		fprintf(stderr, "MAIN...\n");
#endif

		avg += delta;

		num_loops++;

	}

	avg /= num_loops;

	fprintf(stderr, "AVG latency: %llu\n", avg);

	fprintf(stderr, "POLLER THREAD ENDED\n");

	return NULL;
}

static void daemonize()
{
        pid_t pid;
        pid_t sid;
        
        pid = fork();

        if (pid < 0) {
		fprintf(stderr, "failed to fork daemon: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }
	/* exit parent */
        if (pid != 0) {
                exit(EXIT_SUCCESS);
        }

        /* Change the current working directory */
        if ((chdir("/")) < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }
 
        sid = setsid();
        if (sid < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }
	/* make sure we are no session leader */
        pid = fork();
        if (pid < 0) {
		fprintf(stderr, "failed to fork daemon from session: %s\n",
			strerror(errno));
                exit(EXIT_FAILURE);
        }
        if (pid != 0) {
                exit(EXIT_SUCCESS);
        }
	/* now we are really daemonized, close all streams */	

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

 
        umask(0);
}

static int setup_rt(void)
{
	struct sched_param sched;
	cpu_set_t cpuset;
	int ret;

	memset(&sched, 0, sizeof(sched));
	
	sched.sched_priority = 80;

	// TODO: try with SCHED_DEADLINE
	
	ret = sched_setscheduler(0, SCHED_RR, &sched);
	if (ret) {
		fprintf(stderr, "failed to set scheduling parameters: %s\n",
			strerror(errno));
		return -1;
	}

	CPU_ZERO(&cpuset);
	CPU_SET(3, &cpuset);
	
	ret = sched_setaffinity(0, sizeof(cpuset), &cpuset);
	if (ret) {
		fprintf(stderr, "failed to set cpu affinity: %s\n",
			strerror(errno));
		return -1;
	}
	
	return ret;
}

static void dump_settings(struct revpi_control_setting *settings, int num)
{
	int i;

	for (i = 0; i < num; i++) {
		fprintf(stderr, "Setting %i: type=%u, val=%u\n", i,
			settings[i].type, settings[i].value);
	}
}

static void main_loop(struct revpi_control_setting *settings, size_t len)
{
	unsigned int num_settings;
	int ret;
	int i;

	num_settings = sizeof(predefined_settings) / sizeof(predefined_settings[0]);

	while (!stopped) {
		//sleep(1);

		ret = revpi_control_read_settings(settings, num_settings);
#if 0
		dump_settings(settings, num_settings);
#endif
		if (ret) {
			fprintf(stderr, "failed to read settings from device\n");
			continue;
		}

#if 0
		ret = msync(settings, len, MS_SYNC | MS_INVALIDATE);
		if (ret) {
			fprintf(stderr, "failed to sync file: %s\n", strerror(errno));
			continue;
		}
#endif
	
	}
}

static int do_main()
{
	int ret;
	int tret;
	pthread_t poll_tid;
	int comm_file;
	struct revpi_control_setting *settings;

	ret = revpi_control_init(REVPI_CTRL_ACCESS_DIRECT);
	if (ret) {
		fprintf(stderr, "control init failed\n");
		return -1;
	}

	ret = pthread_create(&poll_tid, NULL, din_polling, NULL);
	if (ret < 0) {
		fprintf(stderr, "failed to create polling thread: %s\n", strerror(errno));
		goto release_control;
	}

	comm_file = open(REVPID_COMM_FILE, O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
	if (comm_file < 0) {
		fprintf(stderr, "failed to open communication file %s: %s\n",
			REVPID_COMM_FILE, strerror(errno));
		goto cancel_thread;
	}
	ret  = ftruncate(comm_file, sizeof(predefined_settings));
	if (ret) {
		fprintf(stderr, "failed to set filesize: %s\n", strerror(errno));
		goto cancel_thread;
	}
	
	settings = mmap(NULL, sizeof(predefined_settings), PROT_READ | PROT_WRITE,
		       MAP_SHARED, comm_file, 0);

	if (settings == MAP_FAILED) {
		fprintf(stderr, "failed to map communication file %s: %s\n",
			REVPID_COMM_FILE, strerror(errno));
		goto cancel_thread;
	}
		

	memcpy(settings, predefined_settings, sizeof(predefined_settings));

	dump_settings(settings, sizeof(predefined_settings) / sizeof(predefined_settings[0]));


	main_loop(settings, sizeof(predefined_settings));


	unlink(REVPID_COMM_FILE);

	tret = pthread_join(poll_tid, NULL);
	if (tret)
		fprintf(stderr, "Thread join returned error: %i\n", tret);
	else	
		fprintf(stderr, "Thread successfully joined\n");
	

	return 0;


cancel_thread:
	pthread_cancel(poll_tid);	
	pthread_join(poll_tid, NULL);
release_control:
	revpi_control_release();

	return -1;
}


int main(int argc, char *argv[])
{
	int ret;
	const char *prgname;
	struct sigaction act;

	prgname = argv[0];

	if (geteuid()) {
		fprintf(stderr, "%s must be run as root\n", prgname);
		return -1;
	}

#if 0
	deamonize();
#endif

	fprintf(stderr, "daemon pid: %u\n", getpid());

	memset(&act, 0, sizeof(act));

	act.sa_handler = sighandler;

	ret = sigaction(SIGTERM, &act, NULL);
	if (ret) {
		// TODO: use logger facility
		fprintf(stderr, "failed to install sighandler: %s\n", strerror(errno));
		return -1;
	}

        // TODO: open logfile
                
	ret = setup_rt();
	if (ret < 0) 
      		exit(EXIT_FAILURE); 

	do_main();
       
	exit(EXIT_SUCCESS);
}

