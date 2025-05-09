/*
* Copyright (C) 2016-2025, L-Acoustics

* This file is part of LA_networkInterfaceHelper.

* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:

*  - Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  - Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  - Neither the name of  nor the names of its contributors may be used to
*    endorse or promote products derived from this software without specific
*    prior written permission.

* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.

* You should have received a copy of the BSD 3-clause License
* along with LA_networkInterfaceHelper.  If not, see <https://opensource.org/licenses/BSD-3-Clause>.
*/

/**
* @file macOS_networkEventMonitor.mm
* @author Christophe Calmejane
* @brief macOS specific event monitoring sample (mainly for debugging purposes).
*/

#import <SystemConfiguration/SystemConfiguration.h>
#import <Foundation/Foundation.h>

#include <utility>

template<typename ObjectRef, typename DestroyType = CFTypeRef, void (*DestroyMethod)(DestroyType) = &CFRelease>
class RefGuard final
{
public:
	RefGuard() noexcept = default;

	explicit RefGuard(ObjectRef const ptr) noexcept
		: _ptr{ ptr }
	{
	}

	RefGuard& operator=(RefGuard&& other)
	{
		release();
		std::swap(_ptr, other._ptr);
		return *this;
	}

	~RefGuard() noexcept
	{
		release();
	}

	operator bool() const noexcept
	{
		return _ptr != nullptr;
	}

	ObjectRef operator*() const
	{
		if (_ptr == nullptr)
		{
			return nullptr;
		}
		return _ptr;
	}

	// Deleted compiler auto-generated methods
	RefGuard(RefGuard const&) = delete;
	RefGuard(RefGuard&&) = delete;
	RefGuard& operator=(RefGuard const&) = delete;

private:
	void release()
	{
		if (_ptr != nullptr)
		{
			DestroyMethod(_ptr);
			_ptr = nullptr;
		}
	}

	ObjectRef _ptr{ nullptr };
};

class IteratorGuard final
{
public:
	IteratorGuard() noexcept = default;

	explicit IteratorGuard(io_iterator_t const value) noexcept
		: _value{ value }
	{
	}

	IteratorGuard& operator=(IteratorGuard&& other)
	{
		release();
		std::swap(_value, other._value);
		return *this;
	}

	~IteratorGuard() noexcept
	{
		release();
	}

	operator bool() const noexcept
	{
		return _value != 0;
	}

	io_iterator_t* get() noexcept
	{
		return &_value;
	}

	io_iterator_t operator*() const noexcept
	{
		return _value;
	}

	// Deleted compiler auto-generated methods
	IteratorGuard(IteratorGuard const&) = delete;
	IteratorGuard(IteratorGuard&&) = delete;
	IteratorGuard& operator=(IteratorGuard const&) = delete;

private:
	void release()
	{
		if (_value != 0)
		{
			IOObjectRelease(_value);
			_value = 0;
		}
	}
	io_iterator_t _value{ 0 };
};

static void clearIterator(io_iterator_t iterator)
{
	@autoreleasepool
	{
		io_service_t service;
		while ((service = IOIteratorNext(iterator)))
		{
			IOObjectRelease(service);
		}
	}
}

static void onIONetworkControllerMatched(void* /* refcon */, io_iterator_t iterator) noexcept
{
	// Print the event to the console
	NSLog(@"Matched IONetworkController");

	clearIterator(iterator);
}

static void onIONetworkControllerTerminated(void* /* refcon */, io_iterator_t iterator) noexcept
{
	// Print the event to the console
	NSLog(@"Terminated IONetworkController");

	clearIterator(iterator);
}

static void onIONetworkInterfaceMatched(void* /* refcon */, io_iterator_t iterator) noexcept
{
	// Print the event to the console
	NSLog(@"Matched IONetworkInterface");

	clearIterator(iterator);
}

static void onIONetworkInterfaceTerminated(void* /* refcon */, io_iterator_t iterator) noexcept
{
	// Print the event to the console
	NSLog(@"Terminated IONetworkInterface");

	clearIterator(iterator);
}

// Main function
int main(int /* argc */, const char* /* argv */[])
{
	auto controllerMatchIterator = IteratorGuard{};
	auto controllerTerminateIterator = IteratorGuard{};
	auto interfaceMatchIterator = IteratorGuard{};
	auto interfaceTerminateIterator = IteratorGuard{};
	auto notificationPort = RefGuard<IONotificationPortRef, IONotificationPortRef, &IONotificationPortDestroy>{};
	auto masterPort = mach_port_t{ 0 };

	IOMasterPort(mach_task_self(), &masterPort);
	notificationPort = RefGuard<IONotificationPortRef, IONotificationPortRef, &IONotificationPortDestroy>{ IONotificationPortCreate(masterPort) };
	if (notificationPort)
	{
		IONotificationPortSetDispatchQueue(*notificationPort, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));

		IOServiceAddMatchingNotification(*notificationPort, kIOMatchedNotification, IOServiceMatching("IONetworkController"), onIONetworkControllerMatched, nullptr, controllerMatchIterator.get());
		IOServiceAddMatchingNotification(*notificationPort, kIOTerminatedNotification, IOServiceMatching("IONetworkController"), onIONetworkControllerTerminated, nullptr, controllerTerminateIterator.get());
		IOServiceAddMatchingNotification(*notificationPort, kIOMatchedNotification, IOServiceMatching("IONetworkInterface"), onIONetworkInterfaceMatched, nullptr, interfaceMatchIterator.get());
		IOServiceAddMatchingNotification(*notificationPort, kIOTerminatedNotification, IOServiceMatching("IONetworkInterface"), onIONetworkInterfaceTerminated, nullptr, interfaceTerminateIterator.get());

		// Clear the iterators discarding already discovered adapters, we'll manually list them anyway
		clearIterator(*controllerMatchIterator);
		clearIterator(*controllerTerminateIterator);
		clearIterator(*interfaceMatchIterator);
		clearIterator(*interfaceTerminateIterator);
	}

	// Wait for a key press
	NSLog(@"Press any key to exit...");
	while (true)
	{
		if (getchar() != EOF)
		{
			break;
		}
	}
	return 0;
}
