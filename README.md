# Utility for monitoring battery voltage and Intel x5-z8500 CPU power consumption
### (c) Abylay Ospan, 2016 
### http://jokersys.com
### LICENSE: GPLv2

Battery voltage obtained from BD2613GW PMIC (ADC used for digitize voltage). CPU usage obtained from intel-rapl (ujoules converted to watts). Program tested on the folowing platforms:
 * Personal mobile Linux server (aka "Joker") - http://jokersys.com
 * Microsoft Surface 3 (not 'pro' version) with Debian Linux


Before compilation please install following package: 
 `apt-get install libi2c-dev`

and then you can start compilation:

```
mkdir build
cd build
cmake ../
make
```

now you can start monitoring:
```
# ./joker-power 
ts=1479612120 total_uj=1349088 watts=1.349088 voltage=4.169110
ts=1479612121 total_uj=2526272 watts=1.177184 voltage=4.169110
ts=1479612122 total_uj=3686688 watts=1.160416 voltage=4.169110
ts=1479612123 total_uj=4884320 watts=1.197632 voltage=4.178886
ts=1479612124 total_uj=6111712 watts=1.227392 voltage=4.173998
ts=1479612125 total_uj=7297344 watts=1.185632 voltage=4.173998
```

usage:
```
-i interval         in seconds, default:1
-b i2c_bus_id       default:1
-o outfile          default:none
```

to check BD2613GW PMIC availability you can use `i2cdetect` command:
```
# i2cdetect -ry 1 0x03 0x70
Continue? [Y/n] 
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- 5e -- 
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- 6e -- 
70: --             
```

address 0x5e and 0x6e should be in this output.

notes:
be sure that the following kernel modules are loaded:
```
modprobe i2c_designware_platform
modprobe i2c_dev
modprobe intel_rapl
```
otherwise you need to recompile your kernel with this modules enabled.

## Result for battery drain for Joker

![alt tag](https://raw.githubusercontent.com/aospan/joker-power/master/images/battery-discharge.png)

