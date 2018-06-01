/************************************************************************/
/* File: ShaderProgram.cpp
/* Author: Andrew Chase
/* Date: January 25th, 2018
/* Description: Implementation of the ShaderProgram class
/************************************************************************/
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/File.hpp"
#include "Engine/Renderer/glFunctions.hpp"
#include "Engine/Renderer/ShaderProgram.hpp"

const std::string ShaderProgram::INVALID_SHADER_NAME = "Invalid";
const std::string ShaderProgram::DEFAULT_SHADER_NAME = "Default";

//-----C functions declared here to ignore order-----

// Creating shaders
static GLuint	CreateShader(char const *filenameOrSource, GLenum type, bool isFilename);

// Shader error printing
static void		LogShaderError(GLuint shader_id, const std::string& filePath);
void			FormatAndPrintShaderError(const std::string& errorLog, const std::string& localFilePath);

// Program creation and error reporting
static GLuint	CreateAndLinkProgram( GLint vs, GLint fs );
static void		LogProgramError(GLuint program_id);

//-----------------------------------------------------------------------------------------------
// Default constructor - Do nothing, use LoadProgramFromFiles or LoadProgramFromSources to initialize
//
ShaderProgram::ShaderProgram()
{
}


//-----------------------------------------------------------------------------------------------
// Deletes the program from the GPU
//
ShaderProgram::~ShaderProgram()
{
	if (m_programHandle != NULL)
	{
		glDeleteProgram(m_programHandle);
	}
	m_programHandle = NULL;
}


//-----------------------------------------------------------------------------------------------
// Returns the GLuint handle for this program on the GPU
//
GLuint ShaderProgram::GetHandle() const
{
	return m_programHandle;
}


//-----------------------------------------------------------------------------------------------
// Returns the source file name of this shader, empty string denotes a built-in shader
//
const std::string& ShaderProgram::GetSourceFileName() const
{
	return m_sourceFilename;
}


//-----------------------------------------------------------------------------------------------
// Loads the shaders given by rootName, and compiles and links them into a shader program
//
bool ShaderProgram::LoadProgramFromFiles(const char *rootName)
{
	// If this Shader Program already has a program ID on the gpu, free the memory there before
	// attempting reload
	if (m_programHandle != NULL)
	{
		glDeleteProgram(m_programHandle);
		m_programHandle = NULL;
	}

	// Assign the file paths to this program
	std::string vsFilePath = rootName;
	vsFilePath += ".vs"; 

	std::string fsFilePath = rootName; 
	fsFilePath += ".fs"; 

	// Compile the two stages we're using 
	GLuint vert_shader = CreateShader(vsFilePath.c_str(), GL_VERTEX_SHADER, true); 
	GLuint frag_shader = CreateShader(fsFilePath.c_str(), GL_FRAGMENT_SHADER, true); 

	// Only if both compilations were successful do we bother linking them
	if (vert_shader != 0 && frag_shader != 0)
	{
		m_programHandle = CreateAndLinkProgram( vert_shader, frag_shader ); 
	}

	// Delete the shaders, we don't need them anymore
	glDeleteShader( vert_shader ); 
	glDeleteShader( frag_shader ); 

	m_sourceFilename = rootName;

	return (m_programHandle != NULL); 
}


//-----------------------------------------------------------------------------------------------
// Loads the shaders given by rootName, and compiles and links them into a shader program
//
bool ShaderProgram::LoadProgramFromSources(const char *vertexShaderSource, const char* fragmentShaderSource)
{
	// Compile the two stages we're using (all shaders will implement the vertex and fragment stages)
	// later on, we can add in more stages;
	GLuint vert_shader = CreateShader(vertexShaderSource, GL_VERTEX_SHADER, false); 
	GLuint frag_shader = CreateShader(fragmentShaderSource, GL_FRAGMENT_SHADER, false); 


	// Only if both compilations were successful do we bother linking them
	if (vert_shader != 0 && frag_shader != 0)
	{
		m_programHandle = CreateAndLinkProgram( vert_shader, frag_shader ); 
	}

	// Delete the shaders, we don't need them anymore
	glDeleteShader( vert_shader ); 
	glDeleteShader( frag_shader ); 

	return (m_programHandle != NULL);
}


//-----------------------------------------------------------------------------------------------
//------------------------------------------C Functions------------------------------------------

//-----------------------------------------------------------------------------------------------
// Takes a file, and creates an intermediary binary to be used in the linking process
//
static GLuint CreateShader(char const *filenameOrSource, GLenum type, bool isFileName)
{
	// Create a shader
	GLuint shader_id = glCreateShader(type);
	GUARANTEE_OR_DIE(shader_id != NULL, Stringf("Error: glCreateShader failed in CreateShaderFromFile."));

	// Bind source to it, and compile
	// If we're passed a file name, then load it from file
	if (isFileName)
	{
		char* src = (char*)FileReadToNewBuffer(filenameOrSource);
		GUARANTEE_OR_DIE(src != nullptr, Stringf("Error: File \"%s\" could not be found or opened.", filenameOrSource));

		GLint shader_length = (GLint)strlen(src);
		glShaderSource(shader_id, 1, &src, &shader_length);
		glCompileShader(shader_id);

		free(src);
	}
	// Otherwise treat the passed char* as a string source
	else
	{
		GLint shader_length = (GLint)strlen(filenameOrSource);
		glShaderSource(shader_id, 1, &filenameOrSource, &shader_length);
		glCompileShader(shader_id);
	}

	// Check status
	GLint status;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		LogShaderError(shader_id, filenameOrSource);
		glDeleteShader(shader_id);
		shader_id = NULL;
	}

	return shader_id;
}


//-----------------------------------------------------------------------------------------------
// Logs the shader error to std::out(?)
// If compilation fails, the driver will usually give us a log about why.  The formatting of this 
// log is up to the vendor, but most of us are on Nvidia cards, so it should be fine.  Just be aware
// that if you use a different vendor (Intel, AMD, etc) your errors may look different.
//
static void LogShaderError(GLuint shader_id, const std::string& localFilePath)
{
	// figure out how large the buffer needs to be
	GLint length;
	glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);

	// make a buffer, and copy the log to it. 
	char *buffer = new char[length + 1];
	glGetShaderInfoLog(shader_id, length, &length, buffer);

	// Print it out (may want to do some additional formatting)
	buffer[length] = NULL;

	// Log the error
	DebuggerPrintf("\n==============================================================================\n");

	if (localFilePath == "")
	{
		FormatAndPrintShaderError(buffer, "BUILT IN SHADER ERROR");
	}
	else
	{
		FormatAndPrintShaderError(buffer, localFilePath);
	}

	DebuggerPrintf("==============================================================================\n\n");

	// free up the memory we used. 
	delete buffer;
}


//-----------------------------------------------------------------------------------------------
// Parses the error log and makes a Visual Studio "Double-click to open" shortcut and prints it
// to the output pane
// Only does extra formatting if the error log is in Nvidia formatting, otherwise prints the
// original error log
//
void FormatAndPrintShaderError(const std::string& errorLog, const std::string& localFilePath)
{
	// Parsing the error data from Nvidia format
	size_t firstParenIndex	= errorLog.find_first_of('(');
	size_t lastParenIndex		= errorLog.find_first_of(')');
	size_t colonIndex			= errorLog.find_first_of(':');

	// Nvidia error log has (LINE_NUM) at the start of the error log, and a ':' starting the error message
	bool isNvidiaFormatting = (firstParenIndex != std::string::npos && lastParenIndex != std::string::npos && colonIndex != std::string::npos);

	// Get the full file path, is printed in either case
	std::string fullFilePath = GetFullFilePath(localFilePath);

	if (isNvidiaFormatting)
	{
		// Print with additional formatting
		std::string lineNum			= std::string(errorLog, firstParenIndex + 1, lastParenIndex - firstParenIndex - 1);
		std::string errorMessage	= std::string(errorLog, colonIndex);

		DebuggerPrintf( "SHADER ERROR on line %s of %s\n", lineNum.c_str(), localFilePath.c_str());
		DebuggerPrintf( "%s(%s)%s\n", fullFilePath.c_str(), lineNum.c_str(), errorMessage.c_str() ); // Use this specific format so Visual Studio users can double-click to jump to file-and-line of error
	}
	else
	{
		// Otherwise just print the original log if not in Nvidia format
		DebuggerPrintf( "SHADER ERROR in file %s\n", localFilePath.c_str());
		DebuggerPrintf(errorLog.c_str());
	}
}


//-----------------------------------------------------------------------------------------------
// Takes the binary shader objects and links them into a shader program
//
static GLuint CreateAndLinkProgram( GLint vs, GLint fs )
{
	// create the program handle - how you will reference
	// this program within OpenGL, like a texture handle
	GLuint program_id = glCreateProgram();
	GUARANTEE_OR_DIE( program_id != 0, "Error: Shader program could not be created");

	// Attach the shaders you want to use
	glAttachShader( program_id, vs );
	glAttachShader( program_id, fs );

	// Link the program (create the GPU program)
	glLinkProgram( program_id );

	// Check for link errors - usually a result
	// of incompatibility between stages.
	GLint link_status;
	glGetProgramiv(program_id, GL_LINK_STATUS, &link_status);

	if (link_status == GL_FALSE) {
		LogProgramError(program_id);
		glDeleteProgram(program_id);
		program_id = 0;
	} 

	// no longer need the shaders objects, you can detach them if you want
	// (not necessary)
	glDetachShader( program_id, vs );
	glDetachShader( program_id, fs );

	return program_id;
}


//-----------------------------------------------------------------------------------------------
// Logs program errors
//
static void LogProgramError(GLuint program_id)
{
	// get the buffer length
	GLint length;
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &length);

	// copy the log into a new buffer
	char *buffer = new char[length + 1];
	glGetProgramInfoLog(program_id, length, &length, buffer);

	// print it to the output pane
	buffer[length] = NULL;

	// Log the error (could reformat this later)
	DebuggerPrintf("PROGRAM ERROR: %s" , buffer);      

	// cleanup
	delete buffer;
}
