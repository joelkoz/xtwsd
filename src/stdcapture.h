#ifndef _STDCAPTURE_H
#define _STDCAPTURE_H
/**
 * Capture output to standard out and standard err to a string.
 * Code found online at https://stackoverflow.com/questions/5419356/redirect-stdout-stderr-to-a-string
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string>

class StdCapture
{
public:
    StdCapture();
    ~StdCapture();


    /**
     * Start capturing std out and std err to s tring
     */
    void beginCapture();

    /**
     * End the capturing process, restoring it to its original values
     */
    bool endCapture();

    /**
     * Retrieve the captured output. BeginCapture() and EndCapture() must have been
     * called first.
     */
    const std::string getCapture();


    bool isCapturing() { return m_capturing; }

private:
    enum PIPES { READ, WRITE };
    int m_pipe[2];
    int m_oldStdOut;
    int m_oldStdErr;
    bool m_capturing;
    bool m_init;
    std::string m_captured;
};

#endif