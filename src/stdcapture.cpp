// Found this code online at: https://stackoverflow.com/questions/5419356/redirect-stdout-stderr-to-a-string
// Modifications needed to compile under OS X.
#include "stdcapture.h"


#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string>


#ifdef __MACH__
int pipe2(int pipefd[2], int flags) {
    int result = pipe(pipefd);
    if (result != -1) {
        int flags2 = fcntl(pipefd[0], F_GETFD);
        flags2 |= flags;
        if (fcntl(pipefd[0], F_SETFL, flags2))
            perror("fcntl");    
    }
    return result;
}
#endif

    StdCapture::StdCapture(): m_capturing(false), m_init(false), m_oldStdOut(0), m_oldStdErr(0)
    {
        m_pipe[READ] = 0;
        m_pipe[WRITE] = 0;
        if (pipe2(m_pipe, O_NONBLOCK) == -1)
            return;

        m_oldStdOut = dup(fileno(stdout));
        m_oldStdErr = dup(fileno(stderr));
        if (m_oldStdOut == -1 || m_oldStdErr == -1)
            return;

        m_init = true;
    }

    StdCapture::~StdCapture()
    {
        if (m_capturing)
        {
            endCapture();
        }
        if (m_oldStdOut > 0)
            close(m_oldStdOut);
        if (m_oldStdErr > 0)
            close(m_oldStdErr);
        if (m_pipe[READ] > 0)
            close(m_pipe[READ]);
        if (m_pipe[WRITE] > 0)
            close(m_pipe[WRITE]);
    }


    void StdCapture::beginCapture()
    {
        if (!m_init)
            return;
        if (m_capturing)
            endCapture();
        fflush(stdout);
        fflush(stderr);
        dup2(m_pipe[WRITE], fileno(stdout));
        dup2(m_pipe[WRITE], fileno(stderr));
        m_capturing = true;
    }

    bool StdCapture::endCapture()
    {
        if (!m_init)
            return false;
        if (!m_capturing)
            return false;
        fflush(stdout);
        fflush(stderr);
        dup2(m_oldStdOut, fileno(stdout));
        dup2(m_oldStdErr, fileno(stderr));
        m_captured.clear();

        char buff[1025];
        for(;;)
        {
            int bytesRead = read(m_pipe[READ], buff, 1024);

            if (bytesRead <= 0) {
                break;
            }
            buff[bytesRead] = 0;
            m_captured += buff;
        }
        return true;
    }

    const std::string StdCapture::getCapture()
    {
        endCapture();
        return m_captured;
    }
