/*
 * =====================================================================================
 *
 *       Filename:  strlib.c
 *
 *    Description:  Function for string process.
 *
 *        Version:  0.01.1
 *        Created:  11/11/2014 14:57:35 AM
 *       Revision:  none
 *
 *         Author:  Lin Hui (Link), hui.lin@nufront.com
 *   Organization:  Guangdong Nufront CSC Co., Ltd
 *
 *--------------------------------------------------------------------------------------          
 * ChangLog:
 *  version    Author      Date        Purpose
 *  0.01.1     Lin Hui    11/11/2014   Create and initialize it.   
 *     
 * =====================================================================================
 */

#include "at_cmd.h"


/*
 * Judge the string input is number string.
 * 0: is not a number, 1:is number string.
 **/
int str_is_digit(const char * str)
{
	int i = 0;
	int str_len;

	str_len = strlen(str);

	for (i = 0; i < str_len; i++) {
		if (!isdigit(str[i])) {
			return 0;
		}
	}

	return 1;
}

/* Check the validable of ip string.
 **/
int is_valid_ip(const char *ip) 
{ 
    int section = 0;
    int dot = 0;

    while (*ip) { 
        if (*ip == '.') { 
            dot++; 
            if (dot > 3) { 
                return -1; 
            } 

            if (section >= 0 && section <= 255) { 
                section = 0; 
            } else { 
                return -1; 
            } 
        } else if (*ip >= '0' && *ip <= '9') { 
            section = section * 10 + *ip - '0'; 
            			 
        } else { 
            return -1; 
        } 
        ip++;        
    }
	
	if (section > 0 && section <= 255)	{ 
        if (3 == dot) {
	        section = 0; 
	        return 0;
      	}
    }
		 
    return -1; 	
}


/*
 * Compare two string, if prefix is the sub string in line on the head, 
 * then return 1, else return 0.
 **/
int strStartsWith(const char *line, const char *prefix)
{
    for ( ; *line != '\0' && *prefix != '\0' ; line++, prefix++) {
        if (*line != *prefix) {
            return 0;
        }
    }

    return *prefix == '\0';
}


/*
 * Get next token from string *stringp, where tokens are possibly-empty
 * strings separated by characters from delim.
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no moretokens).
 *
 * If *stringp is NULL, strsep returns NULL.
 */
char *strsep(char **stringp, const char *delim)
{
    char *s;
    const char *spanp;
    int c, sc;
    char *tok;
    if ((s = *stringp)== NULL)
        return (NULL);
    for (tok = s;;) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc =*spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
}
