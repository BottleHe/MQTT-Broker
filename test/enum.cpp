#include <iostream>
using namespace std;

typedef enum test {
    T1 = 0,
    T2 = 1,
    T3 = 2,
    T4 = 3
} TEST;
int main(int argc, char* argv[]) {
    char a = 0x1;
    cout << "T1 = " << T1 << endl;
    cout << (T1 == static_cast<TEST>(a)) << endl;
    cout << "T2 = " << T2 << endl;
    cout << (T2 == static_cast<TEST>(a)) << endl;
    return 0;
}