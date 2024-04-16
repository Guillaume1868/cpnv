#define _XOPEN_SOURCE // This enables the declaration of strptime
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "get_next_line.h"

#define MAX_TOKENS 12 //max tokens in a config file line. If file hat a time for every day of the week in a line. 
                      //It needs to be 12 (including null termined array)
#define MAX_SEARCH 10080 //max amount of minutes to search ahead for nextOpeningDate (10080 = one week)
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

const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
OpeningHours opening_hours;

void exitError(char *str)
{
	write(2, "[\x1b[31mERROR\x1b[0m] ", 18);
	write(2, str, strlen(str));
	write(2, "\n", 1);
	exit(0);
}

bool isBetweenOpenClose(int hour, int minute, int open_hour, int open_minute, int close_hour, int close_minute)
{
	if (hour < open_hour || (hour == open_hour && minute < open_minute))
		return false;
	if (hour > close_hour || (hour == close_hour && minute > close_minute))
		return false;
	return true;
}

void initializeDay(int day)
{
	for (int seg = 0; seg < MAX_SEGMENTS; seg++)
	{
		for (int i = 0; i < 2; i++)
		{
			opening_hours.opening_hours[day][seg][i].tm_min = -1;
			opening_hours.opening_hours[day][seg][i].tm_hour = -1;
		}
	}
}

void initializeOpeningHours()
{
	for (int i = 0; i < 7; i++)
		initializeDay(i);
}

int dayToIndex(char *day)
{
	int day_index = -1;
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

void setOpeningHoursTS(char *day, TimeStamp opening_time, TimeStamp closing_time)
{
	int day_index = dayToIndex(day);
	if (day_index != -1)
	{
		if (opening_time.tm_hour > closing_time.tm_hour
			||
			(
				opening_time.tm_hour == closing_time.tm_hour
				&&
				opening_time.tm_min > closing_time.tm_min
			)
			)
			exitError("addOpeningHoursTS: Closing time cannot be before opening time");
		initializeDay(day_index);
		opening_hours.opening_hours[day_index][0][0] = opening_time;
		opening_hours.opening_hours[day_index][0][1] = closing_time;
	}
	else
		exitError("setOpeningHours: Invalid day string");
}

int findNextSegmentAvailable(int day_index)
{
	for (int i = 0; i < MAX_SEGMENTS; i++)
	{
		if (opening_hours.opening_hours[day_index][i][0].tm_hour == -1 &&
			opening_hours.opening_hours[day_index][i][1].tm_hour == -1)
			return (i);
	}
	exitError("findNextSegmentAvailable: no more space");
	return(-1);
}

void addOpeningHoursTS(char *day, TimeStamp opening_time, TimeStamp closing_time)
{
	int day_index = dayToIndex(day);
	if (day_index != -1)
	{
		if (opening_time.tm_hour > closing_time.tm_hour
			||
			(
				opening_time.tm_hour == closing_time.tm_hour
				&&
				opening_time.tm_min > closing_time.tm_min
			)
			)
			exitError("addOpeningHoursTS: Closing time cannot be before opening time");
		int seg = findNextSegmentAvailable(day_index);
		opening_hours.opening_hours[day_index][seg][0] = opening_time;
		opening_hours.opening_hours[day_index][seg][1] = closing_time;
	}
	else
		exitError("setOpeningHours: Invalid day string");
}

void addOpeningHours(char *day, char *opening_time, char *closing_time)
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
	addOpeningHoursTS(day, open, close);
}


void setOpeningHours(char *day, char *opening_time, char *closing_time)
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
		if (sscanf(closing_time, "%d:%d", &close.tm_hour, &close.tm_min) != 2)
			exitError("setOpeningHours: invalid format");
	}
	else
		exitError("setOpeningHours: invalid format");
	setOpeningHoursTS(day, open, close);
}
void SetOpeningHours(char *day, char *opening_time, char *closing_time)
{
	//I usually code in lowerCamelCase, made this overload to respect the assignement
	setOpeningHours(day, opening_time, closing_time);
}


bool isOpenOn(struct tm *date)
{
	TimeStamp open_time, close_time;
	bool res;
	for (int seg = 0; seg < MAX_SEGMENTS; seg++)
	{
		open_time = opening_hours.opening_hours[date->tm_wday][seg][0];
		close_time = opening_hours.opening_hours[date->tm_wday][seg][1];
		res = isBetweenOpenClose(date->tm_hour, date->tm_min, open_time.tm_hour, open_time.tm_min,
			close_time.tm_hour, close_time.tm_min);
		if (res)
			return (res);
	}
	return (false);
}
bool IsOpenOn(struct tm *date)
{
	//I usually code in lowerCamelCase, made this overload to respect the assignement
	return (isOpenOn(date));
}

void addMinute(struct tm *next_date)
{
	next_date->tm_min++;
	if (next_date->tm_min >= 60)
	{
		next_date->tm_min = 0;
		next_date->tm_hour += 1;
		if (next_date->tm_hour >= 24)
		{
			next_date->tm_hour = 0;
			next_date->tm_wday = (next_date->tm_wday + 1) % 7;
			next_date->tm_mday += 1; 
			if (next_date->tm_mday >= 32) 
			{
				next_date->tm_mday -= 31;
				next_date->tm_mon += 1;
				if (next_date->tm_mon >= 12)
				{
					next_date->tm_mon -= 12;
					next_date->tm_year += 1;
				}
			}
		}
	}
}

struct tm nextOpeningDate(struct tm *current_date)
{
	int i = 0;
	struct tm next_date = *current_date;
	next_date.tm_sec = 0;	// current second is irrelevent if we assume a shop open at the beginning of a minute and
							// close at the end of a minute.

	while (isOpenOn(&next_date)) // in case shop is open. Push the next_date to the first non-open time
	{
		addMinute(&next_date);
		if (++i > MAX_SEARCH)
			exitError("nextOpeningDate: No closing time found. Shop is open 24/7");
	}
	i = 0;
	while (!isOpenOn(&next_date)) // find the next open time
	{ 
		addMinute(&next_date);
		if (++i > MAX_SEARCH)
			exitError("nextOpeningDate: No future opening founds.");
	}

	return next_date;
}
struct tm NextOpeningDate(struct tm *current_date)
{
	//I usually code in lowerCamelCase, made this overload to respect the assignement
	return (nextOpeningDate(current_date));
}
void readInputFile(char *filename) 
{
	int fd1 = open(filename, O_RDONLY);
	char *tokens[MAX_TOKENS];
	char *line;
	char *open_str, *close_str;
	int i = 0;
	while ((line = get_next_line(fd1)))
	{
		//printf("[\x1b[35;1mDEBUG\x1b[0m] Line input |%s|\n", line);
		char *token =strtok(line, " ,\n");
		//save tokens
		while (token)
		{
			tokens[i] = token;
			token = strtok(0, " ,\n");
			if (i >= MAX_TOKENS - 1)
				exitError("readInputFile: Maximum parsing token reached. Please respect input format");
			i++;
		}
		while (i < MAX_TOKENS) //puts the last cells to zero
			tokens[i++] = 0; 
		//save times
		i = 0;
		while ( tokens[i] && strcmp(tokens[i],"from"))
		{ 
			i++;
		}
		if (!tokens[i]) //check if from is found or missing
				exitError("readInputFile: from keyword not found. Please respect input format");
		if (tokens[++i] == 0) // advance index to time string and check data is there
			exitError("readInputFile: Formatting error. Please respect input format");
		open_str = tokens[i];
		// printf("[\x1b[35;1mDEBUG\x1b[0m] Open hour saved to %s\n", open_str);
		while ( tokens[i] && strcmp(tokens[i],"to"))
		{ 
			i++;
		}
		if (!tokens[i])  //check if to is found or missing
				exitError("readInputFile: to keyword not found. Please respect input format");
		if (tokens[++i] == 0) // advance index to time string and check data is there
			exitError("readInputFile: Formatting error. Please respect input format"); 
		close_str = tokens[i];
		// printf("[\x1b[35;1mDEBUG\x1b[0m] Close hour saved to %s\n", close_str);
		//save schedule into data structures
		i = 0;
		while (strcmp(tokens[i], "from") != 0)
		{
			addOpeningHours(tokens[i], open_str, close_str); // can be setOpeningHours or addOpeningHours
			i++;
		}
		i = 0;
		free(line);
	}

}

void dbgPrintOpeningHours(OpeningHours *oh)
{
	int i = 0;
	int j = 0;
	while (i <= 6)
	{
		printf("[\x1b[35;1mDEBUG\x1b[0m] \x1b[1m%s\x1b[0m\n", days[i]);
		while (oh->opening_hours[i][j][0].tm_hour != -1 && j < MAX_SEGMENTS)
		{
			printf("[\x1b[35;1mDEBUG\x1b[0m] \x1b[36m%d\x1b[0m OPEN %02d:%02d CLOSE %02d:%02d\n", j,
				oh->opening_hours[i][j][0].tm_hour, oh->opening_hours[i][j][0].tm_min,
				oh->opening_hours[i][j][1].tm_hour, oh->opening_hours[i][j][1].tm_min);
			j++;
		}
		j = 0;
		i++;
	}
}

int main(int argc, char **argv)
{
	if (argc != 2)
		exitError("main: Please pass a config file in argument");
	
	struct tm next_opening;
	char next_opening_str[20];
	struct tm test_date;
	
	// Initialisation des heures d'ouverture
	initializeOpeningHours();

	readInputFile(argv[1]);
	
	printf("[Validation] Base\n");
	strptime("2024-02-21T07:45:00", "%Y-%m-%dT%H:%M:%S", &test_date);
	printf("IsOpenOn(\x1b[36;1m%s\x1b[0m %s) == %s\n", days[test_date.tm_wday], "2024-02-21T07:45:00", isOpenOn(&test_date) ? "\x1b[32mtrue\x1b[0m" : "\x1b[31mfalse\x1b[0m");
	
	strptime("2024-02-22T12:22:11", "%Y-%m-%dT%H:%M:%S", &test_date);
	printf("IsOpenOn(\x1b[36;1m%s\x1b[0m %s) == %s\n", days[test_date.tm_wday], "2024-02-22T12:22:11", isOpenOn(&test_date) ? "\x1b[32mtrue\x1b[0m" : "\x1b[31mfalse\x1b[0m");
	
	strptime("2024-02-25T09:15:00", "%Y-%m-%dT%H:%M:%S", &test_date);
	printf("IsOpenOn(\x1b[36;1m%s\x1b[0m %s) == %s\n", days[test_date.tm_wday], "2024-02-25T09:15:00", isOpenOn(&test_date) ? "\x1b[32mtrue\x1b[0m" : "\x1b[31mfalse\x1b[0m");

	

	strptime("2024-02-22T14:00:00", "%Y-%m-%dT%H:%M:%S", &test_date);
	next_opening = nextOpeningDate(&test_date);
	strftime(next_opening_str, sizeof(next_opening_str), "%Y-%m-%dT%H:%M:%S", &next_opening);
	printf("NextOpeningDate(\x1b[36;1m%s\x1b[0m %s) == \x1b[36;1m%s\x1b[0m %s\n", days[test_date.tm_wday], "2024-02-22T14:00:00", days[next_opening.tm_wday], next_opening_str);
	
	strptime("2024-02-24T09:15:00", "%Y-%m-%dT%H:%M:%S", &test_date);
	next_opening = nextOpeningDate(&test_date);
	strftime(next_opening_str, sizeof(next_opening_str), "%Y-%m-%dT%H:%M:%S", &next_opening);
	printf("NextOpeningDate(\x1b[36;1m%s\x1b[0m %s) == \x1b[36;1m%s\x1b[0m %s\n", days[test_date.tm_wday], "2024-02-24T09:15:00", days[next_opening.tm_wday], next_opening_str);
	
	strptime("2024-02-22T12:22:11", "%Y-%m-%dT%H:%M:%S", &test_date);
	next_opening = nextOpeningDate( &test_date);
	strftime(next_opening_str, sizeof(next_opening_str), "%Y-%m-%dT%H:%M:%S", &next_opening);
	printf("NextOpeningDate(\x1b[36;1m%s\x1b[0m %s) == \x1b[36;1m%s\x1b[0m %s\n", days[test_date.tm_wday], "2024-02-22T12:22:11", days[next_opening.tm_wday], next_opening_str);
	
	printf("[Validation] Extension\n");
	printf("Avant manipulations\n");
	dbgPrintOpeningHours(&opening_hours);
	SetOpeningHours("Mon", "", "");
	SetOpeningHours("Wed", "07:30", "15:45");
	SetOpeningHours("Sat", "07:30", "20:00");
	SetOpeningHours("Sun", "09:00", "13:00");
	printf("Apres manipulations\n");
	dbgPrintOpeningHours(&opening_hours);
	
	//le premier test de validation dans le README demande de tester la date "monday" 2024-02-25T10:20:00.
	//cette date correspond a un dimanche donc le resultat est true, contrairement au readme qui dit false.
	strptime("2024-02-25T10:20:00", "%Y-%m-%dT%H:%M:%S", &test_date);
	printf("IsOpenOn(\x1b[36;1m%s\x1b[0m %s) == %s\n", days[test_date.tm_wday], "2024-02-25T10:20:00", isOpenOn(&test_date) ? "\x1b[32mtrue\x1b[0m" : "\x1b[31mfalse\x1b[0m");
	
	strptime("2024-02-21T07:45:00", "%Y-%m-%dT%H:%M:%S", &test_date);
	printf("IsOpenOn(\x1b[36;1m%s\x1b[0m %s) == %s\n", days[test_date.tm_wday], "2024-02-21T07:45:00", isOpenOn(&test_date) ? "\x1b[32mtrue\x1b[0m" : "\x1b[31mfalse\x1b[0m");
	//une fois de plus le README a des erreurs sur cette ligne, IsOpenON thursday, mais la date thurday n'est
	//pas definie dans le readme. J'ai donc utilise la date saturday.
	strptime("2024-02-24T19:50:00", "%Y-%m-%dT%H:%M:%S", &test_date);
	printf("IsOpenOn(\x1b[36;1m%s\x1b[0m %s) == %s\n", days[test_date.tm_wday], "2024-02-24T19:50:00", isOpenOn(&test_date) ? "\x1b[32mtrue\x1b[0m" : "\x1b[31mfalse\x1b[0m");
	
	strptime("2024-02-25T09:15:00", "%Y-%m-%dT%H:%M:%S", &test_date);
	printf("IsOpenOn(\x1b[36;1m%s\x1b[0m %s) == %s\n", days[test_date.tm_wday], "2024-02-25T09:15:00", isOpenOn(&test_date) ? "\x1b[32mtrue\x1b[0m" : "\x1b[31mfalse\x1b[0m");

	//Je teste ici une date posee un lundi, pour verifier que la fermeture du lundi fonctionne

	strptime("2024-02-19T10:20:00", "%Y-%m-%dT%H:%M:%S", &test_date);
	printf("IsOpenOn(\x1b[36;1m%s\x1b[0m %s) == %s\n", days[test_date.tm_wday], "2024-02-19T10:20:00", isOpenOn(&test_date) ? "\x1b[32mtrue\x1b[0m" : "\x1b[31mfalse\x1b[0m");
	return 0;
}