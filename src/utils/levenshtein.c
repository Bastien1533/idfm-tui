#include <stddef.h>

size_t min(size_t a, size_t b)
{
        size_t temp = 0;
        temp = (a < b) ? a : b;

        return temp;
}

size_t lev(const char *s1, const char *s2, int i, int j)
{
        if (i == 0)
                return j;
        if (j == 0)
                return i;

        if (s1[i - 1] == s2[j - 1])
                return lev(s1, s2, i - 1, j - 1);
        return (1 + min(lev(s1, s2, i, j - 1),
                        min(lev(s1, s2, i - 1, j), lev(s1, s2, i - 1, j - 1))));
}

size_t levenshtein(const char *s1, const char *s2)
{
        size_t len_s1 = 0;
        size_t len_s2 = 0;

        const char *c1 = s1;
        while (*c1 != '\0') {
                len_s1++;
                c1++;
        }

        const char *c2 = s2;
        while (*c2 != '\0') {
                len_s2++;
                c2++;
        }
        return lev(s1, s2, len_s1, len_s2);
}
