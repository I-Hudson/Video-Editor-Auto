#include "VideoCut.h"
#include "ConsoleCommands.h"

#include <functional>

bool run = true;

void exitApp()
{
	run = false;
}

void helpCommand()
{
	std::string helpMessage = R"(App help:
	--exit - Exit the application.
								)";

	std::cout << helpMessage.c_str() << "\n";
}

int main(int argc, char** argv)
{
	VideoCut c;

	ConsoleCommands::Get()->addCommand("--help", std::bind(&VideoCut::helpCommand, &c));
	ConsoleCommands::Get()->addCommand("--input", std::bind(&VideoCut::inputCommand, &c));
	ConsoleCommands::Get()->addCommand("--output", std::bind(&VideoCut::outputCommand, &c));
	ConsoleCommands::Get()->addCommand("--run", std::bind(&VideoCut::run, &c));
	ConsoleCommands::Get()->addCommand("--frameCompare", std::bind(&VideoCut::frameCompareCommand, &c));
	ConsoleCommands::Get()->addCommand("--frameRate", std::bind(&VideoCut::frameRateCommand, &c));
	ConsoleCommands::Get()->addCommand("--openOutput", std::bind(&VideoCut::openOutputFolderCommand, &c));
	ConsoleCommands::Get()->addCommand("--openOutput", std::bind(&VideoCut::helpCommand, &c));

	ConsoleCommands::Get()->addCommand("--help", std::bind(&helpCommand));
	ConsoleCommands::Get()->addCommand("--exit", std::bind(&exitApp));

	ConsoleCommands::Get()->triggerCommand("--help");
	while (run)
	{
		char in[256];
		std::cin >> in;
		std::string inString(in);
		ConsoleCommands::Get()->triggerCommand(inString);
	}

	system("pause");
	return 0;
}
