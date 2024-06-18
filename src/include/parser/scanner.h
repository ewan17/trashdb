#ifndef SCANNER_H
#define SCANNER_H

typedef union trashYYSTYPE
{
	int			ival;			
	char	   *str;			
} trashYYSTYPE;

typedef void *trashScanner;

extern trashScanner begin_scan();
extern void end_scan(trashScanner ts);

#endif //SCANNER_H