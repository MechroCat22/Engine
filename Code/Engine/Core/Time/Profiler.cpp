/************************************************************************/
/* File: Profiler.cpp
/* Author: Andrew Chase
/* Date: June 30th, 2018
/* Description: Implementation of the Profiler class
/************************************************************************/
#include "Game/Framework/EngineBuildPreferences.hpp" // Only game code in engine

#include "Engine/Core/Gif.hpp"
#include "Engine/Assets/AssetDB.hpp"
#include "Engine/Core/Time/Time.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/LogSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Time/Profiler.hpp"
#include "Engine/Rendering/Meshes/Mesh.hpp"
#include "Engine/Rendering/Core/Renderer.hpp"
#include "Engine/Core/Time/ProfileReport.hpp"
#include "Engine/Rendering/Resources/Sampler.hpp"
#include "Engine/Core/Time/ProfileMeasurement.hpp"
#include "Engine/Core/Time/ProfileReportEntry.hpp"
#include "Engine/Core/DeveloperConsole/Command.hpp"
#include "Engine/Rendering/Materials/MaterialInstance.hpp"

#ifdef PROFILING_ENABLED

// Singleton instance
Profiler*			Profiler::s_instance = nullptr;	

// UI constants
AABB2				Profiler::s_fpsBorderBounds;
AABB2				Profiler::s_frameBorderBounds;
AABB2				Profiler::s_titleBorderBounds;
AABB2				Profiler::s_graphBorderBounds;
AABB2				Profiler::s_viewDataBorderBounds;
AABB2				Profiler::s_graphDetailsBorderBounds;
AABB2				Profiler::s_rottyTopsBorderBounds;


AABB2				Profiler::s_titleBounds;
AABB2				Profiler::s_fpsBounds;
AABB2				Profiler::s_frameBounds;
AABB2				Profiler::s_graphBounds;
AABB2				Profiler::s_viewDataBounds;
AABB2				Profiler::s_viewHeadingBorderBounds;
AABB2				Profiler::s_viewHeadingBounds;
AABB2				Profiler::s_graphDetailsBounds;
AABB2				Profiler::s_rottyTopsBackgroundBounds;
AABB2				Profiler::s_rottyTopsTextureBounds;


float				Profiler::s_titleFontSize;
float				Profiler::s_fpsFrameFontSize;
float				Profiler::s_viewHeadingFontSize;
float				Profiler::s_viewDataFontSize;
float				Profiler::s_borderThickness;

std::string			Profiler::s_titleText;
std::string			Profiler::s_fpsframeText;
std::string			Profiler::s_viewHeadingText;


Rgba				Profiler::s_backgroundColor	= Rgba(0,0,0,180);
Rgba				Profiler::s_borderColor		= Rgba(15, 60, 120, 200);
Rgba				Profiler::s_fontColor		= Rgba(100, 100, 100, 255);
Rgba				Profiler::s_fontHighlightColor = Rgba(200, 200, 200, 255);
Rgba				Profiler::s_graphRedColor = Rgba(255, 0, 0, 150);
Rgba				Profiler::s_graphYellowColor = Rgba(255, 255, 0, 150);
Rgba				Profiler::s_graphGreenColor = Rgba(0, 255, 0, 150);
Rgba				Profiler::s_graphSelectionColor = Rgba(15, 60, 200, 220);
Rgba				Profiler::s_fpsTextColor;

Mesh*				Profiler::s_graphMesh = nullptr;
Gif*				Profiler::s_rottyTopsGif = nullptr;
MaterialInstance*	Profiler::s_rottyTopsMaterial = nullptr;

// C functions
unsigned int	IncrementIndexWithWrapAround(unsigned int currentIndex);
unsigned int	DecrementIndexWithWrapAround(unsigned int currentIndex);
void			DestroyStack(ProfileMeasurement* stack);

// Commands
void Command_ProfilerShow(Command& cmd);
void Command_ProfilerHide(Command& cmd);
void Command_ProfilerPause(Command& cmd);
void Command_ProfilerResume(Command& cmd);
void Command_ProfilerReportType(Command& cmd);
void Command_ProfilerSortOrder(Command& cmd);

//-----------------------------------------------------------------------------------------------
// Constructor
//
Profiler::Profiler()
	: m_generatingReportType(REPORT_TYPE_TREE)
	, m_reportSortOrder(REPORT_SORT_TOTAL_TIME)
	, m_isOpen(false)
	, m_isPaused(false)
	, m_currentFrameNumber(0)
	, m_firstSelectionIndex(-1)
	, m_secondSelectionIndex(-1)
	, m_isSelectingFrames(false)
	, m_framesPerSecond(0.f)
{
	// Initialize all reports to nullptr
	for (int i = 0; i < PROFILER_MAX_REPORT_COUNT; ++i)
	{
		m_measurements[i] = nullptr;
		m_reports[i] = nullptr;
	}
}


//-----------------------------------------------------------------------------------------------
// Destructor
//
Profiler::~Profiler()
{
	// Profile stacks
	for (int i = 0; i < PROFILER_MAX_REPORT_COUNT; ++i)
	{
		if (m_measurements[i] != nullptr)
		{
			delete m_measurements[i];
			m_measurements[i] = nullptr;
		}
	}


	// Reports
	for (int i = 0; i < PROFILER_MAX_REPORT_COUNT; ++i)
	{
		if (m_reports[i] != nullptr)
		{
			delete m_reports[i];
			m_reports[i] = nullptr;
		}
	}
}


//-----------------------------------------------------------------------------------------------
// Static function used to create the singleton instance (constructor is private)
//
void Profiler::Initialize()
{
	s_instance = new Profiler();

	InitializeUILayout();
	InitializeConsoleCommands();
}


//-----------------------------------------------------------------------------------------------
// Sets all the UI layout values for rendering the profiler to screen
//
void Profiler::InitializeUILayout()
{
	AABB2 bounds = Renderer::GetUIBounds();
	Vector2 dimensions = bounds.GetDimensions();

	// Font sizes
	s_titleFontSize			= 48.f;
	s_fpsFrameFontSize		= 48.f;
	s_viewHeadingFontSize	= 20.f;
	s_viewDataFontSize		= 20.f;
	s_borderThickness		= 5.f;

	s_titleBorderBounds = AABB2(Vector2(0.f, dimensions.y - s_titleFontSize - (2.f * s_borderThickness)), 
		Vector2(0.333f * dimensions.x, dimensions.y));

	s_fpsBorderBounds = AABB2(s_titleBorderBounds.GetBottomRight(), 
		Vector2(s_titleBorderBounds.maxs.x + ((dimensions.x - s_titleBorderBounds.GetDimensions().x) * 0.5f), bounds.maxs.y));

	s_fpsBounds = s_fpsBorderBounds;
	s_fpsBounds.AddPaddingToSides(-s_borderThickness, -s_borderThickness);

	s_frameBorderBounds = AABB2(s_fpsBorderBounds.GetBottomRight(), bounds.maxs);

	s_frameBounds = s_frameBorderBounds;
	s_frameBounds.AddPaddingToSides(-s_borderThickness, -s_borderThickness);

	s_graphBorderBounds = AABB2(Vector2(0.05f * dimensions.x, 0.8f * dimensions.y), Vector2(s_fpsBorderBounds.maxs.x, s_fpsBorderBounds.mins.y));

	s_graphDetailsBorderBounds = AABB2(s_graphBorderBounds.GetBottomRight(), s_frameBorderBounds.GetBottomRight());

	s_graphDetailsBounds = s_graphDetailsBorderBounds;
	s_graphDetailsBounds.AddPaddingToSides(-s_borderThickness, -s_borderThickness);

	s_viewHeadingBorderBounds = AABB2(Vector2(0.f, s_graphBorderBounds.mins.y - s_viewHeadingFontSize - (2.f * s_borderThickness)), Vector2(dimensions.x, s_graphBorderBounds.mins.y));
	s_viewHeadingBounds = s_viewHeadingBorderBounds;
	s_viewHeadingBounds.AddPaddingToSides(-s_borderThickness, -s_borderThickness);

	s_viewDataBorderBounds = AABB2(bounds.mins, s_viewHeadingBorderBounds.GetBottomRight());

	s_viewDataBounds = s_viewDataBorderBounds;
	s_viewDataBounds.AddPaddingToSides(-s_borderThickness, -s_borderThickness);

	s_graphBounds = s_graphBorderBounds;
	s_graphBounds.AddPaddingToSides(-s_borderThickness, -s_borderThickness);

	s_titleBounds = s_titleBorderBounds;
	s_titleBounds.AddPaddingToSides(-s_borderThickness, -s_borderThickness);

	s_rottyTopsBorderBounds = AABB2(s_viewHeadingBorderBounds.GetTopLeft(), s_graphBorderBounds.GetTopLeft());
	s_rottyTopsBackgroundBounds = s_rottyTopsBorderBounds;
	s_rottyTopsBackgroundBounds.AddPaddingToSides(-s_borderThickness, -s_borderThickness);

	s_graphMesh = new Mesh();

	s_rottyTopsGif = new Gif();
	s_rottyTopsGif->LoadFromFile("Data/Images/RottyTops.gif");

	IntVector2 gifDimensions = s_rottyTopsGif->GetDimensions();
	float gifAspect = (float) gifDimensions.x / (float) gifDimensions.y;

	float gifHeight = s_rottyTopsBackgroundBounds.GetDimensions().y;
	float gifWidth = gifHeight * gifAspect;

	float startX = 0.5f * (s_rottyTopsBackgroundBounds.GetDimensions().x - gifWidth) + s_rottyTopsBackgroundBounds.mins.x;
	s_rottyTopsTextureBounds = AABB2(Vector2(startX, s_rottyTopsBackgroundBounds.mins.y), Vector2(startX + gifWidth, s_rottyTopsBackgroundBounds.maxs.y));

	s_rottyTopsMaterial = new MaterialInstance(AssetDB::GetSharedMaterial("UI"));
	Sampler* sampler = new Sampler();
	sampler->Initialize(SAMPLER_FILTER_LINEAR, EDGE_SAMPLING_REPEAT);
	s_rottyTopsMaterial->SetSampler(0, sampler);
}


//-----------------------------------------------------------------------------------------------
// Adds the profiler's list of console commands to the command registry
//
void Profiler::InitializeConsoleCommands()
{
	Command::Register("profiler_show",			"Enables Profiler rendering.",								Command_ProfilerShow);
	Command::Register("profiler_hide",			"Disables Profiler rendering.",								Command_ProfilerHide);
	Command::Register("profiler_pause",			"Pauses the profiler report generation.",					Command_ProfilerPause);
	Command::Register("profiler_resume",		"Resumes the profiler report generation.",					Command_ProfilerResume);
	Command::Register("profiler_report_type",	"Sets the profiler report type to the one specified",		Command_ProfilerReportType);
	Command::Register("profiler_sort_order",	"Sets the profiler child sort order to the one provided.",	Command_ProfilerSortOrder);

}


//-----------------------------------------------------------------------------------------------
// Static function used to destroy the singleton instance (destructor is private)
//
void Profiler::Shutdown()
{
	delete s_instance;
	s_instance = nullptr;
}


//-----------------------------------------------------------------------------------------------
// Draws the profiler results to screen
//
void Profiler::Render()
{
	// Only render if the profiler is open
	if (m_isOpen)
	{
		Renderer* renderer = Renderer::GetInstance();
		renderer->SetCurrentCamera(renderer->GetUICamera());
		
		// TITLE, FPS, FRAME COUNT
		RenderTitleInfo();

		// GRAPH
		RenderGraph();

		// DATA
		RenderData();
	}
}


//-----------------------------------------------------------------------------------------------
// Marks the start of the profile frame
//
void Profiler::BeginFrame()
{
	s_instance->m_currentFrameNumber++;

	// Set up the stack frame
	if (s_instance->m_measurements[0] != nullptr) // Check if we already have a measurement
	{
		if (s_instance->m_measurements[PROFILER_MAX_REPORT_COUNT - 1] != nullptr)
		{
			DestroyStack(s_instance->m_measurements[PROFILER_MAX_REPORT_COUNT - 1]);
		}

		for (int i = PROFILER_MAX_REPORT_COUNT - 1; i > 0; --i)
		{
			s_instance->m_measurements[i] = s_instance->m_measurements[i - 1];
		}

		s_instance->PopMeasurement(); // Pop the stack at 0, which finalizes the last frame, and should make [0] null

		ASSERT_OR_DIE(s_instance->m_measurements[0] == nullptr, "Error: Profiler::MarkFrame called before the previous frame could finish"); // if not null, someone forgot to pop after a push somewhere
	}

	// Making a report for the last frame before starting the report generation of the previous frame
	if (s_instance->m_measurements[1] != nullptr && !s_instance->m_isPaused && s_instance->m_isOpen)
	{
		ProfileReport* report = BuildReportForFrame(s_instance->m_measurements[1]);
		s_instance->PushReport(report);
	}

	s_instance->PushMeasurement("Frame"); // Push a new frame into [0]

	// Update the fps if we can
	if (s_instance->m_measurements[1] != nullptr)
	{
		float frameTime = (float) TimeSystem::PerformanceCountToSeconds(s_instance->m_measurements[1]->GetTotalTime_Inclusive());
		s_instance->m_framesPerSecond = (1.0f / frameTime);

		// Color the FPS text
		s_fpsTextColor = s_graphRedColor;
		if (s_instance->m_framesPerSecond > 55.f)
		{
			s_fpsTextColor = s_graphGreenColor;
		}
		else if (s_instance->m_framesPerSecond > 30.f)
		{
			s_fpsTextColor = s_graphYellowColor;
		}
	}
}


//-----------------------------------------------------------------------------------------------
// Checks for input on the profile system, for when the profiler is open
//
void Profiler::ProcessInput()
{
	ProcessKeyboardInput();
	ProcessMouseInput();
}


//-----------------------------------------------------------------------------------------------
// Processes mouse related input
//
void Profiler::ProcessMouseInput()
{
	Mouse& mouse = InputSystem::GetMouse();

	if (!mouse.IsCursorShown()) { return; }

	if (mouse.WasButtonJustPressed(MOUSEBUTTON_LEFT) || mouse.IsButtonPressed(MOUSEBUTTON_LEFT))
	{
		ProcessLeftClick();
	}

	if (mouse.WasButtonJustPressed(MOUSEBUTTON_RIGHT))
	{
		ProcessRightClick();
	}

	// Mouse overs
}


//-----------------------------------------------------------------------------------------------
// Processes left click
//
void Profiler::ProcessLeftClick()
{
	Mouse& mouse = InputSystem::GetMouse();

	// Find out where the mouse was located
	Vector2 mousePos = mouse.GetCursorUIPosition();

	if (mouse.WasButtonJustPressed(MOUSEBUTTON_LEFT))
	{
		if (s_graphBounds.IsPointInside(mousePos))
		{
			Pause();

			// Find which report to display based on where the click happened
			int index = RoundToNearestInt(RangeMapFloat(mousePos.x, s_graphBounds.maxs.x, s_graphBounds.mins.x, 0.f, PROFILER_MAX_REPORT_COUNT - 1.f));

			// Display that report
			SetSelectionState(index, index, true);
		}
	}
	else if (mouse.IsButtonPressed(MOUSEBUTTON_LEFT))
	{
		if (m_isSelectingFrames && s_graphBounds.IsPointInside(mousePos))
		{
			int index = RoundToNearestInt(RangeMapFloat(mousePos.x, s_graphBounds.maxs.x, s_graphBounds.mins.x, 0.f, PROFILER_MAX_REPORT_COUNT - 1.f));
			index = ClampInt(index, 0, PROFILER_MAX_REPORT_COUNT - 1);
			
			m_secondSelectionIndex = index;
		}
	}
}


//-----------------------------------------------------------------------------------------------
// Processes right click
//
void Profiler::ProcessRightClick()
{
	Mouse& mouse = InputSystem::GetMouse();

	// Find out where the mouse was located
	Vector2 mousePos = mouse.GetCursorUIPosition();

	Resume();
}


//-----------------------------------------------------------------------------------------------
// Processes Keyboard input
//
void Profiler::ProcessKeyboardInput()
{
	InputSystem* input = InputSystem::GetInstance();

	// Mouse visibility
	if (input->WasKeyJustPressed('M'))
	{
		Mouse& mouse = InputSystem::GetMouse();
		bool wasShown = mouse.IsCursorShown();

		mouse.ShowMouseCursor(!wasShown);
		mouse.LockCursorToClient(!wasShown);
		mouse.SetCursorMode(wasShown ? CURSORMODE_RELATIVE : CURSORMODE_ABSOLUTE);
	}

	// Entry order
	if (input->WasKeyJustPressed('L'))
	{
		if (m_reportSortOrder == REPORT_SORT_TOTAL_TIME)
		{
			SetReportSortingOrder(REPORT_SORT_SELF_TIME);
		}
		else
		{
			SetReportSortingOrder(REPORT_SORT_TOTAL_TIME);
		}
	}

	// Report type
	if (input->WasKeyJustPressed('V'))
	{
		if (m_generatingReportType == REPORT_TYPE_TREE)
		{
			SetGeneratingReportType(REPORT_TYPE_FLAT);
		}
		else
		{
			SetGeneratingReportType(REPORT_TYPE_TREE);
		}
	}

	// Saving reports to the log file
	if (input->WasKeyJustPressed('P'))
	{
		WriteHistoryAverageToLog();
	}
}


//-----------------------------------------------------------------------------------------------
// Builds the report for the previous frame data, DOES NOT mark the end of a profile frame
//
void Profiler::EndFrame()
{
}


//-----------------------------------------------------------------------------------------------
// Pushes a new profile measurement to the current stack, starting the stack if there isn't one yet
//
void Profiler::PushMeasurement(const char* name)
{
	ProfileMeasurement* measurement = new ProfileMeasurement(name);

	// Set the parent relationship, and the appropriate frame number (though current stack frame number should equal current frame number on profiler)
	if (s_instance->m_measurements[0] == nullptr)
	{
		measurement->m_frameNumber = s_instance->m_currentFrameNumber;
		s_instance->m_measurements[0] = measurement;
	}
	else
	{
		measurement->m_frameNumber = s_instance->m_measurements[0]->m_frameNumber;
		measurement->m_parent = s_instance->m_measurements[0];
		s_instance->m_measurements[0]->m_children.push_back(measurement);
		s_instance->m_measurements[0] = measurement;
	}
}


//-----------------------------------------------------------------------------------------------
// Sets the current stack pointer back one level, to point at the current measurement's parent
//
void Profiler::PopMeasurement()
{
	ASSERT_OR_DIE(s_instance->m_measurements[0] != nullptr, Stringf("Error::Profiler::PopStack called when the current stack was empty"));

	s_instance->m_measurements[0]->Finish();
	s_instance->m_measurements[0] = s_instance->m_measurements[0]->m_parent;
}


//-----------------------------------------------------------------------------------------------
// Sets whether we should be generating new reports or not
// If starting generation, it will regenerate all reports
//
void Profiler::SetGeneratingReportType(eReportType reportType)
{
	bool typeChanged = s_instance->m_generatingReportType != reportType;
	s_instance->m_generatingReportType = reportType;

	if (typeChanged)
	{
		s_instance->FlushReports();
	}
}


//-----------------------------------------------------------------------------------------------
// Sets the sorting order of all reports to the one specified
//
void Profiler::SetReportSortingOrder(eSortOrder order)
{
	bool orderChanged = s_instance->m_reportSortOrder != order;
	s_instance->m_reportSortOrder = order;

	// Update all the reports to be the new order
	if (orderChanged)
	{
		s_instance->FlushReports();
	}
}


//-----------------------------------------------------------------------------------------------
// Enables profiler rendering
//
void Profiler::Show()
{
	bool wasShown  = s_instance->m_isOpen;
	s_instance->m_isOpen = true;

	// Update all the reports to be up to date if we weren't making reports previously
	if (!wasShown)
	{
		s_instance->FlushReports();
	}

	s_instance->m_isPaused = false;
}


//-----------------------------------------------------------------------------------------------
// Disables profiler rendering
//
void Profiler::Hide()
{
	s_instance->m_isOpen = false;

	// Clean up state when we leave
	s_instance->SetSelectionState(-1, -1, false);
}


//-----------------------------------------------------------------------------------------------
// Pauses the profiler's report generation
//
void Profiler::Pause()
{
	s_instance->m_isPaused = true;
}


//-----------------------------------------------------------------------------------------------
// Resumes the profiler's report generation
//
void Profiler::Resume()
{
	bool wasPaused = s_instance->m_isPaused;
	s_instance->m_isPaused = false;

	if (wasPaused)
	{
		s_instance->FlushReports();
	}

	// Disable selecting to clean up state
	s_instance->SetSelectionState(-1, -1, false);
}


//-----------------------------------------------------------------------------------------------
// Sets the profiler's selection state to match the given parameters
//
void Profiler::SetSelectionState(int firstIndex, int secondIndex, bool isSelecting)
{
	m_firstSelectionIndex = firstIndex;
	m_secondSelectionIndex = secondIndex;
	m_isSelectingFrames = isSelecting;
}


//-----------------------------------------------------------------------------------------------
// Returns whether or not the profiler is open, used for rendering
//
bool Profiler::IsProfilerOpen()
{
	return s_instance->m_isOpen;
}


//-----------------------------------------------------------------------------------------------
// Returns the profiler singleton instance
//
Profiler* Profiler::GetInstance()
{
	return s_instance;
}


//-----------------------------------------------------------------------------------------------
// Returns the average frame time for all records between startIndex and endIndex, inclusive
//
float Profiler::GetAverageTotalTime(int index1, int index2) const
{
	int startIndex;
	int endIndex;

	if (index1 > index2)
	{
		startIndex = index2;
		endIndex = index1;
	}
	else
	{
		startIndex = index1;
		endIndex = index2;
	}

	uint64_t totalHPC = 0;
	int count = 0;
	for (int index = startIndex; index <= endIndex; ++index)
	{
		if (m_reports[index] == nullptr) { break; }
		count++;
		totalHPC += m_reports[index]->m_rootEntry->m_totalTime;
	}

	float totalSeconds = (float) TimeSystem::PerformanceCountToSeconds(totalHPC);

	float average = 0.f;
	if (count > 0)
	{
		average = totalSeconds / (float) count;
	}

	return average;
}


//-----------------------------------------------------------------------------------------------
// Returns a report that show the accumulated time and percentages of all reports within the indices
//
ProfileReport* Profiler::GetAccumulatedReport(int firstIndex, int secondIndex) const
{
	int startIndex;
	int endIndex;

	if (firstIndex > secondIndex)
	{
		startIndex = secondIndex;
		endIndex = firstIndex;
	}
	else
	{
		startIndex = firstIndex;
		endIndex = secondIndex;
	}

	ProfileReport* report = new ProfileReport(-1);
	report->m_rootEntry = new ProfileReportEntry("Frame");

	for (int reportIndex = startIndex; reportIndex <= endIndex; ++reportIndex)
	{
		ProfileReport* currReport = m_reports[reportIndex];
		if (currReport == nullptr) { break; }

		AddEntryInfoRecursive(currReport->m_rootEntry, report->m_rootEntry);
	}


	report->Finalize();
	return report;
}


//-----------------------------------------------------------------------------------------------
// Recursively accumulates all entry data from the source to the destination
//
void Profiler::AddEntryInfoRecursive(ProfileReportEntry* srcEntry, ProfileReportEntry* dstEntry) const
{
	dstEntry->AccumulateData(srcEntry);

	for (int srcChildIndex = 0; srcChildIndex < (int) srcEntry->m_children.size(); ++srcChildIndex)
	{
		ProfileReportEntry* currSrcChild = srcEntry->m_children[srcChildIndex];
		std::string srcChildName = currSrcChild->m_name;

		ProfileReportEntry* currDstChild = dstEntry->GetOrCreateReportEntryForChild(srcChildName);

		AddEntryInfoRecursive(currSrcChild, currDstChild);
	}
}


//-----------------------------------------------------------------------------------------------
// Finds the average time values for the given log, prints them to the log file, and recursively
// calls on the children
//
void RecursivelyWriteAverageReportToLog(ProfileReportEntry* accumulatedEntry, int numSamplesForEntry, int indent)
{
	// Write this entry, dividing by the number of samples to get the average
	accumulatedEntry->m_callCount = Ceiling((float)accumulatedEntry->m_callCount / (float)numSamplesForEntry);
	accumulatedEntry->m_selfTime /= numSamplesForEntry;
	accumulatedEntry->m_totalTime /= numSamplesForEntry;

	std::string logText = accumulatedEntry->GetAsStringForUI(indent);
	LogPrintString(logText);

	// Recursively call on children
	int numChildren = (int)accumulatedEntry->m_children.size();
	for (int childIndex = 0; childIndex < numChildren; ++childIndex)
	{
		RecursivelyWriteAverageReportToLog(accumulatedEntry->m_children[childIndex], numSamplesForEntry, indent + 1);
	}
}


//-----------------------------------------------------------------------------------------------
// Finds the average off all profile report entries currently in the history and writes them to
// file using the LogSystem
//
void Profiler::WriteHistoryAverageToLog() const
{
	ProfileReport* accumulatedReport = GetAccumulatedReport(0, PROFILER_MAX_REPORT_COUNT - 1);
	ProfileReportEntry* currEntry = accumulatedReport->m_rootEntry;

	LogPrintf("---------- FRAME PROFILE - AVERAGE OF THE LAST %u FRAMES ----------", PROFILER_MAX_REPORT_COUNT);
	RecursivelyWriteAverageReportToLog(currEntry, PROFILER_MAX_REPORT_COUNT, 0);

	ConsolePrintf(Rgba::GREEN, "Wrote the current %u samples in the history to the log file", PROFILER_MAX_REPORT_COUNT);
}


//-----------------------------------------------------------------------------------------------
// Builds the report to represent the given performance frame
//
ProfileReport* Profiler::BuildReportForFrame(ProfileMeasurement* stack)
{
	ProfileReport* report = new ProfileReport(stack->m_frameNumber);

	switch (s_instance->m_generatingReportType)
	{
	case REPORT_TYPE_TREE:
		report->InitializeAsTreeReport(stack, s_instance->m_reportSortOrder);
		break;
	case REPORT_TYPE_FLAT:
		report->InitializeAsFlatReport(stack, s_instance->m_reportSortOrder);
		break;
	default:
		break;
	}

	return report;
}


//-----------------------------------------------------------------------------------------------
// Adds the given report to the list in the front, removing and deleting the last if it is full
//
void Profiler::PushReport(ProfileReport* report)
{
	// Check the end
	if (m_reports[PROFILER_MAX_REPORT_COUNT - 1] != nullptr)
	{
		delete m_reports[PROFILER_MAX_REPORT_COUNT - 1];
	}

	// Shift all report pointers over one
	for (int i = PROFILER_MAX_REPORT_COUNT - 1; i > 0; --i)
	{
		s_instance->m_reports[i] = s_instance->m_reports[i - 1];
	}

	// Put the new report at the front
	s_instance->m_reports[0] = report;
}


//-----------------------------------------------------------------------------------------------
// Constructs all the reports in the parallel report array to reflect the current measurement array
//
void Profiler::FlushReports()
{
	for (int index = 0; index < PROFILER_MAX_REPORT_COUNT - 1; ++index)
	{
		// Cleanup first (slow but safe)
		if (m_reports[index] != nullptr)
		{
			delete m_reports[index];
			m_reports[index] = nullptr;
		}

		// Reports lag one frame behind active measurements
		if (m_measurements[index + 1] != nullptr)
		{
			m_reports[index] = BuildReportForFrame(m_measurements[index + 1]);
		}
	}
}


//-----------------------------------------------------------------------------------------------
// Renders the title information (title, fps, frame count) to screen
//
void Profiler::RenderTitleInfo() const
{
	Renderer* renderer	= Renderer::GetInstance();
	Material* material	= AssetDB::GetSharedMaterial("UI");
	BitmapFont* font	= AssetDB::GetBitmapFont("Data/Images/Fonts/ConsoleFont.png");

	// Title bounds
	renderer->Draw2DQuad(s_titleBorderBounds,	AABB2::UNIT_SQUARE_OFFCENTER, s_borderColor,		material);
	renderer->Draw2DQuad(s_titleBounds,			AABB2::UNIT_SQUARE_OFFCENTER, s_backgroundColor,	material);
												
	// Fps bounds								
	renderer->Draw2DQuad(s_fpsBorderBounds,		AABB2::UNIT_SQUARE_OFFCENTER, s_borderColor,		material);
	renderer->Draw2DQuad(s_fpsBounds,			AABB2::UNIT_SQUARE_OFFCENTER, s_backgroundColor,	material);
												
	// Frame count bounds						
	renderer->Draw2DQuad(s_frameBorderBounds,	AABB2::UNIT_SQUARE_OFFCENTER, s_borderColor,	material);
	renderer->Draw2DQuad(s_frameBounds,			AABB2::UNIT_SQUARE_OFFCENTER, s_backgroundColor, material);

	// Text
	std::string frameText	= Stringf("FRAME: %*i",		6,	m_currentFrameNumber);
	std::string fpsText		= Stringf("FPS: %*.2f",		8,	m_framesPerSecond);

	renderer->DrawTextInBox2D("PROFILER",	s_titleBounds,		Vector2::ZERO, s_titleFontSize,		TEXT_DRAW_OVERRUN, font, s_fontHighlightColor);
	renderer->DrawTextInBox2D(frameText,	s_frameBounds,		Vector2::ZERO, s_fpsFrameFontSize,	TEXT_DRAW_OVERRUN, font, s_fontHighlightColor);	
	renderer->DrawTextInBox2D(fpsText,		s_fpsBounds,		Vector2::ZERO, s_fpsFrameFontSize,	TEXT_DRAW_OVERRUN, font, s_fpsTextColor);

	// RottyTops!
	s_rottyTopsMaterial->SetDiffuse(s_rottyTopsGif->GetNextFrame());

	renderer->Draw2DQuad(s_rottyTopsBorderBounds,		AABB2::UNIT_SQUARE_OFFCENTER, s_borderColor,		material);
	renderer->Draw2DQuad(s_rottyTopsBackgroundBounds,	AABB2::UNIT_SQUARE_OFFCENTER, s_backgroundColor,	material);
	renderer->Draw2DQuad(s_rottyTopsTextureBounds,		AABB2::UNIT_SQUARE_OFFCENTER, Rgba::WHITE,			s_rottyTopsMaterial);
}


//-----------------------------------------------------------------------------------------------
// Renders the performance graph to screen
//
void Profiler::RenderGraph() const
{
	Renderer* renderer	= Renderer::GetInstance();
	Material* material	= AssetDB::GetSharedMaterial("UI");
	BitmapFont* font	= AssetDB::GetBitmapFont("Data/Images/Fonts/ConsoleFont.png");

	renderer->Draw2DQuad(s_graphBorderBounds,	AABB2::UNIT_SQUARE_OFFCENTER, s_borderColor,		material);
	renderer->Draw2DQuad(s_graphBounds,			AABB2::UNIT_SQUARE_OFFCENTER, s_backgroundColor,	material);

	// Get the worst frame time for scaling, and average frame time
	float worstFrameTime = (1.f / 240.f);
	unsigned int reportCount = 0;
	for (int reportIndex = 0; reportIndex < PROFILER_MAX_REPORT_COUNT; ++reportIndex)
	{
		if (m_reports[reportIndex] == nullptr) { break; }

		reportCount++;
		float currTime = (float) TimeSystem::PerformanceCountToSeconds(m_reports[reportIndex]->m_rootEntry->m_totalTime);
		if (worstFrameTime < currTime)
		{
			worstFrameTime = currTime;
		}
	}


	float timeUsedToScale = (1.f / 30.f);
	timeUsedToScale = (worstFrameTime > timeUsedToScale ? worstFrameTime : timeUsedToScale);

	// Build the mesh
	MeshBuilder mb;
	mb.BeginBuilding(PRIMITIVE_TRIANGLES, false);

	Vector2 graphDimensions = s_graphBounds.GetDimensions();
	Vector2 graphOffset = s_graphBounds.GetBottomRight();

	for (int reportIndex = 0; reportIndex < PROFILER_MAX_REPORT_COUNT - 1; ++reportIndex)
	{
		if (m_reports[reportIndex] == nullptr || m_reports[reportIndex + 1] == nullptr) { break; }

		float currX = graphOffset.x - (graphDimensions.x * reportIndex * (1.f / (PROFILER_MAX_REPORT_COUNT - 1)));
		float nextX = graphOffset.x - (graphDimensions.x * (reportIndex + 1) * (1.f / (PROFILER_MAX_REPORT_COUNT - 1)));

		float currTime = (float) TimeSystem::PerformanceCountToSeconds( m_reports[reportIndex]->m_rootEntry->m_totalTime);
		float currY = RangeMapFloat(currTime, 0.f, timeUsedToScale, s_graphBounds.mins.y, s_graphBounds.maxs.y);

		float nextTime = (float) TimeSystem::PerformanceCountToSeconds( m_reports[reportIndex + 1]->m_rootEntry->m_totalTime);
		float nextY = RangeMapFloat(nextTime, 0.f, timeUsedToScale, s_graphBounds.mins.y, s_graphBounds.maxs.y);

		Rgba currColor = s_graphGreenColor;
		if (currTime > (1.f / 55.f)) // Less than 55 fps
		{
			currColor = s_graphYellowColor;
		}
		else if (currTime > (1.f / 30.f)) // Less than 30 fps
		{
			currColor = s_graphRedColor;
		}

		Rgba nextColor = s_graphGreenColor;
		if (nextTime > (1.f / 55.f)) // Less than 55 fps
		{
			nextColor = s_graphYellowColor;
		}
		else if (nextTime > (1.f / 30.f)) // Less than 30 fps
		{
			nextColor = s_graphRedColor;
		}

		mb.SetColor(nextColor);
		mb.PushVertex(Vector3(nextX, graphOffset.y, 0.f));
		mb.SetColor(currColor);
		mb.PushVertex(Vector3(currX, graphOffset.y, 0.f));
		mb.PushVertex(Vector3(currX, currY, 0.f));

		mb.SetColor(nextColor);
		mb.PushVertex(Vector3(nextX, graphOffset.y, 0.f));
		mb.SetColor(currColor);
		mb.PushVertex(Vector3(currX, currY, 0.f));
		mb.SetColor(nextColor);
		mb.PushVertex(Vector3(nextX, nextY, 0.f));
	}

	mb.FinishBuilding();
	mb.UpdateMesh(*s_graphMesh);

	renderer->DrawMeshWithMaterial(s_graphMesh, material);

	renderer->Draw2DQuad(s_graphDetailsBorderBounds,	AABB2::UNIT_SQUARE_OFFCENTER, s_borderColor,		material);
	renderer->Draw2DQuad(s_graphDetailsBounds,			AABB2::UNIT_SQUARE_OFFCENTER, s_backgroundColor,	material);

	// Graph selection
	if (m_isSelectingFrames)
	{
		// Just draw a line if they equal
		if (m_firstSelectionIndex == m_secondSelectionIndex)
		{
			float x = graphOffset.x - (graphDimensions.x * m_firstSelectionIndex * (1.f / (PROFILER_MAX_REPORT_COUNT - 1)));
			renderer->Draw3DLine(Vector3(x, s_graphBounds.mins.y, 0.f), s_graphSelectionColor, Vector3(x, s_graphBounds.maxs.y, 0.f), s_graphSelectionColor);
		}
		else // Otherwise draw a quad for the selection
		{
			float startX = graphOffset.x - (graphDimensions.x * m_firstSelectionIndex * (1.f / (PROFILER_MAX_REPORT_COUNT - 1)));
			float endX = graphOffset.x - (graphDimensions.x * m_secondSelectionIndex * (1.f / (PROFILER_MAX_REPORT_COUNT - 1)));

			// Swap them if they're out of order
			if (startX > endX)
			{
				float temp = endX;
				endX = startX;
				startX = temp;
			}

			startX = ClampFloat(startX, s_graphBounds.mins.x, s_graphBounds.maxs.x);
			endX = ClampFloat(endX,		s_graphBounds.mins.x, s_graphBounds.maxs.x);

			AABB2 selection = AABB2(Vector2(startX, s_graphBounds.mins.y), Vector2(endX, s_graphBounds.maxs.y));
			renderer->Draw2DQuad(selection, AABB2::UNIT_SQUARE_OFFCENTER, s_graphSelectionColor, material);
		}
	}

	// Details
	if (m_reports[0] != nullptr && !m_isSelectingFrames)
	{
		float currTime = (float) TimeSystem::PerformanceCountToSeconds(m_reports[0]->m_rootEntry->m_totalTime);
		float drawY = RangeMapFloat(currTime, 0.f, timeUsedToScale, s_graphBorderBounds.mins.y, s_graphBorderBounds.maxs.y);
		renderer->DrawText2D(Stringf("%.2f ms", currTime * 1000.f), Vector2(s_graphDetailsBounds.mins.x, drawY), s_viewDataFontSize, font, s_fpsTextColor);
	}

	Mouse& mouse = InputSystem::GetMouse();
	std::string detailText;
	if (mouse.IsCursorShown())
	{
		detailText = "Mouse: SHOWN\n";
	}
	else
	{
		detailText = "Mouse: HIDDEN\n";
	}

	if (m_generatingReportType == REPORT_TYPE_FLAT)
	{
		detailText += "View: FLAT\n";
	}
	else
	{
		detailText += "View: TREE\n";
	}

	if (m_reportSortOrder == REPORT_SORT_SELF_TIME)
	{
		detailText += "Sort: SELF";
	}
	else
	{
		detailText += "Sort: TOTAL";
	}

	renderer->DrawTextInBox2D(detailText, s_graphDetailsBounds, Vector2::ONES, s_viewDataFontSize, TEXT_DRAW_OVERRUN, font, s_fontColor);

	// Show average of selection
	float averageTime;
	if (m_isSelectingFrames)
	{
		averageTime = GetAverageTotalTime(m_firstSelectionIndex, m_secondSelectionIndex);
	}
	else
	{
		averageTime = GetAverageTotalTime(0, PROFILER_MAX_REPORT_COUNT - 1);
	}

	averageTime *= 1000.f;
	renderer->DrawTextInBox2D(Stringf("Average Frame: %*.2f ms", 5, averageTime), s_graphDetailsBounds, Vector2(1.f, 0.f), s_viewDataFontSize, TEXT_DRAW_OVERRUN, font, s_fontColor);
}


//-----------------------------------------------------------------------------------------------
// Renders the latest frame data to screen
//
void Profiler::RenderData() const
{
	Renderer* renderer	= Renderer::GetInstance();
	Material* material	= AssetDB::GetSharedMaterial("UI");
	BitmapFont* font	= AssetDB::GetBitmapFont("Data/Images/Fonts/ConsoleFont.png");

	renderer->Draw2DQuad(s_viewHeadingBorderBounds,		AABB2::UNIT_SQUARE_OFFCENTER, s_borderColor, material);
	renderer->Draw2DQuad(s_viewHeadingBounds,			AABB2::UNIT_SQUARE_OFFCENTER, s_backgroundColor, material);
	renderer->Draw2DQuad(s_viewDataBorderBounds,		AABB2::UNIT_SQUARE_OFFCENTER, s_borderColor,	material);
	renderer->Draw2DQuad(s_viewDataBounds,				AABB2::UNIT_SQUARE_OFFCENTER, s_backgroundColor, material);

	//std::string headingText = Stringf("FUNCTION NAME%*sCALLS%*s%% TOTAL%*sTIME%*s%% SELF%*sTIME", 12, "", 5, "", 8, "", 6, "", 8, "");
	std::string headingText = Stringf("%-*s%*s%*s%*s%*s%*s", 
		44, "FUNCTION NAME", 8, "CALLS", 10, "% TOTAL", 10, "TIME", 10, "% SELF", 10, "TIME");
	
	renderer->DrawTextInBox2D(headingText, s_viewHeadingBounds, Vector2::ZERO, s_viewHeadingFontSize, TEXT_DRAW_OVERRUN, font, s_fontHighlightColor);


	// Render all the report entries
	AABB2 entryBounds = AABB2(Vector2(s_viewDataBounds.mins.x, s_viewDataBounds.maxs.y - s_viewDataFontSize), s_viewDataBounds.maxs);

	if (m_isSelectingFrames) // Display the data from the selected frames
	{
		ProfileReport* report = GetAccumulatedReport(m_firstSelectionIndex, m_secondSelectionIndex);
		RecursivelyPrintEntry(0, entryBounds, report->m_rootEntry);
		delete report;
	}
	else // Display the data from the root of the last report
	{
		ProfileReport* report = m_reports[0];
		if (report == nullptr) { return; }
		RecursivelyPrintEntry(0, entryBounds, report->m_rootEntry);
	}
}


//-----------------------------------------------------------------------------------------------
// Prints the current entry on the list, and
//
void Profiler::RecursivelyPrintEntry(unsigned int indent, AABB2& drawBounds, ProfileReportEntry* entry) const
{
	std::string text = entry->GetAsStringForUI(indent);
	Renderer* renderer = Renderer::GetInstance();
	BitmapFont* font = AssetDB::GetBitmapFont("Data/Images/Fonts/ConsoleFont.png");

	// Light up if hovered over
	Mouse& mouse = InputSystem::GetMouse();
	Rgba drawColor = s_fontColor;
	if (mouse.IsCursorShown() && drawBounds.IsPointInside(mouse.GetCursorUIPosition()))
	{
		drawColor = s_fontHighlightColor;
	}

	renderer->DrawTextInBox2D(text.c_str(), drawBounds, Vector2::ZERO, s_viewDataFontSize, TEXT_DRAW_OVERRUN, font, drawColor);

	// Recursively call on children
	drawBounds.Translate(Vector2(0.f, -s_viewDataFontSize));
	for (int childIndex = 0; childIndex < (int) entry->m_children.size(); ++childIndex)
	{
		RecursivelyPrintEntry(indent + 1, drawBounds, entry->m_children[childIndex]);
	}
}


//-----------------------------------------------------------------------------------------------
// Recursively deletes the measurement stack
//
void DestroyStack(ProfileMeasurement* stack)
{
	ASSERT_OR_DIE(stack->m_parent == nullptr, "Error: Profiler::DestroyStack called on a stack with a parented head");
	delete stack; // Recursive deconstructor; will delete all children
}


//-----------------------------------------------------------------------------------------------
// Increments the index to wrap around the report array
//
unsigned int IncrementIndexWithWrapAround(unsigned int currentIndex)
{
	unsigned int nextIndex = currentIndex + 1;

	if (nextIndex >= PROFILER_MAX_REPORT_COUNT)
	{
		nextIndex = 0;
	}

	return nextIndex;
}


//-----------------------------------------------------------------------------------------------
// Decrements the index to wrap around the report array
//
unsigned int DecrementIndexWithWrapAround(unsigned int currentIndex)
{
	unsigned int nextIndex = currentIndex - 1;

	if (nextIndex < 0)
	{
		nextIndex = PROFILER_MAX_REPORT_COUNT - 1;
	}

	return nextIndex;
}


//////////////////////////////////////////////////////////////////////////
// CONSOLE COMMANDS
//////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------------------------
// Shows the profiler rendering
//
void Command_ProfilerShow(Command& cmd)
{
	UNUSED(cmd);
	Profiler::Show();
	ConsolePrintf(Rgba::GREEN, "Profiler opened.");
}


//-----------------------------------------------------------------------------------------------
// Hides the profiler rendering
//
void Command_ProfilerHide(Command& cmd)
{
	UNUSED(cmd);
	Profiler::Hide();
	ConsolePrintf(Rgba::GREEN, "Profiler closed.");
}


//-----------------------------------------------------------------------------------------------
// Pauses the Profiler's report generation
//
void Command_ProfilerPause(Command& cmd)
{
	UNUSED(cmd);
	Profiler::Pause();
	ConsolePrintf(Rgba::GREEN, "Profiler paused.");
}


//-----------------------------------------------------------------------------------------------
// Resumes the Profiler's report generation
//
void Command_ProfilerResume(Command& cmd)
{
	UNUSED(cmd);
	Profiler::Resume();
	ConsolePrintf(Rgba::GREEN, "Profiler resumed.");
}


//-----------------------------------------------------------------------------------------------
// Switches the report view to the one specified
//
void Command_ProfilerReportType(Command& cmd)
{
	std::string typeText;
	bool specified = cmd.GetParam("t", typeText);

	if (!specified)
	{
		ConsoleWarningf("Defaulting profiler to tree view.");
		Profiler::SetGeneratingReportType(REPORT_TYPE_TREE);
	}
	else
	{
		if (typeText == "flat")
		{
			ConsoleWarningf("Setting profiler to flat view.");
			Profiler::SetGeneratingReportType(REPORT_TYPE_FLAT);
		}
		else if (typeText == "tree")
		{
			ConsoleWarningf("Setting profiler to tree view.");
			Profiler::SetGeneratingReportType(REPORT_TYPE_TREE);
		}
		else
		{
			ConsoleWarningf("Unknown type given, defaulting profiler to tree view.");
			Profiler::SetGeneratingReportType(REPORT_TYPE_TREE);
		}
	}
}


//-----------------------------------------------------------------------------------------------
// Sets the sorting order flag on the Profiler to the one specified
//
void Command_ProfilerSortOrder(Command& cmd)
{
	std::string orderText;
	bool specified = cmd.GetParam("t", orderText);

	if (!specified)
	{
		ConsoleWarningf("Defaulting profiler to total time sort order (descending).");
		Profiler::SetReportSortingOrder(REPORT_SORT_TOTAL_TIME);
	}
	else
	{
		if (orderText == "self")
		{
			ConsoleWarningf("Setting profiler to self time sort order (descending)");
			Profiler::SetReportSortingOrder(REPORT_SORT_SELF_TIME);
		}
		else if (orderText == "total")
		{
			ConsoleWarningf("Setting profiler total time sort order (descending).");
			Profiler::SetReportSortingOrder(REPORT_SORT_TOTAL_TIME);
		}
		else
		{
			ConsoleWarningf("Unknown type given, defaulting profiler to total time sort order (descending).");
			Profiler::SetReportSortingOrder(REPORT_SORT_TOTAL_TIME);
		}
	}
}


#else // If not defined, put empty stubs for all functions

Profiler::Profiler() {}
Profiler::~Profiler() {}

#pragma warning(push)
#pragma warning(disable: 4100) // Ignore unused variable warnings
void				Profiler::Initialize() {}
void				Profiler::InitializeUILayout() {}
void				Profiler::InitializeConsoleCommands() {}
void				Profiler::Shutdown() {}
void				Profiler::BeginFrame() {}
void				Profiler::ProcessInput() {}
void				Profiler::ProcessMouseInput() {}
void				Profiler::ProcessLeftClick() {}
void				Profiler::ProcessRightClick() {}
void				Profiler::ProcessKeyboardInput() {}
void				Profiler::Render() {}
void				Profiler::EndFrame() {}											
void				Profiler::PushMeasurement(const char* name) {}
void				Profiler::PopMeasurement() {}
void				Profiler::SetGeneratingReportType(eReportType reportType) {}
void				Profiler::Show() {}
void				Profiler::Hide() {}
void				Profiler::Pause() {}
void				Profiler::Resume() {}
void				Profiler::SetSelectionState(int firstIndex, int secondIndex, bool isSelecting) {}
bool				Profiler::IsProfilerOpen() { return false; }
Profiler*			Profiler::GetInstance() { return nullptr; }
float				Profiler::GetAverageTotalTime(int startIndex, int endIndex) const { return 0.f; }
ProfileReport*		Profiler::GetAccumulatedReport(int firstIndex, int secondIndex) const { return nullptr; }
void				Profiler::AddEntryInfoRecursive(ProfileReportEntry* sourceEntry, ProfileReportEntry* destinationEntry) const {}
ProfileReport*		Profiler::BuildReportForFrame(ProfileMeasurement* stack) { return nullptr; }
void				Profiler::PushReport(ProfileReport* report) {}
void				Profiler::FlushReports() {} 
void				Profiler::RenderTitleInfo() const {}
void				Profiler::RenderGraph() const {}
void				Profiler::RenderData() const {}
void				Profiler::RecursivelyPrintEntry(unsigned int indent, AABB2& drawBounds, ProfileReportEntry* entry) const {}

#pragma warning(pop)
#endif // PROFILING_ENABLED