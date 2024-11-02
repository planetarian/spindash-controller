# Spinbus

SPI but not really.

8-bit MOSI: 1 clock = 1 byte

1-bit MISO: 8 clocks = 1 byte

1) write data
2) pulse SCK high-low
3) target device returns 1 bit at the same time

# Commands

```
Group Command
GGGG  CCCC
```

## System (`0000`)

Reserved/NOP
```
0000 0000
```

Reset
```
0000 1111
```

## YM (`0001`)

Send YM register + data
```
           Rsv Chip# A1  Addr----  Data----
0001 0001  00  NNNNN X   AAAAAAAA  DDDDDDDD
```
    
Send YM register (no data)
```
           Rsv Chip# A1  Addr----
0001 0010  00  NNNNN X   AAAAAAAA
```
    
Read YM data
```
           Rsv Chip# A1  Addr----
0001 0011  00  NNNNN X   AAAAAAAA
```

Set 2612 mode
```
           Rsv Chip# EN
0001 0101  00  NNNNN X
```

# Return Signals

## General

`0000 0001` Command received

`0010 1011` Awaiting data

`1111 1111` Acknowledged

`1111 0000` Error

## Error codes

```
Header_F0  Header_D4  HdrF Code  Bytes----  Data0----  DataN----
1111 0000  1101 0100  1111 0001  0000 0002  DDDD DDDD  DDDD DDDD
````

`0001` 0xF1 Unknown command

`0010` 0xF2 Invalid state

`1000` 0xF8 YM index out of range

`1001` 0xF9 YM double submission