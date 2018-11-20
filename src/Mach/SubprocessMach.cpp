/**
 * @file SubprocessMach.cpp
 *
 * This module contains the Mach implementation of the
 * SystemAbstractions::Subprocess class.
 *
 * © 2018 by Richard Walters
 */

#include <inttypes.h>
#include <libproc.h>
#include <string>
#include <SystemAbstractions/Subprocess.hpp>
#include <sys/proc_info.h>
#include <unistd.h>
#include <vector>

namespace SystemAbstractions {

    void CloseAllFilesExcept(int keepOpen) {
        const auto pid = getpid();
        auto bufferSize = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, 0, 0);
        if (bufferSize < 0) {
            return;
        }
        std::vector< struct proc_fdinfo > fds(bufferSize / sizeof(struct proc_fdinfo));
        bufferSize = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, fds.data(), bufferSize);
        if (bufferSize < 0) {
            return;
        }
        fds.resize(bufferSize / sizeof(struct proc_fdinfo));
        for (const auto& fd: fds) {
            if (fd.proc_fd != keepOpen) {
                (void)close(fd.proc_fd);
            }
        }
    }

    auto Subprocess::GetProcessList() -> std::vector< ProcessInfo > {
        std::vector< ProcessInfo > processes;

        auto bufferSize = proc_listallpids(0, 0);
        if (bufferSize < 0) {
            return {};
        }
        std::vector< pid_t > pids(bufferSize);
        bufferSize = proc_listallpids(pids.data(), bufferSize * sizeof(pid_t));
        pids.resize(bufferSize);
        processes.reserve(bufferSize);
        for (auto pid: pids) {
            std::vector< char > nameChars(PROC_PIDPATHINFO_MAXSIZE);
            bufferSize = proc_pidpath(pid, nameChars.data(), nameChars.size());
            ProcessInfo process;
            process.id = (unsigned int)pid;
            process.image.assign(nameChars.begin(), nameChars.begin() + bufferSize);
            bufferSize = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, 0, 0);
            if (bufferSize < 0) {
                continue;
            }
            std::vector< struct proc_fdinfo > fds(bufferSize / sizeof(struct proc_fdinfo));
            bufferSize = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, fds.data(), bufferSize);
            if (bufferSize < 0) {
                continue;
            }
            fds.resize(bufferSize / sizeof(struct proc_fdinfo));
            for (const auto& fd: fds) {
                if (fd.proc_fdtype != PROX_FDTYPE_SOCKET) {
                    continue;
                }
                struct socket_fdinfo socketInfo;
                if (proc_pidfdinfo(pid, fd.proc_fd, PROC_PIDFDSOCKETINFO, &socketInfo, sizeof(socketInfo)) < 0) {
                    continue;
                }
                if (
                    (socketInfo.psi.soi_family == AF_INET)
                    && (socketInfo.psi.soi_kind == SOCKINFO_TCP)
                    && (socketInfo.psi.soi_proto.pri_tcp.tcpsi_ini.insi_fport == 0)
                ) {
                    (void)process.tcpServerPorts.insert(
                        (uint16_t)(
                            (uint16_t)((socketInfo.psi.soi_proto.pri_tcp.tcpsi_ini.insi_lport >> 8) & 0x00FF)
                            | (uint16_t)((socketInfo.psi.soi_proto.pri_tcp.tcpsi_ini.insi_lport << 8) & 0xFF00)
                        )
                    );
                }
            }
            processes.push_back(std::move(process));
        }
        return processes;
    }

}
