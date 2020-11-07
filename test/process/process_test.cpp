/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * process_test.cpp - Process test
 */

#include <iostream>
#include <unistd.h>
#include <vector>

#include "libcamera/internal/event_dispatcher.h"
#include "libcamera/internal/process.h"
#include "libcamera/internal/thread.h"
#include "libcamera/internal/timer.h"
#include "libcamera/internal/utils.h"

#include "test.h"

using namespace std;
using namespace libcamera;

class ProcessTestChild
{
public:
	int run(int status)
	{
		usleep(50000);

		return status;
	}
};

class ProcessTest : public Test
{
public:
	ProcessTest()
		: exitStatus_(Process::NotExited), exitCode_(-1)
	{
	}

protected:
	int run()
	{
		EventDispatcher *dispatcher = Thread::current()->eventDispatcher();
		Timer timeout;

		int exitCode = 42;
		vector<std::string> args;
		args.push_back(to_string(exitCode));
		proc_.finished.connect(this, &ProcessTest::procFinished);

		/* Test that kill() on an unstarted process is safe. */
		proc_.kill();

		/* Test starting the process and retrieving the exit code. */
		int ret = proc_.start("/proc/self/exe", args);
		if (ret) {
			cerr << "failed to start process" << endl;
			return TestFail;
		}

		timeout.start(2000);
		while (timeout.isRunning() && exitStatus_ == Process::NotExited)
			dispatcher->processEvents();

		if (exitStatus_ != Process::NormalExit) {
			cerr << "process did not exit normally" << endl;
			return TestFail;
		}

		if (exitCode != exitCode_) {
			cerr << "exit code should be " << exitCode
			     << ", actual is " << exitCode_ << endl;
			return TestFail;
		}

		return TestPass;
	}

private:
	void procFinished([[maybe_unused]] Process *proc,
			  enum Process::ExitStatus exitStatus, int exitCode)
	{
		exitStatus_ = exitStatus;
		exitCode_ = exitCode;
	}

	ProcessManager processManager_;

	Process proc_;
	enum Process::ExitStatus exitStatus_;
	int exitCode_;
};

/*
 * Can't use TEST_REGISTER() as single binary needs to act as both
 * parent and child processes.
 */
int main(int argc, char **argv)
{
	if (argc == 2) {
		int status = std::stoi(argv[1]);
		ProcessTestChild child;
		return child.run(status);
	}

	return ProcessTest().execute();
}
