#include <cassert>

// Example function to test
int add(int a, int b)
{
    return a + b;
}

int main()
{
    // Test case 1: Check if add(2, 2) equals 4
    assert(add(2, 2) == 4);

    // Test case 2: Check if add(-1, -1) equals -2
    assert(add(-1, -1) == -2);

    // If no assertions fail, exit with a success code
    return 0;
}
