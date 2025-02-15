/*
Copyright (c) 2017, Michael Kazhdan
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer. Redistributions in binary form must reproduce
the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution. 

Neither the name of the Johns Hopkins University nor the names of its contributors
may be used to endorse or promote products derived from this software without specific
prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/
#ifndef MY_MISCELLANY_INCLUDED
#define MY_MISCELLANY_INCLUDED

#include <iostream>
#include <sstream>

//////////////////
// OpenMP Stuff //
//////////////////
#ifdef _OPENMP
#include <omp.h>
#endif // _OPENMP

////////////////////////////
// Formatted float output //
////////////////////////////
struct StreamFloatPrecision
{
	StreamFloatPrecision( std::ostream &str , unsigned int precision , bool scientific=false ) : _str(str)
	{
		_defaultPrecision = (int)_str.precision();
		_str.precision( precision );
		if( scientific ) _str << std::scientific;
		else             _str << std::fixed;
	}
	~StreamFloatPrecision( void )
	{
		_str << std::defaultfloat;
		_str.precision( _defaultPrecision );
	}
protected:
	int _defaultPrecision;
	std::ostream &_str;
};

////////////////
// Time Stuff //
////////////////
#include <string.h>

inline double Time( void )
{
	return 0; //Removed as platform dependent
}

#include <cstdio>
#include <ctime>
#include <chrono>
struct Timer
{
	Timer( void ){ _startCPUClock = std::clock() , _startWallClock = std::chrono::system_clock::now(); }
	double cpuTime( void ) const{ return (std::clock() - _startCPUClock) / (double)CLOCKS_PER_SEC; };
	double wallTime( void ) const{ std::chrono::duration<double> diff = (std::chrono::system_clock::now() - _startWallClock) ; return diff.count(); }
	std::string operator()( bool showCpuTime , unsigned int precision=1 ) const
	{
		std::stringstream ss;
		StreamFloatPrecision sfp( ss , precision );
		ss << wallTime() << " (s)";
		if( showCpuTime ) ss << " / " << cpuTime() << " (s)";
		return ss.str();
	}
	friend std::ostream &operator << ( std::ostream &os , const Timer &timer ){ return os << timer(false); }
protected:
	std::clock_t _startCPUClock;
	std::chrono::time_point< std::chrono::system_clock > _startWallClock;
};

///////////////
// I/O Stuff //
///////////////
#if defined( _WIN32 ) || defined( _WIN64 )
const char FileSeparator = '\\';
#else // !_WIN
const char FileSeparator = '/';
#endif // _WIN

#ifndef SetTempDirectory
#if defined( _WIN32 ) || defined( _WIN64 )
#define SetTempDirectory( tempDir , sz ) GetTempPath( (sz) , (tempDir) )
#else // !_WIN32 && !_WIN64
#define SetTempDirectory( tempDir , sz ) if( std::getenv( "TMPDIR" ) ) strcpy( tempDir , std::getenv( "TMPDIR" ) );
#endif // _WIN32 || _WIN64
#endif // !SetTempDirectory

#if defined( _WIN32 ) || defined( _WIN64 )
#include <io.h>
inline void FSync( FILE *fp )
{
	//	FlushFileBuffers( (HANDLE)_fileno( fp ) );
	_commit( _fileno( fp ) );
}
#else // !_WIN32 && !_WIN64
#include <unistd.h>
inline void FSync( FILE *fp )
{
	fsync( fileno( fp ) );
}
#endif // _WIN32 || _WIN64

/////////////////////////////////////
// Exception, Warnings, and Errors //
/////////////////////////////////////
#include <exception>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
namespace MKExceptions
{
	template< typename ... Arguments > void _AddToMessageStream( std::stringstream &stream , Arguments ... arguments );
	inline void _AddToMessageStream( std::stringstream &stream ){ return; }
	template< typename Argument , typename ... Arguments > void _AddToMessageStream( std::stringstream &stream , Argument argument , Arguments ... arguments )
	{
		stream << argument;
		_AddToMessageStream( stream , arguments ... );
	}

	template< typename ... Arguments >
	std::string MakeMessageString( std::string header , std::string fileName , int line , std::string functionName , Arguments ... arguments )
	{
		size_t headerSize = header.size();
		std::stringstream stream;

		// The first line is the header, the file name , and the line number
		stream << header << " " << fileName << " (Line " << line << ")" << std::endl;

		// Inset the second line by the size of the header and write the function name
		for( size_t i=0 ; i<=headerSize ; i++ ) stream << " ";
		stream << functionName << std::endl;

		// Inset the third line by the size of the header and write the rest
		for( size_t i=0 ; i<=headerSize ; i++ ) stream << " ";
		_AddToMessageStream( stream , arguments ... );

		return stream.str();
	}
	struct Exception : public std::exception
	{
		const char *what( void ) const noexcept { return _message.c_str(); }
		template< typename ... Args >
		Exception( const char *fileName , int line , const char *functionName , const char *format , Args ... args )
		{
			_message = MakeMessageString( "[EXCEPTION]" , fileName , line , functionName , format , args ... );
		}
	private:
		std::string _message;
	};

	template< typename ... Args > void Throw( const char *fileName , int line , const char *functionName , const char *format , Args ... args ){ throw Exception( fileName , line , functionName , format , args ... ); }
	template< typename ... Args >
	void Warn( const char *fileName , int line , const char *functionName , const char *format , Args ... args )
	{
		std::cerr << MakeMessageString( "[WARNING]" , fileName , line , functionName , format , args ... ) << std::endl;
	}
	template< typename ... Args >
	void ErrorOut( const char *fileName , int line , const char *functionName , const char *format , Args ... args )
	{
		std::cerr << MakeMessageString( "[ERROR]" , fileName , line , functionName , format , args ... ) << std::endl;
		exit( 0 );
	}
}
#ifndef WARN
#define WARN( ... ) MKExceptions::Warn( __FILE__ , __LINE__ , __FUNCTION__ , __VA_ARGS__ )
#endif // WARN
#ifndef WARN_ONCE
#define WARN_ONCE( ... ) { static bool firstTime = true ; if( firstTime ) MKExceptions::Warn( __FILE__ , __LINE__ , __FUNCTION__ , __VA_ARGS__ ) ; firstTime = false; }
#endif // WARN_ONCE
#ifndef THROW
#define THROW( ... ) MKExceptions::Throw( __FILE__ , __LINE__ , __FUNCTION__ , __VA_ARGS__ )
#endif // THROW
#ifndef ERROR_OUT
#define ERROR_OUT( ... ) MKExceptions::ErrorOut( __FILE__ , __LINE__ , __FUNCTION__ , __VA_ARGS__ )
#endif // ERROR_OUT

struct FunctionCallNotifier
{
	FunctionCallNotifier( std::string str ) : _str(str)
	{
		_depth = _Depth++;
		for( unsigned int i=0 ; i<_depth ; i++ ) std::cout << "  ";
		std::cout << "[START] " << _str << std::endl;
	};
	~FunctionCallNotifier( void )
	{
		for( unsigned int i=0 ; i<_depth ; i++ ) std::cout << "  ";
		std::cout << "[END] " << _str << std::endl;
		_Depth--;
	}
protected:
	static unsigned int _Depth;
	unsigned int _depth;
	std::string _str;
};
unsigned int FunctionCallNotifier::_Depth = 0;
#define FUNCTION_NOTIFY FunctionCallNotifier ___myFunctionCallNotifier___( __FUNCTION__ )

#include <signal.h>
#if defined(_WIN32) || defined( _WIN64 )
#else // !WINDOWS
#include <execinfo.h>
#include <unistd.h>
#include <cxxabi.h>
#include <mutex>
#endif // WINDOWS
struct StackTracer
{
	static const char *exec;
	static void Trace( void )
	{
	} // Removed as platform dependent
};
const char *StackTracer::exec;

inline void SignalHandler( int signal )
{
	printf( "Signal: %d\n" , signal );
	StackTracer::Trace();
	exit( 0 );
};


template< typename Value > bool SetAtomic( volatile Value *value , Value newValue , Value oldValue );
template< typename Data > void AddAtomic( Data& a , Data b );

//////////////////////////////////
// File-backed streaming memory //
//////////////////////////////////
#include "Array.h"
class FileBackedReadWriteStream
{
public:
	struct FileDescription
	{
		FILE *fp;
		char fileName[2048];

		FileDescription( void ) : fp(NULL) { fileName[0] = 0; }
		FileDescription( const FileDescription &fd ) : fp(fd.fp) { strcpy( fileName , fd.fileName ); }
		FileDescription( FILE *fp ) : fp(fp) { fileName[0] = 0; }
		FileDescription( const char *fileHeader ) : fp(NULL)
		{
			if( fileHeader && strlen(fileHeader) ) sprintf( fileName , "%sXXXXXX" , fileHeader );
			else strcpy( fileName , "XXXXXX" );
#ifdef _WIN32
			_mktemp( fileName );
			fp = fopen( fileName , "w+b" );
#else // !_WIN32
			fp = fdopen( mkstemp( fileName ) , "w+b" );
#endif // _WIN32
			if( !fp ) ERROR_OUT( "Failed to open file: " , fileName );
		}
		void remove( void ){ if( fp ){ fclose( fp ) ; fp = NULL ; std::remove( fileName ); } }
	};

	FileBackedReadWriteStream( const char* fileHeader="" ) : _fd( fileHeader ) , _fileHandleOwner(true) {}
	FileBackedReadWriteStream( FILE *fp ) : _fd(fp) , _fileHandleOwner(false) {}
	~FileBackedReadWriteStream( void ){ if( _fileHandleOwner ) _fd.remove(); }
	bool write( ConstPointer(char) data , size_t size ){ return fwrite( data , sizeof(char) , size , _fd.fp )==size; }
	bool read( Pointer(char) data , size_t size ){ return fread( data , sizeof(char) , size , _fd.fp )==size; }
	void reset( void ){ fseek( _fd.fp , 0 , SEEK_SET ); }
protected:
	bool _fileHandleOwner;
	FileDescription _fd;
};



////////////////////
// MKThread Stuff //
////////////////////
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <future>

#include <memory>

struct ThreadPool
{
	enum ParallelType
	{
#ifdef _OPENMP
		OPEN_MP ,
#endif // _OPENMP
		THREAD_POOL ,
		ASYNC ,
		NONE
	};
	static const std::vector< std::string > ParallelNames;

	enum ScheduleType
	{
		STATIC ,
		DYNAMIC
	};
	static const std::vector< std::string > ScheduleNames;

	static size_t DefaultChunkSize;
	static ScheduleType DefaultSchedule;

	template< typename ... Functions >
	static void ParallelSections( const Functions & ... functions )
	{
		std::vector< std::future< void > > futures( sizeof...(Functions) );
		_ParallelSections( &futures[0] , functions ... );
		for( size_t t=0 ; t<futures.size() ; t++ ) futures[t].get();
	}

	static void Parallel_for( size_t begin , size_t end , const std::function< void ( unsigned int , size_t ) > &iterationFunction , ScheduleType schedule=DefaultSchedule , size_t chunkSize=DefaultChunkSize )
	{
		if( begin>=end ) return;
		size_t range = end - begin;
		size_t chunks = ( range + chunkSize - 1 ) / chunkSize;
		unsigned int threads = (unsigned int)NumThreads();
		std::atomic< size_t > index;
		index.store( 0 );


		if( range<chunkSize || _ParallelType==NONE || threads==1 )
		{
			for( size_t i=begin ; i<end ; i++ ) iterationFunction( 0 , i );
			return;
		}

		auto _ChunkFunction = [ &iterationFunction , begin , end , chunkSize ]( unsigned int thread , size_t chunk )
		{
			const size_t _begin = begin + chunkSize*chunk;
			const size_t _end = std::min< size_t >( end , _begin+chunkSize );
			for( size_t i=_begin ; i<_end ; i++ ) iterationFunction( thread , i );
		};
		auto _StaticThreadFunction = [ &_ChunkFunction , chunks , threads ]( unsigned int thread )
		{
			for( size_t chunk=thread ; chunk<chunks ; chunk+=threads ) _ChunkFunction( thread , chunk );
		};
		auto _DynamicThreadFunction = [ &_ChunkFunction , chunks , &index ]( unsigned int thread )
		{
			size_t chunk;
			while( ( chunk=index.fetch_add(1) )<chunks ) _ChunkFunction( thread , chunk );
		};

		if     ( schedule==STATIC  ) _ThreadFunction = _StaticThreadFunction;
		else if( schedule==DYNAMIC ) _ThreadFunction = _DynamicThreadFunction;

		if( false ){}
#ifdef _OPENMP
		else if( _ParallelType==OPEN_MP )
		{
			if( schedule==STATIC )
#pragma omp parallel for num_threads( threads ) schedule( static , 1 )
				for( int c=0 ; c<chunks ; c++ ) _ChunkFunction( omp_get_thread_num() , c );
			else if( schedule==DYNAMIC )
#pragma omp parallel for num_threads( threads ) schedule( dynamic , 1 )
				for( int c=0 ; c<chunks ; c++ ) _ChunkFunction( omp_get_thread_num() , c );
		}
#endif // _OPENMP
		else if( _ParallelType==ASYNC )
		{
			static std::vector< std::future< void > > futures;
			futures.resize( threads-1 );
			for( unsigned int t=1 ; t<threads ; t++ ) futures[t-1] = std::async( std::launch::async , _ThreadFunction , t );
			_ThreadFunction( 0 );
			for( unsigned int t=1 ; t<threads ; t++ ) futures[t-1].get();
		}
		else if( _ParallelType==THREAD_POOL )
		{
			unsigned int targetTasks = 0;
			if( !SetAtomic( &_RemainingTasks , threads-1 , targetTasks ) )
			{
				WARN( "nested for loop, reverting to serial" );
				for( size_t i=begin ; i<end ; i++ ) iterationFunction( 0 , i );
			}
			else
			{
				_WaitingForWorkOrClose.notify_all();
				{
					std::unique_lock< std::mutex > lock( _Mutex );
					_DoneWithWork.wait( lock , [&]( void ){ return _RemainingTasks==0; } );
				}
			}
		}
	}

	static unsigned int NumThreads( void ){ return (unsigned int)_Threads.size()+1; }

	static void Init( ParallelType parallelType , unsigned int numThreads=std::thread::hardware_concurrency() )
	{
		_ParallelType = parallelType;
		if( _Threads.size() && !_Close )
		{
			_Close = true;
			_WaitingForWorkOrClose.notify_all();
			for( unsigned int t=0 ; t<_Threads.size() ; t++ ) _Threads[t].join();
		}
		_Close = true;
		numThreads--;
		_Threads.resize( numThreads );
		if( _ParallelType==THREAD_POOL )
		{
			_RemainingTasks = 0;
			_Close = false;
			for( unsigned int t=0 ; t<numThreads ; t++ ) _Threads[t] = std::thread( _ThreadInitFunction , t );
		}
	}
	static void Terminate( void )
	{
		if( _Threads.size() && !_Close )
		{
			_Close = true;
			_WaitingForWorkOrClose.notify_all();
			for( unsigned int t=0 ; t<_Threads.size() ; t++ ) _Threads[t].join();
			_Threads.resize( 0 );
		}
	}
private:
	ThreadPool( const ThreadPool & ){}
	ThreadPool &operator = ( const ThreadPool & ){ return *this; }

	template< typename Function >
	static void _ParallelSections( std::future< void > *futures , const Function &function ){ *futures = std::async( std::launch::async , function ); }
	template< typename Function , typename ... Functions >
	static void _ParallelSections( std::future< void > *futures , const Function &function , const Functions& ... functions )
	{
		*futures = std::async( std::launch::async , function );
		_ParallelSections( futures+1 , functions ... );
	}
	static void _ThreadInitFunction( unsigned int thread )
	{
		// Wait for the first job to come in
		std::unique_lock< std::mutex > lock( _Mutex );
		_WaitingForWorkOrClose.wait( lock );
		while( !_Close )
		{
			lock.unlock();
			// do the job
			_ThreadFunction( thread );

			// Notify and wait for the next job
			lock.lock();
			_RemainingTasks--;
			if( !_RemainingTasks ) _DoneWithWork.notify_all();
			_WaitingForWorkOrClose.wait( lock );
		}
	}

	static bool _Close;
	static volatile unsigned int _RemainingTasks;
	static std::mutex _Mutex;
	static std::condition_variable _WaitingForWorkOrClose , _DoneWithWork;
	static std::vector< std::thread > _Threads;
	static std::function< void ( unsigned int ) > _ThreadFunction;
	static ParallelType _ParallelType;
};

size_t ThreadPool::DefaultChunkSize = 128;
ThreadPool::ScheduleType ThreadPool::DefaultSchedule = ThreadPool::DYNAMIC;
bool ThreadPool::_Close;
volatile unsigned int ThreadPool::_RemainingTasks;
std::mutex ThreadPool::_Mutex;
std::condition_variable ThreadPool::_WaitingForWorkOrClose;
std::condition_variable ThreadPool::_DoneWithWork;
std::vector< std::thread > ThreadPool::_Threads;
std::function< void ( unsigned int ) > ThreadPool::_ThreadFunction;
ThreadPool::ParallelType ThreadPool::_ParallelType;

const std::vector< std::string >ThreadPool::ParallelNames =
{
#ifdef _OPENMP
	"open mp" ,
#endif // _OPENMP
	"thread pool" ,
	"async" ,
	"none"
};
const std::vector< std::string >ThreadPool::ScheduleNames = { "static" , "dynamic" };

#include <mutex>

#if defined( _WIN32 ) || defined( _WIN64 )
#include <windows.h>
#endif // _WIN32 || _WIN64
template< typename Value >
bool SetAtomic32( volatile Value *value , Value newValue , Value oldValue )
{
#if defined( _WIN32 ) || defined( _WIN64 )
	long *_oldValue = (long *)&oldValue;
	long *_newValue = (long *)&newValue;
	return InterlockedCompareExchange( (long*)value , *_newValue , *_oldValue )==*_oldValue;
#else // !_WIN32 && !_WIN64
	uint32_t *_newValue = (uint32_t *)&newValue;
	return __atomic_compare_exchange_n( (uint32_t *)value , (uint32_t *)&oldValue , *_newValue , false , __ATOMIC_SEQ_CST , __ATOMIC_SEQ_CST );
#endif // _WIN32 || _WIN64
}
template< typename Value >
bool SetAtomic64( volatile Value *value , Value newValue , Value oldValue )
{
#if defined( _WIN32 ) || defined( _WIN64 )
	__int64 *_oldValue = (__int64 *)&oldValue;
	__int64 *_newValue = (__int64 *)&newValue;
	return InterlockedCompareExchange64( (__int64*)value , *_newValue , *_oldValue )==*_oldValue;
#else // !_WIN32 && !_WIN64
	uint64_t *_newValue = (uint64_t *)&newValue;
	return __atomic_compare_exchange_n( (uint64_t *)value , (uint64_t *)&oldValue , *_newValue , false , __ATOMIC_SEQ_CST , __ATOMIC_SEQ_CST );
#endif // _WIN32 || _WIN64
}

template< typename Number >
void AddAtomic32( volatile Number &a , Number b )
{
#if defined( _WIN32 ) || defined( _WIN64 )
	Number current = a;
	Number sum = current+b;
	long *_current = (long *)&current;
	long *_sum = (long *)&sum;
	while( InterlockedCompareExchange( (long*)&a , *_sum , *_current )!=*_current ) current = a , sum = a+b;
#else // !_WIN32 && !_WIN64
	Number current = a;
	Number sum = current+b;
	uint32_t *_current = (uint32_t *)&current;
	uint32_t *_sum = (uint32_t *)&sum;
	while( __sync_val_compare_and_swap( (uint32_t *)&a , *_current , *_sum )!=*_current ) current = a , sum = a+b;
#endif // _WIN32 || _WIN64
}

template< typename Number >
void AddAtomic64( volatile Number &a , Number b )
{
#if 1
	Number current = a;
	Number sum = current+b;
	while( !SetAtomic64( &a , sum , current ) ) current = a , sum = a+b;
#else
#if defined( _WIN32 ) || defined( _WIN64 )
	Number current = a;
	Number sum = current+b;
	__int64 *_current = (__int64 *)&current;
	__int64 *_sum = (__int64 *)&sum;
	while( InterlockedCompareExchange64( (__int64*)&a , *_sum , *_current )!=*_current ) current = a , sum = a+b;
#else // !_WIN32 && !_WIN64
	Number current = a;
	Number sum = current+b;
	uint64_t *_current = (uint64_t *)&current;
	uint64_t *_sum = (uint64_t *)&sum;
	while( __sync_val_compare_and_swap( (uint64_t *)&a , *_current , *_sum )!=*_current ) current = a , sum = a+b;
#endif // _WIN32 || _WIN64
#endif
}

template< typename Value >
bool SetAtomic( volatile Value *value , Value newValue , Value oldValue )
{
	switch( sizeof(Value) )
	{
	case 4: return SetAtomic32( value , newValue , oldValue );
	case 8: return SetAtomic64( value , newValue , oldValue );
	default:
		WARN_ONCE( "should not use this function: " , sizeof(Value) );
		static std::mutex setAtomicMutex;
		std::lock_guard< std::mutex > lock( setAtomicMutex );
		if( *value==oldValue ){ *value = newValue ; return true; }
		else return false;
	}
}

template< typename Data >
void AddAtomic( Data& a , Data b )
{
	switch( sizeof(Data) )
	{
	case 4: return AddAtomic32( a , b );
	case 8: return AddAtomic64( a , b );
	default:
		WARN_ONCE( "should not use this function: " , sizeof(Data) );
		static std::mutex addAtomicMutex;
		std::lock_guard< std::mutex > lock( addAtomicMutex );
		a += b;
	}
}

/////////////////////////
// NumberWrapper Stuff //
/////////////////////////
#include <vector>
struct EmptyNumberWrapperClass{};

template< typename Number , typename Type=EmptyNumberWrapperClass , size_t I=0 >
struct NumberWrapper
{
	typedef Number type;

	Number n;

	NumberWrapper( Number _n=0 ) : n(_n){}
	NumberWrapper operator + ( NumberWrapper _n ) const { return NumberWrapper( n + _n.n ); }
	NumberWrapper operator - ( NumberWrapper _n ) const { return NumberWrapper( n - _n.n ); }
	NumberWrapper operator * ( NumberWrapper _n ) const { return NumberWrapper( n * _n.n ); }
	NumberWrapper operator / ( NumberWrapper _n ) const { return NumberWrapper( n / _n.n ); }
	NumberWrapper &operator += ( NumberWrapper _n ){ n += _n.n ; return *this; }
	NumberWrapper &operator -= ( NumberWrapper _n ){ n -= _n.n ; return *this; }
	NumberWrapper &operator *= ( NumberWrapper _n ){ n *= _n.n ; return *this; }
	NumberWrapper &operator /= ( NumberWrapper _n ){ n /= _n.n ; return *this; }
	bool operator == ( NumberWrapper _n ) const { return n==_n.n; }
	bool operator != ( NumberWrapper _n ) const { return n!=_n.n; }
	bool operator <  ( NumberWrapper _n ) const { return n<_n.n; }
	bool operator >  ( NumberWrapper _n ) const { return n>_n.n; }
	bool operator <= ( NumberWrapper _n ) const { return n<=_n.n; }
	bool operator >= ( NumberWrapper _n ) const { return n>=_n.n; }
	NumberWrapper operator ++ ( int ) { NumberWrapper _n(n) ; n++ ; return _n; }
	NumberWrapper operator -- ( int ) { NumberWrapper _n(n) ; n-- ; return _n; }
	NumberWrapper &operator ++ ( void ) { n++ ; return *this; }
	NumberWrapper &operator -- ( void ) { n-- ; return *this; }
	explicit operator Number () const { return n; }
};

#if 0
template< typename Number , typename Type , size_t I >
struct std::atomic< NumberWrapper< Number , Type , I > >
{
	typedef Number type;

	std::atomic< Number > n;

	atomic( Number _n=0 ) : n(_n){}
	atomic( const std::atomic< Number > &_n ) : n(_n){}
	atomic( NumberWrapper< Number , Type , I > _n ) : n(_n.n){}
	atomic &operator = ( Number _n ){ n = _n ; return *this; }
//	atomic &operator = ( const atomic &a ){ n = a.n ; return *this; }
//	atomic &operator = ( const NumberWrapper< Number , Type , I > &_n ){ n = _n.n ; return *this; }
	atomic operator + ( atomic _n ) const { return atomic( n + _n.n ); }
	atomic operator - ( atomic _n ) const { return atomic( n * _n.n ); }
	atomic operator * ( atomic _n ) const { return atomic( n * _n.n ); }
	atomic operator / ( atomic _n ) const { return atomic( n / _n.n ); }
	atomic &operator += ( atomic _n ){ n += _n.n ; return *this; }
	atomic &operator -= ( atomic _n ){ n -= _n.n ; return *this; }
	atomic &operator *= ( atomic _n ){ n *= _n.n ; return *this; }
	atomic &operator /= ( atomic _n ){ n /= _n.n ; return *this; }
	bool operator == ( atomic _n ) const { return n==_n.n; }
	bool operator != ( atomic _n ) const { return n!=_n.n; }
	bool operator <  ( atomic _n ) const { return n<_n.n; }
	bool operator >  ( atomic _n ) const { return n>_n.n; }
	bool operator <= ( atomic _n ) const { return n<=_n.n; }
	bool operator >= ( atomic _n ) const { return n>=_n.n; }
	atomic operator ++ ( int ) { atomic _n(n) ; n++ ; return _n; }
	atomic operator -- ( int ) { atomic _n(n) ; n-- ; return _n; }
	atomic &operator ++ ( void ) { n++ ; return *this; }
	atomic &operator -- ( void ) { n-- ; return *this; }
	operator NumberWrapper< Number , Type , I >() const { return NumberWrapper< Number , Type , I >(n); }
	explicit operator Number () const { return n; }
};
#endif


namespace std
{
	template< typename Number , typename Type , size_t I >
	struct hash< NumberWrapper< Number , Type , I > >
	{
		size_t operator()( NumberWrapper< Number , Type , I > n ) const { return std::hash< Number >{}( n.n ); }
	};
}

template< typename Data , typename _NumberWrapper >
struct VectorWrapper : public std::vector< Data >
{
	VectorWrapper( void ){}
	VectorWrapper( size_t sz ) : std::vector< Data >( sz ){}
	VectorWrapper( size_t sz , Data d ) : std::vector< Data >( sz , d ){}

//	void resize( _NumberWrapper n )         { std::vector< Data >::resize( (size_t)(_NumberWrapper::type)n ); }
//	void resize( _NumberWrapper n , Data d ){ std::vector< Data >::resize( (size_t)(_NumberWrapper::type)n , d ); }

	typename std::vector< Data >::reference operator[]( _NumberWrapper n ){ return std::vector< Data >::operator[]( n.n ); }
	typename std::vector< Data >::const_reference operator[]( _NumberWrapper n ) const { return std::vector< Data >::operator[]( n.n ); }
};

//////////////////
// Memory Stuff //
//////////////////
size_t getPeakRSS( void );
size_t getCurrentRSS( void );

struct Profiler
{
	Profiler( unsigned int ms=0 )
	{
		_t = Time();
		_currentPeak = 0;
		if( ms )
		{
			_thread = std::thread( &Profiler::_updatePeakMemoryFunction , std::ref( *this ) , ms );
			_spawnedSampler = true;
		}
		else _spawnedSampler = false;
	}

	~Profiler( void )
	{
		if( _spawnedSampler )
		{
			{
				std::lock_guard< std::mutex > lock( _mutex );
				_terminate = true;
			}
			_thread.join();
		}
	}

	void reset( void )
	{
		_t = Time();
		if( _spawnedSampler )
		{
			std::lock_guard< std::mutex > lock( _mutex );
			_currentPeak = 0;
		}
		else _currentPeak = 0;
	}

	void update( void )
	{
		size_t currentPeak = getCurrentRSS();
		if( _spawnedSampler )
		{
			std::lock_guard< std::mutex > lock( _mutex );
			if( currentPeak>_currentPeak ) _currentPeak = currentPeak;
		}
		else if( currentPeak>_currentPeak ) _currentPeak = currentPeak;
	}

	std::string operator()( bool showTime=true ) const
	{
		std::stringstream ss;
		double dt = Time()-_t;
		double  localPeakMB = ( (double)_currentPeak )/(1<<20);
		double globalPeakMB = ( (double)getPeakRSS() )/(1<<20);
		{
			StreamFloatPrecision sfp( ss , 1 );
			if( showTime ) ss << dt << " (s), ";
			ss << localPeakMB << " (MB) / " << globalPeakMB << " (MB)";
		}
		return ss.str();
	}

	friend std::ostream &operator << ( std::ostream &os , const Profiler &profiler ){ return os << profiler(); }

protected:
	std::thread _thread;
	std::mutex _mutex;
	bool _spawnedSampler;
	double _t;
	volatile size_t _currentPeak;
	volatile bool _terminate;

	void _updatePeakMemoryFunction( unsigned int ms )
	{
		while( true )
		{
			std::this_thread::sleep_for( std::chrono::milliseconds( ms ) );
			update();
			if( _terminate ) return;
		}
	};
};

struct MemoryInfo
{
	static size_t Usage( void ){ return getCurrentRSS(); }
	static int PeakMemoryUsageMB( void ){ return (int)( getPeakRSS()>>20 ); }
};

#if defined( _WIN32 ) || defined( _WIN64 )
#include <Windows.h>
#include <Psapi.h>
inline void SetPeakMemoryMB( size_t sz )
{
	sz <<= 20;
	SIZE_T peakMemory = sz;
	HANDLE h = CreateJobObject( NULL , NULL );
	AssignProcessToJobObject( h , GetCurrentProcess() );

	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
	jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_JOB_MEMORY;
	jeli.JobMemoryLimit = peakMemory;
	if( !SetInformationJobObject( h , JobObjectExtendedLimitInformation , &jeli , sizeof( jeli ) ) ) WARN( "Failed to set memory limit" );
}
#else // !_WIN32 && !_WIN64
#include <sys/time.h> 
#include <sys/resource.h> 
inline void SetPeakMemoryMB( size_t sz )
{
	sz <<= 20;
	struct rlimit rl;
	getrlimit( RLIMIT_AS , &rl );
	rl.rlim_cur = sz;
	setrlimit( RLIMIT_AS , &rl );
}
#endif // _WIN32 || _WIN64

/*
* Author:  David Robert Nadeau
* Site:    http://NadeauSoftware.com/
* License: Creative Commons Attribution 3.0 Unported License
*          http://creativecommons.org/licenses/by/3.0/deed.en_US
*/

#if defined(_WIN32) || defined( _WIN64 )
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>

#endif

#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif





/**
* Returns the peak (maximum so far) resident set size (physical
* memory use) measured in bytes, or zero if the value cannot be
* determined on this OS.
*/
inline size_t getPeakRSS( )
{
#if defined(_WIN32)
	/* Windows -------------------------------------------------- */
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
	return (size_t)info.PeakWorkingSetSize;

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
	/* AIX and Solaris ------------------------------------------ */
	struct psinfo psinfo;
	int fd = -1;
	if ( (fd = open( "/proc/self/psinfo", O_RDONLY )) == -1 )
		return (size_t)0L;      /* Can't open? */
	if ( read( fd, &psinfo, sizeof(psinfo) ) != sizeof(psinfo) )
	{
		close( fd );
		return (size_t)0L;      /* Can't read? */
	}
	close( fd );
	return (size_t)(psinfo.pr_rssize * 1024L);

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
	/* BSD, Linux, and OSX -------------------------------------- */
	struct rusage rusage;
	getrusage( RUSAGE_SELF, &rusage );
#if defined(__APPLE__) && defined(__MACH__)
	return (size_t)rusage.ru_maxrss;
#else
	return (size_t)(rusage.ru_maxrss * 1024L);
#endif

#else
	/* Unknown OS ----------------------------------------------- */
	return (size_t)0L;          /* Unsupported. */
#endif
}





/**
* Returns the current resident set size (physical memory use) measured
* in bytes, or zero if the value cannot be determined on this OS.
*/
inline size_t getCurrentRSS( )
{
#if defined(_WIN32) || defined( _WIN64 )
	/* Windows -------------------------------------------------- */
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
	return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
	/* OSX ------------------------------------------------------ */
	struct mach_task_basic_info info;
	mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
	if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO,
		(task_info_t)&info, &infoCount ) != KERN_SUCCESS )
		return (size_t)0L;      /* Can't access? */
	return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	/* Linux ---------------------------------------------------- */
	long rss = 0L;
	FILE* fp = NULL;
	if ( (fp = fopen( "/proc/self/statm", "r" )) == NULL )
		return (size_t)0L;      /* Can't open? */
	if ( fscanf( fp, "%*s%ld", &rss ) != 1 )
	{
		fclose( fp );
		return (size_t)0L;      /* Can't read? */
	}
	fclose( fp );
	return (size_t)rss * (size_t)sysconf( _SC_PAGESIZE);

#else
	/* AIX, BSD, Solaris, and Unknown OS ------------------------ */
	return (size_t)0L;          /* Unsupported. */
#endif
}

#endif // MY_MISCELLANY_INCLUDED
