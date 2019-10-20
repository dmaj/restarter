#ifndef __RESTARTER_HPP__
#define __RESTARTER_HPP__

#include <string>
#include <unistd.h>
#include <iostream>
#include <cstdarg>

using namespace std;

struct ParamTyp
{
    int hour;
    int min;
    int grace;
    string executable;
    string param;
};


string str_pf (string, ...);






#endif //__RESTARTER_HPP__
