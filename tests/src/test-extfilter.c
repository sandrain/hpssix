#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <hpssix.h>

int main(int argc, char **argv)
{
    uint64_t i = 0;
    hpssix_extfilter_t *filter = NULL;

    filter = hpssix_extfilter_get();
    assert(filter);

    printf("%lu registered extensions:\n", filter->n_exts);

    for (i = 0; i < filter->n_exts; i++)
        printf(" (%2lu)  %s\n", i+1, filter->exts[i]);

    hpssix_extfilter_free(filter);

    return 0;
}
