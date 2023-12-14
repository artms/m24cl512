# STOP!!!

>[!CAUTION]
>If you're looking for Microchip 24LC512 linux kernel module - use upstream [at24](https://github.com/torvalds/linux/blob/master/drivers/misc/eeprom/at24.c#L3) module instead. 
>
>This module is strictly for learning purposes!
>
Device Tree section for 24LC512 using at24 module, make sure to put it into appropriate i2c section:
```
m24lc512_eeprom: eeprom@50 {
	compatible = "microchip,24lc512", "atmel,24c512";
	reg = <0x50>;
	size = 65536;
	address-width = 16;
	pagesize = 128;
};
```
# Notable bits

This module is specifically tailored for 24LC512 EEPROM chip hence:
1) Page size is upto 128 bytes though it is limit for write and not to reads but this will as well limit the time I2C bus is hogged so sticking with it.
2) Page write is 5ms maximum, chip acknowledges writes when you try to write next batch.
