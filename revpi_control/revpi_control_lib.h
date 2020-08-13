


#ifndef _REVPI_CONTROL_H
#define _REVPI_CONTROL_H

#define REVPI_CTRL_ACCESS_DIRECT	1
#define REVPI_CTRL_ACCESS_DAEMON	2


#define REVPI_CTRL_SETTING_DIGITAL0	0x00001
#define REVPI_CTRL_SETTING_DIGITAL1	0x00002
#define REVPI_CTRL_SETTING_DIGITAL2	0x00003
#define REVPI_CTRL_SETTING_DIGITAL3	0x00004
#define REVPI_CTRL_SETTING_DIGITAL4	0x00005
#define REVPI_CTRL_SETTING_DIGITAL5	0x00006
#define REVPI_CTRL_SETTING_DIGITAL6	0x00007
#define REVPI_CTRL_SETTING_DIGITAL7	0x00008
#define REVPI_CTRL_SETTING_ANALOG0	0x00100
#define REVPI_CTRL_SETTING_ANALOG1	0x00200
#define REVPI_CTRL_SETTING_ANALOG2	0x00300
#define REVPI_CTRL_SETTING_ANALOG3	0x00400
#define REVPI_CTRL_SETTING_ANALOG4	0x00500
#define REVPI_CTRL_SETTING_ANALOG5	0x00600
#define REVPI_CTRL_SETTING_ANALOG6	0x00700
#define REVPI_CTRL_SETTING_ANALOG7	0x00800
#define REVPI_CTRL_SETTING_CORE_TEMP	0x10000


/**
 * struct revpi_control_setting - Specify which type to read/write
 *
 * @type: the type of information to read or write.
 * @value: the read value as a result of a call to
 *         revpi_control_read_settings() or the value to be written by
 *         means of revpi_control_write_settings().
 */
struct revpi_control_setting {
	unsigned int type;
	unsigned int value;
};

/**
 * revpi_control_init() - Initialize the revpi control library
 * @type: The access type to use for reading digital/analog input signals
 *
 * Initialize the revpi control api for direct access to the revpimod
 * kernel module (REVPI_CTRL_ACCESS_DIRECT) or access by means of the
 * revpid daemon (REVPI_CTRL_ACCESS_DAEMON). The latter requires the
 * daemon to be running.
 */
int revpi_control_init(unsigned int type);

/**
 * revpi_control_release() - Release the revpi control library
 *
 */
void revpi_control_release(void);

/**
 * revpi_control_write_settings() - Write values to digital/analog outputs.
 *
 * @settings an array of control settings that specify which
	     digital/analog output lines to write.
 */
int revpi_control_write_settings(struct revpi_control_setting *settings, unsigned int num);


/**
 * revpi_control_read_settings() - Read values from digital/analog inputs.
 *
 * @settings an array of control settings that specify which
	     digital/analog input lines to read.
 */
int revpi_control_read_settings(struct revpi_control_setting *settings, unsigned int num);

#endif /* _REVPI_CONTROL_H */
