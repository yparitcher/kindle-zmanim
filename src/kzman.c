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

int fontsize = 30;
int spacing = 13;
int topstart = 53;
int rightcol = 40;
int leftcol = 420;

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
			strncat(parsha, "פרשת ", strlen("פרשת "));
			strncat(parsha, parshahformat(getparshah(shabbos)), strlen(parshahformat(getparshah(shabbos))));
		} else {
			strncat(parsha, "שבוע של ", strlen("שבוע של "));
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

int zman(hdate date, location place)
{
	location here = place;
	hdate hebrewDate = date;

	FBInkConfig config1 = {.is_quiet=1, .halign=EDGE, .is_cleared=1, .bg_color=BG_GRAYD};
	fbink_init(FBFD_AUTO, &config1);
	fbink_add_ot_font(FONTPATH, FNT_REGULAR);
	fbink_print_image(FBFD_AUTO, BASEPATH, 0, 0, &config1);

	FBInkConfig config6 = {.is_quiet=1, .halign=EDGE, .is_centered=1, .bg_color=BG_GRAYD};
	fbink_init(FBFD_AUTO, &config6);
	FBInkOTConfig fontconf7 = {.margins={.top=50,.right=0}, .size_pt=fontsize};
	
	char kzman0[50]={'\0'};
	hdate night = getnightfall(hebrewDate, here);
	if (hdatecompare(hebrewDate, night) < 0)
	{
		hdateaddday(&hebrewDate, 1);
		strncat(kzman0, "ליל ", strlen("ליל "));
	}else if (hdatecompare(hebrewDate, getalosbaalhatanya(hebrewDate, here)) > 0){
		strncat(kzman0, "ליל ", strlen("ליל "));
	}
	strncat(kzman0, hdateformat(hebrewDate), strlen(hdateformat(hebrewDate)));
	reverse_string(kzman0);
	topstart = fbink_print_ot(FBFD_AUTO, kzman0, &fontconf7, &config6)+spacing;

	char* parsha = parshahday(hebrewDate);
	reverse_string(parsha);
	FBInkOTConfig fontconf20 = {.margins={.top=125,.right=0}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, parsha, &fontconf20, &config6)+spacing;
	
	FBInkConfig config3 = {.is_quiet=1, .halign=EDGE, .bg_color=BG_GRAYD};
	fbink_init(FBFD_AUTO, &config3);
	
//	FBInkOTConfig fontconf1 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "רחשה תולע", &fontconf1, &config3);
	FBInkOTConfig fontconf4 = {.margins={.top=200, .right=leftcol}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, formattime(getalosbaalhatanya(hebrewDate, here)), &fontconf4, &config3)+spacing;
//	FBInkOTConfig fontconf5 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "ריכישמ", &fontconf5, &config3);
	FBInkOTConfig fontconf6 = {.margins={.top=275, .right=leftcol}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, formattime(getmisheyakir10p2degrees(hebrewDate, here)), &fontconf6, &config3)+spacing;
//	FBInkOTConfig fontconf8 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "המחה ץנ", &fontconf8, &config3);
	FBInkOTConfig fontconf9 = {.margins={.top=350, .right=leftcol}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, formattime(getsunrise(hebrewDate, here)), &fontconf9, &config3)+spacing;
//	FBInkOTConfig fontconf10 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "עמש תאירק", &fontconf10, &config3);
	FBInkOTConfig fontconf11 = {.margins={.top=425, .right=leftcol}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, formattime(getshmabaalhatanya(hebrewDate, here)), &fontconf11, &config3)+spacing;
//	FBInkOTConfig fontconf12 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "תוצח", &fontconf12, &config3);
	FBInkOTConfig fontconf13 = {.margins={.top=500, .right=leftcol}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, formattime(getchatzosbaalhatanya(hebrewDate, here)), &fontconf13, &config3)+spacing;
//	FBInkOTConfig fontconf14 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "הלודג החנמ", &fontconf14, &config3);
	FBInkOTConfig fontconf15 = {.margins={.top=575, .right=leftcol}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, formattime(getminchagedolabaalhatanya(hebrewDate, here)), &fontconf15, &config3)+spacing;
//	if (ltm.tm_wday == 5) printf("candle lighting: %s\n", formattime(getcandlelighting(hebrewDate, here)));
//	FBInkOTConfig fontconf16 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "העיקש", &fontconf16, &config3);
	FBInkOTConfig fontconf17 = {.margins={.top=650, .right=leftcol}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, formattime(getsunset(hebrewDate, here)), &fontconf17, &config3)+spacing;
//	FBInkOTConfig fontconf18 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "םיבכוכה תאצ", &fontconf18, &config3);
	FBInkOTConfig fontconf19 = {.margins={.top=725, .right=leftcol}, .size_pt=fontsize};
	fbink_print_ot(FBFD_AUTO, formattime(getnightfall(hebrewDate, here)), &fontconf19, &config3);


	fbink_free_ot_fonts();
	return 0;
}

int shuir(hdate date, location place)
{
	location here = place;
	hdate hebrewDate = date;

	FBInkConfig config1 = {.is_quiet=1, .halign=EDGE, .is_cleared=1, .bg_color=BG_GRAYD};
	fbink_init(FBFD_AUTO, &config1);
	fbink_add_ot_font(FONTPATH, FNT_REGULAR);
	fbink_print_image(FBFD_AUTO, BGPSHPATH, 0, 0, &config1);

	FBInkConfig config6 = {.is_quiet=1, .halign=EDGE, .is_centered=1, .bg_color=BG_GRAYD};
	fbink_init(FBFD_AUTO, &config6);
	FBInkOTConfig fontconf7 = {.margins={.top=50,.right=0}, .size_pt=fontsize};
	
	char kzman0[50]={'\0'};
	hdate night = getnightfall(hebrewDate, here);
	if (hdatecompare(hebrewDate, night) < 0)
	{
		hdateaddday(&hebrewDate, 1);
		strncat(kzman0, "ליל ", strlen("ליל "));
	}else if (hdatecompare(hebrewDate, getalosbaalhatanya(hebrewDate, here)) > 0){
		strncat(kzman0, "ליל ", strlen("ליל "));
	}
	strncat(kzman0, hdateformat(hebrewDate), strlen(hdateformat(hebrewDate)));
	reverse_string(kzman0);
	topstart = fbink_print_ot(FBFD_AUTO, kzman0, &fontconf7, &config6)+spacing;
	fontconf7.margins.top += 75;

	char chumashbuf[100]={'\0'};
	chumash(hebrewDate, chumashbuf);
	char * chb2 = strchr(chumashbuf, '\n');
	*chb2++ = '\0';
	reverse_string(chumashbuf);
	topstart = fbink_printf(FBFD_AUTO, &fontconf7, &config6, "%s", chumashbuf)+spacing;
	fontconf7.margins.top += 75;
	reverse_string(chb2);
	topstart = fbink_printf(FBFD_AUTO, &fontconf7, &config6, "%s", chb2)+spacing;
	fontconf7.margins.top += 75;
	
	char tehillimbuf[100]={'\0'};
	tehillim(hebrewDate, tehillimbuf);
	char * teb2 = strchr(tehillimbuf, '\n');
	*teb2++ = '\0';
	reverse_string(tehillimbuf);
	topstart = fbink_printf(FBFD_AUTO, &fontconf7, &config6, "%s", tehillimbuf)+spacing;
	fontconf7.margins.top += 75;
	reverse_string(teb2);
	topstart = fbink_printf(FBFD_AUTO, &fontconf7, &config6, "%s", teb2)+spacing;
	fontconf7.margins.top += 75;

fontconf7.margins.top += 150;

	char rambambuf[100]={'\0'};
	rambam(hebrewDate, rambambuf);
	char * rab2 = strchr(rambambuf, '\n');
	*rab2++ = '\0';
	char * rab3 = strstr(rab2, " - ");
	if (rab3){
		memset(rab3, '\0', sizeof(char)*2);
		rab3+=sizeof(char)*2;
	}
	reverse_string(rambambuf);
	topstart = fbink_printf(FBFD_AUTO, &fontconf7, &config6, "%s", rambambuf)+spacing;
	fontconf7.margins.top += 75;
	reverse_string(rab2);
	topstart = fbink_printf(FBFD_AUTO, &fontconf7, &config6, "%s", rab2)+spacing;
	fontconf7.margins.top += 75;
	if (rab3){
		reverse_string(++rab3);
		topstart = fbink_printf(FBFD_AUTO, &fontconf7, &config6, "%s", rab3)+spacing;
		fontconf7.margins.top += 75;
	}

	fbink_free_ot_fonts();
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
	unsigned int usl = 300000;
	usleep(usl);

	location here = {.latitude = 40.66896, .longitude = -73.94284, .elevation = 34};
	unsigned int state = context(argc, argv);
	ini_t *config = ini_load(CONFFILE);
	if (config)
	{
		ini_sget(config, NULL, "latitude", "%lf", &here.latitude);
		ini_sget(config, NULL, "longitude", "%lf", &here.longitude);
		ini_sget(config, NULL, "elevation", "%lf", &here.elevation);
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

	time_t now = time(NULL);
	struct tm *pltm = localtime(&now);
	hdate hebrewDate = convertDate(*pltm);
	hebrewDate.offset = pltm->tm_gmtoff;
	setEY(&hebrewDate, 0);

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
