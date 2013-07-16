#pragma once

#include <string>

// todo: replace __int32

namespace tp
{
    struct algo
    {
        static unsigned __int32 crc32(const void * buf, size_t len)
        {
            return crc32_update(0xFFFFFFFF, buf, len) ^ 0xFFFFFFFF;
        }
        static void crc32_make_table(unsigned __int32 (&ct)[256], unsigned __int32 polynom)
        {
            for (unsigned __int32 n = 0; n < 256; n++)
            {
                unsigned __int32 c = n;
                for (size_t k = 0; k < 8; k++)
                {
                    if (c & 1)
                        c = polynom ^ (c >> 1);
                    else
                        c = c >> 1;
                }
                ct[n] = c;
            }
        }
        static unsigned __int32 crc32_update(unsigned __int32 old_crc, const void * buf, size_t len)
        {
            static bool crc_table_computed = false;
            static unsigned __int32 crc_table[256];

            if (!crc_table_computed)
            {
                crc32_make_table(crc_table, 0xedb88320);
                crc_table_computed = true;
            }

            const unsigned char * byte_buf = static_cast<const unsigned char*>(buf);
            for (size_t i = 0; i < len; i++)
            {
                old_crc = crc_table[(old_crc ^ byte_buf[i]) & 0xff] ^ (old_crc >> 8);
            }

            return old_crc;
        }

        static std::string  base64_encode(const void * buf, size_t len)
        {
            const char * cvt_tbl =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
            const unsigned char *s = static_cast<const unsigned char *>(buf);
            const unsigned char *sEnd = s + len;
            const unsigned char *p = s;
            std::string ret;
            for (p = s; p + 3 <= sEnd; p += 3)
            {
                ret += cvt_tbl[p[0] >> 2];
                ret += cvt_tbl[((p[0] & 0x03) << 4) | (p[1] >> 4)];
                ret += cvt_tbl[((p[1] & 0x0F) << 2) | (p[2] >> 6)];
                ret += cvt_tbl[p[2] & 0x3F];
            }
            if (len % 3 == 1)
            {
                ret += cvt_tbl[p[0] >> 2];
                ret += cvt_tbl[((p[0] & 0x03) << 4)];
                ret.append("==");
            }
            else if (len % 3 == 2)
            {
                ret += cvt_tbl[p[0] >> 2];
                ret += cvt_tbl[((p[0] & 0x03) << 4) | (p[1] >> 4)];
                ret += cvt_tbl[((p[1] & 0x0F) << 2)];
                ret.append("=");
            }
            return ret;
        }

        static std::wstring base64_encodew(const void * buf, size_t len)
        {
            std::string str_a = base64_encode(buf, len);
            std::wstring ret(str_a.begin(), str_a.end());
            return ret;
        }

        static std::string base64_decode(const char * code, size_t len)
        {
            const char * rcvt_tbl =
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3e\x00\x00\x00\x3f"
                "\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e"
                "\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x00\x00\x00\x00\x00"
                "\x00\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25\x26\x27\x28"
                "\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

            const char *code_end = code + len;
            std::string str;

            for (const char *p = code; p < code_end; p += 4)
            {
                str += (rcvt_tbl[p[0]] << 2) | (rcvt_tbl[p[1]] >> 4);
                str += ((rcvt_tbl[p[1]] & 0x0F) << 4) | (rcvt_tbl[p[2]] >> 2);
                str += ((rcvt_tbl[p[2]] & 0x03) << 6) | rcvt_tbl[p[3]];
            }
            if (str.size() > 0 && len > 0)
            {
                if (code[len - 2] == L'=') str.resize(str.size() - 2);
                else if (code[len - 1] == L'=') str.resize(str.size() - 1);
            }

            return str;
        }

        static std::string base64_decode(const wchar_t * code, size_t len)
        {
            std::string code_a(len, '\0');
            for (size_t i = 0; i < len; i++)
                code_a[i] = static_cast<char>(code[i]);
            return base64_decode(code_a.c_str(), len);
        }

        static std::string base64_decode(const char * code)
        {
            return base64_decode(code, strlen(code));
        }

        static std::string base64_decode(const wchar_t * code)
        {
            return base64_decode(code, wcslen(code));
        }
    };
}