/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* this implements a GPS hardware library for the Android emulator.
 * the following code should be built as a shared library that will be
 * placed into /system/lib/hw/gps.goldfish.so
 *
 * it will be loaded by the code in hardware/libhardware/hardware.c
 * which is itself called from android_location_GpsLocationProvider.cpp
 */


#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <math.h>
#include <time.h>
#include <cutils/log.h>
#include "lidbg_servicer.h"

/*
#define LOG_NDEBUG 0
#define  LOG_TAG  "gps_ublox"
#include <cutils/log.h>
*/

#include <cutils/sockets.h>

#include <android/log.h>
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG , "ublox", __VA_ARGS__)

#include <hardware/gps.h>

#define  GPS_DEV_NAME  "/dev/ubloxgps0"

#define  GPS_DEBUG  (0)

#if GPS_DEBUG
#define  D(...)   LOGD(__VA_ARGS__)
#else
#define  D(...)
#endif

#define GPS_START       _IO('g', 1)
#define GPS_STOP        _IO('g', 2)

/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       N M E A   T O K E N I Z E R                     *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

typedef struct
{
    const char  *p;
    const char  *end;
} Token;

#define  MAX_NMEA_TOKENS  32

typedef struct
{
    int     count;
    Token   tokens[ MAX_NMEA_TOKENS ];
} NmeaTokenizer;

static int
nmea_tokenizer_init( NmeaTokenizer  *t, const char  *p, const char  *end )
{
    int    count = 0;
    char  *q;

    D("%s", __FUNCTION__);
    // the initial '$' is optional
    if (p < end && p[0] == '$')
        p += 1;

    // remove trailing newline
    if (end > p && end[-1] == '\n')
    {
        end -= 1;
        if (end > p && end[-1] == '\r')
            end -= 1;
    }

    // get rid of checksum at the end of the sentecne
    if (end >= p + 3 && end[-3] == '*')
    {
        end -= 3;
    }

    while (p < end)
    {
        const char  *q = p;

        q = memchr(p, ',', end - p);
        if (q == NULL)
            q = end;

        if (q >= p)
        {
            if (count < MAX_NMEA_TOKENS)
            {
                t->tokens[count].p   = p;
                t->tokens[count].end = q;
                count += 1;
            }
        }

        if (q < end)
            q += 1;

        p = q;
    }

    t->count = count;
    return count;
}

static Token
nmea_tokenizer_get( NmeaTokenizer  *t, int  index )
{
    Token  tok;
    static const char  *dummy = "";

    if (index < 0 || index >= t->count)
    {
        tok.p = tok.end = dummy;
    }
    else
        tok = t->tokens[index];

    return tok;
}


static int
str2int( const char  *p, const char  *end )
{
    int   result = 0;
    int   len    = end - p;

    for ( ; len > 0; len--, p++ )
    {
        int  c;

        if (p >= end)
            goto Fail;

        c = *p - '0';
        if ((unsigned)c >= 10)
            goto Fail;

        result = result * 10 + c;
    }
    return  result;

Fail:
    return -1;
}

static double
str2float( const char  *p, const char  *end )
{
    int   result = 0;
    int   len    = end - p;
    char  temp[16];

    if (len >= (int)sizeof(temp))
        return 0.;

    memcpy( temp, p, len );
    temp[len] = 0;
    return strtod( temp, NULL );
}

/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       N M E A   P A R S E R                           *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

#define  NMEA_MAX_SIZE  83

typedef struct
{
    int     pos;
    int     overflow;
    int     utc_year;
    int     utc_mon;
    int     utc_day;
    int     utc_diff;
    GpsLocation  fix;
    GpsSvStatus  sv;
    GpsStatus	status;
    uint32_t	fix_mask;
    gps_location_callback  callback;
    GpsCallbacks callbacks;
    char    in[ NMEA_MAX_SIZE + 1 ];
} NmeaReader;


static void
nmea_reader_update_utc_diff( NmeaReader  *r )
{
    time_t         now = time(NULL);
    struct tm      tm_local;
    struct tm      tm_utc;
    long           time_local, time_utc;

    gmtime_r( &now, &tm_utc );
    localtime_r( &now, &tm_local );

    time_local = tm_local.tm_sec +
                 60 * (tm_local.tm_min +
                       60 * (tm_local.tm_hour +
                             24 * (tm_local.tm_yday +
                                   365 * tm_local.tm_year)));

    time_utc = tm_utc.tm_sec +
               60 * (tm_utc.tm_min +
                     60 * (tm_utc.tm_hour +
                           24 * (tm_utc.tm_yday +
                                 365 * tm_utc.tm_year)));

    r->utc_diff = time_local - time_utc;
}


static void
nmea_reader_init( NmeaReader  *r )
{
    memset( r, 0, sizeof(*r) );

    r->pos      = 0;
    r->overflow = 0;
    r->utc_year = -1;
    r->utc_mon  = -1;
    r->utc_day  = -1;
    r->callback = NULL;
    r->fix.size = sizeof(r->fix);

    nmea_reader_update_utc_diff( r );
}


static void
nmea_reader_set_callback( NmeaReader  *r, gps_location_callback  cb )
{
    r->callback = cb;
    if (cb != NULL && r->fix.flags != 0)
    {
        D("%s: sending latest fix to new callback", __FUNCTION__);
        r->callback( &r->fix );
        r->fix.flags = 0;
    }
}


static int
nmea_reader_update_time( NmeaReader  *r, Token  tok )
{
    int        hour, minute;
    double     seconds;
    struct tm  tm;
    time_t     fix_time;

    if (tok.p + 6 > tok.end)
        return -1;

    if (r->utc_year < 0)
    {
        // no date yet, get current one
        time_t  now = time(NULL);
        gmtime_r( &now, &tm );
        r->utc_year = tm.tm_year + 1900;
        r->utc_mon  = tm.tm_mon + 1;
        r->utc_day  = tm.tm_mday;
    }

    hour    = str2int(tok.p,   tok.p + 2);
    minute  = str2int(tok.p + 2, tok.p + 4);
    seconds = str2float(tok.p + 4, tok.end);
    printf("hour=%d, minute=%d, seconds=%d\n", hour, minute, seconds);
    tm.tm_hour  = hour;
    tm.tm_min   = minute;
    tm.tm_sec   = (int) seconds;
    tm.tm_year  = r->utc_year - 1900;
    tm.tm_mon   = r->utc_mon - 1;
    tm.tm_mday  = r->utc_day;
    tm.tm_isdst = -1;

    fix_time = mktime( &tm ) + r->utc_diff;
    D("r->utc_diff:%d\n", r->utc_diff);
    r->fix.timestamp = (long long)fix_time * 1000;
    return 0;
}

static int
nmea_reader_update_date( NmeaReader  *r, Token  date, Token  time )
{
    Token  tok = date;
    int    day, mon, year;

    if (tok.p + 6 != tok.end)
    {
        D("date not properly formatted: '%.*s'", tok.end - tok.p, tok.p);
        return -1;
    }
    day  = str2int(tok.p, tok.p + 2);
    mon  = str2int(tok.p + 2, tok.p + 4);
    year = str2int(tok.p + 4, tok.p + 6) + 2000;

    if ((day | mon | year) < 0)
    {
        D("date not properly formatted: '%.*s'", tok.end - tok.p, tok.p);
        return -1;
    }

    r->utc_year  = year;
    r->utc_mon   = mon;
    r->utc_day   = day;

    return nmea_reader_update_time( r, time );
}


static double
convert_from_hhmm( Token  tok )
{
    double  val     = str2float(tok.p, tok.end);
    int     degrees = (int)(floor(val) / 100);
    double  minutes = val - degrees * 100.;
    double  dcoord  = degrees + minutes / 60.0;
    return dcoord;
}


static int
nmea_reader_update_latlong( NmeaReader  *r,
                            Token        latitude,
                            char         latitudeHemi,
                            Token        longitude,
                            char         longitudeHemi )
{
    double   lat, lon;
    Token    tok;

    tok = latitude;
    if (tok.p + 6 > tok.end)
    {
        D("latitude is too short: '%.*s'", tok.end - tok.p, tok.p);
        return -1;
    }
    lat = convert_from_hhmm(tok);
    if (latitudeHemi == 'S')
        lat = -lat;

    tok = longitude;
    if (tok.p + 6 > tok.end)
    {
        D("longitude is too short: '%.*s'", tok.end - tok.p, tok.p);
        return -1;
    }
    lon = convert_from_hhmm(tok);
    if (longitudeHemi == 'W')
        lon = -lon;

    r->fix.flags    |= GPS_LOCATION_HAS_LAT_LONG;
    r->fix.latitude  = lat;
    r->fix.longitude = lon;
    return 0;
}


static int
nmea_reader_update_altitude( NmeaReader  *r,
                             Token        altitude,
                             Token        units )
{
    double  alt;
    Token   tok = altitude;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_ALTITUDE;
    r->fix.altitude = str2float(tok.p, tok.end);
    return 0;
}
static int
nmea_reader_update_accuracy( NmeaReader  *r,
                             Token        hdop )
{
    double  alt;
    Token   tok = hdop;

    if (tok.p >= tok.end)
        return -1;

    r->fix.flags   |= GPS_LOCATION_HAS_ACCURACY;
    r->fix.accuracy = str2float(tok.p, tok.end);
    D("[accuracy]accuracy=%lf", r->fix.accuracy);
    return 0;
}


static int
nmea_reader_update_bearing( NmeaReader  *r,
                            Token        bearing )
{
    double  alt;
    Token   tok = bearing;

    if (tok.p > tok.end)
    {
        D("nmea_reader_update_bearing token error\n");
        return -1;
    }

    r->fix.flags   |= GPS_LOCATION_HAS_BEARING;
    r->fix.bearing  = str2float(tok.p, tok.end);
    D("current bearng:%f degreed\n", r->fix.bearing);
    return 0;
}


static int
nmea_reader_update_speed( NmeaReader  *r,
                          Token        speed )
{
    double  alt;
    Token   tok = speed;

    if (tok.p > tok.end)
    {
        D("nmea_reader_update_speed token error\n");
        return -1;
    }

    r->fix.flags   |= GPS_LOCATION_HAS_SPEED;
    r->fix.speed    = str2float(tok.p, tok.end) * 0.5144;
    /*    r->fix.speed = 200;*/
    D("current speed:%fm/s\n", r->fix.speed);
    return 0;
}

static void nmea_reader_parse_gsa(NmeaReader *r, NmeaTokenizer *t)
{
    int i;
    int prn;
    Token tok_prn;

    D("nmea_reader_parse_gsa");
    if( t->count > 15)
    {
        Token  tok_hdop = nmea_tokenizer_get(t, 15);
        nmea_reader_update_accuracy(r, tok_hdop);
    }

    for (i = 3; i < t->count; i++)
    {
        tok_prn = nmea_tokenizer_get(t, i);
        prn = str2int(tok_prn.p, tok_prn.end);

        D("prn = %d", prn);

        if (prn <= 0 || prn > 32)
            continue;

        if (prn == 0)
            break;
        else
            r->fix_mask |= 1 << (prn - 1);
    }
}

static void nmea_reader_parse_gsv(NmeaReader *r, NmeaTokenizer *t)
{
    static int cur_sv = 0;
    GpsSvInfo sv_info;
    int i;

    //D("[futengfei]===HCPUnmea_reader_parse_gsv");
    Token tok_num = nmea_tokenizer_get(t, 1);
    Token tok_which = nmea_tokenizer_get(t, 2);
    //Token tok_sv_num = nmea_tokenizer_get(t,3);
    //$GPGSV,2,1,08,02,16,138,06,05,29,067,,09,48,088,19,15,69,348,29*76
    int num = str2int(tok_num.p, tok_num.end);
    int which = str2int(tok_which.p, tok_which.end);
    //int sv_num = str2int(tok_sv_num.p, tok_sv_num.end);

    if (which == 1)
    {
        cur_sv = 0;
        r->sv.ephemeris_mask = 0;
        r->sv.almanac_mask = 0;
    }

    for (i = 4; i < t->count; i += 4)
    {
        Token tok_prn = nmea_tokenizer_get(t, i);
        Token tok_elevation = nmea_tokenizer_get(t, i + 1);
        Token tok_azimuth = nmea_tokenizer_get(t, i + 2);
        Token tok_snr = nmea_tokenizer_get(t, i + 3);

        sv_info.prn = str2int(tok_prn.p, tok_prn.end);
        sv_info.snr = str2float(tok_snr.p, tok_snr.end);
        sv_info.elevation = str2float(tok_elevation.p, tok_elevation.end);
        sv_info.azimuth = str2float(tok_azimuth.p, tok_azimuth.end);

        /*		if (sv_info.prn <= 0 || sv_info.prn > 32)
        			continue;

        		if (sv_info.snr <= 0 || sv_info.snr >= 99)
        			continue;
        */
        r->sv.sv_list[cur_sv] = sv_info;
        r->sv.ephemeris_mask |= 1 << (sv_info.prn - 1);
        r->sv.almanac_mask |= 1 << (sv_info.prn - 1);

        D("paser one svInfo:prn=%d,snr=%lf,elevation=%lf,azimuth=%lf", r->sv.sv_list[cur_sv].prn, r->sv.sv_list[cur_sv].snr, r->sv.sv_list[cur_sv].elevation, r->sv.sv_list[cur_sv].azimuth);

        cur_sv++;
    }

    if (num == which)
    {
        r->sv.size = sizeof(GpsSvStatus);
        r->sv.num_svs = cur_sv;
        r->sv.used_in_fix_mask = r->fix_mask;
        r->fix_mask = 0;

        D("commit one SvStatus:num_svs=%d, ephemeris_mask=0x%x, almanac_mask=0x%x, used_in_fix_mask=0x%x", cur_sv, r->sv.ephemeris_mask, r->sv.almanac_mask, r->sv.used_in_fix_mask);

        if (r->callbacks.sv_status_cb)
            r->callbacks.sv_status_cb(&r->sv);
    }
}

static void
nmea_reader_parse( NmeaReader  *r )
{
    /* we received a complete sentence, now parse it to generate
     * a new GPS fix...
     */
    NmeaTokenizer  tzer[1];
    Token          tok;
    unsigned int info;

    D("[gpsdata]Received: '%.*s'", r->pos, r->in);
    if (r->pos < 9)
    {
        D("Too short. discarded.");
        return;
    }

    nmea_tokenizer_init(tzer, r->in, r->in + r->pos);
#if GPS_DEBUG
    {
        int  n;
        D("Found %d tokens", tzer->count);
        for (n = 0; n < tzer->count; n++)
        {
            Token  tok = nmea_tokenizer_get(tzer, n);
            D("%2d: '%.*s'", n, tok.end - tok.p, tok.p);
        }
    }
#endif

    tok = nmea_tokenizer_get(tzer, 0);
    if (tok.p + 5 > tok.end)
    {
        D("sentence id '%.*s' too short, ignored.", tok.end - tok.p, tok.p);
        return;
    }

    // ignore first two characters.
    tok.p += 2;
    if ( !memcmp(tok.p, "GGA", 3) )
    {
        // GPS fix
        Token  tok_time          = nmea_tokenizer_get(tzer, 1);
        Token  tok_latitude      = nmea_tokenizer_get(tzer, 2);
        Token  tok_latitudeHemi  = nmea_tokenizer_get(tzer, 3);
        Token  tok_longitude     = nmea_tokenizer_get(tzer, 4);
        Token  tok_longitudeHemi = nmea_tokenizer_get(tzer, 5);
        Token  tok_altitude      = nmea_tokenizer_get(tzer, 9);
        Token  tok_altitudeUnits = nmea_tokenizer_get(tzer, 10);

        nmea_reader_update_time(r, tok_time);
        nmea_reader_update_latlong(r, tok_latitude,
                                   tok_latitudeHemi.p[0],
                                   tok_longitude,
                                   tok_longitudeHemi.p[0]);
        nmea_reader_update_altitude(r, tok_altitude, tok_altitudeUnits);

    }
    else if ( !memcmp(tok.p, "GSA", 3) )
    {
        // do something ?
        nmea_reader_parse_gsa(r, tzer);
    }
    else if ( !memcmp(tok.p, "GSV", 3) )
    {
        nmea_reader_parse_gsv(r, tzer);
    }
    else if ( !memcmp(tok.p, "RMC", 3) )
    {
        Token  tok_time          = nmea_tokenizer_get(tzer, 1);
        Token  tok_fixStatus     = nmea_tokenizer_get(tzer, 2);
        Token  tok_latitude      = nmea_tokenizer_get(tzer, 3);
        Token  tok_latitudeHemi  = nmea_tokenizer_get(tzer, 4);
        Token  tok_longitude     = nmea_tokenizer_get(tzer, 5);
        Token  tok_longitudeHemi = nmea_tokenizer_get(tzer, 6);
        Token  tok_speed         = nmea_tokenizer_get(tzer, 7);
        Token  tok_bearing       = nmea_tokenizer_get(tzer, 8);
        Token  tok_date          = nmea_tokenizer_get(tzer, 9);

        D("in RMC, fixStatus=%c", tok_fixStatus.p[0]);
        if (tok_fixStatus.p[0] == 'A')
        {
            nmea_reader_update_date( r, tok_date, tok_time );

            nmea_reader_update_latlong( r, tok_latitude,
                                        tok_latitudeHemi.p[0],
                                        tok_longitude,
                                        tok_longitudeHemi.p[0] );

            nmea_reader_update_bearing( r, tok_bearing );
            nmea_reader_update_speed  ( r, tok_speed );
        }
    }
    else
    {
        tok.p -= 2;
        D("unknown sentence '%.*s", tok.end - tok.p, tok.p);
    }
    info = GPS_LOCATION_HAS_LAT_LONG | GPS_LOCATION_HAS_ALTITUDE |
           GPS_LOCATION_HAS_BEARING | GPS_LOCATION_HAS_SPEED | GPS_LOCATION_HAS_ACCURACY;
    if ((r->fix.flags & info) == info)
    {
#if GPS_DEBUG
        char   temp[256];
        char  *p   = temp;
        char  *end = p + sizeof(temp);
        struct tm   utc;

        p += snprintf( p, end - p, "sending fix" );
        if (r->fix.flags & GPS_LOCATION_HAS_LAT_LONG)
        {
            p += snprintf(p, end - p, " lat=%g lon=%g", r->fix.latitude, r->fix.longitude);
        }
        if (r->fix.flags & GPS_LOCATION_HAS_ALTITUDE)
        {
            p += snprintf(p, end - p, " altitude=%g", r->fix.altitude);
        }
        if (r->fix.flags & GPS_LOCATION_HAS_SPEED)
        {
            p += snprintf(p, end - p, " speed=%g", r->fix.speed);
        }
        if (r->fix.flags & GPS_LOCATION_HAS_BEARING)
        {
            p += snprintf(p, end - p, " bearing=%g", r->fix.bearing);
        }
        if (r->fix.flags & GPS_LOCATION_HAS_ACCURACY)
        {
            p += snprintf(p, end - p, " accuracy=%g", r->fix.accuracy);
        }
        gmtime_r( (time_t *) &r->fix.timestamp, &utc );
        p += snprintf(p, end - p, " time=%s", asctime( &utc ) );
#endif
        if (r->callback)
        {
            D("[accuracy]Commit GpsLocation:latitude = %lf, longitude = %lf, altitude = %lf,accuracy=%lf", r->fix.latitude, r->fix.longitude, r->fix.altitude, r->fix.accuracy);
            r->callback( &r->fix );
            r->fix.flags = 0;
        }
        else
        {
            D("no callback, keeping data until needed !");
        }
    }
}


static void
nmea_reader_addc( NmeaReader  *r, int  c )
{
    if (r->overflow)
    {
        r->overflow = (c != '\n');
        return;
    }

    if (r->pos >= (int) sizeof(r->in) - 1 )
    {
        r->overflow = 1;
        r->pos      = 0;
        return;
    }

    r->in[r->pos] = (char)c;
    r->pos       += 1;

    if (c == '\n')
    {
        nmea_reader_parse( r );
        r->pos = 0;
    }
}


/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       C O N N E C T I O N   S T A T E                 *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

/* commands sent to the gps thread */
enum
{
    CMD_QUIT  = 0,
    CMD_START = 1,
    CMD_STOP  = 2
};


/* this is the state of our connection to the qemu_gpsd daemon */
typedef struct
{
    int                     init;
    int                     fd;
    GpsCallbacks            callbacks;
    pthread_t               thread;
    int                     control[2];
} GpsState;

static GpsState  _gps_state[1];


static void
gps_state_done( GpsState  *s )
{
    // tell the thread to quit, and wait for it
    char   cmd = CMD_QUIT;
    void  *dummy;
    write( s->control[0], &cmd, 1 );
    pthread_join(s->thread, &dummy);

    // close the control socket pair
    close( s->control[0] );
    s->control[0] = -1;
    close( s->control[1] );
    s->control[1] = -1;

    // close connection to the QEMU GPS daemon
    close( s->fd );
    s->fd = -1;
    s->init = 0;
}


static void
gps_state_start( GpsState  *s )
{
    char  cmd = CMD_START;
    int   ret;

    do
    {
        ret = write( s->control[0], &cmd, 1 );
    }
    while (ret < 0 && errno == EINTR);

    if (ret != 1)
        LOGD("%s: could not send CMD_START command: ret=%d: %s",
             __FUNCTION__, ret, strerror(errno));
}


static void
gps_state_stop( GpsState  *s )
{
    char  cmd = CMD_STOP;
    int   ret;

    do
    {
        ret = write( s->control[0], &cmd, 1 );
    }
    while (ret < 0 && errno == EINTR);

    if (ret != 1)
        LOGD("%s: could not send CMD_STOP command: ret=%d: %s",
             __FUNCTION__, ret, strerror(errno));
}


static int
epoll_register( int  epoll_fd, int  fd )
{
    struct epoll_event  ev;
    int                 ret, flags;

    /* important: make the fd non-blocking */
    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    ev.events  = EPOLLIN;
    ev.data.fd = fd;
    do
    {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_ADD, fd, &ev );
    }
    while (ret < 0 && errno == EINTR);
    return ret;
}


static int
epoll_deregister( int  epoll_fd, int  fd )
{
    int  ret;
    do
    {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_DEL, fd, NULL );
    }
    while (ret < 0 && errno == EINTR);
    return ret;
}

static void gps_fake_test(NmeaReader *reader)
{
    int i;
    char data[] =
        "$GPRMC,024910.00,A,2303.63577,N,11330.82974,E,3.643,281.61,080313,,,A*65\n"
        "$GPVTG,281.61,T,,M,3.643,N,6.747,K,A*31\n"
        "$GPGGA,024910.00,2303.63577,N,11330.82974,E,1,04,2.12,73.7,M,-4.7,M,,*78\n"
        "$GPGSA,A,3,29,24,21,15,,,,,,,,,4.68,2.12,4.17*01\n"
        "$GPGSV,2,1,08,02,16,138,06,05,29,067,,09,48,088,19,15,69,348,29*76\n"
        "$GPGSV,2,2,08,21,32,320,30,24,41,162,13,26,35,032,,29,37,237,42*79\n"
        "$GPGLL,2303.63577,N,11330.82974,E,024910.00,A,A*65\n";

    for (i = 0; i < sizeof(data); i++)
    {
        nmea_reader_addc(reader, data[i]);
    }
}

/* this is the main thread, it waits for commands from gps_state_start/stop and,
 * when started, messages from the QEMU GPS daemon. these are simple NMEA sentences
 * that must be parsed to be converted into GPS fixes sent to the framework
 */
static void
gps_state_thread( void  *arg )
{
    GpsState   *state = (GpsState *) arg;
    NmeaReader  reader[1];
    int         epoll_fd   = epoll_create(2);
    //    int         started    = 0;
    int         gps_fd     = state->fd;
    int         control_fd = state->control[1];

    nmea_reader_init( reader );

    // register control file descriptors for polling
    epoll_register( epoll_fd, control_fd );
    epoll_register( epoll_fd, gps_fd );

    LOGD("gps thread running");

    // now loop
    for (;;)
    {
        struct epoll_event   events[2];
        int                  ne, nevents;

        nevents = epoll_wait( epoll_fd, events, 2, 200 );
        if (nevents < 0)
        {
            if (errno != EINTR)
                D("epoll_wait() unexpected error: %s", strerror(errno));
            continue;
        }
        // D("[futengfei]===HCPUgps thread received %d events", nevents);
        for (ne = 0; ne < nevents; ne++)
        {
            if ((events[ne].events & (EPOLLERR | EPOLLHUP)) != 0)
            {
                D("EPOLLERR or EPOLLHUP after epoll_wait() !?");
                return;
            }
            if ((events[ne].events & EPOLLIN) != 0)
            {
                int  fd = events[ne].data.fd;

                if (fd == control_fd)
                {
                    char  cmd = 255;
                    int   ret;
                    LOGD("gps control fd event");
                    do
                    {
                        ret = read( fd, &cmd, 1 );
                    }
                    while (ret < 0 && errno == EINTR);

                    if (cmd == CMD_QUIT)
                    {
                        D("gps thread quitting on demand");
                        return;
                    }
                    else if (cmd == CMD_START)
                    {
                        /*if (!started)*/
                        {
                            D("gps thread starting  location_cb=%p", state->callbacks.location_cb);
                            //   started = 1;
                            nmea_reader_set_callback( reader, state->callbacks.location_cb );
                            reader->callbacks = state->callbacks;
                            reader->status.size = sizeof(GpsStatus);
                            reader->status.status = GPS_STATUS_SESSION_BEGIN;
                            reader->callbacks.status_cb(&reader->status);
                        }
                    }
                    else if (cmd == CMD_STOP)
                    {
                        /*  if (started)*/
                        {
                            D("gps thread stopping");
                            // started = 0;
                            reader->status.size = sizeof(GpsStatus);
                            //reader->status.status = GPS_STATUS_ENGINE_ON;
                            reader->status.status = GPS_STATUS_ENGINE_OFF;
                            reader->callbacks.status_cb(&reader->status);
                            reader->sv.size = sizeof(GpsSvStatus);
                            reader->sv.num_svs = 0;
                            reader->sv.ephemeris_mask = 0;
                            reader->sv.almanac_mask = 0;
                            reader->sv.used_in_fix_mask = 0;
                            reader->fix_mask = 0;
                            reader->callbacks.sv_status_cb(&reader->sv);

                            nmea_reader_set_callback( reader, NULL );
                        }
                    }
                }
                else if ((fd == gps_fd) /*&& started*/)
                {
                    char  buff[512];
                    D("gps fd event");
                    for (;;)
                    {
                        int  nn, ret;

                        ret = read( fd, buff, sizeof(buff) );
                        if (ret < 0)
                        {
                            if (errno == EINTR)
                                continue;
                            if (errno != EWOULDBLOCK)
                            {
                                D("error while reading from gps daemon socket: %s:", strerror(errno));
                            }
                            break;
                        }
                        D("received %d bytes: %.*s", ret, ret, buff);
                        for (nn = 0; nn < ret; nn++)
                            nmea_reader_addc( reader, buff[nn] );
                    }
                    D("gps fd event end");
                }
                else
                {
                    D("epoll_wait() returned unkown fd %d ?", fd);
                }
            }
        }
    }
}


static void
gps_state_init( GpsState  *state, GpsCallbacks *callbacks )
{
    int ret;
    state->init       = 1;
    state->control[0] = -1;
    state->control[1] = -1;
    state->fd         = -1;

again:
    state->fd = open(GPS_DEV_NAME, O_RDONLY);
    if (state->fd < 0)
    {
        LOGD("[ublox]futengfei err_access.again=======[%s]", GPS_DEV_NAME);
        lidbg("[ublox]futengfei err_access.again=======[%s]\n", GPS_DEV_NAME);
        sleep(1);
        goto again;
    }
    D("gps will read from '%s'", GPS_DEV_NAME);
    ret = ioctl(state->fd, GPS_STOP);
    if (ret)
    {
        D("ioctl GPS_STOP fail");
    }
    if ( socketpair( AF_LOCAL, SOCK_STREAM, 0, state->control ) < 0 )
    {
        D("could not create thread control socket pair: %s", strerror(errno));
        goto Fail;
    }

    state->thread = callbacks->create_thread_cb( "gps_state_thread", gps_state_thread, state );

    if ( !state->thread )
    {
        D("could not create gps thread: %s", strerror(errno));
        goto Fail;
    }

    state->callbacks = *callbacks;

    D("gps state initialized");
    return;

Fail:
    gps_state_done( state );
}


/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       I N T E R F A C E                               *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

static int ublox_gps_start();
static int
ublox_gps_init(GpsCallbacks *callbacks)
{
    GpsState  *s = _gps_state;

    if (!s->init)
        gps_state_init(s, callbacks);

    if (s->fd < 0)
        return -1;

    return 0;
}

static void
ublox_gps_cleanup(void)
{
    GpsState  *s = _gps_state;

    if (s->init)
        gps_state_done(s);
}


static int
ublox_gps_start()
{
    int ret;

    GpsState  *s = _gps_state;

    if (!s->init)
    {
        LOGD("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }

    LOGD("%s: called", __FUNCTION__);
    gps_state_start(s);
    ret = ioctl(s->fd, GPS_START);
    if (ret)
    {
        LOGD("ioctl error");
    }
    return 0;
}


static int
ublox_gps_stop()
{
    int ret;
    GpsState  *s = _gps_state;

    if (!s->init)
    {
        D("%s: called with uninitialized state !!", __FUNCTION__);
        return -1;
    }

    LOGD("%s: called", __FUNCTION__);

    gps_state_stop(s);
    ret = ioctl(s->fd, GPS_STOP);
    if (ret)
    {
        LOGD("ioctl error");
    }
    return 0;
}


static int
ublox_gps_inject_time(GpsUtcTime time, int64_t timeReference, int uncertainty)
{
    return 0;
}

static int
ublox_gps_inject_location(double latitude, double longitude, float accuracy)
{
    return 0;
}

static void
ublox_gps_delete_aiding_data(GpsAidingData flags)
{
}

static int ublox_gps_set_position_mode(GpsPositionMode mode, int fix_frequency)
{
    // FIXME - support fix_frequency
    return 0;
}

static const void *
ublox_gps_get_extension(const char *name)
{
    // no extensions supported
    return NULL;
}

#ifdef SOC_msm8x25
static int
ublox_gps_update_criteria(UlpLocationCriteria criteria)
{
    return 0;
}
#endif

static const GpsInterface  ubloxGpsInterface =
{
    sizeof(GpsInterface),
    ublox_gps_init,
    ublox_gps_start,
    ublox_gps_stop,
    ublox_gps_cleanup,
    ublox_gps_inject_time,
    ublox_gps_inject_location,
    ublox_gps_delete_aiding_data,
    ublox_gps_set_position_mode,
    ublox_gps_get_extension,
#ifdef SOC_msm8x25
    ublox_gps_update_criteria,
#endif
};

const GpsInterface *gps__get_gps_interface(struct gps_device_t *dev)
{
    return &ubloxGpsInterface;
}

static int open_gps(const struct hw_module_t *module, char const *name,
                    struct hw_device_t **device)
{
    struct gps_device_t *dev = malloc(sizeof(struct gps_device_t));
    D("%s", __FUNCTION__);
    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t *)module;
    //    dev->common.close = (int (*)(struct hw_device_t*))close_lights;
    dev->get_gps_interface = gps__get_gps_interface;

    *device = (struct hw_device_t *)dev;

    return 0;
}


static struct hw_module_methods_t gps_module_methods =
{
    .open = open_gps
};

struct hw_module_t HAL_MODULE_INFO_SYM =
{
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = GPS_HARDWARE_MODULE_ID,
    .name = "ublox GPS Module",
    .author = "The Android Open Source Project",
    .methods = &gps_module_methods,
};

