// lab3.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <thread>
#include <ctime>

using namespace std;

struct Process {
	short queueNum;
	short weight;
};

class TaskManager {
public:
	TaskManager(short taskWeightIntervalStart, short taskWeightIntervalEnd,
		short inputFlowIntensityIntervalStart, short inputFlowIntensityIntervalEnd,
		short queuesMaxValue) {
		this->taskWeightIntervalStart = taskWeightIntervalStart;
		this->taskWeightIntervalEnd = taskWeightIntervalEnd;

		this->inputFlowIntensityIntervalStart = inputFlowIntensityIntervalStart;
		this->inputFlowIntensityIntervalEnd = inputFlowIntensityIntervalEnd;

		this->queuesMaxValue = queuesMaxValue;
		this->taskManagerIsFree = true;
	}

	void generateProcess() {
		Process* newProcess = new Process();
		newProcess->queueNum = 1;
		newProcess->weight = taskWeightIntervalStart + 
			rand()%(taskWeightIntervalEnd - taskWeightIntervalStart+1);
		cout << "NEW PROCESS IN SYSTEM. Expected time to execute: " 
			<< newProcess->weight << "ms.\n";
		if (processesQueues.empty()) {
			list<Process*> lst;
			lst.push_back(newProcess);
			processesQueues.push_back(lst);
		}
		else
			processesQueues.front().push_back(newProcess);
		newProcessAdded = true;
		taskManagerIsFree = false;
	}

	void processesGenerator() {
		srand((unsigned) time(0));
		while (true) {
			generateProcess();
			std::this_thread::sleep_for(std::chrono::milliseconds(
				inputFlowIntensityIntervalStart +
				rand() % (taskWeightIntervalEnd - taskWeightIntervalStart + 1)));
		}
	}

	void execTaskManager() {
		while (true)
			if (!taskManagerIsFree)
				taskManagerIteration();
	}
	
	void taskManagerIteration() {
		list<Process*> lastItemUnfinishedProcesses;
		for (auto queue = processesQueues.begin(); queue != processesQueues.end(); queue++) {
			if (!(*queue).empty()) {
				list<Process*> processesToDelete;
				list<Process*> unfinishedProcesses;
				for (auto process = (*queue).begin(); process != (*queue).end(); ++process) {
					if (queue == processesQueues.begin() || !newProcessAdded) {
						short weight = (*process)->weight;
						short queueNum = (*process)->queueNum;
						if (weight <= queueNum * 10 || queueNum == queuesMaxValue) {
							std::this_thread::sleep_for(
								std::chrono::milliseconds(weight));
							cout << "Finished process from queue " << queueNum
								<< " in " << weight << "ms.\n";
							processesToDelete.push_back(*process);
						}
						else {
							std::this_thread::sleep_for(
								std::chrono::milliseconds(queueNum*10));
							processesToDelete.push_back(*process);
							(*process)->queueNum = queueNum + 1;
							(*process)->weight -= queueNum * 10;
							unfinishedProcesses.push_back(*process);
							cout << "Process from queue " << queueNum
								<< " haven't been finished in " << queueNum*10 
								<< "ms. Time left to process: " << (*process)->weight 
								<< "ms. Moved to queue " << queueNum+1 << "\n";
						}

					}
					else {
						for(auto processToDelete = processesToDelete.begin();
							processToDelete != processesToDelete.end(); ++processToDelete)
							(*queue).remove(*processToDelete);
						if(!unfinishedProcesses.empty())
							relocateProcesses(queue, unfinishedProcesses, lastItemUnfinishedProcesses);
						newProcessAdded = false;
						return;
					}
				}
				(*queue).clear();
				if (!unfinishedProcesses.empty())
					relocateProcesses(queue, unfinishedProcesses, lastItemUnfinishedProcesses);
			}
		}
		if (lastItemUnfinishedProcesses.empty()) {
			cout << "No tasks left. Task Manager is free.\n";
			taskManagerIsFree = true;
		}
		else 
			processesQueues.push_back(lastItemUnfinishedProcesses);
	}


private:
	short taskWeightIntervalStart;
	short taskWeightIntervalEnd;
	short inputFlowIntensityIntervalStart;
	short inputFlowIntensityIntervalEnd;
	short queuesMaxValue;
	bool newProcessAdded;
	bool taskManagerIsFree;

	list<list<Process*>> processesQueues;

	void relocateProcesses(std::list<std::list<Process *>>::iterator queue, std::list<Process *> &unfinishedProcesses, std::list<Process *> &lastItemUnfinishedProcesses)
	{
		if (++queue != processesQueues.end()) {
			for (auto unfinishedProcess = unfinishedProcesses.begin();
				unfinishedProcess != unfinishedProcesses.end(); ++unfinishedProcess)
				(*queue).push_back(*unfinishedProcess);
		}
		else
			lastItemUnfinishedProcesses = unfinishedProcesses;
	}

};

int main()
{
	short taskWeightIntervalStart = 5;
	short taskWeightIntervalEnd = 100;
	short inputFlowIntensityIntervalStart = 10;
	short inputFlowIntensityIntervalEnd = 250;
	short queuesMaxValue = 3;
	cout << "Task Manager's parameters: \n"
		<< "\tTask Weight Interval : [" << taskWeightIntervalStart << "; " << taskWeightIntervalEnd << "]\n"
		<< "\tInput Flow Intensity Interval : [" << inputFlowIntensityIntervalStart
		<< "; " << inputFlowIntensityIntervalEnd << "]\n"
		<< "\tMax Value of Queues : " << queuesMaxValue << "\n\n\n";
	TaskManager* taskManager = new TaskManager(taskWeightIntervalStart, taskWeightIntervalEnd, 
		inputFlowIntensityIntervalStart, inputFlowIntensityIntervalEnd, queuesMaxValue);
	std::thread thread1(&TaskManager::execTaskManager, taskManager);
	std::thread thread2(&TaskManager::processesGenerator, taskManager);
	thread1.join();
	thread2.join();
}
