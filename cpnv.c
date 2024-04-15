#define _XOPEN_SOURCE // This enables the declaration of strptime
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_SEGMENTS 8 // the instructions do not specify a maximum ammount of opening segments each days can have.
					   // I could implement a linked list to have virtually no limits, but I will not do it to respect
					   // the 4h time limit.
typedef struct
{
	int tm_min;	 //  minutes 0 to 59
	int tm_hour; //  hours   0 to 23
} TimeStamp;

typedef struct
{
	TimeStamp opening_hours[7][MAX_SEGMENTS][2]; //[day of the week][SEGMENT_ID][0: open time 1: close time]
} OpeningHours;

void exitError(char *str)
{
	write(2, str, strlen(str));
	exit(0);
}

bool is_between_open_close(int hour, int minute, int open_hour, int open_minute, int close_hour, int close_minute)
{
	if (hour < open_hour || (hour == open_hour && minute < open_minute))
		return false;
	if (hour > close_hour || (hour == close_hour && minute > close_minute))
		return false;
	return true;
}

void initializeDay(OpeningHours *opening_hours, int day)
{
	for (int seg = 0; seg < MAX_SEGMENTS; seg++)
	{
		for (int i = 0; i < 2; i++)
		{
			opening_hours->opening_hours[day][seg][i].tm_min = -1;
			opening_hours->opening_hours[day][seg][i].tm_hour = -1;
		}
	}
}

void initializeOpeningHours(OpeningHours *opening_hours)
{
	for (int i = 0; i < 7; i++)
		initializeDay(opening_hours, i);
}

int dayToIndex(char *day)
{
	int day_index = -1;
	const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	for (int i = 0; i < 7; i++)
	{
		if (strcmp(days[i], day) == 0)
		{
			day_index = i;
			break;
		}
	}
	return (day_index);
}

void setOpeningHoursTS(OpeningHours *opening_hours, char *day, TimeStamp opening_time, TimeStamp closing_time)
{
	int day_index = dayToIndex(day);
	if (day_index != -1)
	{
		initializeDay(opening_hours, day_index);
		opening_hours->opening_hours[day_index][0][0] = opening_time;
		opening_hours->opening_hours[day_index][0][1] = closing_time;
	}
	else
		exitError("[ERROR] setOpeningHours: Invalid day string");
}

int findNextSegmentAvailable(OpeningHours *opening_hours, int day_index)
{
	for (int i = 0; i < MAX_SEGMENTS; i++)
	{
		if (opening_hours->opening_hours[day_index][i][0].tm_hour == -1 &&
			opening_hours->opening_hours[day_index][i][1].tm_hour == -1)
			return (i);
	}
	exitError("findNextSegmentAvailable: no more space");
}

void addOpeningHoursTS(OpeningHours *opening_hours, char *day, TimeStamp opening_time, TimeStamp closing_time)
{
	int day_index = dayToIndex(day);
	if (day_index != -1)
	{
		int seg = findNextSegmentAvailable(opening_hours, day_index);
		opening_hours->opening_hours[day_index][seg][0] = opening_time;
		opening_hours->opening_hours[day_index][seg][1] = closing_time;
	}
	else
		exitError("[ERROR] setOpeningHours: Invalid day string");
}

void addOpeningHours(OpeningHours *opening_hours, char *day, char *opening_time, char *closing_time)
{
	TimeStamp open;
	TimeStamp close;
	if (*opening_time != '\0' && *closing_time != '\0')
	{
		if (sscanf(opening_time, "%d:%d", &open.tm_hour, &open.tm_min) != 2)
			exitError("setOpeningHours: invalid format");
		;
		if (sscanf(closing_time, "%d:%d", &close.tm_hour, &close.tm_min) != 2)
			exitError("setOpeningHours: invalid format");
		;
	}
	else
		exitError("setOpeningHours: invalid format");
	addOpeningHoursTS(opening_hours, day, open, close);
}

void setOpeningHours(OpeningHours *opening_hours, char *day, char *opening_time, char *closing_time)
{
	TimeStamp open;
	TimeStamp close;
	if (*opening_time == '\0' && *closing_time == '\0')
	{
		open.tm_hour = -1;
		open.tm_min = -1;
		close.tm_hour = -1;
		close.tm_min = -1;
	}
	else if (*opening_time != '\0' && *closing_time != '\0')
	{
		if (sscanf(opening_time, "%d:%d", &open.tm_hour, &open.tm_min) != 2)
			exitError("setOpeningHours: invalid format");
		;
		if (sscanf(closing_time, "%d:%d", &close.tm_hour, &close.tm_min) != 2)
			exitError("setOpeningHours: invalid format");
		;
	}
	else
		exitError("setOpeningHours: invalid format");
	setOpeningHoursTS(opening_hours, day, open, close);
}

bool isOpenOn(OpeningHours opening_hours, struct tm *date)
{
	TimeStamp open_time, close_time;
	bool res;
	for (int seg = 0; seg < MAX_SEGMENTS; seg++)
	{
		open_time = opening_hours.opening_hours[date->tm_wday][seg][0];
		close_time = opening_hours.opening_hours[date->tm_wday][seg][1];
		res = is_between_open_close(date->tm_hour, date->tm_min, open_time.tm_hour, open_time.tm_min, close_time.tm_hour,
									close_time.tm_min);
		if (res)
			return (res);
	}
	return (false);
}

struct tm nextOpeningDate(OpeningHours opening_hours, struct tm *current_date)
{
	struct tm next_date = *current_date;
	next_date.tm_sec = 0;						// current second is irrelevent if we assume a shop open at the beginning of a minute and
												// close at the end of a minute.

	while (isOpenOn(opening_hours, &next_date)) // in case shop is open. Push the next_date to the first non-open time
	{
		next_date.tm_min++;
		if (next_date.tm_min >= 60)
		{
			next_date.tm_min = 0;
			next_date.tm_hour += 1;
			if (next_date.tm_hour >= 24)
			{
				next_date.tm_hour = 0;
				next_date.tm_wday = (next_date.tm_wday + 1) % 7;
			}
		}
	}
	while (!isOpenOn(opening_hours, &next_date)) // find the next open time
	{ 
		next_date.tm_min++;
		if (next_date.tm_min >= 60)
		{
			next_date.tm_min = 0;
			next_date.tm_hour += 1;
			if (next_date.tm_hour >= 24)
			{
				next_date.tm_hour = 0;
				next_date.tm_wday = (next_date.tm_wday + 1) % 7;
			}
		}
	}

	return next_date;
}

int main()
{
	// Initialisation des heures d'ouverture
	OpeningHours opening_hours;
	initializeOpeningHours(&opening_hours);

	// Définition des heures d'ouverture
	setOpeningHours(&opening_hours, "Mon", "", "");
	setOpeningHours(&opening_hours, "Wed", "08:00", "16:00");
	setOpeningHours(&opening_hours, "Fri", "08:00", "16:00");
	setOpeningHours(&opening_hours, "Tue", "08:00", "12:00");
	setOpeningHours(&opening_hours, "Thu", "08:00", "12:00");
	setOpeningHours(&opening_hours, "Sat", "08:00", "12:00");
	setOpeningHours(&opening_hours, "Tue", "14:00", "18:00");
	setOpeningHours(&opening_hours, "Thu", "14:00", "18:00");
	setOpeningHours(&opening_hours, "Sat", "14:00", "18:00");
	addOpeningHours(&opening_hours, "Mon", "01:00", "01:10");
	addOpeningHours(&opening_hours, "Mon", "02:00", "02:10");
	addOpeningHours(&opening_hours, "Mon", "03:00", "03:10");
	addOpeningHours(&opening_hours, "Mon", "04:00", "04:10");
	addOpeningHours(&opening_hours, "Mon", "05:00", "05:10");
	addOpeningHours(&opening_hours, "Mon", "06:00", "06:10");
	addOpeningHours(&opening_hours, "Mon", "07:00", "07:10");
	addOpeningHours(&opening_hours, "Mon", "08:00", "08:10");

	// Dates de test
	struct tm test_date;
	char *test_dates[] = {
		"2024-02-21T07:45:00",
		"2024-02-22T12:22:11",
		"2024-02-24T09:15:00",
		"2024-02-25T09:15:00",
		"2024-02-23T08:00:00",
		"2024-02-26T08:00:00",
		"2024-02-22T14:00:00",
		"2024-04-15T00:59:00",
		"2024-04-15T01:59:00",
		"2024-04-15T02:09:00",
	};

	for (int i = 0; i < (int)(sizeof(test_dates)) / (int)(sizeof(test_dates[0])); i++)
	{
		strptime(test_dates[i], "%Y-%m-%dT%H:%M:%S", &test_date);
		printf("IsOpenOn(%s) == %s\n", test_dates[i], isOpenOn(opening_hours, &test_date) ? "true" : "false");

		struct tm next_opening = nextOpeningDate(opening_hours, &test_date);
		char next_opening_str[20];
		strftime(next_opening_str, sizeof(next_opening_str), "%Y-%m-%dT%H:%M:%S", &next_opening);
		printf("NextOpeningDate(%s) == %s\n", test_dates[i], next_opening_str);
	}
	return 0;
}

//TODO: read initial config file