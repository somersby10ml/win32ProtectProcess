//#pragma comment(linker, "/INTEGRITYCHECK")

#include "hook.h"

HANDLE protectID = (HANDLE)0;
PVOID hRegistration = NULL;

OB_PREOP_CALLBACK_STATUS PreCallback(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION pOperationInformation)
{
    UNREFERENCED_PARAMETER(RegistrationContext);
    UNREFERENCED_PARAMETER(pOperationInformation);

    /*
        POB_PRE_OPERATION_INFORMATION 에 있는 정보

        Operation
            OB_OPERATION_HANDLE_CREATE
                프로세스 또는 스레드에 대한 새 핸들이 열립니다. 사용 변수는 인자> CreateHandleInformation 만들 관련을 정보를 얻을 수 있습니다.

            OB_OPERATION_HANDLE_DUPLICATE
                프로세스 또는 스레드 핸들이 복제됩니다. 중복 정보에 대해서는 매개변수->DuplicateHandleInformation 을 사용 하십시오.

        KernelHandle
            핸들이 커널 핸들인지 여부를 지정하는 비트. 이 멤버가 TRUE 이면 핸들은 커널 핸들입니다. 그렇지 않으면 이 핸들은 커널 핸들이 아닙니다.

        Object
            핸들 작업의 대상인 프로세스 또는 스레드 개체에 대한 포인터입니다.

        ObjectType
            개체의 개체 유형에 대한 포인터입니다. 이 멤버는 PsProcessType 프로세스 나에 대한 PsThreadType 스레드에 대한.
        
        CallContext
            작업에 대한 드라이버별 컨텍스트 정보에 대한 포인터입니다. 
            기본적으로 필터 관리자는 이 멤버를 NULL 로 설정 하지만 ObjectPreCallback 루틴은 드라이버별 방식으로 CallContext 멤버를 재설정할 수 있습니다 . 
            필터 관리자는 이 값을 일치하는 ObjectPostCallback 루틴에 전달합니다.

        Parameters
            작업별 정보를 포함 하는 OB_PRE_OPERATION_PARAMETERS 공용체에 대한 포인터 입니다. 조작 부재는 부재의 조합이 유효한지를 결정.

            CreateHandleInformation
                열려 있는 핸들과 관련된 정보를 포함 하는 OB_PRE_CREATE_HANDLE_INFORMATION 구조입니다.
                    OB_PRE_CREATE_HANDLE_INFORMATION
                    ACCESS_MASK DesiredAccess; <-- 조작가능
                    ACCESS_MASK OriginalDesiredAccess;

            DuplicateHandleInformation
                복제 중인 핸들과 관련된 정보를 포함 하는 OB_PRE_DUPLICATE_HANDLE_INFORMATION 구조입니다.
    */

    //pOperationInformation->ObjectType    PreInfo->ObjectType == *PsProcessType
    
    HANDLE pid = PsGetProcessId((PEPROCESS)pOperationInformation->Object);
    if (protectID && pid == protectID) {


       /* if ((pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROCESS_TERMINATE) == PROCESS_TERMINATE)
        {
            pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;
        }*/

        pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
    }


    //DbgPrintEx(DPFLTR_ACPI_ID, DPFLTR_TRACE_LEVEL, "[+] Pre Callback Routine");
    return OB_PREOP_SUCCESS;
}

void PostCallback(PVOID RegistrationContext, POB_POST_OPERATION_INFORMATION pOperationInformation)
{
    UNREFERENCED_PARAMETER(RegistrationContext);
    UNREFERENCED_PARAMETER(pOperationInformation);
    //DbgPrintEx(DPFLTR_ACPI_ID, DPFLTR_TRACE_LEVEL, "[+] Post Callback Routine\n");
}

void testHook() {
	// DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "testHook Start\n");



    OB_OPERATION_REGISTRATION CBOperationRegistrations[1] = { { 0 } };
    /*
        PsProcessType : 프로세스 핸들 동작을 위한 유형
        PsThreadType : 스레드 핸들 동작을 위한 유형
        ExDesktopObjectType : 데스크톱 핸들 동작을 위한 유형
    */
    CBOperationRegistrations[0].ObjectType = PsProcessType;

    /*
        OB_OPERATION_HANDLE_CREATE : 새로운 핸들(ObjectType에 따른)이 생성되거나 열었을 경우 동작
        OB_OPERATION_HANDLE_DUPLICATE : 새로운 핸들을 복제하거나 복제된 경우 동작
    */
    CBOperationRegistrations[0].Operations |= OB_OPERATION_HANDLE_CREATE;
    CBOperationRegistrations[0].Operations |= OB_OPERATION_HANDLE_DUPLICATE;

    /*
        OB_PRE_OPERATION_CALLBACK의 포인터, 요청된 작업이 발생하기 전에 해당 루틴을 호출
    */
    CBOperationRegistrations[0].PreOperation = PreCallback;

    /*
        PostOperation : OB_POST_OPERATION_CALLBACK의 포인터, 요청된 작업이 발생한 후에 해당 루틴을 호출
    */
    //CBOperationRegistrations[0].PostOperation = PostCallback;


    OB_CALLBACK_REGISTRATION  CBObRegistration = { 0 };
    {
        CBObRegistration.Version = OB_FLT_REGISTRATION_VERSION;
        // OperationRegistration  배열의 수
        CBObRegistration.OperationRegistrationCount = 1;

        // 드라이버의 고도(유니코드), MS에 등록되어 사용되며 로드 순서와도 관계가 있음
        UNICODE_STRING CBAltitude = { 0 };
        RtlInitUnicodeString(&CBAltitude, L"1000");
        CBObRegistration.Altitude = CBAltitude;

        // 콜백 루틴이 실행될 때 해당 값을 콜백 루틴으로 전달
        CBObRegistration.RegistrationContext = NULL;

        // OB_OPERATION_REGISTRATION의 포인터, ObjectPre, PostCallback 루틴이 호출되는 유형을 지정
        CBObRegistration.OperationRegistration = CBOperationRegistrations;
    }

    //PVOID pCBRegistrationHandle = NULL;
    NTSTATUS Status = ObRegisterCallbacks(
        &CBObRegistration,
        &hRegistration       // save the registration handle to remove callbacks later
    );

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "ObRegisterCallbacks : %08X\n", Status);
    //DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "hook End\n");
}

void unhook() {
    if (hRegistration) {
        ObUnRegisterCallbacks(hRegistration);
    }
    //DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "unhook\n");
}

void setProcessId(unsigned int pid) {
    protectID = (HANDLE)pid;
}