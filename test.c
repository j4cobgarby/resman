#include <stdio.h>
#include <stdint.h>

struct weird {
    uint8_t x;
    uint8_t y;
    uint64_t z;
}
__attribute__((packed))
;

int main() {
    struct weird s;
    s.x = 10;
    s.y = 16;
    s.z = 2312312;

    printf("%p, %p, %p\n", &s.x, &s.y, &s.z);
}
