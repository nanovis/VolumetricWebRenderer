#pragma once

#include "FileWatcher/FileWatcher.h"

#include "VolumeRenderer.h"
#include "PlaneRenderer.h"
#include "AnnotationHandler.h"

class FileWatchHelper : public FW::FileWatchListener
{
public:
	FileWatchHelper();

	void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action);
	FW::FileWatcher fileWatcher;

	VolumeRenderer* volumeRenderer = nullptr;
	PlaneRenderer* planeRenderer = nullptr;
	AnnotationHandler* annotationHandler = nullptr;
	bool reloaded = false;

};

