#ifndef __RSH_H_
#define __RSH_H_

typedef enum redirection {
	INPUT, OUTPUT, APPEND, NONE
} Redirection;

// table 
char *redirectionTable[] = {"<", ">", ">>"};

#endif
