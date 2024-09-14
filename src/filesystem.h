#pragma once
#include <cstdio>
typedef void* FileHandle_t;

enum FileSystemSeek_t
{
	FILESYSTEM_SEEK_HEAD = SEEK_SET,
	FILESYSTEM_SEEK_CURRENT = SEEK_CUR,
	FILESYSTEM_SEEK_TAIL = SEEK_END,
};

class IBaseFileSystem
{
public:
	virtual int				Read(void* pOutput, int size, FileHandle_t file) = 0;
	virtual int				Write(void const* pInput, int size, FileHandle_t file) = 0;

	// if pathID is NULL, all paths will be searched for the file
	virtual FileHandle_t	Open(const char* pFileName, const char* pOptions, const char* pathID = 0) = 0;
	virtual void			Close(FileHandle_t file) = 0;


	virtual void			Seek(FileHandle_t file, int pos, FileSystemSeek_t seekType) = 0;
	virtual unsigned int	Tell(FileHandle_t file) = 0;
	virtual unsigned int	Size(FileHandle_t file) = 0;
	virtual unsigned int	Size(const char* pFileName, const char* pPathID = 0) = 0;

	virtual void			Flush(FileHandle_t file) = 0;
	virtual bool			Precache(const char* pFileName, const char* pPathID = 0) = 0;

	virtual bool			FileExists(const char* pFileName, const char* pPathID = 0) = 0;
	virtual bool			IsFileWritable(char const* pFileName, const char* pPathID = 0) = 0;
	virtual bool			SetFileWritable(char const* pFileName, bool writable, const char* pPathID = 0) = 0;

	virtual long			GetFileTime(const char* pFileName, const char* pPathID = 0) = 0;

	//--------------------------------------------------------
	// Reads/writes files to utlbuffers. Use this for optimal read performance when doing open/read/close
	//--------------------------------------------------------
	// virtual bool			ReadFile(const char* pFileName, const char* pPath, CUtlBuffer& buf, int nMaxBytes = 0, int nStartingByte = 0, FSAllocFunc_t pfnAlloc = NULL) = 0;
	// virtual bool			WriteFile(const char* pFileName, const char* pPath, CUtlBuffer& buf) = 0;
	// virtual bool			UnzipFile(const char* pFileName, const char* pPath, const char* pDestination) = 0;
	// not worth it
};
class ICommandLine
{
public:
	virtual void		CreateCmdLine(const char* commandline) = 0;
	virtual void		CreateCmdLine(int argc, char** argv) = 0;
	virtual const char* GetCmdLine(void) const = 0;

	// Check whether a particular parameter exists
	virtual	const char* CheckParm(const char* psz, const char** ppszValue = 0) const = 0;
	virtual void		RemoveParm(const char* parm) = 0;
	virtual void		AppendParm(const char* pszParm, const char* pszValues) = 0;

	// Returns the argument after the one specified, or the default if not found
	virtual const char* ParmValue(const char* psz, const char* pDefaultVal = 0) const = 0;
	virtual int			ParmValue(const char* psz, int nDefaultVal) const = 0;
	virtual float		ParmValue(const char* psz, float flDefaultVal) const = 0;

	// Gets at particular parameters
	virtual int			ParmCount() const = 0;
	virtual int			FindParm(const char* psz) const = 0;	// Returns 0 if not found.
	virtual const char* GetParm(int nIndex) const = 0;

	// copies the string passwed
	virtual void SetParm(int nIndex, char const* pNewParm) = 0;
};
extern IBaseFileSystem* g_pFileSystem;
bool InitFileSystem(const char* gamePath, const char* mapName);