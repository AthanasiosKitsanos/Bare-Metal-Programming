#pragma once

#ifdef __x86_64__
    #define REGPARM(n)
#else
    #define REGPARM(n) gnu::regparm(n)
#endif
