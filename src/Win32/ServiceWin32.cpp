/**
 * @file ServiceWin32.cpp
 *
 * This module contains the Windows implementation of the
 * SystemAbstractions::Service class.
 *
 * © 2018 by Richard Walters
 */

/**
 * Windows.h should always be included first because other Windows header
 * files, such as KnownFolders.h, don't always define things properly if
 * you don't include Windows.h first.
 */
#include <Windows.h>

#include <stdlib.h>
#include <string>
#include <string.h>
#include <SystemAbstractions/Service.hpp>

namespace {

    /**
     * Before StartServiceCtrlDispatcherA is called, a pointer to the
     * Service instance is stored here so that it can be recovered in
     * the ServiceMain function.
     */
    SystemAbstractions::Service* instance = nullptr;

}

namespace SystemAbstractions {

    /**
     * This structure contains the private methods and properties of
     * the Service class.
     */
    struct Service::Impl {
        SERVICE_STATUS_HANDLE serviceStatusHandle;

        /**
         * This holds the current status of the service as told to the
         * Windows Service Control Manager.
         */
        SERVICE_STATUS serviceStatus;

        /**
         * This method is called to run the service to its completion, starting
         * from outside the service control dispatcher.  It enters the
         * service control dispatcher, with a dispatch table having a single
         * entry for the service.  This doesn't return until the service is
         * stopped.
         *
         * @return
         *     The exit code that should be returned from the main function
         *     of the program is returned.
         */
        int Run() {
            auto name = instance->GetServiceName();
            SERVICE_TABLE_ENTRYA dispatchTable[] = {
                {(LPSTR)name.data(), ServiceMain},
                {NULL, NULL}
            };
            if (StartServiceCtrlDispatcherA(dispatchTable) == 0) {
                return EXIT_SUCCESS;
            } else {
                return EXIT_FAILURE;
            }
        }

        /**
         * This function is called by the Windows Service Control Manager as
         * the main entrypoint to the service.
         *
         * @param[in] dwArgc
         *     This is a count of the number of arguments given to the service.
         *
         * @param[in] lpszArgv
         *     This points to an array of pointers to the arguments given
         *     to the service.
         */
        static VOID WINAPI ServiceMain(DWORD dwArgc, LPSTR* lpszArgv) {
            instance->impl_->Main();
        }

        /**
         * This method is called from the service entrypoint to run the service
         * to completion.  This happens within the context of the service
         * control manager.  The service control handler is registered, and all
         * service status is communicated through the handle provided by the
         * registration.
         *
         * This method does not return until the service has stopped.
         */
        void Main() {
            serviceStatusHandle = RegisterServiceCtrlHandlerExA(
                instance->GetServiceName().c_str(),
                ServiceControlHandler,
                NULL
            );
            serviceStatus.dwCurrentState = SERVICE_RUNNING;
            ReportServiceStatus();
            const auto runResult = instance->Run();
            if (runResult == 0) {
                serviceStatus.dwWin32ExitCode = NO_ERROR;
            } else {
                serviceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
                serviceStatus.dwServiceSpecificExitCode = (DWORD)runResult;
            }
            serviceStatus.dwCurrentState = SERVICE_STOPPED;
            ReportServiceStatus();
        }

        /**
         * This function is called by the Windows Service Control Manager in
         * order to give it a control command, such as to start, pause, resume,
         * or stop the service.
         *
         * @param[in] dwControl
         *     This indicates what control command has been given.
         *
         * @param[in] dwEventType
         *     This holds additional information for some control command
         *     types, and the meaning depends on the control command type
         *     given.
         *
         * @param[in] lpEventData
         *     This points to additional information for some control command
         *     types, and the meaning depends on the control command type
         *     given.
         *
         * @param[in] lpContext
         *     This is the same context pointer given to
         *     RegisterServiceCtrlHandlerExA when this handler was registered.
         *     It should point back to the service implementation structure.
         *
         * @return
         *     A value whose meaning depends on what control command was given
         *     and the result of applying the control command is returned.
         */
        static DWORD WINAPI ServiceControlHandler(
            DWORD dwControl,
            DWORD dwEventType,
            LPVOID lpEventData,
            LPVOID lpContext
        ) {
            switch (dwControl) {
                case SERVICE_CONTROL_STOP: {
                    instance->Stop();
                } return NO_ERROR;

                case SERVICE_CONTROL_INTERROGATE: {
                } return NO_ERROR;

                default: {
                } return ERROR_CALL_NOT_IMPLEMENTED;
            }
        }

        /**
         * This method sends the current service status to the service control
         * manager through the handle provided when the service was registered.
         */
        void ReportServiceStatus() {
            (void)SetServiceStatus(
                serviceStatusHandle,
                &serviceStatus
            );
        }
    };

    Service::~Service() noexcept = default;
    Service::Service(Service&&) noexcept = default;
    Service& Service::operator=(Service&&) noexcept = default;

    Service::Service()
        : impl_(new Impl())
    {
        (void)memset(&impl_->serviceStatus, 0, sizeof(impl_->serviceStatus));
        impl_->serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        impl_->serviceStatus.dwCurrentState = SERVICE_STOPPED;
        impl_->serviceStatus.dwControlsAccepted = (
            SERVICE_ACCEPT_STOP
        );
        impl_->serviceStatus.dwWin32ExitCode = NO_ERROR;
    }

    int Service::Start() {
        instance = this;
        return impl_->Run();
    }

}
