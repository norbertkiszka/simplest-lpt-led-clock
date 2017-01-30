/**
*	simplest-lpt-led-clock
*	Copyright (C) 2017  Norbert Kiszka <norbert at linux dot pl>
*	This program is free software; you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation; either version 2 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License along
*	with this program; if not, write to the Free Software Foundation, Inc.,
*	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/**
 * Currently - only common cathode supported.
 * Tested with 4 digit 7-segment display only.
 */

#include <sys/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>


/**
 * CONFIGURATION
 */

#define BASEPORT 0x378
#define DIGITS_COUNT 4
// REFRESH_TIME in us (microseconds) - currently used in REFRESH_TIME_ONE_DIGIT only
#define REFRESH_TIME 20000
#define REFRESH_TIME_ONE_DIGIT (REFRESH_TIME / DIGITS_COUNT)

// in us - how often to switch display mode
#define MODE_SWITCH_TIME 6500000

#define TEXT_FILE "/etc/lpt_led.txt"

// how often to reparse data to display per one whole display (one display = every working digit)
#define REPARSE_DATA_PER_DISPLAYS 5

// 1 to cache parsed data from txt file (re-parse on every display-mode switch). When given 0, it will parse on every loop (function main_loop)...
#define USE_FILE_CACHE 1

/**
 * CONFIGURATION - END
 */


typedef enum display_mode_e {display_clock, display_file} display_mode_t;

display_mode_t display_mode;

char conv[256];

char display[DIGITS_COUNT];
char display_file_cache[DIGITS_COUNT];
char dot[DIGITS_COUNT]; // 0 = disabled, 1 = dot visible


char parse_buffer[DIGITS_COUNT+1] = "";

void clear(void)
{
	memset(display, 0, sizeof display);
	memset(display_file_cache, 0, sizeof display_file_cache);
	memset(dot, 0, sizeof dot);
}

void reset(void)
{
	outb(0b00000100, BASEPORT+2);
	clear();
}

void init(void)
{
	if(ioperm(BASEPORT, 3, 1))
	{
		perror("ioperm");
		exit(1);
	}
	
	reset();
	
	display_mode = display_clock;
	
	memset(conv, 0b01100011, sizeof conv);
	
	/// NOTE: Not all of it was tested
	conv['a'] = 0b01110111;
	conv['A'] = 0b01110111;
	conv['b'] = 0b01111100;
	conv['B'] = 0b01111111;
	conv['c'] = 0b01011000;
	conv['C'] = 0b00111001;
	conv['d'] = 0b01011110;
	conv['D'] = 0b00111111;
	conv['e'] = 0b01111001;
	conv['E'] = 0b01111001;
	conv['f'] = 0b01110001;
	conv['F'] = 0b01110001;
	conv['g'] = 0b01101111;
	conv['G'] = 0b01111101;
	conv['h'] = 0b01110100;
	conv['H'] = 0b01110110;
	conv['i'] = 0b00000100;
	conv['I'] = 0b00000110;
	conv['j'] = 0b00001100;
	conv['J'] = 0b00001110;
	conv['k'] = 0b01111000;
	conv['K'] = 0b01111000;
	conv['l'] = 0b00011000;
	conv['L'] = 0b00111000;
	conv['m'] = 0b00100011;
	conv['n'] = 0b01010100;
	conv['N'] = 0b00110111;
	conv['o'] = 0b01011100;
	conv['O'] = 0b00111111;
	conv['p'] = 0b01110011;
	conv['P'] = 0b01110011;
	conv['q'] = 0b00111011;
	conv['Q'] = 0b00111011;
	conv['r'] = 0b01010000;
	conv['R'] = 0b00110001;
	conv['s'] = 0b01101101;
	conv['S'] = 0b01101101;
	conv['t'] = 0b00000101;
	conv['T'] = 0b00000101;
	conv['u'] = 0b00011100;
	conv['U'] = 0b00111110;
	conv['v'] = 0b00100110;
	conv['V'] = 0b00100110;
	conv['w'] = 0b00010010;
	conv['W'] = 0b00010010;
	conv['y'] = 0b01100110;
	conv['Y'] = 0b01100110;
	conv['x'] = 0b01011111;
	conv['X'] = 0b01011111;
	conv['z'] = 0b01011011;
	conv['Z'] = 0b01011011;
	
	conv['0'] = 0b00111111;
	conv['1'] = 0b00000110;
	conv['2'] = 0b01011011;
	conv['3'] = 0b01001111;
	conv['4'] = 0b01100110;
	conv['5'] = 0b01101101;
	conv['6'] = 0b01111101;
	conv['7'] = 0b00000111;
	conv['8'] = 0b01111111;
	conv['9'] = 0b01101111;
	
	conv['-'] = 0b01000000;
	conv['_'] = 0b01000000;
	conv[' '] = 0b00000000;
}

// for debugging
// const char *byte_to_binary(int x)
// {
//     static char b[9];
//     b[0] = '\0';
// 
//     int z;
//     for (z = 128; z > 0; z >>= 1)
//     {
//         strcat(b, ((x & z) == z) ? "1" : "0");
//     }
// 
//     return b;
// }


void set_one(char symbol, char which)
{
	if(which >= DIGITS_COUNT || which < 0)
	{
		//perror("Tried to set an unexisting display\n");
		return;
	}
	display[which] = conv[symbol];
	if(dot[which])
		display[which] |= 0b10000000;
	else
		display[which] &= ~(0b10000000);
}

/**
 * Parse given string and set it to display variable by set_one()
 * @var char* str pointer to string to parse it
 * @var char length 
 */
void parse_and_set(char *str, char length)
{
	int i, j = 0;
	
	clear();
	
	for(i = 0; i < length && i < DIGITS_COUNT; i++)
	{
		if(str[i] == '.' || str[i] == ':')
		{
			dot[i] = 1;
			j++;
		}
		set_one(str[j], i);
		j++;
	}
}

void parse_and_set_backwards(char *str, char length)
{
	int i, j;
	
	clear();
	
	if(length == 0)
		return;
	
	i = DIGITS_COUNT - 1;
	
	for(j = length - 1; i >= 0 && j >= 0; j--, i--)
	{
		if(str[j] == '.' || str[j] == ':')
		{
			dot[i] = 1;
			j--;
		}
		set_one(str[j], i);
	}
}

void main_loop(void);

int main(void)
{
	init();
	main_loop();
	return 0;
}

void parse_time_and_set(void)
{
	time_t rawtime;
	struct tm * timeinfo;
	
	time (&rawtime);
	timeinfo = localtime(&rawtime);
	
	if(timeinfo->tm_sec & 1)
		sprintf(parse_buffer, "%02d%02d", timeinfo->tm_hour, timeinfo->tm_min);
	else
		sprintf(parse_buffer, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
	parse_and_set_backwards(parse_buffer, strlen(parse_buffer));
}

void parse_file_and_set_cached(void)
{
	memcpy(display, display_file_cache, sizeof(display_file_cache));
}

void parse_file_and_set(void)
{
	// copied and modified from: http://stackoverflow.com/questions/3747086/reading-the-whole-text-file-into-a-char-array-in-c
	FILE *fp;
	long lSize;
	char *buffer;

	fp = fopen(TEXT_FILE, "rb");
	if(!fp)
	{
		perror("Cant read TEXT_FILE");
		display_mode = display_clock;
		return;
	}

	fseek(fp, 0L, SEEK_END);
	lSize = ftell(fp);
	rewind( fp );

	/* allocate memory for entire content */
	buffer = calloc( 1, lSize+1 );
	if(!buffer)
	{
		fclose(fp);
		fputs("memory alloc fails", stderr);
		display_mode = display_clock;
		return;
	}

	/* copy the file into the buffer */
	if(1 != fread(buffer, lSize, 1 ,fp))
	{
		fclose(fp);
		free(buffer);
		fputs("entire read fails", stderr);
		display_mode = display_clock;
		return;
	}

	//printf("%s\n", buffer);
	
	parse_and_set(buffer, lSize);

	fclose(fp);
	free(buffer);
	
	memcpy(display_file_cache, display, sizeof(display));
}

void main_loop(void)
{
	char current_digit = 0;
	struct timeval time;
	long long us_time_to_sleep, previous_sleep_time = 0, previous_sleep_at, previous_mode_switch_time, i;
	unsigned short redraws = 0; // see REPARSE_DATA_PER_DISPLAYS
	
	gettimeofday(&time, NULL);
	previous_sleep_at = previous_mode_switch_time = time.tv_sec * 1000000 + time.tv_usec;
	
	
	while(1)
	{
		redraws++;
		
		switch(current_digit)
		{
			case 0:
				outb(0b00000101, BASEPORT+2);
			break;
			case 1:
				outb(0b00000110, BASEPORT+2);
			break;
			case 2:
				outb(0b00000000, BASEPORT+2);
			break;
			case 3:
				outb(0b00001100, BASEPORT+2);
			break;
		}
		outb(display[current_digit], BASEPORT);
		
		gettimeofday(&time, NULL);
		
		us_time_to_sleep = REFRESH_TIME_ONE_DIGIT - ((time.tv_sec * 1000000 + time.tv_usec) - (previous_sleep_at + previous_sleep_time));
		
		if(us_time_to_sleep > 0)
			usleep(us_time_to_sleep);
		
		
		previous_sleep_at = time.tv_sec * 1000000 + time.tv_usec;
		
		if(us_time_to_sleep > 0)
			previous_sleep_time = us_time_to_sleep;
		else
			previous_sleep_time = 0;
		
		current_digit++;
		if(current_digit >= DIGITS_COUNT)
		{
			current_digit = 0;
			
			if(redraws >= REPARSE_DATA_PER_DISPLAYS)
			{
				redraws = 0;
				i = previous_sleep_at;
				if(us_time_to_sleep > 0)
					i += us_time_to_sleep;
				
				if(i - MODE_SWITCH_TIME > previous_mode_switch_time)
				{
					if(display_mode == display_file)
					{
						display_mode = display_clock;
					}
					else
					{
#if USE_FILE_CACHE
						parse_file_and_set();
#endif
						display_mode = display_file;
					}
					previous_mode_switch_time = i;
				}
				
				if(display_mode == display_file)
#if USE_FILE_CACHE
					parse_file_and_set_cached();
#else
					parse_file_and_set();
#endif
				else
					parse_time_and_set();
			}
		}
	}
}
