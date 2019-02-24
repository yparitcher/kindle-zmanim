/****
Copyright (c) 2019 Y Paritcher
****/

#include <stdio.h>
#include <time.h>
#include "hebrewcalendar.h"
#include "zmanim.h"
#include "openlipc.h"

hdate getnightfall(hdate date, location here)
{
	if (isassurbemelachah(date))
		{return gettzais8p5(date, here);}
	else {return gettzaisbaalhatanya(date, here);}
}

int main()
{
	int delta;
	location here = {.latitude = 40.66896, .longitude = -73.94284, .elevation = 34};
	time_t now = time(NULL);
	struct tm *pltm = localtime(&now);
	hdate hebrewDate = convertDate(*pltm);
	hebrewDate.offset=pltm->tm_gmtoff;
	setEY(&hebrewDate, 0);
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
