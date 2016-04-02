#include "BigInt.h"
#include <iostream>

using namespace std;

int main() {
    BigInt a("123456", 16);
    BigInt b("3456F", 16);
    BigInt n("374982", 16);
    auto c = ModPow(a, b, n);
    auto str = c.toString(10);
    printf("%s\n", a.toString(10).data());
    printf("%s\n", str.data());
    return 0;
}
