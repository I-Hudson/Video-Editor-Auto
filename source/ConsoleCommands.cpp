#include "ConsoleCommands.h"

#include <iostream>

std::unique_ptr<ConsoleCommands> ConsoleCommands::m_instance = std::make_unique<ConsoleCommands>();

ConsoleCommands::ConsoleCommands()
{
}

ConsoleCommands::~ConsoleCommands()
{
}

/*
 Add a new function to a command
*/
void ConsoleCommands::addCommand(std::string a_command, std::function<void()> a_fn)
{
	m_commands[a_command].push_back(a_fn);
}

/*
 Trigger all functions associated to the command
*/
void ConsoleCommands::triggerCommand(std::string a_command)
{
	std::map<std::string, std::vector<std::function<void()>>>::iterator it = m_commands.find(a_command);
	if (it != m_commands.end())
	{
		for (int i = 0; i < it->second.size(); i++)
		{
			it->second[i]();
		}
	}
	else
	{
		std::cout << "Unknow command. Use --help to show all commands. \n";
	}
}

/*
 Return the input from the console
*/
std::string ConsoleCommands::getInput()
{
	char in[256];
	std::cin >> in;
	return std::string(in);
}
