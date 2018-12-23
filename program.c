/****
Copyright (c) 2018 Y Paritcher

This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser GeneralPublic License as published by the Free Software Foundation; 
either version 2.1 of the License, or (at your option)any later version.

This library is distributed in the hope that it will be useful,but WITHOUT ANY WARRANTY; 
without even the impliedwarranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
See the GNU Lesser General Public License for more details. 
You should have received a copy of the GNU Lesser General Public License along with this library; 
if not, write tothe Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA, 
or connect to: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
****/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define NONE LIBZMANIMNONE
#include "hebrewcalendar.h"
#include "zmanim.h"
#include "hdateformat.h"
#define NONE FBINKNONE
#include "fbink.h"
#undef NONE

int fontsize = 30;
int spacing = 13;
int topstart = 53;
int rightcol = 40;
int leftcol = 420;

char* formattime(hdate date)
{
	static char final[10];
	final[0] = '\0';
	time_t time = hdatetime_t(&date);
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

const char* parshahday(hdate *date)
{
	if (date->wday == 7)
	{
		return parshahformat(getparshah(date));
	} else {
		hdate shabbos = *date;
		hdateaddday(&shabbos, (7-shabbos.wday));
		return parshahformat(getparshah(&shabbos));
	}
}

hdate getnightfall(hdate *date, location *here)
{
	if (isassurbemelachah(date))
		{return gettzais8p5(date, here);}
	else {return gettzaisbaalhatanya(date, here);}
}

int parseargs(int argc, char *argv[], hdate *hebrewDate, location *here)
{
	here->latitude = 40.66896;
	here->longitude = -73.94284;
	here->elevation = 34;
	float timezone = -4.0;
	time_t now = time(NULL);
	struct tm *pltm = localtime(&now);
	struct tm ltm = *pltm;
	_Bool ey = 0;

	if ( argc != 1 )
	{
		if ( argc != 8 )
		{
			printf("usage: year month day latitude longitude timezone Eretz_Yisroel\n       2018 01 01 40.66896 -73.94284 -4.0 0\n");
			return 1;
		}
		if ( atoi(argv[1]) )
		{
			ltm.tm_year = (atoi(argv[1]) - 1900 );
		}
		else
		{
			printf("%s: is not a valid year\n", argv[1]);
			return 1;
		}
		if ( atoi(argv[2]) )
		{
			ltm.tm_mon = (atoi(argv[2]) -1);
		}
		else
		{
			printf("%s: is not a valid month\n", argv[2]);
			return 1;
		}
		if ( atoi(argv[3]) )
		{
			ltm.tm_mday = atoi(argv[3]);
		}
		else
		{
			printf("%s: is not a valid year\n", argv[3]);
			return 1;
		}
		now = mktime(&ltm);
		if ( -90 < atof(argv[4]) && atof(argv[4]) < 90 )
		{
			here->latitude = atof(argv[4]);
		}
		else
		{
			printf("Latitude must be between -90 and 90!\n");
			return 1;
		}
		if ( -180 < atof(argv[5]) && atof(argv[5]) < 180)
		{
			here->longitude = atof(argv[5]);
		}
		else
		{
			printf("Longitude must be between -180 and 180!\n");
			return 1;
		}
		if( -13 < atof(argv[6]) && atof(argv[6]) < 15 )
		{
			timezone = atof(argv[6]);
		}
		else
		{
			printf("%s: is not a valid timezone\n", argv[6]);
			return 1;
		}
		if( -1 < atof(argv[7]) && atof(argv[7]) < 2 )
		{
				ey = atof(argv[7]);
		}
		else
		{
			printf("%s: is not a valid boolean, use 0 or 1\n", argv[6]);
			return 1;
		}
	}

	long int offset = (long int) 3600 * timezone;

	*hebrewDate = convertDate(&ltm);
	hebrewDate->offset=offset;
	setEY(hebrewDate, ey);
	return 0;
}

int main(int argc, char *argv[])
{
	location here;
	hdate hebrewDate;
	parseargs(argc, argv, &hebrewDate, &here);

	FBInkConfig config1 = {.is_quiet=1, .halign=EDGE, .is_cleared=1, .bg_color=BG_GRAYD};
	fbink_init(FBFD_AUTO, &config1);
	fbink_add_ot_font("/mnt/us/zman/ezra.ttf", FNT_REGULAR);
	fbink_print_image(FBFD_AUTO, "/mnt/us/zman/base.png", 0, 0, &config1);
//fbink_print(FBFD_AUTO, "", &config1);

	FBInkConfig config6 = {.is_quiet=1, .halign=EDGE, .is_centered=1, .bg_color=BG_GRAYD};
	fbink_init(FBFD_AUTO, &config6);
	FBInkOTConfig fontconf7 = {.margins={.top=50,.right=0}, .size_pt=fontsize};
	
	char kzman0[50]={'\0'};
	hdate night = getnightfall(&hebrewDate, &here);
	if (hdatetime_t(&hebrewDate) > hdatetime_t(&night))
	{
		hdateaddday(&hebrewDate, 1);
		strncat(kzman0, "ליל ", strlen("ליל "));
	}
	strncat(kzman0, hdateformat(&hebrewDate), strlen(hdateformat(&hebrewDate)));
	reverse_string(kzman0);
	topstart = fbink_print_ot(FBFD_AUTO, kzman0, &fontconf7, &config6)+spacing;

	char parsha[50]={'\0'};
	strncat(parsha, parshahday(&hebrewDate), strlen(parshahday(&hebrewDate)));
	reverse_string(parsha);
	FBInkOTConfig fontconf20 = {.margins={.top=125,.right=0}, .size_pt=fontsize};
	topstart = fbink_printf(FBFD_AUTO, &fontconf20, &config6, "%s %s", parsha, "תשרפ")+spacing;
	
	FBInkConfig config3 = {.is_quiet=1, .halign=EDGE, .bg_color=BG_GRAYD};
	fbink_init(FBFD_AUTO, &config3);
	
//	FBInkOTConfig fontconf1 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "רחשה תולע", &fontconf1, &config3);
	FBInkOTConfig fontconf4 = {.margins={.top=200, .right=leftcol}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, formattime(getalosbaalhatanya(&hebrewDate, &here)), &fontconf4, &config3)+spacing;
//	FBInkOTConfig fontconf5 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "ריכישמ", &fontconf5, &config3);
	FBInkOTConfig fontconf6 = {.margins={.top=275, .right=leftcol}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, formattime(getmisheyakir10p2degrees(&hebrewDate, &here)), &fontconf6, &config3)+spacing;
//	FBInkOTConfig fontconf8 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "המחה ץנ", &fontconf8, &config3);
	FBInkOTConfig fontconf9 = {.margins={.top=350, .right=leftcol}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, formattime(getsunrise(&hebrewDate, &here)), &fontconf9, &config3)+spacing;
//	FBInkOTConfig fontconf10 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "עמש תאירק", &fontconf10, &config3);
	FBInkOTConfig fontconf11 = {.margins={.top=425, .right=leftcol}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, formattime(getshmabaalhatanya(&hebrewDate, &here)), &fontconf11, &config3)+spacing;
//	FBInkOTConfig fontconf12 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "תוצח", &fontconf12, &config3);
	FBInkOTConfig fontconf13 = {.margins={.top=500, .right=leftcol}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, formattime(getchatzosbaalhatanya(&hebrewDate, &here)), &fontconf13, &config3)+spacing;
//	FBInkOTConfig fontconf14 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "הלודג החנמ", &fontconf14, &config3);
	FBInkOTConfig fontconf15 = {.margins={.top=575, .right=leftcol}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, formattime(getminchagedolabaalhatanya(&hebrewDate, &here)), &fontconf15, &config3)+spacing;
//	if (ltm.tm_wday == 5) printf("candle lighting: %s\n", formattime(getcandlelighting(&hebrewDate, &here)));
//	FBInkOTConfig fontconf16 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "העיקש", &fontconf16, &config3);
	FBInkOTConfig fontconf17 = {.margins={.top=650, .right=leftcol}, .size_pt=fontsize};
	topstart = fbink_print_ot(FBFD_AUTO, formattime(getsunset(&hebrewDate, &here)), &fontconf17, &config3)+spacing;
//	FBInkOTConfig fontconf18 = {.margins={.top=topstart, .right=rightcol}, .size_pt=fontsize};
//	fbink_print_ot(FBFD_AUTO, "םיבכוכה תאצ", &fontconf18, &config3);
	FBInkOTConfig fontconf19 = {.margins={.top=725, .right=leftcol}, .size_pt=fontsize};
	fbink_print_ot(FBFD_AUTO, formattime(getnightfall(&hebrewDate, &here)), &fontconf19, &config3);


	fbink_free_ot_fonts();
	return 0;
}
