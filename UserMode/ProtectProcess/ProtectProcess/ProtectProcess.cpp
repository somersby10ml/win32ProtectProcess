#include <conio.h>
#include "protectProcess.h"

int main()
{
	// dll injection defense
	PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY policy = { 0, };
	policy.MicrosoftSignedOnly = 1;
	BOOL bR = ::SetProcessMitigationPolicy(ProcessSignaturePolicy, &policy, sizeof(policy));
	int err = ::GetLastError();

	ProtectProcess::Start();

	while (true) Sleep(1);
	return 0;
}