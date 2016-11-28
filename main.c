/*
 * Utility for monitoring battery voltage and CPU (Intel x5-z8500) power consumption
 * Battery voltage obtained from BD2613GW PMIC (ADC used for digitize voltage)
 * CPU usage obtained from intel-rapl (ujoules converted to watts)
 *
 * (c) Abylay Ospan, 2016
 * http://jokersys.com
 *
 * LICENSE: GPLv2
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <ctype.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>

#define FILENAME_PKG "/sys/devices/virtual/powercap/intel-rapl/intel-rapl:0/energy_uj"
#define FILENAME_CORE "/sys/devices/virtual/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:0/energy_uj"
#define I2C_ADDR 0x6E 

void usage()
{
    printf("Usage:\n");
    printf("-i interval         in seconds, default:1\n");
    printf("-b i2c_bus_id       default:1\n");
    printf("-o outfile          default:none\n");
}

int64_t get_ts()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*1000000 + tv.tv_usec;
}

int64_t get_uj(char *file)
{
    int64_t uj = 0;
    FILE * fd = NULL;

    fd = fopen(file, "r");
    if (fd < 0)
    {
        fprintf(stderr, "can't open file %s. error = %s (%d)\n", file, strerror(errno), errno);
        return 0;
    }
    fscanf(fd, "%llu", &uj);
    fclose(fd);
    return uj;
}

double get_voltage(int bus_id)
{
    int fd;
    char buf[10];
    char filename[20] = { 0 };
    double voltage = 0;
    int res = 0, voltage_i = 0;

    snprintf(filename, 19, "/dev/i2c-%d", bus_id);
    fd = open(filename, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "can't open file %s. error = %s (%d)\n", filename, strerror(errno), errno);
        return 0;
    }

    if (ioctl(fd, I2C_SLAVE, I2C_ADDR) < 0) {
        fprintf(stderr, "i2c failed for %s. error = %s (%d)\n", filename, strerror(errno), errno);
        return 0;
    }

    /* start battery voltage calculation */
    if (i2c_smbus_write_byte_data(fd, 0x72, 0x01) < 0)
        return 0;

    /* wait until done */
    buf[0] = 0x72;
    while (1)
    {
        res = i2c_smbus_read_byte_data(fd, 0x72);
        if (res < 0)
            return 0;
        if ( !res&0x01 )
            break;
        usleep(50000);
    }

    res = i2c_smbus_read_byte_data(fd, 0x80);
    if (res < 0)
        return 0;
    voltage_i = res << 8;

    res = i2c_smbus_read_byte_data(fd, 0x81);
    if (res < 0)
        return 0;
    voltage_i |= res;
    voltage = 5 * voltage_i/1023.0;

    return voltage;
}

int main (int argc, char **argv)
{
    int fdo, res, c, interval=1, bus_id = 1, i = 0;
    char buf[512], obuf[512];
    int64_t uj[2], ujprev[2] = {0}, uj_start[2], ts, last_ts;
    double delta = 0, uj_cur[2] = {0}, voltage = 0;
    char *outfile = NULL;
    time_t t = 0;
    struct tm *tm = NULL;

    while ((c = getopt (argc, argv, "i:b:o:")) != -1)
    {
        switch (c)
        {
            case 'o':
                outfile = strdup(optarg);
                break;
            case 'i':
                interval = atoi(optarg);
                break;
            case 'b':
                bus_id = atoi(optarg);
                break;
            case '?':
                usage();
                return 1;
            default:
                abort ();
        }
    }

    if (outfile)
    {
        fdo = open(outfile, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO);
        if (fdo < 0)
        {
            fprintf(stderr, "Can't open outfile %s. error = %s (%d)\n", outfile, strerror(errno), errno);
            return 1;
        }

        /* write header to outfile */
        res = snprintf(obuf, sizeof(obuf), "date;time;timestamp;ujoules;watt_pkg;watt_core;voltage\n");
        write(fdo, obuf, res);
    }

    uj_start[0] = get_uj(FILENAME_PKG);
    uj_start[1] = get_uj(FILENAME_CORE);

    last_ts = ts;
    while (1)
    {
        uj[0] = get_uj(FILENAME_PKG);
        uj[1] = get_uj(FILENAME_CORE);

        /* update timestamps */
        ts = get_ts();
        delta = (ts - last_ts)/1000000;
        last_ts = ts;
        for (i = 0; i < 2; i++)
            uj_cur[i] = (double)(ujprev[i]?(uj[i]-ujprev[i]):0)/(delta*1000000);
        voltage = get_voltage(bus_id);

        if (fdo > 0)
        {
            t = (time_t)(ts/1000000);
            tm = localtime(&t);
            strftime(buf, sizeof(buf), "%m/%d/%Y;%T", tm);

            res = snprintf(obuf, sizeof(obuf), "%s;%llu;%llu;%f;%f;%f\n", buf, ts, uj[0] - uj_start[0], uj_cur[0], uj_cur[1], voltage);
            write(fdo, obuf, res);
        }

        fprintf(stderr, "ts=%llu total_uj=%llu watts(pkg)=%f watts(core)=%f voltage=%f\n", \
                ts/1000000, uj[0] - uj_start[0], uj_cur[0], uj_cur[1], voltage);

        for (i = 0; i < 2; i++)
            ujprev[i] = uj[i];
        sleep(interval);
    }
}
