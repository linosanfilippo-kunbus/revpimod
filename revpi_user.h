
#ifndef _REVPI_USER_H
#define _REVPI_USER_H

#include <linux/types.h>

#define REVPI_IOCTL_MAGIC 'R'
#define REVPI_IOCTL_GET_VALUES		_IOWR(REVPI_IOCTL_MAGIC, 0, struct revpi_command *)
#define REVPI_IOCTL_SET_VALUES		_IOWR(REVPI_IOCTL_MAGIC, 1, struct revpi_command *)


struct revpi_channel {
	__u32 num;
#define REVPI_CHANNEL_TYPE_ANALOG	0
#define REVPI_CHANNEL_TYPE_DIGITAL	1
	__u32 type;
	__s32 value;
}__attribute__((packed));


struct revpi_cmd_channels {
	__u32 num_chans;
	struct revpi_channel chans[0]; 
} __attribute__((packed));


#endif /* _REVPI_USER_H */
