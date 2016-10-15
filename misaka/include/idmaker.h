#ifndef ID_MAKER_H
#define ID_MAKER_H
#include <misaka.h>

struct idmaker{
    int start;
    int stop;

    struct list *store;
    int curr;
};

struct idmaker * idmaker_new(int start, int end);
int idmaker_get(struct idmaker *m, int *flag);
int idmaker_put(struct idmaker *m, int i);

#endif
