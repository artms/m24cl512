/*
 *  EEPROM Microchip 24LC512 compatible module
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kmod.h>

#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/regmap.h>

#define M24LC512 0

static const struct i2c_device_id m24lc512_idtable[] = {
	{ "24lc512", M24LC512 },
	{ /* END OF LIST */ },
};

// Device Tree matches
static const struct of_device_id m24lc512_of_match[] = {
	{ .compatible = "microchip,24lc512" },
	{ /* END OF LIST */ },
};

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
