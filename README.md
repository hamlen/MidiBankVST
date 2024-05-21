# MidiBank

*MidiBankVST* is a utility VST3 that saves incoming MIDI CC messages into 16 *banks*, one for each MIDI channel.  Messages saved to the current bank (selected by adjusting *MidiBankVST*'s **Bank** parameter) are immediately retransmitted, but all others are suppressed.  Changing the **Bank** parameter causes *MidiBankVST* to transmit all saved CC values for the new bank that differ from the old bank.

For example, if you connect the outputs of two MIDI keyboards to *MidiBankVST*, one sending on channel 1 and the other sending on channel 2, and you connect the output of *MidiBankVST* to a single MIDI display device, then switching *MidiBankVST* from Bank 0 to Bank 1 will reset the MIDI display device's display to show the current state of the second keyboard, and switching it back to Bank 0 will reset it to show the current state of the first keyboard.

*MidiBankVST* also responds to two special SysEx messages:

- 0xF0 0x00 0xF7: Resets all saved CC values for all banks to zero.
- 0xF0 0x01 *mm* ... *mm* 0xF7: Causes *MidiBankVST* to immediately transmit the current saved CC values of selected controller numbers for the current bank.  Data bytes *mm* ... *mm* form a bit mask that identifies which controller numbers to transmit.  The bit mask consists of the low 7 bits of each byte *mm* in little endian order (least significant bytes first).  For example, to transmit the values of CCs 1, 2, 8, and 10, send SysEx message 0xF0 0x01 0x03 0x05 0xF7.  (The data bytes 0x03 0x05 encode binary value 00001010000011, in which the 1st (rightmost, lowest significance) bit, 2nd bit, 8th bit, and 10th bit are set.)
