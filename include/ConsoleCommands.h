#pragma once

#include <vector>
#include <map>
#include <functional>
#include <memory>

class ConsoleCommands
{
public:
	ConsoleCommands();
	~ConsoleCommands();

	/*
	 Add a new function toa  command
	*/
	void addCommand(std::string a_command, std::function<void()> a_fn);

	/*
	 Trigger all functions associated to the command
	*/
	void triggerCommand(std::string a_command);

	/*
	 Return the input from the console 
	*/
	std::string getInput();

	/*
	 Return the ConsoleCommand instance
	*/
	inline static ConsoleCommands* Get()
	{ 
		return m_instance.get();
	}

private:
	/*
	 Store all the commands and functions
	*/
	std::map<std::string, std::vector<std::function<void()>>> m_commands;

	/*
	 ConsoleCommand instance
	*/
	static std::unique_ptr<ConsoleCommands> m_instance;
};
