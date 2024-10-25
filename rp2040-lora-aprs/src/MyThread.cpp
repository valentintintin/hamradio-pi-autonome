#include "MyThread.h"
#include "ArduinoLog.h"

MyThread::MyThread(System *system, unsigned long interval, const char *name) : system(system) {
    setInterval(interval);
    ThreadName = name;
}

bool MyThread::begin() {
    Log.infoln(F("[%s] Begin"), ThreadName.c_str());

    initiated = init();

    if (initiated) {
        lastUpdateHasError = false;
        Log.noticeln(F("[%s] Begin OK"), ThreadName.c_str());
    } else {
        lastUpdateHasError = true;
        Log.errorln(F("[%s] Begin KO"), ThreadName.c_str());
    }

    return initiated;
}

void MyThread::run() {
    Log.traceln(F("[%s] Run"), ThreadName.c_str());

    if (!initiated && !begin()) {
        runned();
        return;
    }

    if (runOnce()) {
        Log.noticeln(F("[%s] Run OK"), ThreadName.c_str());
        lastUpdateHasError = false;
    } else {
        Log.warningln(F("[%s] Run KO"), ThreadName.c_str());
        lastUpdateHasError = true;
    }

    runned();
}