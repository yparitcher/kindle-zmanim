/****
Copyright (c) 2018 Y Paritcher
****/

#define FONTPATH "/mnt/us/zman/ezra.ttf"
#define BASEPATH "/mnt/us/zman/base.png"
#define BGPSHPATH "/mnt/us/zman/bgpicshuir.png"
#define CONFFILE "/mnt/us/zman/zman.conf"

#include <errno.h>
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
int screenswitch = 0;
_Bool program = 0;
struct timespec sleeptime = {.tv_sec=1, .tv_nsec=500000000};
uint32_t rota = KEEP_CURRENT_ROTATE;
int deltaG = -1;

//screen size dependent
int zmanlinespace = 75;
int shuirlinespace = 70;
int rightzmanmargin = 420;
FBInkOTConfig zmanfontconf = {.margins={.top=50,.right=0}, .size_pt=30};
FBInkOTConfig shuirfontconf = {.margins={.top=30,.right=0}, .size_pt=28};
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
    if ((end[0] >= '0') && (end[0] <= '9')) {
		while((end[1] >= '0') && (end[1] <= '9')) { end++; }
	} else {
		while( (end[1] & 0xC0) == 0x80 ) { end++; }
    }
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

void print_ot(const char* restrict string, FBInkOTConfig* fontconf, const FBInkConfig* restrict fbink_cfg, int margin)
{
	fbink_print_ot(fbfd, string, fontconf, fbink_cfg, NULL);
	fontconf->margins.top += margin;
}

void print_heb(char* string, FBInkOTConfig* fontconf, int margin)
{
	reverse_string(string);
	print_ot(string, fontconf, &configCT, margin);
}

void print_time(hdate date, FBInkOTConfig* fontconf, int margin)
{
	char final[10] = {'\0'};
	time_t time = hdatetime_t(date);
	struct tm tm;
	localtime_r(&time, &tm);
	strftime(final, 6, "%-I:%M", &tm);
	print_ot(final, fontconf, &configLT, margin);
}

void print_parshah(FBInkOTConfig* fontconf, hdate date, int margin)
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
	print_heb(parsha, fontconf, margin);
}

void print_shuir(FBInkOTConfig* fontconf, hdate hebrewDate, int (*f)(hdate date, char* buffer), int margin)
{
	char buf[250]={'\0'};
	memset(buf, 0, sizeof buf);
	(*f)(hebrewDate, buf);
	char * buf2 = strchr(buf, '\n');
	*buf2++ = '\0';
	char * buf3 = strchr(buf2, '\n');;
	if (buf3){ *buf3++ = '\0';}
	print_heb(buf, fontconf, margin);
	print_heb(buf2, fontconf, margin);
	if (buf3){
		print_heb(buf3, fontconf, margin);
	}
}

void print_date(FBInkOTConfig* fontconf, hdate* hebrewDate, location here, int margin)
{
	char buf[37];
	size_t len = 37;
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
	print_heb(buf, fontconf, margin);
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
	FBInkOTConfig fontconf = zmanfontconf;
	fbink_cls(fbfd, &configCT, NULL, 0);
	fbink_print_image(fbfd, zmanbg, 0, 0, &configCT);
	int margin = zmanlinespace;

	print_date(&fontconf, &hebrewDate, here, margin);
	print_parshah(&fontconf, hebrewDate, margin);
	
	fbink_init(fbfd, &configLT);
	fontconf.margins.right = rightzmanmargin;

	print_time(getalosbaalhatanya(hebrewDate, here), &fontconf, margin);
	print_time(getmisheyakir10p2degrees(hebrewDate, here), &fontconf, margin);
	print_time(getsunrise(hebrewDate, here), &fontconf, margin);
	print_time(getshmabaalhatanya(hebrewDate, here), &fontconf, margin);
	print_time(getchatzosbaalhatanya(hebrewDate, here), &fontconf, margin);
	print_time(getminchagedolabaalhatanya(hebrewDate, here), &fontconf, margin);
	print_time(getsunset(hebrewDate, here), &fontconf, margin);
	print_time(getnightfall(hebrewDate, here), &fontconf, margin);

	fbink_refresh(fbfd, 0, 0, 0, 0, &configRF);
	syslog(LOG_INFO, "zman: new picture\n");
}

void shuir()
{
	location here = place.here;
	_Bool EY = place.EY;
	hdate hebrewDate = getNow(EY);

	FBInkOTConfig fontconf = shuirfontconf;
	fbink_init(fbfd, &configCT);
	fbink_cls(fbfd, &configCT, NULL, 0);
	fbink_print_image(fbfd, shuirbg, 0, 0, &configCT);
	int margin = shuirlinespace;

	print_date(&fontconf, &hebrewDate, here, margin);

	print_shuir(&fontconf, hebrewDate, chumash, margin);
	print_shuir(&fontconf, hebrewDate, tehillim, margin);
	print_shuir(&fontconf, hebrewDate, tanya, margin);
	print_shuir(&fontconf, hebrewDate, rambam, margin);

	fbink_refresh(fbfd, 0, 0, 0, 0, &configRF);
	syslog(LOG_INFO, "shuir: new picture\n");
}

void PW5()
{
	//Stub
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

	deltaG = delta;
	syslog(LOG_INFO, "delta: %d\n", delta);
	return ret;
}

void printSS()
{
	FBInkState state = {0};
	fbink_get_state(&configCT, &state);
	uint32_t current_rota = state.current_rota;
	syslog(LOG_INFO, "Rota: %d\n", current_rota);
	if (rota != KEEP_CURRENT_ROTATE) {
		int ret = fbink_reinit(fbfd, &configCT);
		ret = fbink_set_fb_info(fbfd, rota, KEEP_CURRENT_BITDEPTH, KEEP_CURRENT_GRAYSCALE, &configCT);
		if (ret) {syslog(LOG_INFO, "Error rotating: %d\n", ret);}
	}

	switch (screenswitch)
	{
		case 0:
			zman();
			break;
		case 1:
			shuir();
			break;
		case 2:
			PW5();
			break;
	}

	if (rota != KEEP_CURRENT_ROTATE) {
		int ret2 = fbink_set_fb_info(fbfd, current_rota, KEEP_CURRENT_BITDEPTH, KEEP_CURRENT_GRAYSCALE, &configCT);
		if (ret2) {syslog(LOG_INFO, "Error reseting rotation: %d\n", ret2);}
	}
}

void goingToSS()
{
	fbink_dump(fbfd, &dump);

	if (!program)
	{
		screenswitch = !screenswitch;
	}
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
*/
LIPCcode lipcCallback(LIPC *lipc, const char *name, LIPCevent *event, void *data __attribute__ ((unused)))
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
	if (width == 1072 &&  height == 1448)
	{
		zmanlinespace = 136;
		shuirlinespace = 125;
		rightzmanmargin = 756;
		zmanfontconf.margins.top = 88;
		zmanfontconf.margins.right = 0;
		zmanfontconf.size_pt = 30;
		shuirfontconf.margins.top = 73;
		shuirfontconf.margins.right = 0;
		shuirfontconf.size_pt = 28;
		zmanbg = "/mnt/us/zman/base300.png";
		shuirbg = "/mnt/us/zman/bgpicshuir300.png";
		return 0;
	}
	if (width == 1236 &&  height == 1648)
	{
		program = 1;
		screenswitch = 2;
		return 0;
	}
	syslog(LOG_INFO, "Unknown screen size: %u x %u\n", width, height);
	return 0;
}

void logCurrentTime()
{
	char final[32];
	final[0] = '\0';
	struct tm tm;
	hdate now = getNow(place.EY);
	time_t time = hdatetime_t(now);
	localtime_r(&time, &tm);
	strftime(final, 31, "%I:%M %p %Z", &tm);
	syslog(LOG_INFO, "Start time: %s\n", final);
	syslog(LOG_INFO, "Start time (Hebrew): %d:%d-%d %d:%d:%d\n", now.year, now.month, now.day, now.hour, now.min, now.sec);

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
		ini_sget(config, NULL, "rota", "%lu", &rota);
		const char *timez = ini_get(config, NULL, "timezone");
		if (timez)
		{
			setenv("TZ", timez, 1);
		}
		const char *programs = ini_get(config, NULL, "program");
		if(programs)
		{
			program = 1;
			if (!strcmp(programs, "zman")) {screenswitch = 0;}
			else if (!strcmp(programs, "shuir")) {screenswitch = 1;}
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

	logCurrentTime();

	LIPCcode ret = LIPC_OK;

	fbfd = fbink_open();
	fbink_init(fbfd, &configCT);
	FBInkState fbink_state;
	fbink_get_state(&configCT, &fbink_state);
	if ((ret = setScreenSize(fbink_state.screen_width, fbink_state.screen_height)) == 0) {
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
		} else {
			syslog(LOG_INFO, "Unable to open LIPC\n");
			ret = 1;
		}
		fbink_free_ot_fonts();
	}
	fbink_close(fbfd);
	return ret;
}
