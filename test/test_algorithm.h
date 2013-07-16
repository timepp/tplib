#pragma once

#include <algorithm.h>
#include <unittest.h>

TPUT_DEFINE_BLOCK(L"algorithm", L"")
{
    TPUT_EXPECT(tp::algo::crc32("123456789", 9) == 0xCBF43926, NULL);
    TPUT_EXPECT(tp::algo::crc32("", 0) == 0, NULL);
    TPUT_EXPECT(tp::algo::base64_encode("sure.", 5) == "c3VyZS4=", NULL);
}
