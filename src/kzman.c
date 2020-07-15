/****
Copyright (c) 2018 Y Paritcher
****/

#define FONTPATH "/mnt/us/zman/ezra.ttf"
#define BASEPATH "/mnt/us/zman/base.png"
#define BGPSHPATH "/mnt/us/zman/bgpicshuir.png"
#define CONFFILE "/mnt/us/zman/zman.conf"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include "hebrewcalendar.h"
#include "zmanim.h"
#include "hdateformat.h"
#include "shuir.h"
#include "fbink.h"
#include "openlipc.h"
#include "ini.h"

const FBInkConfig configCT = {.is_quiet=1, .halign=EDGE, .no_refresh=1, .is_bgless=1, .is_centered=1};
const FBInkConfig configLT = {.is_quiet=1, .halign=EDGE, .no_refresh=1, .is_bgless=1};
const FBInkConfig configRF = {.is_quiet=1, .is_flashing=1};
int fbfd;
FBInkDump dump = {0};
struct {
	_Bool EY;
	location here;
} place = {.here = {.latitude = 40.66896, .longitude = -73.94284, .elevation = 34}, .EY=0};
_Bool screenswitch = 0;
struct timespec sleeptime = {.tv_sec=1, .tv_nsec=500000000};

//screen size dependent
int linespace = 75;
int rightzmanmargin = 420;
int skiptanya = 150;
FBInkOTConfig basefontconf = {.margins={.top=50,.right=0}, .size_pt=30};
const char* zmanbg = BASEPATH;
const char* shuirbg = BGPSHPATH;

void reverse( char *start, char *end )
{
    while( start < end )
    {
        char c = *start;
        *start++ = *end;
        *end-- = c;
    }
}

char *reverse_char( char *start )
{
    char *end = start;
    while( (end[1] & 0xC0) == 0x80 ) end++;
    reverse( start, end );
    return( end+1 );
}

void reverse_string( char *string )
{
    char *end = string;
    while( *end ) end = reverse_char( end );
    reverse( string, end-1 );
}

hdate getnightfall(hdate date, location here)
{
	if (isassurbemelachah(date))
		{return gettzais8p5(date, here);}
	else {return gettzaisbaalhatanya(date, here);}
}

void print_ot(const char* restrict string, FBInkOTConfig* fontconf, const FBInkConfig* restrict fbink_cfg)
{
	fbink_print_ot(fbfd, string, fontconf, fbink_cfg, NULL);
	fontconf->margins.top += linespace;
}

void print_heb(char* string, FBInkOTConfig* fontconf)
{
	reverse_string(string);
	print_ot(string, fontconf, &configCT);
}

void print_time(hdate date, FBInkOTConfig* fontconf)
{
	char final[10] = {'\0'};
	time_t time = hdatetime_t(date);
	struct tm tm;
	localtime_r(&time, &tm);
	strftime(final, 6, "%-I:%M", &tm);
	print_ot(final, fontconf, &configLT);
}

void print_parshah(FBInkOTConfig* fontconf, hdate date)
{
	char parsha[50]={'\0'};
	memset(parsha, 0, sizeof parsha);
	hdate shabbos = date;
	if (getyomtov(date))
	{
		strncat(parsha, yomtovformat(getyomtov(date)), strlen(yomtovformat(getyomtov(date))));
	} else {
		if (date.wday != 0)
		{
			hdateaddday(&shabbos, (7-shabbos.wday));
		}
		if (getparshah(shabbos)) {
			strcat(parsha, "פרשת ");
			strncat(parsha, parshahformat(getparshah(shabbos)), strlen(parshahformat(getparshah(shabbos))));
		} else {
			strcat(parsha, "שבוע של ");
			strncat(parsha, yomtovformat(getyomtov(shabbos)), strlen(yomtovformat(getyomtov(shabbos))));
		}
	}
	print_heb(parsha, fontconf);
}

void print_shuir(FBInkOTConfig* fontconf, hdate hebrewDate, int (*f)(hdate date, char* buffer), _Bool rambam)
{
	char buf[100]={'\0'};
	memset(buf, 0, sizeof buf);
	(*f)(hebrewDate, buf);
	char * buf2 = strchr(buf, '\n');
	*buf2++ = '\0';
	char * buf3 = NULL;
	if (rambam){
		buf3 = strstr(buf2, " - ");
	}
	if (buf3){
		memset(buf3, '\0', sizeof(char)*2);
		buf3+=sizeof(char)*2;
	}
	print_heb(buf, fontconf);
	print_heb(buf2, fontconf);
	if (buf3){
		print_heb(++buf3, fontconf);
	}
}

void print_date(FBInkOTConfig* fontconf, hdate* hebrewDate, location here)
{
	char buf[36];
	size_t len = 36;
	memset(buf, 0, sizeof buf);
	char *date = buf;
	hdate night = getnightfall(*hebrewDate, here);
	if (hdatecompare(*hebrewDate, night) < 0)
	{
		hdateaddday(hebrewDate, 1);
		strcat(date, "ליל ");
		date += 7;
		len -= 7;
	}else if (hdatecompare(*hebrewDate, getalosbaalhatanya(*hebrewDate, here)) > 0){
		strcat(date, "ליל ");
		date += 7;
		len -= 7;
	}
	hdateformat(date, len, *hebrewDate);
	print_heb(buf, fontconf);
}

hdate getNow(_Bool EY)
{
	time_t now = time(NULL);
	struct tm *pltm = localtime(&now);
	hdate hebrewDate = convertDate(*pltm);
	hebrewDate.offset = pltm->tm_gmtoff;
	setEY(&hebrewDate, EY);
	return hebrewDate;
}

void zman()
{
	location here = place.here;
	_Bool EY = place.EY;
	hdate hebrewDate = getNow(EY);

	fbink_init(fbfd, &configCT);
	FBInkOTConfig fontconf = basefontconf;
	fbink_cls(fbfd, &configCT, NULL);
	fbink_print_image(fbfd, zmanbg, 0, 0, &configCT);

	print_date(&fontconf, &hebrewDate, here);
	print_parshah(&fontconf, hebrewDate);
	
	fbink_init(fbfd, &configLT);
	fontconf.margins.right = rightzmanmargin;

	print_time(getalosbaalhatanya(hebrewDate, here), &fontconf);
	print_time(getmisheyakir10p2degrees(hebrewDate, here), &fontconf);
	print_time(getsunrise(hebrewDate, here), &fontconf);
	print_time(getshmabaalhatanya(hebrewDate, here), &fontconf);
	print_time(getchatzosbaalhatanya(hebrewDate, here), &fontconf);
	print_time(getminchagedolabaalhatanya(hebrewDate, here), &fontconf);
	print_time(getsunset(hebrewDate, here), &fontconf);
	print_time(getnightfall(hebrewDate, here), &fontconf);

	fbink_refresh(fbfd, 0, 0, 0, 0, &configRF);
	syslog(LOG_INFO, "zman: new picture\n");
}

void shuir()
{
	location here = place.here;
	_Bool EY = place.EY;
	hdate hebrewDate = getNow(EY);

	FBInkOTConfig fontconf = basefontconf;
	fbink_init(fbfd, &configCT);
	fbink_cls(fbfd, &configCT, NULL);
	fbink_print_image(fbfd, shuirbg, 0, 0, &configCT);

	print_date(&fontconf, &hebrewDate, here);

	print_shuir(&fontconf, hebrewDate, chumash, 0);
	print_shuir(&fontconf, hebrewDate, tehillim, 0);
fontconf.margins.top += skiptanya;
	print_shuir(&fontconf, hebrewDate, rambam, 1);

	fbink_refresh(fbfd, 0, 0, 0, 0, &configRF);
	syslog(LOG_INFO, "shuir: new picture\n");
}

LIPCcode delta(LIPC *lipc)
{
	location here = place.here;
	_Bool EY = place.EY;
	hdate hebrewDate = getNow(EY);
	int delta;
	hdate next = getalosbaalhatanya(hebrewDate, here);
	if 	(hdatecompare(hebrewDate, next) != 1)
	{
		next = getnightfall(hebrewDate, here);
		if 	(hdatecompare(hebrewDate, next) != 1)
		{
			hdate tomorrow = hebrewDate;
			hdateaddday(&tomorrow, 1);
			next = getalosbaalhatanya(tomorrow, here);
		}
	}
	delta = hdatetime_t(next) - hdatetime_t(hebrewDate);
	delta += 45;

	LIPCcode ret = LIPC_OK;
	ret = LipcSetIntProperty(lipc, "com.lab126.powerd", "rtcWakeup", delta);

	syslog(LOG_INFO, "delta: %d\n", delta);
	return ret;
}

void printSS()
{
	switch (screenswitch)
	{
		case 0:
			zman();
			break;
		case 1:
			shuir();
			break;
	}
}

void goingToSS()
{
	fbink_dump(fbfd, &dump);
	screenswitch = !screenswitch;
	printSS();
}

void outOfSS()
{
	syslog(LOG_INFO, "outOfScreenSaver\n");
	if (access("/var/tmp/koreader.sh" , F_OK)) {
		syslog(LOG_INFO, "Restoring dump\n");
		fbink_restore(fbfd, &configRF, &dump);
	}
	fbink_free_dump_data(&dump);
}

LIPCcode wakeup(LIPC *lipc, LIPCevent *event)
{
	LIPCcode ret = LIPC_OK;
	char* state = NULL;
	_Bool print = 0;
	ret = LipcGetStringProperty(lipc, "com.lab126.powerd", "state", &state);
	if(ret){return ret;}
	syslog(LOG_INFO, "wakeupFromSuspend state: %s\n", state);
	if (!strcmp(state, "screenSaver") || !strcmp(state, "suspended"))
	{
		LipcFreeString(state);
		state = NULL;
		nanosleep(&sleeptime, NULL);
		ret = LipcGetStringProperty(lipc, "com.lab126.powerd", "state", &state);
		syslog(LOG_INFO, "wakeupFromSuspend state wait: %s\n", state);
		if (!strcmp(state, "screenSaver") || !strcmp(state, "suspended")){print=1;}
	}
	LipcFreeString(state);
//**********
//syslog(LOG_WARNING, "Name: %s\n", LipcGetEventName(event));
//int intparam = 0;
//char* stringparam = NULL;
//LipcGetIntParam(event, &intparam);
//LipcGetStringParam(event, &stringparam);
//syslog(LOG_WARNING, "Int: %d\n", intparam);
//syslog(LOG_WARNING, "String: %s\n", stringparam);
//**********
	if (print) {printSS();}
	return ret;
}

LIPCcode lipcCallback(LIPC *lipc, const char *name, LIPCevent *event __attribute__ ((unused)), void *data __attribute__ ((unused)))
{

	LIPCcode ret = LIPC_OK;
	if (!strcmp(name, "outOfScreenSaver")){
		outOfSS();
	} else if (!strcmp(name, "goingToScreenSaver")){
		goingToSS();
	} else if (!strcmp(name, "wakeupFromSuspend")){
		ret = wakeup(lipc, event);
	} else if (!strcmp(name, "readyToSuspend")){
		ret = delta(lipc);
	}
	return ret;
}

int setScreenSize(uint32_t width, uint32_t height)
{
	syslog(LOG_INFO, "Screen width: %u\nScreen height: %u\n", width, height);
	if (width == 600 &&  height == 800) { return 0;}
	if (width == 1080 &&  height == 1440)
	{
		linespace = 135;
		rightzmanmargin = 756;
		skiptanya = 270;
		basefontconf.margins.top = 90;
		basefontconf.margins.right = 0;
		basefontconf.size_pt = 30;
		zmanbg = "/mnt/us/zman/base300.png";
		shuirbg = "/mnt/us/zman/bgpicshuir300.png";
		return 0;
	}
	syslog(LOG_INFO, "Unknown screen size: %u x %u\n", width, height);
	return 0;
}

void config()
{
	ini_t *config = ini_load(CONFFILE);
	if (config)
	{
		ini_sget(config, NULL, "latitude", "%lf", &place.here.latitude);
		ini_sget(config, NULL, "longitude", "%lf", &place.here.longitude);
		ini_sget(config, NULL, "elevation", "%lf", &place.here.elevation);
		ini_sget(config, NULL, "EY", "%d", &place.EY);
		const char *timez = ini_get(config, NULL, "timezone");
		if (timez)
		{
			setenv("TZ", timez, 1);
		}
		ini_free(config);
	}
}

int main()
{
	int sig;
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGINT);
	sigprocmask(SIG_BLOCK, &set, NULL);
	
	config();
	openlog(NULL, LOG_PID, LOG_DAEMON);

	LIPCcode ret = LIPC_OK;

	fbfd = fbink_open();
	fbink_init(fbfd, &configCT);
	FBInkState fbink_state;
	fbink_get_state(&configCT, &fbink_state);
	if ((ret = setScreenSize(fbink_state.screen_width, fbink_state.screen_height)) != 0) {
		fbink_add_ot_font(FONTPATH, FNT_REGULAR);
		LIPC *lipc;
		if ((lipc = LipcOpenNoName()) != NULL)
		{
			ret = LipcSubscribeExt(lipc, "com.lab126.powerd", NULL, &lipcCallback, NULL);
			ret += LipcSetStringProperty(lipc, "com.lab126.blanket" , "unload", "screensaver");

			sigwait(&set, &sig);

			ret += LipcSetStringProperty(lipc, "com.lab126.blanket" , "load", "screensaver");
			ret += LipcUnsubscribeExt(lipc, "com.lab126.powerd", NULL, NULL);
			LipcClose(lipc);
		} else {ret = 1;}
		fbink_free_ot_fonts();
	}
	fbink_close(fbfd);
	return ret;
}
