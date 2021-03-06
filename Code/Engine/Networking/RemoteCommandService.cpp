#include "Engine/Core/LogSystem.hpp"
/************************************************************************/
/* File: RemoteCommandService.cpp
/* Author: Andrew Chase
/* Date: September 20th, 2018
/* Description: Implementation of the RemoteCommandService class
/************************************************************************/
#include "Engine/Assets/AssetDB.hpp"
#include "Engine/Core/Time/Time.hpp"
#include "Engine/Core/Time/Stopwatch.hpp"
#include "Engine/Networking/BytePacker.hpp"
#include "Engine/Rendering/Core/Renderer.hpp"
#include "Engine/Core/Utility/StringUtils.hpp"
#include "Engine/Core/Threading/Threading.hpp"
#include "Engine/Rendering/Materials/Material.hpp"
#include "Engine/Core/DeveloperConsole/Command.hpp"
#include "Engine/Networking/RemoteCommandService.hpp"
#include "Engine/Core/DeveloperConsole/DevConsole.hpp"

#define DEFAULT_SERVICE_PORT 29283
#define MAX_CLIENTS 32
#define DELAY_TIME 5

// For checking thread ID's
#if defined( _WIN32 )
#define PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

// Commands for the RCS
void Command_RemoteCommand(Command& cmd);
void Command_RemoteCommandBroadcast(Command& cmd);
void Command_RemoteCommandAll(Command& cmd);
void Command_RemoteJoin(Command& cmd);
void Command_RemoteHost(Command& cmd);
void Command_CloneProcess(Command& cmd);

// For sending echo responses
void SendEchoResponse(ConsoleOutputText text, void* args);

// Singleton instance
RemoteCommandService* RemoteCommandService::s_instance = nullptr;


//-----------------------------------------------------------------------------------------------
// Starts up the RCS
//
void RemoteCommandService::Initialize()
{
	s_instance = new RemoteCommandService();
	InitializeConsoleCommands();
}


//-----------------------------------------------------------------------------------------------
// Shuts down and deletes the RCS
//
void RemoteCommandService::Shutdown()
{
	delete s_instance;
	s_instance = nullptr;
}


//-----------------------------------------------------------------------------------------------
// Update loop
//
void RemoteCommandService::BeginFrame()
{
	switch (m_state)
	{
	case STATE_INITIAL:				Update_Initial();			break;
	case STATE_TRYTOJOINLOCAL:		Update_TryToJoinLocal();	break;
	case STATE_TRYTOJOINADDRESS:	Update_TryToJoinAddress();	break;
	case STATE_TRYTOHOST:			Update_TryToHost();			break;
	case STATE_DELAY:				Update_Delay();				break;
	case STATE_HOST:				Update_Host();				break;
	case STATE_CLIENT:				Update_Client();			break;
	default:
		break;
	}
}


//-----------------------------------------------------------------------------------------------
// Renders the RCS widget to the screen
// *Should only be called by DevConsole::Render()
//
void RemoteCommandService::Render()
{
	// Render the box and border
	Renderer* renderer = Renderer::GetInstance();

	std::string headingText = Stringf("Remote Connection - [");
	std::string stateText;

	switch (m_state)
	{
	case STATE_INITIAL:
		stateText = "INITIAL]";
		break;
	case STATE_TRYTOJOINLOCAL:
		stateText = "JOINING LOCAL]";
		break;
	case STATE_TRYTOJOINADDRESS:
		stateText = "JOINING ADDRESS]";
		break;
	case STATE_TRYTOHOST:
		stateText = "TRYING TO HOST]";
		break;
	case STATE_DELAY:
		stateText = "DELAY]";
		break;
	case STATE_HOST:
		stateText = "HOST]";
		break;
	case STATE_CLIENT:
		stateText = "CLIENT]";
		break;
	default:
		break;
	}

	Vector2 alignment = Vector2(1.0f, 0.f);
	headingText += stateText;
	BitmapFont* font = AssetDB::GetBitmapFont("Data/Images/Fonts/ConsoleFont.png");
	AABB2 drawBounds = m_bounds;
	renderer->DrawTextInBox2D(headingText, drawBounds, alignment, m_textHeight, TEXT_DRAW_SHRINK_TO_FIT, font);
	drawBounds.Translate(Vector2(0.f, -m_textHeight));

	std::string hostAddress;
	if (m_state == STATE_CLIENT)
	{
		hostAddress = m_connections[0]->GetNetAddress().ToString();
	}
	else if (m_state == STATE_HOST)
	{
		hostAddress = m_hostListenSocket.GetNetAddress().ToString();
	}

	renderer->DrawTextInBox2D(Stringf("Host Address: %s", hostAddress.c_str()), drawBounds, alignment, m_textHeight, TEXT_DRAW_SHRINK_TO_FIT, font);
	drawBounds.Translate(Vector2(0.f, -m_textHeight));

	// Connections
	int connectionCount = (int)m_connections.size();

	if (connectionCount > 0)
	{
		renderer->DrawTextInBox2D(Stringf("Connections: %i", connectionCount), drawBounds, alignment, m_textHeight, TEXT_DRAW_SHRINK_TO_FIT, font, Rgba::DARK_GREEN);
		drawBounds.Translate(Vector2(0.f, -m_textHeight));

		for (int i = 0; i < connectionCount; ++i)
		{
			std::string address = m_connections[i]->GetNetAddress().ToString();
			std::string toPrint = Stringf("[%i]: %s", i, address.c_str());

			renderer->DrawTextInBox2D(toPrint, drawBounds, alignment, m_textHeight, TEXT_DRAW_SHRINK_TO_FIT, font);
			drawBounds.Translate(Vector2(0.f, -m_textHeight));
		}
	}
	else
	{
		renderer->DrawTextInBox2D("No connections", drawBounds, alignment, m_textHeight, TEXT_DRAW_SHRINK_TO_FIT, font, Rgba::RED);
	}
}


//-----------------------------------------------------------------------------------------------
// Returns the singleton instance
//
RemoteCommandService* RemoteCommandService::GetInstance()
{
	return s_instance;
}


//-----------------------------------------------------------------------------------------------
// Sends the given message to the connection at index, flagging if it is an echo string
//
bool RemoteCommandService::Send(const std::string& message, int connectionIndex, bool isEcho)
{
	if (connectionIndex >= (int)s_instance->m_connections.size() || message.size() == 0)
	{
		return false;
	}

	// Make the message
	BytePacker sendPack(BIG_ENDIAN);

	sendPack.WriteBytes(1, &isEcho);
	sendPack.WriteString(message);

	uint16_t messageLength = (uint16_t)sendPack.GetWrittenByteCount();
	uint16_t msgBigEndian = messageLength;
	ToEndianness(2, &msgBigEndian, BIG_ENDIAN);

	s_instance->m_connections[connectionIndex]->Send(&msgBigEndian, 2);
	int amountSent = s_instance->m_connections[connectionIndex]->Send(sendPack.GetBuffer(), messageLength);

	if (amountSent > 0)
	{
		LogTaggedPrintf("RCS", "Sent message \"%s\" to connection index %i", message.c_str(), connectionIndex);
	}
	else
	{
		LogTaggedPrintf("RCS", "Failed to send message \"%s\" to connection index %i", message.c_str(), connectionIndex);
	}

	return (amountSent > 0);
}


//-----------------------------------------------------------------------------------------------
// Sets the join request address to signal if the RCS should join an address
//
void RemoteCommandService::Join(const std::string& address)
{
	s_instance->m_joinRequestAddress = address;
}


//-----------------------------------------------------------------------------------------------
// Attempts to start an RCS session using the given port
//
void RemoteCommandService::Host(unsigned short port)
{
	// Reset all state
	s_instance->CloseAllConnections();
	s_instance->m_joinRequestAddress.clear();

	// Set the listen port
	s_instance->m_hostListenPort = port;

	// Go into try to host state next
	s_instance->m_state = STATE_TRYTOHOST;
}


//-----------------------------------------------------------------------------------------------
// Returns the number of active connections in this RCS instance
//
int RemoteCommandService::GetConnectionCount()
{
	return (int)s_instance->m_connections.size();
}


//-----------------------------------------------------------------------------------------------
// Constructor
//
RemoteCommandService::RemoteCommandService()
	: m_state(STATE_INITIAL)
	, m_hostListenPort(DEFAULT_SERVICE_PORT)
{
	m_hostListenSocket.SetBlocking(false);
	m_delayTimer = new Stopwatch(nullptr);

	InitializeUILayout();

	LogTaggedPrintf("RCS", "Entered Initial State");
}


//-----------------------------------------------------------------------------------------------
// Initializes the UI Layout of the RCS widget on the DevConsole
//
void RemoteCommandService::InitializeUILayout()
{
	m_borderThickness = 10.f;
	m_textHeight = 20.f;
	m_textPadding = 3.f;

	AABB2 uiBounds = Renderer::GetUIBounds();
	m_bounds = AABB2(Vector2(0.65f * uiBounds.maxs.x, 0.f), uiBounds.maxs - Vector2(0.f, m_textHeight));
	m_bounds.AddPaddingToSides(-m_textPadding, -m_textPadding);
}


//-----------------------------------------------------------------------------------------------
// Registers the console commands for the RCS into the command system
//
void RemoteCommandService::InitializeConsoleCommands()
{
	Command::Register("rc", "Sends a command to a remote connection to execute.", Command_RemoteCommand);
	Command::Register("rcb", "Broadcasts a command to all remote connections.", Command_RemoteCommandBroadcast);
	Command::Register("rca", "Sends a command to all remote connections AND executes it locally.", Command_RemoteCommandAll);

	Command::Register("rc_join", "Tells the RCS to connect to the host at the supplied address.", Command_RemoteJoin);
	Command::Register("rc_host", "Tries to host an RCS with the given port.", Command_RemoteHost);
	Command::Register("clone_process", "Clones the current process up to the number specified", Command_CloneProcess);
}


//-----------------------------------------------------------------------------------------------
// Destructor
//
RemoteCommandService::~RemoteCommandService()
{
	for (int i = 0; i < (int)m_connections.size(); ++i)
	{
		m_connections[i]->Close();
		delete m_connections[i];
	}

	m_connections.clear();

	for (int j = 0; j < (int)m_buffers.size(); ++j)
	{
		delete m_buffers[j];
	}

	m_buffers.clear();

	m_hostListenSocket.Close();
}


//-----------------------------------------------------------------------------------------------
// Update, for the initial state
//
void RemoteCommandService::Update_Initial()
{
	CloseAllConnections();

	if (m_joinRequestAddress.size() > 0)
	{
		LogTaggedPrintf("RCS", "RCS is trying to join address %s...", m_joinRequestAddress.c_str());
		m_state = STATE_TRYTOJOINADDRESS;
	}
	else
	{
		m_state = STATE_TRYTOJOINLOCAL;
	}
}


//-----------------------------------------------------------------------------------------------
// Update, for the Join Attempt state
//
void RemoteCommandService::Update_TryToJoinLocal()
{
	NetAddress_t localAddress;
	bool localAddressFound = NetAddress_t::GetLocalAddress(&localAddress, DEFAULT_SERVICE_PORT, false);

	if (!localAddressFound)
	{
		m_state = STATE_INITIAL;
		LogTaggedPrintf("RCS", "Entered Initial State");
		return;
	}

	TCPSocket* joinSocket = new TCPSocket();
	joinSocket->SetBlocking(true);

	bool connected = joinSocket->Connect(localAddress);

	if (!connected)
	{
		m_state = STATE_TRYTOHOST;
		LogTaggedPrintf("RCS", "RCS trying to host");
		return;
	}

	// Connected successfully, store off socket and go to client state
	joinSocket->SetBlocking(false);
	m_connections.push_back(joinSocket);
	m_buffers.push_back(new BytePacker(BIG_ENDIAN));
	m_state = STATE_CLIENT;

	LogTaggedPrintf("RCS", "RCS is a host");
}


//-----------------------------------------------------------------------------------------------
// Update, for the Join Address state
//
void RemoteCommandService::Update_TryToJoinAddress()
{
	NetAddress_t netAddress(m_joinRequestAddress.c_str());

	TCPSocket* joinSocket = new TCPSocket();
	joinSocket->SetBlocking(true);
	bool connected = joinSocket->Connect(netAddress);

	if (!connected)
	{
		delete joinSocket;
		m_state = STATE_INITIAL;
		LogTaggedPrintf("RCS", "RCS entered initial state");

		m_joinRequestAddress.clear();

		return;
	}

	joinSocket->SetBlocking(false);
	m_connections.push_back(joinSocket);
	m_buffers.push_back(new BytePacker(BIG_ENDIAN));
	m_state = STATE_CLIENT;

	LogTaggedPrintf("RCS", "RCS successfully joined address %s", m_joinRequestAddress.c_str());

	// Always clear the join request
	m_joinRequestAddress.clear();
}


//-----------------------------------------------------------------------------------------------
// Update, for the Try to Host state
//
void RemoteCommandService::Update_TryToHost()
{
	bool isListening = m_hostListenSocket.Listen(m_hostListenPort, MAX_CLIENTS);

	if (!isListening)
	{
		m_delayTimer->SetInterval(DELAY_TIME);
		m_state = STATE_DELAY;

		LogTaggedPrintf("RCS", "RCS failed to host, moving to delay");
	}
	else
	{
		m_state = STATE_HOST;

		LogTaggedPrintf("RCS", "RCS is now hosting");
	}
}


//-----------------------------------------------------------------------------------------------
// Update, for the Delay state
//
void RemoteCommandService::Update_Delay()
{
	if (m_delayTimer->HasIntervalElapsed())
	{
		m_delayTimer->Reset();
		m_state = STATE_INITIAL;
	
		LogTaggedPrintf("RCS", "Entered Initial State");
	}
}


//-----------------------------------------------------------------------------------------------
// Update, for the Host state
//
void RemoteCommandService::Update_Host()
{
	// Check if a join request is available
	if (m_joinRequestAddress.size() > 0)
	{
		m_state = STATE_INITIAL;
		LogTaggedPrintf("RCS", "Entered Initial State");
	}

	CheckForNewConnections();
	ProcessAllConnections();
	CleanUpClosedConnections();
}


//-----------------------------------------------------------------------------------------------
// Update, for the Client state
//
void RemoteCommandService::Update_Client()
{
	// Check if a join request is available
	if (m_joinRequestAddress.size() > 0)
	{
		m_state = STATE_INITIAL;
		LogTaggedPrintf("RCS", "Entered Initial State");
		return;
	}

	ProcessAllConnections();
	CleanUpClosedConnections();

	// No longer connected to the host, so we reset
	if (m_connections.size() == 0)
	{
		m_state = STATE_INITIAL;
		LogTaggedPrintf("RCS", "RCS lost connection to host, re-entering initial state");
	}
}


//-----------------------------------------------------------------------------------------------
// Checks if there are any new connections connecting to the listen socket
//
void RemoteCommandService::CheckForNewConnections()
{
	m_hostListenSocket.SetBlocking(false);
	TCPSocket* socket = m_hostListenSocket.Accept();

	if (socket != nullptr)
	{
		m_connections.push_back(socket);
		m_buffers.push_back(new BytePacker(BIG_ENDIAN));
	}
}


//-----------------------------------------------------------------------------------------------
// Processes all connections, used in the update loop for Clients and Hosts
//
void RemoteCommandService::ProcessAllConnections()
{
	for (int connectionIndex = 0; connectionIndex < (int)m_connections.size(); ++connectionIndex)
	{
		ProcessConnection(connectionIndex);
	}
}


//-----------------------------------------------------------------------------------------------
// Processes the given connection by calling receive on the socket
//
void RemoteCommandService::ProcessConnection(int connectionIndex)
{
	TCPSocket* connection = m_connections[connectionIndex];
	BytePacker *buffer = m_buffers[connectionIndex];

	buffer->Reserve(2);

	// Need to get the message length still
	if (buffer->GetWrittenByteCount() < 2)
	{
		int amountReceived = connection->Receive(buffer->GetWriteHead(), 2 - buffer->GetWrittenByteCount());
		buffer->AdvanceWriteHead(amountReceived);
	}

	bool isReadyToProcess = false;
	if (buffer->GetWrittenByteCount() >= 2)
	{
		uint16_t len;
		buffer->Peek(&len, sizeof(len));

		// Reserve enough for the size 
		buffer->Reserve(len + 2U);

		size_t bytesNeeded = len + 2U - buffer->GetWrittenByteCount();

		// If we still need more of the message
		if (bytesNeeded > 0)
		{
			size_t read = connection->Receive(buffer->GetWriteHead(), bytesNeeded);
			buffer->AdvanceWriteHead(read);

			bytesNeeded -= read;
		}

		isReadyToProcess = (bytesNeeded == 0);
	}

	if (isReadyToProcess)
	{
		buffer->AdvanceReadHead(2U);
		ProcessMessage(connectionIndex);

		// Clean up to be used
		buffer->ResetWrite();
	}
}


//-----------------------------------------------------------------------------------------------
// Processes the message fully received on the given connection index
//
void RemoteCommandService::ProcessMessage(int connectionIndex)
{
	TCPSocket* connection = m_connections[connectionIndex];
	BytePacker *buffer = m_buffers[connectionIndex];

	bool isEcho;
	buffer->ReadBytes(&isEcho, 1);;

	std::string str;
	if (buffer->ReadString(str))
	{
		// Succeeded in getting a command string
		if (isEcho)
		{
			// Print to console, with info
			NetAddress_t address = connection->GetNetAddress();
			ConsolePrintf("[%s]: %s", address.ToString().c_str(), str.c_str());
		}
		else
		{
			// Run the command, sending back the echo response
			DevConsole::AddConsoleHook(SendEchoResponse, &connectionIndex);
			Command::Run(str);
			DevConsole::RemoveConsoleHook(SendEchoResponse);
		}
	}
}


//-----------------------------------------------------------------------------------------------
// Checks for closed connections and removes them from the list
//
void RemoteCommandService::CleanUpClosedConnections()
{
	for (int i = (int) m_connections.size() - 1; i >= 0; --i)
	{
		if (m_connections[i]->IsClosed())
		{
			m_connections.erase(m_connections.begin() + i);
			
			// Free up the byte packer for this connection
			delete m_buffers[i];
			m_buffers.erase(m_buffers.begin() + i);
		}
	}
}


//-----------------------------------------------------------------------------------------------
// Closes all connections on the RCS
//
void RemoteCommandService::CloseAllConnections()
{
	// Ensure we're no longer hosting
	m_hostListenSocket.Close();

	// Close all existing connections
	for (int index = 0; index < (int)m_connections.size(); ++index)
	{
		m_connections[index]->Close();
		delete m_connections[index];

		// Clear the buffers too
		delete m_buffers[index];
	}

	m_connections.clear();
	m_buffers.clear();
}


//-----------------------------------------------------------------------------------------------
// CONSOLE COMMANDS
//-----------------------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------------------
// Command for sending a command to another RCS process
//
void Command_RemoteCommand(Command& cmd)
{
	std::string commandToExecute;
	cmd.GetParam("c", commandToExecute);

	if (commandToExecute.size() == 0)
	{
		ConsoleErrorf("No command specified for remote command");
		return;
	}

	int connectionIndex = 0;
	cmd.GetParam("i", connectionIndex, &connectionIndex);

	bool sent = RemoteCommandService::Send(commandToExecute, connectionIndex, false);

	if (sent)
	{
		ConsolePrintf(Rgba::GREEN, "Command \"%s\" sent to connection %i", commandToExecute.c_str(), connectionIndex);
	}
	else
	{
		ConsoleErrorf("Couldn't sent command %s to connection %i", commandToExecute.c_str(), connectionIndex);
	}
}


//-----------------------------------------------------------------------------------------------
// Command for broadcasting another console command to all different RCS processes
//
void Command_RemoteCommandBroadcast(Command& cmd)
{
	std::string commandToExecute;
	cmd.GetParam("c", commandToExecute);

	if (commandToExecute.size() == 0)
	{
		ConsoleErrorf("No command specified for remote command");
		return;
	}

	int connectionCount = RemoteCommandService::GetConnectionCount();
	for (int connectionIndex = 0; connectionIndex < connectionCount; ++connectionIndex)
	{
		bool sent = RemoteCommandService::Send(commandToExecute, connectionIndex, false);

		if (sent)
		{
			ConsolePrintf(Rgba::GREEN, "Command \"%s\" sent to connection %i", commandToExecute.c_str(), connectionIndex);
		}
		else
		{
			ConsoleErrorf("Couldn't sent command %s to connection %i", commandToExecute.c_str(), connectionIndex);
		}
	}
}


//-----------------------------------------------------------------------------------------------
// Command for broadcasting another console command to all RCS processes, including this one
//
void Command_RemoteCommandAll(Command& cmd)
{
	Command_RemoteCommandBroadcast(cmd);

	std::string commandToExecute;
	cmd.GetParam("c", commandToExecute);

	if (commandToExecute.size() == 0)
	{
		ConsoleErrorf("No command specified for remote command");
		return;
	}

	// Stall, to ensure all other requests are fully sent (non-blocking sends)
	int64_t startHpc = GetPerformanceCounter();
	while (TimeSystem::PerformanceCountToSeconds(GetPerformanceCounter() - startHpc) < 1.0f)
	{}

	// Run the command on this program
	Command::Run(commandToExecute);
}


//-----------------------------------------------------------------------------------------------
// Command to have the current RCS join another RCS process
//
void Command_RemoteJoin(Command& cmd)
{
	std::string address;
	cmd.GetParam("a", address);

	if (address.size() == 0)
	{
		ConsoleErrorf("No address specified");
	}

	ConsolePrintf("Attempting to join address %s...", address.c_str());
	RemoteCommandService::Join(address);
}


//-----------------------------------------------------------------------------------------------
// Command to have the current RCS host a session
//
void Command_RemoteHost(Command& cmd)
{
	uint16_t port = DEFAULT_SERVICE_PORT;
	cmd.GetParam("p", port, &port);

	RemoteCommandService::Host(port);
}


//-----------------------------------------------------------------------------------------------
// Clones the given process (actual game instance)
//
void Command_CloneProcess(Command& cmd)
{
	// Get the executable path
	char path[1024];

	::GetModuleFileName(NULL, (LPWSTR) path, 1024);

	int numClones = 1;
	cmd.GetParam("c", numClones, &numClones);

	int createdCount = 0;
	for (int i = 0; i < numClones; ++i)
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		memset(&pi, 0, sizeof(pi));
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);

		bool success = ::CreateProcess(
			NULL,			// No module name (use command line)
			(LPWSTR)path,	// Command line
			NULL,			// Process handle not inheritable
			NULL,			// Thread handle not inheritable
			FALSE,			// Set handle inheritance to FALSE
			0,				// No creation flags
			NULL,			// Use parent's environment block
			NULL,			// Use parent's starting directory 
			&si,			// Pointer to STARTUPINFO structure
			&pi				// Pointer to PROCESS_INFORMATION structure
		);

		if (success)
		{
			createdCount++;
		}
	}

	if (createdCount == numClones)
	{
		ConsolePrintf(Rgba::GREEN, "Created %i clones.", createdCount);
	}
	else if (createdCount > 0)
	{
		ConsoleWarningf("Could only create %i clones.", createdCount);
	}
	else
	{
		ConsoleErrorf("Couldn't create any clones.");
	}
}


//-----------------------------------------------------------------------------------------------
// Echo callback for sending an echo response after running a command
//
void SendEchoResponse(ConsoleOutputText text, void* args)
{
	int connectionIndex = *((int*)args);

	// Check first if the text was from this thread (main thread)
	int currentThreadID = ::GetCurrentThreadId();

	if (text.m_threadID != currentThreadID)
	{
		return;
	}

	RemoteCommandService::Send(text.m_text, connectionIndex, true);
}
