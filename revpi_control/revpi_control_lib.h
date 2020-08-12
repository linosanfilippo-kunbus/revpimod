


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


struct revpi_control_setting {
	unsigned int type;
	unsigned int value;
};

int revpi_control_init(unsigned int type);
void revpi_control_release(void);
int revpi_control_write_settings(struct revpi_control_setting *settings, unsigned int num);
int revpi_control_read_settings(struct revpi_control_setting *settings, unsigned int num);

#endif /* _REVPI_CONTROL_H */
