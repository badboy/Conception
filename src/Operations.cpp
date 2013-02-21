#include "Main.h"

std::string Diff(const std::string & Content1, const std::string & Content2)
{
	WriteToFile("./GenDiff1.txt", Content1);
	WriteToFile("./GenDiff2.txt", Content2);

	std::string Output = "";
	{
		int PipeFd[2];			// Pipe for reading from child's stdout+stderr
		pipe(PipeFd);
		fcntl(PipeFd[0], F_SETFL, O_NONBLOCK);

		uint8 ProcessResult;

		{
			auto Pid = fork();

			if (0 == Pid)
			{
				close(PipeFd[0]);    // close reading end in the child

				dup2(PipeFd[1], 1);  // send stdout to the pipe
				dup2(PipeFd[1], 2);  // send stderr to the pipe

				close(PipeFd[1]);    // this descriptor is no longer needed

				execl("/usr/bin/diff", "/usr/bin/diff", /*"-u",*/ "./GenDiff1.txt", "./GenDiff2.txt", (char *)0);

				// TODO: Add error checking on above execl(), and do exit() in case execution reaches here
				//exit(1);		// Not needed, just in case I comment out the above
				throw 0;
			}
			else if (-1 == Pid)
			{
				std::cerr << "Error forking.\n";
				throw 0;
			}
			else
			{
				// Wait for child process to complete
				{
					int status;
					waitpid(Pid, &status, 0);
					Pid = 0;

					std::cout << "Child finished with status " << status << ".\n";

					ProcessResult = static_cast<uint8>(status >> 8);
				}

				// Read output from pipe and put it into Output
				//if (0 == ProcessResult)
				{
					char buffer[1024];
					ssize_t n;
					while (0 != (n = read(PipeFd[0], buffer, sizeof(buffer))))
					{
						if (-1 == n) {
							if (EAGAIN == errno) {
								break;
							} else {
								std::cerr << "Error: Reading from pipe " << PipeFd[0] << " failed with errno " << errno << ".\n";
								break;
							}
						}
						else
						{
							Output.append(buffer, n);
						}
					}
				}
			}
		}

		close(PipeFd[0]);
		close(PipeFd[1]);
	}

	return Output;
}

void WriteToFile(const std::string & Path, const std::string & Content)
{
	//std::cout << "Writing the following to '" << Path << "':\n" << Content << endl;
	std::ofstream OutFile(Path);
	OutFile << Content;
}

void PlayBeep()
{
	//std::cout << "Beep.\n";
	//BeepWidget->m_ExecuteWidget->GetAction()();
	LaunchProcessInBackground({"/usr/bin/afplay", "--volume", "0.5", "data/Cannot Distribute/hitsound.wav"});		// HACK: OS X dependency
}

// Trim last newline, if there is one
void TrimLastNewline(std::string & InOut)
{
	if (   InOut.size() >= 1
		&& InOut.back() == '\n')
	{
		InOut.pop_back();
	}
}

void Gofmt(std::string & InOut)
{
	std::string Output = "";

	int PipeFd[2];			// Pipe for reading from child's stdout+stderr
	int PipeInFd[2];		// Pipe for writing to child process stdin
	pipe(PipeFd);
	pipe(PipeInFd);
	fcntl(PipeFd[0], F_SETFL, O_NONBLOCK);

	uint8 ProcessResult;

	{
		auto Pid = fork();

		if (0 == Pid)
		{
			close(PipeFd[0]);    // close reading end in the child

			dup2(PipeFd[1], 1);  // send stdout to the pipe
			dup2(PipeFd[1], 2);  // send stderr to the pipe

			close(PipeFd[1]);    // this descriptor is no longer needed

			close(PipeInFd[1]);    // close writing end in the child
			dup2(PipeInFd[0], 0);  // get stdin from the pipe
			close(PipeInFd[0]);    // this descriptor is no longer needed

			execl("/usr/local/go/bin/gofmt", "/usr/local/go/bin/gofmt", (char *)0);
			//execl("/bin/cat", "/bin/cat", (char *)0);

			// TODO: Add error checking on above execl(), and do exit() in case execution reaches here
			//exit(1);		// Not needed, just in case I comment out the above
			throw 0;
		}
		else if (-1 == Pid)
		{
			std::cerr << "Error forking.\n";
			throw 0;
		}
		else
		{
			// Write to child's stdin and end it
			// TODO: Error check the write, perhaps need multiple tries to fully flush it
			write(PipeInFd[1], InOut.c_str(), InOut.length());
			close(PipeInFd[1]);

			// Wait for child process to complete
			{
				int status;
				waitpid(Pid, &status, 0);
				Pid = 0;

				std::cout << "Child finished with status " << status << ".\n";

				ProcessResult = static_cast<uint8>(status >> 8);
			}

			// Read output from pipe and put it into Output
			//if (0 == ProcessResult)
			{
				char buffer[1024];
				ssize_t n;
				while (0 != (n = read(PipeFd[0], buffer, sizeof(buffer))))
				{
					if (-1 == n) {
						if (EAGAIN == errno) {
							break;
						} else {
							std::cerr << "Error: Reading from pipe " << PipeFd[0] << " failed with errno " << errno << ".\n";
							break;
						}
					}
					else
					{
						Output.append(buffer, n);
					}
				}
			}
		}
	}

	close(PipeFd[0]);
	close(PipeFd[1]);
	close(PipeInFd[0]);

	InOut = Output;
}

std::vector<std::string> Ls(std::string & InOut)
{
	std::vector<std::string> Entries;

	std::string Output = "";
	{
		int PipeFd[2];			// Pipe for reading from child's stdout+stderr
		pipe(PipeFd);
		fcntl(PipeFd[0], F_SETFL, O_NONBLOCK);

		uint8 ProcessResult;

		{
			auto Pid = fork();

			if (0 == Pid)
			{
				close(PipeFd[0]);    // close reading end in the child

				dup2(PipeFd[1], 1);  // send stdout to the pipe
				dup2(PipeFd[1], 2);  // send stderr to the pipe

				close(PipeFd[1]);    // this descriptor is no longer needed

				// TODO: Option -p misses symbolic link folders, so change to using -F option and do some parsing, or something
				if (InOut.empty())
					execl("/bin/ls", "/bin/ls", "-p", (char *)0);
				else
					execl("/bin/ls", "/bin/ls", "-p", InOut.c_str(), (char *)0);

				// TODO: Add error checking on above execl(), and do exit() in case execution reaches here
				//exit(1);		// Not needed, just in case I comment out the above
				throw 0;
			}
			else if (-1 == Pid)
			{
				std::cerr << "Error forking.\n";
				throw 0;
			}
			else
			{
				// Wait for child process to complete
				{
					int status;
					waitpid(Pid, &status, 0);
					Pid = 0;

					std::cout << "Child finished with status " << status << ".\n";

					ProcessResult = static_cast<uint8>(status >> 8);
				}

				// Read output from pipe and put it into Output
				//if (0 == ProcessResult)
				{
					char buffer[1024];
					ssize_t n;
					while (0 != (n = read(PipeFd[0], buffer, sizeof(buffer))))
					{
						if (-1 == n) {
							if (EAGAIN == errno) {
								break;
							} else {
								std::cerr << "Error: Reading from pipe " << PipeFd[0] << " failed with errno " << errno << ".\n";
								break;
							}
						}
						else
						{
							Output.append(buffer, n);
						}
					}
				}
			}
		}

		close(PipeFd[0]);
		close(PipeFd[1]);
	}

	// Parse Output and populate Entries
	// TODO: Clean up
	{
		std::stringstream ss;
		ss << Output;
		std::string Line;

		std::getline(ss, Line);
		while (!Line.empty() && !ss.eof())
		{
			Entries.push_back(Line);
			std::getline(ss, Line);
		}
		if (!Line.empty())
			Entries.push_back(Line);
	}

	return Entries;
}

void LaunchProcessInBackground(std::initializer_list<std::string> Argv)
{
	auto Pid = fork();

	if (0 == Pid)
	{
		std::vector<char *> argv;
		for (auto & arg : Argv)
		{
			argv.push_back(const_cast<char *>(arg.c_str()));
		}
		argv.push_back(nullptr);

		execv(argv[0], &argv[0]);

		// TODO: Add error checking on above execl(), and do exit() in case execution reaches here
		//exit(1);		// Not needed, just in case I comment out the above
		throw 0;
	}
	else if (-1 == Pid)
	{
		std::cerr << "Error forking.\n";
		throw 0;
	}
	else
	{
	}
}
