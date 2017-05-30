// Adapted from http://ideone.com/mt1MLS (found via stackoverflow).

#include <ctype.h>
#include "hexdump.h"
#include <iostream>
#include <iomanip>
 
using namespace std;
 
int
hexdump(const char* bytes, size_t num_bytes, std::ostream& cout)
{
    ios::fmtflags cout_flags( cout.flags() );
    unsigned long address = 0;

    cout << hex << setfill('0');
    const char *cp = bytes;
    for (int nleft = num_bytes; nleft > 0; ) {
        int nread;
        char buf[16];
 
        for ( nread = 0; nread < 16 && nleft-- > 0; ) {
            buf[nread++] = *cp++;
        }
        if( nread == 0 ) break;
 
        // Show the address
        cout << setw(8) << address;
 
        // Show the hex codes
        for( int i = 0; i < 16; i++ ) {
            if( i % 8 == 0 ) cout << ' ';
            if( i < nread )
                cout << ' ' << setw(2) << (0xffu&(unsigned)buf[i]);
            else 
                cout << "   ";
        }
 
        cout << "  ";
        for( int i = 0; i < nread; i++)
        {
            if( !isprint(buf[i]))
                cout << '.';
            else
                cout << buf[i];
        }
 
        cout << "\n";
        address += 16;
    }
    cout.flags( cout_flags );
    return 0;
}
