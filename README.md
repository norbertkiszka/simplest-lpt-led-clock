## Required hardware

1. 4-digit 7-segment led display with common cathode (common anode is not supported).
2. Eight ~100 Ohm resistors.
3. Any wire to make 12 connections.
4. PCB or prototype board - eventually without it...
5. DB25 male connector - or simply put wires into lpt socket...

## Conection

<pre>LPT PIN NUM ------ display
1 (STROBE)         Cathodes - digit 1 (pin 12)
14 (AUTO FEED)     Cathodes - digit 2 (pin 9)
16 (INIT)          Cathodes - digit 3 (pin 8)
17 (SELECT)        Cathodes - digit 4 (pin 6)
2 (D0)             A
3 (D1)             B
4 (D2)             C
5 (D3)             D
6 (D4)             E
7 (D5)             F
8 (D6)             G
9 (D7)             Dot
</pre>

![Schema](https://raw.githubusercontent.com/norbertkiszka/simplest-lpt-led-clock/master/Schema.png)

## Photos

![Temp](https://raw.githubusercontent.com/norbertkiszka/simplest-lpt-led-clock/master/photo/temp_from_api.jpg)

![Clock](https://raw.githubusercontent.com/norbertkiszka/simplest-lpt-led-clock/master/photo/clock.jpg)

![LPT connection](https://raw.githubusercontent.com/norbertkiszka/simplest-lpt-led-clock/master/photo/lpt.jpg)

## License

[GPL V2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html) - included with repository in text format.

7-segment-4-digit-red-12-pin-pinout-1.jpg - this schema was taken from mklec.com and I'm not owner of it.
