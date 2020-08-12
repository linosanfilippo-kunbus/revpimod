

#include "revpi_control_lib.h"

#include <sched.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>



#if 0
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif


struct revpi_control_setting test_settings[] = {
	{
		.type = REVPI_CTRL_SETTING_DIGITAL0,
		.value = 1,
	},
	{
		.type = REVPI_CTRL_SETTING_DIGITAL1,
		.value = 1,
	}, 
#if 1
	{
		.type = REVPI_CTRL_SETTING_DIGITAL2,
		.value = 0,
	}, 
#endif
	{
		.type = REVPI_CTRL_SETTING_DIGITAL3,
		.value = 1,
	}, 
#if 1
	{
		.type = REVPI_CTRL_SETTING_DIGITAL4,
		.value = 1,
	}, 
#endif
	{
		.type = REVPI_CTRL_SETTING_DIGITAL5,
		.value = 1,
	}, 
#if 1
	{
		.type = REVPI_CTRL_SETTING_DIGITAL6,
		.value = 0,
	}, 
#endif
	{
		.type = REVPI_CTRL_SETTING_DIGITAL7,
		.value = 1,
	}, 
#if 1
	{
		.type = REVPI_CTRL_SETTING_CORE_TEMP,
		.value = 0,
	}, 
#endif
};


int main()
{
	struct revpi_control_setting *result_settings;
	struct sched_param sched;
	int num_settings;
	int ret;
	int i;

        struct timespec start; 
        struct timespec end; 
        long delta;


	if (geteuid()) {
		fprintf(stderr, "You must be root to run this program!\n");
		return -1;
	}

	/* Set realtime priority */
	memset(&sched, 0, sizeof(sched));
	sched.sched_priority = 80;
	// TODO: try with SCHED_DEADLINE
	ret = sched_setscheduler(0, SCHED_RR, &sched);
	if (ret) {
		fprintf(stderr, "failed to set scheduling parameters: %s\n",
			strerror(errno));
		return -1;
	}

#if 0
	ret = revpi_control_init(REVPI_CTRL_ACCESS_DAEMON);
#endif

	ret = revpi_control_init(REVPI_CTRL_ACCESS_DIRECT);
	if (ret) {
		fprintf(stderr, "control init failed\n");
		return -1;
	}

	num_settings = sizeof(test_settings) / sizeof(struct revpi_control_setting);

	result_settings = malloc(sizeof(*result_settings) * num_settings);
	if (!result_settings) { 
		fprintf(stderr, "failed to allocate result settings\n");
		return -1;
	}

	/* first delete all outputs */
	for (i = 0; i < num_settings; i++) {
		result_settings[i].type = test_settings[i].type;	
		result_settings[i].value = 0;
	}

	fprintf(stderr, "clearing outputs...\n");
	revpi_control_write_settings(result_settings, num_settings);

	sleep(5);

	fprintf(stderr, "doing testloop...\n");

	clock_gettime(CLOCK_MONOTONIC, &start);
	revpi_control_write_settings(test_settings, num_settings);

	do {
		ret = revpi_control_read_settings(result_settings, num_settings);
	} while(result_settings[1].value == 0);

	clock_gettime(CLOCK_MONOTONIC, &end);


	delta = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);

	fprintf(stderr, "delta is %li\n", delta);

	for (i = 0; i < num_settings; i++) {
		fprintf(stderr, "type: %i, val: %i\n",
			test_settings[i].type, 
			test_settings[i].value);
	}

	revpi_control_release();

	return 0;
}

