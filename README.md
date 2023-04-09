# AT32_InputTest
Simple test program for timer input capture, derived from two existing at32 examples.

I'm trying to understand timer input capture on the AT32. This program was made by merging two examples, one of which generates a pwm output signal on pin B08 at 36kHz, the other uses input capture to read from a pin and report the frequency. This works when using the original configuration of the input timer (timer3, ch2, A07) but fails for several other configurations.

Output is on pin A09 at 115200 baud.

The original examples are https://github.com/ArteryTek/AT32F435_437_Firmware_Library/tree/master/project/at_start_f435/examples/tmr/input_capture for the input and https://github.com/ArteryTek/AT32F435_437_Firmware_Library/tree/master/project/at_start_f435/examples/tmr/pwm_output_tmr10 for output. Grab one of these for the bsp/firmware infrastructure and drop the files in this project over the contents of the user directory.
