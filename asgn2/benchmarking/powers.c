#include <stdlib.h>

int main(int argc, char **argv)
{
    int loops = 0;
    if (argc > 1)
        loops = atoi(argv[1]);

    unsigned res[4];
    unsigned ind = 0;

    while (ind < loops) {
        res[ind % 4] = ind * ind;
        ++ind;
    }

	return 0;
}
