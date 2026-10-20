#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <emscripten.h>

extern "C" {

    EMSCRIPTEN_KEEPALIVE
    void SetValue(int *a, int *b) {
        int original_a = *a;
        int original_b = *b;

        *a = original_a + original_b;
        *b = std::abs(original_a - original_b);
    }

    EMSCRIPTEN_KEEPALIVE
    int* countDigitFrequency(const char* str) {
        static int freq[10];
        for(int i = 0; i < 10; i++) freq[i] = 0;

        std::string s(str);
        for (char ch : s) {
            if (ch >= '0' && ch <= '9') {
                freq[ch - '0']++;
            }
        }
        return freq;
    }
}