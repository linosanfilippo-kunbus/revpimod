
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/iio/iio.h>
#include <linux/iio/consumer.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#include <linux/gpio/machine.h>
#include <linux/spi/spi.h>
#include <linux/workqueue.h>

#include "revpi_user.h"


#define REVPI_DEV		"revpi_dev"



struct revpi_device {
	struct class *class;
	struct device *dev;
	dev_t devnum;
	struct cdev cdev;
#if 0
	unsigned int num_io_chans;
	unsigned int num_aio_chans;
#endif
	struct iio_dev *ain_dev;
#if 0
	struct device *ain_dev;
#endif
	struct iio_dev *out_dev;
	struct iio_channel *ain_chans;
	struct iio_channel *aout_chans;
	struct gpio_descs *din_descs;
	struct gpio_descs *dout_descs;
	int dout_vals[8];
	bool poll_dout;
	struct mutex dout_lock;
	struct workqueue_struct *dout_wq;
	struct work_struct dout_work;
	u64 avg_latency;
	u64 max_latency;
	u64 num_cycles;	
} revpi;

static const struct iio_map revpi_compact_ain[] = {
	IIO_MAP("ain0", REVPI_DEV, "ain0"),
	IIO_MAP("ain1", REVPI_DEV, "ain1"),
	IIO_MAP("ain2", REVPI_DEV, "ain2"),
	IIO_MAP("ain3", REVPI_DEV, "ain3"),
	IIO_MAP("ain4", REVPI_DEV, "ain4"),
	IIO_MAP("ain5", REVPI_DEV, "ain5"),
	IIO_MAP("ain6", REVPI_DEV, "ain6"),
	IIO_MAP("ain7", REVPI_DEV, "ain7"),
	IIO_MAP("ain0_rtd", REVPI_DEV, "ain0_rtd"),
	IIO_MAP("ain1_rtd", REVPI_DEV, "ain1_rtd"),
	IIO_MAP("ain2_rtd", REVPI_DEV, "ain2_rtd"),
	IIO_MAP("ain3_rtd", REVPI_DEV, "ain3_rtd"),
	IIO_MAP("ain4_rtd", REVPI_DEV, "ain4_rtd"),
	IIO_MAP("ain5_rtd", REVPI_DEV, "ain5_rtd"),
	IIO_MAP("ain6_rtd", REVPI_DEV, "ain6_rtd"),
	IIO_MAP("ain7_rtd", REVPI_DEV, "ain7_rtd"),
	{ }
};

static struct gpiod_lookup_table revpi_compact_gpios = {
	.dev_id = REVPI_DEV,
	.table  = { GPIO_LOOKUP_IDX("max31913", 0, "din", 0, 0),
		    GPIO_LOOKUP_IDX("max31913", 1, "din", 1, 0),
		    GPIO_LOOKUP_IDX("max31913", 2, "din", 2, 0),
		    GPIO_LOOKUP_IDX("max31913", 3, "din", 3, 0),
		    GPIO_LOOKUP_IDX("max31913", 4, "din", 4, 0),
		    GPIO_LOOKUP_IDX("max31913", 5, "din", 5, 0),
		    GPIO_LOOKUP_IDX("max31913", 6, "din", 6, 0),
		    GPIO_LOOKUP_IDX("max31913", 7, "din", 7, 0),
		    GPIO_LOOKUP_IDX("74hc595", 0, "dout", 0, 0),
		    GPIO_LOOKUP_IDX("74hc595", 1, "dout", 1, 0),
		    GPIO_LOOKUP_IDX("74hc595", 2, "dout", 2, 0),
		    GPIO_LOOKUP_IDX("74hc595", 3, "dout", 3, 0),
		    GPIO_LOOKUP_IDX("74hc595", 4, "dout", 4, 0),
		    GPIO_LOOKUP_IDX("74hc595", 5, "dout", 5, 0),
		    GPIO_LOOKUP_IDX("74hc595", 6, "dout", 6, 0),
		    GPIO_LOOKUP_IDX("74hc595", 7, "dout", 7, 0),
		  },
};

#define NUM_AIN_CHANS	8

static void revpi_dout_work(struct work_struct *work)
{
	bool rearm = false;

	mutex_lock(&revpi.dout_lock);
	rearm = revpi.poll_dout;
	gpiod_set_array_value_cansleep(revpi.dout_descs->ndescs,
				       revpi.dout_descs->desc,
				       revpi.dout_vals);
	mutex_unlock(&revpi.dout_lock);

	if (rearm)
		queue_work(revpi.dout_wq, &revpi.dout_work);
}

static int revpi_get_digital_values(struct revpi_channel *chans, unsigned int num)
{
//	printk(KERN_INFO "GET DIGITAL VALUE\n");
	int val[8];
	int i;
	int ret;
	

#if 1
	ret = gpiod_get_array_value_cansleep(revpi.din_descs->ndescs,
					     revpi.din_descs->desc, val);
	if (ret)
		return ret;
#else
	ret = 0;
#endif

#if 1
	for (i = 0; i < num; i++) {
		if (chans[i].num >= revpi.din_descs->ndescs) {
			ret = -EINVAL;
			break;
		}
		chans[i].value = val[chans[i].num];
	}
#endif

//	printk(KERN_INFO "READ DIGITAL VALUE: %i\n", chan->value);

	return ret;
}

static int revpi_set_digital_values(struct revpi_channel *chans, unsigned int num)
{
	int val[8];
	int i;
	int ret = 0;

	memset(val, 0, sizeof(val));

	for (i = 0; i < num; i++) {
		if (chans[i].num >= revpi.dout_descs->ndescs) {
			ret = -EINVAL;
			break;
		}
#if 0
		val[i] = !!chans[i].value;
#else
		val[chans[i].num] = chans[i].value;
#endif
		printk(KERN_INFO "Setting DOUT VAL %i to %u\n", chans[i].num, val[chans[i].num]);
	}

	gpiod_set_array_value_cansleep(revpi.dout_descs->ndescs,
				       revpi.dout_descs->desc, val);

//	printk(KERN_INFO "SETTING VALUES!!\n");

	mutex_lock(&revpi.dout_lock);
	memcpy(revpi.dout_vals, val, sizeof(revpi.dout_vals));
	mutex_unlock(&revpi.dout_lock);


	return ret;
}

static int revpi_get_analog_value(struct revpi_channel *chan)
{
	int rawval;
	int ret;

	if (chan->num >= NUM_AIN_CHANS)
		return -EINVAL;

	ret = iio_read_channel_raw(&revpi.ain_chans[chan->num], &rawval);
	if (ret < 0) {
		printk(KERN_ERR "failed to read raw val: %i\n", ret);
		return ret;
	}


//	printk(KERN_INFO "READ RAW VALUE: %i\n", rawval);
	chan->value = rawval;		

	return 0;
}


static int revpi_get_values(struct revpi_channel *chans, unsigned int num)
{
	int ret = 0;

	if (chans->type == REVPI_CHANNEL_TYPE_DIGITAL) {
		ret = revpi_get_digital_values(chans, num);

	} else if (chans->type == REVPI_CHANNEL_TYPE_DIGITAL) {

	} else
		ret = -EINVAL;


#if 0
	for (i = 0; i < num; i++) {
		switch(chans[i].type) {
		case REVPI_CHANNEL_TYPE_ANALOG:
			printk(KERN_INFO "get analog value for channel %i\n", chans[i].num);
			ret = revpi_get_analog_value(chans);
			break;
		case REVPI_CHANNEL_TYPE_DIGITAL:
			ret = revpi_get_digital_value(chans);
			break;
		default: 
			ret = -EINVAL;
		}

	}
#endif

	return ret;
}

static int revpi_set_values(struct revpi_channel *chans, unsigned int num)
{
	int ret;

//	dev_info(revpi.dev, "CHANNELS SET VALUES\n");

	if (chans->type == REVPI_CHANNEL_TYPE_DIGITAL) {
		ret = revpi_set_digital_values(chans, num);
	} else if (chans->type == REVPI_CHANNEL_TYPE_DIGITAL) {

	} else
		ret = -EINVAL;

	return ret;
}

static long revpi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct revpi_cmd_channels *usr_chan_cmd = (struct revpi_cmd_channels __user *) arg;
	struct revpi_cmd_channels chan_cmd;
	struct revpi_channel *chans;
	size_t copylen;
	u64 start;
	u64 end;
	u64 diff;
	int ret;

//	printk(KERN_INFO "GET VALUES IOCTL (usr ptr 1: %p)\n", (void *) arg);
//	printk(KERN_INFO "GET VALUES IOCTL (usr ptr: %p)\n", usr_chan_cmd);

	start = ktime_to_ns(ktime_get());

	if (copy_from_user(&chan_cmd, usr_chan_cmd, sizeof(chan_cmd))) {
		return -EFAULT;
	}

//	printk(KERN_INFO "got %i chans in command\n", chan_cmd.num_chans);

	if (!chan_cmd.num_chans)
		return 0;

	// TODO: proper MAX VALUE
	if (chan_cmd.num_chans > 26)
		return -EINVAL;

	copylen = chan_cmd.num_chans * sizeof(struct revpi_channel);

	chans = kmalloc(copylen, GFP_KERNEL);
	if (!chans)
		return -ENOMEM;

	if (copy_from_user(chans, usr_chan_cmd->chans, copylen)) {
		ret = -EFAULT;
		goto free_chans;
	}

#if 0
	printk(KERN_INFO "got channel num: %u, type: %u, value: %i\n",
		chans[0].num,
		chans[0].type,
		chans[0].value);
#endif

	switch(cmd) {
	case REVPI_IOCTL_GET_VALUES:
		ret = revpi_get_values(chans, chan_cmd.num_chans);
		if (ret)
			break;
		if (copy_to_user(usr_chan_cmd->chans, chans, copylen))
			ret = -EFAULT;
		break;
	case REVPI_IOCTL_SET_VALUES:
		ret = revpi_set_values(chans, chan_cmd.num_chans);
		break;
	default:
		ret = -EOPNOTSUPP;
	}

free_chans:
	kfree(chans);
			
	end = ktime_to_ns(ktime_get());

	//printk(KERN_INFO "latency: %llu\n", end - start);

	diff = end - start;

	if (diff > revpi.max_latency)
		revpi.max_latency = diff;

	revpi.avg_latency += diff;
	revpi.num_cycles++;
	

	return ret;
}

static struct file_operations revpi_fops = {
	owner: THIS_MODULE,
	open: nonseekable_open,
	unlocked_ioctl: revpi_ioctl,
#if 0
	write:	revpi_write,
	llseek:piControlSeek,
	release: revpi_close,
#endif
};

static int match_name(struct device *dev, void *data)
{
	const char *name = data;

	if (dev->bus == &spi_bus_type)
		return sysfs_streq(name, to_spi_device(dev)->modalias);
	else if (dev->bus == &iio_bus_type)
		return sysfs_streq(name, dev_to_iio_dev(dev)->name);
	else
		return sysfs_streq(name, dev_name(dev));
}

static int revpi_init_analog_channels(void)
{
	struct device *dev;
	struct iio_dev *iio_dev;
	struct iio_channel *iio_chans;
	int ret;

	dev = bus_find_device(&iio_bus_type, NULL, "ain_muxed", match_name);
	if (!dev) {
		dev_err(revpi.dev, "cannot find analog input device\n");
		return -ENODEV;
	}

	iio_dev = dev_to_iio_dev(dev);	

	ret = iio_map_array_register(iio_dev, (struct iio_map *) revpi_compact_ain);
	if (ret) {
		dev_err(revpi.dev, "failed to register map array\n");
		goto put_ain;
	}
	
	iio_chans = iio_channel_get_all(revpi.dev);
	if (IS_ERR(iio_chans)) {
		dev_err(revpi.dev, "failed to acquire analog input chans\n");
		ret = PTR_ERR(iio_chans);
		goto unregister_map;
	}

	{
		unsigned int num_chans = 0;

		while (iio_chans[num_chans].indio_dev)
			num_chans++;

		printk(KERN_INFO "FOUND %i IIO CHANS\n", num_chans);
	}

	revpi.ain_dev = iio_dev;
	revpi.ain_chans = iio_chans;


	return 0;

unregister_map:
	iio_map_array_unregister(iio_dev);
put_ain:
	iio_device_put(iio_dev);	

	return ret;
}

static int revpi_init_digital_channels(void)
{
	struct gpio_descs *din;
	struct gpio_descs *dout;
	int ret;

	gpiod_add_lookup_table(&revpi_compact_gpios);

	din = gpiod_get_array(revpi.dev, "din", GPIOD_ASIS);
	if (IS_ERR(din)) {
		dev_err(revpi.dev, "cannot acquire digital input pins\n");
		ret = PTR_ERR(din);
		goto remove_table;
	}
	// TODO: set bouning

	printk(KERN_INFO "GOT %u DIGIT IN\n", din->ndescs);

	dout = gpiod_get_array(revpi.dev, "dout", GPIOD_ASIS);
	if (IS_ERR(dout)) {
		pr_err("cannot acquire digital output pins\n");
		ret = PTR_ERR(dout);
		goto release_din;
	}

	revpi.din_descs = din;
	revpi.dout_descs = dout;

	return 0;

release_din:
	gpiod_put_array(din);
remove_table:
	gpiod_remove_lookup_table(&revpi_compact_gpios);

	return ret;
}

static void revpi_deinit_analog_channels(void)
{
	iio_channel_release_all(revpi.ain_chans);
	iio_map_array_unregister(revpi.ain_dev);
	iio_device_put(revpi.ain_dev);
}

static void revpi_deinit_digital_channels(void)
{
	gpiod_put_array(revpi.dout_descs);
	gpiod_put_array(revpi.din_descs);
	gpiod_remove_lookup_table(&revpi_compact_gpios);
}


static int revpi_init_channels(void)
{
	int ret;

	ret = revpi_init_analog_channels();
	if (ret)
		return ret;

	ret = revpi_init_digital_channels();
	if (ret)
		goto deinit_analog;

	return 0;

deinit_analog:
	revpi_deinit_analog_channels();

	return ret;
}

static void revpi_deinit_channels(void)
{
	revpi_deinit_analog_channels();
	revpi_deinit_digital_channels();
}

static int __init revpi_init(void)
{
	struct class *class;
	struct device *dev;
	dev_t devnum;
	int ret;

	printk(KERN_INFO "PROBE CALLED\n");

	ret = alloc_chrdev_region(&devnum, 0, 1, "revpi");
	if (ret) {
		pr_err("cannot allocate char device numbers: %i\n", ret);
		return ret;
	}
	
	class = class_create(THIS_MODULE, "revpi_class");
	if (IS_ERR(class)) {
		ret = PTR_ERR(class);
		pr_err("failed to create class for revpi: %i\n", ret);
		goto release_chrdev;
	}

	dev = device_create(class, NULL, devnum, NULL, REVPI_DEV);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		pr_err("failed to create revpi device: %i\n", ret);
		goto destroy_class;
	}

	cdev_init(&revpi.cdev, &revpi_fops);

	ret = cdev_add(&revpi.cdev, devnum, 1);
	if (ret) {
		dev_err(dev, "failed to add cdev: %i", ret);
		goto destroy_device;
	}

	revpi.dev = dev;
	revpi.class = class;
	revpi.devnum = devnum;

	ret = revpi_init_channels();
	if (ret) {
		dev_err(dev, "failed to init channels: %i", ret);
		goto del_cdev;
	}

	mutex_init(&revpi.dout_lock);
	revpi.dout_wq = create_workqueue(REVPI_DEV "/dout_wq");
	if (!revpi.dout_wq) {
		goto deinit_channels;
	}
	INIT_WORK(&revpi.dout_work, revpi_dout_work);
	mutex_lock(&revpi.dout_lock);
	revpi.poll_dout = true;
	mutex_unlock(&revpi.dout_lock);
	queue_work(revpi.dout_wq, &revpi.dout_work);

	revpi.avg_latency = 0;
	revpi.max_latency = 0;
	revpi.num_cycles = 0;

	printk(KERN_INFO "ALL DONE\n");

	return 0;

deinit_channels:
	revpi_deinit_channels();
del_cdev:
	cdev_del(&revpi.cdev);
destroy_device:
	device_destroy(class, devnum);
destroy_class:
	class_destroy(class);
release_chrdev:
	unregister_chrdev_region(devnum, 1);

	return ret;
}

static void __exit revpi_exit(void)
{
		
		

	mutex_lock(&revpi.dout_lock);
	
	printk(KERN_INFO "AVG latency 1: %llu \n", revpi.avg_latency);
	do_div(revpi.avg_latency, revpi.num_cycles);
	revpi.poll_dout = false;
	mutex_unlock(&revpi.dout_lock);

	printk(KERN_INFO "MAX latency: %llu (%llu cycles) \n", revpi.max_latency, revpi.num_cycles);
	printk(KERN_INFO "AVG latency: %llu \n", revpi.avg_latency);

	cancel_work_sync(&revpi.dout_work);
	destroy_workqueue(revpi.dout_wq);
	revpi_deinit_channels();

	cdev_del(&revpi.cdev);
	device_destroy(revpi.class, revpi.devnum);
	class_destroy(revpi.class);
	unregister_chrdev_region(revpi.devnum, 1);
}


module_init(revpi_init);
module_exit(revpi_exit);


MODULE_AUTHOR("Lino Sanfilippo <l.sanfilippo@kunbus.com>");
MODULE_DESCRIPTION("REVPI Input/Output driver module");
MODULE_LICENSE("GPL v2");

