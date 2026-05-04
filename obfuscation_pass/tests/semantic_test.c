#include <stdio.h>

// Simple conditional
static int sign(int x) {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

// Loop with accumulator
static int sum(int n) {
    int s = 0;
    for (int i = 1; i <= n; i++)
        s += i;
    return s;
}

// Nested conditionals
static int clamp(int x, int lo, int hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

// Multiple early returns
static int classify(int x) {
    if (x < 0)   return -1;
    if (x == 0)  return 0;
    if (x < 10)  return 1;
    if (x < 100) return 2;
    return 3;
}

// Loop with conditional break
static int find(int arr[], int n, int target) {
    for (int i = 0; i < n; i++)
        if (arr[i] == target)
            return i;
    return -1;
}

// Nested loops
static int mat_trace(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (i == j)
                sum += i * n + j;
    return sum;
}

// Conditional with logical operators lowered to branches
static int between(int x, int lo, int hi) {
    if (x >= lo && x <= hi)
        return 1;
    return 0;
}

// Loop with multiple exits
static int first_negative(int arr[], int n) {
    int i = 0;
    while (i < n) {
        if (arr[i] < 0)
            return i;
        i++;
    }
    return -1;
}

// Recursive function
static int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

// Long if-else chain
static const char *day(int d) {
    if (d == 0) return "Sun";
    if (d == 1) return "Mon";
    if (d == 2) return "Tue";
    if (d == 3) return "Wed";
    if (d == 4) return "Thu";
    if (d == 5) return "Fri";
    if (d == 6) return "Sat";
    return "???";
}

int main() {
    // sign
    printf("%d %d %d\n", sign(-5), sign(0), sign(3));

    // sum
    printf("%d %d %d\n", sum(0), sum(1), sum(10));

    // clamp
    printf("%d %d %d\n", clamp(-5, 0, 10), clamp(5, 0, 10), clamp(15, 0, 10));

    // classify
    printf("%d %d %d %d %d\n", classify(-3), classify(0), classify(7), classify(42), classify(200));

    // find
    int arr1[] = {3, 1, 4, 1, 5, 9, 2, 6};
    printf("%d %d %d\n", find(arr1, 8, 9), find(arr1, 8, 7), find(arr1, 8, 3));

    // mat_trace
    printf("%d %d %d\n", mat_trace(1), mat_trace(3), mat_trace(4));

    // between
    printf("%d %d %d\n", between(5, 0, 10), between(-1, 0, 10), between(11, 0, 10));

    // first_negative
    int arr2[] = {1, 2, -3, 4, -5};
    int arr3[] = {1, 2, 3};
    printf("%d %d\n", first_negative(arr2, 5), first_negative(arr3, 3));

    // fib
    printf("%d %d %d %d\n", fib(0), fib(1), fib(7), fib(10));

    // day
    printf("%s %s %s\n", day(0), day(3), day(6));

    return 0;
}