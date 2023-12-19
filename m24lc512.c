/*
 *  EEPROM Microchip 24LC512 compatible module
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kmod.h>

#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/nvmem-provider.h>

#define M24LC512 0
// size of EEPROM
#define M24LC512_SIZE 65536
#define M24LC512_PAGE_SIZE 128
// chip writes data in 5ms max
#define M24LC512_WRITE_TIME_MS 5

static const struct i2c_device_id m24lc512_idtable[] = {
	{ "24lc512", M24LC512 },
	{ /* END OF LIST */ },
};

// Device Tree matches
static const struct of_device_id m24lc512_of_match[] = {
	{ .compatible = "microchip,24lc512" },
	{ /* END OF LIST */ },
};

struct m24lc512_i2c_data {
	struct mutex lock;

	struct regmap *regmap;
};

int m24lc512_i2c_read(void *priv, unsigned int offset, void *val, size_t count)
{
	struct m24lc512_i2c_data *data = priv;
	int ret;

	if (count == 0) {
		return 0;
	}
	
	if (offset + count > M24LC512_SIZE) {
		return -EINVAL;
	}

	mutex_lock(&data->lock);

	for (unsigned int i=0; i < count; i = i + M24LC512_PAGE_SIZE) {
		int bytes_to_read = count-i > M24LC512_PAGE_SIZE ? M24LC512_PAGE_SIZE : count-i;
		ret = regmap_bulk_read(data->regmap, offset+i, val+i, bytes_to_read);
		if (ret != 0) {
			break;
		}
	}

	mutex_unlock(&data->lock);

	return ret;
}

int m24lc512_i2c_write(void *priv, unsigned int offset, void *val, size_t count) {
	struct m24lc512_i2c_data *data = priv;
	int ret;

	if (count == 0) {
		return 0;
	}
	
	if (offset + count > M24LC512_SIZE) {
		return -EINVAL;
	}

	mutex_lock(&data->lock);

	for (unsigned int i=0; i < count; i = i + M24LC512_PAGE_SIZE) {
		int bytes_to_write = count-i > M24LC512_PAGE_SIZE ? M24LC512_PAGE_SIZE : count-i;
		ret = regmap_bulk_write(data->regmap, offset+i, val+i, bytes_to_write);
		if (ret != 0) {
			break;
		}
		// 24LC512 specification says that write will finish in 5ms max hence wait between 5ms to 6ms
		usleep_range(M24LC512_WRITE_TIME_MS*1000, (M24LC512_WRITE_TIME_MS+1)*1000);
	}

	mutex_unlock(&data->lock);

	return ret;
}

static int m24lc512_i2c_probe(struct i2c_client *client)
{
	unsigned char buf[1];
	int ret;
	struct device *dev = &client->dev;

	struct regmap *regmap;
	const struct regmap_config regmap_config = {
		.reg_bits        = 16,
		.val_bits        = 8,
		.disable_locking = true,
		.can_sleep       = true,
	};

	struct nvmem_device *nvmem;
	struct nvmem_config nvmem_config = {
		.dev       = dev,
		.owner     = THIS_MODULE,
		.type      = NVMEM_TYPE_EEPROM,
		.size      = M24LC512_SIZE,
		.word_size = 1,
		.reg_read  = m24lc512_i2c_read,
		.reg_write = m24lc512_i2c_write,
	};
	struct m24lc512_i2c_data *data;

	dev_info(dev, "starting probing\n");
	// we only care for I2C with full implementation of I2C
	dev_info(dev, "checking I2C functionality\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(dev, "I2C adapter doesn't have full I2C functionality\n");
		return -EIO;
	}

	dev_info(dev, "checking device presence on the bus\n");
	ret = i2c_master_recv(client, buf, 1);
	if (ret != 1) {
		dev_err(dev, "device is not responding\n");
		return -ENODEV;
	}

	dev_info(dev, "preparing regmap to read/write data\n");
	regmap = devm_regmap_init_i2c(client, &regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(dev, "unable to setup regmap\n");
		return PTR_ERR(regmap);
	}

	data = devm_kzalloc(dev, sizeof(struct m24lc512_i2c_data), GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}
	i2c_set_clientdata(client, data);

	mutex_init(&data->lock);
	data->regmap = regmap;

	dev_info(dev, "registering nvmem subsystem\n");
	nvmem_config.priv = data;
	nvmem = devm_nvmem_register(dev, &nvmem_config);
	if (IS_ERR(nvmem)) {
		dev_err(dev, "failed to register with nvmem subsystem\n");
		return PTR_ERR(nvmem);
	}

	return 0;
}

static void m24lc512_i2c_remove(struct i2c_client *client)
{
	dev_info(&client->dev, "removing\n");
}

static struct i2c_driver m24lc512_i2c_driver = {
	.driver       = {
		.name = "m24lc512",
		.of_match_table = of_match_ptr(m24lc512_of_match),
	},
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,6,0)
	.probe_new    = m24lc512_i2c_probe,
#else
	.probe        = m24lc512_i2c_probe,
#endif
	.remove       = m24lc512_i2c_remove,
	.id_table     = m24lc512_idtable,
};

static int __init m24lc512_i2c_init(void)
{
	pr_info("module m24lc512 init\n");
	return i2c_add_driver(&m24lc512_i2c_driver);
}
module_init(m24lc512_i2c_init);

static void __exit m24lc512_i2c_exit(void)
{
	pr_info("module m24lc512 exit\n");
	i2c_del_driver(&m24lc512_i2c_driver);
}
module_exit(m24lc512_i2c_exit);

MODULE_AUTHOR("Arturas Moskvinas <arturas.moskvinas@gmail.com>");
MODULE_DESCRIPTION("Microchip 24LC512 EEPROM compatible module");
MODULE_LICENSE("GPL v2");
