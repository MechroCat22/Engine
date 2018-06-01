/************************************************************************/
/* Project: Game Engine
/* File: InputSystem.cpp
/* Author: Andrew Chase
/* Date: September 13th, 2017
/* Bugs: None
/* Description: Implementation of the InputSystem class
/************************************************************************/
#include "Engine/Input/InputSystem.hpp"
#define WIN32_LEAN_AND_MEAN			// Always #define this before #including <windows.h>
#include <windows.h>
#include "Engine/Core/EngineCommon.hpp"


// Singleton instance
InputSystem* InputSystem::s_instance = nullptr;

// Mapping InputSystem constants to Windows macros
const unsigned char	InputSystem::KEYBOARD_ESCAPE		= VK_ESCAPE;
const unsigned char InputSystem::KEYBOARD_SPACEBAR		= VK_SPACE;
const unsigned char	InputSystem::KEYBOARD_F1			= VK_F1;
const unsigned char	InputSystem::KEYBOARD_F2			= VK_F2;
const unsigned char	InputSystem::KEYBOARD_F3			= VK_F3;
const unsigned char	InputSystem::KEYBOARD_F4			= VK_F4;
const unsigned char	InputSystem::KEYBOARD_F5			= VK_F5;
const unsigned char	InputSystem::KEYBOARD_F6			= VK_F6;
const unsigned char	InputSystem::KEYBOARD_F7			= VK_F7;
const unsigned char	InputSystem::KEYBOARD_F8			= VK_F8;
const unsigned char	InputSystem::KEYBOARD_F9			= VK_F9;
const unsigned char	InputSystem::KEYBOARD_F10			= VK_F10;
const unsigned char	InputSystem::KEYBOARD_LEFT_ARROW	= VK_LEFT;
const unsigned char	InputSystem::KEYBOARD_UP_ARROW		= VK_UP;
const unsigned char	InputSystem::KEYBOARD_DOWN_ARROW	= VK_DOWN;
const unsigned char	InputSystem::KEYBOARD_RIGHT_ARROW	= VK_RIGHT;
const unsigned char InputSystem::KEYBOARD_TILDE			= VK_OEM_3;

//-----------------------------------------------------------------------------------------------
// Constructor - Creates 4 XboxControllers with their ID's equal to their index in the array
//
InputSystem::InputSystem()
{
	for (int i = 0; i < NUM_CONTROLLERS; i++)
	{
		m_xboxControllers[i] = XboxController(i);
	}
}


//-----------------------------------------------------------------------------------------------
// Destructor - made private (use InputSystem::Shutdown() instead)
//
InputSystem::~InputSystem()
{
}


//-----------------------------------------------------------------------------------------------
// Processes all Windows messages (WM_xxx) for this app that have queued up since last frame.
// For each message in the queue, our WindowsMessageHandlingProcedure (or "WinProc") function
//	is called, telling us what happened (key up/down, minimized/restored, gained/lost focus, etc.)
//
void RunMessagePump()
{
	MSG queuedMessage;
	for( ;; )
	{
		const BOOL wasMessagePresent = PeekMessage( &queuedMessage, NULL, 0, 0, PM_REMOVE );
		if( !wasMessagePresent )
		{
			break;
		}

		TranslateMessage( &queuedMessage );
		DispatchMessage( &queuedMessage ); // This tells Windows to call our "WindowsMessageHandlingProcedure" function
	}
}


//-----------------------------------------------------------------------------------------------
// Creates the InputSystem singleton instance
//
void InputSystem::Initialize()
{
	GUARANTEE_OR_DIE(s_instance == nullptr, "Error: InputSystem::Initialize() called with an existing instance.");
	s_instance = new InputSystem();
}


//-----------------------------------------------------------------------------------------------
// Deletes the InputSystem singleton instance
//
void InputSystem::Shutdown()
{
	if (s_instance != nullptr)
	{
		delete s_instance;
		s_instance = nullptr;
	}
}


//-----------------------------------------------------------------------------------------------
// Resets the 'just' fields of all key states and setup up to receive messages from windows
//
void InputSystem::BeginFrame()
{
	ResetJustKeyStates();
	UpdateControllers();
	RunMessagePump();
}


//-----------------------------------------------------------------------------------------------
// Handle End-of-frame tasks for the input system
// 
void InputSystem::EndFrame()
{
}


//-----------------------------------------------------------------------------------------------
// Called whenever a key (keyCode) was pressed, and updates its KeyButtonState
//
void InputSystem::OnKeyPressed(unsigned char keyCode)
{
	if (!m_keyStates[keyCode].m_isPressed)
	{
		m_keyStates[keyCode].m_wasJustPressed = true;
	}

	m_keyStates[keyCode].m_isPressed = true;
}


//-----------------------------------------------------------------------------------------------
// Called whenever a key (keyCode) was released, and updates its KeyButtonState
//
void InputSystem::OnKeyReleased(unsigned char keyCode)
{
	m_keyStates[keyCode].m_isPressed = false;
	
	m_keyStates[keyCode].m_wasJustReleased = true;
}


//-----------------------------------------------------------------------------------------------
// Returns true if the key 'keyCode' is currently pressed this frame, false otherwise
//
bool InputSystem::IsKeyPressed(unsigned char keyCode) const
{
	return m_keyStates[keyCode].m_isPressed;
}


//-----------------------------------------------------------------------------------------------
// Returns true if the key 'keyCode' was just pressed this frame, and false otherwise
//
bool InputSystem::WasKeyJustPressed(unsigned char keyCode) const
{
	return m_keyStates[keyCode].m_wasJustPressed;
}


//-----------------------------------------------------------------------------------------------
// Returns true if the key 'keyCode' was currently released this frame, and false otherwise
bool InputSystem::WasKeyJustReleased(unsigned char keyCode) const
{
	return m_keyStates[keyCode].m_wasJustReleased;
}


//-----------------------------------------------------------------------------------------------
// Returns the controller given by the passed index
//
XboxController& InputSystem::GetController(int controllerNumber)
{
	return m_xboxControllers[controllerNumber];
}


//-----------------------------------------------------------------------------------------------
// Returns the InputSystem singleton instance
//
InputSystem* InputSystem::GetInstance()
{
	return s_instance;
}


//-----------------------------------------------------------------------------------------------
// Returns the controller currently used by player one
//
XboxController& InputSystem::GetPlayerOneController()
{
	return s_instance->m_xboxControllers[0];
}


//-----------------------------------------------------------------------------------------------
// Sets the 'just' states of all KeyButtonStates to false, so that they may be updated correctly
// next frame
//
void InputSystem::ResetJustKeyStates()
{
	for (int i = 0; i < NUM_KEYS; i++)
	{
		m_keyStates[i].m_wasJustPressed = false;
		m_keyStates[i].m_wasJustReleased = false;
	}
}


//-----------------------------------------------------------------------------------------------
// Fetches the last-frame input for each controller from XInput
//
void InputSystem::UpdateControllers()
{
	for (int i = 0; i < NUM_CONTROLLERS; i++)
	{
		m_xboxControllers[i].Update();
	}
}


