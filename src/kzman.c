/****
Copyright (c) 2018 Y Paritcher
****/

#ifdef KINDLEBUILD
#define FONTPATH "/mnt/us/zman/ezra.ttf"
#define BASEPATH "/mnt/us/zman/base.png"
#define BGPSHPATH "/mnt/us/zman/bgpicshuir.png"
#define CONFFILE "/mnt/us/zman/zman.conf"
#else
#define FONTPATH "zman/ezra.ttf"
#define BASEPATH "zman/base.png"
#define BGPSHPATH "zman/bgpicshuir.png"
#define CONFFILE "zman/zman.conf"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "hebrewcalendar.h"
#include "zmanim.h"
#include "hdateformat.h"
#include "shuir.h"
#include "fbink.h"
#include "openlipc.h"
#include "ini.h"

#define FONTSIZE 30
#define LEFTCOL 420

const FBInkConfig configBG = {.is_quiet=1, .halign=EDGE, .no_refresh=1, .bg_color=BG_GRAYD};
const FBInkConfig configCT = {.is_quiet=1, .halign=EDGE, .no_refresh=1, .is_bgless=1, .is_centered=1};
const FBInkConfig configLT = {.is_quiet=1, .halign=EDGE, .no_refresh=1, .is_bgless=1};
const FBInkConfig configRF = {.is_quiet=1};
FBInkOTConfig fontconf = {.margins={.top=50,.right=0}, .size_pt=FONTSIZE};

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
	fbink_init(fbfd, &configRF);
	fbink_add_ot_font(FONTPATH, FNT_REGULAR);
	fbink_cls(fbfd, &configRF);
	return fbfd;
}

void fbclose(int fbfd)
{
	fbink_refresh(fbfd, 0, 0, 0, 0, HWD_PASSTHROUGH, &configRF);
	fbink_free_ot_fonts();
	fbink_close(fbfd);
}

void print_ot(int fbfd, const char* restrict string, const FBInkConfig* restrict fbink_cfg)
{
	fbink_print_ot(fbfd, string, &fontconf, fbink_cfg, NULL);
	fontconf.margins.top += 75;
}

void print_heb(int fbfd, char* string)
{
	reverse_string(string);
	print_ot(fbfd, string, &configCT);
}

void print_shuir(int fbfd, hdate hebrewDate, int (*f)(hdate date, char* buffer), _Bool rambam)
{
	char buf[100]={'\0'};
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
	print_heb(fbfd, buf);
	print_heb(fbfd, buf2);
	if (buf3){
		print_heb(fbfd, ++buf3);
	}
}

void print_date(int fbfd, hdate* hebrewDate, location here)
{
	fbink_init(fbfd, &configCT);
	
	char date[50]={'\0'};
	hdate night = getnightfall(*hebrewDate, here);
	if (hdatecompare(*hebrewDate, night) < 0)
	{
		hdateaddday(hebrewDate, 1);
		strcat(date, "ליל ");
	}else if (hdatecompare(*hebrewDate, getalosbaalhatanya(*hebrewDate, here)) > 0){
		strcat(date, "ליל ");
	}
	strncat(date, hdateformat(*hebrewDate), strlen(hdateformat(*hebrewDate)));
	print_heb(fbfd, date);
}

int zman(hdate date, location place)
{
	location here = place;
	hdate hebrewDate = date;

	int fbfd = fbopen();
	fbink_init(fbfd, &configBG);
	fbink_print_image(fbfd, BASEPATH, 0, 0, &configBG);

	print_date(fbfd, &hebrewDate, here);

	char* parsha = parshahday(hebrewDate);
	print_heb(fbfd, parsha);
	
	fbink_init(fbfd, &configLT);
	fontconf.margins.right=LEFTCOL;

	print_ot(fbfd, formattime(getalosbaalhatanya(hebrewDate, here)), &configLT);
	print_ot(fbfd, formattime(getmisheyakir10p2degrees(hebrewDate, here)), &configLT);
	print_ot(fbfd, formattime(getsunrise(hebrewDate, here)), &configLT);
	print_ot(fbfd, formattime(getshmabaalhatanya(hebrewDate, here)), &configLT);
	print_ot(fbfd, formattime(getchatzosbaalhatanya(hebrewDate, here)), &configLT);
	print_ot(fbfd, formattime(getminchagedolabaalhatanya(hebrewDate, here)), &configLT);
	print_ot(fbfd, formattime(getsunset(hebrewDate, here)), &configLT);
	print_ot(fbfd, formattime(getnightfall(hebrewDate, here)), &configLT);

	fbclose(fbfd);
	return 0;
}

int shuir(hdate date, location place)
{
	location here = place;
	hdate hebrewDate = date;

	int fbfd = fbopen();
	fbink_init(fbfd, &configBG);
	fbink_print_image(fbfd, BGPSHPATH, 0, 0, &configBG);

	print_date(fbfd, &hebrewDate, here);

	print_shuir(fbfd, hebrewDate, chumash, 0);
	print_shuir(fbfd, hebrewDate, tehillim, 0);
fontconf.margins.top += 150;
	print_shuir(fbfd, hebrewDate, rambam, 1);

	fbclose(fbfd);
	return 0;
}

int delta(hdate date, location place)
{
	int delta;
	hdate next = getalosbaalhatanya(date, place);
	if 	(hdatecompare(date, next) != 1)
	{
		next = getnightfall(date, place);
		if 	(hdatecompare(date, next) != 1)
		{
			hdate tomorrow = date;
			hdateaddday(&tomorrow, 1);
			next = getalosbaalhatanya(tomorrow, place);
		}
	}
	delta = hdatetime_t(next) - hdatetime_t(date);
	delta += 45;

#ifdef KINDLEBUILD
	LIPC *lipc;
	if ((lipc = LipcOpenNoName()) == NULL) {return 1;}
	LIPCcode ret = LIPC_OK;
	ret = LipcSetIntProperty(lipc, "com.lab126.powerd", "rtcWakeup", delta);
	LipcClose(lipc);
	printf("%d\n", ret);
#endif

	printf("%d", delta);
	return 0;
}

unsigned int context(int argc, char* argv[])
{
	const char *caller = strrchr(argv[0], '/');
	if (caller == NULL){caller = argv[0];}
    else {caller++;}
	if (!strcmp(caller, "delta")){return 3;}
	else if(!strcmp(caller, "shuir")){return 2;}
	else if(!strcmp(caller, "zman")){return 1;}
	if (argc > 1)
	{
		if (!strcmp(argv[1], "delta")){return 3;}
		else if(!strcmp(argv[1], "shuir")){return 2;}
		else if(!strcmp(argv[1], "zman")){return 1;}
	}
	return 0;
}

int main(int argc, char* argv[])
{
	unsigned int usl = 30;

	location here = {.latitude = 40.66896, .longitude = -73.94284, .elevation = 34};
	_Bool EY = 0;
	unsigned int state = context(argc, argv);
	ini_t *config = ini_load(CONFFILE);
	if (config)
	{
		ini_sget(config, NULL, "latitude", "%lf", &here.latitude);
		ini_sget(config, NULL, "longitude", "%lf", &here.longitude);
		ini_sget(config, NULL, "elevation", "%lf", &here.elevation);
		ini_sget(config, NULL, "EY", "%d", &EY);
		ini_sget(config, NULL, "delay", "%u", &EY);
		const char *timez = ini_get(config, NULL, "timezone");
		if (timez)
		{
			setenv("TZ", timez, 1);
		}
		if (!state)
		{
			const char *program = ini_get(config, NULL, "program");
			if(program)
			{
				if (!strcmp(program, "delta")){state = 3;}
				if (!strcmp(program, "shuir")){state = 2;}
				if (!strcmp(program, "zman")){state = 1;}
			}
		}
	ini_free(config);
	}

	usl *= 10000;
	usleep(usl);

	time_t now = time(NULL);
	struct tm *pltm = localtime(&now);
	hdate hebrewDate = convertDate(*pltm);
	hebrewDate.offset = pltm->tm_gmtoff;
	setEY(&hebrewDate, EY);

	if (!state) {state = (now%2)+1;}
	switch(state)
	{
		case 3:
			return delta(hebrewDate, here);
		case 2:
			return shuir(hebrewDate, here);
		case 1:
			return zman(hebrewDate, here);
	}
}
