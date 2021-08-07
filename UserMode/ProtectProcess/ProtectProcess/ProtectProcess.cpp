#include <conio.h>
#include "protectProcess.h"


int main()
{
    ProtectProcess::Start();

    while (true) Sleep(1);
    return 0;
}

