# i2cselect
A quick selector utility for selecting a I2C-device bus on Raspberry Pi when using a TCA9548 Multiplexer

***

If you ever happened to use the Texas Instruments TCA9548 I²C-Multiplexer in combination with a Raspberry Pi, you might find it handy to simply activate the "i2c-mux"-Device-Tree Overlay that came right with the Raspbian Linux Distribution.
And this DTO indeed _is_ very useful and convenient!

The DTO basically adds the 8 additional I²C-Lines to your Device Tree as /dev/i2c-3 through /dev/i2c-10, while the one true I²C-Device of the Raspberry Pi remains on /dev/i2c-1.
Apart from the TCA9548 itself, devices connected to the I²C-Line of the TCA9548-Multiplexer which was accessed the last can also be seen and interacted with on the i2c-1 device, too. 

While some people consider the continuing existence of the i2c-1 bus under /dev as a shortcoming of this particular DTO, in some cases this might be a significant advantage - especially when dealing with some 3rd-Party-libraries trying to access a device only on i2c-1 whatsoever.

The mentioned advantage however, comes with another drawback itself, namely that the Kernel inevitably claims ownership of the TCA9548 (since it manages the device for you...) and therefore won't let you directly write to the TCA9548s registers in order to switch between the different lines.

You could switch lines by simply triggering a random read or write operation to another line and thereby causing the Kernel to switch lines, what doubtlessly works but may become sluggish and inflexible depending on your specific requirements.

The i2cselect utility works around this problem by simply opening the requested line for you and reading a single byte from the opened line*. More importantly, though, it lets you assign aliases for the lines either directly hardcoded at compile-time, from a config-file at runtime or even use both variants at the same time. This allows for a quick access by a simple call line `$> i2cselect ir-sensor` as an example to have the corresponding line (and its devices) appear on i2c-1.

The utility is written to be easy to integrate in scripts with different options available to control its behaviour and verbosity.
