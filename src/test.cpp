#include "stdafx.h"

#include "comm.h"

std::ofstream __debug_file;

using namespace Common;

class CommandNotifier : public ICommandNotifier
{
public:
	CommandNotifier(CComm& comm)
		: _comm(comm)
	{}

private:
	virtual void OnCommand() override {
		while (Command* bpCmd = _comm.get_command()) {
			if (bpCmd->type == CommandType::kErrorMessage) {
				auto pCmd = static_cast<Command_ErrorMessage*>(bpCmd);
				fprintf(stderr, "错误：%s(%d).\n", pCmd->what.c_str(), pCmd->code);
				delete pCmd;
			}
			else if (bpCmd->type == CommandType::kUpdateCounter) {
                int nRead, nWritten, nQueued;
                _comm.get_counter(&nRead, &nWritten, &nQueued);
                fprintf(stdout, "状态：接收计数:%u,发送计数:%u,等待发送:%u", nRead, nWritten, nQueued);
				delete bpCmd;
			}
			else {
				delete bpCmd;
			}
		}
	}

private:
	CComm& _comm;
};

int main()
{
	CComm comm;
	CComm::s_setting_comm settings;
	CommandNotifier notifier(comm);

	comm.open(1);

	settings.baud_rate = CBR_9600;
	settings.databit = 8;
	settings.parity = NOPARITY;
	settings.stopbit = ONESTOPBIT;

	comm.setting_comm(&settings);

	comm.begin_threads();

	comm.set_notifier(&notifier);

	MSG msg;
	while (::GetMessage(&msg, nullptr, 0, 0)) {

	}

	return 0;
}
