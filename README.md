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

# Compilation
Install linux kernel headers, `gcc`, `make` on your favorite linux distribution and then run `make`. Module can be inserted using `insmod` and remove `rmmod`. Do keep in mind you will need to have device tree configured to make sure module detects hw. Device tree can be used the same as above though other than `reg` and `compatible` the rest of parameters are ignored.
