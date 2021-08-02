#include <iostream>

#include "fss1bit.h"
#include "util.h"

using namespace std;

int main() {
    FSS1Bit generator;
    FSS1Bit evaluators[2];

    for (uint m = 1; m <= 20; m++) {
        uint64_t range = 1ULL << m;

        for (uint i = 0; i < 100; i++) {
            bool pass = true;
            uint64_t alpha = rand_long(range);
            uchar *keys[2];
            generator.Gen(alpha, m, keys);
            uchar *share0 = new uchar[range];
            uchar *share1 = new uchar[range];
            evaluators[0].EvalAll(keys[0], m, share0);
            evaluators[1].EvalAll(keys[1], m, share1);

            for (uint64_t x = 0; x < range; x++) {
                uchar output = share0[x] ^ share1[x];
                if (x == alpha) {
                    if (output == 0) {
                        cout << "Failed: alpha=" << alpha << ", x=" << x
                             << ", outValue=" << output << endl;
                        pass = false;
                    }
                } else {
                    if (output != 0) {
                        cout << "Failed: alpha=" << alpha << ", x=" << x
                             << ", outValue=" << output << endl;
                        pass = false;
                    }
                }
            }

            if (pass)
                cout << "m=" << m << ", i=" << i << ": passed" << endl;
            else
                cout << "m=" << m << ", i=" << i << ": failed" << endl;

            delete[] keys[0];
            delete[] keys[1];
            delete[] share0;
            delete[] share1;
        }
        cout << endl;
    }

    return 0;
}
