# FM Registers
See: <a href="https://www.plutiedev.com/ym2612-registers">[PlutieDev] YM2612 register reference</a>

## `$22` [Global]: low frequency oscillator

|Bit 7|Bit 6|Bit 5|Bit 4|Bit 3  |Bit 2  |Bit 1  |Bit 0  |
|-----|-----|-----|-----|-------|-------|-------|-------|
|`0`  |`0`  |`0`  |`0`  |`LFOEN`|`LFO:2`|`LFO:1`|`LFO:0`|

`LFO`: LFO frequency
`LFOEN`: 1 to turn on the LFO
The low frequency oscillator (LFO) is used to enable FMS and AMS to work for some simple vibrato- and tremolo-like effects. When the LFO is enabled, the following frequencies are available:

|Value|LFO frequency|
|-----|-------------|
|`000`|3.82 Hz      |
|`001`|5.33 Hz      |
|`010`|5.77 Hz      |
|`011`|6.11 Hz      |
|`100`|6.60 Hz      |
|`101`|9.23 Hz      |
|`110`|46.11 Hz     |
|`111`|69.22 Hz     |


## `$24`,`$25` [Global]:  timer A frequency

### `$25`: timer A frequency (high)
|Bit 7   |Bit 6   |Bit 5   |Bit 4   |Bit 3   |Bit 2   |Bit 1   |Bit 0   |
|--------|--------|--------|--------|--------|--------|--------|--------|
|`TMRA:9`|`TMRA:8`|`TMRA:7`|`TMRA:6`|`TMRA:5`|`TMRA:4`|`TMRA:3`|`TMRA:2`|

### `$25`: timer A frequency (low)
|Bit 7|Bit 6|Bit 5|Bit 4|Bit 3|Bit 2|Bit 1   |Bit 0   |
|-----|-----|-----|-----|-----|-----|--------|--------|
|`0`  |`0`  |`0`  |`0`  |`0`  |`0`  |`TMRA:1`|`TMRA:0`|

`TMRA`: timer A frequency

Timer A counts from `TMRA` to `$400` (when it "overflows"), then reloads and repeats. The exact frequency is computed as follows (approximately):

(`$400` - `TMRA`) × 18.77µs


## `$26` [Global]: timer B frequency

|Bit 7   |Bit 6   |Bit 5   |Bit 4   |Bit 3   |Bit 2   |Bit 1   |Bit 0   |
|--------|--------|--------|--------|--------|--------|--------|--------|
|`TMRB:7`|`TMRB:6`|`TMRB:5`|`TMRB:4`|`TMRB:3`|`TMRB:2`|`TMRB:1`|`TMRB:0`|

`TMRB`: timer B frequency (higher = faster)

Timer B counts from `TMRB` to `$100` (when it "overflows"), then reloads and repeats. The exact frequency is computed as follows (approximately):

(`$100` - `TMRB`) × 300.34µs


## `$27` [Global]: channel 3 mode and timer control

|Bit 7   |Bit 6   |Bit 5  |Bit 4  |Bit 3   |Bit 2   |Bit 1   |Bit 0   |
|--------|--------|-------|-------|--------|--------|--------|--------|
|`MODE:1`|`MODE:0`|`RST:B`|`RST:A`|`ENBL:B`|`ENBL:A`|`LOAD:B`|`LOAD:A`|

`LOAD`: controls whether the respective timer runs

* `0`: timer is frozen

* `1`: timer is running

`ENBL`: determines what happens when the respective timer overflows

* `0`: does nothing

* `1`: sets the flag

`RST`: modifies the respective timer flag when reading from `$A04000`

* `0`: leave it as-is

* `1`: clear the flag

`MODE`: channel 3 mode

* `00`: normal (like the other channels)

* `01`: special (each operator has its own frequency)

* `10`: CSM

Quick guide to using the timers (if you just want to let them run and check when they fire, which is probably the only thing you'll need):

| Value | Timers                  | ch3 mode  |
|-------|-------------------------|-----------|
| `$0F`   | —                       | Normal    |
| `$1F`   | Acknowledge timer A     | Normal    |
| `$2F`   | Acknowledge timer B     | Normal    |
| `$3F`   | Acknowledge both timers | Normal    |
| `$4F`   | —                       | Special   |
| `$5F`   | Acknowledge timer A     | Special   |
| `$6F`   | Acknowledge timer B     | Special   |
| `$7F`   | Acknowledge both timers | Special   |


## `$28` [Global]: key-on and key-off

| Bit 7    | Bit 6    | Bit 5    | Bit 4    | Bit 3 | Bit 2  | Bit 1  | Bit 0  |
| -------- | -------- | -------- | -------- | ----- | ------ | ------ | ------ |
| `OPERS3` | `OPERS2` | `OPERS1` | `OPERS0` | `0`   | `CH:2` | `CH:1` | `CH:0` |

CH: which channel

`OPERS0`: operator S1 (`0` = key-off, `1` = key-on)

`OPERS1`: operator S2 (`0` = key-off, `1` = key-on)

`OPERS2`: operator S3 (`0` = key-off, `1` = key-on)

`OPERS3`: operator S4 (`0` = key-off, `1` = key-on)

The first three channels are `000`-`010`, the last three channels are `100`-`110`.


Each operator can be set separately, but you'll usually set them all the same way. Attack rate starts when going from off to on, release rate starts when going from on to off. Envelope is not affected when staying the same.


## `$2A` [Global]: DAC output

When DAC output is enabled, the value written here is output as-is on the 6th channel. To play PCM sound you need to be constantly writing to this register at the exact moment the samples are meant to be output.


## `$2B` [Global]: DAC enable

| Bit 7   | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
| ------- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| `DACEN` | `0`   | `0`   | `0`   | `0`   | `0`   | `0`   | `0`   |

`DACEN`: 6th channel output

`0`: output FM

`1`: output DAC


## `$30+` [Operator]: MUL (multiply) and DT (detune)

| Channel | Oper S1   | Oper S2   | Oper S3   | Oper S4    |
|---------|-----------|-----------|-----------|------------|
| 1st/4th | `$30`     | `$38`     | `$34`     | `$3C`      |
| 2nd/5th | `$31`     | `$39`     | `$35`     | `$3D`      |
| 3rd/6th | `$32`     | `$3A`     | `$36`     | `$3E`      |

| Bit 7 | Bit 6  | Bit 5  | Bit 4  | Bit 3   | Bit 2   | Bit 1   | Bit 0   |
| ----- | ------ | ------ | ------ | ------- | ------- | ------- | ------- |
| `0`   | `DT:2` | `DT:1` | `DT:0` | `MUL:3` | `MUL:2` | `MUL:1` | `MUL:0` |

`MUL`: multiplier

`DT`: detune

The `MUL` field specifies by how much the base frequency is multiplied (by 1, 2, 3… up to 15). The exception to the rule is if this value is 0, in which case it multiplies by 0.5 (i.e. halves the frequency).

The DT field specifies a frequency detune, as follows (where `E` is a small value that depends on the exact tone):

| Value | Detune    |
| ----- | --------- |
| `000` | No detune |
| `001` | +1 × E    |
| `010` | +2 × E    |
| `011` | +3 × E    |
| `100` | No detune |
| `101` | \-1 × E   |
| `110` | \-2 × E   |
| `111` | \-3 × E   |


## `$40+` [Operator]: TL (total level)

| Channel | Oper S1   | Oper S2   | Oper S3   | Oper S4    |
|---------|-----------|-----------|-----------|------------|
| 1st/4th | `$40`     | `$48`     | `$44`     | `$4C`      |
| 2nd/5th | `$41`     | `$49`     | `$45`     | `$4D`      |
| 3rd/6th | `$42`     | `$4A`     | `$46`     | `$4E`      |

| Bit 7 | Bit 6  | Bit 5  | Bit 4  | Bit 3  | Bit 2  | Bit 1  | Bit 0  |
| ----- | ------ | ------ | ------ | ------ | ------ | ------ | ------ |
| `0`   | `TL:6` | `TL:5` | `TL:4` | `TL:3` | `TL:2` | `TL:1` | `TL:0` |

`TL`: total level (every step is 0.75dB quieter)

Total level is what is normally thought of as the "volume". If you want to affect the volume of a channel, make sure to only touch the `TL` of "slot" (output) operators and not those that feed into other operators.


## `$50+` [Operator]: AR (attack rate) and RS (rate scaling)

| Channel | Oper S1   | Oper S2   | Oper S3   | Oper S4    |
|---------|-----------|-----------|-----------|------------|
| 1st/4th | `$50`     | `$58`     | `$54`     | `$5C`      |
| 2nd/5th | `$51`     | `$59`     | `$55`     | `$5D`      |
| 3rd/6th | `$52`     | `$5A`     | `$56`     | `$5E`      |

| Bit 7  | Bit 6  | Bit 5 | Bit 4  | Bit 3  | Bit 2  | Bit 1  | Bit 0  |
| ------ | ------ | ----- | ------ | ------ | ------ | ------ | ------ |
| `RS:1` | `RS:0` | `0`   | `AR:4` | `AR:3` | `AR:2` | `AR:1` | `AR:0` |

`AR`: attack rate (higher = steeper)

`RS`: rate scaling

Rate scaling makes envelopes steeper at higher frequencies (needed to recreate how some instruments change sound as pitch becomes higher). A rate scaling of 0 does nothing, higher values increase this effect.


## `$60+` [Operator]: DR (decay rate) and AM enable

| Channel | Oper S1   | Oper S2   | Oper S3   | Oper S4    |
|---------|-----------|-----------|-----------|------------|
| 1st/4th | `$60`     | `$68`     | `$64`     | `$6C`      |
| 2nd/5th | `$61`     | `$69`     | `$65`     | `$6D`      |
| 3rd/6th | `$62`     | `$6A`     | `$66`     | `$6E`      |

| Bit 7  | Bit 6 | Bit 5 | Bit 4  | Bit 3  | Bit 2  | Bit 1  | Bit 0  |
| ------ | ----- | ----- | ------ | ------ | ------ | ------ | ------ |
| `AMON` | `0`   | `0`   | `DR:4` | `DR:3` | `DR:2` | `DR:1` | `DR:0` |

`DR`: decay rate (higher = steeper)

`AMON`: `1` to let AMS affect this operator

Sometimes also called "first decay rate" (`D1R`).


## `$70+` [Operator]: SR (sustain rate)

| Channel | Oper S1   | Oper S2   | Oper S3   | Oper S4    |
|---------|-----------|-----------|-----------|------------|
| 1st/4th | `$70`     | `$78`     | `$74`     | `$7C`      |
| 2nd/5th | `$71`     | `$79`     | `$75`     | `$7D`      |
| 3rd/6th | `$72`     | `$7A`     | `$76`     | `$7E`      |

| Bit 7 | Bit 6 | Bit 5 | Bit 4  | Bit 3  | Bit 2  | Bit 1  | Bit 0  |
| ----- | ----- | ----- | ------ | ------ | ------ | ------ | ------ |
| `0`   | `0`   | `0`   | `SR:4` | `SR:3` | `SR:2` | `SR:1` | `SR:0` |

`SR`: sustain rate (higher = steeper)

Sometimes also called "second decay rate" (`D2R`).


## `$80+` [Operator]: RR (release rate) and SL (sustain level)

| Channel | Oper S1   | Oper S2   | Oper S3   | Oper S4    |
|---------|-----------|-----------|-----------|------------|
| 1st/4th | `$80`     | `$88`     | `$84`     | `$8C`      |
| 2nd/5th | `$81`     | `$89`     | `$85`     | `$8D`      |
| 3rd/6th | `$82`     | `$8A`     | `$86`     | `$8E`      |

| Bit 7  | Bit 6  | Bit 5  | Bit 4  | Bit 3  | Bit 2  | Bit 1  | Bit 0  |
| ------ | ------ | ------ | ------ | ------ | ------ | ------ | ------ |
| `SL:3` | `SL:2` | `SL:1` | `SL:0` | `RR:3` | `RR:2` | `RR:1` | `RR:0` |

`RR`: release rate (higher = steeper)

`SL`: sustain level (higher = at a quieter level)

Release rate has one bit less than the other rates.

Sustain level is at the point in the envelope where it switches from the first to the second decay rate. 0 is the peak of the envelope, 15 is the bottom of the envelope.

## `$90+` [Operator]: SSG-EG

! This register is often incorrectly implemented in inaccurate clones and if you're unlucky they'll result in awful noise. If you really want to support those make sure to set this register to 0 in those cases.

| Channel | Oper S1   | Oper S2   | Oper S3   | Oper S4    |
|---------|-----------|-----------|-----------|------------|
| 1st/4th | `$90`     | `$98`     | `$94`     | `$9C`      |
| 2nd/5th | `$91`     | `$99`     | `$95`     | `$9D`      |
| 3rd/6th | `$92`     | `$9A`     | `$96`     | `$9E`      |

| Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3      | Bit 2     | Bit 1     | Bit 0     |
| ----- | ----- | ----- | ----- | ---------- | --------- | --------- | --------- |
| `0`   | `0`   | `0`   | `0`   | `SSGEG:EN` | `SSGEG:2` | `SSGEG:1` | `SSGEG:0` |

`SSGEG`: envelope shape

When `SSGEG:EN` is clear, nothing unusual happens. When it's set, the envelope is processed in different ways (e.g. looping) depending on what values the `SSGEG:2-0` bits have. You must make sure that attack rate is `31` or it will not work properly.

| Value | Shape |
| ----- | ----- |
| `000` | \\\\  |
| `001` | \___  |
| `010` | \/\/  |
| `011` | \'''  |
| `100` | ////  |
| `101` | /'''  |
| `110` | /\/\  |
| `111` | /___  |


## `$A0+` [Channel]: frequency

| Channel | High half   | Low half    |
|---------|-------------|-------------|
| 1st/4th | `$A4`       | `$A0`       |
| 2nd/5th | `$A5`       | `$A1`       |
| 3rd/6th | `$A6`       | `$A2`       |

When in channel 3 special mode, the third FM channel (but not the sixth) can have its frequency set per operator. In this case, the registers for channel 3 are as follows:

| Operator | High half | Low half |
|----------|-----------|----------|
| S1       | `$AD`     | `$A9`    |
| S2       | `$AE`     | `$AA`    |
| S3       | `$AC`     | `$A8`    |

### `$A4` onward: frequency (high)
| Bit 7 | Bit 6 | Bit 5   | Bit 4   | Bit 3   | Bit 2     | Bit 1    | Bit 0    |
| ----- | ----- | ------- | ------- | ------- | --------- | -------- | -------- |
| `0`   | `0`   | `BLK:2` | `BLK:1` | `BLK:0` | `FREQ:10` | `FREQ:9` | `FREQ:8` |

### `$A0` onward: frequency (low)
| Bit 7    | Bit 6    | Bit 5    | Bit 4    | Bit 3    | Bit 2    | Bit 1    | Bit 0    |
| -------- | -------- | -------- | -------- | -------- | -------- | -------- | -------- |
| `FREQ:7` | `FREQ:6` | `FREQ:5` | `FREQ:4` | `FREQ:3` | `FREQ:2` | `FREQ:1` | `FREQ:0` |

`FREQ`: frequency

`BLK`: block (octave)

To set the frequency, first write its higher half then its lower half (frequency won't be updated until the latter write).

The block acts like the octave: frequency is doubled every time it's incremented by 1. As for the frequency, here are some approximate values (as used by Echo), though they're off by a bit depending on whether it's a NTSC or PAL system:

| Semitone | Frequency |
| -------- | --------- |
| C        | 644       |
| C#       | 681       |
| D        | 722       |
| D#       | 765       |
| E        | 810       |
| F        | 858       |
| F#       | 910       |
| G        | 964       |
| G#       | 1021      |
| A        | 1081      |
| A#       | 1146      |
| B        | 1214      |


## `$B0+` [Channel]: algorithm and feedback

| Channel | Register  |
|---------|-----------|
| 1st/4th | `$B0`     |
| 2nd/5th | `$B1`     |
| 3rd/6th | `$B2`     |

| Bit 7 | Bit 6 | Bit 5    | Bit 4    | Bit 3    | Bit 2    | Bit 1    | Bit 0    |
| ----- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |
| `0`   | `0`   | `FEED:2` | `FEED:1` | `FEED:0` | `ALGO:2` | `ALGO:1` | `ALGO:0` |

`ALGO`: algorithm

`FEED`: channel 1 feedback

The algorithm describes which operators modulate other operators, and which operators generate the final output. Different algorithms can generate wildly different sounds. The are eight available arrangements.

| Algo      | Arrangement               |
|-----------|---------------------------|
| 0 (`000`) | `S1`->`S2`->`S3`->`S4`    |
| 1 (`001`) | (`S1`,`S2`)->`S3`->`S4`   |
| 2 (`010`) | (`S1`,`S2`->`S3`)->`S4`   |
| 3 (`011`) | (`S1`->`S2`,`S3`)->`S4`   |
| 4 (`100`) | (`S1`->`S2`),(`S3`->`S4`) |
| 5 (`101`) | `S1`->(`S2`,`S3`,`S4`)    |
| 6 (`110`) | `S1`->`S2`,`S3`,`S4`      |
| 7 (`111`) | `S1`,`S2`,`S3`,`S4`       |

Operator S1 can modulate itself (on top of the algorithm's arrangement), FEED controls how much it does so. 0 is no feedback, higher values increase the self-modulation.


## `$B4+` [Channel]: panning, PMS, AMS

| Channel | Register |
|---------|----------|
| 1st/4th | `$B4`    |
| 2nd/5th | `$B5`    |
| 3rd/6th | `$B6`    |

| Bit 7 | Bit 6 | Bit 5   | Bit 4   | Bit 3 | Bit 2   | Bit 1   | Bit 0   |
| ----- | ----- | ------- | ------- | ----- | ------- | ------- | ------- |
| `L`   | `R`   | `AMS:1` | `AMS:0` | `0`   | `PMS:2` | `PMS:1` | `PMS:0` |

`L`: 1 to output to left speaker

`R`: 1 to output to right speaker

`AMS`: amplitude modulation sensivity

`PMS`: frequency modulation sensivity

Frequency and amplitude sensivity (`PMS` and `AMS` respectively) are meant to be used together with the LFO and indicate how much is the waveform affected by it.

PMS can be used as a cheap form of vibrato. The values are:

| Value | Modulation       |
| ----- | ---------------- |
| `000` | None             |
| `001` | ±0.034 semitones |
| `010` | ±0.067 semitones |
| `011` | ±0.10 semitones  |
| `100` | ±0.14 semitones  |
| `101` | ±0.20 semitones  |
| `110` | ±0.40 semitones  |
| `111` | ±0.80 semitones  |

AMS only affects those operators where the `AM` bit has been set, and it can be used as a cheap form of tremolo. The possible values are:

| Value | Modulation |
| ----- | ---------- |
| `00`  | None       |
| `01`  | ±1.4 dB    |
| `10`  | ±5.9 dB    |
| `11`  | ±11.8 dB   |

* `PMS` actually stands for "phase modulation sensivity" (since the YM2612 actually does phase modulation instead of frequency modulation). The outcome is more or less the same, aside from rounding errors.


# `$21`, `$2C` [Global] Test registers
See: <a href="https://gendev.spritesmind.net/forum/viewtopic.php?f=24&t=386&start=825#p31285">[Spritesmind.net] New Documentation: An authoritative reference on the YM2612</a>

`$21:0`: Select which of two unknown signals is read as bit 14 of the test read output.

`$21:1`: Some LFO control, unknown function.

`$21:2`: Timers increment once every internal clock rather than once every sample. (Untested)

`$21:3`: Freezes PG. Presumably disables writebacks to the phase register.

`$21:4`: Ugly bit. Inverts MSB of operators.

`$21:5`: Freezes EG. Presumably disables writebacks to the envelope counter register. Unknown whether this affects the 
other EG state bits.

`$21:6`: Enable reading test data from OPN2 rather than status flags.

`$21:7`: Select LSB (`1`) or MSB (`0`) of read test data. (Yes, it's backwards.)

`$2C:2 downto 0`: Ignored by OPN2, confirmed by die shot.

`$2C:3`: Bit 0 of Channel 6 DAC value

`$2C:4`: Read 9-bit channel output (`1`) instead of 14-bit operator output (`0`)

`$2C:5`: Play DAC output over all channels (possibly except for Channel 5--in my testing the DAC is the only thing you 

hear and it's much louder, you do not get any output from Channel 5; but someone else supposedly found that the pan flags for Channel 5 don't affect the panning of this sound, which is only possible if it's not being output during that time slot for some reason. I don't have any other reason to believe this is true though).

`$2C:6`: Select function of TEST pin input--both unknown functions.

`$2C:7`: Set the TEST pin to be an output (`1`) instead of input (`0`).


# Test Read Data Format

`ABPPPPPP PPPPPPPP` (if `$2C:4` is `0`)

`AB00000C CCCCCCCC` (if `$2C:4` is `1`)

`A`: Unknown signal.

`B`: One of two unknown signals; which one is read is selected by `$21:0`.

`C`: 9-bit channel output.

`P`: 14-bit operator output.


# Test Bit Functions

When the test bit is configured as an output (`$2C:7` is `1`), it outputs the SYNC signal.

This signal goes high for one internal cycle every sample (24 cycles).

Its falling edge occurs just after an internal clock pulse, so wait half an internal cycle before sampling the OPN2's output.

Then, four cycles after that, the data on the output will be for Ch 1 Op 1.

Presumably, the SYNC signal is when it begins calculation of Ch 1 Op 1, and the output is available after four cycles.
Sample on each internal cycle to receive the data for channels 1-6 operator 1, then 1-6 op 3, then 1-6 op 2, then 1-6 op 4.
