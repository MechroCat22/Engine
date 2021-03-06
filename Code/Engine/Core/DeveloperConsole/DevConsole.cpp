/************************************************************************/
/* File: DevConsole.cpp
/* Author: Andrew Chase
/* Date: February 3rd, 2018
/* Description: Implementation of the Developer Console class
/************************************************************************/
#include <stdarg.h>
#include "Engine/Core/File.hpp"
#include "Engine/Core/Window.hpp"
#include "Engine/Core/Time/Time.hpp"
#include "Engine/Core/LogSystem.hpp"
#include "Engine/Assets/AssetDB.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Time/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Rendering/Core/Renderer.hpp"
#include "Engine/Core/Time/ProfileScoped.hpp"
#include "Engine/Core/DeveloperConsole/Command.hpp"
#include "Engine/Rendering/Animation/SpriteAnim.hpp"
#include "Engine/Networking/RemoteCommandService.hpp"
#include "Engine/Core/DeveloperConsole/DevConsole.hpp"
#include "Engine/Rendering/Animation/SpriteAnimDef.hpp"
#include "Engine/Rendering/Animation/SpriteAnimSet.hpp"

const int STRINGF_STACK_LOCAL_TEMP_LENGTH = 2048;	// Used for ConsolePrintf
DevConsole* DevConsole::s_instance = nullptr;		// Singleton instance

// Rendering Console constants

const float DevConsole::TEXT_HEIGHT				= 20.f;		// Text is 20 world units tall
const float	DevConsole::TEXT_PADDING			= 3.f;			
const int	DevConsole::MAX_INPUT_BUFFER_SIZE	= 64;

// Color constants
const Rgba DevConsole::INPUT_BOX_COLOR				= Rgba(50, 50, 50, 100);
const Rgba DevConsole::INPUT_TEXT_COLOR				= Rgba(200,200,200,255);
const Rgba DevConsole::LOG_BOX_COLOR				= Rgba(0, 0, 0, 150);
const Rgba DevConsole::DEFAULT_PRINT_LOG_COLOR		= Rgba(200,200,200,255);
const Rgba DevConsole::DEFAULT_COMMAND_LOG_COLOR	= Rgba(0,255,255,255);


const std::string DEFAULT_LOG_FILENAME = "ConsoleLog.txt";
const std::string LOCAL_LOGS_DIRECTORY = "Data\\Logs\\";

// C functions for managing the input buffer and output log
bool ConsoleMessageHandler(unsigned int msg, size_t wparam, size_t lparam);

// Command Pre-Declarations
void Command_Echo(Command& cmd);
void Command_SaveLog(Command& cmd);
void Command_Clear(Command& cmd);
void Command_HideLog(Command& cmd);
void Command_ShowLog(Command& cmd);
void Command_HookToLogSystem(Command& cmd);
void Command_RunBatchFile(Command& cmd);

// For LogSystem printing
static void WriteToDevConsole(LogMessage_t log, void* args);


//-----------------------------------------------------------------------------------------------
// Prints the given args list to console with the given color, used internally only
//
void ConsolePrintv(const Rgba& color, char const* format, va_list args)
{
	char textLiteral[ STRINGF_STACK_LOCAL_TEMP_LENGTH ];
	vsnprintf_s( textLiteral, STRINGF_STACK_LOCAL_TEMP_LENGTH, _TRUNCATE, format, args );	
	textLiteral[ STRINGF_STACK_LOCAL_TEMP_LENGTH - 1 ] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

	// Add it to the console log
	ConsoleOutputText colorText;
	colorText.m_text	= std::string(textLiteral);
	colorText.m_color	= color;
	colorText.m_threadID = ::GetCurrentThreadId();

	DevConsole* console = DevConsole::GetInstance();
	GUARANTEE_OR_DIE(console != nullptr, "Error: ConsolePrintf called with no DevConsole initialized.");

	console->AddToMessageQueue(colorText);
}


//-----------------------------------------------------------------------------------------------
// Prints the given text to the console in white
//
void ConsolePrintf(char const *format, ...)
{
	// Construct the string
	va_list variableArgumentList;
	va_start( variableArgumentList, format );
	ConsolePrintv(DevConsole::DEFAULT_PRINT_LOG_COLOR, format, variableArgumentList);
	va_end(variableArgumentList);
}


//-----------------------------------------------------------------------------------------------
// Adds the given text to the console log to be printed, with the given color
//
void ConsolePrintf(const Rgba &color, char const *format, ...)
{
	// Construct the string
	char textLiteral[ STRINGF_STACK_LOCAL_TEMP_LENGTH ];
	va_list variableArgumentList;
	va_start( variableArgumentList, format );
	vsnprintf_s( textLiteral, STRINGF_STACK_LOCAL_TEMP_LENGTH, _TRUNCATE, format, variableArgumentList );	
	va_end( variableArgumentList );
	textLiteral[ STRINGF_STACK_LOCAL_TEMP_LENGTH - 1 ] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

	// Add it to the console log
	ConsoleOutputText colorText;
	colorText.m_text		= std::string(textLiteral);
	colorText.m_color		= color;
	colorText.m_threadID	= ::GetCurrentThreadId();

	DevConsole* console = DevConsole::GetInstance();
	GUARANTEE_OR_DIE(console != nullptr, "Error: ConsolePrintf called with no DevConsole initialized.");

	console->AddToMessageQueue(colorText);
}


//-----------------------------------------------------------------------------------------------
// Prints the text to the console in yellow
//
void ConsoleWarningf(char const *format, ...)
{
	// Construct the string
	va_list variableArgumentList;
	va_start( variableArgumentList, format );
	ConsolePrintv(Rgba::ORANGE, format, variableArgumentList);
	va_end(variableArgumentList);
}


//-----------------------------------------------------------------------------------------------
// Prints the text to the console in red
//
void ConsoleErrorf(char const *format, ...)
{
	// Construct the string
	va_list variableArgumentList;
	va_start( variableArgumentList, format );
	ConsolePrintv(Rgba::RED, format, variableArgumentList);
	va_end(variableArgumentList);
}


//-----------------------------------------------------------------------------------------------
// Constructor - initializes UI parameters and registers the window message handler
//
DevConsole::DevConsole()
	: m_isOpen(false)
	, m_showLog(true)
	, m_cursorBlinkTimer(0.f)
	, m_cursorPosition(0)
	, m_historyIndex(0)
	, m_FLChanSecondsPerDance(4.0f)
	, m_wasMouseShown(true)
	, m_wasMouseLocked(false)
{
	Window* theWindow = Window::GetInstance();
	GUARANTEE_OR_DIE(theWindow != nullptr, "Error: DevConsole::InitializeDevConsole called with no Window initialized");

	float windowAspect		= theWindow->GetAspect();
	float consoleOrthoWidth	= windowAspect * Renderer::UI_ORTHO_HEIGHT;

	m_inputFieldBounds = AABB2(Vector2::ZERO, Vector2(consoleOrthoWidth, TEXT_HEIGHT + 2.f * TEXT_PADDING));		// Leave space for padding above and below lines
	m_consoleLogBounds = AABB2(Vector2(0.f, TEXT_HEIGHT + 2.f * TEXT_PADDING), Vector2(consoleOrthoWidth, Renderer::UI_ORTHO_HEIGHT));

	// Set up reference to the window to listen to Windows messages
	theWindow->RegisterHandler(ConsoleMessageHandler);

	SetUpFLChan();
}


//-----------------------------------------------------------------------------------------------
// Destructor (unused)
//
DevConsole::~DevConsole()
{
	Window::GetInstance()->UnregisterHandler(ConsoleMessageHandler);

	delete m_FLChanAnimations;
	m_FLChanAnimations = nullptr;
}


//-----------------------------------------------------------------------------------------------
// Runs the command currently in the input buffer, and flushes the input buffer
//
void DevConsole::HandleEnter()
{
	if (m_inputBuffer.size() > 0)
	{
		// Run the command
		Command::Run(m_inputBuffer);

		// Clear the buffer
		m_inputBuffer.clear();
		m_cursorPosition = 0;
	}
}


//-----------------------------------------------------------------------------------------------
// Removes a character from the input buffer at the cursor location
//
void DevConsole::HandleBackSpace()
{
	if (m_cursorPosition == 0)
	{
		return;
	}

	// Handle boundary cases explicitly for safety
	if (m_cursorPosition == static_cast<int>(m_inputBuffer.size()))
	{
		m_inputBuffer = std::string(m_inputBuffer, 0, m_cursorPosition - 1);
	}
	else if (m_cursorPosition == 1)
	{
		m_inputBuffer = std::string(m_inputBuffer, 1, m_inputBuffer.size() - 1);
	}
	else
	{
		m_inputBuffer = std::string(m_inputBuffer, 0, m_cursorPosition - 1) + std::string(m_inputBuffer, m_cursorPosition);
	}

	m_cursorPosition--;
}


//-----------------------------------------------------------------------------------------------
// Handles the delete key event when the console is on
// deletes the character after the cursor
//
void DevConsole::HandleDelete()
{
	if (m_cursorPosition == static_cast<int>(m_inputBuffer.size()))
	{
		return;
	}

	// Handle boundary cases explicitly for safety
	if (m_cursorPosition == 0)
	{
		m_inputBuffer = std::string(m_inputBuffer, 1, m_inputBuffer.size() - 1);
	}
	else if (m_cursorPosition == static_cast<int>(m_inputBuffer.size() - 1))
	{
		m_inputBuffer = std::string(m_inputBuffer, 0, m_inputBuffer.size() - 1);
	}
	else
	{
		m_inputBuffer = std::string(m_inputBuffer, 0, m_cursorPosition) + std::string(m_inputBuffer, m_cursorPosition + 1);
	}
}


//-----------------------------------------------------------------------------------------------
// Escape key press - clears the input field if non-empty, closes the console otherwise
//
void DevConsole::HandleEscape()
{
	if (m_inputBuffer.size() > 0)
	{
		m_inputBuffer.clear();
		m_cursorPosition = 0;
		m_historyIndex = static_cast<int>(m_commandHistory.size());
	}
	else
	{
		m_isOpen = false;
	}
}


//-----------------------------------------------------------------------------------------------
// Up arrow input - cycle through command history from most recent to least recent
//
void DevConsole::HandleUpArrow()
{
	// Ensure there's history to begin with
	if (m_commandHistory.size() != 0)
	{
		m_historyIndex--;

		// Wrap around end
		if (m_historyIndex < 0)
		{
			m_historyIndex = static_cast<int>(m_commandHistory.size() - 1);
		}

		// Put the history command in the input field
		m_inputBuffer = m_commandHistory[m_historyIndex];
		m_cursorPosition = static_cast<int>(m_inputBuffer.size());
	}
}


//-----------------------------------------------------------------------------------------------
// Down arrow input - cycle through command history from least recent to most recent
//
void DevConsole::HandleDownArrow()
{
	// Ensure there's history to begin with
	if (m_commandHistory.size() != 0)
	{
		// Wrap around end
		m_historyIndex++;
		if (m_historyIndex >= static_cast<int>(m_commandHistory.size()))
		{
			m_historyIndex = 0;
		}

		// Put the history command in the input field
		m_inputBuffer = m_commandHistory[m_historyIndex];
		m_cursorPosition = static_cast<int>(m_inputBuffer.size());
	}
}


//-----------------------------------------------------------------------------------------------
// Adds the given character to the input text field at the current cursor position
//
void DevConsole::AddCharacterToInputBuffer(unsigned char character)
{
	if (m_inputBuffer.size() < MAX_INPUT_BUFFER_SIZE)
	{
		if (m_cursorPosition == static_cast<int>(m_inputBuffer.size()))
		{
			m_inputBuffer += character;
		}
		else
		{
			m_inputBuffer.insert(m_cursorPosition, 1, character);
		}
		m_cursorPosition++;
	}
}


//-----------------------------------------------------------------------------------------------
// Frees the mouse if the dev console is opened, or reverts to previous mouse settings if closed
//
void DevConsole::UpdateMouseCursorSettings()
{
	Mouse& mouse = InputSystem::GetMouse();
	if (m_isOpen)
	{
		m_wasMouseShown = mouse.IsCursorShown();
		m_wasMouseLocked = mouse.IsCursorLocked();
		m_prevMouseMode = mouse.GetCursorMode();

		// Now free the mouse
		mouse.ShowMouseCursor(true);
		mouse.LockCursorToClient(false);
		mouse.SetCursorMode(CURSORMODE_ABSOLUTE);
	}
	else
	{
		// Else the DevConsole was closed, so revert to original mouse state
		mouse.ShowMouseCursor(m_wasMouseShown);
		mouse.LockCursorToClient(m_wasMouseLocked);
		mouse.SetCursorMode(m_prevMouseMode);
	}
}


//-----------------------------------------------------------------------------------------------
// Draws the input field, current input buffer, and the cursor
//
void DevConsole::RenderInputField(Renderer* renderer, BitmapFont* font) const
{
	// Draw the AABB2 background for the field
	Material* uiMaterial = AssetDB::CreateOrGetSharedMaterial("UI");
	renderer->Draw2DQuad(m_inputFieldBounds, AABB2::UNIT_SQUARE_OFFCENTER, INPUT_BOX_COLOR, uiMaterial);

	// Draw the input text buffer
	AABB2 inputTextBounds = m_inputFieldBounds;
	inputTextBounds.Translate(Vector2(TEXT_PADDING * Window::GetInstance()->GetAspect(), 0.f));
	renderer->DrawTextInBox2D(m_inputBuffer, inputTextBounds, Vector2(0.f, 0.5f), TEXT_HEIGHT, TEXT_DRAW_OVERRUN, font, INPUT_TEXT_COLOR);

	// Draw the input cursor if the timer is over 0.5f (timer goes from 0 to 1, blinks once per second)
	if (m_cursorBlinkTimer > 0.5f)
	{
		float cursorOffset = m_cursorPosition * TEXT_HEIGHT * font->GetGlyphAspect();		// Don't apply the window aspect here
		AABB2 cursorBounds = inputTextBounds;
		cursorBounds.Translate(Vector2(cursorOffset - 0.3f * TEXT_HEIGHT * font->GetGlyphAspect(), 0.f));
		renderer->DrawTextInBox2D("|", cursorBounds, Vector2(0.f, 0.5f), m_inputFieldBounds.GetDimensions().y, TEXT_DRAW_OVERRUN, font, Rgba::GRAY, 0.5f);
	}
}


//-----------------------------------------------------------------------------------------------
// Draws the log text (past commands entered and results)
//
void DevConsole::RenderLogWindow(Renderer* renderer, BitmapFont* font) const
{
	// Draw the background
	Material* uiMaterial = AssetDB::CreateOrGetSharedMaterial("UI");
	renderer->Draw2DQuad(m_consoleLogBounds, AABB2::UNIT_SQUARE_OFFCENTER, LOG_BOX_COLOR, uiMaterial);

	//-----Draw the log text-----
	AABB2 currentLogLineBounds = AABB2(Vector2(m_inputFieldBounds.mins.x, m_inputFieldBounds.mins.y + TEXT_HEIGHT), Vector2(m_consoleLogBounds.maxs.x, m_inputFieldBounds.maxs.y + TEXT_HEIGHT));
	currentLogLineBounds.Translate(Vector2(TEXT_PADDING * Window::GetInstance()->GetAspect(), TEXT_PADDING));
	std::vector<ConsoleOutputText>::const_reverse_iterator rItr = m_consoleOutputLog.rbegin();
	for (rItr; rItr != m_consoleOutputLog.rend(); rItr++)
	{
		renderer->DrawTextInBox2D(rItr->m_text, currentLogLineBounds, Vector2::ZERO, TEXT_HEIGHT, TEXT_DRAW_OVERRUN, font, rItr->m_color);
		currentLogLineBounds.Translate(Vector2(0.f, TEXT_HEIGHT + TEXT_PADDING));

		// Check if the next line is off of the screen - break if so
		if (currentLogLineBounds.mins.y > Renderer::UI_ORTHO_HEIGHT)
		{
			break;
		}
	}
}


//-----------------------------------------------------------------------------------------------
// Draws the FPS to the screen
//
void DevConsole::RenderFPS() const
{
	static float fps = 0.f;
	static float timeSinceLastCalculation = 1.f;

	// Define the bounds
	Vector2 topRight = m_consoleLogBounds.maxs - Vector2(Window::GetInstance()->GetAspect() * TEXT_PADDING, TEXT_PADDING);
	Vector2 bottomLeft = topRight - Vector2(Window::GetInstance()->GetAspect() * TEXT_HEIGHT, TEXT_HEIGHT);
	AABB2 fpsBounds = AABB2(bottomLeft, topRight);

	// Calculate the fps
	Renderer* renderer = Renderer::GetInstance();
	float deltaTime = Clock::GetMasterDeltaTime();

	// Update the FPS only twice per second
	if (timeSinceLastCalculation > 0.5f)
	{
		fps = 1.f / deltaTime;
		timeSinceLastCalculation = 0.f;
	}
	else
	{
		timeSinceLastCalculation += deltaTime;
	}

	// Draw to screen
	BitmapFont* font = AssetDB::CreateOrGetBitmapFont("Data/Images/Fonts/ConsoleFont.png");
	Rgba color = Rgba::DARK_GREEN;
	if (fps < 30.f)
	{
		color = Rgba::RED;
	}
	else if (fps < 55.f)
	{
		color = Rgba::YELLOW;
	}
	renderer->DrawTextInBox2D(Stringf("FPS: %.2f", fps), fpsBounds, Vector2(1.0f, 0.f), TEXT_HEIGHT, TEXT_DRAW_OVERRUN, font, color);
}


//-----------------------------------------------------------------------------------------------
// Draws FLChan to the screen
//
void DevConsole::RenderFLChan() const
{
	// Define the small icon bounds
	Vector2 smallTopRight = m_inputFieldBounds.maxs + Vector2(-TEXT_PADDING, 128.f + TEXT_PADDING);
	Vector2 smallBottomLeft = m_inputFieldBounds.maxs + Vector2(-110.f - TEXT_PADDING, TEXT_PADDING);
	AABB2 smallDrawBounds = AABB2(smallBottomLeft, smallTopRight);

	// Define the large bounds
	Vector2 largeTopRight = m_consoleLogBounds.maxs - Vector2(TEXT_PADDING, TEXT_PADDING);
	Vector2 largeBottomLeft = largeTopRight - Vector2(1100.f, 1280.f);
	AABB2 largeDrawBounds = AABB2(largeBottomLeft, largeTopRight);

	// Get the texture
	Renderer* renderer = Renderer::GetInstance();

	// Draw the current sprite in the animation
	AABB2 uvs = m_FLChanAnimations->GetCurrentUVs();
	Material* flChanMat = AssetDB::CreateOrGetSharedMaterial("FLChan");
	renderer->Draw2DQuad(smallDrawBounds, uvs, Rgba(255, 255, 255, 200), flChanMat);
	renderer->Draw2DQuad(largeDrawBounds, uvs, Rgba(255, 255, 255, 100), flChanMat);
}


//-----------------------------------------------------------------------------------------------
// Constructs the necessary SpriteAnimations/Animation definitions to let FLChan dance
//
void DevConsole::SetUpFLChan()
{
	// Make a sprite animation set for the animations
	m_FLChanAnimations = new SpriteAnimSet();

	// Make the sprite sheet
	Texture* texture = AssetDB::CreateOrGetTexture("Data/Images/DevConsole/FLChan.png");
	SpriteSheet* spriteSheet = new SpriteSheet(*texture, IntVector2(8,10));

	// Iterate across all the animations of the sprite sheet
	std::vector<int> currentAnimationIndices;
	for (int animIndex = 0; animIndex < 10; animIndex++)
	{
		// Push back the current animation sprite indices
		currentAnimationIndices.clear();
		int startFrameIndex = 8 * animIndex;
		for (int i = startFrameIndex; i < startFrameIndex + 8; i++)
		{
			currentAnimationIndices.push_back(i);
		}

		// Construct a definition to which an animation can use
		SpriteAnimDef* def = new SpriteAnimDef(*spriteSheet, 8, currentAnimationIndices, Stringf("%i", animIndex), PLAY_MODE_ONCE);
		SpriteAnim* anim = new SpriteAnim(def, true);

		// Add the animation
		m_FLChanAnimations->AddAnimation(anim->GetName(), anim);
	}

	// Set the first animation as the starting one
	m_FLChanAnimations->SetCurrentAnimation(Stringf("%i", 0));
}


//-----------------------------------------------------------------------------------------------
// Update - not sure what it does yet
//
void DevConsole::Update()
{
	float deltaTime = Clock::GetMasterDeltaTime();

	// Update the blink timer
	m_cursorBlinkTimer += deltaTime;
	if (m_cursorBlinkTimer > 1.0f) 
	{
		m_cursorBlinkTimer = 0.f; 
	}

	// Update FLChan's dance animation
	if (m_FLChanAnimations->GetCurrentAnimation()->GetTotalSecondsElapsed() > m_FLChanSecondsPerDance)
	{
		// Choose a random animation and start again
		int randomInt = GetRandomIntLessThan(10);
		m_FLChanAnimations->SetCurrentAnimation(Stringf("%i", randomInt));
	}
	m_FLChanAnimations->Update(deltaTime);

	FlushOutputQueue();
}


//-----------------------------------------------------------------------------------------------
// Draws the console log and input field to the screen
//
void DevConsole::Render() const
{
	Renderer* renderer = Renderer::GetInstance();
	renderer->SetCurrentCamera(renderer->GetUICamera());

	BitmapFont* font = AssetDB::CreateOrGetBitmapFont("Data/Images/Fonts/ConsoleFont.png");

	if (m_showLog)
	{
		RenderFLChan();	// Draw FLChan first, so text appears over her
		RenderLogWindow(renderer, font);
	}

	RenderInputField(renderer, font);

	// Render the RCS widget
	if (RemoteCommandService::GetInstance() != nullptr)
	{
		RemoteCommandService::GetInstance()->Render();
	}

	RenderFPS();
}


//-----------------------------------------------------------------------------------------------
// Sets the flag to render the dev console to screen, and begin listening to input
//
void DevConsole::Open()
{
	m_isOpen = true;
	UpdateMouseCursorSettings();
}


//-----------------------------------------------------------------------------------------------
// Sets the flag to stop rendering the dev console to screen, and stop listening to input
//
void DevConsole::Close()
{
	m_isOpen = false;
	UpdateMouseCursorSettings();
}


//-----------------------------------------------------------------------------------------------
// Returns whether the console is currently open
//
bool DevConsole::IsOpen()
{
	return m_isOpen;
}


//-----------------------------------------------------------------------------------------------
// Adds the given color-text object to the console log window to be drawn each frame
//
void DevConsole::AddToMessageQueue(ConsoleOutputText text)
{
	m_messageQueue.Enqueue(text);
}


//-----------------------------------------------------------------------------------------------
// Returns the console output log
//
std::vector<ConsoleOutputText> DevConsole::GetConsoleLog()
{
	return s_instance->m_consoleOutputLog;
}


//-----------------------------------------------------------------------------------------------
// Clears the console log
//
void DevConsole::ClearConsoleLog()
{
	s_instance->m_consoleOutputLog.clear();
}


//-----------------------------------------------------------------------------------------------
// Adds the command line to the command history
// Allows for duplicates, so long as their not adjacent in the list (like Linux)
// This means the command isn't the most OR least recent command (because of wrap around)
//
void DevConsole::AddCommandLineToHistory(const std::string& commandLine)
{
	// Be sure to check if it's non-empty - .back() and .front() have undefined behavior on empty vectors
	if (s_instance->m_commandHistory.size() == 0)
	{
		s_instance->m_commandHistory.push_back(commandLine);
	}
	else
	{
		bool isMostRecentCommand = s_instance->m_commandHistory.back() == commandLine;

		if (!isMostRecentCommand)
		{
			s_instance->m_commandHistory.push_back(commandLine);
		}
	}

	// Reset the history index
	s_instance->m_historyIndex = static_cast<int>(s_instance->m_commandHistory.size());
}


//-----------------------------------------------------------------------------------------------
// Processes the character key input - used for WM_CHAR messages
//
void DevConsole::ProcessCharacterCode(unsigned char keyCode)
{
	if (m_isOpen)
	{
		switch (keyCode)
		{
		case VK_RETURN:
			HandleEnter();
			break;
		case VK_BACK:
			HandleBackSpace();
			break;
		case VK_ESCAPE:
			HandleEscape();
			break;
		case '~':
		case '`':
			// Case for `~
			// Since we are letting the app control when the console should be opened or closed,
			// ensure we don't do anything here for it (fallthrough intentional)
			break;
		default:
			// Regular input, so add to the input field
			AddCharacterToInputBuffer(keyCode);
			break;
		}
	}
}


//-----------------------------------------------------------------------------------------------
// Takes the keycode of a Keydown event and processes it - used for special keys
// 
//
void DevConsole::ProcessKeydownCode(unsigned char keyCode)
{
	switch (keyCode)
	{
	case VK_DELETE:
		HandleDelete();
		break;
	case VK_RIGHT:
		// Move the cursor to the right, not going pass the very end of the string
		m_cursorPosition++;
		m_cursorPosition = ClampInt(m_cursorPosition, 0, static_cast<int>(m_inputBuffer.size()));
		break;
	case VK_LEFT:
		// Move the cursor to the left, not going beyond right before the first character
		m_cursorPosition--;
		m_cursorPosition = ClampInt(m_cursorPosition, 0, static_cast<int>(m_inputBuffer.size()));
		break;
	case VK_UP:
		HandleUpArrow();
		break;
	case VK_DOWN:
		HandleDownArrow();
		break;
	default:
		// Do nothing here, or else we risk doing duplicate things for a character
		break;
	}
}


//-----------------------------------------------------------------------------------------------
// Writes the command history to a text file
//
void DevConsole::WriteCommandHistoryToFile()
{
	File file;

	bool success = file.Open("Data/Logs/Command_History.log", "w");

	if (!success)
	{
		LogTaggedPrintf("DEV_CONSOLE", "Error: Couldn't open the log history file for write");
		return;
	}

	int numCommandsInHistory = (int) m_commandHistory.size();
	int lineCountToWrite = (numCommandsInHistory < MAX_HISTORY_WRITE_COUNT ? numCommandsInHistory : MAX_HISTORY_WRITE_COUNT);

	for (int i = 0; i < lineCountToWrite; ++i)
	{
		int index = (int)m_commandHistory.size() - lineCountToWrite + i;
		std::string toWrite = m_commandHistory[index] + '\n';
		file.Write(toWrite.c_str(), toWrite.size());
	}

	//file.Flush();
	file.Close();
}


//-----------------------------------------------------------------------------------------------
// Loads the command history from the history log and puts it in the DevConsole's history to
// be reused
//
void DevConsole::LoadCommandHistoryFromFile()
{
	File file;

	bool success = file.Open("Data/Logs/Command_History.log", "r");

	if (!success)
	{
		LogTaggedPrintf("DEV_CONSOLE", "Error: Couldn't open the log history file for read");
		return;
	}

	file.LoadFileToMemory();

	while (!file.IsAtEndOfFile())
	{
		std::string line;
		file.GetNextLine(line);

		if (line.size() > 0)
		{
			m_commandHistory.push_back(line);
		}
	}

	file.Close();
}


//-----------------------------------------------------------------------------------------------
// Constructs the singleton console instance
//
void DevConsole::Initialize()
{
	s_instance = new DevConsole();

	// Register commands related to the DevConsole
	Command::Register("echo",							"Prints the given text to screen with the given color",	Command_Echo);
	Command::Register("save_log",						"Writes the output log to file",						Command_SaveLog);
	Command::Register("clear",							"Clears the output log",								Command_Clear);
	Command::Register("hide_log",						"Disables rendering of the log window and text",		Command_HideLog);
	Command::Register("show_log",						"Enables rendering of the log window and text",			Command_ShowLog);
	Command::Register("hook_console_to_logsystem",		"Enables rendering of the log window and text",			Command_HookToLogSystem);
	Command::Register("run_batch",						"Runs a batch job file",								Command_RunBatchFile);

	// Load the DevConsole History
	s_instance->LoadCommandHistoryFromFile();

	// Load the font here to prevent hitch on first log open
	AssetDB::CreateOrGetBitmapFont("Data/Images/Fonts/ConsoleFont.png");
}


//-----------------------------------------------------------------------------------------------
// Destroys the console singleton instance
//
void DevConsole::Shutdown()
{
	s_instance->WriteCommandHistoryToFile();

	delete s_instance;
	s_instance = nullptr;
}


//-----------------------------------------------------------------------------------------------
// Static accessor to check if the console is open
//
bool DevConsole::IsDevConsoleOpen()
{
	if (s_instance == nullptr)
	{
		return false;
	}
	else
	{
		return s_instance->m_isOpen;
	}
}


//-----------------------------------------------------------------------------------------------
// Returns a pointer to the singleton instance
//
DevConsole* DevConsole::GetInstance()
{
	return s_instance;
}


//-----------------------------------------------------------------------------------------------
// Toggles the open/close state of the console
//
void DevConsole::ToggleConsole()
{
	s_instance->m_isOpen = !s_instance->m_isOpen;
	s_instance->UpdateMouseCursorSettings();
}


//-----------------------------------------------------------------------------------------------
// Shows the log window and text
//
void DevConsole::ShowLogWindow()
{
	s_instance->m_showLog = true;
}


//-----------------------------------------------------------------------------------------------
// Adds a console callback for other systems to receive text printed to the console
//
void DevConsole::AddConsoleHook(DevConsole_cb callback, void* args/*= nullptr*/)
{
	s_instance->m_consoleHooks.push_back(DevConsoleHook_t(callback, args));
}


//-----------------------------------------------------------------------------------------------
// Removes the given callback from the list of DevConsole hooks
//
void DevConsole::RemoveConsoleHook(DevConsole_cb callback)
{
	for (int i = 0; i < (int)s_instance->m_consoleHooks.size(); ++i)
	{
		if (s_instance->m_consoleHooks[i].callback == callback)
		{
			s_instance->m_consoleHooks.erase(s_instance->m_consoleHooks.begin() + i);
			break;
		}
	}
}


//-----------------------------------------------------------------------------------------------
// Flushes the command queue immediately, for commands that need their response now
//
void DevConsole::FlushOutputQueue()
{
	ConsoleOutputText text;
	while (m_messageQueue.Dequeue(text)) // returns false when empty
	{
		// Call the callbacks on the text
		for (int i = 0; i < (int)m_consoleHooks.size(); ++i)
		{
			m_consoleHooks[i].callback(text, m_consoleHooks[i].args);
		}

		// Push the text to the output log
		m_consoleOutputLog.push_back(text);
	}
}


//-----------------------------------------------------------------------------------------------
// Hides the log window and text
//
void DevConsole::HideLogWindow()
{
	s_instance->m_showLog = false;
}


//--------------------C Functions--------------------

//-----------------------------------------------------------------------------------------------
// LogSystem Callback for outputting the log messages to the DevConsole
//
static void WriteToDevConsole(LogMessage_t log, void* args)
{
	UNUSED(args);

	// No need to check for nullptr, as the callback is registered in DevConsole::Initialize()
	ConsoleOutputText outputText;
	outputText.m_text = Stringf("%s: %s\n", log.tag.c_str(), log.message.c_str());

	DevConsole::GetInstance()->AddToMessageQueue(outputText);
}


//-----------------------------------------------------------------------------------------------
// Handler that listens for Windows messages on the current window
//
bool ConsoleMessageHandler(unsigned int msg, size_t wparam, size_t lparam)
{
	if (!DevConsole::IsDevConsoleOpen())
	{
		return true;
	}

	UNUSED(lparam);
	// Get the keycode of the key pressed
	unsigned char keyCode = (unsigned char) wparam;

	switch( msg )
	{
	case WM_CHAR:		
	{
		DevConsole::GetInstance()->ProcessCharacterCode(keyCode);
		return false; // "Consumes" this message (tells Windows "okay, we handled it")
	}
	// For handling arrow keys and delete
	case WM_KEYDOWN:
	{
		DevConsole::GetInstance()->ProcessKeydownCode(keyCode);
		return false;
	}
	}

	return true;
}


//////////////////////////////////////////////////////////////////////////
// Console Commands
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------------------------
// Prints a given string to the console with an optional color
// Usage: echo <text_in_quotes> color
//
void Command_Echo(Command& cmd)
{
	std::string textToEcho;
	bool textSpecified = cmd.GetParam("t", textToEcho);
	
	std::string colorText;
	bool colorSpecified	= cmd.GetParam("c", colorText);

	if (textSpecified)
	{
		if (colorSpecified)
		{
			Rgba color;
			bool successful = color.SetFromText(colorText.c_str());
			if (successful)
			{
				ConsolePrintf(color, textToEcho.c_str());
			}
			else
			{
				ConsoleErrorf("Usage: echo -c <color> -t <text>");
			}
		}
		else
		{
			// No color specified, so just print in white
			ConsolePrintf(textToEcho.c_str());
		}
	}
	else
	{
		ConsoleWarningf("No text specified to echo");
		ConsoleErrorf("Usage: echo -c <color> -t <text>");
	}
}


//-----------------------------------------------------------------------------------------------
// Writes the output of the console log to file (will always write to /Data/Logs directory)
// Usage: save_log <optional filename>
//
void Command_SaveLog(Command& cmd)
{
	// Assemble the log as a character array
	std::vector<ConsoleOutputText> consoleLog = DevConsole::GetConsoleLog();
	std::string result;
	for (unsigned int lineNumber = 0; lineNumber < consoleLog.size(); lineNumber++)
	{
		result += consoleLog[lineNumber].m_text + "\n";
	}

	// Use the file name provided, or the default one if not
	std::string filename;
	cmd.GetParam("f", filename, &DEFAULT_LOG_FILENAME);
	
	std::string localLogFilePath = LOCAL_LOGS_DIRECTORY + filename;

	// Check to see if the directory exists (will make it if it doesn't exist, do nothing otherwise)
	CreateDirectoryA("Data/Logs", NULL);

	// Attempt to write to the file
	bool writeSucceeded = FileWriteFromBuffer(localLogFilePath.c_str(), result.c_str(), static_cast<int>(result.size()));

	if (!writeSucceeded)
	{
		ConsoleErrorf("INVALID FILENAME: \"%s\"", filename.c_str());
	}
}


//-----------------------------------------------------------------------------------------------
// Clears the console output log
// Usage: clear
//
void Command_Clear(Command& cmd)
{
	UNUSED(cmd);
	DevConsole::ClearConsoleLog();
}


//-----------------------------------------------------------------------------------------------
// Hides the console log window
// Usage: hide_log
//
void Command_HideLog(Command& cmd)
{
	UNUSED(cmd);
	DevConsole::HideLogWindow();
	ConsolePrintf("Log window hidden.");
}


//-----------------------------------------------------------------------------------------------
// Shows the console log window
// Usage: show_log
//
void Command_ShowLog(Command& cmd)
{
	UNUSED(cmd);
	DevConsole::ShowLogWindow();
	ConsolePrintf("Log window shown.");
}


//-----------------------------------------------------------------------------------------------
// Shows the console log window
// Usage: hook_console_to_logsystem
//
void Command_HookToLogSystem(Command& cmd)
{
	UNUSED(cmd);

	LogSystem::AddCallback("DevConsole Writer", WriteToDevConsole, nullptr);
	ConsolePrintf(Rgba::GREEN, "LogSystem now writing to DevConsole output");
}


//-----------------------------------------------------------------------------------------------
// Loads a text file and runs each line as a command
//
void Command_RunBatchFile(Command& cmd)
{
	std::string filename;
	bool filenameSpecifed = cmd.GetParam("f", filename);

	if (!filenameSpecifed)
	{
		ConsoleErrorf("Must provide a file name");
		return;
	}

	File file;
	bool fileOpened = file.Open(filename.c_str(), "r");

	if (!fileOpened)
	{
		ConsoleErrorf("Couldn't open file %s", filename.c_str());
		return;
	}

	ConsolePrintf(Rgba::GREEN, "-----Running Batch Job-----");

	file.LoadFileToMemory();

	int numCommandsSuccess = 0;
	while (!file.IsAtEndOfFile())
	{
		std::string line;
		file.GetNextLine(line);

		if (line.size() > 0)
		{
			bool commandSucceeded = Command::Run(line);
			if (commandSucceeded)
			{
				++numCommandsSuccess;
			}
		}
	}

	file.Close();

	ConsolePrintf(Rgba::GREEN, "-----Batch job finished, %i commands executed successfully-----", numCommandsSuccess);
}
