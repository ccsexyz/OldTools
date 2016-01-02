#ifndef YSPLIT_H
#define YSPLIT_H

char **ysplit1(char *src, const char *sp);
char **ysplit(const char *src, const char *sp);
char **ysplit_free(char **p);
char **ysplit_print(char **p);


#endif // YSPLIT_H
