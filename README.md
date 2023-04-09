# AT32_InputTest
Simple test program for timer input capture, derived from at32 examples

I'm trying to understand timer input capture on the AT32. This program was made by merging two examples, one of which generates a pwm output signal on pin B08 at 36kHz, the other uses input capture to read from a pin and report the frequency. This works when using the original configuration of the input timer (timer3, ch2, A07) but fails for several other configurations.
