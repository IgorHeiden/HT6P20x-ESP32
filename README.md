# HT6P20x-ESP32
Lightweight RF OOK decoder for HT6P20x encoders by Igor "Heiden" Becker.

This code is intended to be used with a OOK RF receiver to decode messages sent by a Holtek's HT6P20x-based OOK RF encoder.

NOTICE: HT6P20x ICs use line coding in which logical bits are encoded as 3 physical bits, and, the bitrate is set by anexternal resistor;
therefore, the bitrate-related #defines in the source code MUST be modified accordingly to the encoder oscillating frequency!!!
