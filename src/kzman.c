/****
Copyright (c) 2018 Y Paritcher
****/

#define FONTPATH "/mnt/us/zman/ezra.ttf"
#define BASEPATH "/mnt/us/zman/base.png"
#define BGPSHPATH "/mnt/us/zman/bgpicshuir.png"
#define CONFFILE "/mnt/us/zman/zman.conf"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <syslog.h>
#include "hebrewcalendar.h"
#include "zmanim.h"
#include "hdateformat.h"
#include "shuir.h"
#include "fbink.h"
#include "openlipc.h"
#include "ini.h"

const FBInkConfig configBG = {.is_quiet=1, .halign=EDGE, .no_refresh=1, .bg_color=BG_GRAYD};
const FBInkConfig configCT = {.is_quiet=1, .halign=EDGE, .no_refresh=1, .is_bgless=1, .is_centered=1};
const FBInkConfig configLT = {.is_quiet=1, .halign=EDGE, .no_refresh=1, .is_bgless=1};
const FBInkConfig configCS = {.is_quiet=1, .no_refresh=1};
const FBInkConfig configRF = {.is_quiet=1};
typedef struct {
	int fbfd; _Bool EY;
	location place;
	FBInkDump* dump;
} CallbackData ;
_Bool screenswitch = 0;

char* formattime(hdate date)
{
	static char final[10];
	final[0] = '\0';
	time_t time = hdatetime_t(date);
	struct tm *tm = localtime(&time);
	strftime(final, 6, "%-I:%M", tm);
	return final;
}

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

char* parshahday(hdate date)
{
	static char parsha[50]={'\0'};
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
	return parsha;
}

hdate getnightfall(hdate date, location here)
{
	if (isassurbemelachah(date))
		{return gettzais8p5(date, here);}
	else {return gettzaisbaalhatanya(date, here);}
}

int fbopen()
{
	int fbfd = fbink_open();
	fbink_init(fbfd, &configCS);
	fbink_add_ot_font(FONTPATH, FNT_REGULAR);
	return fbfd;
}

void fbclose(int fbfd)
{
	fbink_refresh(fbfd, 0, 0, 0, 0, HWD_PASSTHROUGH, &configRF);
	fbink_free_ot_fonts();
	fbink_close(fbfd);
}

void print_ot(int fbfd, const char* restrict string, FBInkOTConfig* fontconf, const FBInkConfig* restrict fbink_cfg)
{
	fbink_print_ot(fbfd, string, fontconf, fbink_cfg, NULL);
	fontconf->margins.top += 75;
}

void print_heb(int fbfd, char* string, FBInkOTConfig* fontconf)
{
	reverse_string(string);
	print_ot(fbfd, string, fontconf, &configCT);
}

void print_shuir(int fbfd, FBInkOTConfig* fontconf, hdate hebrewDate, int (*f)(hdate date, char* buffer), _Bool rambam)
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
	print_heb(fbfd, buf, fontconf);
	print_heb(fbfd, buf2, fontconf);
	if (buf3){
		print_heb(fbfd, ++buf3, fontconf);
	}
}

void print_date(int fbfd, FBInkOTConfig* fontconf, hdate* hebrewDate, location here)
{
	fbink_init(fbfd, &configCT);
	
	char date[50]={'\0'};
	memset(date, 0, sizeof date);
	hdate night = getnightfall(*hebrewDate, here);
	if (hdatecompare(*hebrewDate, night) < 0)
	{
		hdateaddday(hebrewDate, 1);
		strcat(date, "ליל ");
	}else if (hdatecompare(*hebrewDate, getalosbaalhatanya(*hebrewDate, here)) > 0){
		strcat(date, "ליל ");
	}
	strncat(date, hdateformat(*hebrewDate), strlen(hdateformat(*hebrewDate)));
	print_heb(fbfd, date, fontconf);
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

int zman(CallbackData* callbackData)
{
	hdate hebrewDate = getNow(callbackData->EY);
	location here = callbackData->place;

	FBInkOTConfig fontconf = {.margins={.top=50,.right=0}, .size_pt=30};
	int fbfd = callbackData->fbfd;
	fbink_cls(fbfd, &configCS);
	fbink_init(fbfd, &configBG);
	fbink_print_image(fbfd, BASEPATH, 0, 0, &configBG);

	print_date(fbfd, &fontconf, &hebrewDate, here);

	char* parsha = parshahday(hebrewDate);
	print_heb(fbfd, parsha, &fontconf);
	
	fbink_init(fbfd, &configLT);
	fontconf.margins.right=420;

	print_ot(fbfd, formattime(getalosbaalhatanya(hebrewDate, here)), &fontconf, &configLT);
	print_ot(fbfd, formattime(getmisheyakir10p2degrees(hebrewDate, here)), &fontconf, &configLT);
	print_ot(fbfd, formattime(getsunrise(hebrewDate, here)), &fontconf, &configLT);
	print_ot(fbfd, formattime(getshmabaalhatanya(hebrewDate, here)), &fontconf, &configLT);
	print_ot(fbfd, formattime(getchatzosbaalhatanya(hebrewDate, here)), &fontconf, &configLT);
	print_ot(fbfd, formattime(getminchagedolabaalhatanya(hebrewDate, here)), &fontconf, &configLT);
	print_ot(fbfd, formattime(getsunset(hebrewDate, here)), &fontconf, &configLT);
	print_ot(fbfd, formattime(getnightfall(hebrewDate, here)), &fontconf, &configLT);

	syslog(LOG_INFO, "zman: new picture\n");
	return 0;
}

int shuir(CallbackData* callbackData)
{
	hdate hebrewDate = getNow(callbackData->EY);
	location here = callbackData->place;

	FBInkOTConfig fontconf = {.margins={.top=50,.right=0}, .size_pt=30};
	int fbfd = callbackData->fbfd;
	fbink_cls(fbfd, &configCS);
	fbink_init(fbfd, &configBG);
	fbink_print_image(fbfd, BGPSHPATH, 0, 0, &configBG);

	print_date(fbfd, &fontconf, &hebrewDate, here);

	print_shuir(fbfd, &fontconf, hebrewDate, chumash, 0);
	print_shuir(fbfd, &fontconf, hebrewDate, tehillim, 0);
fontconf.margins.top += 150;
	print_shuir(fbfd, &fontconf, hebrewDate, rambam, 1);

	syslog(LOG_INFO, "shuir: new picture\n");
	return 0;
}

int delta(LIPC *lipc, CallbackData* callbackData)
{
	hdate hebrewDate = getNow(callbackData->EY);
	location here = callbackData->place;
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

int printSS(CallbackData* callbackData)
{
	switch (screenswitch)
	{
		case 0:
			return zman(callbackData);
		case 1:
			return shuir(callbackData);
	}
	return 0;
}

LIPCcode goingToSS(CallbackData* callbackData)
{
	screenswitch = !screenswitch;
	int err = fbink_dump(callbackData->fbfd, callbackData->dump);
	if (err){fprintf(stderr, "SS dump: %s\n", strerror (err));}
	return printSS(callbackData);
}

LIPCcode outOfSS(CallbackData* callbackData)
{
	syslog(LOG_INFO, "outOfScreenSaver\n");
	int err = fbink_restore(callbackData->fbfd, &configRF, callbackData->dump);
	if (err){fprintf(stderr, "SS restore: %s\n", strerror (err));}
	fbink_free_dump_data(callbackData->dump);
	return LIPC_OK;
}

LIPCcode wakeup(LIPC *lipc, CallbackData* callbackData)
{
	LIPCcode ret = LIPC_OK;
	char* state = NULL;
	_Bool print = 0;
	ret = LipcGetStringProperty(lipc, "com.lab126.powerd", "state", &state);
	if(ret){return ret;}
	syslog(LOG_INFO, "wakeupFromSuspend state: %s\n", state);
	if (!strcmp(state, "screenSaver") || !strcmp(state, "suspended")){print=1;}
	LipcFreeString(state);
	if (print) {return printSS(callbackData);}
	return ret;
}

LIPCcode lipcCallback(LIPC *lipc, const char *name, LIPCevent *event, void *data)
{
	LIPCcode ret = LIPC_OK;
	CallbackData* callbackData = (CallbackData*)data;
	callbackData->fbfd = fbopen();
	if (!strcmp(name, "outOfScreenSaver")){
		ret = outOfSS(callbackData);
	} else if (!strcmp(name, "goingToScreenSaver")){
		ret = goingToSS(callbackData);
	} else if (!strcmp(name, "wakeupFromSuspend")){
		ret = wakeup(lipc, callbackData);
	} else if (!strcmp(name, "readyToSuspend")){
		ret = delta(lipc, callbackData);
	}
	fbclose(callbackData->fbfd);
	callbackData->fbfd = 0;
	return ret;
}

int main()
{
	location here = {.latitude = 40.66896, .longitude = -73.94284, .elevation = 34};
	_Bool EY = 0;
	ini_t *config = ini_load(CONFFILE);
	if (config)
	{
		ini_sget(config, NULL, "latitude", "%lf", &here.latitude);
		ini_sget(config, NULL, "longitude", "%lf", &here.longitude);
		ini_sget(config, NULL, "elevation", "%lf", &here.elevation);
		ini_sget(config, NULL, "EY", "%d", &EY);
		const char *timez = ini_get(config, NULL, "timezone");
		if (timez)
		{
			setenv("TZ", timez, 1);
		}
		ini_free(config);
	}

	openlog(NULL, LOG_PID, LOG_DAEMON);

	FBInkDump dump = {0};
	CallbackData callbackData = {.EY=EY, .place=here, .dump=&dump};
	LIPC *lipc;
	if ((lipc = LipcOpenNoName()) == NULL) {return 1;}
	LIPCcode ret = LIPC_OK;
	ret = LipcSubscribeExt(lipc, "com.lab126.powerd", NULL, &lipcCallback, &callbackData);
	LipcSetStringProperty(lipc, "com.lab126.blanket" , "unload", "screensaver");
sleep(60);
	LipcSetStringProperty(lipc, "com.lab126.blanket" , "load", "screensaver");
	ret = LipcUnsubscribeExt(lipc, "com.lab126.powerd", NULL, NULL);
	LipcClose(lipc);
	return ret;
}
