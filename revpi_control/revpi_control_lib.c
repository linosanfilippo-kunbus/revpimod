
#include "../revpi_user.h"
#include "revpid.h"
#include "revpi_control_lib.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>


#define REVPI_SERVER_PATH 	"/var/revpi_daemon"
#define REVPI_DEVICE_PATH	"/dev/revpi_dev"
#define REVPI_CORE_TEMP_PATH	"/sys/class/thermal/thermal_zone0/temp"

#define D_CHAN_MASK 		(0x000000FF)
#define A_CHAN_MASK		(0x0000FF00)

struct revpi_control_data {
	volatile struct revpi_control_setting *settings;
	unsigned int access_type;
	int dev_fd;
	int core_temp_fd;
	int client_socket;
} ctrl_data;



static int read_core_temp(int *temp)
{
	int ret;
	char buffer[256];
	char *endptr;
	int val;

	ret = lseek(ctrl_data.core_temp_fd, SEEK_SET, 0);
	if (ret) {
		fprintf(stderr, "failed to set seek offset: %s\n",
			strerror(errno));
		return ret;
	}

	ret = read(ctrl_data.core_temp_fd, buffer, sizeof(buffer) - 1);
	if (ret < 0) {
		fprintf(stderr, "failed to read temperature file: %s\n",
			strerror(errno));
	} else if (ret == 0) {
		fprintf(stderr, "got EOF from temperature file\n");
	} else {
		buffer[ret] = 0;
	}

	errno = 0;

	val = strtoull(buffer, &endptr, 10);

	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) ||
	    (errno != 0 && val == 0)) {
		fprintf(stderr, "failed to get temp value: %s\n", strerror(errno));
		return -1;
	}

	if (endptr == buffer) {
               fprintf(stderr, "could not convert %s to number \n", buffer);
		return -1;
	}

	*temp = val;

	return 0;
}

int revpi_control_write_settings(struct revpi_control_setting *settings,
				 unsigned int num)
{
	struct revpi_cmd_channels *cmd;
	unsigned int num_chans = 0;
	struct revpi_channel *chan;
	bool is_valid = true;
	int ret = 0;
	int i;

	cmd = calloc(1, sizeof(*cmd) + num * sizeof(*chan));
	if (!cmd) {
		fprintf(stderr, "failed to allocate memory for channels: %s\n",
			strerror(errno));
		return -1;
	}

	chan = cmd->chans;

	for (i = 0; (i < num) && is_valid; i++) {
		if (settings[i].type & (A_CHAN_MASK | D_CHAN_MASK)) {

			if (settings[i].type & D_CHAN_MASK) {
				chan[num_chans].type = REVPI_CHANNEL_TYPE_DIGITAL;
				chan[num_chans].num = settings[i].type - 1;
				chan[num_chans].value = settings[i].value;
			} else {
				chan[num_chans].type = REVPI_CHANNEL_TYPE_ANALOG;
				chan[num_chans].num = ((settings[i].type - 1) >> 8);
				chan[num_chans].value = settings[i].value;
			}

			num_chans++;
		} else if (settings[i].type == REVPI_CTRL_SETTING_CORE_TEMP) {

		} else {
			is_valid = false;
		}
	}


	if (!is_valid) {
		fprintf(stderr, "one or more invalid settings\n");
		ret = -1;
		goto err_free;
	}

	if (num_chans) {
//		unsigned long long delta;
//		struct timespec start;
//		struct timespec end;

		cmd->num_chans = num_chans;

//		clock_gettime(CLOCK_MONOTONIC, &start);
		ret = ioctl(ctrl_data.dev_fd, REVPI_IOCTL_SET_VALUES, cmd);
//		clock_gettime(CLOCK_MONOTONIC, &end);
		if (ret) {
			fprintf(stderr, "failed to write channel values: %s\n",
				strerror(errno));
			goto err_free;
		}
//		delta = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
//		fprintf(stderr, "delta2 is %llu\n", delta);
	}


err_free:
	free(cmd);

	return ret;
}


static int revpi_control_read_settings_direct(struct revpi_control_setting *settings,
					      unsigned int num)
{
	struct glue_helper {
		struct revpi_control_setting *setting;
		struct revpi_channel *chan;
	} *glue;
	struct revpi_control_setting *core_temp = NULL;
	struct revpi_cmd_channels *cmd;
	unsigned int num_chans = 0;
	struct revpi_channel *chan;
	bool is_valid = true;
	int ret = 0;
	int i;

	cmd = calloc(1, sizeof(*cmd) + num * sizeof(*chan));
	if (!cmd) {
		fprintf(stderr, "failed to allocate memory for channels: %s\n",
			strerror(errno));
		return -1;
	}

	glue = calloc(num, sizeof(*glue));
	if (!glue) {
		fprintf(stderr, "failed to allocate memory for glue struct: %s\n",
			strerror(errno));
		ret = -1;
		goto err;
	}

	chan = cmd->chans;

	for (i = 0; (i < num) && is_valid; i++) {
		if (settings[i].type & (A_CHAN_MASK | D_CHAN_MASK)) {

			if (settings[i].type & D_CHAN_MASK) {
				chan[num_chans].type = REVPI_CHANNEL_TYPE_DIGITAL;
				chan[num_chans].num = settings[i].type - 1;
			} else {
				chan[num_chans].type = REVPI_CHANNEL_TYPE_ANALOG;
				chan[num_chans].num = ((settings[i].type - 1) >> 8);
			}

			glue[num_chans].setting = &settings[i];
			glue[num_chans].chan = &chan[num_chans];
			num_chans++;
		} else if (settings[i].type == REVPI_CTRL_SETTING_CORE_TEMP) {
			core_temp = &settings[i];
		} else {
			is_valid = false;
		}
	}


	if (!is_valid) {
		fprintf(stderr, "one or more invalid settings\n");
		ret = -1;
		goto err2;
	}

	if (num_chans) {
//		unsigned long long delta;
//		struct timespec start;
//		struct timespec end;

		cmd->num_chans = num_chans;

//		clock_gettime(CLOCK_MONOTONIC, &start);
		ret = ioctl(ctrl_data.dev_fd, REVPI_IOCTL_GET_VALUES, cmd);
//		clock_gettime(CLOCK_MONOTONIC, &end);
		if (ret) {
			fprintf(stderr, "failed to read channel values: %s\n",
				strerror(errno));
			goto err2;
		}
		for (i = 0; i < num_chans; i++) {
			glue[i].setting->value = glue[i].chan->value;
		}
//		delta = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
//		fprintf(stderr, "delta2 is %llu\n", delta);
	}

	if (core_temp) {
		int temp;

		ret = read_core_temp(&temp);
		if (ret)
			goto err2;

//		fprintf(stderr, "got temp value: %i\n", temp);

		core_temp->value = temp;

	}

err2:
	free(glue);
err:
	free(cmd);

	return ret;
}

#if 0
static void dump_settings(struct revpi_control_setting *settings, int num)
{
	int i;

	for (i = 0; i < num; i++) {
		fprintf(stderr, "Setting %i: type=%u, val=%u\n", i,
			settings[i].type, settings[i].value);
	}
}
#endif

static int revpi_control_read_settings_daemon(struct revpi_control_setting *settings,
					      unsigned int num)
{
	bool is_valid = true;
	unsigned int set_num;
	int i;


#if 0
	fprintf(stderr, "reading daemon settings\n");
	dump_settings(ctrl_data.settings, num);
#endif

	for (i = 0; (i < num) && is_valid; i++) {
		if (settings[i].type & (A_CHAN_MASK | D_CHAN_MASK)) {

			if (settings[i].type & D_CHAN_MASK) {
				if ((settings[i].type | D_CHAN_MASK) != D_CHAN_MASK) {
					is_valid = false;
					continue;
				}
				set_num = settings[i].type;
			
				// TODO: use variable values
				if (set_num > 8) {
					is_valid = false;
					continue;
				}
				set_num--;
				settings[i].value = ctrl_data.settings[set_num].value;
			} else {
				if ((settings[i].type | A_CHAN_MASK) != A_CHAN_MASK) {
					is_valid = false;
					continue;
				}
				set_num = settings[i].type >> 8;
			
				// TODO: use variable values
				if (set_num > 8) {
					is_valid = false;
					continue;
				}
				set_num--;
				settings[i].value = ctrl_data.settings[8 + set_num].value;
			}
		} else if (settings[i].type == REVPI_CTRL_SETTING_CORE_TEMP) {

		} else {
			is_valid = false;
		}
	}

	if (!is_valid) {
		fprintf(stderr, "one or more invalid settings\n");
		return -1;
	}

	return 0;
}

int revpi_control_read_settings(struct revpi_control_setting *settings, unsigned int num)
{
	int ret;

	if (ctrl_data.access_type == REVPI_CTRL_ACCESS_DIRECT) {
		ret = revpi_control_read_settings_direct(settings, num);
	} else {
		ret = revpi_control_read_settings_daemon(settings, num);
	}

	return ret;
}

static int revpi_control_init_daemon_communication(void)
{
	// TODO: read from parsed config
	int comm_file;
	struct stat st;

	comm_file = open(REVPID_COMM_FILE, O_RDONLY);
	if (comm_file < 0) {
		fprintf(stderr, "failed to open communication file %s: %s\n",
			REVPID_COMM_FILE, strerror(errno));
		return -1;
	}

	if (fstat(comm_file, &st)) {
		fprintf(stderr, "failed to get size of communication file: %s\n",
			strerror(errno));
		goto err_mmap;
	}

	ctrl_data.settings = mmap(NULL, st.st_size, PROT_READ , MAP_SHARED, comm_file, 0);

	if (ctrl_data.settings == MAP_FAILED) {
		fprintf(stderr, "failed to map communication file %s: %s\n",
			REVPID_COMM_FILE, strerror(errno));
		goto err_mmap;
	}

	fprintf(stderr, "MAPPING revpid memory area done\n");

return 0;

err_mmap:
	close(comm_file);

	return -1;
}

int revpi_control_init(unsigned int type)
{

	if (type == REVPI_CTRL_ACCESS_DAEMON) {
		if (revpi_control_init_daemon_communication()) {
			fprintf(stderr, "failed to initialize socket connection\n");
			return -1;
		}
	} else if (type == REVPI_CTRL_ACCESS_DIRECT) {

	} else {
		fprintf(stderr, "Unsupported access type %u\n", type);
		return -1;
	}

	ctrl_data.dev_fd = open(REVPI_DEVICE_PATH, 0, O_RDONLY);
	if (ctrl_data.dev_fd < 0) {
		fprintf(stderr, "failed to open %s: %s", REVPI_DEVICE_PATH, strerror(errno));
		return -1;
	}

	ctrl_data.access_type = type;
	ctrl_data.core_temp_fd = open(REVPI_CORE_TEMP_PATH, 0, O_RDONLY);

	if (ctrl_data.core_temp_fd < 0) {
		fprintf(stderr, "failed to open %s: %s", REVPI_CORE_TEMP_PATH, strerror(errno));
		goto close_dev;
	}

	return 0;

close_dev:
	close(ctrl_data.dev_fd);

	return -1;
}

void revpi_control_release(void)
{
	close(ctrl_data.core_temp_fd);
	if (ctrl_data.access_type == REVPI_CTRL_ACCESS_DIRECT) {
		close(ctrl_data.dev_fd);

	} else {

	}
}
