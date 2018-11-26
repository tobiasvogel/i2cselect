# i2cselect
A quick selector utility for selecting a I2C-device bus on Raspberry Pi when using a TCA9548 Multiplexer

===

If you ever happened to use the Texas Instruments TCA9548 I²C-Multiplexer in combination with a Raspberry Pi, you might find it handy to simply activate the "i2c-mux"-Device-Tree Overlay that came right with the Raspbian Linux Distribution.
And this DTO indeed _is_ very useful and convenient!

The DTO basically adds the 8 additional I²C-Lines to your Device Tree as /dev/i2c-3 through /dev/i2c-10, while the one true I²C-Device of the Raspberry Pi remains on /dev/i2c-1.
Apart from the TCA9548 itself, devices connected to the I²C-Line of the TCA9548-Multiplexer which was access the last can also be seen and interacted with on the i2c-1 device. 

While some people consider the remainder of the i2c-1 bus under /dev as a shortcoming of this particular DTO, in some cases this might be a significant advantage - especially when dealing with some 3rd-Party-libraries trying to access a device only on i2c-1 whatsoever.

The mentioned advantage however, comes at the price of another drawback, namely that the Kernel inevitably claims ownership of the TCA9548 (hence it manages the device for you...) and therefore won't let you directly write to the TCA9548s registers in order to switch between the different lines.

You could switch lines by simply triggering a random read or write operation to another line and thereby causing the Kernel to switch lines, what doubtlessly works but may become annoying depending on your specific requirements.
