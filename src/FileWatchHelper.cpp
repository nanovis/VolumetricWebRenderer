#include "FileWatchHelper.h"

#include <string>
#include <iostream>
#include <chrono>
#include <thread>

void FileWatchHelper::handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action)
{
	std::cout << "DIR (" << dir + ") FILE (" + filename + ") has event " << action << std::endl;

	// this is actually a bug. Sometimes the event is received before the system is
	// finished writing the change and you will get file contention errors. So, wait
	// for the write to finish.
	std::cout << "Wait for file to be written..." << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << "Reload Shaders and Pipeline..." << std::endl;

	volumeRenderer->reload();
	annotationHandler->reload();
	planeRenderer->init();

	reloaded = true;

	std::cout << "Done." << std::endl;
}

FileWatchHelper::FileWatchHelper() 
{
	// add a watch to the system
	// the file watcher doesn't manage the pointer to the listener - so make sure you don't just
	// allocate a listener here and expect the file watcher to manage it - there will be a leak!
	FW::WatchID watchID = fileWatcher.addWatch("work/shader", this);
	fileWatcher.addWatch("work/config", this);
}


