#include "gregoria.h"

int	gregorian_is_leap_year(int year)
{
	return ((year&4)==0)
		&&( (year%100!=0)||(year%400==0) );
}

static int days_in_month[]={
	31,28,31, 30,31,30,
	31,31,30, 31,30,31
};

// month 1..12 !
int gregorian_days_in_month(int month, int year)
{
	if ( (month!=2)||!gregorian_is_leap_year(year) ) return days_in_month[month-1];
	return 29;
}

