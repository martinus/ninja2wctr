#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

// compile with e.g.
// g++ -O2 -Wall -std=c++17 ninja2wctr.cpp -o ninja2wctr
//
// usage: move into build directory that contains .ninja_log
// run 'ninjalog2wctr'

using namespace std::literals;

struct StartStop {
	std::chrono::milliseconds start = 0ms;
	std::chrono::milliseconds stop = 0ms;
};

// parse line by line, which looks like this:
// 10      3908    1601299613944115493     CMakeFiles/lgr.dir/src/cpp/unittests/nanobench.cpp.o    5ff3f2b631310730
//
// First collect all data into a map, to make sure each output is only there exactly once. Ninja has the tendency to just add
// to the log and only cleans up duplicates from time to time. We only use the latest entry for an output.
auto parseNinjaLog() -> std::unordered_map<std::string, StartStop> {
	static constexpr auto expectedFirstLine = "# ninja log v5";

	std::ifstream fin(".ninja_log");
	if (!fin.is_open()) {
		throw std::runtime_error("could not open .ninja_log");
	}
	std::string line;
	std::getline(fin, line);
	if (line != expectedFirstLine) {
		throw std::runtime_error("expected first line '" + std::string(expectedFirstLine) + "' but got '" + line + "'");
	}

	int64_t startMs = 0;
	int64_t stopMs = 0;
	int64_t restatMtime = 0;
	std::string output;
	std::string hash;

	std::unordered_map<std::string, StartStop> outputToStartStop;
	while (fin >> startMs >> stopMs >> restatMtime >> output >> hash) {
		outputToStartStop[output] = StartStop{.start = std::chrono::milliseconds(startMs), .stop = std::chrono::milliseconds(stopMs)};
	}

	return outputToStartStop;
}

struct Event {
	std::chrono::milliseconds time = 0ms;
	std::string output{};
	bool isStart = false;
};

// Creates a vector of all events (start & stop are one event each), sorted by time.
auto createdSortedEvents(std::unordered_map<std::string, StartStop> const& outputToStartStop) -> std::vector<Event> {
	std::vector<Event> events;
	for (auto const& outputAndStartStop : outputToStartStop) {
		events.push_back(Event{.time = outputAndStartStop.second.start, .output = outputAndStartStop.first, .isStart = true});
		events.push_back(Event{.time = outputAndStartStop.second.stop, .output = outputAndStartStop.first, .isStart = false});
	}

	std::sort(events.begin(), events.end(), [](Event const& a, Event const& b) { return a.time < b.time; });
	return events;
}

struct WallClockTimeResponsibility {
	std::chrono::duration<double> time = 0s;
	std::string output;
};

// Iterates the sorted events and for each time point builds a map of all currently active tasks. The time difference from the previous
// event's time divided by the number of currently active tasks is then added as WCTR to all currently active tasks.
//
// When a task has finished, it is added to the finishedTasks which is then sorted by WCTR.
auto processEventsToWallClockTimeResponsibilities(std::vector<Event> const& sortedEvents) -> std::vector<WallClockTimeResponsibility> {
	std::unordered_map<std::string, std::chrono::duration<double>> activeTasks;
	std::vector<WallClockTimeResponsibility> finishedTasks;
	auto previousTime = 0ms;
	for (auto const& e : sortedEvents) {
		// Add WCTR of time difference to all currently active tasks.
		for (auto& outputAndDuration : activeTasks) {
			outputAndDuration.second += std::chrono::duration<double>(e.time - previousTime) / activeTasks.size();
		}

		if (e.isStart) {
			// output processing has just started, add it to the currently active tasks
			activeTasks.emplace(e.output, 0s);
		} else {
			// output processing has finished! remove it from active tasks and put it into finished tasks.
			auto it = activeTasks.find(e.output);
			if (it == activeTasks.end()) {
				throw std::runtime_error("task '" + e.output + "' not found! this shouldn't happen.");
			}
			finishedTasks.push_back(WallClockTimeResponsibility{.time = it->second, .output = e.output});
			activeTasks.erase(it);
		}

		previousTime = e.time;
	}

	// done calculating, now sort and show the list
	std::sort(finishedTasks.begin(), finishedTasks.end(), [](WallClockTimeResponsibility const& a, WallClockTimeResponsibility const& b) {
		return a.time > b.time;
	});

	return finishedTasks;
}

// Prints the top WCTR outputs, total time the output took, and output name. E.g. like this:
//      0.594     19.004    32.0 whatever.cpp.o
void printWallClockTimeResponsibilities(
		std::vector<WallClockTimeResponsibility> const& wallClockTimeResponsibilities,
		std::unordered_map<std::string, StartStop> const& outputToStartStop,
		size_t numLinesToPrint) {

	if (numLinesToPrint == 0 || numLinesToPrint > wallClockTimeResponsibilities.size()) {
		numLinesToPrint = wallClockTimeResponsibilities.size();
	}

	std::cout << "      WCTR  wallclock parallel output" << std::endl;
	for (size_t i = 0; i < numLinesToPrint; ++i) {
		auto const& wctr = wallClockTimeResponsibilities[i];
		auto it = outputToStartStop.find(wctr.output);
		auto wallClockTime = std::chrono::duration<double>(it->second.stop - it->second.start);
		auto parallelism = wallClockTime / wctr.time;
		std::cout << std::fixed << std::setprecision(3) << std::setw(10) << wctr.time.count() << " " << std::setw(10)
				  << wallClockTime.count() << " " << std::setprecision(1) << std::setw(8) << parallelism << " " << wctr.output << std::endl;
	}
}

auto main(int argc, char** argv) -> int {
	if (argc != 2) {
		std::cerr << "usage: ninjalog2wctr <numlines or 0 for all>" << std::endl;
		return -1;
	}
	auto numLinesToPrint = atoi(argv[1]);

	auto outputToStartStop = parseNinjaLog();
	auto sortedEvents = createdSortedEvents(outputToStartStop);
	auto wallClockTimeResponsibilities = processEventsToWallClockTimeResponsibilities(sortedEvents);

	printWallClockTimeResponsibilities(wallClockTimeResponsibilities, outputToStartStop, numLinesToPrint);
}
