#include <Misra/Std.h>
#include <Misra/Types.h>

int main(int argc, char** argv) {
    LogInit(false);

    Vec(int) iv = VecInit();

    VecPushBack(&iv, 10);
    VecFirst(&iv) = 20;

    printf("%d", VecFirst(&iv));

    LogDeinit();
    return 0;
}
