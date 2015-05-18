#ifndef __XRunObject_Adler_h
#define __XRunObject_Adler_h
#include "stdio.h"
#include <iostream>
//typename unsigned long uLong;
typedef unsigned char  Byte;  /* 8 bits */
typedef Byte Bytef;
typedef unsigned int   uInt;  /* 16 bits or more */
typedef unsigned long  uLong; /* 32 bits or more */
//local uLong adler32_combine_ OF((uLong adler1, uLong adler2, z_off64_t len2));



class Adler
{
public:
	static uLong adler32( uLong adler,const Bytef *buf, uInt len);
};


#endif